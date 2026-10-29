#include "common.h"


#ifndef XENO_PC_PORT
typedef struct {
    u8 pad[0x44];
    s16 cur;
    s16 target;
    s16 a;
    s16 b;
    s16 c;
    s16 d;
} Obj;

extern void func_800A6444(s32 a0, s32 a1, s32 a2, s32 a3, s32 a4, s32 a5);

/* task->0x04 object: when cur (+0x44) != target (+0x46), cur = target and
 * func_800A6444(0, +0x48, 0x20 - target, +0x4A, +0x4C, +0x4E). */
void func_800B3C74(Obj** task) {
    Obj* w = task[1];

    if (w->cur != w->target) {
        w->cur = w->target;
        func_800A6444(0, w->a, 0x20 - w->target, w->b, w->c, w->d);
    }
}

extern u32 WorkListsAddTasks(u32 a0, u32 a1, void* a2, void* a3, void* a4);
extern void* HeapAlloc(u32 size, u32 flags);
extern u32 HeapFree(void* p);
extern u8* D_800C3560__a[] asm("D_800C3560");
extern void func_800B3B94(u8* t);
extern void func_800B3C2C(void);
extern void func_800A5EB4(void);

/* func_800B3CD4.s: show the singleton task D_800C3560 (0x50 bytes; timer
 * func_800B3B94, work func_800B3C74, free func_800B3C2C): the first call
 * creates it, runs func_800A5EB4 on a 0x1000-byte HeapAlloc stack ($sp switch
 * as func_800B626C, saved at buf+0xC00) and zeroes +0x40/+0x44/+0x46; later
 * calls copy +0x46 into +0x40.  Then it stores the parameters (+0x42 a0,
 * +0x38/+0x3C a1 * 2, +0x48 a2, +0x4A..+0x4E a3..a5) and runs
 * func_800B3B94 once.  a3 is u16 like the stack parameters: as a 32-bit
 * parameter it lands in a different saved register (9 off).  main87
 * declares it (u32, u32, u32, s32, s32, s32). */
void func_800B3CD4(u32 a0, u32 a1, u32 a2, u16 a3, u16 a4, u16 a5) {
    u8* t;
    u8* stack;

    if (D_800C3560__a[0] == NULL) {
        t = (u8*)WorkListsAddTasks(0x50, 0, func_800B3B94, func_800B3C74, func_800B3C2C);
        D_800C3560__a[0] = t;
        stack = HeapAlloc(0x1000, 1);
        __asm__ volatile(
            "\taddu  $t0, %0, $zero\n"
            "\tsw    $sp, 0($t0)\n"
            "\taddiu $t0, $t0, -4\n"
            "\taddu  $sp, $t0, $zero\n"
            :
            : "r"(stack + 0xC00)
            : "t0", "memory");
        func_800A5EB4();
        __asm__ volatile(
            "\taddiu $sp, $sp, 4\n"
            "\tlw    $sp, 0($sp)\n"
            :
            :
            : "memory");
        HeapFree(stack);
        *(u16*)(t + 0x40) = 0;
        *(u16*)(t + 0x44) = 0;
        *(u16*)(t + 0x46) = 0;
    } else {
        t = D_800C3560__a[0];
        *(u16*)(t + 0x40) = *(u16*)(t + 0x46);
    }
    *(u16*)(t + 0x42) = a0;
    *(s32*)(t + 0x38) = a1 * 2;
    *(s32*)(t + 0x3C) = a1 * 2;
    *(u16*)(t + 0x4A) = a3;
    *(u16*)(t + 0x4C) = a4;
    *(u16*)(t + 0x4E) = a5;
    *(u16*)(t + 0x48) = a2;
    func_800B3B94(t);
}
#include "psyq/libgpu.h"
extern void* HeapAlloc(u32 size, u32 flags);
extern u32 HeapFree(void* p);
extern RECT D_800C3C9C__a[] asm("D_800C3C9C");
extern s16 D_800C3668[];

/* func_800B3E04.s: on a 0x1000-byte HeapAlloc stack (the caller's $sp saved
 * at buf+0xF00, $sp set to buf+0xEFC with GCC inline asm, as func_800B626C),
 * copy three 0x40 x 0x100
 * VRAM columns (x 0x280, 0x240, 0x200; y 0x100) through the D_800C3C9C RECT
 * to the positions in D_800C3668 with MoveImage, then HeapFree.  The RECT
 * goes through an array view so its address stays in a register. */
void func_800B3E04(void) {
    u8* stack = HeapAlloc(0x1000, 0);

    __asm__ volatile(
        "\taddu  $t0, %0, $zero\n"
        "\tsw    $sp, 0($t0)\n"
        "\taddiu $t0, $t0, -4\n"
        "\taddu  $sp, $t0, $zero\n"
        :
        : "r"(stack + 0xF00)
        : "t0", "memory");
    D_800C3C9C__a[0].x = 0x280;
    D_800C3C9C__a[0].y = 0x100;
    D_800C3C9C__a[0].w = 0x40;
    D_800C3C9C__a[0].h = 0x100;
    MoveImage(D_800C3C9C__a, D_800C3668[0], D_800C3668[1]);
    D_800C3C9C__a[0].x = 0x240;
    D_800C3C9C__a[0].y = 0x100;
    D_800C3C9C__a[0].w = 0x40;
    D_800C3C9C__a[0].h = 0x100;
    MoveImage(D_800C3C9C__a, D_800C3668[2], D_800C3668[3]);
    D_800C3C9C__a[0].x = 0x200;
    D_800C3C9C__a[0].y = 0x100;
    D_800C3C9C__a[0].w = 0x40;
    D_800C3C9C__a[0].h = 0x100;
    MoveImage(D_800C3C9C__a, D_800C3668[4], D_800C3668[5]);
    __asm__ volatile(
        "\taddiu $sp, $sp, 4\n"
        "\tlw    $sp, 0($sp)\n"
        :
        :
        : "memory");
    HeapFree(stack);
}
#include "psyq/libgte.h"

