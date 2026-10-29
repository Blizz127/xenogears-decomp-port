#include "common.h"


#ifndef XENO_PC_PORT
typedef struct {
    s16 x;
    s16 z;
} PairXZ;

extern s16 func_80023124(PairXZ a, PairXZ b);
extern void func_80021FE0(u8* obj, s16 angle);
extern void func_800223B0(u8* obj, s16 angle);

/* turn the object toward its +0xA0/+0xA4 target. */
void func_800B6518(u8* obj) {
    PairXZ pa;
    PairXZ pb;
    s32 ax, az;
    u32 bx, bz;
    s16 angle;

    ax = *(s16*)(obj + 2);
    pa.x = ax;
    az = *(s16*)(obj + 0xA);
    pa.z = az;
    bx = *(u16*)(obj + 0xA0);
    pb.x = bx;
    bz = *(u16*)(obj + 0xA4);
    pb.z = bz;
    angle = func_80023124(pb, pa);
    func_80021FE0(obj, angle);
    func_800223B0(obj, angle);
}

#include "psyq/libgte.h"
extern void func_80021B04(SVECTOR* v, s16 len, s16 a2, s16 a3);
#include "psyq/abs.h"

/* Steer the velocity toward the +0xA0 target: the 12-bit wrapped yaw/pitch
 * deltas are clamped to +-(p[0] * 4) (the port body below describes it in
 * full).  diff/step are s16: an s32 step lets jump1 hoist the else arm and
 * CSE then negates the step copy (negu a3,a3 instead of retail's a3,a1). */
void func_800B65B0(u8* obj, u8* p) {
    SVECTOR aim;
    SVECTOR cur;
    VECTOR d;
    VECTOR sq;
    VECTOR e;
    SVECTOR len;
    MATRIX m;
    VECTOR out;
    s16 flat;
    s32 speed;
    s32 limit;
    s16 diff;
    s16 step;

    d.vx = *(s16*)(obj + 0xA0) - *(s16*)(obj + 0x2);
    d.vy = *(s16*)(obj + 0xA2) - *(s16*)(obj + 0x6);
    d.vz = *(s16*)(obj + 0xA4) - *(s16*)(obj + 0xA);
    Square0(&d, &sq);
    flat = SquareRoot0(sq.vx + sq.vz);
    aim.vy = -ratan2(d.vz, d.vx);
    aim.vz = ratan2(d.vy, flat);
    aim.vx = 0;
    e.vx = *(s32*)(obj + 0xC) >> 7;
    e.vy = *(s32*)(obj + 0x10) >> 7;
    e.vz = *(s32*)(obj + 0x14) >> 7;
    Square0(&e, &sq);
    speed = SquareRoot0(sq.vy + sq.vx + sq.vz);
    flat = SquareRoot0(sq.vx + sq.vz);
    cur.vy = -ratan2(e.vz, e.vx);
    cur.vz = ratan2(e.vy, flat);
    cur.vx = 0;
    limit = p[0] * 4;
    diff = (s16)(aim.vy - cur.vy) << 20 >> 20;
    step = diff;
    if (abs(diff) > limit) {
        if (diff < 0) {
            step = -limit;
        } else {
            step = limit;
        }
    }
    cur.vy += step;
    diff = (s16)(aim.vz - cur.vz) << 20 >> 20;
    step = diff;
    if (abs(diff) > limit) {
        if (diff < 0) {
            step = -limit;
        } else {
            step = limit;
        }
    }
    cur.vz += step;
    func_80021B04(&len, speed, 0, 0);
    RotMatrix(&cur, &m);
    ApplyMatrix(&m, &len, &out);
    *(s32*)(obj + 0xC) = out.vx << 7;
    *(s32*)(obj + 0x10) = out.vy << 7;
    *(s32*)(obj + 0x14) = out.vz << 7;
}

/* Point the object's velocity (+0xC/+0x10/+0x14, << 7) at its +0xA0 target:
 * yaw/pitch from ratan2 over the integer-position delta (the yaw is also
 * stored at +0x32), magnitude (+0x18 << 9) >> 16 rotated by RotMatrix. */
