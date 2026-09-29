/*
 * W34B5J: World-map callback 0x800923A8 — DRAWENV/tpage/clip initializer.
 *
 * Exact transcription of retail 0x800923A8–0x8009259C (126 instructions,
 * 0x1F8 bytes).  Scheduler slot 0, state-0 callback.  Returns 1.
 *
 * Direct dependencies: GetTPage, SetDrawTPage, SetSemiTrans (all PsyQ).
 *
 * Fresh retail decode authority: W34B5-I (scratchpad/w34b5i_callback_800923a8/).
 */
#include <stdio.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "world_map_scheduler.h"
#include "world_map_callback_923a8.h"

/* ---- Retail absolute addresses (signed-immediate derivation) ---- */

/* Scheduler pool base pointer (u32). */
#define WM_POOL_PTR_ABS      0x8009BE24u

/* Graphics work buffer pointer (u32), loaded but unused in this callback. */
#define WM_GFX_BUF_ABS       0x8009CCA4u

/* DR_TPAGE destination (8 bytes: tag + code[1]). */
#define WM_DR_TPAGE_ABS      0x8009D310u

/* DRAWENV-like region base (92 bytes). */
#define WM_DRAWENV_ABS        0x8009CE6Cu

/* Slot field offsets. */
#define WM_SLOT_OFF_50        0x50u

/* ---- Convenience macros (same pattern as world_map_init.c) ---- */
#define WM_U8(a)  (*(u8*)PSX_ADDR(a))
#define WM_U16(a) (*(u16*)PSX_ADDR(a))
#define WM_U32(a) (*(u32*)PSX_ADDR(a))

/* ---- Instrumentation ---- */
static int s_cb923a8_calls;

int  wm_cb923a8_get_calls(void) { return s_cb923a8_calls; }
void wm_cb923a8_reset(void)     { s_cb923a8_calls = 0; }

/* ---- Production callback ---- */

/* Retail 0x800923A8: DRAWENV/tpage/clip initializer.
 * a0 = slot index; returns s16 = 1 (new slot state).
 *
 * Exact instruction-by-instruction transcription.  Every store width,
 * address, value, and ordering matches the fresh retail decode. */