/* 4-bit battle slot of an actor record: +0xAC bits 0-1 above +0xA8 bits 30-31.
 * The statement expression keeps retail's order (+0xA8 half first) and stops
 * a narrow destination from shortening the +0xAC load. */
#define B3F04_ACTOR_SLOT(a) \
    ({ u32 hi_ = *(u32*)((a) + 0xA8) >> 30; ((*(u32*)((a) + 0xAC) & 3) << 2) | hi_; })

typedef struct {
    u8 unk0[6];
    u8 unk6;
    u8 unk7[7];
    u16 unkE;
    u8 unk10[0xC];
} B3F04SlotRow;
extern B3F04SlotRow D_800C3EB0__slot[] asm("D_800C3EB0");

typedef struct {
    u8 unk0[4];
    SVECTOR eye;
    SVECTOR at;
} B3F04Camera;

typedef struct {
    u8 pad[0x1C];
    s32 unk1C;
} B3F04Actor;

extern u8 D_800C3EB0[];
/* small-data scalar: retail loads it with the assembler's lui/lw macro */
extern int D_80010000;
extern u8 D_80059198[];
extern u8* D_8005919C[];
extern s16 D_80059454[];
extern u8 D_800C3564[];
extern u8* D_800C3568[];
extern u8 D_800C35D4[];
extern u8 D_800C3621[];
extern u8 D_800C3622[];
extern u16 D_800C3626[];
extern u8 D_800C3664[];
extern u8 D_800C3688[];
extern u8 D_800C372C[];
extern u8 D_800C3CAC[];
extern u8 D_800C3CB0[];
extern u8* D_800C3E1C[];
extern s32 D_800C3E54[];
extern u8 D_800C492A[];
extern s16 D_800D2D4C[];
extern B3F04Camera D_800D309C;
extern s16 D_800D3354[];
extern s16 D_800D335C[];
extern u16 D_800D3634[];
extern u8 D_800D3638[];
extern s16 D_800D36BC[];

extern void func_8001CE74(u8* task);
extern u8* func_8001D0A4(u8* task, void* callback);
extern u8* func_8001D164(void* callback);
extern void func_8001D4E8(u8* actor);
extern u8* func_8001FBA4(u8* actor, u8* args);
extern void func_80021FE0(u8* actor, s32 v);
extern void func_800222BC(u8* actor, s32 anim);
extern void func_800245D8(u32 a0, u32 a1);
extern void func_80039DB8(s32 id);
extern void func_80039E60(s32 id);
extern void func_80039EC4(s32 id, s32 ch);
extern void func_8003A14C(s32 id);
extern void func_8003A89C(s32 a0, s32 a1, s32 a2);
extern void func_800AA788(u32 v);
extern void func_800B3E04(void);
void func_800B4EDC(u8* task);
extern void func_800B5588(u8* task);
extern void func_800B572C(u8* actor, u8* slot);
extern void func_800B5B3C(u8* task);
extern void func_800B5C18(u8* actor, u8* args);
extern void func_800B5DC4(u8* actor);
extern void func_800B5FBC(u8* actor, u8* args);
extern void func_800B61B0(u8* actor, u8* args);
extern void func_800B61F8(u8* actor, u8* args);
extern void func_800B626C(u8* actor, u8* args);
extern void func_800B62C8(u8* actor, u8* args);
extern void func_800B639C(u8* actor, u8* args);
extern void func_800B63F0(u8* actor, u8* args);
extern void func_800B6438(u8* actor, u8* args);
extern void func_800B6464(u8* actor, u8* args);
extern void func_800B64D4(u8* actor, u8* args);
extern void func_800B6518(u8* actor, u8* args);
extern void func_800B65B0(u8* actor, u8* args);
extern void func_800B6808(u8* actor, u8* args);
extern void func_800B6930(u8* actor, u8* args);
extern void func_800B6990(u8* p, u8* args);
extern void func_800B69E4(u8* p, u8* args);
extern void func_800B6A50(u8* actor, u8* args);
extern void func_800B6A7C(u8* actor, u8* args);
extern void func_800B6B98(u8* actor, u8* args);
extern void func_800B6BFC(u8* actor, u8* args);
extern void func_800B6C44(u8* actor, u8* args);
extern void func_800B6C98(u8* actor, u8* args);
extern void func_800B6CEC(u8* actor, u8* args);
extern void func_800B6DC0(u8* actor, u8* args);
extern void func_800B6E84(u8* actor, u8* args);
extern void func_800BA768(u8* actor);
extern void func_800BA8F4(u8* actor);
extern void func_800BC404(u32 mask);
extern void func_800BD1FC(u32 slot);
extern void func_800BF730(u8* actor);
extern void func_800BF8CC(u8* actor);
extern void func_801FC6FC(u8* actor, u8* args);
extern void func_801FC7B0(u8* actor, u8* args);
extern void func_801FC898(void);

/* Battle actor script command: run command `cmd` (1..0x6B) on the actor
 * record with the command's argument bytes `args`.  Covers animation picks
 * (0x68/0x69), +0x3C/+0x40/+0xAC/+0xB0 flag bits, velocity set/add from
 * args[0] scaled by +0x82 (0x21/0x22/0x28/0x29/0x2A/0x2B), packed-id cues
 * (args[0] | bank id << 16, the bank from the actor's +0x50 record or
 * D_8005919C; 0x4D/0x4E/0x4F/0x52), battle-wide flags, the waits that rewind
 * the script pointer (+0x64 -= 2) until the camera settles (0x67) or until
 * D_800D36BC is non-zero, taking one count (0x50), the jump toward the +0x74
 * target (0x61) and slot-mask broadcasts to func_800BC404
 * (0x06/0x07/0x41/0x55/0x65).  Values outside 1..0x6B do nothing. */
