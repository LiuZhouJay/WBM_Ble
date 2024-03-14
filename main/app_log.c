#include "driver/uart.h"

#include <string.h>

#if 1
#include <stdio.h>

int fputc(int ch, FILE *stream)
{
    uart_write_bytes(UART_NUM_0, (const char*) ch, 1);
    return ch;
}
#endif

int app_log_init(void)
{
    return 0;
}
