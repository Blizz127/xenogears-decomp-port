/* Host port bodies of func_8008D328 / func_8008D598 (the battle command menu
 * glyph builder and its fade).  The byte-exact C stays in main49.c under
 * #ifndef XENO_PC_PORT; this TU is the only owner an adopted host build links.
 *
 * Every overlay pointer is held as a u32 guest address and dereferenced
 * through BG_*, with retail's 32-bit wrapping arithmetic; each global is
 * re-read where retail re-reads it (pointer re-loads between the SetSemiTrans /
 * SetShadeTex calls and the stores that follow them). */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

/* PsyCross LIBGPU (production USE_EXTENDED_PRIM_POINTERS=0). */
extern void SetSemiTrans(void* p, s32 abe);
extern void SetShadeTex(void* p, s32 tge);
/* main4.c's native body: atlas entry `index` into the POLY_FT4 run at `polys`
 * (current buffer D_800CCB34); returns the glyph count it laid out. */
extern s32 func_80076A10(s32 index, u8* polys, s16 x, s16 y);

#define MENU_BLK() BG_U32(0x800D2D28u)    /* D_800D2D28: command menu block */
#define PRIM_BASE() BG_U32(0x800C3EA4u)   /* D_800C3EA4: prim pool */
#define ACTOR_BASE() BG_U32(0x800C3EACu)   /* D_800C3EAC: per-actor records */
#define BUF_IDX() BG_U32(0x800CCB34u)     /* D_800CCB34: current buffer */
#define MENU_TBL 0x800C3000u              /* D_800C3000: u8* per menu */
#define LIST_TBL 0x800C2F4Cu              /* D_800C2F4C: u16* per list */
#define FT4_SIZE 40u                      /* sizeof(POLY_FT4) */
#define GRP_STRIDE 0xFA0u                 /* prim bytes per glyph group */
#define GRP_BASE 0x641Cu                  /* group 0 offset in the prim pool */

/* func_8008D328.s, byte-exact: fade out the two groups of POLY_FT4 prims at
 * D_800C3EA4 + 0x641C + g * 0xFA0 (D_800D2D28 + 0xD0 + g * 4 glyphs each, two
 * prims per glyph, every other one from the D_800D2D28[0xA3] buffer):
 * SetSemiTrans(1), SetShadeTex(0), tpage (+0x16) |= 0x20, and if the r0 byte
 * (+4) is nonzero, r0/g0/b0 = r0 - D_800C3EAC[0x2E0] * 16.  When no prim was
 * lit, D_800D2D28[0xCB] = 0. */
void func_8008D328(void) {
    u8 active = 0;
    u32 g;
    u32 blk = 0; /* retail's cached D_800D2D28 value (a0) */

    for (g = 0; g < 2; g++) {
        u32 grp = g * GRP_STRIDE;           /* s4 */
        u32 base = grp + GRP_BASE;          /* s5/s3 */
        u32 slot = g * 4;                   /* s2 */
        s32 limit;
        u32 k;

        blk = MENU_BLK();
        if (BG_U32(blk + slot + 0xD0u) == 0) {
            continue;
        }
        limit = (s32)(BG_U32(blk + slot + 0xD0u) << 1);
        if (limit <= 0) {
            continue;
        }
        k = 0;
        do {
            u32 a1;
            u32 off;
            u32 prim;
            u8 c;

            /* SetSemiTrans: the cached block pointer, a fresh pool pointer */
            off = (k + BG_U8(blk + 0xA3u)) * FT4_SIZE;
            SetSemiTrans(BG_PTR(base + PRIM_BASE() + off), 1);
            /* SetShadeTex: both pointers re-read */
            blk = MENU_BLK();
            off = (k + BG_U8(blk + 0xA3u)) * FT4_SIZE;
            SetShadeTex(BG_PTR(base + PRIM_BASE() + off), 0);
            /* tpage |= 0x20 */
            blk = MENU_BLK();
            off = (k + BG_U8(blk + 0xA3u)) * FT4_SIZE;
            a1 = PRIM_BASE();
            prim = a1 + (off + grp);
            BG_U16(prim + 0x6432u) = (u16)(BG_U16(prim + 0x6432u) | 0x20);
            /* tint, with the byte and pool pointers cached as retail does */
            off = (k + BG_U8(blk + 0xA3u)) * FT4_SIZE;
            c = BG_U8(a1 + (off + grp) + 0x6420u);
            if (c != 0) {
                u32 v;

                active = 1;
                v = grp + a1 + off;
                BG_U8(v + 0x6420u) = (u8)(c - ((u32)BG_U8(ACTOR_BASE() + 0x2E0u) << 4));
                blk = MENU_BLK();
                off = (k + BG_U8(blk + 0xA3u)) * FT4_SIZE;
                v = grp + PRIM_BASE() + off;
                BG_U8(v + 0x6421u) = (u8)(c - ((u32)BG_U8(ACTOR_BASE() + 0x2E0u) << 4));
                blk = MENU_BLK();
                off = (k + BG_U8(blk + 0xA3u)) * FT4_SIZE;
                v = grp + PRIM_BASE() + off;
                BG_U8(v + 0x6422u) = (u8)(c - ((u32)BG_U8(ACTOR_BASE() + 0x2E0u) << 4));
            }
            blk = MENU_BLK();
            k += 2;
        } while ((s32)k < (s32)(BG_U32(blk + slot + 0xD0u) << 1));
    }
    if (active == 0) {
        BG_U8(MENU_BLK() + 0xCBu) = 0;
    }
}

