/* W34B36 production-linked certificate for the frame tail wrapper. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_frame_tail_71984.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define GEOM 0x8009BE0Cu
#define ENV  0x800A1000u
#define OT   0x800A2000u
#define BE3C 0x8009BE3Cu
#define D554 0x8009D554u

static int s_pass;
static int s_fail;
static int s_total;
static int s_geom_calls;
static int s_last_ofx;
static int s_last_ofy;
static int s_draw_calls;

static void check_case(const char *name, const char *assertion, int ok)
{
    s_total++;
    if (ok) {
        s_pass++;
    } else {
        s_fail++;
        printf("FAIL [%s]: ASSERTION %s\n", name, assertion);
    }
}

static void store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void reset_fixture(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    s_geom_calls = 0;
    s_last_ofx = 0;
    s_last_ofy = 0;
    s_draw_calls = 0;
    store_u32(GEOM, 140u);
    store_u32(BE3C, ENV);
    store_u32(ENV + 0x70u, OT);
    store_u32(D554, 1u);
    wm_71984_tail_reset();
}

static void run_known_and_no_backedge(void)
{
    reset_fixture();
    check_case("known-tail", "geom-map-draw-and-hold-backedge",
               wm_80071984_tail() == 1 && s_geom_calls == 1 &&
               s_last_ofx == 160 && s_last_ofy == 140 &&
               wm_71984_tail_get_draw_calls() == 1 &&
               wm_71984_tail_get_backedge_hits() == 1 &&
               wm_71984_tail_get_unknowns() == 0);

    reset_fixture();
    store_u32(D554, 0u);
    store_u32(GEOM, 0xFFFFFFF0u);
    check_case("no-backedge", "zero-d554-returns-after-draw",
               wm_80071984_tail() == 0 && s_geom_calls == 1 &&
               s_last_ofy == -16 && wm_71984_tail_get_draw_calls() == 1 &&
               wm_71984_tail_get_backedge_hits() == 0);
}

static void run_unknown_pointer_paths(void)
{
    reset_fixture();
    store_u32(ENV + 0x70u, 0x12345678u);
    check_case("unknown-ot", "log-and-skip-drawotag",
               wm_80071984_tail() == 1 && s_geom_calls == 1 &&
               s_draw_calls == 0 && wm_71984_tail_get_unknowns() == 1 &&
               wm_71984_tail_get_backedge_hits() == 1);

    reset_fixture();
    store_u32(BE3C, 0x12345678u);
    check_case("unknown-env", "log-and-skip-env-dereference",
               wm_80071984_tail() == 1 && s_geom_calls == 1 &&
               s_draw_calls == 0 && wm_71984_tail_get_unknowns() == 1 &&
               wm_71984_tail_get_backedge_hits() == 1);
}

void SetGeomOffset(int ofx, int ofy)
{
    s_geom_calls++;
    s_last_ofx = ofx;
    s_last_ofy = ofy;
}

void DrawOTag(unsigned long *ot)
{
    (void)ot;
    s_draw_calls++;
}
/* Headless link-only: packet walk validated via OT memory, not GL. */
void DrawPrim(void *p) { (void)p; }
void DrawAllSplits(void) {}

int main(void)
{
    printf("=== W34B36 0x80071984 frame tail ===\n");
    printf("RETAIL_BOUNDARY [0x80071994,0x800719CC) tail/backedge\n");
    run_known_and_no_backedge();
    run_unknown_pointer_paths();
    printf("=== Results: %d/%d PASS ===\n", s_pass, s_total);
    return s_fail == 0 ? 0 : 1;
}
