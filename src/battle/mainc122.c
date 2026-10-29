#include "common.h"


#ifndef XENO_PC_PORT
#include "psyq/libgte.h"
extern MATRIX D_800C3760;
extern void func_80021B24(void* dst, void* src);
extern void func_800BD810(u8* glyph, u32 rgb);

/* func_800BE1C4.s: work callback -- draw the task's glyph quads (0x18-byte
 * entries from +0x70, count +0x6C, colour +0x68) six times, as
 * func_800BDD3C (mainc120) does once: each pass rotates by the copied +0x38
 * angles, translates to (160, 70) * 2 at the projection distance, composes
 * with D_800C3760 and scales by the copied +0x40 vector, then steps the z
 * angle by -5 and each scale axis by -0x330.  pad (as in func_800BDD3C)
 * fills sp+0x30..0x37, so rot sits at sp+0x38 as in retail. */
void func_800BE1C4(u8* task) {
    MATRIX m;
    u8 pad[8];
    SVECTOR rot;
    VECTOR t;
    VECTOR scale;
    long ofx;
    long ofy;
    u8* o = *(u8**)(task + 4);
    s32 k;
    s32 i;
    u8* e;

    ReadGeomOffset(&ofx, &ofy);
    t.vx = (0xA0 - ofx) * 2;
    t.vy = (0x46 - ofy) * 2;
    t.vz = ReadGeomScreen();
    D_800C3760.t[2] = ReadGeomScreen();
    func_80021B24(&rot, o + 0x38);
    scale.vx = *(s32*)(o + 0x40);
    scale.vy = *(s32*)(o + 0x44);
    scale.vz = *(s32*)(o + 0x48);
    for (k = 0; k != 6; k++) {
        RotMatrix(&rot, &m);
        TransMatrix(&m, &t);
        CompMatrix(&D_800C3760, &m, &m);
        ScaleMatrix(&m, &scale);
        SetRotMatrix(&m);
        SetTransMatrix(&m);
        for (i = 0, e = o + 0x70; i != *(s32*)(o + 0x6C); i++, e += 0x18) {
            func_800BD810(e, *(u32*)(o + 0x68));
        }
        rot.vz -= 5;
        scale.vx -= 0x330;
        scale.vy -= 0x330;
        scale.vz -= 0x330;
    }
}
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern int rand(void);
extern void func_800BE11C(void);
extern void func_800BE1C4(u8* task);
extern void func_800BE6E8(s32 value, u8* out, s32 digits, u8 lead, s32 bias);
extern s32 func_80026DCC(u8* pTable, s32 index, u8* pPrimBuffer, s16 ofsX, s16 ofsY);
extern s32 D_800D3630;
extern s16 D_800C3752[];
extern u8* D_800D2F5C[];

/* func_800BE330.s: spawn a number popup task (0x130 bytes; timer
 * func_800BE11C, work func_800BE1C4) for `value`: random drift +0x64 from
 * {16, 8, 6, -8}, +0x60 = 0x20, code 0x2E, scale 0x2000, rotation 0, colour
 * by D_800D3630 (2 green, 3 magenta, 0xB blue, else grey), the digits as
 * glyph quads from +0x70 (count +0x6C), each glyph's +6/+7 shrunk by one.
 * Codegen points: the cases go 0xB, 3, 2, default (the order the bodies
 * appear in retail); D_800D3630 is read before the zeroing stores; the glyph
 * buffer offset is added to t last; the shrink loop indexes from t. */