/* func_8008D598.s, byte-exact: draw the battle command-menu glyph groups.
 * fade != 0: func_8008D328 and return D_800D2D28[0xA3].  Otherwise the menu's
 * two group numbers (D_800C3000[menu][0..1], 0xFF ends) and list numbers
 * ([2..3]) are copied out and both groups' glyph counts (D_800D2D28 + 0xD0 +
 * g * 4) cleared; each list (D_800C2F4C[l], 4-halfword entries code/x/y/attr
 * ending in 0xFFFF) is laid out through func_80076A10 into the group's prims.
 * A code >= 0x4000 is a per-actor remap: the D_800C3EAC[actor * 64 + (code &
 * 0xFF)] byte is also stored at D_800C3EAC[0x2C0 + (code & 3)], 0xFF skips the
 * entry, else it is the glyph.  The new prims (every other one of the current
 * buffer D_800CCB34) are tinted: c = (attr & 0xFF) >> 1, or for attr >= 0x2000
 * 0x10 (or (attr & 0xF0) >> 1 when the actor's halfword at +0x1C + (attr & 0xF)
 * * 2 is zero); c != 0: SetSemiTrans(1), SetShadeTex(0), r0/g0/b0 = c and
 * tpage |= 0x20, else r0/g0/b0 = 0x80.  Returns D_800CCB34. */
