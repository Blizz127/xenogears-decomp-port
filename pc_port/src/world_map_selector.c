/*
 * World-map selector producer module.
 *
 * Extracted from world_map_init.c — the one authoritative implementation
 * of the selector binning logic at retail 0x80071B9C.
 *
 * Both the production game build and the production-linked test link
 * this same object.  Do NOT duplicate these functions elsewhere.
 *
 * Provenance: exact transcription of retail selector writer.
 * Corrections vs. earlier inline version:
 *   1. Seed: zero-extended u16 (retail lhu), not signed s16.
 *   2. Thresholds: zero-extended u16 (retail lhu), not signed cast.
 *   3. Loop: index increments in delay slot on every iteration including
 *      the terminating comparison (retail 0x80071BDC delay slot).
 *
 * Valid seeds 0x0000–0xFFFE produce selectors 0–8.
 * Raw seed 0xFFFF is a retail malformed edge (unbounded threshold scan);
 * the defensive backstop logs and preserves the existing C610 value.
 */
#include <stdio.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_selector.h"

/* Retail layout constants (absolute PSX addresses). */
#define WM_THRESH_TABLE_ABS      0x8009B564u

#define WM_U32(a) (*(u32*)PSX_ADDR(a))
#define WM_U16(a) (*(u16*)PSX_ADDR(a))

/* Per-world-init instrumentation counters. */
static int s_sel_entries;
static int s_sel_valid_writes;
static int s_sel_nowrite_ge8;
static int s_sel_malformed_seeds;
static int s_sel_last_selector;

/* ---- Counter accessors ---- */

int wm_selector_get_entries(void)        { return s_sel_entries; }
int wm_selector_get_valid_writes(void)   { return s_sel_valid_writes; }
int wm_selector_get_nowrite_ge8(void)    { return s_sel_nowrite_ge8; }
int wm_selector_get_malformed_seeds(void){ return s_sel_malformed_seeds; }
int wm_selector_get_last_selector(void)  { return s_sel_last_selector; }

void wm_selector_reset(void)
{
    s_sel_entries = 0;
    s_sel_valid_writes = 0;
    s_sel_nowrite_ge8 = 0;
    s_sel_malformed_seeds = 0;
    s_sel_last_selector = 0;
}

/* ---- Selector producer ---- */

/*
 * wm_selector_producer — corrected retail 0x80071B9C selector binning.
 *
 * Instruction-equivalent logical order for valid seeds 0x0000–0xFFFE:
 *
 *   index = 0
 *   repeat:
 *     threshold = zero_extend_u16(table[index])
 *     stop = zero_extend_u16(seed) < threshold
 *     index = index + 1
 *     if stop: break
 *   selector = index - 1
 *
 * When entrance < 8 and seed is well-formed:
 *   *(uint32_t *)0x8009C610 = selector
 *
 * Returns the incremented index (a0) for record-table addressing:
 *   entrance < 8: a0 (1–9); caller uses (a0 << 3) for record offset.
 *   entrance >= 8: entrance (passthrough); caller uses (entrance << 3).
 *   malformed 0xFFFF: returns 0 (no valid record index).
 *
 * Entrance >= 8: no write; world-entry BSS clear supplies zero.
 * Seed 0xFFFF: defensive backstop (retail malformed edge).
 */