s16 wm_800923A8(int slot_index)
{
    u32 pool_ptr;
    u32 slot_addr;
    u16 tpage;
    u8* src;
    u8* dst;
    u8* sentinel;

    s_cb923a8_calls++;
    fprintf(stderr, "[worldmap-cb923a8] entry slot=%d (call %d)\n",
            slot_index, s_cb923a8_calls);

    /* 800923AC: sll v0, a0, 7  →  slot_addr = pool + (slot_index << 7) */
    pool_ptr  = WM_U32(WM_POOL_PTR_ABS);
    slot_addr = pool_ptr + ((u32)slot_index << 7);

    /* 800923DC/800923E4: sw v0, 0x50(s0)  →  slot+0x50 = 0xF8
     * (delay slot of jal GetTPage) */
    WM_U32(slot_addr + WM_SLOT_OFF_50) = 0xF8u;

    /* 800923E0: jal 0x80043A1C  →  GetTPage(0, 0, 0x380, 0x100)
     * Expected result: 0x001E */
    tpage = GetTPage(0, 0, 0x380, 0x100);

    /* 800923F8: jal 0x80043E20  →  SetDrawTPage(0x8009D310, 1, 0, tpage)
     * Generates GPU E1h cmd: 0xE100041E */
    SetDrawTPage((DR_TPAGE*)PSX_ADDR(WM_DR_TPAGE_ABS), 1, 0,
                 (int)(tpage & 0xFFFFu));

    /* ---- DRAWENV byte-field initialization (14 sb instructions) ---- */

    /* 80092408: sb v0(8), CE6F */
    WM_U8(WM_DRAWENV_ABS + 0x03u) = 0x08u;
    /* 80092414: sb v0(0x38), CE73 */
    WM_U8(WM_DRAWENV_ABS + 0x07u) = 0x38u;

    /* 80092420: sb slot+0x50, CE70 */
    WM_U8(WM_DRAWENV_ABS + 0x04u) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 8009242C: sb slot+0x50, CE71 */
    WM_U8(WM_DRAWENV_ABS + 0x05u) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 80092438: sb slot+0x50, CE72 */
    WM_U8(WM_DRAWENV_ABS + 0x06u) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 80092444: sb slot+0x50, CE78 */
    WM_U8(WM_DRAWENV_ABS + 0x0Cu) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 80092450: sb slot+0x50, CE79 */
    WM_U8(WM_DRAWENV_ABS + 0x0Du) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 8009245C: sb slot+0x50, CE7A */
    WM_U8(WM_DRAWENV_ABS + 0x0Eu) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 80092468: sb slot+0x50, CE80 */
    WM_U8(WM_DRAWENV_ABS + 0x14u) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 80092474: sb slot+0x50, CE81 */
    WM_U8(WM_DRAWENV_ABS + 0x15u) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 80092480: sb slot+0x50, CE82 */
    WM_U8(WM_DRAWENV_ABS + 0x16u) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 80092494: sb slot+0x50, CE88 */
    WM_U8(WM_DRAWENV_ABS + 0x1Cu) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 800924A4: sb slot+0x50, CE89 */
    WM_U8(WM_DRAWENV_ABS + 0x1Du) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));
    /* 800924B0: sb slot+0x50, CE8A */
    WM_U8(WM_DRAWENV_ABS + 0x1Eu) = (u8)(WM_U32(slot_addr + WM_SLOT_OFF_50));

    /* 800924B4: jal 0x80043BFC  →  SetSemiTrans(0x8009CE6C, 1) */
    SetSemiTrans(PSX_ADDR(WM_DRAWENV_ABS), 1);

    /* ---- 36-byte copy: 0x8009CE6C..0x8009CE8F → 0x8009CE90..0x8009CEB3 ---- */

    /* 800924BC: addiu a2, s1, 0x24  →  dst = src + 36 */
    /* 800924C0: addiu a3, s1, 0x20  →  sentinel = src + 32 */
    src      = (u8*)PSX_ADDR(WM_DRAWENV_ABS);
    dst      = src + 0x24;  /* 0x8009CE90 */
    sentinel = src + 0x20;  /* 0x8009CE8C — loop terminates when src reaches here */

    /* 800924C4–800924EC: copy loop — 2 iterations × 16 bytes = 32 bytes.
     * Each iteration: lw v0/v1/a0/a1 from src, sw to dst, src+=16, dst+=16.
     * Loop exits when src == sentinel (0x8009CE6C + 0x20 = 0x8009CE8C). */
    while (src != sentinel) {
        u32 w0 = *(u32*)(src + 0);
        u32 w1 = *(u32*)(src + 4);
        u32 w2 = *(u32*)(src + 8);
        u32 w3 = *(u32*)(src + 12);
        *(u32*)(dst + 0)  = w0;
        *(u32*)(dst + 4)  = w1;
        *(u32*)(dst + 8)  = w2;
        *(u32*)(dst + 12) = w3;
        src += 16;
        dst += 16;
    }

    /* 800924F0–800924F8: final word copy — lw from src(0x8009CE8C),
     * sw to dst(0x8009CEB0).  4 bytes. */
    *(u32*)dst = *(u32*)src;

    /* ---- Clip/offset halfword initialization (16 sh instructions) ---- */

    /* 8009250C: sh zero, CE74 */
    WM_U16(WM_DRAWENV_ABS + 0x08u) = 0;
    /* 80092514: sh zero, CE76 */
    WM_U16(WM_DRAWENV_ABS + 0x0Au) = 0;
    /* 8009251C: sh 320, CE7C */
    WM_U16(WM_DRAWENV_ABS + 0x10u) = 320;
    /* 80092524: sh zero, CE7E */
    WM_U16(WM_DRAWENV_ABS + 0x12u) = 0;
    /* 8009252C: sh zero, CE84 */
    WM_U16(WM_DRAWENV_ABS + 0x18u) = 0;
    /* 80092534: sh 216, CE86 */
    WM_U16(WM_DRAWENV_ABS + 0x1Au) = 216;
    /* 8009253C: sh 320, CE8C */
    WM_U16(WM_DRAWENV_ABS + 0x20u) = 320;
    /* 80092544: sh 216, CE8E */
    WM_U16(WM_DRAWENV_ABS + 0x22u) = 216;
    /* 8009254C: sh zero, CE98 */
    WM_U16(WM_DRAWENV_ABS + 0x2Cu) = 0;
    /* 80092554: sh zero, CE9A */
    WM_U16(WM_DRAWENV_ABS + 0x2Eu) = 0;
    /* 8009255C: sh 320, CEA0 */
    WM_U16(WM_DRAWENV_ABS + 0x34u) = 320;
    /* 80092564: sh zero, CEA2 */
    WM_U16(WM_DRAWENV_ABS + 0x36u) = 0;
    /* 8009256C: sh zero, CEA8 */
    WM_U16(WM_DRAWENV_ABS + 0x3Cu) = 0;
    /* 80092574: sh 216, CEAA */
    WM_U16(WM_DRAWENV_ABS + 0x3Eu) = 216;
    /* 8009257C: sh 320, CEB0 */
    WM_U16(WM_DRAWENV_ABS + 0x44u) = 320;
    /* 80092584: sh 216, CEB2 */
    WM_U16(WM_DRAWENV_ABS + 0x46u) = 216;

    /* 800924FC: addiu v0, zero, 1  →  return 1 */
    fprintf(stderr, "[worldmap-cb923a8] exit return=1\n");
    return 1;
}
