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
uint32_t func_801E6830(uint8_t *o, int32_t s, uint16_t *m)
{ (void)o; (void)s; *m = 0; return 0; }
void func_801E6D94(uint8_t *o, uint8_t *n, int32_t f)
{ (void)o; (void)n; (void)f; }
int32_t func_801DC848(uint8_t *r, int32_t s) { (void)r; (void)s; return 0; }
int32_t func_801DC5C0(uint8_t *r, int32_t s) { (void)r; (void)s; return 0; }
void func_801E6974(uint8_t *o,uint8_t *p,uint8_t*n,int32_t a,int32_t b,int32_t c,int32_t d,int32_t e,int32_t f,int32_t g,int32_t h,int32_t i,int32_t j,int32_t k){(void)o;(void)p;(void)n;(void)a;(void)b;(void)c;(void)d;(void)e;(void)f;(void)g;(void)h;(void)i;(void)j;(void)k;}

static uint8_t *expected_obj, *expected_root, *expected_pose;
static int lookup_calls, apply_calls;
static int track_calls, tick_calls;
void func_801DFE8C(uint8_t *pool, uint8_t *root)
{ (void)pool; (void)root; }
void func_801DF52C(uint8_t *pool, uint8_t *root, int32_t index, int32_t mask)
{ (void)pool; (void)root; (void)index; (void)mask; }
int32_t func_801E632C(uint8_t *object)
{ (void)object; return -1; }
uint32_t func_801DF7F4(uint8_t *pool, uint8_t *root, uint8_t *pose, int32_t loop, int32_t tag)
{ assert(pool == NULL && root == expected_root && pose == expected_pose); assert(loop == 1 && tag == 0x11); ++track_calls; return 0; }
void func_801E5C74(uint8_t *object, uint8_t *data, int32_t loop)
{ assert(object == expected_obj && data == expected_pose); assert(loop == 1 || loop == -2); ++tick_calls; }
uint32_t func_801E6910(uint8_t *obj, int32_t index, uint32_t *flags)
{
    assert(obj == expected_obj && index == 0x5a);
    *flags = 0;
    ++lookup_calls;
    return (uint32_t)(uintptr_t)expected_pose;
}
void func_801DEF10(uint8_t *root, uint8_t *pose)
{
    assert(root == expected_root && pose == expected_pose);
    ++apply_calls;
}

static uint8_t *low_page(void)
{
    uint8_t *p = mmap(NULL, 0x1000, PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_32BIT, -1, 0);
    assert(p != MAP_FAILED);
    memset(p, 0, 0x1000);
    return p;
}

int main(void)
{
    uint8_t *obj = low_page(), *root = low_page(), *pose = low_page();
    uint8_t *stream = low_page();
    PcPortFieldClipControl state;
    expected_obj = obj; expected_root = root; expected_pose = pose;
    *(uint32_t *)(obj + 4) = (uint32_t)(uintptr_t)root;
    *(uint16_t *)stream = (uint16_t)((0x5a << 8) | 0x10);
    state = (PcPortFieldClipControl){.object = obj, .pool = NULL,
                                     .stream = (uint32_t)(uintptr_t)stream,
                                     .limit = 0, .ticks = 1, .running = 1,
                                     .postprocess = 0, .operand = 0};
    assert(PcPort_FieldClipDataStep(&state) == 1);
    assert(lookup_calls == 1 && apply_calls == 1);
    assert(state.stream == (uint32_t)(uintptr_t)(stream + 2));
    *(int16_t *)(obj + 0x1c) = 0x1000;
    *(int16_t *)(root + 0x50) = 0x1000;
    *(int16_t *)(pose + 0x10) = 0x1000;
    *(uint16_t *)stream = 0x5a11;
    *(uint16_t *)(stream + 2) = 0x0100;
    state.stream = (uint32_t)(uintptr_t)stream;
    assert(PcPort_FieldClipDataStep(&state) == 1);
    assert(track_calls == 1 && tick_calls == 1);
    *(uint16_t *)stream = 0x5a18;
    *(uint16_t *)(stream + 2) = 0xfffe;
    state.stream = (uint32_t)(uintptr_t)stream;
    assert(PcPort_FieldClipDataStep(&state) == 1);
    assert(tick_calls == 2);
    munmap(obj, 0x1000); munmap(root, 0x1000);
    munmap(pose, 0x1000); munmap(stream, 0x1000);
    puts("CLIP OPCODES 10/11/18 PASS pose/track retail call chains");
    return 0;
}
