#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include "field_clip_control.h"
uint32_t D_801E8670[10];
uint32_t D_801E85CC;
uint32_t func_801DF0B4(uint8_t *pool, uint8_t *root, uint8_t *pose,
                     int32_t duration, int32_t absolute, int32_t loop, int32_t tag)
{
    (void)pool; (void)root; (void)pose; (void)duration;
    (void)absolute; (void)loop; (void)tag;
    assert(!"opcode 13 requires the focused blend fixture"); return 0;
}
uint32_t func_801E6830(uint8_t*o,int32_t s,uint16_t*m){(void)o;(void)s;*m=0;return 0;}
void func_801E6D94(uint8_t*o,uint8_t*n,int32_t f){(void)o;(void)n;(void)f;}
void func_801E6974(uint8_t*o,uint8_t*p,uint8_t*n,int32_t a,int32_t b,int32_t c,int32_t d,int32_t e,int32_t f,int32_t g,int32_t h,int32_t i,int32_t j,int32_t k){(void)o;(void)p;(void)n;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;}
int32_t func_801DC848(uint8_t*r,int32_t s){(void)r;(void)s;return 0;}
int32_t func_801DC5C0(uint8_t*r,int32_t s){(void)r;(void)s;return 0;}
static uint8_t *want_pool, *want_root, *want_obj;
static int release_calls, sentinel_calls, indexed_calls, indexed_index, indexed_mask;
static uint16_t get16(const uint8_t *p) { uint16_t v; memcpy(&v, p, 2); return v; }
static uint32_t get32(const uint8_t *p) { uint32_t v; memcpy(&v, p, 4); return v; }
uint32_t func_801E6910(uint8_t *obj, int32_t index, uint32_t *flags)
{ (void)obj; (void)index; *flags = 0; return 0; }
void func_801DEF10(uint8_t *root, uint8_t *pose)
{ (void)root; (void)pose; }
void func_801DFE8C(uint8_t *pool, uint8_t *root)
{ assert(pool == want_pool && root == want_root); ++release_calls; }
int32_t func_801E632C(uint8_t *object)
{ assert(object == want_obj); ++sentinel_calls; return -1; }
void func_801DF52C(uint8_t *pool, uint8_t *root, int32_t index, int32_t mask)
{ assert(pool == want_pool && root == want_root); ++indexed_calls; indexed_index = index; indexed_mask = mask; }
uint32_t func_801DF7F4(uint8_t *pool, uint8_t *root, uint8_t *pose, int32_t loop, int32_t tag)
{ (void)pool; (void)root; (void)pose; (void)loop; (void)tag; return 0; }
void func_801E5C74(uint8_t *object, uint8_t *data, int32_t loop)
{ (void)object; (void)data; (void)loop; }
static uint8_t *page(void)
{
    uint8_t *p = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    assert(p != MAP_FAILED); memset(p, 0, 0x1000); return p;
}
int main(void)
{
    uint8_t *obj = page(), *pool = page(), *root = page(), *stream = page();
    PcPortFieldClipControl state;
    want_obj = obj; want_pool = pool; want_root = root;
    *(uint32_t *)(obj + 4) = (uint32_t)(uintptr_t)root;
    *(uint16_t *)stream = 0x0008;
    state = (PcPortFieldClipControl){.object=obj, .pool=pool,
        .stream=(uint32_t)(uintptr_t)stream, .limit=0, .ticks=1,
        .running=1, .postprocess=0, .operand=0};
    assert(PcPort_FieldClipDataStep(&state) == 1);
    assert(release_calls == 1 && sentinel_calls == 1);
    assert(state.stream == (uint32_t)(uintptr_t)(stream + 2));
    *(uint16_t *)(root + 0x0a) = 3;
    for (unsigned i = 1; i < 3; ++i) {
        uint8_t *node = root + i * 124u;
        memset(node + 0x4f, 0xa5, 0x30);
    }
    *(uint16_t *)stream = 0x000b;
    state.stream = (uint32_t)(uintptr_t)stream;
    assert(PcPort_FieldClipDataStep(&state) == 1);
    assert(release_calls == 2 && sentinel_calls == 1);
    for (unsigned i = 1; i < 3; ++i) {
        uint8_t *node = root + i * 124u;
        assert(get16(node + 0x4f) == 0);
        assert(get16(node + 0x51) == 0);
        assert(get16(node + 0x53) == 0);
        assert(get32(node + 0x57) == 0);
        assert(get32(node + 0x5b) == 0);
        assert(get32(node + 0x5f) == 0);
        assert(node[0x7c] == 1 && node[0x7d] == 1);
    }
    for (unsigned opcode = 0x0a; opcode <= 0x0e; ++opcode) {
        if (opcode == 0x0b || opcode == 0x0c) continue;
        *(uint16_t *)stream = (uint16_t)((0x5a << 8) | opcode);
        state.stream = (uint32_t)(uintptr_t)stream;
        assert(PcPort_FieldClipDataStep(&state) == 1);
        assert(indexed_index == 0x5a);
        assert(indexed_mask == (opcode == 0x0a ? 7 : (opcode == 0x0d ? 1 : 2)));
    }
    *(uint16_t *)stream = 0x0019;
    state.stream = (uint32_t)(uintptr_t)stream;
    assert(PcPort_FieldClipDataStep(&state) == 1);
    assert(sentinel_calls == 2);
    munmap(obj,0x1000); munmap(pool,0x1000); munmap(root,0x1000); munmap(stream,0x1000);
    puts("CLIP OPCODE 08 PASS pool release/root sentinel call chain");
    return 0;
}
