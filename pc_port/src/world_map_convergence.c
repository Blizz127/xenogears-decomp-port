/*
 * World-map convergence P1/P2 module (W34B1 + W34B3).
 *
 * Extracted from world_map_init.c — the one authoritative implementation
 * of wm_800726C0_convergence_p1, wm_8007272C_convergence_p2, and
 * wm_pool_register.
 *
 * Both the production game build and the production-linked test link
 * this same object.  Do NOT duplicate these functions elsewhere.
 *
 * P1 retail slice: 0x800726C0–0x80072728.
 * P2 retail slice: 0x8007272C–0x80072780.
 * Provenance: exact transcription of retail dispatch + table loops.
 */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_convergence.h"

/* Retail layout constants (absolute PSX addresses). */
#define WM_FLAG_C894_ABS         0x8009C894u
#define WM_CONV_TABLE_A_BASE     0x80099E8Cu
#define WM_CONV_TABLE_B_BASE     0x8009A034u
#define WM_SLOT_C610_ABS         0x8009C610u
#define WM_POOL_BE24             0x8009BE24u
#define WM_POOL_SLOT_COUNT       64
#define WM_POOL_SLOT_STRIDE      0x80u
#define WM_POOL_OFF_18           0x18u
#define WM_POOL_OFF_1C           0x1Cu

#define WM_U32(a) (*(u32*)PSX_ADDR(a))

/* Resolve a 32-bit guest value to a host pointer. */
static void* psx_u32_to_host(u32 p)
{
    if (p == 0)
        return NULL;
    if (p >= 0x80000000u && p < 0x80200000u)
        return PSX_ADDR(p);
    return (void*)(uintptr_t)p;
}

/* Per-world-init instrumentation counters.
 * All reset at each new world init boundary via wm_conv_p1_reset(). */
static int s_wm_conv_p1_entry;
static int s_wm_conv_p1_flag0;
static int s_wm_conv_p1_flag1_cut;
static int s_wm_conv_p1_other_cut;
static int s_wm_conv_p1_empty;
static int s_wm_conv_p1_iterations;
static int s_wm_conv_p1_helper_calls;
static int s_wm_conv_p1_cut_second_table;
static int s_wm_conv_p1_forbidden_c610_read;
static int s_wm_conv_p1_forbidden_a034_read;
static int s_wm_conv_p1_forbidden_976fc;
static int s_wm_conv_p1_forbidden_common_tail;
static int s_wm_conv_p1_forbidden_excluded_instr;
static u32  s_wm_conv_p1_last_next;
static int s_wm_pool_alloc;

/* P2 instrumentation counters. */
static int s_wm_conv_p2_entry;
static int s_wm_conv_p2_empty_stream;
static int s_wm_conv_p2_iterations;
static int s_wm_conv_p2_helper_calls;
static int s_wm_conv_p2_forbidden_c894;
static int s_wm_conv_p2_forbidden_976fc;
static int s_wm_conv_p2_forbidden_common_tail;
static int s_wm_conv_p2_forbidden_excluded_instr;
static u32  s_wm_conv_p2_selector;
static u32  s_wm_conv_p2_selected_ptr;

/* ---- Counter accessors ---- */

int  wm_conv_p1_get_entry(void)                 { return s_wm_conv_p1_entry; }
int  wm_conv_p1_get_flag0(void)                  { return s_wm_conv_p1_flag0; }
int  wm_conv_p1_get_flag1_cut(void)              { return s_wm_conv_p1_flag1_cut; }
int  wm_conv_p1_get_other_cut(void)              { return s_wm_conv_p1_other_cut; }
int  wm_conv_p1_get_empty(void)                  { return s_wm_conv_p1_empty; }
int  wm_conv_p1_get_iterations(void)             { return s_wm_conv_p1_iterations; }
int  wm_conv_p1_get_helper_calls(void)           { return s_wm_conv_p1_helper_calls; }
int  wm_conv_p1_get_cut_second_table(void)       { return s_wm_conv_p1_cut_second_table; }
u32  wm_conv_p1_get_last_next(void)              { return s_wm_conv_p1_last_next; }
int  wm_conv_p1_get_forbidden_c610_read(void)    { return s_wm_conv_p1_forbidden_c610_read; }
int  wm_conv_p1_get_forbidden_a034_read(void)    { return s_wm_conv_p1_forbidden_a034_read; }
int  wm_conv_p1_get_forbidden_976fc(void)        { return s_wm_conv_p1_forbidden_976fc; }
int  wm_conv_p1_get_forbidden_common_tail(void)  { return s_wm_conv_p1_forbidden_common_tail; }
int  wm_conv_p1_get_forbidden_excluded_instr(void) { return s_wm_conv_p1_forbidden_excluded_instr; }
int  wm_conv_p1_get_pool_alloc(void)             { return s_wm_pool_alloc; }

