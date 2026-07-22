#ifndef KEYBOARD_H
# define KEYBOARD_H

# include <stdint.h>

# define SC_LSHIFT 0x2A
# define SC_RSHIFT 0x36
# define SC_CAPS   0x3A
# define SC_RELEASE 0x80
# define SC_F1 0x3B
# define CHR_F1 -10

char kbd_feed(uint8_t sc);

#endif