/*
 * W34B4C: World-map terrain/position initializer 0x80097BC0.
 *
 * Exact transcription of retail 0x80097BC0–0x80097CB4.
 * Copies identity-like matrix from 0x8009A180 to terrain matrix at D534,
 * clears 256 words descending from C580 through C184, initializes terrain
 * period (C618=1024),
 * masks initial world position, and calls terrain helpers 0x800981C8 and
 * 0x80097DC0.
 *
 * Called from mode_update_init at retail 0x8007250C when C894==0.
 *
 * Both the production game build and the production-linked test link
 * this same object.  Do NOT duplicate this function elsewhere.
 */
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_terrain_init.h"

/* ---- Retail absolute addresses (signed-immediate derivation) ---- */

/* Identity-like source matrix (32 bytes at 0x8009A180).
 * lui 0x800A + addiu -24192 = 0x8009A180.
 * Contains: {0x1000,0,0,0, 0x1000,0,0,0, 0x1000,0,0,0, 0,0,0,0}. */
#define WM_IDENTITY_MATRIX_SRC  0x8009A180u
#define WM_MATRIX_SIZE          32  /* 8 words = 32 bytes */

/* Terrain matrix destination (32 bytes at 0x8009D534).
 * lui 0x800A + addiu -10956 = 0x8009D534. */
#define WM_D534_ABS             0x8009D534u

/* Retail descending clear starts at 0x8009C580.
 * lui 0x800A + addiu -14976 = 0x8009C580.
 * The 256-word loop covers C580,C57C,...,C184. */
#define WM_C580_ABS             0x8009C580u
#define WM_CLEAR_WORD_COUNT     256
#define WM_CLEAR_STRIDE         4u

/* Terrain period (word at 0x8009C618).
 * lui 0x800A + addiu -14824 = 0x8009C618. */
#define WM_C618_ABS             0x8009C618u

/* Terrain active flag (word at 0x8009BBB8).
 * lui 0x800A + addiu -17480 = 0x8009BBB8. */
#define WM_BBB8_ABS             0x8009BBB8u

/* Terrain sub-flag (word at 0x8009C5BC).
 * lui 0x800A + addiu -14916 = 0x8009C5BC. */
#define WM_C5BC_ABS             0x8009C5BCu

/* Position X/Z mask destination words.
 * lui 0x800A + addiu -17484 = 0x8009BBB4 (masked X from word 0 of input).
 * lui 0x800A + addiu -17476 = 0x8009BBBC (masked Z from word 2 of input). */
#define WM_BBB4_ABS             0x8009BBB4u
#define WM_BBBC_ABS             0x8009BBBCu

/* Position mask: 0x007FFFFF (23-bit, built via lui+ori).
 * lui 0x007F + ori 0xFFFF = 0x007FFFFF. */
#define WM_POS_MASK             0x007FFFFFu

/* Terrain cell index halfwords.
 * lui 0x800A + addiu -14280 = 0x8009C838 (terrain cell X).
 * lui 0x800A + addiu -14278 = 0x8009C83A (zero).
 * lui 0x800A + addiu -14276 = 0x8009C83C (terrain cell Z). */
#define WM_C838_ABS             0x8009C838u
#define WM_C83A_ABS             0x8009C83Au
#define WM_C83C_ABS             0x8009C83Cu

/* Input position pointer (passed as $a0 from caller).
 * Caller loads 0x8009C5AC (lui 0x800A + addiu -14932). */
#define WM_C5AC_ABS             0x8009C5ACu

#define WM_U32(a) (*(u32*)PSX_ADDR(a))
#define WM_U16(a) (*(u16*)PSX_ADDR(a))

/* External terrain helpers.
 * 0x800981C8: normalizes position and computes terrain cell indices.
 * 0x80097DC0: allocates/loads terrain tile buffers from CD. */
extern void wm_800981C8(u32 pos_ptr);
extern void wm_80097DC0(void);

#if defined(WM_97BC0_CLEAR_TRACE)
/* Focused production-linked tests provide this observer. It is absent from
 * ordinary production builds and records the completed retail store order. */
extern void wm_97bc0_trace_store(u32 guest_addr);
#endif

/* Instrumentation. */
static int s_wm_tpi_calls;

int wm_tpi_get_calls(void) { return s_wm_tpi_calls; }

void wm_tpi_reset(void) { s_wm_tpi_calls = 0; }

/* ---- Production function ---- */

/* W34B4C: terrain/position initializer.
 * Exact transcription of retail 0x80097BC0–0x80097CB4. */