/* P2 counter accessors. */
int  wm_conv_p2_get_entry(void)                    { return s_wm_conv_p2_entry; }
int  wm_conv_p2_get_empty_stream(void)             { return s_wm_conv_p2_empty_stream; }
int  wm_conv_p2_get_iterations(void)               { return s_wm_conv_p2_iterations; }
int  wm_conv_p2_get_helper_calls(void)             { return s_wm_conv_p2_helper_calls; }
int  wm_conv_p2_get_forbidden_c894(void)           { return s_wm_conv_p2_forbidden_c894; }
int  wm_conv_p2_get_forbidden_976fc(void)          { return s_wm_conv_p2_forbidden_976fc; }
int  wm_conv_p2_get_forbidden_common_tail(void)    { return s_wm_conv_p2_forbidden_common_tail; }
int  wm_conv_p2_get_forbidden_excluded_instr(void) { return s_wm_conv_p2_forbidden_excluded_instr; }
u32  wm_conv_p2_get_selector(void)                 { return s_wm_conv_p2_selector; }
u32  wm_conv_p2_get_selected_ptr(void)             { return s_wm_conv_p2_selected_ptr; }

/* ---- Counter reset ---- */

void wm_conv_p1_reset(void)
{
    s_wm_conv_p1_entry = 0;
    s_wm_conv_p1_flag0 = 0;
    s_wm_conv_p1_flag1_cut = 0;
    s_wm_conv_p1_other_cut = 0;
    s_wm_conv_p1_empty = 0;
    s_wm_conv_p1_iterations = 0;
    s_wm_conv_p1_helper_calls = 0;
    s_wm_conv_p1_cut_second_table = 0;
    s_wm_conv_p1_forbidden_c610_read = 0;
    s_wm_conv_p1_forbidden_a034_read = 0;
    s_wm_conv_p1_forbidden_976fc = 0;
    s_wm_conv_p1_forbidden_common_tail = 0;
    s_wm_conv_p1_forbidden_excluded_instr = 0;
    s_wm_conv_p1_last_next = 0;
    s_wm_pool_alloc = 0;

    s_wm_conv_p2_entry = 0;
    s_wm_conv_p2_empty_stream = 0;
    s_wm_conv_p2_iterations = 0;
    s_wm_conv_p2_helper_calls = 0;
    s_wm_conv_p2_forbidden_c894 = 0;
    s_wm_conv_p2_forbidden_976fc = 0;
    s_wm_conv_p2_forbidden_common_tail = 0;
    s_wm_conv_p2_forbidden_excluded_instr = 0;
    s_wm_conv_p2_selector = 0;
    s_wm_conv_p2_selected_ptr = 0;
}

/* ---- Pool registration helper ---- */

/* Native transcription of retail 0x80097718.
 * Pool registration: searches WM_POOL_BE24 for a free slot (occupancy at
 * +0x1C == 0), zeroes status halfwords, stores a0 at +0x18 and a1 at +0x1C.
 * Bounded to 64 iterations; silent no-op if pool full.
 * Leaf — no callees, no stack frame. */
