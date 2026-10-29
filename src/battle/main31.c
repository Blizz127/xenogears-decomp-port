#include "common.h"

#ifdef XENO_PC_PORT
#include "battle_guest_call.h"
#define MAIN31_CALL_8FA60() BATTLE_GUEST_CALL(0x8008FA60u, 0u)
#define MAIN31_CALL_7765C() BATTLE_GUEST_CALL0(0x8007765Cu)
#else
#define MAIN31_CALL_8FA60() func_8008FA60(0)
#define MAIN31_CALL_7765C() func_8007765C()
#endif

#ifndef XENO_PC_PORT
extern u8 func_8007A7BC(u8** pp, u8* buf, u8 e, u8 state);
extern u8 func_8007A828(u8** pp, u8* buf, u8 state);
extern void func_8007A874(u8** pp, u8* buf, u8 e, u8 state);
extern void func_8007A8B4(u8** pp, u8* buf);
extern void func_8007A900(u8** pp, u8 e);
extern void func_8007A92C(u8** pp, u8 e);
extern void func_8007A968(u8** pp, u8 e);
extern void func_8007A9A8(u8** pp);
extern void func_8007A9D0(u8** pp, u8 e);
extern void func_8007AA1C(u8** pp, u8 e);
extern void func_8007AA60(u8** pp, u8 e);
extern void func_8007AAB8(u8** pp, u8 e);
extern void func_8007AAF4(u8** pp, u8 e);
extern void func_8007AB30(u8** pp, u8 e);
extern void func_8007AB68(u8** pp, u8 e);
extern void func_8007ABA0(u8** pp, u8 e);
extern void func_8007ABD8(u8** pp, u8 e);
extern void func_8007AC30(u8** pp, u8 e);
extern void func_8007AC80(u8** pp, u8 e);
extern void func_8007ACDC(u8** pp, u8 e);
extern void func_8007AD24(u8** pp, u8 e);
extern void func_8007AD6C(u8** pp, u8 e);
extern void func_8007ADB0(u8** pp, u8 e);
extern void func_8007ADF4(u8** pp, u8 e);
extern void func_8007AE38(u8** pp, u8 e);
extern void func_8007AE98(u8** pp, u8 e);
extern void func_8007AEF0(u8** pp, u8 e);
extern void func_8007AF5C(u8** pp, u8 e);
extern void func_8007AFAC(u8** pp, u8 e);
extern void func_8007AFFC(u8** pp, u8 e);
extern void func_8007B040(u8** pp, u8 e);
extern void func_8007B084(u8** pp, u8 e);
extern void func_8007B0C8(u8** pp, u8 e);
extern void func_8007B134(u8** pp, u8 e);
extern void func_8007B198(u8** pp, u8 e);
extern void func_8007B208(u8** pp, u8 e);
extern void func_8007B264(u8** pp, u8 e);
extern void func_8007B2C0(u8** pp, u8 e);
extern void func_8007B310(u8** pp, u8 e);
extern void func_8007B360(u8** pp, u8 e);
extern void func_8007B3B0(u8** pp, u8 e);
extern void func_8007B3E4(u8** pp, u8 e);
extern void func_8007B424(u8** pp, u8 e);
extern u8 func_8007B4B8(u8** pp, u8 e, u8 state);
extern void func_8007B578(u8** pp, u8 e);
extern u8 func_8007B608(u8** pp, u8 e, u8 state);
extern void func_8007B6C0(u8** pp, u8 e);
extern u8 func_8007B7B0(u8** pp, u8 e, u8 state);
extern void func_8007B8D4(u8** pp, u8 e);
extern void func_8007B914(u8** pp, u8 e);
extern void func_8007B958(u8** pp, u8 e);
extern void func_8007B98C(u8** pp, u8 e);
extern void func_8007B9C8(u8** pp, u8 e);
extern void func_8007BA04(u8** pp, u8 e);
extern void func_8007BA44(u8** pp, u8 e);
extern void func_8007BA88(u8 e);
extern void func_8007BAB8(u8 e);
extern void func_8007BAE8(u8** pp, u8 e);
extern void func_8007BB2C(u8** pp, u8 e);
extern void func_8007BB70(u8** pp, u8 e);
extern void func_8007BBD8(u8** pp, u8 e);
extern void func_8007BC40(u8** pp, u8* buf, u8 state);
extern void func_8007BC84(u8** pp, u8 e);
extern void func_8007BCE8(u8** pp, u8 e);
extern void func_8007BD5C(u8** pp, u8 e);
extern void func_8007BEA8(u8** pp, u8 e);
extern void func_8007C040(u8** pp, u8 e);
extern void func_8007C1A4(u8** pp, u8 e);
extern void func_8007C33C(u8** pp, u8 e);
extern void func_8007C4A0(u8** pp, u8 e);
extern void func_8007C580(u8** pp, u8 e);
extern void func_8007C678(u8** pp, u8 e);
extern void func_8007C75C(u8** pp, u8 e);
extern void func_8007C840(u8** pp, u8 e);
extern void func_8007C9D4(u8** pp, u8 e);
extern void func_8007CB20(u8** pp, u8 e);
extern void func_8007CC50(u8** pp, u8 e);
extern void func_8007CD10(u8** pp, u8 e);
extern void func_8007CDD0(u8** pp, u8 e);
extern void func_8007CEA4(u8** pp, u8 e);
extern void func_8007CFB8(u8** pp, u8 e);
extern void func_8007D0CC(u8** pp, u8 e);
extern void func_8007D148(u8** pp, u8* buf, u8 e, u8 state);
extern void func_8007D1A8(u8** pp, u8 e);
extern void func_8007D1DC(u8** pp, u8 e);
extern void func_8007D30C(u8** pp, u8 e);
extern void func_8007D344(u8** pp, u8 e);
extern void func_8007D478(u8** pp, u8 e);
extern void func_8007D5B0(u8** pp, u8 e);
extern void func_8007D610(u8** pp, u8 e);
extern void func_8007D6A8(u8** pp, u8 e);
extern void func_8007D7B4(u8** pp, u8 e);
extern void func_8007D8C0(u8** pp, u8 e);
extern void func_8007DA1C(u8** pp, u8 e);
extern void func_8007DB78(u8** pp, u8 e);
extern void func_8007DCF8(u8** pp, u8 e);
extern void func_8007DE78(u8** pp, u8 e);
extern void func_8007DFD4(u8** pp, u8 e);
extern void func_8007E154(u8** pp, u8 e);
extern void func_8007E1D0(u8** pp, u8 e);
extern void func_8007E234(u8** pp, u8 e);
extern void func_8007E334(u8** pp, u8 e);
extern void func_8007E438(u8** pp, u8 e);
extern void func_8007E554(u8** pp, u8 e);
extern void func_8007E674(u8 e);
extern void func_8007E6A0(u8** pp, u8 e);
extern void func_8007E6F0(u8** pp, u8 e);
extern void func_8007E740(u8** pp, u8 e);
extern void func_8007E780(u8** pp, u8 e);
extern void func_8007E7C0(u8** pp);
extern void func_8007E7E4(u8** pp, u8 e);
extern void func_8007E8AC(u8** pp);
extern void func_8007E8E0(u8** pp, u8 e);
extern void func_8007E934(void);
extern void func_80079934(u8** pp);
extern u8 D_800D2E5C[];

