#include "types.h"

// panic.c
void loop();

// sbi.c
void console_putchar(int);
int console_getchar();
void shutdown();

// console.c
void consputc(int);

// printf.c
void printf(char *, ...);
void printfinit(void);
void panic(char*);
void closeprint(char *fmt, ...);
void error(char *fmt, ...);
void warn(char *fmt, ...);
void info(char *fmt, ...);
void debug(char *fmt, ...);
void printfcolor(char *fmt,char *s);

// number of elements in fixed-size array
#define NELEM(x) (sizeof(x) / sizeof((x)[0]))