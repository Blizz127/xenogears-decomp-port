#include "common.h"


extern void* HeapAlloc(u32 allocSize, u32 allocFlags);
extern void func_800B7424(u8* p);


/* func_800B73EC.s */
void func_800B73EC(void) {
    u8* p = HeapAlloc(0x10F7C, 1);

    *(u32*)(p + 4) = (u32)(unsigned long)p;
    *(u32*)(p + 0x20) = (u32)(unsigned long)p;
    func_800B7424(p);
}
