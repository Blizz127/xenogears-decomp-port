/* Host port bodies of func_80074D4C / func_80073FB8 / func_80074AB8 and their
 * shared strip linker func_800728B8: four of the draw steps of the per-frame
 * battle render chain func_80076418 (main2.c).  The byte-exact C stays in
 * main2.c under #ifndef XENO_PC_PORT; this TU is the only owner an adopted
 * host build links.
 *
 * Every overlay pointer is held as a u32 guest address and dereferenced
 * through BG_*, with retail's 32-bit wrapping arithmetic; each global is
 * re-read where retail re-reads it.
 *
 * Primitive linking: retail AddPrim (SLUS 0x80043B48) is two word reads and
 * two word writes on guest memory: the packet takes the OT word's low 24 bits,
 * the OT word takes the packet address's low 24 bits (each keeps its own high
 * byte).  add_prim() below is that sequence on guest addresses, exactly what
 * the bridge's AddPrim service (battle_mips_runtime.c) does.  PsyCross's own
 * AddPrim cannot be used here: with USE_EXTENDED_PRIM_POINTERS=0 it links the
 * low 24 bits of a HOST pointer, and the OT and packets live in g_PsxRam. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

#define MENU_BLK() BG_U32(0x800D2D28u)   /* D_800D2D28: HUD / menu state block */
#define OT_HEAD() BG_U32(0x800CCB04u)    /* D_800CCB04: current ordering table */
#define BUF_IDX() BG_U32(0x800CCB34u)    /* D_800CCB34: current buffer */
#define PRIM_BASE() BG_U32(0x800C3EA4u)  /* D_800C3EA4: HUD prim pool */
#define FX_BLK() BG_U32(0x800C3E24u)     /* D_800C3E24: pulsing-arrow block */
#define PANEL_BLK() BG_U32(0x800D2DB4u)  /* D_800D2DB4: window strip block */
#define PANEL_TBL 0x800D2E38u            /* D_800D2E38: u32 per record */
#define SLOT_FLAG() BG_U8(0x800C3E4Cu)   /* D_800C3E4C */

/* AddPrim(ot, prim) on guest addresses (SLUS 0x80043B48). */
static void add_prim(u32 ot, u32 prim) {
    u32 p = BG_U32(prim);
    u32 o = BG_U32(ot);

    BG_U32(prim) = (p & 0xFF000000u) | (o & 0x00FFFFFFu);
    o = BG_U32(ot);
    BG_U32(ot) = (o & 0xFF000000u) | (prim & 0x00FFFFFFu);
}

/* func_800728B8.s, byte-exact: link `count` 0x28-byte primitives of `prims`,
 * every other one from `start` (prim = prims + (start + 2 * k) * 0x28), into
 * OT entry 1 (D_800CCB04 + 4, re-read for every primitive).  The count is
 * compared signed, so a zero or negative count links nothing. */
static void strip_link(u32 prims, s32 count, u32 start) {
    s32 k;

    for (k = 0; k < count; k++) {
        add_prim(OT_HEAD() + 4u, prims + (start * 5u << 3));
        start += 2;
    }
}

/* The retail entry takes the strip's guest address as a pointer argument: the
 * bridge hands the host pointer of that guest word. */
void func_800728B8(void* prims, s32 count, s32 start) {
    strip_link(BG_ADDR(prims), count, (u32)start);
}

/* func_80074D4C.s, byte-exact: when D_800D2D28[0xC6] is set, pulse the
 * D_800C3E24 block's fade level (word +0xE0, +0xE5 = falling): fall by 4 and
 * stop (clamped to 0x40) below 0x40, or rise by 4 and turn (0xFC) from 0x100.
 * Then, for each of the four D_800C3E24[+0xE6 + i] flags that is set, give
 * the POLY_G3 [i * 2 + buffer(+0xE4)] (0x1C bytes each) the level's low byte
 * as r0 with g0 = b0 = 0 and link it into OT entry 1.  The block pointer and
 * the buffer byte are re-read before every store, as retail does (the four
 * triangles lie before +0xE4, but a buffer byte above 1 would reach it). */
