#ifndef KEYBOARD_H
# define KEYBOARD_H

# include <stdint.h>

/* --- scancodes, translated set 1 (what the 8042 gives us under GRUB/BIOS) --- */
# define SC_RELEASE		0x80	/* bit 7 set = key released (break code)     */
# define SC_EXTENDED	0xE0	/* prefix: the NEXT code is an extended key  */
# define SC_LSHIFT		0x2A
# define SC_RSHIFT		0x36
# define SC_LCTRL		0x1D
# define SC_LALT		0x38
# define SC_CAPS		0x3A
# define SC_F1			0x3B	/* F1..F10 are contiguous, 0x3B..0x44       */
# define SC_UP			0x48	/* these four only after an 0xE0 prefix     */
# define SC_LEFT		0x4B
# define SC_RIGHT		0x4D
# define SC_DOWN		0x50

/* --- modifier bitmask --- */
# define MOD_SHIFT		0x01
# define MOD_CTRL		0x02
# define MOD_ALT		0x04

/*
** Keys with no character to stand for them start at 0x100, above every ASCII
** value, so `code` never needs a companion field saying which kind it holds.
*/
# define KEY_F1			0x100
# define KEY_F10		(KEY_F1 + 9)
# define KEY_UP			0x110
# define KEY_DOWN		0x111
# define KEY_LEFT		0x112
# define KEY_RIGHT		0x113

/*
** The decoder's output.  Not a char: there is no character for F1, and no way
** for a char to say "Ctrl was held".  code == 0 means this scancode produced no
** event (a modifier press, a release, a prefix byte), so 0 is reserved and is
** never a valid key.
*/
typedef struct {
	uint16_t	code;	/* ASCII 1..127, or a KEY_* constant >= 0x100 */
	uint8_t		mods;	/* MOD_SHIFT | MOD_CTRL | MOD_ALT             */
}	key_event_t;

/* Feed one raw scancode. Knows nothing about polling vs interrupts. */
key_event_t	kbd_feed(uint8_t sc);

#endif
