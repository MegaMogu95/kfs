#include "shell.h"
#include "terminal.h"
#include "printk.h"
#include "debug.h"
#include "io.h"
#include "libk.h"

const t_command	cmd_arr[] =
{
	{"stack_dump",	"hexdump the live kernel stack",	shell_stack_dump},
	{"gdt_dump",		"print the GDT entries",			shell_gdt_dump},
	{"regs",			"dump CPU registers",				shell_regs},
	{"clear",		"blank the current screen",			terminal_clear},
	{"reboot",		"reset the machine via the 8042",	shell_reboot},
	{"halt",		"stop the CPU (cli; hlt)",			shell_halt},
	{"help",		"list these commands",				shell_help},
	{0, 0, 0}
};

void	shell_stack_dump(void)
{
	print_stack();
}

/*
** Read the GDTR with sgdt, then walk every 8-byte descriptor and print
** base, limit (G-bit expanded), raw access byte, flags nibble and DPL.
** Entry names are hard-coded for the 7-entry flat-model table built by
** gdt.s; any extra entries beyond that are labelled "?".
*/
void	shell_gdt_dump(void)
{
	static const char *names[] = {
		"null",
		"kernel code",	"kernel data",	"kernel stack",
		"user code",	"user data",	"user stack"
	};
	static const int name_count = (int)(sizeof(names) / sizeof(names[0]));

	struct {
		uint16_t	limit;
		uint32_t	base;
	} __attribute__((packed)) gdtr;

	__asm__ volatile ("sgdt %0" : "=m"(gdtr));

	int count = (int)((gdtr.limit + 1) / 8);

	printk("GDT  base=0x%08x  limit=0x%04x  (%d entries)\n\n",
		gdtr.base, gdtr.limit, count);
	printk(" #  sel   base        limit(eff)  access  flg  dpl  type\n");

	for (int i = 0; i < count; i++) {
		uint8_t *e = (uint8_t *)(gdtr.base + (uint32_t)(i * 8));

		uint32_t limit_raw = (uint32_t)e[0]
			| ((uint32_t)e[1] << 8)
			| (((uint32_t)e[6] & 0x0f) << 16);
		uint32_t base_val  = (uint32_t)e[2]
			| ((uint32_t)e[3] << 8)
			| ((uint32_t)e[4] << 16)
			| ((uint32_t)e[7] << 24);
		uint8_t  access    = e[5];
		uint8_t  flags     = (e[6] >> 4) & 0x0f;
		uint32_t limit_eff = (flags & 0x8)
			? (limit_raw << 12) | 0xfff
			: limit_raw;
		int      dpl       = (access >> 5) & 0x3;
		const char *type;

		if (!(access & 0x80))
			type = "<null>";
		else if (!(access & 0x10))
			type = "system";
		else if (access & 0x08)
			type = "code";
		else
			type = "data";

		const char *name = (i < name_count) ? names[i] : "?";

		printk(" %d  0x%02x  0x%08x  0x%08x  0x%02x    0x%x   %d   %s (%s)\n",
			i, i * 8, base_val, limit_eff, access, flags, dpl, type, name);
	}
}

/*
** Pulse the keyboard controller's reset line.  Poll until the input buffer
** (status bit 1) is clear, or the 0xFE lands in a full buffer and is dropped.
** The reset is not instant, hence the cli;hlt backstop.
*/
void	shell_reboot(void)
{
	uint8_t status = 0x02;

	while (status & 0x02)
		status = inb(0x64);
	outb(0x64, 0xfe);
	__asm__ volatile ("cli; hlt");
}

void	shell_halt(void)
{
	__asm__ volatile ("cli; hlt");
}

/*
** Capture GPRs, segment selectors, EFLAGS and EIP via inline asm and
** pretty-print them.  Values reflect the state inside shell_regs itself
** (compiler prologue + our capture stmts), so esp/ebp show this
** function's own frame, not the caller's -- that is expected and fine
** for a debug snapshot.
*/
void	shell_regs(void)
{
	uint32_t eax, ebx, ecx, edx, esi, edi, ebp, esp, eflags, eip;
	uint32_t cs, ds, es, fs, gs, ss;

	__asm__ volatile ("mov %%eax, %0" : "=m"(eax));
	__asm__ volatile ("mov %%ebx, %0" : "=m"(ebx));
	__asm__ volatile ("mov %%ecx, %0" : "=m"(ecx));
	__asm__ volatile ("mov %%edx, %0" : "=m"(edx));
	__asm__ volatile ("mov %%esi, %0" : "=m"(esi));
	__asm__ volatile ("mov %%edi, %0" : "=m"(edi));
	__asm__ volatile ("mov %%ebp, %0" : "=m"(ebp));
	__asm__ volatile ("mov %%esp, %0" : "=m"(esp));
	__asm__ volatile ("pushfl; popl %0" : "=rm"(eflags));
	__asm__ volatile ("call 1f\n1: popl %0" : "=r"(eip));
	__asm__ volatile ("xorl %0, %0; movw %%cs, %w0" : "=r"(cs));
	__asm__ volatile ("xorl %0, %0; movw %%ds, %w0" : "=r"(ds));
	__asm__ volatile ("xorl %0, %0; movw %%es, %w0" : "=r"(es));
	__asm__ volatile ("xorl %0, %0; movw %%fs, %w0" : "=r"(fs));
	__asm__ volatile ("xorl %0, %0; movw %%gs, %w0" : "=r"(gs));
	__asm__ volatile ("xorl %0, %0; movw %%ss, %w0" : "=r"(ss));

	printk("--- CPU registers ---\n");
	printk(" eax=0x%08x  ebx=0x%08x  ecx=0x%08x  edx=0x%08x\n",
		eax, ebx, ecx, edx);
	printk(" esi=0x%08x  edi=0x%08x  ebp=0x%08x  esp=0x%08x\n",
		esi, edi, ebp, esp);
	printk(" eip=0x%08x  eflags=0x%08x\n", eip, eflags);
	printk(" cs=0x%04x  ds=0x%04x  es=0x%04x  fs=0x%04x  gs=0x%04x  ss=0x%04x\n",
		cs, ds, es, fs, gs, ss);
	printk(" CF=%d PF=%d AF=%d ZF=%d SF=%d TF=%d IF=%d DF=%d OF=%d\n",
		(eflags >> 0)  & 1,
		(eflags >> 2)  & 1,
		(eflags >> 4)  & 1,
		(eflags >> 6)  & 1,
		(eflags >> 7)  & 1,
		(eflags >> 8)  & 1,
		(eflags >> 9)  & 1,
		(eflags >> 10) & 1,
		(eflags >> 11) & 1);
}

void	shell_help(void)
{
	size_t	pad;

	printk("Available commands:\n");
	for (size_t i = 0; cmd_arr[i].name; i++)
	{
		printk("  %s", cmd_arr[i].name);
		pad = strlen(cmd_arr[i].name);
		while (pad++ < 12)
			printk(" ");
		printk("%s\n", cmd_arr[i].desc);
	}
}