void func_80074D4C(void) {
    u32 blk;
    u32 slot;
    u32 half;
    u32 prim;
    s32 level;

    if (BG_U8(MENU_BLK() + 0xC6u) == 0) {
        return;
    }
    blk = FX_BLK();
    if (BG_U8(blk + 0xE5u) != 0) {
        level = (s32)BG_U32(blk + 0xE0u) - 4;
        BG_U32(blk + 0xE0u) = (u32)level;
        if (level < 0x40) {
            BG_U8(blk + 0xE5u) = 0;
            blk = FX_BLK();
            BG_U32(blk + 0xE0u) = 0x40;
        }
    } else {
        level = (s32)BG_U32(blk + 0xE0u) + 4;
        BG_U32(blk + 0xE0u) = (u32)level;
        if (level >= 0x100) {
            BG_U8(blk + 0xE5u) = 1;
            blk = FX_BLK();
            BG_U32(blk + 0xE0u) = 0xFC;
        }
    }
    for (slot = 0; (s32)slot < 4; slot++) {
        blk = FX_BLK();
        if (BG_U8(blk + slot + 0xE6u) == 0) {
            continue;
        }
        half = slot * 2;
        BG_U8(((half + BG_U8(blk + 0xE4u)) * 0x1Cu) + blk + 4u) = BG_U8(blk + 0xE0u);
        blk = FX_BLK();
        BG_U8(((half + BG_U8(blk + 0xE4u)) * 0x1Cu) + blk + 5u) = 0;
        blk = FX_BLK();
        BG_U8(((half + BG_U8(blk + 0xE4u)) * 0x1Cu) + blk + 6u) = 0;
        blk = FX_BLK();
        prim = ((half + BG_U8(blk + 0xE4u)) * 0x1Cu) + blk;
        add_prim(OT_HEAD() + 4u, prim);
    }
}

/* One func_800728B8 strip of the D_800D2DB4 block: strip at +off, count byte
 * +cnt, start byte +start, each read from a fresh block pointer. */
#define PANEL_STRIP(off, cnt, start)                                         \
    do {                                                                     \
        u32 pb_ = PANEL_BLK();                                               \
        strip_link(pb_ + (off), (s32)BG_U8(pb_ + (cnt)), BG_U8(pb_ + (start))); \
    } while (0)

/* func_80073FB8.s, byte-exact: link into OT entry 1 the +0x8920 prim of the
 * D_800C3EA4 pool for buffer D_800CCB34 (0xC stride); while D_800C3E4C is 1, a
 * 0x34-byte prim for each of the four slots whose D_800D2D28 +0x7F flag is
 * set (index slot * 2 + the slot's +0x84 byte, at +0x5A0); the +0x8908 prim;
 * then for each of the seven D_800D2E38 records whose D_800D2D28 +0xB0 flag is
 * set, the record's strip (+0x5A0 count, +0x5A4 start) and ten prims picked by
 * the +0x5A4 index (eight 0x28-byte at +0x280/0x2D0/0x320/0x370/0x140/0x190/
 * 0x1E0/0x230, one 0x24-byte at +0x3C0, one 0xC-byte at +0x408).  The OT and
 * the index byte are re-read before every prim. */
void func_80073FB8(void) {
    s32 i;
    u32 blk;
    u32 rec;
    u32 ot;
    u32 buf;
    u32 pool;
    u32 idx;
    static const u32 ft4_off[8] = {0x280u, 0x2D0u, 0x320u, 0x370u, 0x140u, 0x190u, 0x1E0u, 0x230u};
    s32 n;

    ot = OT_HEAD();
    buf = BUF_IDX();
    pool = PRIM_BASE();
    add_prim(ot + 4u, buf * 0xCu + 0x8920u + pool);
    for (i = 0; i < 4; i++) {
        blk = MENU_BLK() + (u32)i;
        if (BG_U8(blk + 0x7Fu) != 0 && SLOT_FLAG() == 1) {
            ot = OT_HEAD();
            idx = (u32)i * 2u + BG_U8(blk + 0x84u);
            add_prim(ot + 4u, idx * 0x34u + 0x5A0u + PRIM_BASE());
        }
    }
    ot = OT_HEAD();
    buf = BUF_IDX();
    pool = PRIM_BASE();
    add_prim(ot + 4u, buf * 0xCu + 0x8908u + pool);
    for (i = 0; i < 7; i++) {
        blk = MENU_BLK();
        if (BG_U8(blk + (u32)i + 0xB0u) == 0) {
            continue;
        }
        rec = BG_U32(PANEL_TBL + (u32)i * 4u);
        strip_link(rec, (s32)BG_U32(rec + 0x5A0u), BG_U8(rec + 0x5A4u));
        for (n = 0; n < 8; n++) {
            ot = OT_HEAD();
            idx = BG_U8(rec + 0x5A4u);
            add_prim(ot + 4u, rec + (idx * 0x28u + ft4_off[n]));
        }
        ot = OT_HEAD();
        idx = BG_U8(rec + 0x5A4u);
        add_prim(ot + 4u, rec + (idx * 0x24u + 0x3C0u));
        ot = OT_HEAD();
        idx = BG_U8(rec + 0x5A4u);
        add_prim(ot + 4u, rec + (idx * 0xCu + 0x408u));
    }
}

