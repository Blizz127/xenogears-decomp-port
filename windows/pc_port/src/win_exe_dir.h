#ifndef XENO_WIN_EXE_DIR_H
#define XENO_WIN_EXE_DIR_H

#include <stdlib.h>
#include <string.h>

/* The CRT provides the executable path in the same encoding as fopen().
 * Keep windows.h out of data TUs that also include the PsyQ SDK headers. */
static inline const char* PcPort_WinLastSeparator(const char* path)
{
    const char* forward = strrchr(path, '/');
    const char* backward = strrchr(path, '\\');
    return backward && (!forward || backward > forward) ? backward : forward;
}

static inline int PcPort_WinExeDir(char* out, size_t capacity)
{
    char* executable = NULL;
    if (_get_pgmptr(&executable) != 0 || !executable)
        return 0;
    const char* slash = PcPort_WinLastSeparator(executable);
    if (!slash || (size_t)(slash - executable) >= capacity)
        return 0;
    size_t length = (size_t)(slash - executable);
    memcpy(out, executable, length);
    out[length] = '\0';
    return 1;
}

#endif
