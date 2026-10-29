#include "common.h"


#ifndef XENO_PC_PORT
extern u8* D_800C3610[];
extern s32 D_800C360C[];
extern u16 D_800C3608[];
extern s16 D_800C3614[];
extern u16 D_800C3DF0[];
extern u8 D_800C3623[];
extern void func_80021BF8(u8* p, void* cb);
extern void func_800245D8(u32 a0, u32 a1);
extern void func_80079E18(u8 index);
extern void func_80079E4C(u8 index);
extern void func_800B8048(u32 v);
extern void func_800BC404(u32 v);
extern void func_800BF600(u32 id, u8* actor);
extern void func_800BF730(u8* actor);
/* The current command step is the 0x48-byte row D_800C360C of D_800C3EB0;
 * STEP_U16 reads its u16 at a D_800C3EB0-relative offset.  Retail adds the
 * offset in the load (lhu 0x172(row)), which needs the row as an array
 * element: flat `D_800C3EB0 + D_800C360C * 0x48 + 0x172` arithmetic folds
 * the 0x172 into the symbol's %lo. */
typedef union BattleStepRow {
    u8 b[0x48];
    u16 h[0x24];
} BattleStepRow;
extern BattleStepRow D_800C3EB0__steps[] asm("D_800C3EB0");
#define STEP_U16(off) (D_800C3EB0__steps[D_800C360C[0]].h[(off) / 2])

/* func_800B9284.s, byte-exact: one command-step action for step kinds
 * 0xF3..0xFA (from func_800B9C78 and func_800B9508).  Most kinds use the
 * step's u16 parameter at +0x172: 0xFA/0xF8/0xF7 set the command task's
 * +0x48 flag, call func_80021BF8(actor, 0) and pass the parameter to
 * func_80079E18 / func_80079E4C / D_800C3614; 0xF5 and 0xF4 call
 * func_800B8048(actor) and split it into a 6-bit field for D_800C3DF0 and a
 * 9-bit id for func_800BF600 (0xF5 then also calls func_800BF730 and
 * func_800245D8(actor, 0x13) and sets D_800C3623); 0xF6 passes the step's
 * +0x14E u16 to func_800BC404; 0xF3 ORs the parameter into D_800C3608; 0xF9
 * does nothing.
 * The switch table is this object's .rodata (jtbl_80070A28, placed by
 * config/battle.yaml).  Codegen: the cases are in retail's block order, the
 * globals are arrays (the CDK compiler's split %hi/%lo; as scalars the
 * stores become $at macros) and the parameter is an s32 (retail's sra; a
 * u16 shift is shortened to srl). */
void func_800B9284(u8* actor, u32 kind) {
    s32 param;

    switch (kind) {
    case 0xFA:
        D_800C3610[0][0x48] = 1;
        func_80021BF8(actor, 0);
        func_80079E18(STEP_U16(0x172));
        break;
    case 0xF8:
        D_800C3610[0][0x48] = 1;
        func_80021BF8(actor, 0);
        func_80079E4C(STEP_U16(0x172));
        break;
    case 0xF7:
        D_800C3610[0][0x48] = 1;
        func_80021BF8(actor, 0);
        D_800C3614[0] = STEP_U16(0x172);
        break;
    case 0xF5:
        func_800B8048((u32)actor);
        param = STEP_U16(0x172);
        D_800C3DF0[0] = (param >> 9) & 0x3F;
        actor[0xAF] = 0x11;
        func_800BF600(param & 0x1FF, actor);
        func_800BF730(actor);
        func_800245D8((u32)actor, 0x13);
        D_800C3623[0] = 1;
        break;
    case 0xF4:
        func_800B8048((u32)actor);
        param = STEP_U16(0x172);
        D_800C3DF0[0] = (param >> 9) & 0x3F;
        actor[0xAF] = 0x11;
        func_800BF600(param & 0x1FF, actor);
        break;
    case 0xF6:
        func_800BC404(STEP_U16(0x14E));
        break;
    case 0xF3:
        D_800C3610[0][0x48] = 1;
        D_800C3608[0] |= STEP_U16(0x172);
        break;
    }
}
typedef struct { u16 a; u16 b; } B95Pair;
typedef struct {
    u8 unk0[0x2C];
    s32 unk2C;
    s32 unk30;
} B95Heap;