/* func_80074AB8.s, byte-exact: window strips of the D_800D2DB4 block.  With
 * D_800D2D28[0xC7]: the four strips +0x3E80 / +0x5280 / +0x53C0 / +0x43D0
 * (count bytes +0x5D77/7C/7D/7F, start bytes +0x5D86/8B/8C/8E).  With
 * D_800D2D28[0xAD]: the 0x14-prim +0x46A0 strip (start +0x5D98), +0x5D70
 * (strip 0) and +0x5D74 (+0x1720); then the halfword counter +0x5DA2 is
 * incremented and, as a signed 16-bit value, below 0xF the +0x4CE0 strip is
 * linked (count +0x5DA1, start +0x5DA0) while from 0x15 it wraps to 0; then
 * the fourteen strips +0x3AC0 .. +0xFA0. */
void func_80074AB8(void) {
    u32 blk;
    u32 counter;

    if (BG_U8(MENU_BLK() + 0xC7u) != 0) {
        PANEL_STRIP(0x3E80u, 0x5D77u, 0x5D86u);
        PANEL_STRIP(0x5280u, 0x5D7Cu, 0x5D8Bu);
        PANEL_STRIP(0x53C0u, 0x5D7Du, 0x5D8Cu);
        PANEL_STRIP(0x43D0u, 0x5D7Fu, 0x5D8Eu);
    }
    if (BG_U8(MENU_BLK() + 0xADu) != 0) {
        blk = PANEL_BLK();
        strip_link(blk + 0x46A0u, 0x14, BG_U8(blk + 0x5D98u));
        PANEL_STRIP(0x0u, 0x5D70u, 0x5D92u);
        PANEL_STRIP(0x1720u, 0x5D74u, 0x5D83u);
        blk = PANEL_BLK();
        counter = BG_U16(blk + 0x5DA2u) + 1u;
        BG_U16(blk + 0x5DA2u) = (u16)counter;
        if ((s16)counter < 0xF) {
            strip_link(blk + 0x4CE0u, (s32)BG_U8(blk + 0x5DA1u), BG_U8(blk + 0x5DA0u));
        } else if ((s16)counter >= 0x15) {
            BG_U16(blk + 0x5DA2u) = 0;
        }
        PANEL_STRIP(0x3AC0u, 0x5D75u, 0x5D84u);
        PANEL_STRIP(0x3E30u, 0x5D76u, 0x5D85u);
        PANEL_STRIP(0x4E70u, 0x5D78u, 0x5D87u);
        PANEL_STRIP(0x4FB0u, 0x5D79u, 0x5D88u);
        PANEL_STRIP(0x50A0u, 0x5D7Au, 0x5D89u);
        PANEL_STRIP(0x51E0u, 0x5D7Bu, 0x5D8Au);
        PANEL_STRIP(0x2530u, 0x5D7Eu, 0x5D8Du);
        PANEL_STRIP(0x32A0u, 0x5D96u, 0x5D97u);
        PANEL_STRIP(0x3C0u, 0x5D71u, 0x5D93u);
        PANEL_STRIP(0xD20u, 0x5D72u, 0x5D94u);
        PANEL_STRIP(0xFA0u, 0x5D73u, 0x5D95u);
    }
}
#endif
