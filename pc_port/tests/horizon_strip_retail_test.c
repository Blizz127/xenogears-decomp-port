/* M1(b) retail-differential proof: func_800278F8 (horizon/line-scroll
 * backdrop strip draw, temp1f.c native body) vs the retail SLUS
 * instructions on the port MIPS interpreter, identical guest memory.
 * Compares the whole 0x34C heap block (8 FT4 packets x 2 buffers, header
 * fields) and the OT word. Scope: see sprite_draw_retail_harness.h. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "psyq/libgte.h"
#include "psx_memory.h"
uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
#include "sprite_draw_retail_harness.h"

extern void func_800278F8(uint8_t* ctx, int32_t scroll, int32_t screenY, int32_t fade, void* ot, int32_t renderCtx);

enum { O_CTX = 0x0000, CTX_SIZE = 0x34C, O_OT = 0x1000 };
static const char* label(uint32_t off, char* b)
{
    static const char* ft4[] = {"tag","tag","tag","len","rgb+code","rgb","rgb","code","x0","x0","y0","y0","u0","v0","clut","clut","x1","x1","y1","y1","u1","v1","tpage","tpage","x2","x2","y2","y2","u2","v2","pad","pad","x3","x3","y3","y3","u3","v3","pad","pad"};
    if (off < 0x280) { sprintf(b, "ft4[%u].%s", off / 0x28, ft4[off % 0x28]); return b; }
    if (off < CTX_SIZE) { sprintf(b, "ctx+0x%x", off); return b; }
    if (off >= O_OT && off < O_OT + 16) { sprintf(b, "OT"); return b; }
    sprintf(b, "other"); return b;
}
static char desc[200];
int main(int argc, char** argv)
{
    unsigned n = argc > 1 ? (unsigned)atoi(argv[1]) : 6000;
    load_slus("HORIZON STRIP"); map_low();
    static const int32_t widths[] = {1, 2, 7, 33, 64, 100, 255, 256, 300, 320, 321, 512, 640, 1024, 4096};
    static const int32_t heights[] = {1, 16, 64, 100, 128, 200, 240, 256};
    static const int32_t fades[] = {-255, -128, -1, 0, 1, 64, 255, 256, 512, 1000, 3000};
    static const int32_t tpxs[] = {0, 64, 128, 320, 0x3C0, 37, 1, 63};
    static const int32_t tpys[] = {0, 1, 256, 255, 511, 0x1F0};
    unsigned pass = 0, fail = 0, first = 1; unsigned west_like = 0; unsigned fadeFail[11] = {0}, fadeTot[11] = {0}, shown = 0;
    for (unsigned id = 0; id < n; id++) {
        seed_case(id * 5 + 11);
        for (uint32_t i = 0; i < LOW_SIZE; i++) low[i] = 0xA5;
        for (uint32_t i = 0; i < CTX_SIZE; i++) low[O_CTX + i] = (uint8_t)rnd();
        unsigned fi = rn(11);
        int32_t width = widths[rn(15)], height = heights[rn(8)], fade = fades[fi];
        int32_t tpx = tpxs[rn(8)], tpy = tpys[rn(6)], abr = rn(3), scroll = rn(4) ? rrange(-700, 700) : (int32_t)rnd() >> rn(16);
        int32_t screenY = (int32_t[]){-100, -1, 0, 1, 100, 239, 240, 300, height, height + 0xF0, height + 0xF1, height + 0xF0 + 1}[rn(12)];
        int32_t rctx = rn(5);
        put32(low + O_CTX + 0x328, (uint32_t)width); put32(low + O_CTX + 0x32C, (uint32_t)height);
        put16(low + O_CTX + 0x334, (uint32_t)tpx); put16(low + O_CTX + 0x336, (uint32_t)tpy); put16(low + O_CTX + 0x338, (uint32_t)abr);
        low[O_CTX + 0x33A] = (uint8_t)rnd();
        put32(low + O_OT, rnd());
        void* ot = low + O_OT;
        snprintf(desc, sizeof desc, "w=%d h=%d fade=%d tp=(%d,%d) abr=%d scroll=%d screenY=%d rctx=%d", width, height, fade, tpx, tpy, abr, scroll, screenY, rctx);
        if (scroll < 0 || tpx < 0) west_like++;
        fadeTot[fi]++;
        memcpy(low_init, low, LOW_SIZE);
        retail_stack_args[0] = lo_addr(O_OT); retail_stack_args[1] = (uint32_t)rctx; retail_nstack = 2;
        retail_call(0x800278f8u, lo_addr(O_CTX), (uint32_t)scroll, (uint32_t)screenY, (uint32_t)fade, "HORIZON STRIP");
        memcpy(low_retail, low, LOW_SIZE);
        memcpy(low, low_init, LOW_SIZE);
        func_800278F8(low + O_CTX, scroll, screenY, fade, ot, rctx);
        if (memcmp(low, low_retail, LOW_SIZE)) {
            fail++; fadeFail[fi]++;
            if (first || (shown < 3 && fade >= 0 && fade <= 256)) { first = 0; shown++; prints_left = 1; diff_low("HORIZON STRIP", "func_800278F8", id, desc, low, low_retail, label); }
        } else pass++;
    }
    for (unsigned i = 0; i < 11; i++) printf("  fade=%d: %u/%u diverge\n", fades[i], fadeFail[i], fadeTot[i]);
    printf("HORIZON STRIP func_800278F8 %s pass=%u fail=%u (scroll<0 or tpX<0 cases: %u)\n", fail ? "DIVERGES" : "PASS", pass, fail, west_like);
    return fail ? 1 : 0;
}
