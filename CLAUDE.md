# CLAUDE.md — KFS (Kernel From Scratch, 42)

Context for Claude sessions on this project. Read this before answering questions
about the kernel.

## What this is

42's **Kernel From Scratch** series. A freestanding 32-bit x86 kernel, booted by
GRUB via Multiboot, written in **C and assembly**.

**KFS-1 goal:** boot via GRUB, write to the screen, display `42`. That's it —
no processes, no syscalls, no filesystem.

**KFS-2 goal:** build a GDT with kernel + user code/data/stack entries, install it
at `0x00000800`, and write a human-friendly kernel-stack dumper. Bonus: a
minimal debug shell. *Current state: GDT working and verified; stack dump
working; shell in progress.*

**Series layout** (verified against the subject PDFs in this repo — use this,
not guesses, when reasoning about what to defer):

| # | Subject |
|---|---|
| KFS-1 | boot, VGA screen output |
| KFS-2 | GDT (Global Descriptor Table) |
| KFS-3 | memory / paging / kmalloc |
| KFS-4 | IDT, ISR/IRQ, keyboard, PIT |
| KFS-5 | processes: data structure, kinship, signals, sockets, rights, memory separation, multitasking; helpers for `fork`/`wait`/`_exit`/`getuid`/`signal`/`kill` |
| KFS-6 | filesystem: ext2, IDE / IDE controller |
| KFS-7 | syscall table + syscall system, Unix environment, user accounts + passwords, IPC sockets, FS hierarchy |
| KFS-8 | kernel modules: register, load at boot, kernel↔module callbacks |
| KFS-9 | ELF parser + loader, `execve`-like syscall, runtime-loadable modules |
| KFS-X | final — make it a complete Unix system |

**There is no dedicated "TTY" subject.** Multi-screen support is purely a KFS-1
bonus. Its forward relevance is KFS-5 (each process gets its own output target)
and KFS-7 (`write()` to a file descriptor resolves to a terminal).

## Toolchain

| Tool | Choice | Notes |
|---|---|---|
| Compiler | `i686-elf-gcc` (cross-compiler, built from source) | installed to `~/opt/cross`, no root needed |
| Assembler | `i686-elf-as` (GAS, AT&T syntax) | came with binutils |
| Linker | host `ld` via gcc, with **our own** `linker.ld` | subject forbids using the host's `.ld` file |
| Emulator | QEMU (`qemu-system-i386`) | VirtualBox also works with the same ISO |
| Image | `grub-mkrescue` → `kfs.iso` | |
| Build env | **Docker** (`Dockerfile` in repo) | school machines lack the toolchain |

**Docker is the primary build path.** Source is mounted (`-v "$PWD":/kfs`), not
copied — edit on the host, compile in the container. QEMU runs on the **host**,
not in the container (no display in Docker).

```bash
docker build -t kfs-toolchain .                  # once, slow
docker run --rm -v "$PWD":/kfs kfs-toolchain     # runs make
qemu-system-i386 -cdrom kfs.iso                  # on the host
```

Author is on a **French (AZERTY) keyboard** — use `qemu-system-i386 -k fr -cdrom
kfs.iso` when testing keyboard input, otherwise QEMU's translation makes
scancode debugging confusing.

## Architecture facts (don't re-derive these)

- **Target is 32-bit (i386/i686) and that's mandatory.** GRUB's Multiboot hands
  off in 32-bit protected mode. Going 64-bit would require entering long mode,
  which requires paging — deliberately deferred to KFS-3.
- `i686-elf` **is** i386 architecture. `i686` = CPU generation baseline, `elf`
  where an OS name would go = freestanding, no OS.
- **Kernel loads at `0x100000`** (1 MB). Multiboot header must be in the first
  8 KB of the binary, hence its own `.multiboot` section placed first by
  `linker.ld`.
- **Multiboot flags = `0x3`** — bit 0 (`ALIGN`, page-align modules) and bit 1
  (`MEMINFO`, ask GRUB for the memory map). Neither has a visible effect yet;
  both are set for later projects. Checksum makes magic+flags+checksum == 0.
- **Stack lives in `.bss`**, 16 KB, `.align 16`, `%esp` set to `stack_top` in
  `boot.s` (stack grows downward). Static allocation — works today, no memory
  manager needed.
- **The `.align 16` on the stack is load-bearing**: the SysV i386 ABI requires
  `%esp` 16-byte aligned at every `call`. GCC maintains it after that; we only
  have to seed it correctly before `call kernel_main`. Same obligation applies
  in hand-written interrupt stubs (KFS-4).
- **Don't rely on `.bss` being zeroed** by GRUB. Initialize globals explicitly
  at runtime, or zero `.bss` in `boot.s`.
- **Static memory works; dynamic does not.** No `malloc`/heap until KFS-3.
  Anything fixed at compile time (globals, `.bss` arrays, the stack) is fine.
- **Freestanding headers are available** even under `-nostdlib`, as long as
  `-ffreestanding` is set: `stdint.h`, `stddef.h`, `stdbool.h`, `stdarg.h`.
  Never hand-roll `uint8_t` or a `va_list`.
- **cdecl calling convention.** GCC emits it and hand-written asm must match:
  arguments pushed on the stack (first at `4(%esp)` in a leaf function that
  pushes nothing, second at `8(%esp)`), return value in `eax`/`al`. 32-bit x86
  passes on the stack rather than in registers because it only has 8 GPRs, most
  of them spoken for; the 64-bit ABI switched to registers once there were 16.

### VGA text mode

- **Screen = memory-mapped at `0xB8000`.** 80×25 = 2000 cells, laid out row by
  row, no padding. Index math: `y * VGA_WIDTH + x`.
