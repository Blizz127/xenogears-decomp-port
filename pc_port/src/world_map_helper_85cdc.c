/* World-map helper 0x80085CDC (object projection/render/animation pass). */
#include <stdint.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#if !defined(WM_85CDC_CERTIFICATE)
#include <psx/inline_c.h>
#endif
#include "world_map_helper_85cdc.h"
#include "world_map_helper_93484.h"

#define D_8009BE24  0x8009BE24u  /* pool pointer */
#define D_8009BE28  0x8009BE28u  /* camera position */
#define D_8009BE3C  0x8009BE3Cu  /* active draw environment */
#define D_8009BD3A  0x8009BD3Au  /* world heading */
#define D_8009C808  0x8009C808u  /* camera matrix */
#define SCRATCH     0x1F800000u
#define OBJ_COUNT   64
#define OBJ_STRIDE  0x80

static s16 o_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s32 o_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static u16 o_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static void o_sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }

static void n_sw(u32 a, u32 v) { memcpy((void *)(uintptr_t)a, &v, 4); }

#if !defined(WM_85CDC_CERTIFICATE)
static void o_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static s16 n_lh(u32 a) { s16 v; memcpy(&v, (void *)(uintptr_t)a, 2); return v; }
#endif

#if defined(WM_85CDC_CERTIFICATE)
extern s32 wm_85cdc_test_project_depth(void *object);
extern void wm_85cdc_test_set_render_matrix(MATRIX *matrix);
extern void wm_85cdc_test_render(void *object, void *ot);
extern void wm_85cdc_test_set_direction(void *object, s16 direction);
extern void wm_85cdc_test_animation_tick(void *object);
#define WM_SET_RENDER_MATRIX(matrix) wm_85cdc_test_set_render_matrix(matrix)
#define WM_RENDER(object, ot) wm_85cdc_test_render((object), (ot))
#define WM_SET_DIRECTION(object, direction) \
    wm_85cdc_test_set_direction((object), (direction))
#define WM_ANIMATION_TICK(object) wm_85cdc_test_animation_tick(object)
#else
extern void func_80024FF4(MATRIX *matrix);
extern void func_8001E298(void *object, void *ot);
extern void func_800223B0(void *object, s16 direction);
extern void AnimScriptTick(void *object);
#define WM_SET_RENDER_MATRIX(matrix) func_80024FF4(matrix)
#define WM_RENDER(object, ot) func_8001E298((object), (ot))
#define WM_SET_DIRECTION(object, direction) func_800223B0((object), (direction))
#define WM_ANIMATION_TICK(object) AnimScriptTick(object)
#endif

static s32 wm_85cdc_project_depth(u32 object)
{
#if defined(WM_85CDC_CERTIFICATE)
    return wm_85cdc_test_project_depth((void *)(uintptr_t)object);
#else
    void *vertex = PSX_ADDR(SCRATCH);
    u32 depth;

    o_sh(SCRATCH + 0u, (u16)n_lh(object + 2u));
    o_sh(SCRATCH + 2u, (u16)n_lh(object + 6u));
    o_sh(SCRATCH + 4u, (u16)n_lh(object + 0x0Au));
    gte_ldv0(vertex);
    gte_rtps();
    gte_stsz(&depth);
    return (s32)depth;
#endif
}

