#include "common.h"
#ifndef XENO_PC_PORT
#include "system/memory.h"
#endif


#ifndef XENO_PC_PORT
void func_800BD024(void *arg0);
#endif

#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BCD98);
INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BCEAC);
#include "psyq/libgte.h"

typedef struct {
    u8 pad0[0x38];
    SVECTOR* sprite; /* +0x38; x at +2, y at +6, z at +0xA */
    s32 x;           /* +0x3C */
    s32 y;           /* +0x40 */
    s32 z;           /* +0x44 */
    u8 pad48[6];
    s16 spin;        /* +0x4E */
    u8 pad50[4];
    s32 height;      /* +0x54 */
    s32 phase;       /* +0x58 */
} Popup;

extern void func_800BCAFC(void* sprite, s32 phase);

void func_800BCFAC(u8* task) {
    Popup* s = *(Popup**)(task + 4);
    s16* sprite = (s16*)s->sprite;
    s32 phase = s->phase;
    s32 height = s->height;
    s32 y;
    s32 z;

    s->x = sprite[1];
    y = sprite[3];
    phase++;
    s->y = y;
    z = sprite[5];
    s->phase = phase;
    s->y = y - height - 0x20;
    s->z = z;
    func_800BCAFC(sprite, phase);
    s->spin += 0x10;
}


#ifndef XENO_PC_PORT
void func_800BD024(void *arg0) {
    void *temp_a0;
    void *temp_s1;

    temp_s1 = *(void **)((s8*)(arg0) + 4);
    temp_a0 = *(void **)((s8*)(temp_s1) + 0x38);
    *(u8 *)((s8*)(temp_a0) + 0x2B) = (u8) (*(u8 *)((s8*)(temp_a0) + 0x2B) | 1);
    func_8001F6B0(temp_a0);
    func_80025180(*(s32 *)((s8*)(temp_s1) + 0x5C));
    func_8001CE74(arg0);
    TimerWorkListRemoveTask(arg0);
    HeapFree(temp_s1);
}
#endif

INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BD098);
INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BD1FC);
INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BD2E4);
INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BD3AC);
INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BD7A0);
INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BD810);
#include "psyq/libgte.h"

extern MATRIX D_800D30BC;

/* project `point` through D_800D30BC: out = (2*(sx - ofx), 2*(sy - ofy),
 * screen distance). */
void func_800BD974(SVECTOR* point, s32* out) {
    SVECTOR unused;
    DVECTOR sxy;
    long p;
    long flag;
    long ofx;
    long ofy;

    SetRotMatrix(&D_800D30BC);
    SetTransMatrix(&D_800D30BC);
    RotTransPers(point, (long*)&sxy, &p, &flag);
    ReadGeomOffset(&ofx, &ofy);
    out[0] = (sxy.vx - ofx) * 2;
    out[1] = (sxy.vy - ofy) * 2;
    out[2] = ReadGeomScreen();
}

INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BDA1C);
extern void func_800BDF1C(void);

/* fade step: run the shared popup step, darken the three colour bytes at
 * +0x80..+0x82 by 2, count down +0x88 and call +0x0C(task) once it goes
 * negative. */
void func_800BDB08(u8* task) {
    func_800BDF1C();
    task[0x80] -= 2;
    task[0x81] -= 2;
    task[0x82] -= 2;
    if (--*(s32*)(task + 0x88) < 0) {
        (*(void (**)(u8*))(task + 0xC))(task);
    }
}

INCLUDE_ASM("asm/battle/nonmatchings/mainc118", func_800BDB74);
#endif
extern void func_800BDB74(void* task);
/* func_800BDC14.s: run func_800BDF1C, count the task's +0x88 timer down and,
 * when it expires, rearm it at 0x10, set bit 1 / clear bit 0 of +0x83 and
 * switch the task callback to func_800BDB74. */
