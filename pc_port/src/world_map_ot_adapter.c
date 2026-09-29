/*
 * W34B38 — world-map-only OT adapter (see world_map_ot_adapter.h and
 * AUDIT_W34B38_ADAPTER_DESIGN.md for the retail contract citations).
 *
 * Retail contract reproduced:
 *   ClearOTagR (0x80044AD8 wrapper -> driver clear via D_800568C8):
 *     w[i] = (guest addr of entry i-1) & 0xFFFFFF, len byte 0, i=1..n-1
 *     w[0] = D_8005698C & 0xFFFFFF   (link to the null sentinel packet
 *            {0x04FFFFFF,0,0,0,0} at 0x8005698C, 3F290.sdata.s:12074)
 *   DrawOTag(entry): follow low-24 guest links, DMA each len>0 packet,
 *     end at link 0xFFFFFF.
 *
 * PORT SUBSTITUTION (documented): the port does not populate main-exe
 * .sdata inside g_PsxRam, so entry 0 is written as the direct end tag
 * 0x00FFFFFF instead of a link to 0x8005698C; the parser-visible result
 * (nothing drawn, walk terminates) is identical.
 *
 * Host boundary: DrawPrim(void*) parses exactly one primitive from
 * retail-layout packet memory (USE_EXTENDED_PRIM_POINTERS=0 P_TAG is
 * byte-identical to the retail tag word). DrawAllSplits() mirrors
 * DrawOTag's flush. No PsyCross file is modified.
 */
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_ot_adapter.h"

extern void DrawPrim(void *p);
extern void DrawAllSplits(void);

#define WM_OTA_TERM_LINK   0x00FFFFFFu
#define WM_OTA_RAM_LIMIT   0x00200000u
#define WM_OTA_MAX_LEN     32u          /* parser's own invalid threshold */
#define WM_OTA_MAX_STEPS   0x1400u      /* 0x400 buckets + prim headroom */

#if defined(WM_OTA_MUTANT_M1)          /* wrong link mask */
#define WM_OTA_LINK_MASK   0x0000FFFFu
#else
#define WM_OTA_LINK_MASK   0x00FFFFFFu
#endif

static int s_ota_packets;
static int s_ota_steps;
static int s_ota_abort_range;
static int s_ota_abort_align;
static int s_ota_abort_len;
static int s_ota_abort_steps;
static int s_ota_multi_prim;
static u32 s_ota_last_tag;
static u32 s_ota_last_addr;

void wm_ot_reset(void)
{
    s_ota_packets = 0;
    s_ota_steps = 0;
    s_ota_abort_range = 0;
    s_ota_abort_align = 0;
    s_ota_abort_len = 0;
    s_ota_abort_steps = 0;
    s_ota_multi_prim = 0;
    s_ota_last_tag = 0;
    s_ota_last_addr = 0;
}

int wm_ot_get_packets_submitted(void) { return s_ota_packets; }
int wm_ot_get_steps(void) { return s_ota_steps; }
int wm_ot_get_abort_range(void) { return s_ota_abort_range; }
int wm_ot_get_abort_align(void) { return s_ota_abort_align; }
int wm_ot_get_abort_len(void) { return s_ota_abort_len; }
int wm_ot_get_abort_steps(void) { return s_ota_abort_steps; }
int wm_ot_get_multi_prim_packets(void) { return s_ota_multi_prim; }
u32 wm_ot_get_last_tag(void) { return s_ota_last_tag; }
u32 wm_ot_get_last_addr(void) { return s_ota_last_addr; }

static u32 wm_ota_load_u32(u32 address)
{
    u32 value;
    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static void wm_ota_store_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

void wm_ot_clear_r_guest(u32 ot_guest, u32 count)
{
    u32 i;

    if (count == 0u)
        return;
    /* Entry 0: terminator (port substitution for the 0x8005698C link). */
    wm_ota_store_u32(ot_guest, WM_OTA_TERM_LINK);
    for (i = 1u; i < count; i++) {
#if defined(WM_OTA_MUTANT_M3)          /* wrong direction: forward links */
        u32 prev = ot_guest + (i + 1u) * 4u;
#else
        u32 prev = ot_guest + (i - 1u) * 4u;
#endif
        wm_ota_store_u32(ot_guest + i * 4u, prev & 0x00FFFFFFu);
    }
}

/* Expected payload length (words after the tag) for the packet codes the
 * world writers emit; 0 = unknown code, no check. */
static u32 wm_ota_expected_len(u32 code)
{
    switch (code & 0xFCu) {
    case 0x20u: return 4u;   /* POLY_F3 */
    case 0x28u: return 5u;   /* POLY_F4 */
    case 0x24u: return 7u;   /* POLY_FT3 */
    case 0x2Cu: return 9u;   /* POLY_FT4 */
    case 0x30u: return 6u;   /* POLY_G3 */
    case 0x38u: return 8u;   /* POLY_G4 */
    case 0x34u: return 9u;   /* POLY_GT3 */
    case 0x3Cu: return 12u;  /* POLY_GT4 */
    case 0x64u: return 4u;   /* SPRT */
    case 0x74u: return 3u;   /* SPRT_8 */
    case 0x7Cu: return 3u;   /* SPRT_16 */
    case 0x60u: return 3u;   /* TILE */
    default:
        if (code == 0xE1u) return 1u;  /* DR_TPAGE */
        return 0u;
    }
}

static void wm_ota_abort(const char *kind, int *counter, u32 value, u32 at)
{
    (*counter)++;
    fprintf(stderr,
            "[worldmap-ot-adapter] ABORT %s value=0x%08x at=0x%08x "
            "(walk stopped, nothing dereferenced raw)\n",
            kind, value, at);
}

int wm_ot_draw_otag_guest(u32 entry_guest)
{
    u32 cur = entry_guest;
    u32 steps = 0u;
    int complete = 0;

    for (;;) {
        u32 tag = wm_ota_load_u32(cur);
        u32 len = tag >> 24;
        u32 link = tag & WM_OTA_LINK_MASK;

        s_ota_last_tag = tag;
        s_ota_last_addr = cur;

        if (len > WM_OTA_MAX_LEN) {
            wm_ota_abort("len", &s_ota_abort_len, tag, cur);
            break;
        }
        if (len > 0u) {
            u32 word1 = wm_ota_load_u32(cur + 4u);
            u32 code = word1 >> 24;
            u32 expect = wm_ota_expected_len(code);
            if (expect != 0u && expect != len)
                s_ota_multi_prim++;
            DrawPrim(PSX_ADDR(cur));
            s_ota_packets++;
        }
#if defined(WM_OTA_MUTANT_M2)          /* wrong terminator check */
        if (link == 0x00FFFFFEu) {
#else
        if (link == (WM_OTA_TERM_LINK & WM_OTA_LINK_MASK)) {
#endif
            complete = 1;
            break;
        }
#if !defined(WM_OTA_MUTANT_M4)         /* M4: missing bounds check */
        if (link >= WM_OTA_RAM_LIMIT) {
            wm_ota_abort("range", &s_ota_abort_range, link, cur);
            break;
        }
#endif
        if ((link & 3u) != 0u) {
            wm_ota_abort("align", &s_ota_abort_align, link, cur);
            break;
        }
        steps++;
        s_ota_steps++;
        if (steps > WM_OTA_MAX_STEPS) {
            wm_ota_abort("steps", &s_ota_abort_steps, steps, cur);
            break;
        }
        cur = 0x80000000u | link;
    }

    DrawAllSplits();
    return complete;
}
