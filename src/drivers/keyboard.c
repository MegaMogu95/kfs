#include "keyboard.h"

static int shift = 0, caps = 0;

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

/* Feed one raw scancode. Returns an ASCII char to emit, or 0.
 * Knows nothing about polling vs interrupts. */
char kbd_feed(uint8_t sc) {
    if (sc & SC_RELEASE) {              /* break code */
        uint8_t make = sc & 0x7F;
        if (make == SC_LSHIFT || make == SC_RSHIFT) shift = 0;
        return 0;
    }
    switch (sc) {
        case SC_LSHIFT: case SC_RSHIFT: shift = 1; return 0;
        case SC_CAPS:   caps ^= 1;      return 0;
    }
    if (sc >= 128) return 0;
	if (SC_F1 <= sc && sc < SC_F1 + 10)
		return (CHR_F1 + sc - SC_F1);
    int upper = shift ^ caps;   /* good enough for now; caps should only flip letters — refine later */
    return upper ? map_upper[sc] : map_lower[sc];
}