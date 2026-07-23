#include "kbd_buffer.h"

#define KBD_BUF_SIZE 256                 /* power of two */

static volatile key_event_t	buf[KBD_BUF_SIZE];
static volatile uint32_t	head = 0;    /* producer */
static volatile uint32_t	tail = 0;    /* consumer */

void	kbd_buf_push(key_event_t ev)
{
	uint32_t next = (head + 1) & (KBD_BUF_SIZE - 1);

	if (next == tail)                    /* full -> drop */
		return ;
	buf[head].code = ev.code;
	buf[head].mods = ev.mods;
	head = next;
}

/*
** Copied field by field rather than `*out = buf[tail]`: struct assignment from
** a volatile source is not guaranteed to keep the volatile semantics, and GCC
** may turn it into a memcpy that reads the slot however it likes.
*/
int	kbd_buf_pop(key_event_t *out)
{
	if (tail == head)                    /* empty */
		return (0);
	out->code = buf[tail].code;
	out->mods = buf[tail].mods;
	tail = (tail + 1) & (KBD_BUF_SIZE - 1);
	return (1);
}