void func_800BE330(s32 value) {
    u8* t;
    u8 digits[8];
    s32 v;
    s32 x;
    s32 i;
    u8* g;
    s32 mode;

    t = (u8*)WorkListsAddTasks(0x130, 0, func_800BE11C, func_800BE1C4, 0);
    v = -(((rand() & 3) - 2) * 8);
    *(s32*)(t + 0x64) = v;
    if (v == 0) {
        *(s32*)(t + 0x64) = 6;
    }
    *(s32*)(t + 0x60) = 0x20;
    t[0x6B] = 0x2E;
    *(s32*)(t + 0x40) = 0x2000;
    *(s32*)(t + 0x44) = 0x2000;
    *(s32*)(t + 0x48) = 0x2000;
    mode = D_800D3630;
    *(s32*)(t + 0x6C) = 0;
    *(s16*)(t + 0x38) = 0;
    *(s16*)(t + 0x3A) = 0;
    *(s16*)(t + 0x3C) = 0;
    switch (mode) {
    case 0xB:
        t[0x68] = 0;
        t[0x69] = 0;
        t[0x6A] = 0x80;
        break;
    case 3:
        t[0x68] = 0x80;
        t[0x69] = 0;
        t[0x6A] = 0x80;
        break;
    case 2:
        t[0x68] = 0;
        t[0x69] = 0x80;
        t[0x6A] = 0;
        break;
    default:
        t[0x68] = 0x80;
        t[0x69] = 0x80;
        t[0x6A] = 0x80;
        break;
    }
    func_800BE6E8(value, digits, 5, 0, 0);
    x = D_800C3752[digits[0]];
    *(s32*)(t + 0x6C) = 0;
    for (i = 0; i != digits[0]; i++, x += 10) {
        *(s32*)(t + 0x6C) += func_80026DCC(D_800D2F5C[0], digits[i + 1] + 0x72,
                                           t + (*(s32*)(t + 0x6C) * 0x18 + 0x70), x, -8);
    }
    for (i = 0; i != *(s32*)(t + 0x6C); i++) {
        t[i * 0x18 + 0x76]--;
        t[i * 0x18 + 0x77]--;
    }
}
extern u8 D_800C3EB0[];
extern u8* D_800C3610[];
extern s32 D_80059464;
extern u8 D_800591AC;
extern u8 D_800C3780;
extern void func_800245D8(u32 a0, u32 a1);
extern void func_800BD3AC(u8* target, u32 value, u32 kind);
extern s32 func_800BF6F8(void);
extern void func_800BE790(void);
extern void func_800BE0DC(void);

/* func_800BE538.s: show party slot `slot`'s popups synchronously: with
 * D_80059464 = 0, D_800591AC = 1 and D_800C3780 = 1,
 * play the slot actor's animation 0xA, spawn up to three popups
 * (func_800BD3AC kinds 0, 1 and 0xA for the nonzero values) with
 * D_800C3610 forced to 1, run frames (func_800BE790) until func_800BF6F8
 * reports idle and the animation leaves 0xA, then restore the animation and
 * the flags and call func_800BE0DC.  main36 declares it (u8 slot, ...). */
void func_800BE538(u32 slot, u32 v0, u32 v1, u32 v2) {
    u8* base;
    u8* actor;
    s32 anim;
    u8* saved;

    D_80059464 = 0;
    D_800591AC = 1;
    base = (u8*)(u32)D_800C3EB0;
    actor = *(u8**)(slot * 4 + base + 0x8C8C);
    D_800C3780 = 1;
    if (actor != NULL) {
        anim = *(s8*)(actor + 0xAF);
        func_800245D8((u32)actor, 0xA);
        saved = D_800C3610[0];
        D_800C3610[0] = (u8*)1;
        if (v0 != 0) {
            func_800BD3AC(actor, v0, 0);
        }
        if (v1 != 0) {
            func_800BD3AC(actor, v1, 1);
        }
        if (v2 != 0) {
            func_800BD3AC(actor, v2, 0xA);
        }
        D_800C3610[0] = saved;
        while (func_800BF6F8() != 0) {
            func_800BE790();
        }
        while (*(s8*)(actor + 0xAF) == 0xA) {
            func_800BE790();
        }
        func_800245D8((u32)actor, anim);
    }
    D_800C3780 = 0;
    func_800BE0DC();
    D_80059464 = 0;
    D_800591AC = 0;
}
extern u8 D_800C3784[];

