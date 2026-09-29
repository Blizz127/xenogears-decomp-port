/*
 * World-map helpers 0x80096130 and 0x800967E4 — queue synchronization.
 */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_96130.h"
#include "world_map_helper_966cc.h"

extern u32 func_8002C3D8(void);
extern u32 wm_80096668_circular_distance(void);
extern int Vsync(int mode);

#if defined(W34N21_TEST_HOOKS)
extern void w34n21_test_q96_sw(u32 address, u32 value);
extern void w34n21_test_state4_after_countdown(void);
#endif

#define D_8009BE44  0x8009BE44u
#define D_8009D788  0x8009D788u
#define D_8009C624  0x8009C624u
#define D_8009BCB8  0x8009BCB8u
#define D_8009D808  0x8009D808u
#define D_8009CD44  0x8009CD44u
#define D_8009BD2C  0x8009BD2Cu

static u32 q96_lw(u32 a)
{
    volatile const u8 *p = (volatile const u8 *)PSX_ADDR(a);

    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) |
           ((u32)p[3] << 24);
}

static void q96_sw(u32 a, u32 v)
{
    volatile u8 *p = (volatile u8 *)PSX_ADDR(a);

    p[0] = (u8)(v & 0xFFu);
    p[1] = (u8)((v >> 8) & 0xFFu);
    p[2] = (u8)((v >> 16) & 0xFFu);
    p[3] = (u8)((v >> 24) & 0xFFu);
#if defined(W34N21_TEST_HOOKS)
    w34n21_test_q96_sw(a, v);
#endif
}

/* Retail 0x80096694..0x800966C8. This is a do/while barrier: even an
 * already-empty queue performs one Vsync + dispatcher poll before testing
 * the circular distance. */
void wm_80096694(void)
{
#if defined(WM_96694_MUTANT_PRECHECK)
    if (wm_80096668_circular_distance() == 0u)
        return;
#endif

    do {
#if defined(WM_96694_MUTANT_REVERSED_ORDER)
        wm_800967E4();
        Vsync(0);
#else
#if !defined(WM_96694_MUTANT_NO_VSYNC)
        Vsync(0);
#endif
        wm_800967E4();
#endif
#if defined(WM_96694_MUTANT_THRESHOLD_TWO)
    } while (wm_80096668_circular_distance() >= 2u);
#else
    } while (wm_80096668_circular_distance() != 0u);
#endif
}

/* Retail 0x800968E0..0x8009699C: CD transfer-state dispatcher. */
u32 wm_800968E0(void)
{
    u32 state = q96_lw(D_8009CD44);

    if (state >= 6u)
        return 3u;
    switch (state) {
    case 0u:
#if defined(W34N21_MUTANT_DISPATCH_IDLE_BUSY)
        return 1u;
#else
        return 0u;
#endif
    case 1u:
    case 2u:
    case 3u:
        return 1u;
    case 4u: {
        u32 countdown = q96_lw(D_8009BD2C);
#if !defined(W34N21_MUTANT_DISPATCH_SKIP_COUNTDOWN)
        countdown--;
        q96_sw(D_8009BD2C, countdown);
#endif
        if (countdown == 0u) {
#if defined(W34N21_TEST_HOOKS)
            w34n21_test_state4_after_countdown();
#endif
#if defined(W34N21_MUTANT_DISPATCH_CACHED_STATE)
            q96_sw(D_8009CD44, state + 1u);
#else
            q96_sw(D_8009CD44, q96_lw(D_8009CD44) + 1u);
#endif
        }
        return 1u;
    }
    case 5u: {
        u32 tail = q96_lw(D_8009BCB8);
#if defined(W34N21_MUTANT_STATE5_REORDER)
        q96_sw(D_8009D788 + tail * 4u, 0u);
        q96_sw(D_8009CD44, 0u);
#else
        q96_sw(D_8009CD44, 0u);
#endif
#if defined(W34N21_MUTANT_DISPATCH_WRONG_TAIL)
        q96_sw(D_8009D788 + ((tail + 1u) & 0xFu) * 4u, 0u);
#elif !defined(W34N21_MUTANT_STATE5_REORDER)
        q96_sw(D_8009D788 + tail * 4u, 0u);
#endif
        q96_sw(D_8009BCB8, (tail + 1u) & 0xFu);
        return 2u;
    }
    default:
        return 3u;
    }
}

/* Retail 0x800967E4 (W34C11 re-wiring, PCs inline). ready = (r1 == 0) |
 * (~r2 == 0). Ready (0x8009681C-0x80096860): propagate the exact
 * 0x800968E0 dispatcher status when nonzero; otherwise take
 * D788[lw 0x8009BCB8] (0x80096834-48) and start the CD loader 0x8009699C.
 * Not ready (0x80096868-0x800968C0): take C624[lw 0x8009BCB8], run the
 * PC-file loader 0x800966CC, clear the entry, BCB8 = (BCB8 + 1) & 0xF. */
u32 wm_800967E4(void)
{
    u32 r1 = func_8002C3D8();
    u32 r2 = func_8002C3D8();
    u32 ready = ((r1 < 1) | (~r2 < 1));
    u32 result = 0u;
#if defined(WM_967E4_MUTANT_M1)                  /* pre-W34C11 wiring */
    ready = !ready;
#endif

    if (ready) {
        result = wm_800968E0();
#if defined(W34N21_MUTANT_DROP_DISPATCH_STATUS)
        result = 0u;
#endif
        if (result != 0u)
            return result;
#if defined(WM_967E4_MUTANT_M2)
        u32 idx = q96_lw(D_8009BE44);
#else
        u32 idx = q96_lw(D_8009BCB8);             /* 0x80096834 */
#endif
        u32 record = q96_lw(D_8009D788 + idx * 4u);
        if (record != 0u) {
            wm_8009699C(record);                  /* 0x80096858 */
        }
    } else {
        u32 idx = q96_lw(D_8009BCB8);             /* 0x8009686C */
        u32 record = q96_lw(D_8009C624 + idx * 4u);
        if (record != 0u) {
            wm_800966CC(record);                  /* 0x80096894 */
#if !defined(W34N21_MUTANT_C624_STALE_TAIL)
            idx = q96_lw(D_8009BCB8);             /* 0x8009689C */
#endif
            q96_sw(D_8009C624 + idx * 4u, 0u);    /* 0x800968B8 */
            q96_sw(D_8009BCB8, (idx + 1) & 0xF);  /* 0x800968C0 */
        }
    }
    return result;
}

void wm_80096130(void)
{
    /* First check: archive debug table path */
    u32 r1 = func_8002C3D8();
    u32 r2 = func_8002C3D8();
    u32 ready = ((r1 < 1) | (~r2 < 1));

    if (ready) {
        /* Drain D_8009D788 queue */
        u32 idx = q96_lw(D_8009BE44);
        u32* table = (u32*)PSX_ADDR(D_8009D788);

        while (table[idx] != 0) {
            Vsync(0);
            wm_800967E4();
            if (table[idx] == 0) break;
        }
    } else {
        /* Drain D_8009C624 queue */
        u32 idx = q96_lw(D_8009BCB8);
        u32* table = (u32*)PSX_ADDR(D_8009C624);

        while (table[idx] != 0) {
            Vsync(0);
            wm_800967E4();
            if (table[idx] == 0) break;
        }
    }
}
