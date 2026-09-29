/* W34N41 — production-linked certificate for retail wm_80099BFC. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "guest_prim_link.h"
#include "psx_memory.h"
#include "psyq/libgte.h"
#include <psx/gtereg.h>
#include "world_map_helper_99bfc.h"

#define SCRATCH      0x1F800000u
#define COUNT_G      0x8009BE04u
#define CAM_X        0x8009BE28u
#define CAM_Z        0x8009BE30u
#define WRAP_X       0x8009D160u
#define WRAP_Z       0x8009D2B4u
#define ENTRIES      0x80100000u
#define OT_BASE      0x80110000u
#define PACKETS      0x80120000u

uint8_t g_PsxRam[PSX_RAM_SIZE];
GTERegisters gteRegs;

static int s_failures;
static int s_set_rot_calls;
static int s_set_trans_calls;
static int s_apply_calls;
static int s_rtpt_calls;
static int s_rtps_calls;
static int s_link_calls;
static u32 s_set_rot[32];
static u32 s_set_trans[16];
static SVECTOR s_apply_input[32];
static s32 s_model_t[32][3];
static u32 s_link_ot[16];
static u32 s_link_packet[16];

static void st16(u32 address, u16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void st32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u16 ld16(u32 address)
{
    u16 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 ld32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static u32 guest_of(const void *pointer)
{
    return PsxMemory_GuestAddr(pointer);
}

static void check(const char *name, int condition)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

void SetRotMatrix(MATRIX *matrix)
{
    if (s_set_rot_calls < 32)
        s_set_rot[s_set_rot_calls] = guest_of(matrix);
    s_set_rot_calls++;
}

void SetTransMatrix(MATRIX *matrix)
{
    if (s_set_trans_calls < 16) {
        s_set_trans[s_set_trans_calls] = guest_of(matrix);
        s_model_t[s_set_trans_calls][0] = matrix->t[0];
        s_model_t[s_set_trans_calls][1] = matrix->t[1];
        s_model_t[s_set_trans_calls][2] = matrix->t[2];
    }
    s_set_trans_calls++;
}

VECTOR *ApplyRotMatrix(SVECTOR *input, VECTOR *output)
{
    if (s_apply_calls < 32)
        s_apply_input[s_apply_calls] = *input;
    output->vx = 10 + s_apply_calls;
    output->vy = 20 + s_apply_calls;
    output->vz = 30 + s_apply_calls;
    output->pad = 0;
    s_apply_calls++;
    return output;
}

int RotTransPers3(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2,
                  long *xy0, long *xy1, long *xy2,
                  long *p, long *flag)
{
    int call = s_rtpt_calls;
    (void)v0;
    (void)v1;
    (void)v2;
    *p = 0;
    *flag = call == 1 ? (long)UINT32_C(0x80000000) : 0;
    if (call == 2) {
        *xy0 = (long)UINT32_C(0x012C0190);
        *xy1 = (long)UINT32_C(0x012D0191);
        *xy2 = (long)UINT32_C(0x012E0192);
    } else {
        *xy0 = (long)(UINT32_C(0x00100010) + (u32)call);
        *xy1 = (long)(UINT32_C(0x00200020) + (u32)call);
        *xy2 = (long)(UINT32_C(0x00300030) + (u32)call);
    }
    if (call == 3)
        C2_SZ3 = 0x0D00u;
    else if (call == 4)
        C2_SZ3 = 0x0E00u;
    else
        C2_SZ3 = 0x0100u;
    s_rtpt_calls++;
    return 0;
}

int RotTransPers(SVECTOR *vertex, int *xy, long *p, long *flag)
{
    (void)vertex;
    *xy = (int)(UINT32_C(0x00400040) + (u32)s_rtps_calls);
    *p = 0;
    *flag = 0;
    C2_IR0 = (s16)(s_rtps_calls == 0 ? 5000 : 0x0700);
    s_rtps_calls++;
    return 0;
}

void PcPort_AddPrimDomainAware(void *ot_pointer, void *prim_pointer)
{
    u32 ot = guest_of(ot_pointer);
    u32 prim = guest_of(prim_pointer);
    u32 old_ot = ld32(ot);
    u32 old_prim = ld32(prim);

    if (s_link_calls < 16) {
        s_link_ot[s_link_calls] = ot;
        s_link_packet[s_link_calls] = prim;
    }
    st32(prim, (old_prim & 0xFF000000u) | (old_ot & 0x00FFFFFFu));
    st32(ot, (old_ot & 0xFF000000u) | (prim & 0x00FFFFFFu));
    s_link_calls++;
}

void PcPort_PrimLinkReset(void) { }
int PcPort_PrimLinkGuestCount(void) { return s_link_calls; }
int PcPort_PrimLinkNativeCount(void) { return 0; }
int PcPort_PrimLinkRejectCount(void) { return 0; }

static void reset_fixture(void)
{
    u32 index;
    MATRIX *camera;

    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(&gteRegs, 0, sizeof(gteRegs));
    s_set_rot_calls = 0;
    s_set_trans_calls = 0;
    s_apply_calls = 0;
    s_rtpt_calls = 0;
    s_rtps_calls = 0;
    s_link_calls = 0;
    memset(s_link_ot, 0, sizeof(s_link_ot));
    memset(s_link_packet, 0, sizeof(s_link_packet));

    st32(CAM_X, 10u << 12u);
    st32(CAM_Z, 20u << 12u);
    st32(WRAP_X, 16u);
    st32(WRAP_Z, 16u);
    camera = (MATRIX*)PSX_ADDR(SCRATCH + 0x28u);
    memset(camera, 0, sizeof(*camera));
    camera->t[0] = 100;
    camera->t[1] = 200;
    camera->t[2] = 300;
    memset(PSX_ADDR(SCRATCH + 0x48u), 0, sizeof(MATRIX));
    for (index = 0u; index < 16u; index++)
        st16(SCRATCH + 0x68u + index * 2u, (u16)(0x500u + index));
    for (index = 0u; index < 6u; index++) {
        s16 x = index == 0u ? (s16)-19990 : (s16)(100 + (s16)index);
        s16 y = (s16)(33 + (s16)index);
        s16 z = index == 0u ? (s16)20020 : (s16)(200 + (s16)index);
        st32(ENTRIES + index * 8u,
             (u32)(u16)x | ((u32)(u16)y << 16u));
        st16(ENTRIES + index * 8u + 4u, (u16)z);
    }
    for (index = 0u; index < 0x400u; index += 4u)
        st32(OT_BASE + index, UINT32_C(0xAA000123));
}

static void run_main_scenario(void)
{
    u8 entries_before[48];
    u32 scratch_guest;

    reset_fixture();
    scratch_guest = guest_of(PSX_ADDR(SCRATCH));
    memcpy(entries_before, PSX_ADDR(ENTRIES), sizeof(entries_before));
    wm_80099BFC(ENTRIES, 6, OT_BASE, PACKETS);

    check("camera-relative-wrap-and-z-sign",
          s_apply_input[0].vx == 12768 && s_apply_input[0].vy == 33 &&
          s_apply_input[0].vz == 12768);
    check("two-stage-matrix-installation",
          s_set_rot_calls == 12 && s_set_trans_calls == 6 &&
          s_set_rot[0] == scratch_guest + 0x28u &&
          s_set_rot[1] == scratch_guest + 0x48u &&
          s_set_trans[0] == scratch_guest + 0x48u &&
          s_model_t[0][0] == 110 && s_model_t[0][1] == 220 &&
          s_model_t[0][2] == 330);
    check("retail-rejection-and-compaction",
          s_rtpt_calls == 6 && s_rtps_calls == 3 && s_link_calls == 3 &&
          s_link_packet[0] == PACKETS &&
          s_link_packet[1] == PACKETS + 0x28u &&
          s_link_packet[2] == PACKETS + 0x50u);
    check("retail-depth-gate-and-buckets",
          s_link_ot[0] == OT_BASE + 0x40u &&
          s_link_ot[1] == OT_BASE + 0x340u &&
          s_link_ot[2] == OT_BASE + 0x40u);
    check("shade-clamp-and-clut-selection",
          ld16(PACKETS + 0x0Eu) == 0x50Fu &&
          ld16(PACKETS + 0x28u + 0x0Eu) == 0x507u);
    check("ft4-xy-and-tag-publication",
          (ld32(PACKETS) & 0xFF000000u) == 0x09000000u &&
          ld32(PACKETS + 0x08u) == 0x00100010u &&
          ld32(PACKETS + 0x20u) == 0x00400040u &&
          (ld32(OT_BASE + 0x40u) & 0x00FFFFFFu) ==
              ((PACKETS + 0x50u) & 0x00FFFFFFu));
    check("output-count-and-input-read-only",
          ld32(COUNT_G) == 3u &&
          memcmp(entries_before, PSX_ADDR(ENTRIES), sizeof(entries_before)) == 0);
}

static void run_ceiling_scenario(void)
{
    reset_fixture();
    st32(COUNT_G, 511u);
    wm_80099BFC(ENTRIES, 2, OT_BASE, PACKETS);
    check("retail-output-ceiling",
          ld32(COUNT_G) == 512u && s_rtpt_calls == 1 && s_link_calls == 1);
}

int main(void)
{
    s_failures = 0;
    run_main_scenario();
    run_ceiling_scenario();
    if (s_failures == 0)
        puts("W34N41 0x80099BFC full-body certificate PASS");
    return s_failures == 0 ? 0 : 1;
}