- **Each cell is one `uint16_t`**: low byte = character (code page 437, matches
  ASCII for 0x20–0x7E), high byte = attribute. Attribute splits into fg (bits
  11–8, 4 bits / 16 colours), bg (bits 14–12, 3 usable), blink (bit 15).
  Setting a "bright" background gets you blinking text instead — stick to the
  low 8 for backgrounds.
- Little-endian means `c | (color << 8)` lands the bytes in the order the
  hardware expects. No swapping needed.
- **Output needs no `outb` at all** — it's plain memory writes. Only the
  *cursor* uses ports, because it lives in a register, not in the framebuffer.

### CRT controller (the cursor)

- Ports `0x3D4` (index) / `0x3D5` (data). Classic **index/data pair**: write
  which internal register you mean to `0x3D4`, then its value to `0x3D5`. Used
  because the chip has ~25 registers and ISA port space was scarce. The PIC,
  PIT and CMOS use the same pattern.
- These are Motorola **MC6845** register numbers, unchanged since 1977. Modern
  machines emulate the interface; QEMU intercepts the `out` and updates a C
  struct. Program it as though the chip were there.
- **Position** — register `0x0E` (high byte) / `0x0F` (low byte) of the linear
  offset `y * VGA_WIDTH + x`. Two registers because each is 8 bits wide and the
  offset reaches 1999.
- **Shape** — register `0x0A` (start scanline) / `0x0B` (end scanline). These
  are *scanlines within one character cell*, not screen coordinates. Cells are
  16 px tall with the standard 8×16 font: `(14, 15)` = underline, `(0, 15)` =
  block. `start > end` is undefined-ish and can make the cursor vanish or split.
- Both registers pack other fields into the top bits, hence the read-modify-
  write: mask `0xC0` for `0x0A`, `0xE0` for `0x0B`. **Bit 5 of `0x0A` disables
  the cursor** — so masking with `0xC0` re-enables it as a side effect, and
  `vga_disable_cursor` is just writing `0x20`.
- Colour adapters are at `0x3D4`; mono (MDA/Hercules) at `0x3B4`. We hardcode
  `0x3D4`.
- Registers `0x0C`/`0x0D` are the **start address** — changing them scrolls the
  display without moving any framebuffer bytes. Unused; noted as a possible
  optimisation for `terminal_scroll`.

## Module layout and data flow

There are two stacks — one for output, one for input — but they are **not** two
independent columns joined only at the top. `shell_loop` sits above both: it
reads events from the keyboard stack and writes characters to the terminal
stack, in the same function. That is what a shell *is*, and pretending
otherwise produced a diagram that did not describe the code.

```
   OUTPUT                              INPUT
   printk       format                 kbd_buffer   ring buffer (the seam)
   terminal     state, scroll, cursor  keyboard     scancode → key_event_t
   vga          0xB8000 + CRTC         ps2          poll now, IRQ later
        \                                   /
         io.h  (inb/outb — the ONLY door to hardware)
        /                                   \
   VGA screen                          PS/2 keyboard

   shell_loop (src/shell/) reads INPUT and writes OUTPUT.
   kernel_main sets up the terminal, then hands control to it and never returns.
```

**Invariants that are actually true — do not break them:**

1. **The *drivers* never call each other.** `keyboard.c` does not know terminals
   exist; `terminal.c` does not know a keyboard exists. Everything that joins
   them is in `shell/`. This is what makes the Alt+F-key → screen-switch policy
   the shell's business (see Design: shell) and not the decoder's.
2. **Nothing above `io.h` runs an `in`/`out` instruction directly.** `io.h` is
   the only x86-specific code in the tree — with one deliberate exception,
   `shell_reboot`, which pulses the 8042 through `inb`/`outb` from `commands.c`.
3. **`ps2.c` is the only poll-specific file.** Everything above it is source-
   agnostic, which is the entire point (see Design: keyboard).
4. **Keep authoritative state in variables, never read the framebuffer back to
   make decisions.**

## Naming and style conventions

- **Prefix by layer**: `vga_*` touches the framebuffer/CRTC, `terminal_*`
  manages screen state, `printk*` formats, `kbd_*` decodes. Seeing an unprefixed
  or wrong-prefixed call is the signal a layer got skipped.
- **snake_case everywhere.** `terminal_set_color`, not `terminal_setcolor`.
- **Type suffixes are mixed and that's tolerated, not endorsed.** `terminal_t`,
  `key_event_t`, `printk_sink_t`, `line_state_t` use the OSDev/C `_t` suffix;
  `t_command` / `struct s_command` uses the 42 norme convention. If it ever gets
  unified, `_t` is the one to keep — the norme does not apply to KFS, and every
  reference the project reads from (OSDev, the Multiboot spec, libc) uses `_t`.
- **Empty parameter lists are `(void)`.** `f()` in C means "unspecified
  arguments" and disables call-site checking; `f(void)` means "none".
- **Functions not in a header are `static`.** Module-private helpers like the
  shell's `prompt`/`get_char`/`get_line` must not leak into the global namespace
  — there is one link namespace for the whole kernel and no way to shadow.
- **`x` is always the column, `y` is always the row, always in that order.**
  Never `(row, col)`.
- **`printk` is the name** — not `printf`. There's no userspace, no `FILE*`, no
  syscall boundary, so the `f` (formatted output to a *stream*) doesn't apply.
  Do not also define a `printf` alias; two names invites inconsistent use.
- libk functions keep their **standard libc names** unprefixed (`strlen`,
  `memcpy`) since they're reimplementations of known functions.
- `static` at file scope = internal linkage = module-private. C has no
  namespaces; this is the boundary mechanism. Internal helpers
  (`terminal_scroll`, `terminal_newline`) stay out of headers deliberately.
- Link through `i686-elf-gcc -T linker.ld ... -lgcc` (not raw `ld`) — `-lgcc`
  provides compiler helper routines (e.g. 64-bit division on 32-bit).
