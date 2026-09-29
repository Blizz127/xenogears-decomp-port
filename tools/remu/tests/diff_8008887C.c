/* Differential test: host-compiled src/battle/main40.c func_8008887C
 * vs the retail bytes (asm/battle/nonmatchings/main40/func_8008887C.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_8008887C(x0, y0, x1, y1): records the endpoints, early-returns
 * when x1==x0 or y1==y0, else records signed direction flags
 * (D_800C3A94/D_800C3A98), the 0x100-pinned axis step and the
 * (other<<8)/dominant quotient (D_800C3A8C/D_800C3A90), zeroes the
 * D_800C2080/D_800C2084 accumulators, samples func_8001BD40(1, 8, x1, y1)
 * into D_800C3A9C, and clears D_800C207C.
 *
 * Callee control: func_8001BD40 is a main-exe routine built on rand(),
 * so it is stubbed on BOTH sides (remu forces v0=0 for unloaded jal
 * targets; the host stub returns 0). The test asserts the exact stub
 * count matches (0 on early return, 1 otherwise) and the only stub
 * target is 0x8001BD40. No GTE/cop1/HW regs.
 *
 * Seed discipline: non-equal endpoint pairs stay within +-2^20 so the
 * (d<<8) shift and signed divide stay host-UB-free; equal pairs use
 * full-range extremes (they never reach the divide).
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8008887C src/battle/main40.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main40.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_8008887C(s32 x0, s32 y0, s32 x1, s32 y1);

/* TU externs the test must provide (only what func_8008887C touches). */
static u8 backing[64];
__asm__(".globl D_800C3A7C\n.set D_800C3A7C, backing+0x00");
__asm__(".globl D_800C3A80\n.set D_800C3A80, backing+0x04");
__asm__(".globl D_800C3A84\n.set D_800C3A84, backing+0x08");
__asm__(".globl D_800C3A88\n.set D_800C3A88, backing+0x0C");
__asm__(".globl D_800C3A8C\n.set D_800C3A8C, backing+0x10");
__asm__(".globl D_800C3A90\n.set D_800C3A90, backing+0x14");
__asm__(".globl D_800C3A94\n.set D_800C3A94, backing+0x18");
__asm__(".globl D_800C3A98\n.set D_800C3A98, backing+0x19");
__asm__(".globl D_800C3A9C\n.set D_800C3A9C, backing+0x1C");
__asm__(".globl D_800C207C\n.set D_800C207C, backing+0x20");
__asm__(".globl D_800C2080\n.set D_800C2080, backing+0x24");
__asm__(".globl D_800C2084\n.set D_800C2084, backing+0x28");

/* Dual-side stub for the rand()-based main-exe callee (always 0). */
static int host_stubs;
u32 func_8001BD40(u32 a0, u32 a1, u32 a2, u32 a3) {
    (void)a0; (void)a1; (void)a2; (void)a3;
    host_stubs++;
    return 0;
}

#define RETAIL_S "asm/battle/nonmatchings/main40/func_8008887C.s"

typedef struct {
    s32 x0, y0, x1, y1;
    s32 stepx, stepy;
    u8 dirx, diry;
    s32 n;
    u8 flag;
    s32 accx, accy;
} state_t;

static void write_host(const state_t *s) {
    memcpy(backing + 0x00, &s->x0, 4);
    memcpy(backing + 0x04, &s->y0, 4);
    memcpy(backing + 0x08, &s->x1, 4);
    memcpy(backing + 0x0C, &s->y1, 4);
    memcpy(backing + 0x10, &s->stepx, 4);
    memcpy(backing + 0x14, &s->stepy, 4);
    backing[0x18] = s->dirx;
    backing[0x19] = s->diry;
    memcpy(backing + 0x1C, &s->n, 4);
    backing[0x20] = s->flag;
    memcpy(backing + 0x24, &s->accx, 4);
    memcpy(backing + 0x28, &s->accy, 4);
}

