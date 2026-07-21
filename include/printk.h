#ifndef PRINTK_H
#define PRINTK_H

#include <stdarg.h>
#include <stdint.h>
#include "vga.h"

typedef struct {
    void    (*put)(char c);
    void    (*set_color)(uint8_t color);   /* may be NULL */
    uint8_t (*get_color)(void);            /* may be NULL */
} printk_sink_t;

void printk(const char *fmt, ...);
void vprintk(const printk_sink_t *sink, const char *fmt, va_list ap);

#define C_INFO    vga_entry_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK)
#define C_WARN    vga_entry_color(VGA_COLOR_LIGHT_BROWN, VGA_COLOR_BLACK)
#define C_ERR     vga_entry_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK)

#define printk_info(fmt, ...) printk("%C[INFO]%C " fmt, C_INFO, C_DEFAULT, ##__VA_ARGS__)
#define printk_warn(fmt, ...) printk("%C[WARN]%C " fmt, C_WARN, C_DEFAULT, ##__VA_ARGS__)
#define printk_err(fmt, ...)  printk("%C[ERR ]%C " fmt, C_ERR,  C_DEFAULT, ##__VA_ARGS__)
#endif