- **GAS label scope**, since asm has no block scope: plain labels are file-wide
  (forward references fine, GAS is two-pass); `.globl` exports to the linker and
  is the only way C can call in; `.L`-prefixed labels never reach the object
  file; numeric labels `1:`–`9:` are redefinable and referenced as `1f`
  (forward) / `1b` (backward). Use numeric labels for targets within eyeshot,
  `.L` for named-but-private, `.globl` only for real entry points.
- **GAS directive sizes on i686**: `.byte` 1, `.word`/`.short`/`.value` 2,
  `.long`/`.int` 4, `.quad` 8. These are target-dependent in GAS (unlike NASM's
  fixed `dw`/`dd`/`dq`) — safe here because the toolchain is pinned. `.space N` /
  `.skip N` emit N zero bytes. Pick one spelling per size and stay consistent;
  mixing makes descriptor layouts unreadable.

## Directory layout

This is the **actual** tree as of KFS-2 — verified, not aspirational:

```
kfs/
├── Makefile  Dockerfile  kfs-docker.sh  linker.ld  grub.cfg  .gitignore
├── include/        io.h  vga.h  terminal.h  printk.h  keyboard.h
│                   kbd_buffer.h  ps2.h  libk.h  debug.h  shell.h
└── src/
    ├── boot.s      kernel.c      gdt.s
    ├── drivers/    vga.c  terminal.c  ps2.c  keyboard.c  kbd_buffer.c
    ├── libk/       printk.c  strlen.c  strcmp.c  memset.c  memcpy.c  memmove.c
    ├── debug/      hexdump.c  stack.c
    └── shell/      shell_loop.c  commands.c
```

Directories are **by role, not by subsystem**: `drivers/` holds everything that
talks to a device (both the output and the input column), `libk/` the libc
reimplementations, `debug/` the inspection tools, `shell/` the KFS-2 bonus.
`gdt.s` sits at `src/` root next to `boot.s` because both are boot-time
machine setup, not a subsystem.

**libk is one function per file** (`strlen.c`, not a shared `string.c`) — the
same layout a real libc and 42's libft use, and it keeps the linker pulling in
only what is referenced.

**There is no `gdt.h`, `types.h` or `kernel.h`.** They were in an earlier draft
of this file; none has a caller. `gdt_init` is invoked from `boot.s`, so a C
prototype for it would be decoration, and the selector constants belong as
`.set` directives inside `gdt.s` where the code that uses them lives. Don't
create a header until something includes it.

Generated, gitignored: `build/`, `isodir/`, `kfs`, `kfs.iso`, `*.o`, `*.d`.
**Objects mirror the source tree under `build/`** (`build/src/drivers/vga.o`), so
the source tree stays clean and `make clean` is one `rm -rf`. `-MMD -MP` drops a
`.d` beside each `.o`, so editing a header rebuilds every file that includes it.
`isodir/` is pure scaffolding rebuilt by every `make`.

**The Makefile lists every source explicitly.** No wildcards — a new file is a
deliberate one-line edit, and the build never silently picks up a stray `.c`.

`memcpy`/`memset` are **not optional** — GCC emits calls to them on its own for
struct assignment and array init, even under `-ffreestanding`. Missing them
produces mysterious link errors. `memmove` is needed for scrolling (overlapping
regions).

## Design: `io.h`

Four `static inline` wrappers. `static inline` because: `inline` pastes the
single instruction at the call site (zero call overhead for something hit
thousands of times per second in the poll loop), `static` gives internal linkage
so a header-only function included in several `.c` files doesn't produce
duplicate-definition link errors.

- `volatile` on the asm is load-bearing — the whole point is a hardware side
  effect GCC cannot see, so without it an `outb` with an unused result can be
  optimised away entirely.
- Constraint `"a"` = accumulator (`AL`/`AX`/`EAX`); the ISA hardwires `in`/`out`
  to it, it's not a preference.
- Constraint `"Nd"` = "8-bit immediate **or** `DX`". Ports ≤ 255 get the compact
  immediate encoding; larger ones (e.g. `0x3D5`) go through `DX` automatically.
- `io_wait()` = `outb(0x80, 0)`. Port `0x80` is the BIOS POST diagnostic port,
  unused after boot; the write costs ~1 µs, giving a clock-independent delay for
  hardware that needs settling time between consecutive `out`s (notably the 8259
  PIC in KFS-4).

**Keep these inline in the header, not in a `.s` file.** Inlining is a compiler
operation — code in a separately-assembled `.s` file is an opaque symbol reached
by a real `call`, and the call overhead dwarfs the single instruction. `.s` files
are for real procedures that can't be one expression and don't need inlining:
`boot.s` now, the GDT flush in KFS-2, IDT load + interrupt stubs in KFS-4,
context switch in KFS-5.

## Design: terminal

**10 screens** (F1–F10), each a struct with its own full shadow buffer:

```c
typedef struct {
    uint16_t buffer[VGA_WIDTH * VGA_HEIGHT];
    size_t   x, y;      /* x = column, y = row */
    uint8_t  color;
} terminal_t;

static terminal_t  screens[TERMINAL_COUNT];   /* ~40 KB in .bss for 10 */
static terminal_t *active;
```

`terminal_current()` returns `active - screens`, the index. It exists so the
shell can key its per-terminal line state on the screen without the terminal
layer knowing what a line *is*.

**The struct holds screen state only.** No `line[]`, no `line_len`, no
`prompted` — those are the shell's (see Design: shell). A terminal knows what
characters are on it and where the cursor is, and nothing about what they mean.

**Every function writes to `active`.** There are no explicit-target
(`terminal_putchar_to(t, c)`) variants and no `terminal_put` choke point —
`terminal_putchar` writes `active->buffer` and calls `vga_put_entry`
unconditionally, side by side, in each of its branches. This is correct
*today* and only today: one screen is visible, the only writer is the human at
the keyboard, so "the active terminal" and "the visible terminal" are the same
thing and writing straight through to `0xB8000` is always right.

