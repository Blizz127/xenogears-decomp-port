/* SPDX-License-Identifier: MIT */
#include "battle_win32_host.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Direct Windows game calls use the pointer-returning port ABI. */
void *bzero(void *destination, size_t length)
{
    return memset(destination, 0, length);
}

/* The guest bzero call has two arguments and no return value. */
static void clear_bytes(void *destination, size_t length)
{
    memset(destination, 0, length);
}

void *PcPort_Win32MainImageSymbol(const char *name)
{
    if (!name || !*name) return NULL;

    HMODULE image = GetModuleHandleW(NULL);
    FARPROC exported = image ? GetProcAddress(image, name) : NULL;
    if (exported) return (void *)exported;

    /* Memory and string routines in the bridge catalog use the linked C
     * library. Game-owned exports take precedence, including its RNG. */
    static const struct { const char *name; void *entry; } library[] = {
        {"bzero", (void *)clear_bytes},
        {"memchr", (void *)memchr}, {"memcpy", (void *)memcpy},
        {"memmove", (void *)memmove}, {"memset", (void *)memset},
        {"printf", (void *)printf}, {"srand", (void *)srand},
        {"strcat", (void *)strcat}, {"strcmp", (void *)strcmp},
        {"strcpy", (void *)strcpy}, {"strlen", (void *)strlen}
    };
    for (size_t i = 0; i < sizeof(library) / sizeof(library[0]); ++i) {
        if (!strcmp(name, library[i].name)) return library[i].entry;
    }
    return NULL;
}

size_t PcPort_Win32DataSymbolSize(const char *name)
{
    static const struct { const char *name; size_t bytes; } sizes[] = {
#include "battle_win32_binding_sizes.inc"
    };
    if (!name || !*name) return 0;
    size_t begin = 0, end = sizeof(sizes) / sizeof(sizes[0]);
    while (begin < end) {
        size_t mid = begin + (end - begin) / 2;
        int order = strcmp(name, sizes[mid].name);
        if (!order) return sizes[mid].bytes;
        if (order < 0) end = mid;
        else begin = mid + 1;
    }
    return 0;
}

int PcPort_Win32HostAddressValid(uintptr_t address, unsigned width, int write)
{
    if (!address || !width || width - 1u > UINTPTR_MAX - address) return 0;
    MEMORY_BASIC_INFORMATION region;
    if (VirtualQuery((const void *)address, &region, sizeof(region)) != sizeof(region) ||
        region.State != MEM_COMMIT || (region.Protect & PAGE_GUARD)) return 0;

    uintptr_t begin = (uintptr_t)region.BaseAddress;
    if (address < begin || address - begin >= region.RegionSize ||
        width > region.RegionSize - (address - begin)) return 0;
    switch (region.Protect & 0xffu) {
    case PAGE_READONLY:
    case PAGE_EXECUTE_READ:
        return !write;
    case PAGE_READWRITE:
    case PAGE_WRITECOPY:
    case PAGE_EXECUTE_READWRITE:
    case PAGE_EXECUTE_WRITECOPY:
        return 1;
    default:
        return 0;
    }
}

uint32_t PcPort_Win32ThreadId(void)
{
    return (uint32_t)GetCurrentThreadId();
}

int PcPort_Win32BiosConsoleWrite(const void *bytes, unsigned length)
{
    if (!length) return 0;
    if (!bytes || length > INT_MAX) return -1;
    HANDLE output = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD written;
    if (!output || output == INVALID_HANDLE_VALUE ||
        !WriteFile(output, bytes, length, &written, NULL)) return -1;
    return (int)written;
}