static void read_host(state_t *s) {
    memset(s, 0, sizeof(*s));
    memcpy(&s->x0, backing + 0x00, 4);
    memcpy(&s->y0, backing + 0x04, 4);
    memcpy(&s->x1, backing + 0x08, 4);
    memcpy(&s->y1, backing + 0x0C, 4);
    memcpy(&s->stepx, backing + 0x10, 4);
    memcpy(&s->stepy, backing + 0x14, 4);
    s->dirx = backing[0x18];
    s->diry = backing[0x19];
    memcpy(&s->n, backing + 0x1C, 4);
    s->flag = backing[0x20];
    memcpy(&s->accx, backing + 0x24, 4);
    memcpy(&s->accy, backing + 0x28, 4);
}

static int poke_retail(remu_t *m, const state_t *s) {
    if (remu_poke(m, 0x800C3A7Cu, &s->x0, 4)) return -1;
    if (remu_poke(m, 0x800C3A80u, &s->y0, 4)) return -1;
    if (remu_poke(m, 0x800C3A84u, &s->x1, 4)) return -1;
    if (remu_poke(m, 0x800C3A88u, &s->y1, 4)) return -1;
    if (remu_poke(m, 0x800C3A8Cu, &s->stepx, 4)) return -1;
    if (remu_poke(m, 0x800C3A90u, &s->stepy, 4)) return -1;
    if (remu_poke(m, 0x800C3A94u, &s->dirx, 1)) return -1;
    if (remu_poke(m, 0x800C3A98u, &s->diry, 1)) return -1;
    if (remu_poke(m, 0x800C3A9Cu, &s->n, 4)) return -1;
    if (remu_poke(m, 0x800C207Cu, &s->flag, 1)) return -1;
    if (remu_poke(m, 0x800C2080u, &s->accx, 4)) return -1;
    if (remu_poke(m, 0x800C2084u, &s->accy, 4)) return -1;
    return 0;
}

static int peek_retail(remu_t *m, state_t *s) {
    memset(s, 0, sizeof(*s));
    if (remu_peek(m, 0x800C3A7Cu, &s->x0, 4)) return -1;
    if (remu_peek(m, 0x800C3A80u, &s->y0, 4)) return -1;
    if (remu_peek(m, 0x800C3A84u, &s->x1, 4)) return -1;
    if (remu_peek(m, 0x800C3A88u, &s->y1, 4)) return -1;
    if (remu_peek(m, 0x800C3A8Cu, &s->stepx, 4)) return -1;
    if (remu_peek(m, 0x800C3A90u, &s->stepy, 4)) return -1;
    if (remu_peek(m, 0x800C3A94u, &s->dirx, 1)) return -1;
    if (remu_peek(m, 0x800C3A98u, &s->diry, 1)) return -1;
    if (remu_peek(m, 0x800C3A9Cu, &s->n, 4)) return -1;
    if (remu_peek(m, 0x800C207Cu, &s->flag, 1)) return -1;
    if (remu_peek(m, 0x800C2080u, &s->accx, 4)) return -1;
    if (remu_peek(m, 0x800C2084u, &s->accy, 4)) return -1;
    return 0;
}

