#include "defs.h"
#include <stdarg.h>
static char digits[] = "0123456789abcdef";

static void
printint(int xx, int base, int sign) {
    char buf[16];
    int i;
    uint x;

    if (sign && (sign = xx < 0))
        x = -xx;
    else
        x = xx;

    i = 0;
    do {
        buf[i++] = digits[x % base];
    } while ((x /= base) != 0);

    if (sign)
        buf[i++] = '-';

    while (--i >= 0)
        consputc(buf[i]);
}

static void
printptr(uint64 x) {
    int i;
    consputc('0');
    consputc('x');
    for (i = 0; i < (sizeof(uint64) * 2); i++, x <<= 4)
        consputc(digits[x >> (sizeof(uint64) * 8 - 4)]);
}

// Print to the console. only understands %d, %x, %p, %s.
void printf(char *fmt, ...) {
    va_list ap;
    int i, c;
    char *s;

    if (fmt == 0)
        panic("null fmt");

    va_start(ap, fmt);
    for (i = 0; (c = fmt[i] & 0xff) != 0; i++) {
        if (c != '%') {
            consputc(c);
            continue;
        }
        c = fmt[++i] & 0xff;
        if (c == 0)
            break;
        switch (c) {
            case 'd':
                printint(va_arg(ap, int), 10, 1);
                break;
            case 'x':
                printint(va_arg(ap, int), 16, 1);
                break;
            case 'p':
                printptr(va_arg(ap, uint64));
                break;
            case 's':
                if ((s = va_arg(ap, char *)) == 0)
                    s = "(null)";
                for (; *s; s++)
                    consputc(*s);
                break;
            case '%':
                consputc('%');
                break;
            default:
                // Print unknown % sequence to draw attention.
                consputc('%');
                consputc(c);
                break;
        }
    }
}

void
panic(char *s)
{
    printf("panic: ");
    printf(s);
    printf("\n");
    loop();
}

//实现不同等级的log
void closeprint(char *fmt, ...)
{
    return;
}
void error(char *fmt, ...)
{
    printf("\033[1;31m [error] [0] \033[0m");
    printf("\033[1;31m");
    printf(fmt);
    return;
}
void warn(char *fmt, ...)
{
    printf("\033[1;33m [warn] [0] \033[0m");
    printf("\033[1;33m");
    printf(fmt);
    error(fmt);
    return;
}
void info(char *fmt, ...)
{
    printf("\033[1;34m [info] [0] \033[0m");
    printf("\033[1;34m");
    printf(fmt);
    warn(fmt);
    return;
}
void debug(char *fmt, ...)
{
    printf("\033[1;32m [debug] [0] \033[0m");
    printf("\033[1;32m");
    printf(fmt);
    info(fmt);
    return;
}

//使用彩色输出宏输出 os 内存空间布局
void printfcolor(char *fmt,char *s)
{
    printf("\033[1;32m");
    printf(fmt,s);
    return;
}