/* func_8007EF6C.s, byte-exact: run the AI-script action opcode at *pp for
 * enemy `e` (main13's runners send opcodes below 0x80 here).  Opcodes
 * 0x01..0x74 call their handler with pp and some of buf (the D_800D2E5C
 * work buffer), e and state; 0x62 does nothing here (main13 tests it);
 * 0x6E, 0x6F and anything out of range go to func_8007A7BC.
 * func_80079934 then steps past the record and the state is returned. */
u8 func_8007EF6C(u8** pp, u8 e, u8 state) {
    u8* buf = D_800D2E5C;

    switch (**pp) {
    case 0x1:
        state = func_8007A828(pp, buf, state);
        break;
    case 0x2:
        func_8007A874(pp, buf, e, state);
        break;
    case 0x3:
        func_8007A8B4(pp, buf);
        break;
    case 0x4:
        func_8007A900(pp, e);
        break;
    case 0x5:
        func_8007A92C(pp, e);
        break;
    case 0x6:
        func_8007A968(pp, e);
        break;
    case 0x7:
        func_8007A9A8(pp);
        break;
    case 0x8:
        func_8007A9D0(pp, e);
        break;
    case 0x9:
        func_8007AA1C(pp, e);
        break;
    case 0xA:
        func_8007AA60(pp, e);
        break;
    case 0xB:
        func_8007AAB8(pp, e);
        break;
    case 0xC:
        func_8007AAF4(pp, e);
        break;
    case 0xD:
        func_8007AB30(pp, e);
        break;
    case 0xE:
        func_8007AB68(pp, e);
        break;
    case 0xF:
        func_8007ABA0(pp, e);
        break;
    case 0x10:
        func_8007ABD8(pp, e);
        break;
    case 0x11:
        func_8007AC30(pp, e);
        break;
    case 0x12:
        func_8007AC80(pp, e);
        break;
    case 0x13:
        func_8007ACDC(pp, e);
        break;
    case 0x14:
        func_8007AD24(pp, e);
        break;
    case 0x15:
        func_8007AD6C(pp, e);
        break;
    case 0x16:
        func_8007ADB0(pp, e);
        break;
    case 0x17:
        func_8007ADF4(pp, e);
        break;
    case 0x18:
        func_8007AE38(pp, e);
        break;
    case 0x19:
        func_8007AE98(pp, e);
        break;
    case 0x1A:
        func_8007AEF0(pp, e);
        break;
    case 0x1B:
        func_8007AF5C(pp, e);
        break;
    case 0x1C:
        func_8007AFAC(pp, e);
        break;
    case 0x1D:
        func_8007AFFC(pp, e);
        break;
    case 0x1E:
        func_8007B040(pp, e);
        break;
    case 0x1F:
        func_8007B084(pp, e);
        break;
    case 0x20:
        func_8007B0C8(pp, e);
        break;
    case 0x21:
        func_8007B134(pp, e);
        break;
    case 0x22:
        func_8007B198(pp, e);
        break;
    case 0x23:
        func_8007B208(pp, e);
        break;
    case 0x24:
        func_8007B264(pp, e);
        break;
    case 0x25:
        func_8007B2C0(pp, e);
        break;
    case 0x26:
        func_8007B310(pp, e);
        break;
    case 0x27:
        func_8007B360(pp, e);
        break;
    case 0x28:
        func_8007B3B0(pp, e);
        break;
    case 0x29:
        func_8007B3E4(pp, e);
        break;
    case 0x2A:
        func_8007B424(pp, e);
        break;
    case 0x2B:
        state = func_8007B4B8(pp, e, state);
        break;
    case 0x2C:
        func_8007B578(pp, e);
        break;
    case 0x2D:
        state = func_8007B608(pp, e, state);
        break;
    case 0x2E:
        func_8007B6C0(pp, e);
        break;
    case 0x2F:
        state = func_8007B7B0(pp, e, state);
        break;
    case 0x30:
        func_8007B8D4(pp, e);
        break;
    case 0x31:
        func_8007B914(pp, e);
        break;
    case 0x32:
        func_8007B958(pp, e);
        break;
    case 0x33:
        func_8007B98C(pp, e);
        break;
    case 0x34:
        func_8007B9C8(pp, e);
        break;
    case 0x35:
        func_8007BA04(pp, e);
        break;
    case 0x36:
        func_8007BA44(pp, e);
        break;
    case 0x37:
        func_8007BA88(e);
        break;
    case 0x38:
        func_8007BAB8(e);
        break;
    case 0x39:
        func_8007BAE8(pp, e);
        break;
    case 0x3A:
        func_8007BB2C(pp, e);
        break;
    case 0x3B:
        func_8007BB70(pp, e);
        break;
    case 0x3C:
        func_8007BBD8(pp, e);
        break;
    case 0x3D:
        func_8007BC40(pp, buf, state);
        break;
    case 0x3E:
        func_8007BC84(pp, e);
        break;
    case 0x3F:
        func_8007BCE8(pp, e);
        break;
    case 0x40:
        func_8007BD5C(pp, e);
        break;
    case 0x41:
        func_8007BEA8(pp, e);
        break;
    case 0x42:
        func_8007C040(pp, e);
        break;
    case 0x43:
        func_8007C1A4(pp, e);
        break;
    case 0x44:
        func_8007C33C(pp, e);
        break;
    case 0x45:
        func_8007C4A0(pp, e);
        break;
    case 0x46:
        func_8007C580(pp, e);
        break;
    case 0x47:
        func_8007C678(pp, e);
        break;
    case 0x48:
        func_8007C75C(pp, e);
        break;
    case 0x49:
        func_8007C840(pp, e);
        break;
    case 0x4A:
        func_8007C9D4(pp, e);
        break;
    case 0x4B:
        func_8007CB20(pp, e);
        break;
    case 0x4C:
        func_8007CC50(pp, e);
        break;
    case 0x4D:
        func_8007CD10(pp, e);
        break;
    case 0x4E:
        func_8007CDD0(pp, e);
        break;
    case 0x4F:
        func_8007CEA4(pp, e);
        break;
    case 0x50:
        func_8007CFB8(pp, e);
        break;
    case 0x51:
        func_8007D0CC(pp, e);
        break;
    case 0x52:
        func_8007D148(pp, buf, e, state);
        break;
    case 0x53:
        func_8007D1A8(pp, e);
        break;
    case 0x54:
        func_8007D1DC(pp, e);
        break;
    case 0x55:
        func_8007D30C(pp, e);
        break;
    case 0x56:
        func_8007D344(pp, e);
        break;
    case 0x57:
        func_8007D478(pp, e);
        break;
    case 0x58:
        func_8007D5B0(pp, e);
        break;
    case 0x59:
        func_8007D610(pp, e);
        break;
    case 0x5A:
        func_8007D6A8(pp, e);
        break;
    case 0x5B:
        func_8007D7B4(pp, e);
        break;
    case 0x5C:
        func_8007D8C0(pp, e);
        break;
    case 0x5D:
        func_8007DA1C(pp, e);
        break;
    case 0x5E:
        func_8007DB78(pp, e);
        break;
    case 0x5F:
        func_8007DCF8(pp, e);
        break;
    case 0x60:
        func_8007DE78(pp, e);
        break;
    case 0x61:
        func_8007DFD4(pp, e);
        break;
    case 0x62:
        break;
    case 0x63:
        func_8007E154(pp, e);
        break;
    case 0x64:
        func_8007E1D0(pp, e);
        break;
    case 0x65:
        func_8007E234(pp, e);
        break;
    case 0x66:
        func_8007E334(pp, e);
        break;
    case 0x67:
        func_8007E438(pp, e);
        break;
    case 0x68:
        func_8007E554(pp, e);
        break;
    case 0x69:
        func_8007E674(e);
        break;
    case 0x6A:
        func_8007E6A0(pp, e);
        break;
    case 0x6B:
        func_8007E6F0(pp, e);
        break;
    case 0x6C:
        func_8007E740(pp, e);
        break;
    case 0x6D:
        func_8007E780(pp, e);
        break;
    case 0x70:
        func_8007E7C0(pp);
        break;
    case 0x71:
        func_8007E7E4(pp, e);
        break;
    case 0x72:
        func_8007E8AC(pp);
        break;
    case 0x73:
        func_8007E8E0(pp, e);
        break;
    case 0x74:
        func_8007E934();
        break;
    default:
        state = func_8007A7BC(pp, buf, e, state);
        break;
    }
    func_80079934(pp);
    return state;
}
extern u32 func_8007E954(u8** pp, u8 index);
extern u32 func_8007E98C(u8** pp, u8 index);
extern u32 func_8007E9D0(u8** pp, u8 index);
extern u32 func_8007EA08(u8** pp, u8 index);
extern u32 func_8007EA4C(u8** pp, u8 index);
extern u32 func_8007EA84(u8** pp, u8 index);
extern u32 func_8007EAC8(u8** pp, u8 index);
extern u32 func_8007EB08(u8** pp, u8 index);
extern u32 func_8007EB50(u8** pp, u8 index);
extern u32 func_8007EB90(u8** pp, u8 index);
extern u32 func_8007EBD8(u8** pp, u8 index);
extern u32 func_8007EC10(u8** pp, u8 index);
extern u32 func_8007EC54(u8** pp, u8 index);
extern u32 func_8007EC94(u8** pp, u8 index);
extern u32 func_8007ECDC(u8** pp, u8 index);
extern u32 func_8007ED14(u8** pp, u8 index);
extern u32 func_8007ED58(u8** pp, u8 index);
extern u32 func_8007ED98(u8** pp, u8 index);
extern u32 func_8007EDE0(u8** pp, u8 index);
extern u32 func_8007EE28(u8** pp, u8 index);
extern u32 func_8007EE70(u8** pp);
extern u32 func_8007EEA8(u8** pp);
extern u32 func_8007EED0(void);
extern u32 func_8007EEE8(void);
extern u32 func_8007EF44(u8 index);

