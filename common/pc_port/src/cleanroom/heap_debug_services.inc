#include <stdarg.h>
#include <stdio.h>

void HeapPrintf(char *format, ...)
{
    char line[1024];
    va_list arguments;
    va_start(arguments, format);
    int length = vsnprintf(line, sizeof(line), format, arguments);
    va_end(arguments);

    if (length < 0) {
        line[0] = '\0';
    }
    FnPrintf_t *sink = g_HeapDebugPrintfFn;
    if (sink) {
        sink("%s", line);
    }
}

int HeapLoadSymbols(char *path)
{
    PCinit();
    int file = PCopen(path, 0, 0);
    if (file == -1) {
        return -1;
    }
    int size = PClseek(file, 0, SEEK_END);
    PClseek(file, 0, SEEK_SET);

    u_char *allocation = NULL;
    int received = 0;
    if (size >= 4) {
        HeapSetCurrentContentType(HEAP_CONTENT_SYMBOL_DATA);
        allocation = HeapAlloc((u_int)size, 0);
        if (allocation) {
            g_SymbolData = allocation;
            g_SymbolDataEndAddress = allocation + size;
            while (received < size) {
                int count = size - received;
                if (count > 32768) {
                    count = 32768;
                }
                if (PCread(file, (char *)allocation + received, count) != count) {
                    break;
                }
                received += count;
            }
        }
    }
    PCclose(file);

    if (allocation && received == size) {
        const volatile u_char *magic = g_SymbolData;
        if (magic && magic[0] == 'S' && magic[1] == 'Y' &&
            magic[2] == 'M' && magic[3] == '1') {
            return 0;
        }
        allocation = (u_char *)magic;
    }
    if (allocation) {
        HeapFree(allocation);
    }
    g_SymbolData = NULL;
    g_SymbolDataEndAddress = NULL;
    return 0;
}

void HeapGetSymbolNameFromAddress(mem_addr address, u_char *out)
{
    uintptr_t start = (uintptr_t)g_SymbolData;
    uintptr_t finish = (uintptr_t)g_SymbolDataEndAddress;
    const volatile u_char *selected = NULL;
    if (start && finish >= start && finish - start >= 4u) {
        uintptr_t cursor = start + 4u;
        while (cursor < finish) {
            if (finish - cursor < 5u) {
                selected = NULL;
                break;
            }
            const volatile u_char *record = (const volatile u_char *)cursor;
            u32 key = record[0];
            key |= (u32)record[1] << 8;
            key |= (u32)record[2] << 16;
            key |= (u32)record[3] << 24;
            if (key >= address) {
                break;
            }
            unsigned length = record[4];
            if (finish - cursor - 5u < length) {
                selected = NULL;
                break;
            }
            selected = record + 4;
            cursor += 5u + length;
        }
    }
    unsigned length = selected ? *selected : 0u;
    if (length > 63u) {
        length = 63u;
    }
    volatile u_char *destination = out;
    for (unsigned i = 0; i < length; ++i) {
        destination[i] = selected[i + 1u];
    }
    destination[length] = 0;
}