void func_800B6808(u8* obj) {
    SVECTOR rot;
    VECTOR d;
    VECTOR sq;
    SVECTOR len;
    MATRIX m;
    VECTOR out;
    s32 flat;

    d.vx = *(s16*)(obj + 0xA0) - *(s16*)(obj + 0x2);
    d.vy = *(s16*)(obj + 0xA2) - *(s16*)(obj + 0x6);
    d.vz = *(s16*)(obj + 0xA4) - *(s16*)(obj + 0xA);
    Square0(&d, &sq);
    flat = SquareRoot0(sq.vx + sq.vz);
    rot.vy = -ratan2(d.vz, d.vx);
    rot.vz = ratan2(d.vy, flat);
    rot.vx = 0;
    *(s16*)(obj + 0x32) = rot.vy;
    func_80021B04(&len, (*(s32*)(obj + 0x18) << 9) >> 16, 0, 0);
    RotMatrix(&rot, &m);
    ApplyMatrix(&m, &len, &out);
    *(s32*)(obj + 0xC) = out.vx << 7;
    *(s32*)(obj + 0x10) = out.vy << 7;
    *(s32*)(obj + 0x14) = out.vz << 7;
}
#endif


/* The first two bytes of p are an offset added to the pointer before the six
 * bytes are read. In retail that is 32-bit guest arithmetic which the address
 * bus then masks to the 2 MiB RAM mirror; a host pointer has neither the wrap
 * nor the mask, so an out-of-range offset field walks off g_PsxRam. Rebase
 * through the guest address in port mode. */
#ifdef XENO_PC_PORT
#define XENO_ADVANCE_PTR(p, n) ((p) = (u8*)PSX_ADDR(PsxMemory_GuestAddr(p) + (n)))
#else
#define XENO_ADVANCE_PTR(p, n) ((p) += (n))
#endif
/* func_800B6930.s */
void func_800B6930(u32* dst, u8* p) {
    u32 n = (u32)((*(s8*)(p + 1) << 8) | p[0]);

    XENO_ADVANCE_PTR(p, n);
    dst[0] = (u32)((*(s8*)(p + 1) << 8) | p[0]) << 16;
    dst[1] = (u32)((*(s8*)(p + 3) << 8) | p[2]) << 16;
    dst[2] = (u32)((*(s8*)(p + 5) << 8) | p[4]) << 16;
}
/* func_800B6990.s */
void func_800B6990(u16* dst, u8* p) {
    s32 hi;
    u32 lo;
    u32 n = (u32)((*(s8*)(p + 1) << 8) | p[0]);

    XENO_ADVANCE_PTR(p, n);
    hi = *(s8*)(p + 1);
    lo = p[0];
    *(u16*)((u8*)dst + 0) = (u16)((hi << 8) | lo);
    hi = *(s8*)(p + 3);
    lo = p[2];
    *(u16*)((u8*)dst + 2) = (u16)((hi << 8) | lo);
    hi = *(s8*)(p + 5);
    lo = p[4];
    *(u16*)((u8*)dst + 4) = (u16)((hi << 8) | lo);
}

#ifdef XENO_PC_PORT
#include "battle_host_guest.h"
#include "psyq/libgte.h"
extern s32 func_80023124(s32 pointA, s32 pointB);
extern void func_80021FE0(void* pSpriteData, s16 angle);
extern void func_800223B0(void* pSpriteData, s16 angle);
extern void func_80021B04(void* arg0, s16 arg1, s16 arg2, s16 arg3);
extern VECTOR* Square0(VECTOR* v0, VECTOR* v1);

/* func_800B6518.s: turn the object toward its +0xA0/+0xA4 target: angle =
 * func_80023124(target x|z<<16, position x|z<<16), then apply it with
 * func_80021FE0 and func_800223B0. */
void func_800B6518(u8* obj) {
    u32 from = (u32)*(u16*)(obj + 0xA0) | ((u32)*(u16*)(obj + 0xA4) << 16);
    u32 to = (u32)(u16)*(s16*)(obj + 0x2) | ((u32)(u16)*(s16*)(obj + 0xA) << 16);
    s16 angle = (s16)func_80023124((s32)from, (s32)to);

    func_80021FE0(obj, angle);
    func_800223B0(obj, angle);
}

/* func_800B65B0.s: steer the object's velocity (+0xC/+0x10/+0x14, 1/128
 * units) toward its +0xA0 target.  The yaw/pitch differences (12-bit
 * wrapped) between the target direction and the current velocity direction
 * are clamped to +-(p[0] * 4), added to the velocity angles, and the
 * velocity is rebuilt at its old length by RotMatrix/ApplyMatrix. */