void func_800BDC14(u8* task) {
    s32 t;

    func_800BDF1C();
    t = *(s32*)(task + 0x88) - 1;
    *(s32*)(task + 0x88) = t;
    if (t < 0) {
        *(s32*)(task + 0x88) = 0x10;
        task[0x83] = (task[0x83] | 2) & 0xFE;
        TimerWorkListSetTaskCallback(task, func_800BDB74);
    }
}


extern u8* TimerWorkListAllocateTask(u32 owner, u32 size);
extern void TimerWorkListSetTaskCallback(void* pTask, void* callback);
extern u32 func_800B57E4(u8* s);
extern void func_800B5B3C(u8* task);
extern void func_800B5854(void);
extern void func_800BF73C(void);
extern void func_800B5CC0(void);
extern void func_800BDC14(u8* task);
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


/* func_800BDC78.s */
void func_800BDC78(u8* a0) {
    u8* s0 = a0;
    s32 v;

    func_800BDF1C();
    if (*(u8*)(s0 + 0x7C) != 0) {
        *(u32*)(s0 + 0x48) += 0xC0000;
    } else {
        *(u32*)(s0 + 0x48) += 0xFFF40000;
    }
    v = *(s32*)(s0 + 0x88) - 1;
    *(s32*)(s0 + 0x88) = v;
    if (v < 0) {
        *(s32*)(s0 + 0x88) = 0x10;
        TimerWorkListSetTaskCallback(s0, func_800BDC14);
    }
}
/* func_800BDCF8.s */
#ifndef XENO_PC_PORT
extern u32 D_800D2D68[];
#endif
#ifndef XENO_PC_PORT
extern void WorkListRemoveTask(u32 p);
extern void TimerWorkListRemoveTask(u32 p);
void func_800BDCF8(u8* p) {
    D_800D2D68[0] = 0;
    WorkListRemoveTask((u32)(p + 0x1C));
    TimerWorkListRemoveTask((u32)p);
}
#else
#include "battle_host_guest.h"
/* Port body: the task is a native work-list entry pair; hand the native
 * removers host pointers instead of truncated u32s. */
void func_800BDCF8(u8* p) {
    extern void WorkListRemoveTask(void* pTargetEntry);
    extern void TimerWorkListRemoveTask(void* pTargetEntry);

    BG_U32(0x800D2D68) = 0;
    WorkListRemoveTask(p + 0x1C);
    TimerWorkListRemoveTask(p);
}
#endif


#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
#include "psyq/libgte.h"
#include <string.h>
/* Port bodies for the battle HUD damage/number popups and the sprite
 * "highlight" tasks.  Every overlay address is a guest address (BG_*);
 * task pointers handed out by the native work-list allocators are raw u32
 * host addresses, which BG_* dereference unchanged. */
extern void func_800BCB54(void);
extern void func_800BCC60(void);
extern void func_800BCAFC(u8* sprite, s32 phase);
extern void func_800BDF1C(void);
extern void func_800BDE58(void);
extern u32 func_800B16A4(const u8* data);
extern void func_800B1720(u8* src, u8* dst, u32 a2, u32 a3);
extern void func_800B1F6C(void* model, void* buffer, void* ot, s32 unused,
                          s32 depthBias, s32 tpage);
extern void func_800BE6E8(s32 value, u8* out, s32 digits, u32 lead, u32 bias);
extern void* func_8001D0A4(void* pTask, void* pCallback);
extern void func_8001F6B0(void* pSpriteData);
extern void func_80025180(void* pData);
extern void func_8001CE74(void* pTargetEntry);
extern void TimerWorkListRemoveTask(void* pTargetEntry);
/* The file-scope matching extern types WorkListsAddTasks with u32 owner and
 * result; call the native definition (pc_port/src/work_list_port.c) with its
 * real pointer types. */
#define BG_WORKLISTS_ADD_TASKS \
    ((void* (*)(u32, void*, void*, void*, void*))(void*)WorkListsAddTasks)