void func_800B3F04(u8* actor, s32 cmd, u8* args) {
    s32 unused[10]; /* 40 bytes of unused stack, as retail */
    VECTOR a;
    VECTOR b;
    u8* tgt;
    u8* task;
    s32 d;
    s32 m;
    s32 slot;
    u32 hi;
    u8 k;

    switch (cmd) {
    case 0x6B:
        func_800B3E04();
        break;
    case 0x6A:
        D_800C492A[0] = 1;
        D_800C35D4[0] = 0;
        func_8003A89C(D_800C3E54[0], 0, 0x78);
        break;
    case 0x69:
        if ((*(u32*)(actor + 0xB0) >> 10) & 1) {
            func_800222BC(actor, *(s32*)(actor + 0x4C));
            *(u32*)(actor + 0xB0) |= 0x400;
        } else {
            func_800222BC(actor, *(s32*)(actor + 0x48));
            *(u32*)(actor + 0xB0) &= ~0x400;
        }
        break;
    case 0x68:
        if (*(s8*)(actor + 0xAF) < 0) {
            func_800222BC(actor, *(s32*)(actor + 0x4C));
            *(u32*)(actor + 0xB0) |= 0x400;
        } else {
            func_800222BC(actor, *(s32*)(actor + 0x48));
            *(u32*)(actor + 0xB0) &= ~0x400;
        }
        break;
    case 0x66:
        D_800C3688[0] = 1;
        break;
    case 0x64:
        *(u32*)(actor + 0xB0) |= 0x200;
    case 0x63:
        if (D_80059454[0] > 0x200) {
            *(s16*)(actor + 0x3A) = D_80059454[0];
            *(s32*)(actor + 0x40) = (*(s32*)(actor + 0x40) & ~0x1F00) | 0x300;
            *(s32*)(actor + 0x3C) |= 0x10000000;
        }
        break;
    case 0x62:
        func_800BD1FC(B3F04_ACTOR_SLOT(actor));
        break;
    case 0x61:
        *(s32*)(actor + 4) = (*(s16*)(actor + 0x84) - 1) << 16;
        tgt = *(u8**)(actor + 0x74);
        d = (s8)args[0] * *(s16*)(actor + 0x2C) / 4096;
        if ((*(s32*)(tgt + 0) >> 16) == D_800C3EB0__slot[B3F04_ACTOR_SLOT(tgt)].unkE) {
            if (!((*(u32*)(actor + 0xAC) >> 2) & 1)) {
                d = -d;
            }
        } else if ((*(s32*)(tgt + 0) >> 16) < D_800C3EB0__slot[B3F04_ACTOR_SLOT(tgt)].unkE) {
            d = -d;
        }
        *(s16*)(actor + 0xA0) = (*(s32*)(tgt + 0) >> 16) + d;
        *(s16*)(actor + 0xA4) = *(s32*)(tgt + 8) >> 16;
        *(s16*)(actor + 0xA2) = 0;
        func_800BA768(actor);
        break;
    case 0x5F:
        D_800C3564[0] = 1;
        break;
    case 0x60:
        D_800C3564[0] = B3F04_ACTOR_SLOT(actor) + 2;
        break;
    case 0x59:
        D_800D3638[0] = 0;
        break;
    case 0x5A:
        D_800D3638[0] = 1;
        break;
    case 0x57:
        func_800AA788(1);
        break;
    case 0x58:
        func_800AA788(0);
        break;
    case 0x56:
        if (D_800C3622[0] != 0) {
            hi = *(u32*)(actor + 0xA8) >> 30;
            slot = ((*(u32*)(actor + 0xAC) & 3) << 2) | hi;
            k = D_800C3EB0__slot[slot].unk6;
            if (k == 9) {
                k = 2;
            }
            if (k == 10) {
                k = 6;
            }
            if (k == 8) {
                k = 15;
            }
            if (!((D_800C3626[0] >> slot) & 1)) {
                D_800C3626[0] |= 1 << slot;
                func_80039E60((k + 0x52) | (*(u16*)(D_8005919C[0] + 0x14) << 16));
            }
        }
        break;
    case 0x54:
        func_801FC898();
        break;
    case 0x4F:
        if (*(u8**)(actor + 0x50) != NULL) {
            func_80039DB8(args[0] | (*(u16*)(*(u8**)(actor + 0x50) + 0x14) << 16));
        }
        break;
    case 0x52:
        if (*(u8**)(actor + 0x50) != NULL) {
            func_80039EC4(args[0] | (*(u16*)(*(u8**)(actor + 0x50) + 0x14) << 16), args[1]);
        }
        break;
    case 0x4D:
        if (*(u8**)(actor + 0x50) != NULL) {
            func_8003A14C(args[0] | (*(u16*)(*(u8**)(actor + 0x50) + 0x14) << 16));
        }
        break;
    case 0x4E:
        if (D_8005919C[0] != NULL) {
            func_8003A14C(args[0] | (*(u16*)(D_8005919C[0] + 0x14) << 16));
        }
        break;
    case 0x4C:
        *(s32*)(actor + 0x40) |= 2;
        break;
    case 0x48:
        *(s32*)(actor + 0x40) |= 1;
        break;
    case 0x67:
        *(u16*)(actor + 0x9E) += 1;
        a.vx = D_800D309C.eye.vx - D_800D3354[0];
        a.vy = D_800D309C.eye.vy - D_800D3354[1];
        a.vz = D_800D309C.eye.vz - D_800D3354[2];
        Square0(&a, &a);
        b.vx = D_800D309C.at.vx - D_800D335C[0];
        b.vy = D_800D309C.at.vy - D_800D335C[1];
        b.vz = D_800D309C.at.vz - D_800D335C[2];
        Square0(&b, &b);
        if (SquareRoot0(a.vx + a.vy + a.vz) < 4 && SquareRoot0(b.vx + b.vy + b.vz) < 4) {
            break;
        }
        *(s32*)(actor + 0x64) -= 2;
        break;
    case 0x50:
        *(u16*)(actor + 0x9E) += 1;
        if (D_800D36BC[0] != 0) {
            D_800D36BC[0]--;
        } else {
            *(s32*)(actor + 0x64) -= 2;
        }
        break;
    case 0x46:
        func_800B61F8(actor, args);
        D_800D2D4C[0] = 0;
        break;
    case 0x51:
        func_800B626C(actor, args);
        break;
    case 0x47:
        func_800B62C8(actor, args);
        break;
    case 0x44:
        if (D_80010000 != -1) {
            D_800C3568[0] = actor;
        }
        break;
    case 0x45:
        if (D_80010000 != -1) {
            D_800C3568[0] = NULL;
        }
        break;
    case 0x43:
        *(s32*)(actor + 0x3C) |= 0x80000000;
        break;
    case 0x3E:
        func_800BF8CC(actor);
        break;
    case 0x3D:
        func_800B4EDC(actor);
        break;
    case 0x3A:
        func_800B639C(actor, args);
        break;
    case 0x42:
        *(u32*)(actor + 0xAC) &= ~0x20;
        break;
    case 0x38:
        func_800B63F0(actor, args);
        break;
    case 0x37:
        D_800C3621[0] = 1;
        break;
    case 0x34:
        /* field store + plain deref: lets the D_80059198 load schedule ahead
         * of the first +0x1C store, as retail */
        ((B3F04Actor*)actor)->unk1C = (((s8)args[0] << 1) * *(s16*)(actor + 0x82) / 4096) << 5;
        ((B3F04Actor*)actor)->unk1C *= (*(s32*)D_80059198 + 1) * (*(s32*)D_80059198 + 1);
        break;
    case 0x33:
        if (*(s32*)(actor + 0x48) != 0) {
            func_800245D8((u32)actor, args[0]);
        }
        break;
    case 0x31:
        SetGeomOffset(0xA0, 0x70);
        break;
    case 0x32:
        SetGeomOffset(0xA0, 0xA4);
        break;
    case 0x30:
        if (*(u8**)(actor + 0x20) != NULL && (*(s32*)(actor + 0x3C) & 3) == 1) {
            if (*(u8**)(*(u8**)(actor + 0x20) + 0x34) != NULL) {
                (*(u8**)(actor + 0x20))[0x3D] = (*(u8**)(*(u8**)(actor + 0x20) + 0x34))[args[0] * 8 + 1];
                (*(u8**)(actor + 0x20))[0x3C] = (*(u8**)(*(u8**)(actor + 0x20) + 0x34))[args[0] * 8];
            }
        }
        break;
    case 0x2F:
        func_800BF730(actor);
        break;
    case 0x2C:
        func_800B6438(actor, args);
        break;
    case 0x27:
        func_800B6464(actor, args);
        break;
    case 0x26:
        func_800B64D4(actor, args);
        break;
    case 0x28:
        *(s32*)(actor + 0xC) = (((s8)args[0] << 4) * *(s16*)(actor + 0x82) / 4096) << 8;
        break;
    case 0x29:
        *(s32*)(actor + 0xC) += (((s8)args[0] << 4) * *(s16*)(actor + 0x82) / 4096) << 8;
        break;
    case 0x21:
        *(s32*)(actor + 0x14) = (((s8)args[0] << 4) * *(s16*)(actor + 0x82) / 4096) << 12;
        break;
    case 0x22:
        *(s32*)(actor + 0x14) += (((s8)args[0] << 4) * *(s16*)(actor + 0x82) / 4096) << 8;
        break;
    case 0x2A:
        *(s32*)(actor + 0x10) = (((s8)args[0] << 4) * *(s16*)(actor + 0x82) / 4096) << 12;
        break;
    case 0x2B:
        *(s32*)(actor + 0x10) += (((s8)args[0] << 4) * *(s16*)(actor + 0x82) / 4096) << 8;
        break;
    case 0x1F:
        *(s32*)(actor + 0x3C) &= ~0x04000000;
        func_800BA8F4(actor);
        break;
    case 0x3F:
        *(s32*)(actor + 0x3C) |= 0x04000000;
        break;
    case 0x3B:
        *(s32*)(actor + 0x3C) |= 0x20000000;
        break;
    case 0x3C:
        *(s32*)(actor + 0x3C) &= ~0x20000000;
        break;
    case 0x35:
        *(s32*)(actor + 0x3C) |= 0x08000000;
        break;
    case 0x36:
        *(s32*)(actor + 0x3C) &= ~0x08000000;
        break;
    case 0x23:
        func_800B6518(actor, args);
        break;
    case 0x40:
        func_800B65B0(actor, args);
        break;
    case 0x1C:
        func_800B6808(actor, args);
        break;
    case 0x1B:
        func_800B6A50(actor, args);
        break;
    case 0x1A:
        func_8001CE74(*(u8**)(actor + 0x6C));
        break;
    case 0x2D:
        func_800B6930(actor, args);
        break;
    case 0x2E:
        func_800B6990(actor + 0xA0, args);
        break;
    case 0x5B:
        func_800B6990(*(u8**)(actor + 0x20) + 0x44, args);
        break;
    case 0x5C:
        func_800B6990(*(u8**)(actor + 0x20) + 0x4C, args);
        break;
    case 0x5D:
        func_800B69E4(*(u8**)(actor + 0x20) + 0x44, args);
        break;
    case 0x5E:
        func_800B69E4(*(u8**)(actor + 0x20) + 0x4C, args);
        break;
    case 0x1D:
        func_800B6A7C(actor, args);
        break;
    case 0x19:
        func_800B6B98(actor, args);
        break;
    case 0x18:
        func_800B6BFC(actor, args);
        break;
    case 0x17:
        m = (s8)args[0] << 2;
        *(s32*)(actor + 0xC) = *(s32*)(actor + 0xC) * m / 256;
        *(s32*)(actor + 0x10) = *(s32*)(actor + 0x10) * m / 256;
        *(s32*)(actor + 0x14) = *(s32*)(actor + 0x14) * m / 256;
        break;
    case 0x16:
        *(s32*)(actor + 0x3C) |= 0x02000000;
        break;
    case 0x15:
        *(s32*)(actor + 0x3C) |= 0x01000000;
        break;
    case 0x11:
        D_800C3664[0] = 1;
        break;
    case 0x12:
        D_800C372C[0] = 1;
        D_800C3CAC[0] = D_800C3EB0[0xB88];
        D_800C3CB0[0] = D_800C3EB0[0xB89];
        D_800C3CB0[1] = D_800C3EB0[0xB8A];
        D_800C3CB0[2] = D_800C3EB0[0xB8B];
        D_800C3EB0[0x4BF8] = 1;
        D_800C3EB0[0xB88] = 1;
        D_800C3EB0[0xB89] = 0;
        D_800C3EB0[0x4BF9] = 0;
        D_800C3EB0[0xB8A] = 0;
        D_800C3EB0[0x4BFA] = 0;
        D_800C3EB0[0xB8B] = 0;
        D_800C3EB0[0x4BFB] = 0;
        break;
    case 0x13:
        D_800C3664[0] = 0;
        break;
    case 0x14:
        D_800C372C[0] = 0;
        D_800C3EB0[0x4BF8] = D_800C3CAC[0];
        D_800C3EB0[0xB88] = D_800C3CAC[0];
        D_800C3EB0[0xB89] = D_800C3CB0[0];
        D_800C3EB0[0x4BF9] = D_800C3CB0[0];
        D_800C3EB0[0xB8A] = D_800C3CB0[1];
        D_800C3EB0[0x4BFA] = D_800C3CB0[1];
        D_800C3EB0[0xB8B] = D_800C3CB0[2];
        D_800C3EB0[0x4BFB] = D_800C3CB0[2];
        break;
    case 0x10:
        *(s32*)(actor + 0) = D_800D335C[0] << 16;
        *(s32*)(actor + 4) = D_800D335C[1] << 16;
        *(s32*)(actor + 8) = D_800D335C[2] << 16;
        break;
    case 0x53:
        *(u32*)(actor + 0xAC) &= ~8;
        *(u32*)(actor + 0xAC) &= ~4;
        *(s32*)(actor + 0x3C) &= ~8;
        *(s32*)(actor + 0x3C) &= ~0x10;
        break;
    case 0xF:
        *(u32*)(actor + 0xAC) &= ~8;
        *(u32*)(actor + 0xAC) &= ~4;
        *(s32*)(actor + 0x3C) &= ~8;
        *(s32*)(actor + 0x3C) &= ~0x10;
        func_80021FE0(actor, 0);
        break;
    case 0xE:
        func_80021FE0(actor, (s8)args[0] * 16);
        break;
    case 0xD:
        func_800B6E84(actor, args);
        break;
    case 0xA:
        func_800B6C44(actor, args);
        break;
    case 0xB:
        func_800B6C98(actor, args);
        break;
    case 0x49:
        if (*(u8**)(actor + 0x20) != NULL) {
            *(u16*)(*(u8**)(actor + 0x20) + 0) += (s8)args[0];
            *(s32*)(actor + 0x3C) |= 0x10000000;
        }
        break;
    case 0x4A:
        if (*(u8**)(actor + 0x20) != NULL) {
            *(u16*)(*(u8**)(actor + 0x20) + 2) += (s8)args[0];
            *(s32*)(actor + 0x3C) |= 0x10000000;
        }
        break;
    case 0x4B:
        if (*(u8**)(actor + 0x20) != NULL) {
            *(u16*)(*(u8**)(actor + 0x20) + 4) += (s8)args[0];
            *(s32*)(actor + 0x3C) |= 0x10000000;
        }
        break;
    case 0xC:
        func_800B6CEC(actor, args);
        break;
    case 0x9:
        *(u16*)(actor + 0x32) += 0x800;
        *(s32*)(actor + 0xC) = -*(s32*)(actor + 0xC);
        *(s32*)(actor + 0x10) = -*(s32*)(actor + 0x10);
        *(s32*)(actor + 0x14) = -*(s32*)(actor + 0x14);
        break;
    case 0x1:
        func_8001D4E8(actor);
        func_800B572C(actor, func_8001FBA4(actor, args));
        break;
    case 0x24:
        func_801FC7B0(actor, args);
        break;
    case 0x25:
        func_801FC6FC(actor, args);
        break;
    case 0x2:
        task = func_8001D0A4(*(u8**)(actor + 0x6C), func_800B5588);
        if (task != NULL) {
            (*(void (**)(u8*))(task + 0xC))(task);
        }
        break;
    case 0x3:
        func_800B5C18(actor, args);
        break;
    case 0x4:
        while ((task = func_8001D164(func_800B5B3C)) != NULL) {
            (*(void (**)(u8*))(task + 0xC))(task);
        }
        break;
    case 0x1E:
        func_800B61B0(actor, args);
        break;
    case 0x20:
        func_800B5DC4(actor);
        break;
    case 0x5:
        func_800B5FBC(actor, args);
        break;
    case 0x6:
        func_800BC404(1 << B3F04_ACTOR_SLOT(D_800C3E1C[0]));
        break;
    case 0x55:
        func_800BC404((1 << B3F04_ACTOR_SLOT(D_800C3E1C[0])) | D_800D3634[0]);
        break;
    case 0x7:
        func_800BC404(1 << B3F04_ACTOR_SLOT(*(u8**)(actor + 0x74)));
        break;
    case 0x65:
        func_800BC404(D_800D3634[0]);
        break;
    case 0x41:
        {
            u8* q = D_800C3E1C[0];

            func_800BC404((1 << B3F04_ACTOR_SLOT(*(u8**)(actor + 0x74))) | (1 << B3F04_ACTOR_SLOT(q)));
        }
        break;
    case 0x8:
        *(s32*)(actor + 0x40) |= 0x80000;
        func_800B6DC0(actor, args);
        break;
    }
}

