#include "io.h"
#include "keyboard.h"
#include "kbd_buffer.h"

#define PS2_DATA     0x60
#define PS2_STATUS   0x64
#define PS2_OUT_FULL 0x01

/* Poll once. In KFS-4 the IRQ1 handler IS the body of this if,
 * and this whole function gets deleted. */
void ps2_poll(void) {
    if (inb(PS2_STATUS) & PS2_OUT_FULL) {
        uint8_t sc = inb(PS2_DATA);
        char c = kbd_feed(sc);
        if (c) kbd_buf_push(c);
    }
}