extern s32 ReadGeomScreen(void);
extern MATRIX* ScaleMatrixL(MATRIX* m, VECTOR* v);
void func_800BD098(u8* owner);
void func_800BD3AC(u8* target, u32 value, u32 kind);
void func_800BCFAC(u8* task);
void func_800BD810(u8* glyph, u32 rgb);
void func_800BD974(SVECTOR* point, s32* out);
extern void WorkListsDeleteTasks(void* pTasks);
extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern void func_80021B04(void* arg0, s16 arg1, s16 arg2, s16 arg3);
extern void func_80021B14(void* arg0, s32 arg1, s32 arg2, s32 arg3);
extern s32 func_80021AD8(s32 color, s32 value);
extern s32 func_80026DCC(u8* pTable, s32 index, u8* pPrimBuffer, s16 ofsX,
                         s16 ofsY);
extern u8 D_800591AC;
extern s32 D_80059464;
extern void* g_GfxCurWorkBuffer;
extern void* g_GfxCurWorkBufferEnd;
extern u_long* g_GfxCurOT;
extern MATRIX D_8004FBB8;
extern SVECTOR D_8004FB98[4];

#define BG_CODE(a) ((void*)(uintptr_t)(a))
/* D_800C3EB0 + 0x8C8C / + 0x8CB8: the 11 per-slot actor record pointers and
 * their popup-owner task pointers. */
#define BATTLE_SLOT_RECORDS (0x800C3EB0u + 0x8C8Cu)
#define BATTLE_SLOT_OWNERS (0x800C3EB0u + 0x8CB8u)
/* D_800C360C: 1-based current actor; its 0x48-byte record at D_800C3EB0. */
#define BATTLE_CUR_RECORD() (0x800C3EB0u + (BG_U32(0x800C360C) - 1) * 0x48u)

/* func_800BCD98.s: latch the slot mask in D_800C3D14, (re)build the shared
 * popup state, then for each of the 11 slots start the highlight task
 * (func_800BD098) where the mask bit is set and none is running, and free
 * the running one (its on-free callback) where the bit is clear. */
void func_800BCD98(u32 mask) {
    u32 i;

    BG_U16(0x800C3D14) = (u16)mask;
    if ((mask & 0xFFFF) != 0) {
        func_800BCC60();
    } else {
        func_800BCB54();
    }
    for (i = 0; i != 11; i++) {
        u32 owner = BG_U32(BATTLE_SLOT_OWNERS + i * 4);

        if ((mask & 1) != 0) {
            if (owner != 0 &&
                func_8001D0A4(bg_arg(owner), BG_CODE(0x800BCFACu)) == NULL) {
                func_800BD098(BG_PTR(owner));
            }
        } else if (owner != 0) {
            u32 task = (u32)(uintptr_t)func_8001D0A4(bg_arg(owner),
                                                     BG_CODE(0x800BCFACu));

            if (task != 0) {
                bg_jalr(BG_U32(task + 0xC), task, 0, 0, 0);
            }
        }
        mask = (mask & 0xFFFF) >> 1;
    }
}

/* func_800BCEAC.s: work callback -- build the sprite's local matrix
 * (translate +0x3C, rotate +0x4C, compose with D_8004FBB8), scale it by half
 * of 0x1000000 / (screen * 4096 / z), then draw the frame selected by the
 * battle view index (D_800C3EB0 + 0x8C84) through func_800B1F6C. */