#include "psyq/libgpu.h"

/* copy an 8x8 texture cell (u = +4 >> 2, v = +5, page from tpage +0xA) to
 * VRAM (0x3F0, 0x1F0) and its 16-entry CLUT (+0xC) to (0x3F0, 0x1EE). */
void func_800B4EDC(u8* task) {
    RECT rect;
    u8* spr = *(u8**)(*(u8**)(task + 0x20) + 0x30);
    u16 tpage = *(u16*)(spr + 0xA);
    u16 clut;

    rect.x = (spr[4] >> 2) + ((tpage & 0xF) << 6);
    rect.y = spr[5] + ((tpage << 4) & 0x100);
    rect.w = 8;
    rect.h = 8;
    MoveImage(&rect, 0x3F0, 0x1F0);
    clut = *(u16*)(spr + 0xC);
    rect.w = 16;
    rect.h = 1;
    rect.x = clut & 0x3F;
    rect.y = (clut >> 6) & 0x1FF;
    MoveImage(&rect, 0x3F0, 0x1EE);
}

/* func_800B4F88.s: build `m` for pose entry `idx` of the actor's parent
 * (actor +0x20; 8-byte entries from parent +0x34: s8 x, s8 y, then the
 * rotation halves at +2/+4/+6): RotMatrix, translation = the parent's
 * +0x20/+0x24/+0x28 plus x and y scaled by the actor's s16 +0x2C (/ 4096;
 * x negated when +0xAC bit 2 is set), then SetMulMatrix with the parent's
 * matrix at +0xC.  The entry address is integer arithmetic so the index is
 * the first addu operand, as in retail. */
