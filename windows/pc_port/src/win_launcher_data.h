#ifndef XENO_WIN_LAUNCHER_DATA_H
#define XENO_WIN_LAUNCHER_DATA_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

static inline int PcPort_WinRegularFileIn(const char* dir, const char* name,
                                          char* out, size_t capacity)
{
    struct _stat64 st;
    int n = snprintf(out, capacity, "%s/%s", dir, name);
    return n >= 0 && (size_t)n < capacity &&
           _stat64(out, &st) == 0 && (st.st_mode & _S_IFMT) == _S_IFREG;
}

/* Banshee supplies a prepared directory, either flat or containing disc/.
 * As in xenogears.sh, SLUS selects one layout for all extracted files. */
static inline int PcPort_WinPreparedDataDir(const char* root, char* out, size_t capacity)
{
    struct _stat64 st;
    char check[1100];
    if (!root || !*root || _stat64(root, &st) != 0 || (st.st_mode & _S_IFMT) != _S_IFDIR)
        return 0;
    const char* suffix = "";
    if (!PcPort_WinRegularFileIn(root, "SLUS_006.64", check, sizeof check) &&
         PcPort_WinRegularFileIn(root, "disc/SLUS_006.64", check, sizeof check))
        suffix = "/disc";
    int n = snprintf(out, capacity, "%s%s", root, suffix);
    return n >= 0 && (size_t)n < capacity;
}

#endif