void func_800BE6A0(s32 value, u8* out, s32 count, s32 bias) {
    s32 i;
    s32 n;

    for (i = 0; i != count; i++) {
        n = count - 1;
        out[i + 1] = D_800C3784[(value >> ((n - i) * 4)) & 0xF] + bias;
    }
    out[0] = count;
}

extern u32 D_800C37A4[];

/* format `value` as decimal glyphs: `digits` indexes the power-of-ten table
 * D_800C37A4; leading zero digits are skipped until the first non-zero digit
 * or until `lead` is set; a negative value gets a 0x2D glyph first and one
 * digit fewer; out[0] = glyph count.  Divisions are unsigned. */
void func_800BE6E8(s32 value, u8* out, s32 digits, u8 lead, s32 bias) {
    s32 count = 0;
    u8* w = out + 1;
    u32* p;
    u8* base;
    u32 q;

    if (value < 0) {
        count = 1;
        out[1] = 0x2D;
        w = out + 2;
        value = -value;
        digits--;
    }
    base = (u8*)D_800C37A4;
    p = (u32*)(base + digits * 4);
    value = (u32)value % *p;
    while (digits != 0) {
        p--;
        q = (u32)value / *p;
        digits--;
        if ((u8)q != 0) {
            lead = 1;
        }
        if (lead != 0) {
            *w++ = q + bias;
            count++;
        }
        value = (u32)value % *p;
    }
    *w = value + bias;
    out[0] = count + 1;
}

#include "battle/spad_stack.h"
extern s32 D_800C37D0[];
extern u8 D_800C3EB0[];
extern u8 D_800D309C[];
extern s32 D_80010000[];
extern s16 D_80059494__s[] asm("D_80059494");
/* Scalar view for the clamp: under BattleCdkG0 (-G0) its address is split,
 * and as a VAR_DECL left-hand side its %hi is set up before the Vsync call. */
extern s16 D_80059494__v asm("D_80059494");
extern s32 D_80059198[];
extern u8 D_800CCB94[];
extern u32* g_GfxCurOT[];
extern u8* D_800C3610[];
extern u16 D_800591B4[];
extern u8 D_800C37CC[];
extern s32 Vsync__r(s32 mode) asm("Vsync");
extern void DrawSync(u32 v);
extern void GameCheckAndHandleSoftReset(void);
extern u32* ClearOTagR(u32* ot, s32 n);
extern void func_800BEBC4(void);
extern void func_80280A9C(void);
extern void func_800250E0(s32 buf);
extern void func_800BBAB8(void);
extern void func_800BB9D4(void);
extern void func_80024FF4(u8* p);
extern void GfxSetCurrentOT(u32* ot);
extern void FontDrawLetters(void* ot);
extern void func_800A9A50(u8* a0, u8* a1, u32* ot, s32 buf);
extern void func_8001D468(void);
extern void WorkListUpdate(void);
extern void TimerWorkListUpdate(void);
extern void func_80076544(void);
extern void func_8008A9C0(s32 mode);
extern void* PutDispEnv(void* env);
extern void* PutDrawEnv(void* env);
extern void func_80025044(void);
extern void DrawOTag(u32* p);
extern void func_800BEB04(void);
extern void func_800B8068(u16 v);
extern s32 ArchiveDataSync(void);
extern void func_800B136C(void);

/* Battle frame driver: swaps the double buffer and clears its OT, runs the
 * frame's work lists on the scratchpad stack plus one catch-up tick per
 * extra elapsed vblank (D_80059494), records Vsync timings in D_800D309C's
 * stats block, clamps the next catch-up count to 0..4, presents the frame,
 * and on the outermost call services the pending callback, the queued
 * D_800591B4 request and the D_800C37CC archive sync. */