static int check_one(remu_t *m, uint32_t entry, s32 x0, s32 y0, s32 x1,
                     s32 y1, const state_t *pre, int id) {
    state_t want, got;

    /* host side */
    write_host(pre);
    host_stubs = 0;
    func_8008887C(x0, y0, x1, y1);
    read_host(&want);
    int want_stubs = host_stubs;

    /* retail side */
    if (poke_retail(m, pre)) { printf("FAIL [%d] poke\n", id); return 0; }
    int rc = remu_call(m, entry, (uint32_t)x0, (uint32_t)y0,
                       (uint32_t)x1, (uint32_t)y1);
    if (rc != 0) {
        printf("FAIL [%d] (%d,%d)->(%d,%d) remu rc=%d stubs=%d%s\n", id,
               x0, y0, x1, y1, rc, remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    int expect_stubs = (x1 == x0 || y1 == y0) ? 0 : 1;
    if (remu_stub_calls(m) != expect_stubs || want_stubs != expect_stubs) {
        printf("FAIL [%d] stub count retail=%d host=%d expect=%d%s\n", id,
               remu_stub_calls(m), want_stubs, expect_stubs,
               remu_stub_log(m));
        return 0;
    }
    if (expect_stubs && strcmp(remu_stub_log(m), " 8001BD40") != 0) {
        printf("FAIL [%d] wrong stub target%s\n", id, remu_stub_log(m));
        return 0;
    }
    if (peek_retail(m, &got)) { printf("FAIL [%d] peek\n", id); return 0; }
    if (memcmp(&got, &want, sizeof(got)) != 0) {
        printf("FAIL [%d] (%d,%d)->(%d,%d) state mismatch\n", id,
               x0, y0, x1, y1);
        printf("  dirx r=%02X h=%02X diry r=%02X h=%02X stepx r=%08X h=%08X stepy r=%08X h=%08X\n",
               got.dirx, want.dirx, got.diry, want.diry,
               (unsigned)got.stepx, (unsigned)want.stepx,
               (unsigned)got.stepy, (unsigned)want.stepy);
        printf("  n r=%08X h=%08X flag r=%02X h=%02X accx r=%08X h=%08X accy r=%08X h=%08X\n",
               (unsigned)got.n, (unsigned)want.n, got.flag, want.flag,
               (unsigned)got.accx, (unsigned)want.accx,
               (unsigned)got.accy, (unsigned)want.accy);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x8008887Cu) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0, id = 0;
    state_t pre;
    memset(&pre, 0, sizeof(pre));

    /* early-return edges (full-range extremes never reach the divide) */
    s32 big[] = { 0, 1, -1, 0x7FFFFFFF, (s32)0x80000000, 0x12345678,
                  (s32)0xEDCBA988 };
    for (unsigned i = 0; i < sizeof(big) / sizeof(big[0]); i++) {
        pre.stepx = (s32)0xDEAD0000u | (s32)i;
        pre.stepy = (s32)0xBEEF0000u | (s32)i;
        pre.dirx = (u8)(0xA0 + i); pre.diry = (u8)(0xB0 + i);
        pre.n = (s32)(0x1000 + i); pre.flag = (u8)(i + 1);
        pre.accx = (s32)(0x11111111u * (i + 1));
        pre.accy = (s32)(0x22222222u * (i + 1));
        if (!check_one(m, entry, big[i], big[(i + 1) % 7], big[i],
                       big[(i + 3) % 7], &pre, id++)) return 1; /* x1==x0 */
        n++;
        if (!check_one(m, entry, big[(i + 1) % 7], big[i], big[(i + 3) % 7],
                       big[i], &pre, id++)) return 1; /* y1==y0 */
        n++;
    }
    /* live edges: axis-aligned deltas, diagonal, dx<dy / dx>dy / dx==dy */
    s32 pts[] = { 0, 1, -1, 2, -2, 100, -100, 1000, -1000, 0xFFFFF,
                  -0xFFFFF, 0x100000, -0x100000 };
    for (unsigned i = 0; i < sizeof(pts) / sizeof(pts[0]); i++) {
        for (unsigned k = 0; k < sizeof(pts) / sizeof(pts[0]); k++) {
            memset(&pre, 0, sizeof(pre));
            pre.flag = 0x77;
            pre.accx = 0x33333333; pre.accy = (s32)0xCCCCCCCC;
            if (!check_one(m, entry, 0, 0, pts[i], pts[k], &pre, id++))
                return 1;
            n++;
            if (!check_one(m, entry, pts[i], pts[k], 0, 0, &pre, id++))
                return 1;
            n++;
        }
    }
    /* deterministic pseudo-random sweep, +-2^20 discipline */
    uint32_t s = 0x8887087Cu;
    for (int i = 0; i < 300; i++) {
#define R32() (s = s * 1103515245u + 12345u, s)
#define R20() ((s32)(R32() & 0x1FFFFFu) - 0x100000)
        s32 x0 = R20(), y0 = R20(), x1 = R20(), y1 = R20();
        memset(&pre, 0, sizeof(pre));
        pre.stepx = (s32)R32(); pre.stepy = (s32)R32();
        pre.dirx = (u8)R32(); pre.diry = (u8)R32();
        pre.n = (s32)R32(); pre.flag = (u8)R32();
        pre.accx = (s32)R32(); pre.accy = (s32)R32();
        if (!check_one(m, entry, x0, y0, x1, y1, &pre, id++)) return 1;
        n++;
#undef R20
#undef R32
    }
    printf("DIFF 8008887C OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