u32 wm_selector_producer(u32 entrance, u32 seed)
{
    u32 a0;
    u8* pV1;

    s_sel_entries++;
    fprintf(stderr,
            "[selector-producer] entry entrance=%u seed=0x%04x\n",
            entrance, seed);

    if ((s32)entrance < 8) {
        /* Defensive backstop for malformed raw seed 0xFFFF.
         *
         * 0xFFFF does not satisfy the terminal threshold test
         * (65535 < 65535 is false), so retail would continue scanning
         * adjacent memory without a bound. This path is proven
         * unreachable by valid routed producers (GameState +0x1930
         * is written as a u16 from field transition, max 0xFFFE).
         *
         * Checked before the loop to prevent host memory access
         * beyond the threshold table. When hit: log diagnostic,
         * do not write a fabricated selector, preserve the existing
         * C610 value established by lifecycle init. Return 0 (no
         * valid record index). */
        if (seed == 0xFFFFu) {
            s_sel_malformed_seeds++;
            fprintf(stderr,
                    "[selector-producer] WARNING: malformed raw seed 0xFFFF "
                    "(retail unbounded threshold scan). "
                    "Preserving existing C610=0x%08x. "
                    "Diagnostic count=%d\n",
                    WM_U32(WM_SLOT_C610_ABS), s_sel_malformed_seeds);
            return 0;
        }

        /* v1 = 0x8009B564; first threshold load from +2 (0x8009B566).
         *
         * Retail asm (0x80071BA8–0x80071BDC):
         *   lui  v0, 0x800A
         *   lhu  v0, -0x4A9A(v0)     ; load threshold[0] from 0x8009B566
         *   lui  v1, 0x800A
         *   addiu v1, v1, -0x4A9C    ; v1 = 0x8009B564
         *   slt  v0, a1, v0          ; seed < threshold[0]?
         *   bnez v0, store            ; yes → index=1, store
         *   addiu a0, zero, 1         ; (delay slot) a0 = 1
         *   [fallthrough: loop]
         *
         * Loop (0x80071BC4–0x80071BDC):
         *   addiu v1, v1, 2           ; (A) advance pointer
         *   addiu v1, v1, 2           ; (B) advance pointer (total +4/iter)
         *   lhu   v0, (v1)            ; load next threshold
         *   nop
         *   slt   v0, a1, v0          ; seed < threshold?
         *   beqz  v0, (B)             ; no → loop (branch to 0x80071BC8)
         *   addiu a0, a0, 1           ; (delay slot) a0++ BEFORE branch
         *
         * On every iteration including the terminating one, the delay slot
         * increments a0. When beqz falls through (seed < threshold), a0
         * has already been incremented by the terminating comparison's
         * delay slot. Then:
         *   sll  v0, a0, 3
         *   ...
         *   addiu v0, a0, -1          ; selector = a0 - 1
         *   sw   v0, -0x39F0(at)      ; store to 0x8009C610
         */
        pV1 = (u8*)PSX_ADDR(WM_THRESH_TABLE_ABS);
        {
            /* First threshold: *(u16*)(pV1 + 2) at 0x8009B566 */
            u16 thr = *(u16*)(pV1 + 2);
            a0 = 1;
            if ((u32)seed < (u32)thr) {
                /* seed < first threshold → selector 0. a0=1 already set. */
            } else {
                /* Fallthrough: advance pointer past first threshold. */
                pV1 += 2;
                for (;;) {
                    /* Advance to next threshold entry (+2 each half of loop). */
                    pV1 += 2;
                    thr = *(u16*)pV1;
                    /* Delay-slot increment: a0++ on EVERY iteration,
                     * including the terminating comparison. */
                    a0++;
                    if ((u32)seed < (u32)thr)
                        break;
                }
            }
        }

        /* Store selector: a0 - 1 as u32 to 0x8009C610.
         * Retail: 0x80071BF0 addiu v0, a0, -1
         *         0x80071BF8 sw    v0, -0x39F0(at) */
        s_sel_last_selector = (int)(a0 - 1);
        s_sel_valid_writes++;
        WM_U32(WM_SLOT_C610_ABS) = a0 - 1;
        fprintf(stderr,
                "[selector-producer] write C610=%u (a0=%u)\n",
                a0 - 1, a0);
        return a0;
    } else {
        /* Entrance >= 8: no write. World-entry BSS clear is authoritative.
         * Retail: 0x80071C04–0x80071C0C — no store to C610. */
        s_sel_nowrite_ge8++;
        fprintf(stderr,
                "[selector-producer] entrance %u >= 8; no C610 write\n",
                entrance);
        return entrance;
    }
}