void func_800BE790(void) {
    u8* base;
    u8* gfx;
    u8* frame;
    u8* stats;
    u8* tail;
    u8* env;
    s32 t;
    u8* buf;
    s32 n;

    D_800C37D0[0]++;
    GameCheckAndHandleSoftReset();
    t = Vsync__r(-1);
    frame = (u8*)(u32)D_800D309C;
    *(s32*)(frame + 0x48) = t;
    base = (u8*)(u32)D_800C3EB0;
    gfx = base + 0x8000;
    buf = base + 0xB70;
    if (*(u8**)(gfx + 0xC50) == buf) {
        buf = base + 0x4BE0;
    }
    *(u8**)(gfx + 0xC50) = buf;
    *(u32**)(gfx + 0xC54) = (u32*)(buf + 0x70);
    ClearOTagR((u32*)(buf + 0x70), 0x1000);
    *(s32*)(gfx + 0xC84) = 1 - *(s32*)(gfx + 0xC84);
    if (D_80010000[0] != -1) {
        func_800BEBC4();
        __asm__ volatile(".word 0x0001000D");
        func_80280A9C();
    }
    func_800250E0(*(s32*)(gfx + 0xC84));
    func_800BBAB8();
    func_800BB9D4();
    func_80024FF4(frame + 0x20);
    GfxSetCurrentOT(*(u32**)(gfx + 0xC54));
    if (D_80010000[0] != -1) {
        FontDrawLetters(*(u32**)(gfx + 0xC54));
    }
    func_800A9A50(frame + 0x20, D_800CCB94, g_GfxCurOT[0], *(s32*)(gfx + 0xC84));
    BATTLE_SPAD_STACK_SET(0x1F8003FC);
    func_8001D468();
    WorkListUpdate();
    TimerWorkListUpdate();
    n = D_80059494__s[0];
    while (--n != -1) {
        func_800BBAB8();
        TimerWorkListUpdate();
    }
    BATTLE_SPAD_STACK_RESET();
    func_80076544();
    func_8008A9C0(0);
    while (--D_80059494__s[0] != -1) {
        func_8008A9C0(1);
    }
    t = Vsync__r(1);
    stats = (u8*)(u32)D_800D309C;
    *(s32*)(stats + 0x40) = t;
    DrawSync(0);
    *(s32*)(stats + 0x44) = Vsync__r(1);
    D_80059494__v = Vsync__r(-1) - *(s32*)(stats + 0x48) - D_80059198[0];
    if (D_80059494__v < 0) {
        D_80059494__v = 0;
    }
    if (D_80059494__v >= 5) {
        D_80059494__v = 4;
    }
    tail = (u8*)(u32)D_800C3EB0;
    *(s32*)(tail + 0x8000 + 0xDAC) = D_80059494__v + D_80059198[0];
    if (D_80059198[0] != 0) {
        Vsync__r(D_80059198[0] + 1);
    } else {
        Vsync__r(0);
    }
    env = (u8*)(u32)D_800C3EB0;
    env += 0x8000;
    PutDispEnv(*(u8**)(env + 0xC50) + 0x5C);
    PutDrawEnv(*(u8**)(env + 0xC50));
    func_80025044();
    DrawOTag((u32*)(*(u8**)(env + 0xC50) + 0x406C));
    func_800BEB04();
    if (D_800C37D0[0] == 1) {
        if (D_800C3610[0] != NULL) {
            (*(void (**)(u8*))(D_800C3610[0] + 8))(D_800C3610[0]);
        }
        if (D_800591B4[0] != 0) {
            s32 v = D_800591B4[0]; /* s32: retail tests v0, then copies it to a0 */

            D_800591B4[0] = 0;
            func_800B8068(v);
        }
        if (D_800C37CC[0] != 0 && ArchiveDataSync() == 0) {
            D_800C37CC[0] = 0;
            func_800B136C();
        }
    }
    D_800C37D0[0]--;
}
#endif


extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(void);
extern void func_800B5CC0(void);
extern void func_800BDC14(void);
extern void func_800BDF1C(void);
extern void func_8001E148(u32 v);
extern void func_800C08CC(u32 a0, void* a1, void* a2);
extern void func_800B51B0(void);
extern void func_800245D8(u32 a0, u32 a1);
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void func_800B7424(u32 p);
extern void func_800B7364(void);
extern void func_800B6F0C(void);
extern void func_800B7134(void);
extern void WorkListSetTaskCallback(void* pTask, void* callback);
extern void D_80025A88(void);
#ifndef XENO_PC_PORT
extern u8 D_800D3420[];
#endif
#ifndef XENO_PC_PORT
extern u32 D_800C3CE8[];
#endif
extern void WorkListTaskSetOnFreeCallback(void* pTask, void* callback);
extern void func_800B3358(u8* p);
extern void func_800B3588(u8* p);
#ifndef XENO_PC_PORT
extern u32 D_800C3548[];
#endif


/* func_800BEB04.s */
extern u8 D_800591B2[];
extern u8 D_800591B3[];
extern u8 D_800591B0[];
extern void func_800B8354(void);
extern void ArchiveGetArchiveOffsetIndices(u32* a, u32* b);
extern u32 ArchiveSetIndex(u32 dir, u32 entry);
extern void ArchiveReadFileToBuffer(u32 a0, void* buf, u32 a2, u32 a3);
extern void DrawSync(u32 v);
extern void Vsync(u32 v);
extern void EnterCriticalSection(void);
extern void FlushCache(void);
extern void ExitCriticalSection(void);
#ifndef XENO_PC_PORT /* port: lifted body in the h-frontier block */
void func_800BEB04(void) {
    u8 cur = D_800591B2[0];
    u8 nxt = D_800591B3[0];
    u32 a, b;

    if (cur != nxt) {
        D_800591B2[0] = nxt;
        func_800B8354();
        ArchiveGetArchiveOffsetIndices(&a, &b);
        ArchiveSetIndex(0xC, 2);
        ArchiveReadFileToBuffer((u32)nxt + 2, (void*)0x801FC000u, 0, 0x80);
        func_800B8354();
        ArchiveSetIndex(a, b);
        DrawSync(0);
        Vsync(0);
        EnterCriticalSection();
        FlushCache();
        ExitCriticalSection();
    }
    D_800591B0[0] = 1;
}
#endif
/* func_800BEBC4.s */
#ifndef XENO_PC_PORT
extern u8 D_800C3EB0[];
#endif
extern u16 D_80059494[];
extern void func_800BEC18(void);
extern void Vsync(u32 v);
void func_800BEBC4(void) {
    u8* base;
    u8* q;

    func_800BEC18();
    base = (u8*)(u32)D_800C3EB0;
    q = base + 0x8000;
    if ((*(u16*)(q + 0xC58) & 0x100) != 0) {
        Vsync(8);
        D_80059494[0] = 0;
    }
}


#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
/* func_800BE6A0.s: expand the low `count` nibbles of `value` (most
 * significant first) into glyph codes: out[0] = count, out[1..count] =
 * D_800C3784[nibble] + bias.  The shift is SRAV, so `value` is signed and
 * only the low five bits of the shift amount count. */
void func_800BE6A0(s32 value, u8* out, u32 count, u32 bias) {
    u32 i;

    for (i = 0; i != count; i++) {
        u32 nibble = (u32)(value >> (((count - 1 - i) * 4) & 31)) & 0xF;
        out[i + 1] = (u8)(BG_U8(0x800C3784 + nibble) + bias);
    }
    out[0] = (u8)count;
}

/* func_800BE6E8.s: format `value` as decimal glyphs.  `digits` indexes the
 * power-of-ten table D_800C37A4; leading zero digits are skipped until the
 * first non-zero digit or until `lead` is set.  A negative value gets a '-'
 * glyph first and one digit fewer.  out[0] receives the glyph count.  The
 * divisions are DIVU with no zero check, so they use the R3000 results. */
