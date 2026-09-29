/*
 * World-map scheduler callback 0x80087734 (slot 15 Table-B cb1).
 *
 * Leaf: wrapping (slot+0x50 + slot+0x54) & 0xFFF back into +0x50, then
 * two scratch SVECTORs and two RotMatrixYXZ calls into C620+0x410 /
 * C620+0x4B8. Every path returns scheduler state 1.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_callback_87734.h"

#define WM_87734_POOL_PTR    0x8009BE24u
#define WM_87734_CONTEXT_PTR 0x8009C620u
#define WM_87734_SCRATCH     0x1F800000u
#define WM_87734_SLOT_HEAD0  0x50u
#define WM_87734_SLOT_HEAD1  0x54u
#define WM_87734_VY0         0x40Au
#define WM_87734_VY1         0x4B2u
#ifndef WM_87734_MATRIX0
#define WM_87734_MATRIX0     0x410u
#endif
#ifndef WM_87734_MATRIX1
#define WM_87734_MATRIX1     0x4B8u
#endif
#ifndef WM_87734_HEAD_MASK
#define WM_87734_HEAD_MASK   0xFFFu
#endif

#if defined(WM_87734_MUTANT_WRONG_AND_MASK)
#undef WM_87734_HEAD_MASK
#define WM_87734_HEAD_MASK   0xFFu
#endif
#if defined(WM_87734_MUTANT_ROT_DEST_SCRATCH)
#undef WM_87734_MATRIX0
#undef WM_87734_MATRIX1
#define WM_87734_MATRIX0     0xA0u
#define WM_87734_MATRIX1     0xA8u
#endif

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} Wm87734Svector;

typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} Wm87734Matrix;

extern Wm87734Matrix *RotMatrixYXZ(Wm87734Svector *r, Wm87734Matrix *m);

static u16 wm_87734_load_u16(u32 pc, u32 address) __attribute__((noinline));
static u32 wm_87734_load_u32(u32 pc, u32 address) __attribute__((noinline));
static void wm_87734_store_u16(u32 pc, u32 address, u16 value)
    __attribute__((noinline));
static void wm_87734_store_u32(u32 pc, u32 address, u32 value)
    __attribute__((noinline));
static void wm_87734_rotmatrixyxz(u32 pc, u32 r_addr, u32 m_addr)
    __attribute__((noinline));

static u16 wm_87734_load_u16(u32 pc, u32 address)
{
    u16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_87734_TEST_TRACE)
    wm_87734_test_trace(pc, WM_87734_TRACE_LHU, address, 2u, (u32)value);
#else
    (void)pc;
#endif
    return value;
}

static u32 wm_87734_load_u32(u32 pc, u32 address)
{
    u32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_87734_TEST_TRACE)
    wm_87734_test_trace(pc, WM_87734_TRACE_LW, address, 4u, value);
#else
    (void)pc;
#endif
    return value;
}

static void wm_87734_store_u16(u32 pc, u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_87734_TEST_TRACE)
    wm_87734_test_trace(pc, WM_87734_TRACE_SH, address, 2u, (u32)value);
#else
    (void)pc;
#endif
}

static void wm_87734_store_u32(u32 pc, u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_87734_TEST_TRACE)
    wm_87734_test_trace(pc, WM_87734_TRACE_SW, address, 4u, value);
#else
    (void)pc;
#endif
}

static void wm_87734_rotmatrixyxz(u32 pc, u32 r_addr, u32 m_addr)
{
#if defined(WM_87734_TEST_TRACE)
    wm_87734_test_trace(pc, WM_87734_TRACE_CALL, r_addr, 8u, m_addr);
    wm_87734_test_rotmatrixyxz(pc, r_addr, m_addr);
#else
    (void)pc;
    (void)RotMatrixYXZ((Wm87734Svector *)PSX_ADDR(r_addr),
                       (Wm87734Matrix *)PSX_ADDR(m_addr));
#endif
}

s32 wm_80087734(s32 slot_index)
{
    u32 pool;
    u32 slot;
    u32 context;
    u32 head0;
    u32 head1;
    u16 vy;
    u16 vz;
    u32 dest0;
    u32 dest1;

    pool = wm_87734_load_u32(0x8008773Cu, WM_87734_POOL_PTR);
    slot = pool + ((u32)slot_index << 7);
    head0 = wm_87734_load_u32(0x80087750u, slot + WM_87734_SLOT_HEAD0);
    head1 = wm_87734_load_u32(0x80087754u, slot + WM_87734_SLOT_HEAD1);
    context = wm_87734_load_u32(0x8008775Cu, WM_87734_CONTEXT_PTR);
    head0 += head1;
#if defined(WM_87734_MUTANT_VZ_UNMASKED)
    vz = (u16)head0;
#endif
    head0 &= WM_87734_HEAD_MASK;
    wm_87734_store_u32(0x80087768u, slot + WM_87734_SLOT_HEAD0, head0);

    wm_87734_store_u16(0x80087770u, WM_87734_SCRATCH + 0xA8u, 0u);
    wm_87734_store_u16(0x80087778u, WM_87734_SCRATCH + 0xA0u, 0u);
    vy = wm_87734_load_u16(0x8008777Cu, context + WM_87734_VY0);
    wm_87734_store_u16(0x80087788u, WM_87734_SCRATCH + 0xA2u, vy);
    vy = wm_87734_load_u16(0x8008778Cu, context + WM_87734_VY1);
    wm_87734_store_u16(0x80087798u, WM_87734_SCRATCH + 0xAAu, vy);
#if !defined(WM_87734_MUTANT_VZ_UNMASKED)
    vz = wm_87734_load_u16(0x8008779Cu, slot + WM_87734_SLOT_HEAD0);
#endif
    wm_87734_store_u16(0x800877A4u, WM_87734_SCRATCH + 0xACu, vz);
    wm_87734_store_u16(0x800877ACu, WM_87734_SCRATCH + 0xA4u, vz);

#if defined(WM_87734_MUTANT_ROT_DEST_SCRATCH)
    dest0 = WM_87734_SCRATCH + WM_87734_MATRIX0;
    dest1 = WM_87734_SCRATCH + WM_87734_MATRIX1;
#else
    dest0 = context + WM_87734_MATRIX0;
    dest1 = context + WM_87734_MATRIX1;
#endif
    wm_87734_rotmatrixyxz(0x800877B0u, WM_87734_SCRATCH + 0xA0u, dest0);
    wm_87734_rotmatrixyxz(0x800877C0u, WM_87734_SCRATCH + 0xA8u, dest1);

#if defined(WM_87734_MUTANT_RETURN_0)
    return 0;
#else
    return 1;
#endif
}