void wm_80085CDC(void)
{
    s32 i;
    u32 pool = (u32)o_lw(D_8009BE24);
    u32 entry = pool + 0x2C;

    /* Phase 1: Compute camera-relative deltas for 64 objects */
    for (i = 0; i < OBJ_COUNT; i++) {
        s16 state = o_lh(entry - 0x28);
        u32 transform_ptr = (u32)o_lw(entry + 0x20);

        if (state == 0 && transform_ptr != 0) {
            /* Compute delta: object pos - camera pos */
            s32 dx = (s32)((u32)o_lw(entry - 4) -
                           (u32)o_lw(D_8009BE28));
            s32 dz = (s32)((u32)o_lw(entry + 4) -
                           (u32)o_lw(D_8009BE28 + 8));

            /* Write delta to scratchpad */
            o_sw(SCRATCH + 8, (u32)dx);
            o_sw(SCRATCH + 0x10, (u32)dz);

            /* Wrap delta */
            wm_80093484(SCRATCH + 8);

            /* Write scaled delta to transform */
            n_sw(transform_ptr + 0u, (u32)o_lw(SCRATCH + 8u) << 4);
            n_sw(transform_ptr + 8u,
                 (u32)(0u - (u32)o_lw(SCRATCH + 0x10u)) << 4);
            n_sw(transform_ptr + 4u, (u32)o_lw(entry) << 4);
        }

        entry += OBJ_STRIDE;
    }

    /* Phase 2: Set GTE matrices */
    {
        MATRIX* cam = (MATRIX*)PSX_ADDR(D_8009C808);
        SetRotMatrix(cam);
        SetTransMatrix(cam);
    }

    /* Phase 3: project each active object's anchor and retain raw SZ3. */
    entry = pool + 0x4C;
    for (i = 0; i < OBJ_COUNT; i++) {
        s16 state = o_lh(entry - 0x28);
        u32 object = (u32)o_lw(entry);

        if (state == 0 && object != 0u)
            o_sw(SCRATCH + 0x18u + (u32)i * 4u,
                 (u32)wm_85cdc_project_depth(object));

        entry += OBJ_STRIDE;
    }

    /* Phase 4: render, turn toward the requested heading, then tick pose. */
    WM_SET_RENDER_MATRIX((MATRIX *)PSX_ADDR(D_8009C808));
    entry = pool + 0x4C;
    for (i = 0; i < OBJ_COUNT; i++) {
        s16 state = o_lh(entry - 0x28u);
        u32 object = (u32)o_lw(entry);
        s32 depth = o_lw(SCRATCH + 0x18u + (u32)i * 4u);

#if defined(WM_85CDC_MUTANT_WRONG_DEPTH_GATE)
        if (state == 0 && object != 0u && (u32)depth < 0xB00u) {
#else
        if (state == 0 && object != 0u && depth < 0xB00) {
#endif
            u32 environment = (u32)o_lw(D_8009BE3C);
            u32 ot = (u32)o_lw(environment + 0x70u);
            s32 bucket = depth >> 4;
            s32 target = o_lh(entry - 4u);
            s32 current = o_lw(entry + 0x10u);
            s32 difference = (s32)((u32)target - (u32)current);
            s32 next;

#if defined(WM_85CDC_MUTANT_WRONG_OT_BUCKET)
            WM_RENDER((void *)(uintptr_t)object,
                      (u8 *)PSX_ADDR(ot) + bucket);
#else
            WM_RENDER((void *)(uintptr_t)object,
                      (u8 *)PSX_ADDR(ot) + bucket * 4);
#endif

            if (difference < 0)
                difference += 0x1000;
#if defined(WM_85CDC_MUTANT_WRONG_HEADING_CLAMP)
            next = target;
#else
            if (difference < 0x801) {
                next = difference < 0x100
                           ? target
                           : (s32)((u32)current + 0x100u);
            } else {
                difference -= 0x1000;
                next = difference < -0xFF
                           ? (s32)((u32)current - 0x100u)
                           : target;
            }
#endif
            o_sw(entry + 0x10u, (u32)next);
#if defined(WM_85CDC_MUTANT_WRONG_DIRECTION)
            WM_SET_DIRECTION((void *)(uintptr_t)object,
                             (s16)((next - (s32)o_lhu(D_8009BD3A)) & 0xFFF));
#else
            WM_SET_DIRECTION(
                (void *)(uintptr_t)object,
                (s16)((next - (s32)o_lhu(D_8009BD3A) - 0x400) & 0xFFF));
#endif
#if !defined(WM_85CDC_MUTANT_NO_ANIMATION_TICK)
            WM_ANIMATION_TICK((void *)(uintptr_t)object);
#endif
#if !defined(WM_85CDC_CERTIFICATE)
            {
                static int s_walkDump;
                extern char* getenv(const char*);
                const char* dumpEnv = getenv("XENO_WALK_ANIM_DUMP");
                u8* sprite = (u8*)(uintptr_t)object;
                u8* scriptPc;

                if (dumpEnv != NULL && dumpEnv[0] != '\0' && dumpEnv[0] != '0' &&
                    s_walkDump < 80) {
                    scriptPc = (u8*)(uintptr_t)*(u32*)(sprite + 0x64);
                    printf("[walk-anim] world n=%d slot=%d anim=%d pose=%d wait=%d "
                           "op=0x%02x\n",
                           s_walkDump, i, (int)(s8)sprite[0xAF],
                           (int)*(s16*)(sprite + 0x34),
                           (int)*(s16*)(sprite + 0x9E),
                           scriptPc != NULL ? (unsigned)scriptPc[0] : 0u);
                    s_walkDump++;
                }
            }
#endif
        }

        entry += OBJ_STRIDE;
    }
}