void wm_80097BC0(u32 pos_ptr)
{
    u32 *src, *dst;
    u32 pos_x, pos_z;
    u32 clear_addr;
    int i;

    s_wm_tpi_calls++;
    fprintf(stderr, "[worldmap-tpi] entry (call %d) pos_ptr=0x%08X\n",
            s_wm_tpi_calls, pos_ptr);

    /* 0x80097BC8: addu a2, a0, zero — save input pointer to a2.
     * pos_ptr is the $a0 argument (= 0x8009C5AC from caller). */

    /* 0x80097BCC–0x80097C18: copy 32 bytes (8 words) from 0x8009A180
     * to 0x8009D534.  This is the identity-like terrain matrix init.
     * Retail uses paired lw/sw with v0/v1, 4 words per iteration. */
    src = (u32*)PSX_ADDR(WM_IDENTITY_MATRIX_SRC);
    dst = (u32*)PSX_ADDR(WM_D534_ABS);
    for (i = 0; i < WM_MATRIX_SIZE / 4; i += 2) {
        u32 w0 = src[i];
        u32 w1 = src[i + 1];
        dst[i] = w0;
        dst[i + 1] = w1;
    }

    /* 0x80097C1C–0x80097C34: v1=255, v0=C580, SW zero,0(v0),
     * decrement v1, branch while v1>=0, and decrement v0 by four in the
     * branch delay slot. Exactly 256 stores: C580,C57C,...,C184. */
    clear_addr = WM_C580_ABS;
    for (i = 0; i < WM_CLEAR_WORD_COUNT; i++) {
        *(volatile u32 *)PSX_ADDR(clear_addr) = 0;
#if defined(WM_97BC0_CLEAR_TRACE)
        wm_97bc0_trace_store(clear_addr);
#endif
        clear_addr -= WM_CLEAR_STRIDE;
    }

    /* 0x80097C38–0x80097C3C: load mask 0x007FFFFF into a0. */

    /* 0x80097C40: lw v1, 0(a2) — load word 0 (X) of input position. */
    pos_x = WM_U32(pos_ptr);

    /* 0x80097C44: addiu v0, zero, 1024 — terrain period value. */

    /* 0x80097C48–0x80097C4C: store zero at BBB8 (terrain active flag). */
    WM_U32(WM_BBB8_ABS) = 0;

    /* 0x80097C50–0x80097C54: store zero at C5BC (terrain sub-flag). */
    WM_U32(WM_C5BC_ABS) = 0;

    /* 0x80097C58–0x80097C5C: store 1024 at C618 (terrain period). */
    WM_U32(WM_C618_ABS) = 1024;

    /* 0x80097C60: and v1, v1, a0 — mask X with 0x007FFFFF. */
    pos_x &= WM_POS_MASK;

    /* 0x80097C64–0x80097C68: store masked X at BBB4. */
    WM_U32(WM_BBB4_ABS) = pos_x;

    /* 0x80097C6C: lw v0, 8(a2) — load word 2 (Z) of input position. */
    pos_z = WM_U32(pos_ptr + 8);

    /* 0x80097C70–0x80097C88: store 2 at C838, 0 at C83A, 2 at C83C
     * (initial terrain cell indices as halfwords). */
    WM_U16(WM_C838_ABS) = 2;
    WM_U16(WM_C83A_ABS) = 0;
    WM_U16(WM_C83C_ABS) = 2;

    /* 0x80097C8C: and v0, v0, a0 — mask Z with 0x007FFFFF. */
    pos_z &= WM_POS_MASK;

    /* 0x80097C90–0x80097C94: store masked Z at BBBC. */
    WM_U32(WM_BBBC_ABS) = pos_z;

    /* 0x80097C98–0x80097C9C: jal 0x800981C8 with a0 = original pos_ptr.
     * Delay slot: addu a0, a2, zero (restores a0 from a2). */
    wm_800981C8(pos_ptr);

    /* 0x80097CA0–0x80097CA4: jal 0x80097DC0 (no arguments).
     * Delay slot: nop. */
    wm_80097DC0();

    fprintf(stderr, "[worldmap-tpi] exit\n");
}

/* Retail 0x80097CB8..0x80097D60: the resume twin of 0x80097BC0.
 *
 * Called from the base-world session setup at retail 0x80072548 when
 * C894 != 0 (re-entering the world map with a saved position, e.g. after a
 * world encounter), with a0 = 0x8009BE28 instead of 0x8009C5AC.  It performs
 * the same matrix copy, descending 256-word clear and C5BC/C618 stores, but
 * leaves the streamed-terrain origin (BBB4/BBB8/BBBC) and the cell indices
 * (C838/C83A/C83C) untouched -- those were restored with the saved state --
 * before running the same two terrain helpers. */
void wm_80097CB8(u32 pos_ptr)
{
    u32 *src = (u32*)PSX_ADDR(WM_IDENTITY_MATRIX_SRC);
    u32 *dst = (u32*)PSX_ADDR(WM_D534_ABS);
    u32 clear_addr = WM_C580_ABS;
    int i;

    for (i = 0; i < WM_MATRIX_SIZE / 4; i++)
        dst[i] = src[i];
    for (i = 0; i < WM_CLEAR_WORD_COUNT; i++) {
        *(volatile u32 *)PSX_ADDR(clear_addr) = 0;
        clear_addr -= WM_CLEAR_STRIDE;
    }
    WM_U32(WM_C5BC_ABS) = 0;
    WM_U32(WM_C618_ABS) = 1024;
    wm_800981C8(pos_ptr);
    wm_80097DC0();
}
