#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef uint32_t u32;
typedef struct { int16_t x,y,w,h; } RECT;
typedef struct { struct { uint8_t dtd; } drawEnvs[2]; } RenderContext;
static RenderContext g_FieldRenderContexts[2];
static int16_t D_800B2344;
static unsigned stage, base;
static u32 bank[4096];
static uintptr_t storedAddress;

static uint16_t pixel(unsigned x) { return x ? (uint16_t)(x | 0xc63u) : 0; }
static void *HeapAlloc(unsigned size, unsigned flags)
{
    assert(stage++ == 0 && size == sizeof bank && flags == 0);
    assert(g_FieldRenderContexts[0].drawEnvs[0].dtd == 0);
    assert(g_FieldRenderContexts[1].drawEnvs[0].dtd == 0);
    return bank;
}
static int StoreImage(RECT *r, void *p)
{
    assert(stage++ == 1 && p == bank && r->x == 0 && r->y == 480 && r->w == 256 && r->h == 32);
    storedAddress = (uintptr_t)r;
    for (unsigned i = 0; i < 4096; ++i) {
        unsigned lo = base+i, hi = 65535u-lo;
        bank[i] = (hi << 16) | lo;
    }
    return -7;
}
static int DrawSync(int mode) { assert((stage==2 || stage==4) && mode==0); ++stage; return -31; }
static int LoadImage(RECT *r, void *p)
{
    assert(stage++ == 3 && (uintptr_t)r == storedAddress && p == bank);
    for (unsigned i = 0; i < 4096; ++i)
        assert(bank[i] == (u32)(pixel(base+i) | ((u32)pixel(65535u-base-i) << 16)));
    return -19;
}
static unsigned HeapFree(void *p) { assert(stage++ == 5 && p == bank); return 123; }
#include "field_restore_vram.inc"
int main(void)
{
    g_FieldRenderContexts[0].drawEnvs[0].dtd=4;
    g_FieldRenderContexts[1].drawEnvs[0].dtd=8;
    func_80078C5C();
    assert(stage==0 && g_FieldRenderContexts[0].drawEnvs[0].dtd==4 && g_FieldRenderContexts[1].drawEnvs[0].dtd==8);
    for (base = 0; base <= 65535u; base += 4096u) {
        D_800B2344 = base ? -1 : 1;
        stage=0;
        func_80078C5C();
        assert(stage==6);
    }
    puts("pass: disabled path and 65,536 low/high pixel values with nonzero service returns");
    return 0;
}