void func_800BE6E8(s32 value, u8* out, s32 digits, u32 lead, u32 bias) {
    u8* w = out + 1;
    u32 count = 0;
    u32 n = (u32)value;
    u32 t;

    if (value < 0) {
        count = 1;
        out[1] = 0x2D;
        w = out + 2;
        n = 0u - (u32)value;
        digits--;
    }
    t = 0x800C37A4 + (u32)digits * 4;
    n = bg_remu(n, BG_U32(t));
    while (digits != 0) {
        u32 q;

        t -= 4;
        q = bg_divu(n, BG_U32(t));
        digits--;
        if ((q & 0xFF) != 0) {
            lead = 1;
        }
        if ((lead & 0xFF) != 0) {
            *w++ = (u8)(q + bias);
            count++;
        }
        n = bg_remu(n, BG_U32(t));
    }
    *w = (u8)(n + bias);
    out[0] = (u8)(count + 1);
}
#endif

#ifdef XENO_PC_PORT /* h-frontier lifted bodies */
/* Instruction-faithful port bodies (native-only frontier), generated by
 *   tools/scripts/battle_lift_asm.py asm/battle/matchings/<tu>/<fn>.s --args N [--void] --jal-service
 * (include/battle_lift.h; every call goes through the interpreter's own
 * jal path, so each callee gets the binding an interpreted jal gets).  The
 * matched C above stays the matching build's. */
#include "battle_lift.h"

