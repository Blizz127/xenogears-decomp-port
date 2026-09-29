/* Differential test: host-compiled src/battle/mainasm_AD84.c func_8007A874
 * vs the retail bytes (asm/battle/matchings/mainasm_AD84/func_8007A874.s)
 * executed by remu. Fails on any behavioral mismatch.
 *
 * Build (from repo root):
 *   tools/remu/run_diff.sh diff_8007A874 src/battle/mainasm_AD84.c
 */
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "remu.h"

/* ---- host side: the real TU object (mainasm_AD84.c built with remu_shim.h so
 * INCLUDE_ASM is neutralized but every C body is identical). ---- */
#include "common.h"
u8 func_8007A874(u8 **ppBoard, u8 *pDst, s32 index, s32 i);

/* TU externs the test must provide (sizes generous; D_800D3430 is used). */
u8 *D_800D2D28;
__attribute__((aligned(16))) u8 D_800D3420[16384];
__attribute__((aligned(16))) u8 D_800D3430[17408];
__attribute__((aligned(16))) u32 D_800D3410[4352];
u16 D_8005A3A0[8];
u8 D_800D366C;
u8 *D_800C3EAC;
u32 func_80089C08(u32 v) { (void)v; return 0; }
void func_800BC404(u32 v) { (void)v; }
void func_800BCD98(u32 v) { (void)v; }
void func_80085454(s32 v) { (void)v; }
void func_80085618(s32 v) { (void)v; }
void func_8008AA40(u8 v) { (void)v; }
u32 D_800C3EA4;
void *func_8008ABB8(s32 size, s32 flag) { (void)size; (void)flag; return 0; }
/* TU-declared bzero has a different signature than libc's; define the
 * symbol under a distinct C name to avoid the strings.h clash. */
void *tu_bzero(unsigned char *p, int size) __asm__("bzero");
void *tu_bzero(unsigned char *p, int size) {
    for (int i = 0; i < size; i++) p[i] = 0;
    return p;
}
void func_80077074(void) {}

#define RETAIL_S "asm/battle/matchings/mainasm_AD84/func_8007A874.s"
#define TAB_BASE 0x800D3430u
/* 256x64 rows plus slack: retail indexes row[p[2]] with p[2] up to 0xFF,
 * so the max touch is (0xFF << 6) + 0xFF = 16575. */
#define TAB_LEN 17408u
#define PP 0x801F0000u
#define BOARD 0x801F0010u
#define DST 0x801F1000u
#define DST_LEN 4096u

static uint32_t lcg(uint32_t *s) {
    *s = *s * 1103515245u + 12345u;
    return *s;
}

static u8 h_board[8];
static u8 h_dst[DST_LEN];

static int check_one(remu_t *m, uint32_t entry, uint32_t seed) {
    u8 b1 = (u8)seed, b2 = (u8)(seed >> 8);
    u8 idx = (u8)(seed >> 16), ii = (u8)(seed >> 24);
    uint32_t x = seed;
    static u8 tab[TAB_LEN];
    static u8 dst[DST_LEN];
    u8 board[8];

    for (uint32_t j = 0; j < TAB_LEN; j++)
        tab[j] = (u8)(lcg(&x) >> 16);
    for (uint32_t j = 0; j < DST_LEN; j++)
        dst[j] = (u8)(lcg(&x) >> 16);
    board[0] = (u8)(seed >> 24); board[1] = b1;
    board[2] = b2; board[3] = (u8)(seed >> 16);
    board[4] = 0; board[5] = 0; board[6] = 0; board[7] = 0;

    /* host side */
    memcpy(D_800D3430, tab, TAB_LEN);
    memcpy(h_dst, dst, DST_LEN);
    memcpy(h_board, board, sizeof(board));
    u8 *h_pp = h_board;
    u8 got = func_8007A874(&h_pp, h_dst, idx, ii);

    /* retail side */
    uint32_t pc = BOARD;
    if (remu_poke(m, TAB_BASE, tab, TAB_LEN)) { printf("FAIL poke tab\n"); return 0; }
    if (remu_poke(m, DST, dst, DST_LEN)) { printf("FAIL poke dst\n"); return 0; }
    if (remu_poke(m, BOARD, board, sizeof(board))) { printf("FAIL poke board\n"); return 0; }
    if (remu_poke(m, PP, &pc, 4)) { printf("FAIL poke pp\n"); return 0; }
    int rc = remu_call(m, entry, PP, DST, idx, ii);
    if (rc != 0) {
        printf("FAIL seed=%08X remu rc=%d stubs=%d%s\n", seed, rc,
               remu_stub_calls(m), remu_stub_log(m));
        return 0;
    }
    if (remu_stub_calls(m) != 0) {
        printf("FAIL seed=%08X unexpected stub calls%s\n", seed,
               remu_stub_log(m));
        return 0;
    }
    /* v0 holds the loaded byte (lbu zero-extends) */
    uint32_t want = remu_get_reg(m, 2);
    if ((uint32_t)got != want) {
        printf("FAIL seed=%08X retail v0=%08X host=%02X\n", seed, want, got);
        return 0;
    }
    /* memory effects: dst store, and table must be untouched */
    static u8 rdst[DST_LEN];
    static u8 rtab[TAB_LEN];
    if (remu_peek(m, DST, rdst, DST_LEN)) { printf("FAIL peek dst\n"); return 0; }
    if (memcmp(rdst, h_dst, DST_LEN) != 0) {
        printf("FAIL seed=%08X dst mismatch\n", seed);
        return 0;
    }
    if (remu_peek(m, TAB_BASE, rtab, TAB_LEN)) { printf("FAIL peek tab\n"); return 0; }
    if (memcmp(rtab, tab, TAB_LEN) != 0) {
        printf("FAIL seed=%08X table clobbered\n", seed);
        return 0;
    }
    return 1;
}

int main(void) {
    remu_t *m = remu_create();
    if (!m) { printf("FAIL create\n"); return 1; }
    uint32_t entry = remu_load_s(m, RETAIL_S);
    if (entry != 0x8007A874u) { printf("FAIL load entry=%08X\n", entry); return 1; }

    int n = 0;
    /* edge seeds */
    uint32_t edges[] = { 0x00000000u, 0x00000001u, 0xFFFFFFFFu, 0x7FFFFFFFu,
                         0x80000000u, 0xFFFFFFFFu };
    for (unsigned i = 0; i < sizeof(edges) / sizeof(edges[0]); i++) {
        if (!check_one(m, entry, edges[i])) return 1;
        n++;
    }
    /* all single bits */
    for (int b = 0; b < 32; b++) {
        if (!check_one(m, entry, 1u << b)) return 1;
        n++;
    }
    /* deterministic pseudo-random sweep */
    uint32_t s = 0x12345678u;
    for (int i = 0; i < 200; i++) {
        s = s * 1103515245u + 12345u;
        if (!check_one(m, entry, s)) return 1;
        n++;
    }
    printf("DIFF 8007A874 OK (%d seeds, retail-exact)\n", n);
    remu_destroy(m);
    return 0;
}
