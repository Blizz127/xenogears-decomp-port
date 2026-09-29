/*
 * World-map wall-tangent projector 0x80095324.
 *
 * Register-width-faithful transcription of retail world_map.bin
 * [0x80095324, 0x80095414).  See world_map_helper_95324.h.
 *
 * The two libgte residents are canonical externs (PsyCross at the
 * production link); certificate builds provide recording versions.
 * Guest pointers are translated through the accepted PSX_ADDR model
 * (scratchpad 0x1F800000 included), matching wm_80085760.
 */
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_95324.h"

typedef struct {
    s32 vx;
    s32 vy;
    s32 vz;
    s32 pad;
} Wm95324Vector;

extern void OuterProduct12(Wm95324Vector *v0, Wm95324Vector *v1,
                           Wm95324Vector *v2);
extern long VectorNormal(Wm95324Vector *v0, Wm95324Vector *v1);

#if defined(WM_95324_TEST_TRACE)
extern void wm_95324_test_load(u32 address, u32 value);
extern void wm_95324_test_store(u32 address, u32 value);
#define WM_95324_TRACE_LOAD(a, v)  wm_95324_test_load((a), (v))
#define WM_95324_TRACE_STORE(a, v) wm_95324_test_store((a), (v))
#else
#define WM_95324_TRACE_LOAD(a, v)  ((void)0)
#define WM_95324_TRACE_STORE(a, v) ((void)0)
#endif

/* Scratch layout (retail absolute addresses). */
#define WM_95324_TANGENT_UNIT  0x1F800000u /* VectorNormal out */
#define WM_95324_DOWN_VEC      0x1F800010u /* (0, -0x1000, 0) */
#define WM_95324_TANGENT_RAW   0x1F800020u /* OuterProduct12 out */

static void *wm_95324_guest(u32 address)
{
    return PSX_ADDR(address);
}

static u32 wm_95324_load_u32(u32 addr)
{
    u32 v;

    memcpy(&v, wm_95324_guest(addr), sizeof(v));
    WM_95324_TRACE_LOAD(addr, v);
    return v;
}

static void wm_95324_store_u32(u32 addr, u32 v)
{
    memcpy(wm_95324_guest(addr), &v, sizeof(v));
    WM_95324_TRACE_STORE(addr, v);
}

static s32 wm_95324_as_s32(u32 b)
{
    s32 v;

    memcpy(&v, &b, sizeof(v));
    return v;
}

void wm_80095324(u32 normal_vec, u32 mov_vec, u32 out_vec)
{
    u32 tan_x;
    u32 mov_x;
    u32 tan_z;
    u32 mov_z;
    u32 sum;

    /* Down vector at 0x1F800010; retail store order +0x18, +0x10, +0x14. */
    wm_95324_store_u32(WM_95324_DOWN_VEC + 8u, 0u);
    wm_95324_store_u32(WM_95324_DOWN_VEC + 0u, 0u);
#if defined(WM_95324_MUTANT_WRONG_DOWN_SIGN)
    wm_95324_store_u32(WM_95324_DOWN_VEC + 4u, 0x1000u);
#elif defined(WM_95324_MUTANT_WRONG_DOWN_SLOT)
    wm_95324_store_u32(WM_95324_DOWN_VEC + 0u, 0xFFFFF000u);
#else
    wm_95324_store_u32(WM_95324_DOWN_VEC + 4u, 0xFFFFF000u); /* -0x1000 */
#endif

#if defined(WM_95324_MUTANT_SWAPPED_GTE_ORDER)
    (void)VectorNormal((Wm95324Vector *)wm_95324_guest(WM_95324_TANGENT_RAW),
                       (Wm95324Vector *)wm_95324_guest(WM_95324_TANGENT_UNIT));
    OuterProduct12((Wm95324Vector *)wm_95324_guest(normal_vec),
                   (Wm95324Vector *)wm_95324_guest(WM_95324_DOWN_VEC),
                   (Wm95324Vector *)wm_95324_guest(WM_95324_TANGENT_RAW));
#elif defined(WM_95324_MUTANT_WRONG_OP12_ARGS)
    OuterProduct12((Wm95324Vector *)wm_95324_guest(WM_95324_DOWN_VEC),
                   (Wm95324Vector *)wm_95324_guest(normal_vec),
                   (Wm95324Vector *)wm_95324_guest(WM_95324_TANGENT_RAW));
    (void)VectorNormal((Wm95324Vector *)wm_95324_guest(WM_95324_TANGENT_RAW),
                       (Wm95324Vector *)wm_95324_guest(WM_95324_TANGENT_UNIT));
#else
    OuterProduct12((Wm95324Vector *)wm_95324_guest(normal_vec),
                   (Wm95324Vector *)wm_95324_guest(WM_95324_DOWN_VEC),
                   (Wm95324Vector *)wm_95324_guest(WM_95324_TANGENT_RAW));
    (void)VectorNormal((Wm95324Vector *)wm_95324_guest(WM_95324_TANGENT_RAW),
                       (Wm95324Vector *)wm_95324_guest(WM_95324_TANGENT_UNIT));
#endif

    tan_x = wm_95324_load_u32(WM_95324_TANGENT_UNIT + 0u);
    mov_x = wm_95324_load_u32(mov_vec + 0u);
    /* mult + mflo: low 32 bits of the signed product. */
    sum = tan_x * mov_x;
#if defined(WM_95324_MUTANT_WRONG_TAN_OFFSET)
    tan_z = wm_95324_load_u32(WM_95324_TANGENT_UNIT + 4u);
#else
    tan_z = wm_95324_load_u32(WM_95324_TANGENT_UNIT + 8u);
#endif
    mov_z = wm_95324_load_u32(mov_vec + 8u);
    sum += tan_z * mov_z;

#if defined(WM_95324_MUTANT_WRONG_BRANCH_POLARITY)
    if (wm_95324_as_s32(sum) > 0) {
#else
    if (wm_95324_as_s32(sum) < 0) {
#endif
        /* negu: 32-bit two's-complement wrap. */
        wm_95324_store_u32(out_vec + 0u, 0u - tan_x);
        tan_z = wm_95324_load_u32(WM_95324_TANGENT_UNIT + 8u);
#if defined(WM_95324_MUTANT_MISSING_NEGATE)
        wm_95324_store_u32(out_vec + 8u, tan_z);
#else
        wm_95324_store_u32(out_vec + 8u, 0u - tan_z);
#endif
#if defined(WM_95324_MUTANT_WRONG_BRANCH_POLARITY)
    } else if (wm_95324_as_s32(sum) < 0) {
#else
    } else if (wm_95324_as_s32(sum) > 0) {
#endif
        wm_95324_store_u32(out_vec + 0u, tan_x);
        tan_z = wm_95324_load_u32(WM_95324_TANGENT_UNIT + 8u);
        wm_95324_store_u32(out_vec + 8u, tan_z);
    } else {
        /* Retail store order: +8 then +0. */
        wm_95324_store_u32(out_vec + 8u, 0u);
        wm_95324_store_u32(out_vec + 0u, 0u);
    }
#if !defined(WM_95324_MUTANT_SKIPPED_Y_STORE)
    /* Common tail: out.Y = 0 on every path. */
    wm_95324_store_u32(out_vec + 4u, 0u);
#endif
}
