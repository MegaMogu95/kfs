#ifndef KBD_BUFFER_H
# define KBD_BUFFER_H

# include "keyboard.h"

/*
** The producer/consumer seam.  Today the producer is ps2_poll(); in KFS-4 it
** becomes the IRQ1 handler and nothing else changes -- which is the whole
** reason this sits between the decoder and its reader.
*/

void	kbd_buf_push(key_event_t ev);
int		kbd_buf_pop(key_event_t *out);

#endif
