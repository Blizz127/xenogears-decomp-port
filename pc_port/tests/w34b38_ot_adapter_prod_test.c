/* W34B38 — production-linked test for the world-map guest-OT adapter.
 * Links the real pc_port/src/world_map_ot_adapter.c against a recording
 * DrawPrim/DrawAllSplits harness and a test-owned g_PsxRam. */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_ot_adapter.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];

#define MAX_REC 64
static u32 s_rec[MAX_REC];
static int s_rec_count;
static int s_flushes;

void DrawPrim(void *p)
{
    if (s_rec_count < MAX_REC)
        s_rec[s_rec_count] = (u32)((uint8_t *)p - g_PsxRam);
    s_rec_count++;
}

void DrawAllSplits(void) { s_flushes++; }

static void st32(u32 guest, u32 value) { memcpy(PSX_ADDR(guest), &value, 4); }
static u32 ld32(u32 guest) { u32 v; memcpy(&v, PSX_ADDR(guest), 4); return v; }

#define OT_ROOT 0x80100000u
#define OT_N    0x400u

static void reset_all(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    s_rec_count = 0;
    s_flushes = 0;
    wm_ot_reset();
    wm_ot_clear_r_guest(OT_ROOT, OT_N);
}

/* addPrim-equivalent, retail order (925A0 lines 198-216). */
static void add_prim(u32 bucket_idx, u32 prim_guest, u32 len, u32 code)
{
    u32 bucket = OT_ROOT + bucket_idx * 4u;
    u32 old = ld32(bucket);
    st32(prim_guest, (len << 24) | (old & 0x00FFFFFFu));
    st32(prim_guest + 4u, code << 24);
    st32(bucket, (old & 0xFF000000u) | (prim_guest & 0x00FFFFFFu));
}

static int s_pass;
static int s_total;

static void check(const char *name, int cond)
{
    s_total++;
    if (cond) {
        s_pass++;
        printf("PASS %s\n", name);
    } else {
        printf("FAIL %s\n", name);
    }
}

int main(void)
{
    /* 1: clear layout + empty walk */
    reset_all();
    {
        int ok = ld32(OT_ROOT) == 0x00FFFFFFu &&
                 ld32(OT_ROOT + 4u) == (OT_ROOT & 0x00FFFFFFu) &&
                 ld32(OT_ROOT + 0xFFCu) ==
                     ((OT_ROOT + 0xFF8u) & 0x00FFFFFFu);
        int complete = wm_ot_draw_otag_guest(OT_ROOT + 0xFFCu);
        check("clear-layout+empty-walk",
              ok && complete == 1 && wm_ot_get_packets_submitted() == 0 &&
              wm_ot_get_steps() == 0x3FF && s_flushes == 1 &&
              s_rec_count == 0);
    }

    /* 2: single bucket, single prim */
    reset_all();
    add_prim(0x200u, 0x80110000u, 9u, 0x2Cu);
    {
        int complete = wm_ot_draw_otag_guest(OT_ROOT + 0xFFCu);
        check("single-prim",
              complete == 1 && wm_ot_get_packets_submitted() == 1 &&
              s_rec_count == 1 && s_rec[0] == 0x110000u &&
              wm_ot_get_multi_prim_packets() == 0);
    }

    /* 3: multi-bucket order (walk = descending bucket index, LIFO in bucket) */
    reset_all();
    add_prim(0x300u, 0x80110000u, 9u, 0x2Cu);
    add_prim(0x200u, 0x80111000u, 5u, 0x28u);
    add_prim(0x200u, 0x80111100u, 5u, 0x28u);
    add_prim(0x100u, 0x80111200u, 1u, 0xE1u);
    {
        int complete = wm_ot_draw_otag_guest(OT_ROOT + 0xFFCu);
        check("multi-bucket-order",
              complete == 1 && wm_ot_get_packets_submitted() == 4 &&
              s_rec_count == 4 && s_rec[0] == 0x110000u &&
              s_rec[1] == 0x111100u && s_rec[2] == 0x111000u &&
              s_rec[3] == 0x111200u &&
              wm_ot_get_multi_prim_packets() == 0);
    }

    /* 4: out-of-range link aborts before any raw deref */
    reset_all();
    st32(OT_ROOT + 0x300u * 4u, (0u << 24) | 0x00300000u);
    {
        int complete = wm_ot_draw_otag_guest(OT_ROOT + 0xFFCu);
        check("range-abort",
              complete == 0 && wm_ot_get_abort_range() == 1 &&
              wm_ot_get_packets_submitted() == 0);
    }

    /* 5: misaligned link aborts */
    reset_all();
    st32(OT_ROOT + 0x300u * 4u, (0u << 24) | 0x00110001u);
    {
        int complete = wm_ot_draw_otag_guest(OT_ROOT + 0xFFCu);
        check("align-abort",
              complete == 0 && wm_ot_get_abort_align() == 1);
    }

    /* 6: oversized len aborts */
    reset_all();
    add_prim(0x200u, 0x80110000u, 33u, 0x2Cu);
    {
        int complete = wm_ot_draw_otag_guest(OT_ROOT + 0xFFCu);
        check("len-abort",
              complete == 0 && wm_ot_get_abort_len() == 1 &&
              wm_ot_get_packets_submitted() == 0);
    }

    /* 7: link cycle hits the step cap */
    reset_all();
    st32(0x80110000u, (0u << 24) | (0x80110000u & 0x00FFFFFFu));
    st32(OT_ROOT + 0x300u * 4u, 0x80110000u & 0x00FFFFFFu);
    {
        int complete = wm_ot_draw_otag_guest(OT_ROOT + 0xFFCu);
        check("cycle-abort",
              complete == 0 && wm_ot_get_abort_steps() == 1);
    }

    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return (s_pass == s_total) ? 0 : 1;
}
