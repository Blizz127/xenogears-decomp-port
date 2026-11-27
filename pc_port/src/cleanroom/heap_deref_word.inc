#include <stdint.h>

void *HeapDerefPtr(u_int *data)
{
    uint32_t address = *(const volatile u_int *)data;
    return (void *)(uintptr_t)address;
}