/* func_800BEB04.s (48 instructions; --args 0 --void), frontier step1.  Callees through the jal service: ArchiveGetArchiveOffsetIndices, ArchiveReadFileToBuffer, ArchiveSetIndex, DrawSync, EnterCriticalSection, ExitCriticalSection, FlushCache, Vsync, func_800B8354. */
void func_800BEB04(void) {
    static const char FN[] = "func_800BEB04";
    u32 r_at = 0, r_v0 = 0, r_v1 = 0, r_a0 = 0, r_a1 = 0, r_a2 = 0, r_a3 = 0, r_t0 = 0, r_t1 = 0, r_t2 = 0, r_t3 = 0, r_t4 = 0, r_t5 = 0, r_t6 = 0, r_t7 = 0, r_s0 = 0, r_s1 = 0, r_s2 = 0, r_s3 = 0, r_s4 = 0, r_s5 = 0, r_s6 = 0, r_s7 = 0, r_t8 = 0, r_t9 = 0, r_k0 = 0, r_k1 = 0, r_gp = 0, r_sp = 0, r_fp = 0, r_ra = 0;
    u32 hi = 0, lo = 0, bl_saved;
    r_sp = bl_enter(FN, 0x20u, &bl_saved);
    r_a0 = 0x80060000u; /* 800BEB04 lui $a0, %hi(D_800591B2) */
    r_v0 = 0x80060000u; /* 800BEB08 lui $v0, %hi(D_800591B3) */
    r_v1 = bl_lbu(r_v0 + 0xFFFF91B3u); /* 800BEB0C lbu $v1, %lo(D_800591B3)($v0) */
    r_v0 = bl_lbu(r_a0 + 0xFFFF91B2u); /* 800BEB10 lbu $v0, %lo(D_800591B2)($a0) */
    r_sp = r_sp + 0xFFFFFFE0u; /* 800BEB14 addiu $sp, $sp, -0x20 */
    bl_sw(r_sp + 0x1Cu, r_ra); /* 800BEB18 sw $ra, 0x1C($sp) */
    bl_sw(r_sp + 0x18u, r_s0); /* 800BEB1C sw $s0, 0x18($sp) */
    r_s0 = r_v1 & 0xFFu; /* 800BEB20 andi $s0, $v1, 0xFF */
    /* 800BEB24 beq $v0, $s0, .L800BEBA4 */
    { int c_ = (r_v0 == r_s0); ; if (c_) goto L_800BEBA4; }
    /* 800BEB2C jal func_800B8354 */
    bl_sb(r_a0 + 0xFFFF91B2u, r_v1);
    bl_jal(FN, 0x800B8354u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800B8354 */
    r_a0 = r_sp + 0x10u; /* 800BEB34 addiu $a0, $sp, 0x10 */
    /* 800BEB38 jal ArchiveGetArchiveOffsetIndices */
    r_a1 = r_sp + 0x14u;
    bl_jal(FN, 0x800284B4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveGetArchiveOffsetIndices */
    r_a0 = 0u + 0xCu; /* 800BEB40 addiu $a0, $zero, 0xC */
    /* 800BEB44 jal ArchiveSetIndex */
    r_a1 = 0u + 0x2u;
    bl_jal(FN, 0x80028470u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveSetIndex */
    r_a0 = r_s0 + 0x2u; /* 800BEB4C addiu $a0, $s0, 0x2 */
    r_a1 = 0x801F0000u; /* 800BEB50 lui $a1, (0x801FC000 >> 16) */
    r_a1 = r_a1 | 0xC000u; /* 800BEB54 ori $a1, $a1, (0x801FC000 & 0xFFFF) */
    r_a2 = 0u + 0u; /* 800BEB58 addu $a2, $zero, $zero */
    /* 800BEB5C jal ArchiveReadFileToBuffer */
    r_a3 = 0u + 0x80u;
    bl_jal(FN, 0x800295D8u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveReadFileToBuffer */
    /* 800BEB64 jal func_800B8354 */
    ;
    bl_jal(FN, 0x800B8354u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* func_800B8354 */
    r_a0 = bl_lw(r_sp + 0x10u); /* 800BEB6C lw $a0, 0x10($sp) */
    r_a1 = bl_lw(r_sp + 0x14u); /* 800BEB70 lw $a1, 0x14($sp) */
    /* 800BEB74 jal ArchiveSetIndex */
    ;
    bl_jal(FN, 0x80028470u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ArchiveSetIndex */
    /* 800BEB7C jal DrawSync */
    r_a0 = 0u + 0u;
    bl_jal(FN, 0x800445D0u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* DrawSync */
    /* 800BEB84 jal Vsync */
    r_a0 = 0u + 0u;
    bl_jal(FN, 0x8004B54Cu, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* Vsync */
    /* 800BEB8C jal EnterCriticalSection */
    ;
    bl_jal(FN, 0x800404D4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* EnterCriticalSection */
    /* 800BEB94 jal FlushCache */
    ;
    bl_jal(FN, 0x80040454u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* FlushCache */
    /* 800BEB9C jal ExitCriticalSection */
    ;
    bl_jal(FN, 0x800404E4u, r_a0, r_a1, r_a2, r_a3, r_sp, &r_v0, &r_v1); /* ExitCriticalSection */
L_800BEBA4:
    r_v1 = 0x80060000u; /* 800BEBA4 lui $v1, %hi(D_800591B0) */
    r_v0 = 0u + 0x1u; /* 800BEBA8 addiu $v0, $zero, 0x1 */
    bl_sb(r_v1 + 0xFFFF91B0u, r_v0); /* 800BEBAC sb $v0, %lo(D_800591B0)($v1) */
    r_ra = bl_lw(r_sp + 0x1Cu); /* 800BEBB0 lw $ra, 0x1C($sp) */
    r_s0 = bl_lw(r_sp + 0x18u); /* 800BEBB4 lw $s0, 0x18($sp) */
    r_sp = r_sp + 0x20u; /* 800BEBB8 addiu $sp, $sp, 0x20 */
    /* 800BEBBC jr $ra */
    ; goto bl_exit;
bl_exit:
    bl_leave(bl_saved);
    (void)hi; (void)lo; (void)r_at; (void)r_v1; (void)r_k0; (void)r_k1; (void)r_gp; (void)r_ra;
    (void)r_v0;
}
#endif /* XENO_PC_PORT h-frontier */
