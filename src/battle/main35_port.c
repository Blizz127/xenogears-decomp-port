/* Host port body of the command-menu state-1 (Attack ring) handler.  The
 * byte-exact C stays in main35.c, which the retail build and the
 * standalone attack-ring test compile; an adopted host TU cannot, because
 * its D_800C3EAC is the guest-RAM alias, not a typed pointer. */
#include "common.h"
#ifdef XENO_PC_PORT
#include "battle_host_guest.h"

extern void func_80087A38(u32 actor);
extern void func_80084A7C(u32 actor);
extern u32 func_800841E0(u32 arg0);
extern u32 func_80083FF4(u32 arg0, u32 arg1);
extern void func_80077698(void);
extern void func_8008AA74(u32 sound);

/* D_800C3EAC: the command context, 64-byte actor rows from its base. */
#define CMD_CONTEXT() BG_U32(0x800C3EACu)
#define CMD_ROW(actor) (CMD_CONTEXT() + ((actor) & 0xFF) * 0x40u)

/* func_8008115C.s (main35.c): the ring event D_800D3014 drives the menu
 * state at context +0x2DD; a refused event plays sound 0x4F.  The context is
 * re-read after every call, as retail does. */
void func_8008115C(u32 arg0) {
    u32 actor;
    u32 row;

    switch (BG_U8(0x800D3014u)) {
    case 4:
    case 6:
    case 7:
        actor = arg0 & 0xFF;
        if (BG_U8(CMD_ROW(actor) + 0x3C) == 0xFF) {
            arg0 = 0x4F;
            goto reject;
        }
        BG_U8(0x800D366Cu) = 0;
        func_80087A38(actor);
        func_80084A7C(actor);
        func_80077698();
        BG_U8(CMD_CONTEXT() + 0x2DD) = 5;
        return;
    case 1:
        if (BG_U16(CMD_ROW(arg0) + 0x32) != 0) {
            arg0 = 0x4F;
            goto reject;
        }
        BG_U8(CMD_CONTEXT() + 0x2DD) = 2;
        return;
    case 2:
        BG_U8(CMD_CONTEXT() + 0x2DD) = 3;
        return;
    case 3:
        row = CMD_ROW(arg0);
        if (BG_U16(row + 0x30) == 0) {
            BG_U8(CMD_CONTEXT() + 0x2DD) = 4;
            return;
        }
        if (BG_U8(CMD_CONTEXT() + 0x2F6) != 0 && BG_U8(0x800C3E29u) == 3) {
            if (BG_U16(row + 0x2A) != 0) {
                func_8008AA74(0x4F);
            } else {
                BG_U8(CMD_CONTEXT() + 0x2DD) = 10;
            }
            BG_U8(CMD_CONTEXT() + 0x2F6) = 0;
            return;
        }
        arg0 = 0x4F;
        BG_U8(CMD_CONTEXT() + 0x2F6) = 1;
        goto reject;
    case 0:
        if (BG_U16(CMD_ROW(arg0) + 0x26) != 0) {
            arg0 = 0x4F;
            goto reject;
        }
        BG_U8(CMD_CONTEXT() + 0x2DD) = 7;
        return;
    default:
        return;
    }
reject:
    func_8008AA74(arg0);
}
/* ---- Attack-accept target list chain (retail C: target_*_impl.inc) ------ */
#define TL_COUNT 0x800D3274u    /* D_800D3274: entries in the list */
#define TL_LIST 0x800C3E90u     /* D_800C3E90: 12 target bytes, 0xFF = empty */
#define TL_GROUP 0x800C3EB4u    /* D_800C3EB4: group byte, 0x1C stride */
#define TL_BLOCKED 0x800C3EB7u  /* D_800C3EB7: blocked byte, 0x1C stride */
#define TL_RANK 0x800CCD34u     /* D_800CCD34: u16 rank, 0x170 stride */
#define TL_STATUS 0x800CCD64u   /* D_800CCD64: u16 status, 0x170 stride */
#define TL_ALT_STATUS 0x800CCE08u /* D_800CCE08: u16 status, 0x170 stride */
#define TL_PRESENT 0x800D2DCCu  /* D_800D2DCC: present byte */
#define TL_ALT 0x800D32A1u      /* D_800D32A1: alternate-status byte, stride 8 */
#define TL_MATRIX 0x800D3364u   /* D_800D3364: pointer to the group matrix */

/* func_80083FF4.s: may `side` target `id`?  An alternate-status target skips
 * the group matrix entirely, so the matrix pointer is read only on the
 * normal-status path.  The side argument is masked only for that path. */