typedef struct B95Spr {
    u8 unk0[2];
    s16 unk2;
    u8 unk4[6];
    s16 unkA;
    u8 unkC[0x14];
    B95Heap* unk20;
    s32* unk24;
    u8 unk28[0x14];
    u32 unk3C;
    u8 unk40[0x10];
    s32 unk50;
    u8 unk54[0x20];
    struct B95Spr* unk74;
    u8 unk78[4];
    u8* unk7C;
    u8 unk80[0x20];
    u16 unkA0;
    u16 unkA2;
    u16 unkA4;
    u8 unkA6[2];
    u32 unkA8 : 28;
    u32 mode : 2;
    u32 slot : 4 __attribute__((packed));
    u32 unkAC : 22;
    u32 unkAF : 8;
    s8 unkB0;
} B95Spr;

extern u16 D_800C3626[];
extern s16 D_800C3630[];
extern s16 D_800C3648[];
extern u8 D_800D3350[];
extern void* D_800C3618[];
extern u32 func_800C0314(void);
extern u8* func_800BEFF4(s32 slot);
extern void func_800BF3E8(u8* rec);
void func_800B9B30();
extern void func_800BF0B4(u32 value);
extern void func_800BF0C4(u8* rec);
extern void func_80021FB8(u8* rec, u16 arg);
extern void HeapFree(s32 p);
extern s32 HeapGetNextBlockHeader(s32 p);
extern s32 HeapAlloc(s32 size, s32 mode);
extern s32 func_8001EE68(s32 a0);
extern void func_800B8354(void);
extern s32 func_800BF354(void);
extern void SpriteSetSpecialAnimFile(u8* rec, void* file);
extern void func_800BC460(u32 mask);

#define B95_SLOT(r) ((r)->slot)

#define R ((B95Spr*)rec)
extern u8 D_800C3EB0__r[][28] asm("D_800C3EB0");
typedef struct {
    u8 unk0[6];
    u8 unk6;
    u8 unk7;
    u8 unk8[6];
    u16 unkE;
    u16 unk10;
    u8 unk12[10];
} B95Row;
#define ROWS ((B95Row*)D_800C3EB0__steps)
#define T24 (*(s32*)(*(u8**)(void*)D_800C3610 + 0x24))

/* func_800B9508.s, byte-exact: run the actor's next battle step code `kind`.
 * - F3-F8/FA go to func_800B9284.
 * - F9 and FB-FF are the control codes (animation 5, the wait callback,
 *   spawn via func_800BEFF4, step advance, reset).
 * - Any other code starts an animation (< 0x10), or effect id kind - 0x10
 *   offset through D_800C3630 and bounded by D_800C3648 (func_800BF600).
 * CONSTRUCTS (counted separately):
 * - Allocation and order: set-twice `anim` (gives retail's s0/s1) and `id`;
 *   per-case `other`; FE's A0/A4/A2 store order; F3-FA's own `++; return`;
 *   if/else stores for +0xAF.
 * - Scalar and base views: three (void*) scalar-view casts of the
 *   D_800C3610 / D_800C360C / D_800C3618 arrays (retail orders them as
 *   scalars); D_800C3EB0 base views (the __r alias, ROWS, a u8* base block).
 * - Data shapes: the packed 4-bit slot bitfield; the B95Pair copy; the
 *   B95Heap chained store; the unprototyped func_800B9B30 call. */