void func_800B4F88(u8* actor, s32 idx, MATRIX* m) {
    s8* e;
    s32 x;
    s32 y;
    SVECTOR rot;

    if (*(u8**)(actor + 0x20) != NULL) {
        e = (s8*)(idx * 8 + (u32)*(u8**)(*(u8**)(actor + 0x20) + 0x34));
        y = e[1];
        x = e[0];
        if ((*(u32*)(actor + 0xAC) >> 2) & 1) {
            x = -x;
        }
        y = y * *(s16*)(actor + 0x2C) / 4096;
        x = x * *(s16*)(actor + 0x2C) / 4096;
        rot.vx = *(u16*)(e + 2);
        rot.vy = *(u16*)(*(u8**)(*(u8**)(actor + 0x20) + 0x34) + idx * 8 + 4);
        rot.vz = *(u16*)(*(u8**)(*(u8**)(actor + 0x20) + 0x34) + idx * 8 + 6);
        RotMatrix(&rot, m);
        m->t[0] = *(s32*)(*(u8**)(actor + 0x20) + 0x20) + x;
        m->t[1] = *(s32*)(*(u8**)(actor + 0x20) + 0x24) + y;
        m->t[2] = *(s32*)(*(u8**)(actor + 0x20) + 0x28);
        SetMulMatrix(m, (MATRIX*)(*(u8**)(actor + 0x20) + 0xC));
    }
}
/* func_800B50D4.s, byte-exact: when the actor's +0x3C state (low 2 bits)
 * is 1 and its +0x20 record has a +0x34 point table, write five scaled
 * points to dst: for i = 0..4 take the 8-byte table entry D_800C356C[i]
 * (s8 x, y), negate x when the actor's +0xAC bit 2 is set, scale both by
 * the actor's +0x2C halfword / 8192 (signed division), and store
 * (x, y, 0) as s16 at dst + i * 8.
 * Codegen:
 * - CONSTRUCT: the entry pointer passes through an identity inline helper
 *   (F50D4_Id, found by decomp-permuter's inline_fn).  That gives retail's
 *   allocation: x in a0, with the actor parameter copied out to t1 (and
 *   dst to t0).  The plain pointer, a copy through a local, a chained
 *   assignment, or helpers that compute the address keep the actor in a0,
 *   one insn short.
 * - i is incremented in the body before the three stores, where retail
 *   schedules it; in the for header it lands after them.
 * - The walking v gives retail's two dst pointer IVs (v and v + 4). */
