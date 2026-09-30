#include "common.h"


#ifndef XENO_PC_PORT
INCLUDE_ASM("asm/battle/nonmatchings/mainc122", func_800BE1C4);
INCLUDE_ASM("asm/battle/nonmatchings/mainc122", func_800BE330);
INCLUDE_ASM("asm/battle/nonmatchings/mainc122", func_800BE538);
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

INCLUDE_ASM("asm/battle/nonmatchings/mainc122", func_800BE790);
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
