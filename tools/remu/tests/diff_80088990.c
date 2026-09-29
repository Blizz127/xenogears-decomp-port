/* Differential test: host-compiled src/battle/main40.c func_80088990
 * vs the retail bytes (asm/battle/nonmatchings/main40/func_80088990.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * func_80088990(): advances the D_800C2080/D_800C2084 accumulators
 * D_800C3A9C times by the D_800C3A8C/D_800C3A90 steps (subtracting when
 * the D_800C3A94/D_800C3A98 direction flags are set), then sets the
 * D_800C207C arrival flag when the (256-scaled, trunc-div) accumulator
 * passes the D_800C3A7C..D_800C3A84 / D_800C3A80..D_800C3A88 endpoint.
 * Pure leaf: no callees, no GTE/cop1/HW regs.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_80088990 src/battle/main40.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (main40.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
void func_80088990(void);

/* TU externs the test must provide (only what func_80088990 touches). */
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

#define RETAIL_S "asm/battle/nonmatchings/main40/func_80088990.s"

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

static int check_one(remu_t *m, uint32_t entry, const state_t *in, int id) {
    state_t want, got;
    memset(&want, 0, sizeof(want));
    memset(&got, 0, sizeof(got));

    /* host side */
    write_host(in);
    func_80088990();
    read_host(&want);

    /* retail side */
    if (poke_retail(m, in)) { printf("FAIL [%d] poke\n", id); return 0; }
    int rc = remu_call(m, entry, 0, 0, 0, 0);
    if (rc != 0) {
        printf("FAIL [%d] remu rc=%d stubs=%d%s\n", id, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL [%d] unexpected stub calls%s\n", id, remu_stub_log(m));
        return 0;
    }
    if (peek_retail(m, &got)) { printf("FAIL [%d] peek\n", id); return 0; }
    if (memcmp(&got, &want, sizeof(got)) != 0) {
        printf("FAIL [%d] state mismatch (n=%d dirx=%02X diry=%02X stepx=%08X stepy=%08X)\n",
               id, in->n, in->dirx, in->diry, (unsigned)in->stepx,
               (unsigned)in->stepy);
        printf("  accx retail=%08X host=%08X accy retail=%08X host=%08X flag retail=%02X host=%02X\n",
               (unsigned)got.accx, (unsigned)want.accx,
               (unsigned)got.accy, (unsigned)want.accy, got.flag, want.flag);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x80088990u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0, id = 0;
    /* directed edges: n<=0 skip, small n, both directions, 0x100/non-0x100
     * steps, negative accumulators (the +0xFF rounding path), extremes. */
    s32 steps[] = { 0x100, 0x100, 0, 1, -1, 0x7FFFFFFF, (s32)0x80000000,
                    0x12345, -0x12345 };
    s32 accs[] = { 0, 1, -1, -255, -256, -257, 0x7FFFFFFF, (s32)0x80000000,
                   0x00010000, -0x00010000 };
    s32 ns[] = { -1, 0, 1, 2, 3, 7, 20 };
    u8 dirs[] = { 0, 1, 0xFF };
    for (unsigned si = 0; si < sizeof(steps) / sizeof(steps[0]); si++) {
        for (unsigned ai = 0; ai < sizeof(accs) / sizeof(accs[0]); ai++) {
            for (unsigned ni = 0; ni < sizeof(ns) / sizeof(ns[0]); ni++) {
                for (unsigned di = 0; di < sizeof(dirs) / sizeof(dirs[0]); di++) {
                    state_t s;
                    s.x0 = -50; s.y0 = 100; s.x1 = 500; s.y1 = -300;
                    s.stepx = steps[si]; s.stepy = steps[(si + 3) % 9];
                    s.dirx = dirs[di]; s.diry = dirs[(di + 1) % 3];
                    s.n = ns[ni];
                    s.flag = (u8)(si & 1);
                    s.accx = accs[ai]; s.accy = accs[(ai + 5) % 10];
                    if (!check_one(m, entry, &s, id++)) return 1;
                    n++;
                }
            }
        }
    }
    /* rounding-boundary seeds: trunc(acc/256) under the +0xFF rule vs a
     * +0xFE mutant differs exactly when acc = -255 - 256*k; with the
     * endpoint pinned at the boundary the arrival flag must flip. */
    s32 racc[] = { -255, -511, -767, -0x101, -256, -1, -257, -0x10000,
                   0xFF00, 0xFFFF };
    for (unsigned k = 0; k < sizeof(racc) / sizeof(racc[0]); k++) {
        state_t s;
        memset(&s, 0, sizeof(s));
        s.x0 = 0; s.y0 = 0; s.x1 = 0; s.y1 = 0;
        s.stepx = 0x100; s.stepy = 0x100;
        s.dirx = 1; s.diry = 1;
        s.n = 0; s.flag = 0;
        s.accx = racc[k]; s.accy = racc[k];
        if (!check_one(m, entry, &s, id++)) return 1;
        n++;
        s.stepx = 0; s.dirx = 0; /* y-path variant */
        if (!check_one(m, entry, &s, id++)) return 1;
        n++;
    }
    /* deterministic pseudo-random sweep over bounds too */
    uint32_t s = 0x88990890u;
    for (int i = 0; i < 200; i++) {
#define R32() (s = s * 1103515245u + 12345u, s)
        state_t st;
        st.x0 = (s32)R32(); st.y0 = (s32)R32();
        st.x1 = (s32)R32(); st.y1 = (s32)R32();
        st.stepx = (s32)R32(); st.stepy = (s32)R32();
        st.dirx = (u8)R32(); st.diry = (u8)R32();
        st.n = (s32)(R32() % 25u);
        if (i % 7 == 0) st.n = -(s32)(R32() % 3u);
        if (i % 5 == 0) st.stepx = 0x100;
        if (i % 9 == 0) st.stepy = 0x100;
        st.flag = (u8)R32();
        st.accx = (s32)R32(); st.accy = (s32)R32();
        if (!check_one(m, entry, &st, id++)) return 1;
        n++;
#undef R32
    }
    printf("DIFF 80088990 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