extern u8 D_800C356C[];

static inline s8* F50D4_Id(s8* q) {
    return q;
}

void func_800B50D4(u8* actor, u8* dst) {
    s16* v = (s16*)dst;
    s32 i;
    s8* p;
    s32 x;
    s32 y;

    if ((*(u32*)(actor + 0x3C) & 3) != 1) {
        return;
    }
    if (*(u8**)(actor + 0x20) == NULL) {
        return;
    }
    if (*(u8**)(*(u8**)(actor + 0x20) + 0x34) == NULL) {
        return;
    }
    i = 0;
    while (i != 5) {
        p = F50D4_Id((s8*)(*(u8**)(*(u8**)(actor + 0x20) + 0x34) + D_800C356C[i] * 8));
        x = p[0];
        y = p[1];
        if ((*(u32*)(actor + 0xAC) >> 2) & 1) {
            x = -x;
        }
        x = x * *(s16*)(actor + 0x2C) / 8192;
        y = y * *(s16*)(actor + 0x2C) / 8192;
        i++;
        v[0] = x;
        v[1] = y;
        v[2] = 0;
        v += 4;
    }
}
extern u8 g_GfxCurWorkBufferEnd__b[] asm("g_GfxCurWorkBufferEnd");
extern u8 g_GfxCurWorkBuffer__b[] asm("g_GfxCurWorkBuffer");
extern u8 g_GfxCurOT__b[] asm("g_GfxCurOT");
extern u8 D_800C3D4C__b[] asm("D_800C3D4C");
extern u8 D_800D2FCC__b[] asm("D_800D2FCC");
extern u8 D_800D2FD8__b[] asm("D_800D2FD8");
extern u8 D_800D3334__b[] asm("D_800D3334");
extern u8 D_800C3E9C__b[] asm("D_800C3E9C");
extern u8 D_800C3CA4__b[] asm("D_800C3CA4");
extern u8 D_800C3CA6__b[] asm("D_800C3CA4+2");
extern u8 D_800C3CA8__b[] asm("D_800C3CA8");
extern u8 D_800C3CAA__b[] asm("D_800C3CA8+2");

