/*
 * World-map helper 0x80089C78 (scaled object renderer).
 *
 * Retail function boundary: [0x80089C78, 0x8008A2C8).  The implementation
 * iterates the dynamic object pool, builds each record's camera-relative
 * model transform, projects its four static vertices, and compacts accepted
 * POLY_FT4 packets into the active guest OT.  The retired placeholder had
 * invented a fixed object table at 0x8009B040 and passed table storage to
 * wm_80093534, corrupting unrelated globals (notably 0x8009D7EC).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_89c78.h"
#include "world_map_helper_93534.h"

#define SCRATCH      0x1F800000u
#define D_8009C808   0x8009C808u  /* camera matrix */
#define D_8009A180   0x8009A180u  /* model matrix */
#define D_8009AFF0   0x8009AFF0u  /* per-model UV table (8-byte records) */
#define D_8009B040   0x8009B040u  /* retail vertex table, not object records */
#define D_8009BDF4   0x8009BDF4u  /* 256 x 0x4c object-record pool pointer */
#define D_8009BE1C   0x8009BE1Cu  /* double-buffered POLY_FT4 pool roots */
#define D_8009BE28   0x8009BE28u  /* camera position */
#define D_8009BE30   0x8009BE30u  /* camera position Z */
#define D_8009BE3C   0x8009BE3Cu  /* active draw-environment record */
#define D_8009D7F0   0x8009D7F0u  /* double-buffer index */
#define OBJECT_COUNT 256
#define OBJECT_STRIDE 0x4Cu
#define PACKET_STRIDE 0x28u