s32 func_8008D598(u8 actor, u8 menu, u8 fade) {
    u16 grp[2];
    u8 lst[2];
    u32 actor_off;
    u32 j;
    u32 i;

    if (fade != 0) {
        func_8008D328();
        return BG_U8(MENU_BLK() + 0xA3u);
    }
    for (i = 0; i < 2; i++) {
        grp[i] = BG_U8(BG_U32(MENU_TBL + (u32)menu * 4u) + i);
        lst[i] = BG_U8(BG_U32(MENU_TBL + (u32)menu * 4u) + i + 2u);
        BG_U32(MENU_BLK() + i * 4u + 0xD0u) = 0;
    }
    actor_off = (u32)actor << 6;
    for (j = 0; j < 2; j++) {
        u32 code;
        u32 k; /* halfword index of the entry, steps of 4 */

        if (grp[j] == 0xFF) {
            break;
        }
        code = BG_U16(BG_U32(LIST_TBL + (u32)lst[j] * 4u));
        if (code == 0xFFFF) {
            continue;
        }
        k = 0;
        for (;;) {
            u32 entry;
            u32 n0;
            u32 gofs;
            u32 cnt;
            u32 attr;
            u32 c;
            s32 ret;

            if (code >= 0x4000) {
                BG_U8(ACTOR_BASE() + (code & 3u) + 0x2C0u) =
                    BG_U8(ACTOR_BASE() + actor_off + (code & 0xFFu));
                code = BG_U8(ACTOR_BASE() + actor_off + (code & 0xFFu));
                if (code == 0xFF) {
                    goto next_entry;
                }
            }
            /* a1 = the group's next free prim run, n0 = its glyph count * 2
             * (taken before the call) */
            gofs = (u32)grp[j] * GRP_STRIDE + GRP_BASE;
            {
                u32 blk = MENU_BLK();
                u32 pool = PRIM_BASE();

                cnt = BG_U32(blk + (u32)grp[j] * 4u + 0xD0u);
                entry = BG_U32(LIST_TBL + (u32)lst[j] * 4u) + k * 2u;
                {
                    s16 x = BG_S16(entry + 2u);
                    s16 y = BG_S16(entry + 4u);

                    n0 = cnt << 1;
                    ret = func_80076A10((s32)code, BG_PTR(gofs + pool + cnt * 0x50u), x, y);
                }
            }
            BG_U32(MENU_BLK() + (u32)grp[j] * 4u + 0xD0u) =
                (u32)ret + BG_U32(MENU_BLK() + (u32)grp[j] * 4u + 0xD0u);
            entry = k * 2u + BG_U32(LIST_TBL + (u32)lst[j] * 4u);
            attr = BG_U16(entry + 6u);
            c = (attr & 0xFFu) >> 1;
            if (BG_U16(entry + 6u) >= 0x2000u) {
                c = 0x10;
                if (BG_U16(ACTOR_BASE() + actor_off + (attr & 0xFu) * 2u + 0x1Cu) == 0) {
                    c = (attr & 0xF0u) >> 1;
                }
            }
            if (c != 0) {
                s32 n = (s32)n0;

                if (n < (s32)(BG_U32(MENU_BLK() + (u32)grp[j] * 4u + 0xD0u) << 1)) {
                    do {
                        u32 g = grp[j];
                        u32 gp = g * GRP_STRIDE;
                        u32 a;

                        SetSemiTrans(BG_PTR(gp + GRP_BASE + PRIM_BASE() +
                                            ((u32)n + BUF_IDX()) * FT4_SIZE), 1);
                        SetShadeTex(BG_PTR(grp[j] * GRP_STRIDE + GRP_BASE + PRIM_BASE() +
                                           ((u32)n + BUF_IDX()) * FT4_SIZE), 0);
                        a = ((u32)n + BUF_IDX()) * FT4_SIZE + (grp[j] * GRP_STRIDE + PRIM_BASE());
                        BG_U8(a + 0x6420u) = (u8)c;
                        a = ((u32)n + BUF_IDX()) * FT4_SIZE + (grp[j] * GRP_STRIDE + PRIM_BASE());
                        BG_U8(a + 0x6421u) = (u8)c;
                        a = ((u32)n + BUF_IDX()) * FT4_SIZE + (grp[j] * GRP_STRIDE + PRIM_BASE());
                        BG_U8(a + 0x6422u) = (u8)c;
                        a = grp[j] * GRP_STRIDE + (((u32)n + BUF_IDX()) * FT4_SIZE) + PRIM_BASE();
                        BG_U16(a + 0x6432u) = (u16)(BG_U16(a + 0x6432u) | 0x20);
                        n += 2;
                    } while (n < (s32)(BG_U32(MENU_BLK() + (u32)grp[j] * 4u + 0xD0u) << 1));
                }
            } else {
                s32 n = (s32)n0;

                if (n < (s32)(BG_U32(MENU_BLK() + (u32)grp[j] * 4u + 0xD0u) << 1)) {
                    do {
                        u32 a;

                        a = ((u32)n + BUF_IDX()) * FT4_SIZE + (grp[j] * GRP_STRIDE + PRIM_BASE());
                        BG_U8(a + 0x6420u) = 0x80;
                        a = ((u32)n + BUF_IDX()) * FT4_SIZE + (grp[j] * GRP_STRIDE + PRIM_BASE());
                        BG_U8(a + 0x6421u) = 0x80;
                        a = ((u32)n + BUF_IDX()) * FT4_SIZE + (grp[j] * GRP_STRIDE + PRIM_BASE());
                        BG_U8(a + 0x6422u) = 0x80;
                        n += 2;
                    } while (n < (s32)(BG_U32(MENU_BLK() + (u32)grp[j] * 4u + 0xD0u) << 1));
                }
            }
        next_entry:
            k += 4;
            code = BG_U16(BG_U32(LIST_TBL + (u32)lst[j] * 4u) + k * 2u);
            if (code == 0xFFFF) {
                break;
            }
        }
    }
    return (s32)BUF_IDX();
}
#endif