It stops being right at the **first background writer** — the PIT handler in
KFS-4, a background process in KFS-5. A write meant for screen 3 while screen 0
is on the monitor would scribble on the monitor. When that day comes the change
is mechanical and should be done in one go:

- split `active` into `visible` (what's on the monitor — a property of the
  machine, one global forever) and `focused` (where this caller's output goes —
  per-process in a real kernel; in KFS-5 it becomes `current->tty` and stops
  being a global at all);
- give every function that does real work an explicit `terminal_t *t` parameter,
  leaving the no-argument versions as one-line wrappers that read `focused`;
- funnel all framebuffer access through one choke point, so the visible check
  exists in exactly one place:

```c
static void terminal_put(terminal_t *t, size_t x, size_t y, char c) {
    t->buffer[y * VGA_WIDTH + x] = vga_entry(c, t->color);
    if (t == visible)                       /* NOT focused */
        vga_put_entry(x, y, c, t->color);
}
```

That conditional is the whole mechanism for background screens. The same rule
applies to the cursor: a background terminal advancing `x`/`y` must not move the
visible hardware cursor, which the current `vga_update_cursor` calls would do.

Public API as it stands: `terminal_initialize`, `terminal_putchar`,
`terminal_write(data, size)`, `terminal_writestring`, `terminal_set_color`,
`terminal_get_color`, `terminal_clear`, `terminal_switch`, `terminal_current`.
File-private: `terminal_scroll`, `terminal_newline`.

**`C_DEFAULT` lives in `vga.h`, not `terminal.h`.** `printk.h`'s log macros
(`printk_info` etc.) expand to it, and `printk.h` must be includable without
dragging in the terminal layer — otherwise `debug/` and any future serial sink
would depend on a terminal existing. It compiled before only by luck of include
order in the one file that used those macros.

`terminal_get_color` exists so callers can save/restore around a coloured
message instead of hardcoding a "return to" colour.

**Decisions taken deliberately** (an evaluator will probe these):
- **Eager wrap** — writing at column 79 moves to the next line immediately, so
  the cursor is never shown at column 79. Real terminals use deferred wrap
  (cursor parks past the last column). Eager is fine and simpler; deferred needs
  extra clamping because `x` can reach 80 and at `y=24` that indexes 2000.
- **Tab clamps, never wraps.** Next stop is `(x + TAB_WIDTH) & ~(TAB_WIDTH - 1)`
  with `TAB_WIDTH 8`; if that's ≥ 80 clamp to column 79. The wrap then happens on
  the *next printable character* via the normal check. Tabs write spaces
  (destructive); real terminals only move the cursor. Both defensible.
- **Backspace wraps up** to column 79 of the previous line and erases. Real `\b`
  only moves left and stops at column 0. Kept because it feels better in a bare
  echo loop — see Open threads for why it needs a boundary in KFS-2.
- `vga_enable_cursor` belongs in `terminal_initialize` (shape is set once), not
  in `terminal_switch` (position changes per switch).

## Design: `printk`

Sink-based so the formatter never learns what a terminal is:

```c
typedef struct {
    void    (*put)(char c);
    void    (*set_color)(uint8_t color);   /* may be NULL */
    uint8_t (*get_color)(void);            /* may be NULL */
} printk_sink_t;

void vprintk(const printk_sink_t *sink, const char *fmt, va_list ap);
```

A serial sink (very likely by KFS-4 for debugging — QEMU can log a serial console
to the host terminal) leaves the colour callbacks `NULL` and colour directives
are silently skipped. Same formatter, no changes.

Conversions: `%c %s %d %i %u %x %X %b %p %%`, plus **`%C`** which takes an
attribute byte and changes colour mid-string. `vprintk` saves the colour on entry
and restores it on exit, so colour changes don't leak between calls — which is
why the log macros use two `%C` (one to colour the tag, one to return to default).

- `%C` and `%c` must `va_arg(ap, int)`, never `uint8_t`/`char`. Anything smaller
  than `int` is promoted through `...`; reading the smaller type is UB.
- Negation uses `-(unsigned long)v`, not `-v`, so `INT_MIN` doesn't overflow.
- `##__VA_ARGS__` in the log macros is a GNU extension (eats the trailing comma
  when there are no varargs). Fine with `i686-elf-gcc`, not ISO C.

## Design: keyboard

**KFS-1 uses a poll loop, not interrupts.** (This reverses an earlier plan in
this file.) The rationale is that the poll/IRQ distinction is confined to exactly
one file, so KFS-4 is an addition rather than a rewrite:

```
ps2.c        reads the 8042 status/data ports   ← ONLY poll-specific file
keyboard.c   scancode → key_event_t             ← source-agnostic, survives
kbd_buffer.c ring buffer, head/tail             ← the producer/consumer seam
```

In KFS-4 the IRQ1 handler becomes the producer (calling the same `kbd_feed` +
`kbd_buf_push`), `ps2_poll` is deleted, and `kernel_main` becomes
`for(;;) __asm__("hlt");`. `keyboard.c`, `kbd_buffer.c` and the whole output
column are untouched.

**The decoder returns an event, not a `char`** — there is no character for F1,
and no way for a `char` to say "Alt was held":

```c
typedef struct {
    uint16_t code;   /* ASCII 1..127, or a KEY_* constant ≥ 0x100 */
    uint8_t  mods;   /* MOD_SHIFT | MOD_CTRL | MOD_ALT */
} key_event_t;

key_event_t kbd_feed(uint8_t sc);   /* code == 0 means "no event" */
```

`kbd_feed` returns the struct by value (4 bytes — cheaper than an out-parameter
on i386) and the ring buffer carries `key_event_t`. **`code == 0` is reserved as
"this scancode produced nothing"** — a modifier press, a release, a `0xE0`
prefix — so the consumer's loop is `while (!kbd_buf_pop(&ev)) ps2_poll();`.