/* func_8007F8C0.s, byte-exact: evaluate the AI-script condition at *pp
 * for enemy `index`.  0x80 and 0x9A are true, 0x99 opens an OR group,
 * 0x81..0x98 and 0x9B are main30's tests; func_80079934 steps past each
 * record.  Outside a group the first result is returned; inside one the
 * results are ORed while the next opcode is still >= 0x80. */
u8 func_8007F8C0(u8** pp, u8 index) {
    u8 result = 0;
    u8 multi = 0;
    u8 acc = 0;

    while (1) {
        switch (**pp) {
        case 0x80:
        case 0x9A:
            result = 1;
            break;
        case 0x99:
            multi = 1;
            break;
        case 0x81:
            result = func_8007E954(pp, index);
            break;
        case 0x82:
            result = func_8007E98C(pp, index);
            break;
        case 0x83:
            result = func_8007E9D0(pp, index);
            break;
        case 0x84:
            result = func_8007EA08(pp, index);
            break;
        case 0x85:
            result = func_8007EA4C(pp, index);
            break;
        case 0x86:
            result = func_8007EA84(pp, index);
            break;
        case 0x87:
            result = func_8007EAC8(pp, index);
            break;
        case 0x88:
            result = func_8007EB08(pp, index);
            break;
        case 0x89:
            result = func_8007EB50(pp, index);
            break;
        case 0x8A:
            result = func_8007EB90(pp, index);
            break;
        case 0x8B:
            result = func_8007EBD8(pp, index);
            break;
        case 0x8C:
            result = func_8007EC10(pp, index);
            break;
        case 0x8D:
            result = func_8007EC54(pp, index);
            break;
        case 0x8E:
            result = func_8007EC94(pp, index);
            break;
        case 0x8F:
            result = func_8007ECDC(pp, index);
            break;
        case 0x90:
            result = func_8007ED14(pp, index);
            break;
        case 0x91:
            result = func_8007ED58(pp, index);
            break;
        case 0x92:
            result = func_8007ED98(pp, index);
            break;
        case 0x93:
            result = func_8007EDE0(pp, index);
            break;
        case 0x94:
            result = func_8007EE28(pp, index);
            break;
        case 0x95:
            result = func_8007EE70(pp);
            break;
        case 0x96:
            result = func_8007EEA8(pp);
            break;
        case 0x97:
            result = func_8007EED0();
            break;
        case 0x98:
            result = func_8007EEE8();
            break;
        case 0x9B:
            result = func_8007EF44(index);
            break;
        }
        func_80079934(pp);
        if (multi == 0) {
            break;
        }
        acc |= result;
        if (**pp < 0x80) {
            result = acc;
            break;
        }
    }
    return result;
}
extern u8 D_800D2D24[];
extern u8* D_800D2D28;
extern void func_8008FA60(s32 a0);
extern void func_8007765C(void);
#endif

