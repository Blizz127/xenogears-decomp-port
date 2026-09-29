/*
 * World-map scheduler callback 0x800907F4 (slot 8 Table-A cb1).
 *
 * Rotation-animation leaf: optional flag 9/0xA retargets slot+0x20,
 * then a three-state signed dispatch updates two wrapping counters,
 * publishes wrapping heading words, fills two scratch SVECTORs, and
 * calls RotMatrixZYX twice into C620 records 2 and 3. Every path
 * returns scheduler state 1.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_907f4.h"

#define WM_907F4_POOL_PTR    0x8009BE24u
#define WM_907F4_CONTEXT_PTR 0x8009C620u
#define WM_907F4_RECORD_STRIDE 0x54u
#define WM_907F4_RECORD2     0xA8u
#define WM_907F4_RECORD3     0xFCu
#define WM_907F4_REC_VZ      0x1Cu
#define WM_907F4_REC_MATRIX  0x20u
#define WM_907F4_SCRATCH     0x1F800000u

_Static_assert(2u * WM_907F4_RECORD_STRIDE == WM_907F4_RECORD2,
               "C620 record 2 is 2 * 0x54");
_Static_assert(3u * WM_907F4_RECORD_STRIDE == WM_907F4_RECORD3,
               "C620 record 3 is 3 * 0x54");

#define WM_907F4_SLOT_FLAG   0x04u
#ifndef WM_907F4_SLOT_MODE
#define WM_907F4_SLOT_MODE   0x20u
#endif
#define WM_907F4_SLOT_HEAD0  0x50u
#define WM_907F4_SLOT_HEAD1  0x54u
#define WM_907F4_SLOT_CTR0   0x58u
#define WM_907F4_SLOT_CTR1   0x5Cu

#if defined(WM_907F4_MUTANT_SLOT20_OFFSET)
#undef WM_907F4_SLOT_MODE
#define WM_907F4_SLOT_MODE   0x22u
#endif

#if defined(WM_907F4_MUTANT_INC_THRESHOLD)
#define WM_907F4_INC_GATE    0x10
#else
#define WM_907F4_INC_GATE    0x11
#endif

#if defined(WM_907F4_MUTANT_CLAMP)
#define WM_907F4_CLAMP       0x7F
#else
#define WM_907F4_CLAMP       0x80
#endif

typedef struct {
    s16 vx;
    s16 vy;
    s16 vz;
    s16 pad;
} Wm907f4Svector;

typedef struct {
    s16 m[3][3];
    s16 pad;
    s32 t[3];
} Wm907f4Matrix;

extern Wm907f4Matrix *RotMatrixZYX_gte(Wm907f4Svector *r, Wm907f4Matrix *m);

static u16 wm_907f4_load_u16(u32 pc, u32 address) __attribute__((noinline));
static s16 wm_907f4_load_s16(u32 pc, u32 address) __attribute__((noinline));
static u32 wm_907f4_load_u32(u32 pc, u32 address) __attribute__((noinline));
static void wm_907f4_store_u16(u32 pc, u32 address, u16 value)
    __attribute__((noinline));
static void wm_907f4_store_u32(u32 pc, u32 address, u32 value)
    __attribute__((noinline));
static void wm_907f4_rotmatrixzyx(u32 pc, u32 r_addr, u32 m_addr)
    __attribute__((noinline));

static u16 wm_907f4_load_u16(u32 pc, u32 address)
{
    u16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_907F4_TEST_TRACE)
    wm_907f4_test_trace(pc, WM_907F4_TRACE_LHU, address, 2u, (u32)value);
#else
    (void)pc;
#endif
    return value;
}

static s16 wm_907f4_load_s16(u32 pc, u32 address)
{
    s16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_907F4_TEST_TRACE)
    wm_907f4_test_trace(pc, WM_907F4_TRACE_LH, address, 2u,
                        (u32)(s32)value);
#else
    (void)pc;
#endif
    return value;
}

static u32 wm_907f4_load_u32(u32 pc, u32 address)
{
    u32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
#if defined(WM_907F4_TEST_TRACE)
    wm_907f4_test_trace(pc, WM_907F4_TRACE_LW, address, 4u, value);
#else
    (void)pc;
#endif
    return value;
}

static void wm_907f4_store_u16(u32 pc, u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_907F4_TEST_TRACE)
    wm_907f4_test_trace(pc, WM_907F4_TRACE_SH, address, 2u, (u32)value);
#else
    (void)pc;
#endif
}

static void wm_907f4_store_u32(u32 pc, u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
#if defined(WM_907F4_TEST_TRACE)
    wm_907f4_test_trace(pc, WM_907F4_TRACE_SW, address, 4u, value);
#else
    (void)pc;
#endif
}

static s32 wm_907f4_u32_as_s32(u32 value)
{
    s32 result;

    memcpy(&result, &value, sizeof(result));
    return result;
}

static void wm_907f4_rotmatrixzyx(u32 pc, u32 r_addr, u32 m_addr)
{
#if defined(WM_907F4_TEST_TRACE)
    wm_907f4_test_trace(pc, WM_907F4_TRACE_CALL, r_addr, 8u, m_addr);
    wm_907f4_test_rotmatrixzyx(pc, r_addr, m_addr);
#else
    (void)pc;
    (void)RotMatrixZYX_gte((Wm907f4Svector *)PSX_ADDR(r_addr),
                           (Wm907f4Matrix *)PSX_ADDR(m_addr));
#endif
}

s32 wm_800907F4(s32 slot_index)
{
    u32 pool;
    u32 context;
    u32 slot;
    u32 record2;
    u32 record3;
    s16 flag;
    s16 mode;
    u32 ctr0;
    u32 ctr1;
    u32 head0;
    u32 head1;
    u16 vz;

    /* 0x800907FC / 0x80090800 and 0x80090804 / 0x80090808: pool then C620. */
    pool = wm_907f4_load_u32(0x80090800u, WM_907F4_POOL_PTR);
    context = wm_907f4_load_u32(0x80090808u, WM_907F4_CONTEXT_PTR);
    slot = pool + ((u32)slot_index << 7);
    record2 = context + WM_907F4_RECORD2;
    record3 = context + WM_907F4_RECORD3;

    flag = wm_907f4_load_s16(0x80090824u, slot + WM_907F4_SLOT_FLAG);