void func_800B9508(u8* rec) {
    s32 kind;

    if (D_800C3614[0] != 0) {
        D_800C3614[0]--;
        return;
    }
    kind = D_800C3EB0__steps[D_800C360C[0]].b[0x17F];
    switch (kind) {
    case 0xF3:
    case 0xF4:
    case 0xF5:
    case 0xF6:
    case 0xF7:
    case 0xF8:
    case 0xFA:
        func_800B9284(rec, kind);
        D_800C360C[0]++;
        return;
    case 0xF9:
        func_800245D8((u32)rec, 5);
        func_800BC404(0);
        return;
    case 0xFF:
        func_80021BF8(rec, 0);
        D_800C3610[0][0x48] = 1;
        if (func_800C0314() == 0) {
            if (R->mode == 0 && D_800C3EB0__r[B95_SLOT(R)][7] == 0) {
                func_800245D8((u32)rec, R->unkB0);
            }
            return;
        }
    case 0xFE:
        D_800C3610[0][0x48] = 0;
        rec = func_800BEFF4(*(s32*)(D_800C3610[0] + 0x20));
        func_800BF3E8(rec);
        func_80021BF8(rec, func_800B9B30);
        func_800BF0B4(5);
        if (R->unk2 == ROWS[T24].unkE && R->unkA == ROWS[T24].unk10) {
            func_800B9B30(rec);
            return;
        }
        R->unkA0 = ROWS[T24].unkE;
        R->unkA4 = ROWS[T24].unk10;
        R->unkA2 = 0;
        func_800245D8((u32)rec, 4);
        return;
    case 0xFD:
        D_800C3610[0][0x48] = 1;
        if (STEP_U16(0x172) == 0) {
            D_800C3610[0][0x48] = 0;
            func_800BF0C4(rec);
            func_80021BF8(rec, func_800B9B30);
        }
        D_800C360C[0]++;
        return;
    case 0xFC:
        D_800C3610[0][0x48] = 1;
        func_80021BF8(rec, 0);
        func_80021FB8(rec, STEP_U16(0x172));
        D_800C360C[0]++;
        return;
    case 0xFB: {
        B95Spr* other;

        D_800C3610[0][0x48] = 1;
        func_80021BF8(rec, 0);
        {
            u8* base = (u8*)D_800C3EB0__steps;
            u8* q = base + STEP_U16(0x172) * 4 + 0x8C8C;

            other = *(B95Spr**)q;
        }
        R->unk24 = other->unk24;
        *(B95Pair*)(R->unk7C + 0xE) = *(B95Pair*)(other->unk7C + 0xE);
        R->unk3C |= 0x40000000;
        HeapFree(R->unk20->unk2C);
        {
            s32 p = HeapAlloc(HeapGetNextBlockHeader(other->unk20->unk2C), 0);

            R->unk20->unk30 = R->unk20->unk2C = p;
        }
        (*(s32*)(void*)D_800C360C)++;
        return;
    }
    default: {
        B95Spr* other = R->unk74;

        D_800C3626[0] = 0;
        func_800B8048((u32)rec);
        func_80021BF8(rec, func_800B9B30);
        *(s32*)(R->unk7C + 8) = D_800C3EB0__steps[D_800C360C[0]].b[B95_SLOT(other) + 0x150];
        if (func_8001EE68(*R->unk24) == 0) {
            if (kind >= 0x10) {
                s32 id = kind - 0x10;

                id += D_800C3630[ROWS[B95_SLOT(R)].unk6];
                if (id >= D_800C3648[ROWS[B95_SLOT(R)].unk6]) {
                    R->unkAF = 0x11;
                } else {
                    R->unkAF = 0x1C;
                }
                func_800BF600(id, rec);
                func_800BF730(rec);
            } else {
                s32 anim = kind;

                if (D_800D3350[0] == 0 && D_800C3618[0] != 0) {
                    func_800B8354();
                    R->unk50 = func_800BF354();
                    SpriteSetSpecialAnimFile(rec, *(void**)(void*)D_800C3618);
                }
                if (*(s32*)(D_800C3610[0] + 0x20) == B95_SLOT(R)) {
                    anim = ~anim;
                    func_800245D8((u32)rec, anim);
                } else {
                    func_800245D8((u32)rec, anim);
                }
            }
        } else {
            func_800245D8((u32)rec, kind);
            func_800BC460((1 << B95_SLOT(R)) | (1 << B95_SLOT(R->unk74)));
        }
        D_800C3610[0][0x48] = 0;
        break;
    }
    }
    D_800C360C[0]++;
}

#undef T24
#undef ROWS
#undef R
#undef B95_SLOT

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


/* func_800B9B30.s */
#ifndef XENO_PC_PORT
extern u8* D_800C3610[];
#endif
void func_800B9B30(void) {
#ifdef XENO_PC_PORT
    /* main125.c/main68.c read D_800C3610 as a u8*, so the guest-RAM alias is
     * that pointer and the retail array's [0] element is the alias itself.
     * Indexing it a second time would subscript a u8. */
    D_800C3610[0x48] = 1;
    D_800C3610[0x49] = D_800C3610[0x1C];
#else
    D_800C3610[0][0x48] = 1;
    D_800C3610[0][0x49] = D_800C3610[0][0x1C];
#endif
}