void func_800BCEAC(u8* task) {
    u32 s = *(u32*)(task + 4);
    MATRIX m;
    VECTOR scale;
    s32 d;
    s32 half;
    u32 view;

    TransMatrix(&m, (VECTOR*)BG_PTR(s + 0x3C));
    RotMatrix((SVECTOR*)BG_PTR(s + 0x4C), &m);
    CompMatrix(&D_8004FBB8, &m, &m);
    d = (s32)((u32)ReadGeomScreen() << 12);
    if (m.t[2] != 0) {
        d = bg_div(d, m.t[2]);
    }
    d = bg_div(0x1000000, d);
    half = (s32)((u32)d + ((u32)d >> 31)) >> 1;
    func_80021B14(&scale, half, half, half);
    ScaleMatrixL(&m, &scale);
    SetRotMatrix(&m);
    SetTransMatrix(&m);
    view = BG_U32(0x800C3EB0u + 0x8000u + 0xC84u);
    func_800B1F6C(bg_arg(BG_U32(s + 0x64)), bg_arg(BG_U32(s + view * 4 + 0x5C)),
                  g_GfxCurOT, 0, 0, 0);
}

/* func_800BCFAC.s: timer callback -- follow the owner sprite's position
 * (+0x38 -> x at +2, y at +6 minus the +0x54 height minus 0x20, z at +0xA),
 * advance the phase counter, pulse the owner's colour (func_800BCAFC) and
 * spin +0x4E by 0x10. */
void func_800BCFAC(u8* task) {
    u32 s = *(u32*)(task + 4);
    u32 sprite = BG_U32(s + 0x38);
    u32 phase = BG_U32(s + 0x58);
    s32 height = BG_S32(s + 0x54);
    s32 y;
    s32 z;

    BG_S32(s + 0x3C) = BG_S16(sprite + 2);
    y = BG_S16(sprite + 6);
    phase++;
    BG_S32(s + 0x40) = y;
    z = BG_S16(sprite + 0xA);
    BG_U32(s + 0x58) = phase;
    BG_U32(s + 0x40) = (u32)y - (u32)height - 0x20;
    BG_S32(s + 0x44) = z;
    func_800BCAFC(BG_PTR(sprite), (s32)phase);
    BG_U16(s + 0x4E) += 0x10;
}

/* func_800BD024.s: on-free callback -- clear the owner's highlight flag,
 * refresh it, free the frame buffer (+0x5C) and the task pair. */
void func_800BD024(u8* task) {
    u32 s = *(u32*)(task + 4);
    u32 sprite = BG_U32(s + 0x38);

    BG_U8(sprite + 0x2B) |= 1;
    func_8001F6B0(bg_arg(sprite));
    func_80025180(bg_arg(BG_U32(s + 0x5C)));
    func_8001CE74(task);
    TimerWorkListRemoveTask(task);
    HeapFree(bg_arg(s));
}

/* func_800BD098.s: start the highlight task pair for the sprite owned by
 * `owner` (owner->+4): allocate 0x68 bytes, copy the owner's position data,
 * decode the D_8001C76C frame into a heap buffer twice its size (second half
 * a pristine copy) and prime it with one func_800BCFAC step.  func_800B168C
 * (record = base + i * 0x1C + 0xC) is inlined: it has two port definitions
 * with different parameter conventions. */
void func_800BD098(u8* owner) {
    u32 sprite = *(u32*)(owner + 4);
    u32 t;
    u32 size;
    u32 buf;
    u32 copy;

    t = (u32)(uintptr_t)BG_WORKLISTS_ADD_TASKS(0x68, owner, BG_CODE(0x800BCFACu),
                                          BG_CODE(0x800BCEACu),
                                          BG_CODE(0x800BD024u));
    BG_U32(t + 0x38) = sprite;
    BG_U32(t + 0x54) = BG_U16(sprite + 0x36);
    if (D_800591AC != 0) {
        D_80059464--;
    }
    BG_U32(t + 0x14) &= 0x7FFFFFFF;
    func_80021B04(BG_PTR(t + 0x4C), 0, 0, 0);
    size = func_800B16A4(BG_PTR(0x8001C76Cu + 0xC));
    buf = BG_ADDR(HeapAlloc(size * 2, 0));
    func_800B1720(BG_PTR(0x8001C76Cu + 0xC), BG_PTR(buf), 0, 1);
    copy = buf + size;
    memcpy(BG_PTR(copy), BG_PTR(buf), size);
    BG_U32(t + 0x5C) = buf;
    BG_U32(t + 0x60) = copy;
    BG_U32(t + 0x64) = 0x8001C76Cu + 0xC;
    BG_U8(sprite + 0x2B) &= 0xFE;
    func_8001F6B0(bg_arg(sprite));
    func_800BCFAC(BG_PTR(t));
}