/* func_8007FB70.s */
void func_8007FB70(u8 index) {
    if (D_800D2D24[index] == 4 && D_800D2D28[0x8E] != 0) {
        MAIN31_CALL_8FA60();
        MAIN31_CALL_7765C();
        D_800D2D28[0x8E] = 0;
    }
}

#ifndef XENO_PC_PORT
/* func_8007FBE0.s, byte-exact: lay out count - 1 cursor cells (four
 * halfwords each: 0xC, glyph + 0x5E, 0x12, glyph + 0x5E) in the
 * D_800C3EA4 work area at +0x910, 16-byte cells from slot D_800CCB34 two
 * per entry, with the glyphs from D_800C3202 row `count`.  Then record
 * the selection (minus one when it equals the count) and the base slot's
 * low byte in D_800D2D28+0x97/+0x98.  Codegen (gcc 2.6.x/2.7.2):
 * - The cells are stored as struct fields, which gcc treats as in-struct.
 *   So the fixed D_800C3EA4 word is loaded once before the loop, while
 *   the D_800CCB34 slot (a struct-field view) is reloaded before every
 *   store, as in retail.
 * - Indexing a CursorCell pointer gives retail's offset-first address
 *   add; a struct member array gives base-first.
 * - The glyph is read through a [][6] row view, which loop.c reduces to
 *   retail's incremented pointer.  With a plain index the giv is "not
 *   worth while", and a source pointer is set before the entry test.
 * - A plain for loop gives the single blez.  Its exit-test temp is the
 *   dead pseudo behind retail's empty 8-byte frame; an enclosing
 *   if (n > 0) duplicates the test. */
