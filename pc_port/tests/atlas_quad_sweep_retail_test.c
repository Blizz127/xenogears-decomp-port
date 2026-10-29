/* M1(b) retail-differential proof: func_80025FA8 (atlas entry -> double
 * buffered POLY_FT4 packets, temp1f.c native body) vs retail SLUS
 * instructions on the port MIPS interpreter, identical guest memory.
 * Wider sweep than atlas_quad_retail_test: X/Y flip flags x every rotation
 * class x mirrored (negative) scales x both packet buffers x item counts.
 * Retail libgte (PushMatrix/RotMatrixZ/ScaleMatrixL/RotTransPers4/...) is
 * INTERPRETED; native uses the PsyCross/compat versions. */
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

extern int32_t func_80025FA8(uint8_t*, int32_t, uint8_t*, int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
enum { O_TBL = 0x0000, O_PK = 0x2000 };
static const char* label(uint32_t off, char* b)
{
    static const char* ft4[] = {"tag","tag","tag","len","rgb+code","rgb","rgb","code","x0","x0","y0","y0","u0","v0","clut","clut","x1","x1","y1","y1","u1","v1","tpage","tpage","x2","x2","y2","y2","u2","v2","pad","pad","x3","x3","y3","y3","u3","v3","pad","pad"};
    if (off >= O_PK) { uint32_t o = off - O_PK, pk = o / 80, buf = (o % 80) / 40; sprintf(b, "packet[item %u buf %u].%s", pk, buf, ft4[o % 40]); return b; }
    sprintf(b, "table+0x%x", off); return b;
}
static char desc[200];
int main(int argc, char** argv)
{
    unsigned n = argc > 1 ? (unsigned)atoi(argv[1]) : 6000;
    load_slus("ATLAS SWEEP"); map_low();
    static const int32_t rots[] = {0, 0x400, 0x800, 0xC00, 4095, 0x1000, -0x400, 0x123, 0xC01, 0xBFF, 0x2800, 1};
    static const int32_t scs[] = {0x1000, 0x800, 0x2000, -0x1000, -0x800, 0x1800, 1, 0x7FFF, -0x8000, 0x400};
    unsigned pass = 0, fail = 0, flipfail = 0, flipcases = 0, rotfail[12] = {0}, rottot[12] = {0};
    uint8_t corners0[32]; memcpy(corners0, g_PsxRam + 0x4fdc0, 32);
    for (unsigned id = 0; id < n; id++) {
        seed_case(id * 3 + 17);
        for (uint32_t i = 0; i < LOW_SIZE; i++) low[i] = 0xA5;
        int32_t index = rn(4); unsigned count = rn(7) ? rrange(1, 5) : (rn(2) ? 0 : 6);
        for (int k = 0; k < 8; k++) put16(low + O_TBL + 4 + 2 * k, 0x80 + 0x80 * k);
        uint8_t* entry = low + O_TBL + 0x80 + 0x80 * 0; /* entry for index 0; others alias their own slots */
        unsigned slot = 0x80 + 0x80 * index; put16(low + O_TBL + 4 + 2 * index, slot); entry = low + O_TBL + slot; (void)entry;
        put16(entry, count);
        unsigned anyflip = 0;
        for (unsigned it = 0; it < count; it++) {
            uint8_t* m = entry + 4 + it * 28; memset(m, 0, 28);
            put16(m + 0, rn(4) ? rn(256) : rn(3)); put16(m + 2, rn(4) ? rn(256) : rn(3));
            put16(m + 4, rrange(1, 64)); put16(m + 6, rrange(1, 64)); put16(m + 8, (uint32_t)rrange(-100, 100)); put16(m + 10, (uint32_t)rrange(-100, 100));
            put16(m + 16, rn(2)); put16(m + 18, rn(256)); put16(m + 20, rn(512)); put16(m + 22, rn(1024)); put16(m + 24, rn(512));
            m[26] = rn(3) == 0; m[27] = rn(4) == 0; anyflip |= m[26] | m[27];
        }
        int32_t sx = rrange(0, 320), sy = rrange(0, 240), scx = scs[rn(10)], scy = scs[rn(10)], rot = rots[rn(12)], buf = rn(2);
        if (rn(3) == 0) scy = scx;
        unsigned ri = 0; while (ri < 12 && rots[ri] != rot) ri++;
        snprintf(desc, sizeof desc, "index=%d count=%u buf=%d scale=(%d,%d) angle=%d flips=%u screen=(%d,%d)", index, count, buf, scx, scy, rot, anyflip, sx, sy);
        GteSetup g; memset(&g, 0, sizeof g); g.rot[0] = g.rot[4] = g.rot[8] = (int16_t)rrange(1000, 4096); g.tr[0] = rrange(-100, 100); g.tr[1] = rrange(-100, 100); g.tr[2] = rrange(0, 500);
        g.ofx = rrange(0, 320) << 16; g.ofy = rrange(0, 240) << 16; g.h = 100 + rn(900);
        memcpy(g_PsxRam + 0x4fdc0, corners0, 32);
        memcpy(low_init, low, LOW_SIZE);
        retail_stack_args[0] = (uint32_t)sx; retail_stack_args[1] = (uint32_t)sy; retail_stack_args[2] = (uint32_t)scx; retail_stack_args[3] = (uint32_t)scy; retail_stack_args[4] = (uint32_t)rot; retail_nstack = 5;
        gte_setup(&g);
        retail_call(0x80025fa8u, lo_addr(O_TBL), (uint32_t)index, lo_addr(O_PK), (uint32_t)buf, "ATLAS SWEEP");
        memcpy(low_retail, low, LOW_SIZE); uint32_t rv = retail_v0; GTERegisters rg = gteRegs;
        uint8_t rc[32]; memcpy(rc, g_PsxRam + 0x4fdc0, 32);
        memcpy(low, low_init, LOW_SIZE); memcpy(g_PsxRam + 0x4fdc0, corners0, 32);
        gte_setup(&g);
        int32_t nv = func_80025FA8(low + O_TBL, index, low + O_PK, buf, sx, sy, scx, scy, rot);
        int bad = memcmp(low, low_retail, LOW_SIZE) || (uint32_t)nv != rv || memcmp(rc, g_PsxRam + 0x4fdc0, 32);
        flipcases += anyflip; rottot[ri]++;
        if (bad) {
            fail++; flipfail += anyflip; rotfail[ri]++;
            if (fail <= 2) { prints_left = 1; if (!diff_low("ATLAS SWEEP", "func_80025FA8", id, desc, low, low_retail, label)) fprintf(stderr, "ATLAS SWEEP FAIL func_80025FA8 case=%u [%s] ret/corner-scratch differ native=%d retail=%u\n", id, desc, nv, rv); }
        } else pass++;
        if (!bad && memcmp(&gteRegs, &rg, sizeof rg)) gte_only_diffs++;
    }
    for (unsigned i = 0; i < 12; i++) printf("  angle=%d: %u/%u diverge\n", rots[i], rotfail[i], rottot[i]);
    printf("ATLAS SWEEP func_80025FA8 %s pass=%u fail=%u (flip cases %u, flip fails %u; GTE-register-only diffs=%u informational)\n", fail ? "DIVERGES" : "PASS", pass, fail, flipcases, flipfail, gte_only_diffs);
    return fail ? 1 : 0;
}