/* func_800BD1FC.s: if the current actor's slot `slot` has a queued popup
 * (kind byte +0x150 != 0xFF, value halfword +0x138 != 0xFFFF) and a record,
 * publish its colour (+0x15C) and flags (+0x174) and spawn it, then mark the
 * value consumed. */
void func_800BD1FC(u32 slot) {
    u32 rec = BATTLE_CUR_RECORD();
    u32 b = rec + slot;
    u32 h = rec + slot * 2;
    u32 target;

    if (BG_U8(b + 0x150) == 0xFF) {
        return;
    }
    if (BG_U16(h + 0x138) == 0xFFFF) {
        return;
    }
    target = BG_U32(BATTLE_SLOT_RECORDS + slot * 4);
    if (target == 0) {
        return;
    }
    BG_U32(0x800C3D38) = BG_U16(h + 0x15C);
    BG_U32(0x800D3630) = BG_U8(b + 0x174);
    func_800BD3AC(BG_PTR(target), BG_U16(h + 0x138), BG_U8(b + 0x150));
    BG_U16(BATTLE_CUR_RECORD() + slot * 2 + 0x138) = 0xFFFF;
}

/* func_800BD2E4.s: spawn the queued popups of all 11 slots; once a slot of
 * kind 7 has been seen, the current actor's own record is skipped. */
void func_800BD2E4(void) {
    u32 seen7 = 0;
    u32 i;

    for (i = 0; i != 11; i++) {
        u32 target;

        if (BG_U8(BATTLE_CUR_RECORD() + i + 0x150) == 7) {
            seen7 = 1;
        }
        target = BG_U32(BATTLE_SLOT_RECORDS + i * 4);
        if (target != 0 &&
            (BG_U32(0x800C3E1C) != target || (seen7 & 0xFF) == 0)) {
            func_800BD1FC(i);
        }
    }
}

/* func_800BD3AC.s: spawn a damage/heal/status popup over `target`.  Any
 * popup already attached to it is retired (its callback), a 0x1B0-byte task
 * pair is linked at the head of the D_800C3750 chain, and `kind` (1..11,
 * jtbl_80070ADC) picks the glyph strings; kinds other than 4 append the
 * decimal digits of `value` (func_800BE6E8, five digits) as glyphs 0x72+n
 * of the D_800D2F5C font. */
