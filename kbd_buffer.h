#ifndef KBD_BUFFER_H
# define KBD_BUFFER_H

# include "stdint.h"

void kbd_buf_push(char c);

int kbd_buf_pop(char *out);

#endif