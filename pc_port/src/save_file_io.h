#ifndef XENO_SAVE_FILE_IO_H
#define XENO_SAVE_FILE_IO_H

#include <stdio.h>
#ifdef _WIN32
#include <direct.h>
#include <windows.h>
#else
#include <sys/stat.h>
#endif

/* Staged files share the destination volume. Never unlink the previous save
 * before the replacement is ready. Windows CRT rename rejects existing files. */
static inline int replace_save_file(const char* from, const char* to)
{
#ifdef _WIN32
    return MoveFileExA(from, to, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) ? 0 : -1;
#else
    return rename(from, to);
#endif
}

#endif
