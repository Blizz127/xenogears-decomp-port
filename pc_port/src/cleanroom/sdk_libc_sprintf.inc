/* Game-facing variadic ABI from include/system/memory.h.
 * C11 7.21.6.13 supplies the host formatting and character-count contract. */
#include <stdarg.h>
#include <stdio.h>

int Sprintf(char *destination, char *format, ...)
{
    va_list arguments;
    va_start(arguments, format);
    int written = vsprintf(destination, format, arguments);
    va_end(arguments);
    return written;
}