void func_800BD3AC(u8* target, u32 value, u32 kind) {
    u32 tg = BG_ADDR(target);
    u8 digits[0x18];
    u32 t;
    u32 cur;
    s32 x = 0;
    u32 font;

    if (BG_U32(0x800C3610) == 0) {
        return;
    }
    func_800BDE58();
    cur = BG_U32(0x800C3750);
    BG_U8(0x800C4929) = 0;
    while (cur != 0) {
        if (BG_U32(cur + 0x78) == tg) {
            bg_jalr(BG_U32(cur + 0xC), cur, 0, 0, 0);
        }
        cur = BG_U32(cur + 0x38);
    }
    t = (u32)(uintptr_t)BG_WORKLISTS_ADD_TASKS(0x1B0, NULL, BG_CODE(0x800BDC78u),
                                          BG_CODE(0x800BDA1Cu),
                                          BG_CODE(0x800BD7A0u));
    BG_U32(t + 0x78) = tg;
    BG_U32(t + 0x88) = 8;
    BG_U32(t + 0x38) = BG_U32(0x800C3750);
    BG_U32(t + 0x48) = BG_U32(tg + 0);
    BG_U32(0x800C3750) = t;
    BG_U32(t + 0x4C) = BG_U32(tg + 4);
    BG_U32(t + 0x58) = 0x2000;
    BG_U32(t + 0x5C) = 0x2000;
    BG_U32(t + 0x60) = 0x2000;
    BG_U8(t + 0x83) = 0x2D;
    BG_U16(t + 0x40) = 0;
    BG_U16(t + 0x42) = 0;
    BG_U16(t + 0x44) = 0;
    BG_U8(t + 0x80) = 0x80;
    BG_U8(t + 0x81) = 0x80;
    BG_U8(t + 0x82) = 0x80;
    BG_U32(t + 0x50) = BG_U32(tg + 8);
    BG_U32(t + 0x8C) = 0;
    BG_U8(t + 0x7C) = (u8)((BG_U32(tg + 0xAC) >> 2) & 1);
    if (kind != 4) {
        func_800BE6E8((s32)value, digits, 5, 0, 0);
        x = BG_S16(0x800C3752 + (u32)digits[0] * 2);
    }
#define POPUP_GLYPH(glyph, ofsy)                                              \
    func_80026DCC(BG_PTR(BG_U32(0x800D2F5C)), (glyph),                        \
                  BG_PTR(t + BG_U32(t + 0x8C) * 0x18 + 0x90), (s16)x, (ofsy))
    switch (kind) {
    case 1:
        BG_U32(t + 0x88) = 0x40;
        BG_U32(t + 0x8C) = func_80026DCC(BG_PTR(BG_U32(0x800D2F5C)), 0x7C,
                                         BG_PTR(t + 0x90), -0x14, -0x10);
        BG_U8(t + 0x83) = (BG_U8(t + 0x83) & 0xFE) | 2;
        TimerWorkListSetTaskCallback(bg_arg(t), BG_CODE(0x800BDB08u));
        return;
    case 2:
        BG_U8(t + 0x80) = 0x80;
        BG_U8(t + 0x81) = 0;
        BG_U8(t + 0x82) = 0;
        BG_U8(t + 0x83) &= 0xFE;
        break;
    case 3:
    case 11: {
        s32 n = POPUP_GLYPH(0x7F, 0);
        BG_U32(t + 0x8C) += (u32)n;
        break;
    }
    case 5: {
        s32 n = POPUP_GLYPH(0x91, 0);
        BG_U8(t + 0x82) = 0x80;
        BG_U8(t + 0x80) = 0;
        BG_U8(t + 0x81) = 0;
        BG_U32(t + 0x8C) += (u32)n;
        BG_U8(t + 0x83) &= 0xFE;
        break;
    }
    case 6: {
        s32 n = POPUP_GLYPH(0x80, -0x10);
        x += 0x20;
        BG_U32(t + 0x8C) += (u32)n;
        break;
    }
    case 7: {
        s32 n = POPUP_GLYPH(0x80, -0x10);
        BG_U8(t + 0x80) = 0x80;
        BG_U8(t + 0x82) = 0x80;
        x += 0x20;
        BG_U8(t + 0x81) = 0;
        BG_U32(t + 0x8C) += (u32)n;
        BG_U8(t + 0x83) &= 0xFE;
        break;
    }
    case 8:
        BG_U8(t + 0x80) = 0;
        BG_U8(t + 0x81) = 0x80;
        BG_U8(t + 0x82) = 0;
        BG_U8(t + 0x83) &= 0xFE;
        break;
    default:
        break;
    }
    if (kind != 4 && digits[0] != 0) {
        u32 i;

        for (i = 0; i != digits[0]; i++) {
            s32 n = POPUP_GLYPH(digits[1 + i] + 0x72, -0x10);
            BG_U32(t + 0x8C) += (u32)n;
            x += 0xA;
        }
    }
#undef POPUP_GLYPH
}