Non-character keys start at **`0x100`**, above every ASCII value, so one
`uint16_t` distinguishes them without a tag field: `KEY_F1`–`KEY_F10` are
`0x100`–`0x109`, arrows are `0x110`–`0x113`. A consumer that only wants text
tests `ev.code < 0x100`, which is exactly what the shell's `get_line` does
before pushing to the line buffer.

This replaced an earlier scheme where `kbd_feed` returned a `char` and F1–F10
were smuggled through as *negative* values (`CHR_F1 = -10`). That worked — `char`
is signed on i686 — but it was a sign trick rather than a type, had no room for
modifiers at all, and made arrow keys and Ctrl+C unreachable.

**Modifiers are held in one `static uint8_t mods` in `keyboard.c`**, set on make
and cleared on break, and stamped into every event. Keeping them in the decoder
rather than in the event stream is deliberate: a modifier is a *level*, not an
edge, and a consumer reconstructing it from press/release events would desync
the first time the buffer overflowed.

Hardware facts:
- Data port `0x60`, status port `0x64`, bit 0 of status = output buffer full.
- Under GRUB/BIOS the 8042 delivers **translated scancode set 1** — no controller
  configuration needed in KFS-1.
- Bit 7 set (`| 0x80`) = key **release** (break code). `0xE0` prefix = extended
  key (arrows, right Ctrl, AltGr).
- Left Alt make `0x38` / break `0xB8`. F1–F10 are contiguous `0x3B`–`0x44`, so
  screen index is `code - KEY_F1`.
- **Caps Lock only flips letters.** `shift ^ caps` is wrong — it makes Caps Lock
  turn `1` into `!`.
- **AltGr sends `0xE0 0x38`** — the same make code as left Alt with an extended
  prefix. Folding it into `MOD_ALT` breaks Alt+F-keys the moment the author types
  `@` or `#` on their AZERTY keyboard. Extended-Alt is deliberately ignored.
- Ctrl+letter: `c -= 'a' - 1` gives `0x01`–`0x1A`. Needed in KFS-2 for Ctrl+C /
  Ctrl+L.

**Shortcut policy lives in `get_line` (`src/shell/shell_loop.c`)**, not in
`keyboard.c`: bare F1–F10 → `terminal_switch(ev.code - KEY_F1)`, and
`terminal_switch` bounds-checks `n >= TERMINAL_COUNT` itself.

The rule the policy's *location* enforces is "the decoder must not know
terminals exist" — screen switching is one possible meaning of F1, not the
meaning of F1. The rule it does **not** enforce is "policy belongs to
`kernel_main`": `kernel_main` initialises the terminal and calls `shell_loop`,
which never returns, so the shell owns the only loop that reads the keyboard.
Putting the policy anywhere else would mean inventing a dispatch layer with one
caller. If KFS-4/5 ever add a second consumer of the event stream, that is the
point to hoist it — not before.

**No Alt modifier is required.** The keys are bare F1–F10, which is why they can
be tested with a plain range check on `ev.code`. Alt + F-key is what a Linux
console uses because the F-keys themselves belong to applications there; nothing
in KFS needs them yet. If they ever do, the test becomes
`ev.mods & MOD_ALT`, and the `MOD_ALT` bit is already being tracked and
delivered — which is half the reason the modifier field exists now rather than
later.

## Design: GDT (KFS-2)

**Segmentation cannot be turned off on x86.** Every access is `SEG.base + offset`,
checked against `SEG.limit`, even in protected mode. In protected mode a segment
register holds a *selector* (an index into the GDT), not an address.

**Descriptor layout** (8 bytes, fields deliberately scattered by Intel):

| Bits | Field |
|---|---|
| 0–15 | limit 0:15 |
| 16–39 | base 0:23 |
| 40–47 | access byte |
| 48–51 | limit 16:19 |
| 52–55 | flags (G, DB, L, AVL) |
| 56–63 | base 24:31 |

Access byte, bit 7→0: `P | DPL(2) | S | E | DC | RW | A`. For code, `DC` =
conforming and `RW` = readable; for data, `DC` = expand-down and `RW` = writable.
`A` is set by the CPU, initialised 0.

**Table contents — flat model, 7 entries.** Every non-null entry is base
`0x00000000`, limit `0xFFFFF` with `G=1`, `DB=1` (→ full 4 GB). Only the access
byte differs:

| # | Selector | Entry | Access | Flags+limit | `.quad` |
|---|---|---|---|---|---|
| 0 | `0x00` | null | — | — | `0x0000000000000000` |
| 1 | `0x08` | kernel code | `0x9A` | `0xCF` | `0x00cf9a000000ffff` |
| 2 | `0x10` | kernel data | `0x92` | `0xCF` | `0x00cf92000000ffff` |
| 3 | `0x18` | kernel stack | `0x92` | `0xCF` | `0x00cf92000000ffff` |
| 4 | `0x20` | user code | `0xFA` | `0xCF` | `0x00cffa000000ffff` |
| 5 | `0x28` | user data | `0xF2` | `0xCF` | `0x00cff2000000ffff` |
| 6 | `0x30` | user stack | `0xF2` | `0xCF` | `0x00cff2000000ffff` |

Selector = byte offset of the descriptor, because the layout is
`[index:13][TI:1][RPL:2]` and index sits at bit 3. Hence multiples of 8. User
selectors used from ring 3 need `| 3` (RPL) — not needed until KFS-5.

**Constraint types, don't conflate them:**
- *Hardware:* `CS` can't be `mov`'d; only `ljmp`/`lcall`/`lret`/`iret`/interrupts
  change it. The descriptor is cached in a hidden half of each segment register
  at load time and the GDT is never re-read afterwards.
