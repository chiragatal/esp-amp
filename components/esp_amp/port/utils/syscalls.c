#include "stdio.h"

struct _reent *__getreent(void)
{
    return _GLOBAL_REENT;
}

void _fstat_r(void) {}

void _close_r(void) {}

void _lseek_r(void) {}

void _read_r(void) {}

void _write_r(void) {}

void _getpid_r(void) {}

void _kill_r(void) {}

void __assert_func(const char *file, int line, const char *func, const char *expr)
{
    printf("Assert failed in %s, %s:%d (%s)\r\n", func, file, line, expr);
    while (1);
}