#if defined(WM_907F4_MUTANT_FLAG_SKIP)
    (void)flag;
#else
    if (flag == 9) {
        wm_907f4_store_u16(0x80090848u, slot + WM_907F4_SLOT_FLAG, 0u);
        wm_907f4_store_u16(0x8009084Cu, slot + WM_907F4_SLOT_MODE, 1u);
    } else if (flag == 10) {
        wm_907f4_store_u16(0x80090848u, slot + WM_907F4_SLOT_FLAG, 0u);
        wm_907f4_store_u16(0x8009084Cu, slot + WM_907F4_SLOT_MODE, 2u);
    }
#endif

    mode = wm_907f4_load_s16(0x80090850u, slot + WM_907F4_SLOT_MODE);
    if (mode == 1) {
        ctr0 = wm_907f4_load_u32(0x80090898u, slot + WM_907F4_SLOT_CTR0);
        ctr0 += 4u;
        wm_907f4_store_u32(0x800908A4u, slot + WM_907F4_SLOT_CTR0, ctr0);
        if (!(wm_907f4_u32_as_s32(ctr0) < WM_907F4_INC_GATE)) {
            ctr1 = wm_907f4_load_u32(0x800908B4u, slot + WM_907F4_SLOT_CTR1);
            ctr1 += 4u;
            wm_907f4_store_u32(0x800908C0u, slot + WM_907F4_SLOT_CTR1, ctr1);
        }
        ctr0 = wm_907f4_load_u32(0x800908C4u, slot + WM_907F4_SLOT_CTR0);
        if (!(wm_907f4_u32_as_s32(ctr0) < WM_907F4_CLAMP)) {
            wm_907f4_store_u32(0x800908D8u, slot + WM_907F4_SLOT_CTR0,
                               (u32)WM_907F4_CLAMP);
        }
        ctr1 = wm_907f4_load_u32(0x800908DCu, slot + WM_907F4_SLOT_CTR1);
        if (!(wm_907f4_u32_as_s32(ctr1) < WM_907F4_CLAMP)) {
            wm_907f4_store_u32(0x800908F0u, slot + WM_907F4_SLOT_CTR1,
                               (u32)WM_907F4_CLAMP);
        }
        ctr0 = wm_907f4_load_u32(0x800908F4u, slot + WM_907F4_SLOT_CTR0);
        ctr1 = wm_907f4_load_u32(0x800908F8u, slot + WM_907F4_SLOT_CTR1);
        if (!(wm_907f4_u32_as_s32(ctr0) < WM_907F4_CLAMP) &&
            !(wm_907f4_u32_as_s32(ctr1) < WM_907F4_CLAMP)) {
            wm_907f4_store_u16(0x8009091Cu, slot + WM_907F4_SLOT_MODE, 3u);
        }
    } else if (mode < 2) {
        if (mode == 0) {
            wm_907f4_store_u32(0x8009088Cu, slot + WM_907F4_SLOT_CTR0, 0u);
#if !defined(WM_907F4_MUTANT_MODE0_SKIP_5C)
            wm_907f4_store_u32(0x80090894u, slot + WM_907F4_SLOT_CTR1, 0u);
#endif
        }
    } else if (mode == 2) {
        ctr0 = wm_907f4_load_u32(0x80090920u, slot + WM_907F4_SLOT_CTR0);
        ctr0 += 0xFFFFFFFCu;
        wm_907f4_store_u32(0x8009092Cu, slot + WM_907F4_SLOT_CTR0, ctr0);
        if (wm_907f4_u32_as_s32(ctr0) < 0x70) {
            ctr1 = wm_907f4_load_u32(0x8009093Cu, slot + WM_907F4_SLOT_CTR1);
            ctr1 += 0xFFFFFFFCu;
            wm_907f4_store_u32(0x80090948u, slot + WM_907F4_SLOT_CTR1, ctr1);
        }
        ctr0 = wm_907f4_load_u32(0x8009094Cu, slot + WM_907F4_SLOT_CTR0);
        if (wm_907f4_u32_as_s32(ctr0) < 0) {
            wm_907f4_store_u32(0x8009095Cu, slot + WM_907F4_SLOT_CTR0, 0u);
        }
        ctr1 = wm_907f4_load_u32(0x80090960u, slot + WM_907F4_SLOT_CTR1);
        if (wm_907f4_u32_as_s32(ctr1) < 0) {
            wm_907f4_store_u32(0x80090970u, slot + WM_907F4_SLOT_CTR1, 0u);
        }
        ctr0 = wm_907f4_load_u32(0x80090974u, slot + WM_907F4_SLOT_CTR0);
        ctr1 = wm_907f4_load_u32(0x80090978u, slot + WM_907F4_SLOT_CTR1);
        if (ctr0 == 0u && ctr1 == 0u) {
            wm_907f4_store_u16(0x80090990u, slot + WM_907F4_SLOT_MODE, 0u);
        }
    }

    head0 = wm_907f4_load_u32(0x80090994u, slot + WM_907F4_SLOT_HEAD0);
    ctr0 = wm_907f4_load_u32(0x80090998u, slot + WM_907F4_SLOT_CTR0);
    head1 = wm_907f4_load_u32(0x8009099Cu, slot + WM_907F4_SLOT_HEAD1);
    ctr1 = wm_907f4_load_u32(0x800909A0u, slot + WM_907F4_SLOT_CTR1);
    wm_907f4_store_u32(0x800909ACu, slot + WM_907F4_SLOT_HEAD0, head0 + ctr0);
    wm_907f4_store_u32(0x800909B0u, slot + WM_907F4_SLOT_HEAD1, head1 - ctr1);

    /* Retail store order: rot2.vx, rot1.vx, rot1.vy, rot2.vy, rot1.vz,
     * then rot2.vz in the first jal delay slot. */
    wm_907f4_store_u16(0x800909B4u, WM_907F4_SCRATCH + 0xA8u, 0u);
    wm_907f4_store_u16(0x800909B8u, WM_907F4_SCRATCH + 0xA0u, 0u);
    head0 = wm_907f4_load_u32(0x800909BCu, slot + WM_907F4_SLOT_HEAD0);
    wm_907f4_store_u16(0x800909C4u, WM_907F4_SCRATCH + 0xA2u, (u16)head0);
    head1 = wm_907f4_load_u32(0x800909C8u, slot + WM_907F4_SLOT_HEAD1);
    wm_907f4_store_u16(0x800909D0u, WM_907F4_SCRATCH + 0xAAu, (u16)head1);
    vz = wm_907f4_load_u16(0x800909D4u, record2 + WM_907F4_REC_VZ);