void func_800B65B0(u8* obj, u8* p) {
    VECTOR d;
    VECTOR e;
    VECTOR out;
    VECTOR sq;
    SVECTOR aim;
    SVECTOR cur;
    SVECTOR len;
    MATRIX m;
    s32 flat;
    s32 speed;
    s32 limit;
    s32 diff;
    s32 step;
    s32 mag;

    d.vx = (s32)((u32)*(s16*)(obj + 0xA0) - (u32)*(s16*)(obj + 0x2));
    d.vy = (s32)((u32)*(s16*)(obj + 0xA2) - (u32)*(s16*)(obj + 0x6));
    d.vz = (s32)((u32)*(s16*)(obj + 0xA4) - (u32)*(s16*)(obj + 0xA));
    Square0(&d, &sq);
    flat = SquareRoot0((int)((u32)sq.vx + (u32)sq.vz));
    aim.vy = (s16)(0u - (u32)(s32)ratan2(d.vz, d.vx));
    aim.vz = (s16)ratan2(d.vy, (s16)flat);
    aim.vx = 0;

    e.vx = *(s32*)(obj + 0xC) >> 7;
    e.vy = *(s32*)(obj + 0x10) >> 7;
    e.vz = *(s32*)(obj + 0x14) >> 7;
    Square0(&e, &sq);
    speed = SquareRoot0((int)((u32)sq.vy + (u32)sq.vx + (u32)sq.vz));
    flat = SquareRoot0((int)((u32)sq.vx + (u32)sq.vz));
    cur.vy = (s16)(0u - (u32)(s32)ratan2(e.vz, e.vx));
    cur.vz = (s16)ratan2(e.vy, (s16)flat);
    cur.vx = 0;

    limit = (s32)((u32)p[0] << 2);
    diff = (s32)(((u32)(u16)aim.vy - (u32)(u16)cur.vy) << 20) >> 20;
    step = diff;
    mag = diff < 0 ? -diff : diff;
    if (limit < mag) {
        step = diff < 0 ? -limit : limit;
    }
    cur.vy = (s16)((u32)(u16)cur.vy + (u32)step);
    diff = (s32)(((u32)(u16)aim.vz - (u32)(u16)cur.vz) << 20) >> 20;
    step = diff;
    mag = diff < 0 ? -diff : diff;
    if (limit < mag) {
        step = diff < 0 ? -limit : limit;
    }
    cur.vz = (s16)((u32)(u16)cur.vz + (u32)step);

    func_80021B04(&len, (s16)speed, 0, 0);
    RotMatrix(&cur, &m);
    ApplyMatrix(&m, &len, &out);
    *(u32*)(obj + 0xC) = (u32)out.vx << 7;
    *(u32*)(obj + 0x10) = (u32)out.vy << 7;
    *(u32*)(obj + 0x14) = (u32)out.vz << 7;
}

/* func_800B6808.s: point the object's velocity at its +0xA0 target with
 * magnitude (obj->0x18 << 9) >> 16; also stores the yaw in obj->0x32. */
void func_800B6808(u8* obj) {
    VECTOR d;
    VECTOR sq;
    VECTOR out;
    SVECTOR rot;
    SVECTOR len;
    MATRIX m;
    s32 flat;

    d.vx = (s32)((u32)*(s16*)(obj + 0xA0) - (u32)*(s16*)(obj + 0x2));
    d.vy = (s32)((u32)*(s16*)(obj + 0xA2) - (u32)*(s16*)(obj + 0x6));
    d.vz = (s32)((u32)*(s16*)(obj + 0xA4) - (u32)*(s16*)(obj + 0xA));
    Square0(&d, &sq);
    flat = SquareRoot0((int)((u32)sq.vx + (u32)sq.vz));
    rot.vy = (s16)(0u - (u32)(s32)ratan2(d.vz, d.vx));
    rot.vz = (s16)ratan2(d.vy, flat);
    rot.vx = 0;
    *(u16*)(obj + 0x32) = (u16)rot.vy;
    func_80021B04(&len, (s16)((s32)(*(u32*)(obj + 0x18) << 9) >> 16), 0, 0);
    RotMatrix(&rot, &m);
    ApplyMatrix(&m, &len, &out);
    *(u32*)(obj + 0xC) = (u32)out.vx << 7;
    *(u32*)(obj + 0x10) = (u32)out.vy << 7;
    *(u32*)(obj + 0x14) = (u32)out.vz << 7;
}
#endif
