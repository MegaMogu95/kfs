#include "printk.h"
#include "terminal.h"

static void print_number(const printk_sink_t *sink, unsigned long value,
                         unsigned base, int upper, size_t min_digits)
{
    const char *digits = upper ? "0123456789ABCDEF" : "0123456789abcdef";
    char   buf[32];
    size_t i = 0;

    if (min_digits > sizeof(buf))       /* never pad past the buffer */
        min_digits = sizeof(buf);

    if (value == 0)
        buf[i++] = '0';
    while (value != 0) {
        buf[i++] = digits[value % base];
        value /= base;
    }
    while (i < min_digits)
        buf[i++] = '0';
    while (i > 0)                       /* digits came out reversed */
        sink->put(buf[--i]);
}

static void print_signed(const printk_sink_t *sink, int v, size_t min_digits)
{
    unsigned long u;

    if (v < 0) {
        sink->put('-');
        u = -(unsigned long)v;          /* well-defined: modular negation */
    } else {
        u = (unsigned long)v;
    }
    print_number(sink, u, 10, 0, min_digits);
}

static void print_string(const printk_sink_t *sink, const char *s)
{
    if (s == NULL)
        s = "(null)";
    while (*s)
        sink->put(*s++);
}

void vprintk(const printk_sink_t *sink, const char *fmt, va_list ap)
{
    uint8_t saved = sink->get_color ? sink->get_color() : 0;

    for (size_t i = 0; fmt[i]; i++) {
        if (fmt[i] != '%') {
            sink->put(fmt[i]);
            continue;
        }
        i++;

        /* optional digit count: %8x, %08x and %3d all pad with '0' to width */
        size_t digits = 0;
        if (fmt[i] == '*') {
            int n = va_arg(ap, int);
            digits = n > 0 ? (size_t)n : 0;
            i++;
        } else {
            while (fmt[i] >= '0' && fmt[i] <= '9')
                digits = digits * 10 + (size_t)(fmt[i++] - '0');
        }

        switch (fmt[i]) {
            case 'c': sink->put((char)va_arg(ap, int));                    break;
            case 's': print_string(sink, va_arg(ap, const char *));        break;
            case 'd':
            case 'i': print_signed(sink, va_arg(ap, int), digits);         break;
            case 'u': print_number(sink, va_arg(ap, unsigned int), 10, 0, digits); break;
            case 'x': print_number(sink, va_arg(ap, unsigned int), 16, 0, digits); break;
            case 'X': print_number(sink, va_arg(ap, unsigned int), 16, 1, digits); break;
            case 'b': print_number(sink, va_arg(ap, unsigned int),  2, 0, digits); break;
            case 'p':
                sink->put('0'); sink->put('x');
                print_number(sink, (unsigned long)va_arg(ap, void *), 16, 0,
                             digits ? digits : 8);   /* 8 is the default for %p */
                break;
            case 'C': {
                uint8_t color = (uint8_t)va_arg(ap, int);
                if (sink->set_color)
                    sink->set_color(color);
                break;
            }
            case '%': sink->put('%');                                      break;
            case '\0': i--;                                                break;  /* trailing % */
            default:  sink->put('%'); sink->put(fmt[i]);                   break;
        }
    }

    if (sink->get_color && sink->set_color)
        sink->set_color(saved);         /* colour changes don't leak out */
}

static const printk_sink_t terminal_sink = {
    terminal_putchar,
    terminal_set_color,
    terminal_get_color,
};

void printk(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    vprintk(&terminal_sink, fmt, ap);
    va_end(ap);
}