/* func_800B51B0.s, byte-exact: func_800C08CC's draw callback for one
 * trail segment from `from` to `to` (skipped when they are equal).  A
 * LINE_F2 (colour 0xF0; semi-transparent, abr = D_800C3D4C - 1, when
 * D_800C3D4C is non-zero) is projected with RotTransPers3; z = D_800D3334
 * + 2 when the D_800D2FD8 byte for segment D_800D2FCC / 8 is set, else
 * - 2.  For 0 < z < 0x1000, a POLY_FT4 ribbon of half-width D_800C3E9C
 * runs perpendicular to the line (ratan2 + 0x400, rsin/rcos): its first
 * edge is the line start for segment 0, or the previous segment's end
 * (D_800C3CA4..D_800C3CAA).  The end edge is saved there, the prim goes
 * to OT entry z, and the segment counter goes up.  Work-buffer prims are
 * taken only while they fit.
 * Codegen: every scalar global goes through a cast of an unsized u8
 * asm-label array (`*(s16*)D_800C3D4C__b`).  That keeps it out of -G8
 * small data (retail gives each its own %hi/%lo pair, the +2 halves
 * through their own `sym+2` labels) and makes it a plain non-struct MEM,
 * so CSE and sched treat it as a scalar.  This body matched with plain
 * externs only at -G0, which func_800B3F04 cannot share; the casts give
 * the same code under mainc83's -G8, so no .text split is needed.  Check
 * then take for each work-buffer prim; dx before dy for ratan2; z from an
 * if/else; two SetSemiTrans calls (retail cross-jumps them).  Matches only
 * under the CDK cc1 (mainc83's BattleCdk). */
void func_800B51B0(VECTOR* from, VECTOR* to) {
    SVECTOR v0;
    SVECTOR v1;
    long sxy2;
    long flag;
    LINE_F2* line;
    POLY_FT4* poly;
    s32 abr;
    s32 z;
    s32 w;
    s32 ang;
    s32 dx;
    s32 dy;

    if (from->vx == to->vx && from->vy == to->vy && from->vz == to->vz) {
        return;
    }
    v0.vx = from->vx;
    v0.vy = from->vy;
    v0.vz = from->vz;
    v1.vx = to->vx;
    v1.vy = to->vy;
    v1.vz = to->vz;
    if ((u8*)(*(void**)g_GfxCurWorkBuffer__b) + sizeof(LINE_F2) < (u8*)(*(void**)g_GfxCurWorkBufferEnd__b)) {
        line = (*(void**)g_GfxCurWorkBuffer__b);
        (*(void**)g_GfxCurWorkBuffer__b) = line + 1;
        SetLineF2(line);
        abr = (*(s16*)D_800C3D4C__b);
        if (abr != 0) {
            abr--;
            SetSemiTrans(line, 1);
        } else {
            SetSemiTrans(line, 0);
        }
        setRGB0(line, 0xF0, 0xF0, 0xF0);
        if ((*(u8**)D_800D2FD8__b)[(*(s32*)D_800D2FCC__b) / 8] != 0) {
            z = 2;
        } else {
            z = -2;
        }
        z += (*(s16*)D_800D3334__b);
        RotTransPers3(&v0, &v1, &v0, (long*)&line->x0, (long*)&line->x1, &sxy2, &sxy2, &flag);
        if (z > 0 && z < 0x1000) {
            w = (*(s16*)D_800C3E9C__b);
            dx = line->x1 - line->x0;
            dy = line->y1 - line->y0;
            ang = ratan2(dy, dx) + 0x400;
            dx = rsin(ang) * w / 8192;
            dy = rcos(ang) * w / 8192;
            if ((u8*)(*(void**)g_GfxCurWorkBuffer__b) + sizeof(POLY_FT4) < (u8*)(*(void**)g_GfxCurWorkBufferEnd__b)) {
                poly = (*(void**)g_GfxCurWorkBuffer__b);
                (*(void**)g_GfxCurWorkBuffer__b) = poly + 1;
                SetPolyFT4(poly);
                SetShadeTex(poly, 1);
                poly->u0 = ((*(s32*)D_800D2FCC__b) & 7) - 0x40;
                poly->v0 = 0xF0;
                poly->u1 = ((*(s32*)D_800D2FCC__b) & 7) - 0x3F;
                poly->v1 = 0xF0;
                poly->u2 = ((*(s32*)D_800D2FCC__b) & 7) - 0x40;
                poly->v2 = 0xF7;
                poly->u3 = ((*(s32*)D_800D2FCC__b) & 7) - 0x3F;
                poly->v3 = 0xF7;
                poly->tpage = GetTPage(0, abr, 0x3F0, 0x1F0);
                poly->clut = GetClut(0x3F0, 0x1EE);
                setRGB0(poly, 0xF0, 0xF0, 0xF0);
                if ((*(s32*)D_800D2FCC__b) == 0) {
                    poly->x0 = line->x0 - dx;
                    poly->y0 = line->y0 - dy;
                    poly->x1 = line->x0 + dx;
                    poly->y1 = line->y0 + dy;
                } else {
                    poly->x0 = (*(s16*)D_800C3CA4__b);
                    poly->y0 = (*(s16*)D_800C3CA6__b);
                    poly->x1 = (*(s16*)D_800C3CA8__b);
                    poly->y1 = (*(s16*)D_800C3CAA__b);
                }
                (*(s16*)D_800C3CA4__b) = poly->x2 = line->x1 - dx;
                (*(s16*)D_800C3CA6__b) = poly->y2 = line->y1 - dy;
                (*(s16*)D_800C3CA8__b) = poly->x3 = line->x1 + dx;
                (*(s16*)D_800C3CAA__b) = poly->y3 = line->y1 + dy;
                AddPrim((*(u_long**)g_GfxCurOT__b) + z, poly);
                (*(s32*)D_800D2FCC__b)++;
            }
        }
    }
}
typedef struct {
    u8 pad0[0x2E];
    u16 f2E;
    u8 pad30[4];
    u16 f34;
    u8 pad36[0x72];
    u32 fA8;
    u8 padAC[3];
    s8 fAF;
} T5588Actor;