static s16 s_lh(u32 a) { s16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static u16 s_lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }
static s32 s_lw(u32 a) { s32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static u32 s_lwu(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static void s_sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void s_sw(u32 a, s32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static void s_swu(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }

static s32 wm_89c78_add_bits(s32 left, s32 right)
{
    u32 bits = (u32)left + (u32)right;
    s32 result;
    memcpy(&result, &bits, sizeof(result));
    return result;
}

static u32 wm_89c78_table_address(u32 base, s16 index, u32 stride)
{
#if defined(WM_89C78_MUTANT_UNSIGNED_MODEL_INDEX)
    u32 offset = (u32)(u16)index * stride;
#else
    s32 offset = (s32)index * (s32)stride;
#endif
    return base + (u32)offset;
}

static int wm_89c78_overlaps_screen(u32 xy0, u32 xy1, u32 xy2, u32 xy3)
{
    s16 x0 = (s16)(u16)xy0;
    s16 y0 = (s16)(u16)(xy0 >> 16);
#if !defined(WM_89C78_MUTANT_BAD_SCREEN_GATE)
    s16 x1 = (s16)(u16)xy1;
    s16 x2 = (s16)(u16)xy2;
    s16 x3 = (s16)(u16)xy3;
    s16 y1 = (s16)(u16)(xy1 >> 16);
    s16 y2 = (s16)(u16)(xy2 >> 16);
    s16 y3 = (s16)(u16)(xy3 >> 16);
#else
    (void)xy1;
    (void)xy2;
    (void)xy3;
#endif

#if defined(WM_89C78_MUTANT_BAD_SCREEN_GATE)
    return x0 >= 0 && x0 < 320 && y0 >= 0 && y0 < 216;
#else
    /* Retail 0x8008A0C4-0x8008A160: at least one signed X is below
     * 320 and at least one signed Y is below 216.  Negative coordinates
     * deliberately satisfy the upper-edge tests. */
    return (x0 < 320 || x1 < 320 || x2 < 320 || x3 < 320) &&
           (y0 < 216 || y1 < 216 || y2 < 216 || y3 < 216);
#endif
}

static void wm_89c78_link_packet(u32 ot_address, u32 packet)
{
#if defined(WM_89C78_MUTANT_RAW_OT_LINK)
    u32 old_tag = s_lwu(ot_address);
    s_swu(packet, (s_lwu(packet) & 0xFF000000u) |
                   (old_tag & 0x00FFFFFFu));
    s_swu(ot_address, (old_tag & 0xFF000000u) |
                         ((u32)(uintptr_t)PSX_ADDR(packet) & 0x00FFFFFFu));
#else
    PcPort_AddPrimDomainAware(PSX_ADDR(ot_address), PSX_ADDR(packet));
#endif
}

static u32 wm_89c78_record_pool(void)
{
#if defined(WM_89C78_MUTANT_FIXED_TABLE)
    return D_8009B040;
#else
    /* Retail 0x80089D94-0x80089DB0: lw 0x8009BDF4, then record + 6. */
    return (u32)s_lw(D_8009BDF4);
#endif
}

static int wm_89c78_record_is_active(u32 record)
{
#if defined(WM_89C78_MUTANT_ZERO_IS_ACTIVE)
    return s_lh(record + 6u) == 0;
#else
    /* Retail 0x80089DB4-0x80089DC0 skips when the halfword is zero. */
    return s_lh(record + 6u) != 0;
#endif
}

void wm_80089C78(u32 input_addr)
{
    s32 i;
    u32 packet;

    /* Retail does not consume $a0 in [0x80089C78, 0x8008A2C8). */
    (void)input_addr;

    /* Copy camera matrix to scratchpad+0x28 */
    memcpy((void*)PSX_ADDR(SCRATCH + 0x28),
           (void*)PSX_ADDR(D_8009C808), 32);

    /* Copy model matrix to scratchpad+0x68 */
    memcpy((void*)PSX_ADDR(SCRATCH + 0x68),
           (void*)PSX_ADDR(D_8009A180), 32);

    packet = s_lwu(D_8009BE1C + (s_lwu(D_8009D7F0) << 2));

    /* Retail loop: prepare each live object's model matrix and wrapped
     * camera-relative vector, project its four static vertices, then compact
     * accepted POLY_FT4 packets into the active 256-record pool. */
    {
        u32 table = wm_89c78_record_pool();
        s32 cam_x = s_lw(D_8009BE28) >> 12;
        s32 cam_z = s_lw(D_8009BE30) >> 12;

        for (i = 0; i < OBJECT_COUNT; i++) {
            u32 entry = table + (u32)i * OBJECT_STRIDE;
            s32 rel_x;
            s32 rel_z;
            s16 model_index;
            u32 vertex_table;
            u32 uv_table;
            MATRIX *camera;
            MATRIX *model;
            SVECTOR *position;
            VECTOR *rotated;
            VECTOR *scale;
#if defined(WM_89C78_MUTANT_HOST_SCALE)
            VECTOR host_scale;
#endif
            long xy0 = 0;
            long xy1 = 0;
            long xy2 = 0;
            long xy3 = 0;
            long p = 0;
            long flag = 0;
            u16 depth;
            u32 draw_record;
            u32 ot_base;
            u32 ot_address;

            if (!wm_89c78_record_is_active(entry))
                continue;

            /* Retail copies the base model matrix anew for every live record
             * (0x80089DE0-0x80089E1C). */
            memcpy((void*)PSX_ADDR(SCRATCH + 0x48),
                   (void*)PSX_ADDR(SCRATCH + 0x68), 32);

            if ((*(u8*)PSX_ADDR(entry + 0x47u) & 1u) != 0u)
                (void)RotMatrixZ((int)(s16)s_lhu(entry + 2u),
                                 (MATRIX*)PSX_ADDR(SCRATCH + 0x48));

            /* Retail writes a VECTOR (three 32-bit components), not an
             * SVECTOR.  Z scale is the literal fixed-point 1.0. */
#if defined(WM_89C78_MUTANT_HOST_SCALE)
            scale = &host_scale;
#else
            scale = (VECTOR*)PSX_ADDR(SCRATCH + 0x98u);
#endif
            scale->vx = (s32)(u16)s_lhu(entry + 0x38u);
            scale->vy = (s32)(u16)s_lhu(entry + 0x3Au);
            scale->vz = 0x1000;
            scale->pad = 0;
            (void)ScaleMatrix((MATRIX*)PSX_ADDR(SCRATCH + 0x48), scale);

            model_index = s_lh(entry + 6u);
            vertex_table = wm_89c78_table_address(D_8009B040, model_index,
                                                   0x20u);
            memcpy(PSX_ADDR(SCRATCH), PSX_ADDR(vertex_table), 0x20u);

#if defined(WM_89C78_MUTANT_WRONG_POSITION_FIELDS)
            rel_x = (s_lw(entry + 4u) >> 12) - cam_x;
            rel_z = (s_lw(entry + 0x0Cu) >> 12) - cam_z;
#else
            /* Retail s0 is record+6; its +2/+6/+10 words are therefore
             * record +8/+0xc/+0x10 (0x80089F14-0x80089F38). */
            rel_x = (s_lw(entry + 8u) >> 12) - cam_x;
            rel_z = (s_lw(entry + 0x10u) >> 12) - cam_z;
#endif
#if defined(WM_89C78_MUTANT_NO_CAMERA_SUBTRACT)
            rel_x += cam_x;
            rel_z += cam_z;
#endif
            s_sw(SCRATCH + 0x88, rel_x);
            s_sw(SCRATCH + 0x90, rel_z);

#if defined(WM_89C78_MUTANT_WRAP_RECORD)
            wm_80093534(entry + 0x28u);
#else
            /* Retail a0 = 0x1F800088.  Object storage is read-only here. */
            wm_80093534(SCRATCH + 0x88);
#endif

            /* Retail 0x80089F40-0x80089FB0 packs the wrapped relative
             * position as (x, y, -z), rotates it by camera R without camera
             * translation, and stores MAC1..3 back at scratch +0x88. */
            position = (SVECTOR*)PSX_ADDR(SCRATCH + 0x20u);
            position->vx = (s16)s_lw(SCRATCH + 0x88u);
            position->vy = (s16)(s_lw(entry + 0x0Cu) >> 12);
            position->vz = (s16)(u16)(0u - s_lwu(SCRATCH + 0x90u));
            camera = (MATRIX*)PSX_ADDR(SCRATCH + 0x28u);
            rotated = (VECTOR*)PSX_ADDR(SCRATCH + 0x88u);
            SetRotMatrix(camera);
            (void)ApplyRotMatrix(position, rotated);

            /* Retail 0x80089FB4-0x8008A024 installs camera-R*object-R and
             * translated camera-relative object position. */
            model = (MATRIX*)PSX_ADDR(SCRATCH + 0x48u);
            model->t[0] = wm_89c78_add_bits(rotated->vx, camera->t[0]);
            model->t[1] = wm_89c78_add_bits(rotated->vy, camera->t[1]);
            model->t[2] = wm_89c78_add_bits(rotated->vz, camera->t[2]);
            SetRotMatrix(model);
            SetTransMatrix(model);

            (void)RotTransPers4(
                (SVECTOR*)PSX_ADDR(SCRATCH + 0x00u),
                (SVECTOR*)PSX_ADDR(SCRATCH + 0x08u),
                (SVECTOR*)PSX_ADDR(SCRATCH + 0x10u),
                (SVECTOR*)PSX_ADDR(SCRATCH + 0x18u),
                &xy0, &xy1, &xy2, &xy3, &p, &flag);
#if defined(WM_89C78_MUTANT_SKIP_FLAG_GATE)
            (void)flag;
#else
            if (((u32)flag & 0x80000000u) != 0u)
                continue;
#endif
            if (!wm_89c78_overlaps_screen((u32)xy0, (u32)xy1,
                                           (u32)xy2, (u32)xy3))
                continue;

            depth = (u16)C2_SZ3;
#if defined(WM_89C78_MUTANT_BAD_DEPTH_GATE)
            if (depth < 0x0C00u)
                continue;
#else
            if (depth >= 0x0C00u)
                continue;
#endif

            s_swu(packet + 0x08u, (u32)xy0);
            s_swu(packet + 0x10u, (u32)xy1);
            s_swu(packet + 0x18u, (u32)xy2);
            s_swu(packet + 0x20u, (u32)xy3);
            *(u8*)PSX_ADDR(packet + 4u) = *(u8*)PSX_ADDR(entry + 0x40u);
            *(u8*)PSX_ADDR(packet + 5u) = *(u8*)PSX_ADDR(entry + 0x41u);
            *(u8*)PSX_ADDR(packet + 6u) = *(u8*)PSX_ADDR(entry + 0x42u);
            s_sh(packet + 0x16u, s_lhu(entry + 0x48u));

            uv_table = wm_89c78_table_address(D_8009AFF0, model_index, 8u);
#if defined(WM_89C78_MUTANT_WRONG_UV_RECORD)
            uv_table += 8u;
#endif
            s_sh(packet + 0x0Cu, s_lhu(uv_table + 0u));
            s_sh(packet + 0x14u, s_lhu(uv_table + 2u));
            s_sh(packet + 0x1Cu, s_lhu(uv_table + 4u));
            s_sh(packet + 0x24u, s_lhu(uv_table + 6u));

            draw_record = s_lwu(D_8009BE3C);
            ot_base = s_lwu(draw_record + 0x70u);
            ot_address = ot_base + ((u32)depth >> 4) * 4u;
            /* TEMP-DIAG (world-map white-quad hunt): per-object trace. Revert. */
            {
                extern int PcPort_WorldCaptureCurFrame(void);
                extern char *getenv(const char *);
                static int s_inited = 0;
                static int s_on = 0;
                static int s_f0 = 0;
                static int s_f1 = 0;
                if (!s_inited) {
                    const char *e = getenv("XENO_WM_OBJ_DIAG");
                    const char *a = getenv("XENO_WM_OBJ_DIAG_F0");
                    const char *b = getenv("XENO_WM_OBJ_DIAG_F1");
                    s_on = (e != NULL && e[0] != '\0' && e[0] != '0');
                    s_f0 = a != NULL ? atoi(a) : 0;
                    s_f1 = b != NULL ? atoi(b) : 0;
                    s_inited = 1;
                }
                if (s_on) {
                    int wf = PcPort_WorldCaptureCurFrame();
                    if (wf >= s_f0 && wf <= s_f1) {
                        u8 *pk = (u8 *)PSX_ADDR(packet);
                        printf("[wm-obj] wf=%d rec=%d model=%d "
                               "xy=(%d,%d)-(%d,%d)-(%d,%d)-(%d,%d) "
                               "rgb=%02x%02x%02x tpage=%04x "
                               "uv=%04x/%04x/%04x/%04x\n",
                               wf, (int)i, (int)model_index,
                               (int)(s16)(s_lhu(packet + 8u)),
                               (int)(s16)(s_lhu(packet + 10u)),
                               (int)(s16)(s_lhu(packet + 16u)),
                               (int)(s16)(s_lhu(packet + 18u)),
                               (int)(s16)(s_lhu(packet + 24u)),
                               (int)(s16)(s_lhu(packet + 26u)),
                               (int)(s16)(s_lhu(packet + 32u)),
                               (int)(s16)(s_lhu(packet + 34u)),
                               (unsigned)pk[4], (unsigned)pk[5],
                               (unsigned)pk[6],
                               (unsigned)s_lhu(packet + 0x16u),
                               (unsigned)s_lhu(uv_table + 0u),
                               (unsigned)s_lhu(uv_table + 2u),
                               (unsigned)s_lhu(uv_table + 4u),
                               (unsigned)s_lhu(uv_table + 6u));
                    }
                }
            }
            wm_89c78_link_packet(ot_address, packet);
#if defined(WM_89C78_MUTANT_NONCOMPACT_CURSOR)
            packet += PACKET_STRIDE * 2u;
#else
            packet += PACKET_STRIDE;
#endif
        }
    }
}