void wm_pool_register(u32 a0, u32 a1)
{
    u32 pool_psx = WM_U32(WM_POOL_BE24);
    u8* base;
    int i;

    fprintf(stderr, "[worldmap-pool-register] entry a0=0x%08x a1=0x%08x\n",
            a0, a1);

    if (pool_psx == 0)
        return;
    base = (u8*)psx_u32_to_host(pool_psx);
    if (base == NULL)
        return;

    for (i = 0; i < WM_POOL_SLOT_COUNT; i++) {
        u8* slot = base + (u32)i * WM_POOL_SLOT_STRIDE;
        u32 occupancy = *(u32*)(slot + WM_POOL_OFF_1C);
        if (occupancy == 0) {
            *(u16*)(slot + 0x00) = 0;
            *(u16*)(slot + 0x02) = 0;
            *(u16*)(slot + 0x04) = 0;
            *(u32*)(slot + WM_POOL_OFF_18) = a0;
            *(u32*)(slot + WM_POOL_OFF_1C) = a1;
            *(u16*)(slot + 0x20) = 0;
            *(u16*)(slot + 0x22) = 0;
            s_wm_pool_alloc++;
            fprintf(stderr, "[worldmap-pool-register] "
                    "slot=%d inserted a0=0x%08x a1=0x%08x\n",
                    i, a0, a1);
            return;
        }
    }
    /* Pool full — retail silently drops. */
    fprintf(stderr, "[worldmap-pool-register] "
            "no free slot (dropped)\n");
}

/* ---- Convergence P1 (production) ---- */

/* W34B1: first convergence caller slice 0x800726C0–0x80072728.
 * Exact transcription of retail dispatch + first-table loop.
 * Provenance-preserving name wm_800726C0_convergence_p1.
 * Never reads 0x8009C610 / 0x8009A034, never executes 0x8007272C+.
 * Returns the retail PC where control would continue (cut). */
wm_conv_p1_next_t wm_800726C0_convergence_p1(void)
{
    u32 flag;

    s_wm_conv_p1_entry++;
    flag = WM_U32(WM_FLAG_C894_ABS);
    fprintf(stderr,
            "[worldmap-convergence-p1] entry flag=0x%08x\n",
            flag);

    if (flag == 1) {
        s_wm_conv_p1_flag1_cut++;
        s_wm_conv_p1_last_next = WM_CONV_P1_CUT_FLAG1_ARC;
        fprintf(stderr,
                "[worldmap-convergence-p1] flag==1 cut retail_pc=0x%08x\n",
                WM_CONV_P1_CUT_FLAG1_ARC);
        return WM_CONV_P1_CUT_FLAG1_ARC;
    }
    if (flag != 0) {
        s_wm_conv_p1_other_cut++;
        s_wm_conv_p1_last_next = WM_CONV_P1_CUT_COMMON_TAIL;
        fprintf(stderr,
                "[worldmap-convergence-p1] flag other (0x%08x) cut retail_pc=0x%08x\n",
                flag, WM_CONV_P1_CUT_COMMON_TAIL);
        return WM_CONV_P1_CUT_COMMON_TAIL;
    }

    /* flag == 0 : first-table path. */
    s_wm_conv_p1_flag0++;

    {
        u32 *base = (u32*)PSX_ADDR(WM_CONV_TABLE_A_BASE);
        if (*base == 0) {
            s_wm_conv_p1_empty++;
            s_wm_conv_p1_cut_second_table++;
            s_wm_conv_p1_last_next = WM_CONV_P1_CUT_SECOND_TABLE;
            fprintf(stderr,
                    "[worldmap-convergence-p1] empty table cut retail_pc=0x%08x\n",
                    WM_CONV_P1_CUT_SECOND_TABLE);
            return WM_CONV_P1_CUT_SECOND_TABLE;
        }

        {
            u32 *s0 = base;
            do {
                u32 a0 = s0[0];
                u32 a1 = s0[1];
                s_wm_conv_p1_iterations++;
                s_wm_conv_p1_helper_calls++;
                fprintf(stderr,
                        "[worldmap-convergence-p1] iter a0=0x%08x a1=0x%08x (helper @0x80072714)\n",
                        a0, a1);
                wm_pool_register(a0, a1);
                s0 += 2; /* 8-byte stride = 2×u32 */
            } while (*s0 != 0);
        }

        s_wm_conv_p1_cut_second_table++;
        s_wm_conv_p1_last_next = WM_CONV_P1_CUT_SECOND_TABLE;
        fprintf(stderr,
                "[worldmap-convergence-p1] cut retail_pc=0x%08x iterations=%d\n",
                WM_CONV_P1_CUT_SECOND_TABLE, s_wm_conv_p1_iterations);
        return WM_CONV_P1_CUT_SECOND_TABLE;
    }
}

/* ---- Convergence P2 (production) ---- */

