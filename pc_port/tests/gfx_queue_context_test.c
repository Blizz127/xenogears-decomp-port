#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef uint8_t u8;
typedef uint32_t u32;
typedef int32_t s32;

u32 g_GfxCurWorkBuffer;
s32 g_GfxCurContext;
u32 D_80059300[2];
u32 D_80059304;
static u32 work[8];

/* Compiled from the actual port override by the accompanying runner. */
void func_80025180(void *data);

int main(void)
{
    assert((uintptr_t)work <= UINT32_MAX);
    D_80059300[0] = 0x1234;
    D_80059300[1] = 0x5678; /* deliberately not the context-1 head */
    D_80059304 = 0x9abc;
    g_GfxCurWorkBuffer = (u32)(uintptr_t)work;
    g_GfxCurContext = 0;
    func_80025180((void *)(uintptr_t)0x10203040);
    assert(work[0] == 0x10203040 && work[1] == 0x1234);
    assert(D_80059300[0] == (u32)(uintptr_t)work);
    assert(g_GfxCurWorkBuffer == (u32)(uintptr_t)(work + 2));
    g_GfxCurContext = 1;
    func_80025180((void *)(uintptr_t)0x50607080);
    assert(work[2] == 0x50607080 && work[3] == 0x9abc);
    assert(D_80059304 == (u32)(uintptr_t)(work + 2));
    assert(D_80059300[1] == 0x5678);
    assert(g_GfxCurWorkBuffer == (u32)(uintptr_t)(work + 4));
    puts("graphics deferred queue: both context heads passed");
    return 0;
}