- *42 subject:* the table must live at `0x00000800`, and there must be six named
  entries including separate stack descriptors.
- *Convention:* flat model. Nothing forces base 0 / limit 4 GB; it's what every
  modern kernel does, because real protection comes from paging (KFS-3).

**`gdt.s` order — this sequence is load-bearing:**

1. `.set` constants for selectors, access bits, flags (never magic numbers —
   correctors ask "why 0x9A?").
2. Descriptors in `.data`, between `gdt_start:` and `gdt_end:`.
3. GDTR: `.word gdt_end - gdt_start - 1` then `.long 0x800`. Limit = size − 1.
   Base is `0x800`, **not** `gdt_start`.
4. Copy 56 bytes from `gdt_start` to `0x800` (`rep movsl`, `ecx = 14`).
   Note `movl $gdt_start, %esi` — the `$` takes the address, not the bytes.
5. `lgdt gdtr` — **no `$`**, this one is a memory operand.
6. Reload `ds/es/fs/gs` with `KERNEL_DATA` (`0x10`) via `ax`, and `ss` with
   `KERNEL_STACK` (`0x18`).
7. `ljmp $KERNEL_CODE, $1f` / `1:` to reload `CS`. Both operands take `$`;
   without it GAS assembles a far *indirect* jump, which is a different
   instruction.
8. `ret` — `ESP` is untouched by any of the above, so the C caller's frame
   survives.

**`SS` gets `0x18`, not `0x10`, deliberately.** The two descriptors are
byte-identical so behaviour is the same, but "SS uses the stack segment" is a
better evaluation answer than "I built an entry and never used it".

**Why the copy exists at all.** The linker places `.data` above 1 MB; `0x800` is
below the load address and outside the image entirely, so it can't be placed
there statically. The lesson the subject is forcing: the GDT is just bytes at an
address of your choosing, and the GDTR is what gives them meaning. `0x800` itself
is arbitrary — it's the first free low-memory region, past the real-mode IVT
(`0x000`–`0x3FF`) and BDA (`0x400`–`0x4FF`).