#if defined(WM_907F4_MUTANT_VZ_ZERO)
    vz = 0u;
#endif
    wm_907f4_store_u16(0x800909DCu, WM_907F4_SCRATCH + 0xA4u, vz);
    vz = wm_907f4_load_u16(0x800909E0u, record3 + WM_907F4_REC_VZ);
#if defined(WM_907F4_MUTANT_VZ_ZERO)
    vz = 0u;
#endif
    wm_907f4_store_u16(0x800909ECu, WM_907F4_SCRATCH + 0xACu, vz);

#if defined(WM_907F4_MUTANT_ROT_DEST_SCRATCH)
    wm_907f4_rotmatrixzyx(0x800909E8u, WM_907F4_SCRATCH + 0xA0u,
                          WM_907F4_SCRATCH + 0xA0u);
    wm_907f4_rotmatrixzyx(0x800909F4u, WM_907F4_SCRATCH + 0xA8u,
                          WM_907F4_SCRATCH + 0xA8u);
#else
    wm_907f4_rotmatrixzyx(0x800909E8u, WM_907F4_SCRATCH + 0xA0u,
                          record2 + WM_907F4_REC_MATRIX);
    wm_907f4_rotmatrixzyx(0x800909F4u, WM_907F4_SCRATCH + 0xA8u,
                          record3 + WM_907F4_REC_MATRIX);
#endif

#if defined(WM_907F4_MUTANT_RETURN_0)
    return 0;
#else
    return 1;
#endif
}