/* The timer task's data (task + 4). */
typedef struct {
    u8 pad0[0xC];
    void (*fC)(u8*);
    u8 pad10[0x28];
    T5588Actor* a;
    s32 f3C;
    s32 f40;
    u16 f44;
    u8 pad46[2];
    u16 f48;
    u8 pad4A[2];
    s32 f4C;
} T5588Task;

extern void func_800B50D4(u8* actor, u8* dst);

/* func_800B5588.s, byte-exact: timer callback of the trail task (data s =
 * task + 4, actor a = s +0x38).  Publish s +0x4C / +0x48 / +0x44 and the
 * actor's +0x2E to D_800D2FD8 / D_800C3E9C / D_800C3D4C / D_800D3334; when
 * the actor's +0x34 differs from s +0x3C, take it and re-pose the five
 * segment vectors at s +0x50 (func_800B50D4).  Then ease the four s16
 * vec3s at s +0x80..+0x98 halfway towards s +0x58..+0x70, copy s +0x50 to
 * s +0x78, and call s +0xC(s) unless s +0x40 equals the actor's (s8) +0xAF
 * and its +0xA8 bits 28-29 are 2 or 3.
 * Codegen: the globals are stored through the cast views above
 * func_800B51B0 (non-struct, not -G8 small data) and the task fields are
 * read through a struct view (in-struct), so the actor loads pass the
 * global stores as in retail; this used to need -G0.  `v` holds +0x4C and
 * later the +0xA8 bits; the last test reads s->a again instead of
 * reloading `a`.  Matches only under the CDK cc1 (mainc83's BattleCdk). */
void func_800B5588(u8* task) {
    u8* s = *(u8**)(task + 4);
    T5588Actor* a;
    u8* p;
    s32 i;
    s32 v;

    v = ((T5588Task*)s)->f4C;
    *(s32*)D_800D2FD8__b = v;
    a = ((T5588Task*)s)->a;
    *(u16*)D_800C3E9C__b = ((T5588Task*)s)->f48;
    *(u16*)D_800C3D4C__b = ((T5588Task*)s)->f44;
    *(u16*)D_800D3334__b = a->f2E;
    if (a->f34 != ((T5588Task*)s)->f3C) {
        ((T5588Task*)s)->f3C = a->f34;
        func_800B50D4((u8*)a, s + 0x50);
    }
    for (i = 1; i != 5; i++) {
        p = s + i * 8;
        *(s16*)(p + 0x78) += (*(s16*)(p + 0x50) - *(s16*)(p + 0x78)) / 2;
        *(s16*)(p + 0x7A) += (*(s16*)(p + 0x52) - *(s16*)(p + 0x7A)) / 2;
        *(s16*)(p + 0x7C) += (*(s16*)(p + 0x54) - *(s16*)(p + 0x7C)) / 2;
    }
    *(s16*)(s + 0x78) = *(s16*)(s + 0x50);
    *(s16*)(s + 0x7A) = *(s16*)(s + 0x52);
    *(s16*)(s + 0x7C) = *(s16*)(s + 0x54);
    if (((T5588Task*)s)->f40 == ((T5588Task*)s)->a->fAF) {
        v = (((T5588Task*)s)->a->fA8 >> 28) & 3;
        if (v != 0) {
            if (v != 1) {
                return;
            }
        }
    }
    ((T5588Task*)s)->fC(s);
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
#ifdef XENO_PC_PORT
/* Retail defines it above as (VECTOR* from, VECTOR* to). */
extern void func_800B51B0(void);
#endif
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


/* func_800B56E4.s */
void func_800B56E4(u8* p) {
    u8* q = (u8*)(u32)*(u32*)(p + 4);

    func_8001E148(*(u32*)(q + 0x38));
    func_800C08CC(5, q + 0x78, func_800B51B0);
}