/* func_800BD7A0.s: on-free callback -- unlink the popup from the
 * D_800C3750 chain (+0x38 links) and delete its task pair. */
void func_800BD7A0(u8* task) {
    u32 prev = 0;
    u32 cur = BG_U32(0x800C3750);

    while (cur != 0) {
        if (bg_ptr(cur) == (void*)task) {
            if (prev != 0) {
                BG_U32(prev + 0x38) = BG_U32(cur + 0x38);
            } else {
                BG_U32(0x800C3750) = BG_U32(cur + 0x38);
            }
            break;
        }
        prev = cur;
        cur = BG_U32(cur + 0x38);
    }
    WorkListsDeleteTasks(task);
}

/* func_800BD810.s: emit one textured sprite quad (POLY_FT4, code 9 words)
 * for glyph record `glyph` (x,y +0/+2, w,h +6/+7, u,v +4/+5, clut +0xA,
 * tpage +0xC) with RGB word `rgb`, projected through the current GTE
 * matrices, and link it into the current OT (retail 24-bit link). */
void func_800BD810(u8* glyph, u32 rgb) {
    u8* prim = (u8*)g_GfxCurWorkBuffer;
    long sxy[4];
    long p;
    long flag;
    s32 x0, y0, x1, y1;
    u32 u0, v0, u1, v1;
    u32* ot;

    if (!((uintptr_t)(prim + 0x28) < (uintptr_t)g_GfxCurWorkBufferEnd)) {
        return;
    }
    g_GfxCurWorkBuffer = prim + 0x28;
    prim[3] = 9;
    *(u32*)(prim + 4) = rgb;
    *(u16*)(prim + 0x16) = *(u16*)(glyph + 0xA);
    *(u16*)(prim + 0xE) = *(u16*)(glyph + 0xC);
    x0 = *(u16*)(glyph + 0);
    y0 = *(u16*)(glyph + 2);
    x1 = x0 + glyph[6];
    y1 = y0 + glyph[7];
    D_8004FB98[0].vx = (s16)x0;
    D_8004FB98[0].vy = (s16)y0;
    D_8004FB98[1].vy = (s16)y0;
    D_8004FB98[1].vx = (s16)x1;
    D_8004FB98[2].vx = (s16)x1;
    D_8004FB98[3].vx = (s16)x0;
    D_8004FB98[2].vy = (s16)y1;
    D_8004FB98[3].vy = (s16)y1;
    RotTransPers4(&D_8004FB98[0], &D_8004FB98[1], &D_8004FB98[2],
                  &D_8004FB98[3], &sxy[0], &sxy[1], &sxy[3], &sxy[2], &p,
                  &flag);
    *(u32*)(prim + 0x08) = (u32)sxy[0];
    *(u32*)(prim + 0x10) = (u32)sxy[1];
    *(u32*)(prim + 0x20) = (u32)sxy[3];
    *(u32*)(prim + 0x18) = (u32)sxy[2];
    u0 = glyph[4];
    v0 = glyph[5];
    u1 = u0 + glyph[6];
    v1 = v0 + glyph[7];
    prim[0xD] = (u8)v0;
    prim[0x15] = (u8)v0;
    prim[0x1D] = (u8)v1;
    prim[0x25] = (u8)v1;
    prim[0xC] = (u8)u0;
    prim[0x14] = (u8)u1;
    prim[0x1C] = (u8)u0;
    prim[0x24] = (u8)u1;
    ot = (u32*)g_GfxCurOT;
    *(u32*)prim = (*(u32*)prim & 0xFF000000u) | (*ot & 0x00FFFFFFu);
    *ot = (*ot & 0xFF000000u) | ((u32)(uintptr_t)prim & 0x00FFFFFFu);
}

/* func_800BD974.s: project `point` through D_800D30BC and return the
 * screen position relative to the geometry offset, doubled, plus the
 * projection distance: out[0] = 2 * (sx - ofx), out[1] = 2 * (sy - ofy),
 * out[2] = ReadGeomScreen(). */