**Verification** (do this, don't eyeball the bits):
- `qemu-system-i386 -cdrom kfs.iso -monitor stdio`, then `info registers`.
  Expect `GDT= 00000800 00000037` and e.g. `DS =0010 00000000 ffffffff 00cf9300`
  — selector, *cached* base, cached limit, packed access/flags.
- On triple fault: `-d int,cpu_reset -no-reboot` gives the fault and `EIP`.
  Test `reboot` **without** `-no-reboot`, or QEMU exiting looks like success.

## Gotchas: GDT

- **`lgdt` is silent.** It never faults, even with a garbage base. The failure
  always surfaces at the *next* segment register load, which makes the `ljmp`
  look guilty when the descriptor is what's wrong.
- **Never set `DC=1` on the stack entries.** For data segments `DC=1` is
  *expand-down*: valid offsets become `limit+1 .. 0xFFFFFFFF`, i.e. the limit
  becomes a floor. With `limit=0xFFFFF, G=1` the valid range is empty and every
  stack access faults instantly. "Grows down = use for stacks" is a trap.
- **`G=1` fills the low 12 bits with 1s**, not 0s: effective limit is
  `(limit << 12) | 0xFFF`. That's why `0xFFFFF` gives exactly 4 GB.
- **`ES` must be sane before `rep movsl`** — destination is `ES:EDI`, hardwired.
  Fine at boot since GRUB left it flat, but relevant if the copy ever moves.
- **The GDTR base is a *linear* address.** Once paging is enabled in KFS-3, any
  segment-register reload walks the page tables to reach `0x800`. Low memory must
  stay identity-mapped or the next `mov %ax, %ds` faults, and it will look
  unrelated to the paging change. Same applies to the IDTR in KFS-4. **A comment
  saying this belongs next to the `lgdt`.**
- **Check `ebx` (the Multiboot info pointer) doesn't land in `0x800`–`0x837`**
  before copying. GRUB2 normally puts it tens of KB up, but silently corrupting
  it surfaces in KFS-3 when the memory map reads as garbage.
- Multiboot guarantees flat `CS`/`DS`/`ES`/`FS`/`GS`/`SS` with base 0 and limit
  4 GB on entry, but **does not guarantee the selector values**. Never hardcode
  GRUB's.
- **The "10 MB" line in the subject means the ISO**, not segment limits. Clamping
  limits to 10 MB would be unusual and buys nothing paging won't do better; flat
  4 GB is what to defend at evaluation.

## Design: stack dump (KFS-2 mandatory)

Two functions, split on purpose:

```c
void hexdump(const void *addr, size_t len);   /* generic, reused in KFS-3/4 */
void print_stack(void);                       /* computes the range, calls it */
```

`len` is a **byte count**, unrelated to the 16-bytes-per-row formatting constant.
Format is `hexdump -C` style: address, 16 hex bytes grouped in 4s, ASCII column.

`hexdump` is in `src/debug/hexdump.c`, `print_stack` in `src/debug/stack.c`.
The split is the point: `stack.c` is the only file that knows where the kernel
stack lives, so `hexdump` stays reusable for page tables (KFS-3) and interrupt
frames (KFS-4). `shell_stack_dump` in `commands.c` is a one-line wrapper — the
shell owns the *command*, `debug/` owns the *capability*.

- The interesting range is `[esp, stack_top)` — everything below `ESP` is stale.
  `stack_top` is a `boot.s` label with no object behind it, so it is declared
  `extern uint8_t stack_top[];`: as an array it decays to its address, which
  stops it from silently compiling as a *load* of whatever `.bss` variable
  follows the stack. (`extern uint32_t stack_top;` + `&stack_top` works too, but
  the array form makes "address only" explicit and gives byte arithmetic.)
- **Clamp `len`** (`STACK_DUMP_MAX`, currently 512) and **guard `esp >=
  stack_top`**. Without the guard the subtraction underflows to near `SIZE_MAX`,
  and with no paging there's nothing to fault on — it just prints megabytes of
  garbage until the machine is rebooted by hand. The guard is correctness; the
  clamp is a display choice and free to tune.
- Reading `ESP` from C includes the reading function's own frame. Harmless, but
  the top rows are your own call.
- **Filter the ASCII column to `0x20`–`0x7E`, substitute `.`.** This is not
  cosmetic: the screen is CP437 and has a glyph for all 256 values, but raw
  `0x0A`/`0x08` bytes hitting `terminal_putchar` trigger newline/backspace
  handling and destroy the row alignment.
- Byte-order choice to be able to defend: memory order (so a dword `0xDEADBEEF`
  reads `EF BE AD DE`, matching real `hexdump -C`) vs dword-per-column (easier to
  correlate with return addresses). Either is fine; name the tradeoff.
- An `EBP`-chain backtrace is the fancier reading of "print the kernel stack",
  but without symbols it can only print raw addresses and `-fomit-frame-pointer`
  breaks it. Bonus at best.

## Design: shell (KFS-2 bonus)

Loop is: prompt → readline → split → table lookup → dispatch. No processes, no
pipes, no globbing. **Table-driven dispatch**, so `help` derives from the same
array the dispatcher walks and can never drift out of sync:

```c
typedef struct s_command {
    const char *name;
    const char *desc;
    void      (*handler)(void);   /* argc/argv deferred — no splitting yet */
}   t_command;
```

**The table is defined in `commands.c` and declared `extern` in `shell.h`.** Not
`static const` in the header: that gives every translation unit that includes it
a private copy, which silently doubles as more files include it and makes
"the dispatcher and `help` walk the same array" false at the object level.

Forward-declare the handlers above the array — `shell_help` reads `cmd_arr[]` and
`cmd_arr[]` references `shell_help`. `shell.h` declares both, so including it is
enough.

Commands: `help`, `clear`, `stack_dump` (the KFS-2 deliverable), `reboot`,
`halt`. `clear`'s handler is `terminal_clear` directly — the signature already
matches, so no wrapper. A `gdt` command (dump the entries, the thing that wants
`%08x`) is still worth adding.

**No argument splitting yet**, hence `void (*)(void)`. The moment a command needs
an argument, the readline result gets tokenised in `shell_loop` and the handler
signature widens to `(int argc, char **argv)` — one table, one edit.

**`reboot` = pulse the 8042.** Poll port `0x64` until status bit 1 (input buffer
full) clears, then `outb(0x64, 0xFE)`, then `cli; hlt` as a backstop since the
reset isn't instant. Alternative kept in the back pocket: load a null IDTR and
`int $0x03` to force a triple fault — a hack, but portable and worth having as a
`crash` command since accidental triple faults are how bugs currently reboot the
machine. ACPI reset needs FADT parsing; deferred indefinitely.

**`halt` = `cli; hlt`** in a loop.

**The line editor is the "line discipline" layer**, and it lives in
`shell_loop.c`. The terminal stays dumb.

```c
typedef struct {
    char    buf[MAX_LINE_LEN];
    size_t  len;
    uint8_t prompted;     /* has this screen ever had a prompt printed? */
}   line_state_t;

static line_state_t lines[TERMINAL_COUNT];   /* one per screen */
```

**One line state per screen, not one global.** Switching with F1–F10 mid-command
parks the half-typed line on the terminal you left and resumes whatever was
pending on the one you land on — which is what a real VT does, and falls out for
free because each screen already has its own shadow buffer showing the text.

**It is keyed on `terminal_current()` rather than stored inside `terminal_t`.**
Both give the same behaviour; the difference is who owns the concept of "a line
being typed". Putting it in `terminal_t` makes the driver know what characters
*mean*. Keying on the index keeps that knowledge in the shell, and in KFS-5,
when a process owns a tty, this array becomes per-process state without the
driver changing at all.

**`prompted` replaces the `input_x`/`input_y` boundary** an earlier draft of this
file proposed. Backspace can't eat the prompt because `line_pop()` refuses at
`len == 0` — the boundary is the line buffer's own emptiness, so no screen
coordinates need storing. `prompted` answers a different question: a screen you
have never visited is blank and needs a prompt printed on arrival.

Arrow keys and history go here too, and that is what wants `kbd_buf_pop` to
yield a `key_event_t` rather than a `char` — see Open threads.

## Rejected designs (don't re-propose)

- **`input_x`/`input_y` as the backspace boundary.** Superseded — the line
  buffer's own length is the boundary, and storing screen coordinates would need
  invalidating on every scroll.
- **A single `active` pointer** covering both visibility and write target. Note
  the code currently *has* one; it is a known deferral, not an endorsement. See
  the terminal section for what replaces it and when.
- **Line-editing state inside `terminal_t`.** Tried, then moved out: it makes
  the driver understand what the characters on screen mean, and blocks the
  KFS-5 migration where line state follows a process rather than a screen. The
  per-terminal *behaviour* was worth keeping and survives, keyed on
  `terminal_current()` from the shell side.
- **Building `inb`/`outb` in a `.s` file** — see `io.h` section.

## Gotchas already hit

**Build / boot**

- **ISO size limit is 10 MB — it applies to the ISO, not the source or the
  kernel binary.** Default `grub-mkrescue` produced >10 MB because EFI GRUB
  packages were installed. Fixed with:
  ```
  grub-mkrescue -o kfs.iso -d /usr/lib/grub/i386-pc isodir \
    --install-modules="multiboot normal iso9660 biosdisk configfile" \
    --locales="" --fonts="" --themes=""
  ```
  Brings it to ~0.6 MB. `-d /usr/lib/grub/i386-pc` forces BIOS-only (skips EFI).
- `grub-mkrescue` needs `xorriso` **and** `mtools` installed or it fails with
  confusing errors.
- `grub.cfg` must end up at `/boot/grub/grub.cfg` *inside* the image — the
  `boot/` level is required or GRUB finds no menu.
- Sanity check before building the ISO: `grub-file --is-x86-multiboot kfs`

**Terminal (all three found in review of the first `terminal.c`)**

- **Scroll must blank the last row** after shifting, or row 24 keeps a stale copy
  of row 23.
- **Cast to `unsigned char` in `vga_entry`.** `char` is signed on i686, so a byte
  ≥ 0x80 sign-extends to `0xFF8x` and clobbers the attribute half. Only shows up
  with box-drawing characters, which makes it maddening to find later.
- **`volatile` on the framebuffer pointer.** Nothing ever reads it back, so an
  optimising compiler may delete the writes. Write `vga_blit` as a loop rather
  than `memcpy`, since `memcpy` isn't declared to respect `volatile`.
- **`terminal_initialize()` should be `terminal_initialize(void)`.** In C an
  empty parameter list means "unspecified arguments", not "none" — so the
  compiler checks nothing at the call site. Swept through the tree during the
  KFS-2 restructure; `-Wstrict-prototypes` would catch any that come back.

## Open threads / deliberately deferred

- ~~**Input boundary for backspace belongs to the shell (KFS-2)**~~ — *done; the
  shell's line editor owns it. See Design: shell.*
- ~~**`key_event_t`**~~ — *done. `kbd_feed` returns events, the ring buffer
  carries them, modifiers and arrow keys are decoded.*
- **Arrow keys are decoded but ignored.** `KEY_UP`/`KEY_DOWN`/`KEY_LEFT`/
  `KEY_RIGHT` reach `get_line`, which drops them (they fail the `< 0x100` test).
  History and in-line cursor movement are now purely a `shell_loop.c` job — the
  plumbing beneath is finished.
- **`MOD_CTRL` is delivered but nothing consumes it.** Ctrl+letter already
  arrives as `0x01`–`0x1A`; Ctrl+C (abandon line) and Ctrl+L (clear) are a few
  lines in `get_line`.
- **`visible`/`focused` split, explicit-target functions, and the `terminal_put`
  choke point.** Designed in Design: terminal, none implemented — the code has
  one `active` pointer and every function writes the framebuffer unconditionally.
  Harmless while the only writer is the keyboard; **must** be done before the
  first background writer, i.e. the PIT handler in KFS-4.
- **`gdt.s` uses bare literals** — `$0x10`, `$0x18`, `$0x08` for the selectors
  and raw `.quad`s for the descriptors. The file's own design section says
  `.set` constants, "never magic numbers — correctors ask why 0x9A?". Cheap fix,
  directly on the evaluation surface.
- **Width/flag handling in `printk`** — `%08x`, `%-10s`, `%#x` are not
  implemented. Now actively wanted: `%08x` for a future `gdt` command and the
  hexdump address column, `%-12s` for `help`'s alignment (currently padded by
  hand with a `strlen` loop in `shell_help`). Parse flags and width right after
  the `%`, before the `switch`.
