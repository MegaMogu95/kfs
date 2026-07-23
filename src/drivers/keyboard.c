#include "keyboard.h"

static uint8_t	mods = 0;
static int		caps = 0;
static int		extended = 0;	/* the previous byte was 0xE0 */

/* US QWERTY, scancode set 1, indexed by make code. 0 = no printable char. */
static const char map_lower[128] = {
    0,27,'1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',
    0,'a','s','d','f','g','h','j','k','l',';','\'','`',
    0,'\\','z','x','c','v','b','n','m',',','.','/',
    0,'*',0,' ',
};
static const char map_upper[128] = {
    0,27,'!','@','#','$','%','^','&','*','(',')','_','+','\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',
    0,'A','S','D','F','G','H','J','K','L',':','"','~',
    0,'|','Z','X','C','V','B','N','M','<','>','?',
    0,'*',0,' ',
};

static key_event_t	none(void)
{
	key_event_t	ev = {0, 0};

	return (ev);
}

static key_event_t	event(uint16_t code)
{
	key_event_t	ev;

	ev.code = code;
	ev.mods = mods;
	return (ev);
}

/*
** Extended keys arrive as 0xE0 followed by a code that overlaps a normal one:
** 0xE0 0x48 is Up, while a bare 0x48 is keypad 8.  AltGr is 0xE0 0x38 -- the
** same make code as left Alt.  Folding it into MOD_ALT would break Alt+F-key
** the moment an AZERTY user types @ or #, so extended Alt is ignored on
** purpose rather than by omission.
*/
static key_event_t	feed_extended(uint8_t sc)
{
	if (sc & SC_RELEASE)
	{
		if ((sc & 0x7F) == SC_LCTRL)
			mods &= ~MOD_CTRL;
		return (none());
	}
	switch (sc)
	{
		case SC_LCTRL:	mods |= MOD_CTRL;	return (none());	/* right Ctrl */
		case SC_LALT:						return (none());	/* AltGr: drop */
		case SC_UP:		return (event(KEY_UP));
		case SC_DOWN:	return (event(KEY_DOWN));
		case SC_LEFT:	return (event(KEY_LEFT));
		case SC_RIGHT:	return (event(KEY_RIGHT));
	}
	return (none());
}

key_event_t	kbd_feed(uint8_t sc)
{
	char	c;

	if (sc == SC_EXTENDED)
	{
		extended = 1;
		return (none());
	}
	if (extended)
	{
		extended = 0;
		return (feed_extended(sc));
	}
	if (sc & SC_RELEASE)						/* break code */
	{
		switch (sc & 0x7F)
		{
			case SC_LSHIFT: case SC_RSHIFT:	mods &= ~MOD_SHIFT;	break;
			case SC_LCTRL:					mods &= ~MOD_CTRL;	break;
			case SC_LALT:					mods &= ~MOD_ALT;	break;
		}
		return (none());
	}
	switch (sc)
	{
		case SC_LSHIFT: case SC_RSHIFT:	mods |= MOD_SHIFT;	return (none());
		case SC_LCTRL:					mods |= MOD_CTRL;	return (none());
		case SC_LALT:					mods |= MOD_ALT;	return (none());
		case SC_CAPS:					caps ^= 1;			return (none());
	}
	if (sc >= 128)
		return (none());
	if (SC_F1 <= sc && sc <= SC_F1 + 9)
		return (event(KEY_F1 + sc - SC_F1));
	c = (mods & MOD_SHIFT) ? map_upper[sc] : map_lower[sc];
	if (c == 0)
		return (none());
	/*
	** Caps Lock flips LETTERS ONLY.  `shift ^ caps` is the classic bug: it
	** makes Caps Lock turn 1 into !, which no real keyboard does.
	*/
	if (caps)
	{
		if (c >= 'a' && c <= 'z')
			c -= 32;
		else if (c >= 'A' && c <= 'Z')
			c += 32;
	}
	/* Ctrl+letter -> 0x01..0x1A, the control characters those keys mean. */
	if ((mods & MOD_CTRL) && ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')))
		c = (c | 0x20) - ('a' - 1);
	return (event((uint16_t)(unsigned char)c));
}