void func_800BD974(SVECTOR* point, s32* out) {
    MATRIX* m = (MATRIX*)BG_PTR(0x800D30BCu);
    int sxy;
    long p;
    long flag;
    long ofx;
    long ofy;

    SetRotMatrix(m);
    SetTransMatrix(m);
    RotTransPers(point, &sxy, &p, &flag);
    ReadGeomOffset(&ofx, &ofy);
    out[0] = (s32)(((u32)(s16)sxy - (u32)ofx) << 1);
    out[1] = (s32)(((u32)(s16)((u32)sxy >> 16) - (u32)ofy) << 1);
    out[2] = (s32)ReadGeomScreen();
}

/* func_800BDA1C.s: work callback -- draw the popup's glyph quads.  Screen
 * distance goes to D_800C3760 + 0x1C, the anchor (+0x4A/+0x4E/+0x52) is
 * projected to a translation, and rotation +0x40 / scale +0x58 compose onto
 * D_800C3760 before each of the +0x8C glyphs (0x18 bytes from +0x90) is
 * drawn with colour +0x80. */
void func_800BDA1C(u8* task) {
    u32 s = *(u32*)(task + 4);
    MATRIX* base = (MATRIX*)BG_PTR(0x800C3760u);
    MATRIX m;
    SVECTOR anchor;
    s32 pos[4];
    u32 i;

    BG_S32(0x800C3760u + 0x1C) = (s32)ReadGeomScreen();
    anchor.vx = BG_S16(s + 0x4A);
    anchor.vy = BG_S16(s + 0x4E);
    anchor.vz = BG_S16(s + 0x52);
    func_800BD974(&anchor, pos);
    RotMatrix((SVECTOR*)BG_PTR(s + 0x40), &m);
    TransMatrix(&m, (VECTOR*)pos);
    CompMatrix(base, &m, &m);
    ScaleMatrix(&m, (VECTOR*)BG_PTR(s + 0x58));
    SetRotMatrix(&m);
    SetTransMatrix(&m);
    for (i = 0; BG_U32(s + 0x8C) != 0 && i != BG_U32(s + 0x8C);) {
        func_800BD810(BG_PTR(s + 0x90 + i * 0x18), BG_U32(s + 0x80));
        i++;
        if (i == BG_U32(s + 0x8C)) {
            break;
        }
    }
}

/* func_800BDB08.s: fade timer -- run the shared popup step, darken all three
 * colour bytes by 2 and fire the task's callback when +0x88 runs out. */
void func_800BDB08(u8* task) {
    s32 left;

    func_800BDF1C();
    task[0x80] -= 2;
    task[0x81] -= 2;
    task[0x82] -= 2;
    left = *(s32*)(task + 0x88) - 1;
    *(s32*)(task + 0x88) = left;
    if (left < 0) {
        bg_jalr(*(u32*)(task + 0xC), BG_ADDR(task), 0, 0, 0);
    }
}

/* func_800BDB74.s: fade timer -- step all three colour bytes toward zero by
 * 8 (func_80021AD8; each step reads the already-updated +0x80) and fire the
 * task's callback when +0x88 runs out or the colour reaches black. */
void func_800BDB74(void* taskp) {
    u8* task = (u8*)taskp;
    s32 left;

    func_800BDF1C();
    task[0x80] = (u8)func_80021AD8(task[0x80], -8);
    task[0x81] = (u8)func_80021AD8(task[0x80], -8);
    task[0x82] = (u8)func_80021AD8(task[0x80], -8);
    left = *(s32*)(task + 0x88) - 1;
    *(s32*)(task + 0x88) = left;
    if (left < 0 || (task[0x80] | task[0x81] | task[0x82]) == 0) {
        bg_jalr(*(u32*)(task + 0xC), BG_ADDR(task), 0, 0, 0);
    }
}
#endif