u32 func_80083FF4(u32 arg0, u32 arg1) {
    u32 result = 0;
    u32 target = arg1 & 0xFF;

    if (BG_U8(TL_PRESENT + target) != 0 && BG_U8(TL_BLOCKED + target * 0x1Cu) == 0) {
        if (BG_U8(TL_ALT + target * 8u) == 0) {
            u32 target_group = BG_U8(TL_GROUP + target * 0x1Cu);
            u32 side_group = BG_U8(TL_GROUP + (arg0 & 0xFF) * 0x1Cu);

            if (BG_U8(BG_U32(TL_MATRIX) + target_group * 8u + side_group * 64u + 0x140u) == 0) {
                result = (BG_U16(TL_STATUS + target * 0x170u) & 0xC001u) == 0;
            }
        } else if ((BG_U16(TL_ALT_STATUS + target * 0x170u) & 0xC001u) == 0) {
            result = 1;
        }
    }
    return result;
}

/* func_800841E0.s: list the targets func_80083FF4 accepts for `side`
 * (3..10 when side < 3, else 0..2) into D_800C3E90, counting them in
 * D_800D3274.  Same-group targets come first in order, the rest follow;
 * then entry 0 is swapped with each later entry of lower rank (all of
 * them, or only the same-group ones when entry 0 is in the group).  Not a
 * full sort.  Returns entry 0, re-read.  The two 12-byte working arrays are
 * retail stack locals. */
u32 func_800841E0(u32 arg0) {
    u8 candidates[12];
    u8 matching[12];
    s32 i, total, output;
    u32 side = arg0 & 0xFF;

    BG_U8(TL_COUNT) = 0;
    for (i = 0; i < 12; i++) {
        candidates[i] = 0xFF;
        BG_U8(TL_LIST + i) = 0xFF;
        matching[i] = 0;
    }
    total = 0;
    if (side < 3) {
        for (i = 3; i < 11; i++) {
            if (func_80083FF4(side, i) & 0xFF) {
                candidates[total++] = (u8)i;
                BG_U8(TL_COUNT)++;
            }
        }
    } else {
        for (i = 0; i < 3; i++) {
            if (func_80083FF4(side, i) & 0xFF) {
                candidates[total++] = (u8)i;
                BG_U8(TL_COUNT)++;
            }
        }
    }
    output = 0;
    for (i = 0; i < total; i++) {
        if (BG_U8(TL_GROUP + side * 0x1Cu) == BG_U8(TL_GROUP + candidates[i] * 0x1Cu)) {
            BG_U8(TL_LIST + output) = candidates[i];
            candidates[i] = 0xFF;
            matching[output] = 1;
            output++;
        }
    }
    for (i = 0; i < total; i++) {
        if (candidates[i] != 0xFF) {
            BG_U8(TL_LIST + output) = candidates[i];
            output++;
        }
    }
    if (matching[0] != 0) {
        for (i = 1; i < total; i++) {
            if (matching[i] != 0 &&
                BG_U16(TL_RANK + BG_U8(TL_LIST) * 0x170u) >
                    BG_U16(TL_RANK + BG_U8(TL_LIST + i) * 0x170u)) {
                u8 first = BG_U8(TL_LIST);

                BG_U8(TL_LIST) = BG_U8(TL_LIST + i);
                BG_U8(TL_LIST + i) = first;
            }
        }
    } else {
        for (i = 1; i < total; i++) {
            if (BG_U16(TL_RANK + BG_U8(TL_LIST) * 0x170u) >
                BG_U16(TL_RANK + BG_U8(TL_LIST + i) * 0x170u)) {
                u8 first = BG_U8(TL_LIST);

                BG_U8(TL_LIST) = BG_U8(TL_LIST + i);
                BG_U8(TL_LIST + i) = first;
            }
        }
    }
    return BG_U8(TL_LIST);
}

/* func_80084A7C.s: the selection starts as the actor's current target
 * (context +0x2E8, stored before the list is built); if the rebuilt list
 * still holds it, it stays, otherwise it becomes entry 0.  The context
 * pointer and the actor's target are re-read after the build, as retail
 * does, and the context again on the fallback. */
void func_80084A7C(u32 arg0) {
    u32 actor = arg0 & 0xFF;
    u32 count;
    u32 target;
    u32 i;
    s32 found = 0;

    BG_U8(BG_U32(0x800C3EACu) + 0x2E8u) = BG_U8(BG_U32(0x800C3EACu) + actor * 0x40u + 0x3Cu);
    func_800841E0(actor);
    count = BG_U8(TL_COUNT);
    if (count != 0) {
        target = BG_U8(BG_U32(0x800C3EACu) + actor * 0x40u + 0x3Cu);
        for (i = 0; i < count; i++) {
            if (BG_U8(TL_LIST + i) == target) {
                found++;
                break;
            }
        }
    }
    if (found == 0) {
        BG_U8(BG_U32(0x800C3EACu) + 0x2E8u) = BG_U8(TL_LIST);
    }
}
#endif