extern u8* D_800C3EA4;
typedef struct BattleCursorSlot { s32 slot; } BattleCursorSlot;
extern BattleCursorSlot D_800CCB34__s asm("D_800CCB34");
extern u8 D_800C3202__r[][6] asm("D_800C3202");
typedef struct { u16 h0, h1, h2, h3; u8 pad[8]; } CursorCell;
extern u8* D_800D2D28;

void func_8007FBE0(u8 count, u8 sel) {
    u8 chosen = sel;
    s32 i;

    if (count == sel) {
        chosen = sel - 1;
    }
    for (i = 0; i < count - 1; i++) {
        ((CursorCell*)(D_800C3EA4 + 0x910))[i * 2 + D_800CCB34__s.slot].h0 = 0xC;
        ((CursorCell*)(D_800C3EA4 + 0x910))[i * 2 + D_800CCB34__s.slot].h1 = D_800C3202__r[count][i] + 0x5E;
        ((CursorCell*)(D_800C3EA4 + 0x910))[i * 2 + D_800CCB34__s.slot].h2 = 0x12;
        ((CursorCell*)(D_800C3EA4 + 0x910))[i * 2 + D_800CCB34__s.slot].h3 = D_800C3202__r[count][i] + 0x5E;
    }
    D_800D2D28[0x97] = chosen;
    D_800D2D28[0x98] = *(u8*)&D_800CCB34__s.slot;
}
#endif