/* W34B3: second convergence caller slice 0x8007272C–0x80072780.
 * Reads selector 0x8009C610, indexes the nine-pointer table at
 * 0x8009A034, walks the selected {u32 a0, u32 a1} record stream,
 * and calls wm_pool_register for each record.
 * Returns 0x8007290C for both empty and completed streams.
 * Never executes 0x80072784, 0x800976FC, or the common tail. */
u32 wm_8007272C_convergence_p2(void)
{
    u32 selector;
    u32 selected_psx;
    u32 *record;

    s_wm_conv_p2_entry++;
    fprintf(stderr, "[worldmap-convergence-p2] entry\n");

    /* Read selector. */
    selector = WM_U32(WM_SLOT_C610_ABS);
    s_wm_conv_p2_selector = selector;
    fprintf(stderr, "[worldmap-convergence-p2] selector=%u\n", selector);

    /* Index top-level table: selected = *(u32*)(0x8009A034 + (selector << 2)). */
    selected_psx = WM_U32(WM_CONV_TABLE_B_BASE + (selector << 2));
    s_wm_conv_p2_selected_ptr = selected_psx;
    fprintf(stderr, "[worldmap-convergence-p2] selected_psx=0x%08x\n",
            selected_psx);

    /* Resolve to host pointer. */
    record = (u32*)PSX_ADDR(selected_psx);

    /* Pre-test first record a0 for zero (empty stream). */
    if (record[0] == 0) {
        s_wm_conv_p2_empty_stream++;
        fprintf(stderr,
                "[worldmap-convergence-p2] empty stream, cut retail_pc=0x%08x\n",
                WM_CONV_P1_CUT_COMMON_TAIL);
        return WM_CONV_P1_CUT_COMMON_TAIL;
    }

    /* Walk record stream: {u32 a0, u32 a1}, stride 8.
     * jal delay-slot pointer increment preserved. */
    do {
        u32 a0 = record[0];
        u32 a1 = record[1];

        s_wm_conv_p2_iterations++;
        s_wm_conv_p2_helper_calls++;
        fprintf(stderr,
                "[worldmap-convergence-p2] iter a0=0x%08x a1=0x%08x (helper)\n",
                a0, a1);

        wm_pool_register(a0, a1);

        record += 2; /* 8-byte stride = 2×u32, jal delay slot */
    } while (record[0] != 0);

    fprintf(stderr,
            "[worldmap-convergence-p2] done iterations=%d cut retail_pc=0x%08x\n",
            s_wm_conv_p2_iterations, WM_CONV_P1_CUT_COMMON_TAIL);
    return WM_CONV_P1_CUT_COMMON_TAIL;
}

/* Retail 800976FC: re-arm a saved slot without changing its update callback
 * or saved movement state. The initializer recreates its freed sprite. */
static void wm_resume_slot(u32 callback, u32 index)
{
    u8* slot = (u8*)PSX_ADDR(WM_U32(0x8009BE24u) + index * 128u);
    *(u16*)slot = 0;
    *(u32*)(slot + 0x18u) = callback;
}

/* Retail 80072784..80072908, C894 == 1 convergence branch. */
void wm_80072784_convergence_resume(void)
{
    static const u32 callbacks[] = {
        0x800923A8u, 0x8008A52Cu, 0x8008B498u, 0x8008BD1Cu,
        0x8008C6ECu, 0x8008D520u, 0x8008DE9Cu, 0x8008E4F4u,
        0x800907C4u, 0x80092BE4u, 0x80092DF8u, 0x80071A50u
    };
    static const u32 slots[] = {0u,1u,2u,3u,4u,5u,6u,7u,8u,12u,13u,14u};
    u32 selector;
    u32 i;
    for (i = 0; i < 12u; ++i)
        wm_resume_slot(callbacks[i], slots[i]);
    selector = WM_U32(0x8009C610u);
    if (selector >= 3u && selector <= 7u) {
        wm_resume_slot(0x80087F60u, 15u);
        wm_resume_slot(0x8008868Cu, 16u);
    }
    if (selector >= 4u && selector <= 8u)
        wm_resume_slot(0x800879E0u, 17u);
    if (selector == 4u)
        wm_resume_slot(0x80088C90u, 19u);
}
