#include "kbd_buffer.h"
#define KBD_BUF_SIZE 256                 /* power of two */

static volatile char     buf[KBD_BUF_SIZE];
static volatile uint32_t head = 0;       /* producer */
static volatile uint32_t tail = 0;       /* consumer */

void kbd_buf_push(char c)
{
    uint32_t next = (head + 1) & (KBD_BUF_SIZE - 1);
    if (next == tail) return;            /* full → drop */
    buf[head] = c;
    head = next;
}

int kbd_buf_pop(char *out)
{
    if (tail == head) return 0;          /* empty */
    *out = buf[tail];
    tail = (tail + 1) & (KBD_BUF_SIZE - 1);
    return 1;
}