- **Identity-map low memory when paging goes on in KFS-3**, or the GDT at `0x800`
  becomes unreachable on the next segment reload. See Gotchas: GDT.
- **User segments (`0x20`–`0x30`) are built but unused.** Nothing runs in ring 3
  until KFS-5. Their RPL bits (`| 3`) and the TSS needed for the ring 3 → ring 0
  transition are both deferred.
- **`map_lower`/`map_upper` are QWERTY.**
- **Hardware scrolling** via CRTC registers `0x0C`/`0x0D` as an optimisation for
  `terminal_scroll`. Not needed.
- Real terminals keep a per-column bitmap of tab stops (that's how `tabs -4`
  works), with every-8 as the default state. Relevant only if ANSI escape
  handling is ever added.

## Reference

- OSDev Bare Bones (the basis of the current code) — wiki.osdev.org/Bare_Bones
- OSDev Meaty Skeleton (project layout) — wiki.osdev.org/Meaty_Skeleton
- OSDev "PS/2 Keyboard" and "Text Mode Cursor" pages
- Multiboot spec — gnu.org/software/grub/manual/multiboot/multiboot.html
- Yale x86 assembly guide (32-bit, AT&T/GAS)
- felixcloutier.com/x86 (instruction lookup)
- Subject PDFs `kfs1.pdf`–`kfs9.pdf`, `kfsx.pdf` live in this repo — read them
  rather than guessing what a later project requires.

## How to help

**This is a learning project with a peer evaluation at the end. Explaining beats
delivering.**

The author is learning this from scratch and asks a lot of "what does X mean"
and "why is it like that" questions — **explain concepts and the reasoning behind
them rather than just handing over code**. The questions in past sessions
(`4(%esp)`, why two CRTC registers, what start/end scanlines mean, why `outb`
exists at all) are the shape of what's useful.

- **Don't write whole modules on request.** Offer instead: a worked reference
  implementation of the single hardest function, the headers/prototypes so the
  structure is fixed, review of code the author wrote, or ordering the work
  easiest-first. Mechanical work (Makefile, boilerplate, bug-hunting) is fair
  game; the parts an evaluator will grill — scroll, wrap, the layering
  rationale — should be written by the author.
- **Flag which kind of constraint something is**: hardware requirement vs C
  language rule vs 42-subject requirement vs arbitrary convention. These get
  conflated easily and the distinction is most of the understanding.
- **Say when a design choice is a choice.** Eager vs deferred wrap, destructive
  vs non-destructive tab, clamping vs wrapping backspace — name the alternative
  and why this one was picked, because that's exactly what gets asked at
  evaluation.
- The author catches real design bugs (the `active`-pointer conflation was
  their catch). Take pushback seriously rather than defending the first answer.