#ifndef XENO_PC_PORT
extern u8* D_800D2D28;
#endif
#ifndef XENO_PC_PORT
extern u32 D_800D367C;
#endif
#ifndef XENO_PC_PORT
extern u8 D_800D2D24[];
#endif
#ifndef XENO_PC_PORT
extern u8 D_800C3202[];
#endif

#ifdef XENO_PC_PORT
#include "battle_guest_call.h"
/* The port's HeapFree takes the host pointer; D_800D367C holds the guest
 * address the (bridged) HeapAlloc returned. */
extern u_int HeapFree(void* p);
#define MAIN31_HEAP_PTR(word) BATTLE_HOST_PTR(word)
#else
extern void HeapFree(u32 p);
#define MAIN31_HEAP_PTR(word) (word)
#endif


/* func_8007FCE8.s */
void func_8007FCE8(void) {
    if (D_800D2D28[0xAE] != 0) {
        HeapFree(MAIN31_HEAP_PTR(D_800D367C));
        D_800D2D28[0xAE] = 0;
    }
}


#ifdef XENO_PC_PORT
/* func_8007FBE0.s: lay out count-1 cursor cells (4 halfwords at +0x910 in
 * the D_800C3EA4 work area, 32-byte stride from slot D_800CCB34) from the
 * D_800C3202 row `count`, then record the selection (minus one when it equals
 * the count) and the base slot in D_800D2D28+0x97/+0x98. */
void func_8007FBE0(u8 count, u8 sel) {
    /* D_800C3EA4 (work area address) and D_800CCB34 (slot word) are read as
     * the raw guest words retail loads: other TUs declare them with other
     * types, so the shared guest-RAM aliases do not fit this view. */
    u8 chosen = sel;
    s32 n = (s32)count - 1;
    s32 i;

    if (count == sel) {
        chosen = sel - 1;
    }
    if (n > 0) {
        u32 work = BATTLE_G32(0x800C3EA4u);
        u8* src = D_800C3202 + count * 6;

        for (i = 0; i < n; i++, src++) {
            BATTLE_G16(work + (((u32)i * 2 + BATTLE_G32(0x800CCB34u)) << 4) + 0x910) = 0xC;
            BATTLE_G16(work + (((u32)i * 2 + BATTLE_G32(0x800CCB34u)) << 4) + 0x912) = *src + 0x5E;
            BATTLE_G16(work + (((u32)i * 2 + BATTLE_G32(0x800CCB34u)) << 4) + 0x914) = 0x12;
            BATTLE_G16(work + (((u32)i * 2 + BATTLE_G32(0x800CCB34u)) << 4) + 0x916) = *src + 0x5E;
        }
    }
    D_800D2D28[0x97] = chosen;
    D_800D2D28[0x98] = BATTLE_G8(0x800CCB34u);
}
#endif
