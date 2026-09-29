/*
 * World-map common-tail prefix + 0x80089160 initializer (W34B5A).
 *
 * Common-tail prefix: 0x8007290C–0x80072938.
 * Selector-dependent logic: C610=0 calls wm_80089160(14,0,0).
 * Reconvergence at 0x8007293C (first excluded = next-phase helper).
 *
 * 0x80089160: bounded table/state initializer [0x80089160,0x800893E0).
 * Leaf function, no direct calls. Record stride 672 bytes.
 * Sets bit 0x80 at record+0x4F, initializes 8 sub-records (stride 0x54).
 */
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"

#define WM_U8(a)  (*(u8*)PSX_ADDR(a))
#define WM_U16(a) (*(u16*)PSX_ADDR(a))
#define WM_S16(a) (*(s16*)PSX_ADDR(a))
#define WM_U32(a) (*(u32*)PSX_ADDR(a))

/* PsyQ GPU helpers (linked from PsyCross in the full port). */
extern u_short GetTPage(int tp, int abr, int x, int y);
extern u_short GetClut(int x, int y);
extern u8 D_80059179;

/* ---- PSX unaligned load/store helpers (little-endian) ----
 *
 * Exact byte-level emulation of MIPS lwl/lwr/swl/swr for all alignments.
 * The retail binary uses these for 8-byte copies in the record init loop. */

static inline u32 psx_lwl(u32 addr)
{
    /* lwl loads bytes from aligned base up to addr into the HIGH portion
     * of rt.  MIPS R3000A little-endian: loaded bytes occupy the high
     * register positions in NORMAL order (lowest address → lowest
     * replaced position).  Combined with lwr(addr) via OR, the pair
     * produces the correct little-endian word. */
    unsigned off = addr & 3;
    u8 *base = (u8*)PSX_ADDR(addr - off);
    u32 result = 0;
    for (unsigned j = 0; j <= off; j++)
        result |= (u32)base[j] << ((3 - off + j) * 8);
    return result;
}

static inline u32 psx_lwr(u32 addr)
{
    /* lwr loads bytes from addr through end of aligned word.
     * MIPS R3000A little-endian: loaded bytes occupy the LOW register
     * positions starting at 0.  When paired with lwl(addr+3), the OR
     * merge produces the correct full word with no bit conflicts. */
    unsigned off = addr & 3;
    u8 *base = (u8*)PSX_ADDR(addr - off);
    u32 result = 0;
    for (unsigned j = off; j < 4; j++)
        result |= (u32)base[j] << ((j - off) * 8);
    return result;
}

static inline u32 psx_lwl_lwr(u32 lwl_addr, u32 lwr_addr)
{
    return psx_lwl(lwl_addr) | psx_lwr(lwr_addr);
}

static inline void psx_swl(u32 addr, u32 val)
{
    /* swl stores the HIGH bytes of val into memory from the aligned
     * boundary up to addr.  MIPS R3000A little-endian: stores in the
     * same normal order as lwl loads.  swl(addr+3)|swr(addr) stores
     * the full word in correct little-endian memory order. */
    unsigned off = addr & 3;
    u8 *base = (u8*)PSX_ADDR(addr - off);
    for (unsigned j = 0; j <= off; j++)
        base[j] = (u8)(val >> ((3 - off + j) * 8));
}

static inline void psx_swr(u32 addr, u32 val)
{
    /* swr stores bytes from addr through end of aligned word.
     * MIPS R3000A little-endian: val byte (j-off) goes to position j.
     * swl(addr+3)|swr(addr) stores the full word in memory-byte order. */
    unsigned off = addr & 3;
    u8 *base = (u8*)PSX_ADDR(addr - off);
    for (unsigned j = off; j < 4; j++)
        base[j] = (u8)(val >> ((j - off) * 8));
}

/* Exact-access observer used only by the W34B9-B production-linked test.
 * Ordinary builds retain the original direct guest accesses. */
#if defined(WM_89160_TEST_TRACE)
static inline u8 wm_89160_lbu(u32 pc, u32 addr)
{
    u8 value = WM_U8(addr);
    wm_89160_test_trace(pc, WM_89160_TRACE_LBU, addr, 1u, (u32)value);
    return value;
}

static inline u16 wm_89160_lhu(u32 pc, u32 addr)
{
    u16 value = WM_U16(addr);
    wm_89160_test_trace(pc, WM_89160_TRACE_LHU, addr, 2u, (u32)value);
    return value;
}

static inline u32 wm_89160_lw(u32 pc, u32 addr)
{
    u32 value = WM_U32(addr);
    wm_89160_test_trace(pc, WM_89160_TRACE_LW, addr, 4u, value);
    return value;
}

static inline void wm_89160_sb(u32 pc, u32 addr, u8 value)
{
    WM_U8(addr) = value;
    wm_89160_test_trace(pc, WM_89160_TRACE_SB, addr, 1u, (u32)value);
}

static inline void wm_89160_sh(u32 pc, u32 addr, u16 value)
{
    WM_U16(addr) = value;
    wm_89160_test_trace(pc, WM_89160_TRACE_SH, addr, 2u, (u32)value);
}

static inline void wm_89160_sw(u32 pc, u32 addr, u32 value)
{
    WM_U32(addr) = value;
    wm_89160_test_trace(pc, WM_89160_TRACE_SW, addr, 4u, value);
}

static inline u32 wm_89160_lwl(u32 pc, u32 addr)
{
    u32 value = psx_lwl(addr);
    wm_89160_test_trace(pc, WM_89160_TRACE_LWL, addr,
                        (addr & 3u) + 1u, value);
    return value;
}

static inline u32 wm_89160_lwr(u32 pc, u32 addr)
{
    u32 value = psx_lwr(addr);
    wm_89160_test_trace(pc, WM_89160_TRACE_LWR, addr,
                        4u - (addr & 3u), value);
    return value;
}

static inline void wm_89160_swl(u32 pc, u32 addr, u32 value)
{
    psx_swl(addr, value);
    wm_89160_test_trace(pc, WM_89160_TRACE_SWL, addr,
                        (addr & 3u) + 1u, value);
}

static inline void wm_89160_swr(u32 pc, u32 addr, u32 value)
{
    psx_swr(addr, value);
    wm_89160_test_trace(pc, WM_89160_TRACE_SWR, addr,
                        4u - (addr & 3u), value);
}
#else
#define wm_89160_lbu(pc, addr)       WM_U8(addr)
#define wm_89160_lhu(pc, addr)       WM_U16(addr)
#define wm_89160_lw(pc, addr)        WM_U32(addr)
#define wm_89160_sb(pc, addr, value) (WM_U8(addr) = (u8)(value))
#define wm_89160_sh(pc, addr, value) (WM_U16(addr) = (u16)(value))
#define wm_89160_sw(pc, addr, value) (WM_U32(addr) = (u32)(value))
#define wm_89160_lwl(pc, addr)       psx_lwl(addr)
#define wm_89160_lwr(pc, addr)       psx_lwr(addr)
#define wm_89160_swl(pc, addr, value) psx_swl((addr), (value))
#define wm_89160_swr(pc, addr, value) psx_swr((addr), (value))
#endif

/* C610 selector address (same as in convergence module). */
#define WM_SLOT_C610_ABS  0x8009C610u

/* ---- 0x80089160 instrumentation ---- */

static int s_wm_89160_calls;
static int s_wm_89160_iterations;

int  wm_89160_get_calls(void)      { return s_wm_89160_calls; }
int  wm_89160_get_iterations(void) { return s_wm_89160_iterations; }
void wm_89160_reset(void)          { s_wm_89160_calls = 0; s_wm_89160_iterations = 0; }

/* ---- Common-tail P0 instrumentation ---- */

static int s_wm_ctp0_entry;
static int s_wm_ctp0_c610_zero;
static int s_wm_ctp0_c610_nonzero;
static int s_wm_ctp0_89160_calls;
static u32 s_wm_ctp0_last_cut;
static int s_wm_ctp0_forbidden_978fc;
static int s_wm_ctp0_forbidden_scheduler;
static int s_wm_ctp0_forbidden_world_loop;
static int s_wm_ctp0_forbidden_8901c;
static int s_wm_ctp0_forbidden_865a0;
static int s_wm_ctp0_forbidden_85fe0;
static int s_wm_ctp0_forbidden_75228;

int  wm_ctp0_get_entry(void)              { return s_wm_ctp0_entry; }
int  wm_ctp0_get_c610_zero(void)          { return s_wm_ctp0_c610_zero; }
int  wm_ctp0_get_c610_nonzero(void)       { return s_wm_ctp0_c610_nonzero; }
int  wm_ctp0_get_89160_calls(void)        { return s_wm_ctp0_89160_calls; }
u32  wm_ctp0_get_last_cut(void)           { return s_wm_ctp0_last_cut; }
int  wm_ctp0_get_forbidden_978fc(void)    { return s_wm_ctp0_forbidden_978fc; }
int  wm_ctp0_get_forbidden_scheduler(void){ return s_wm_ctp0_forbidden_scheduler; }
int  wm_ctp0_get_forbidden_world_loop(void){ return s_wm_ctp0_forbidden_world_loop; }
int  wm_ctp0_get_forbidden_8901c(void)    { return s_wm_ctp0_forbidden_8901c; }
int  wm_ctp0_get_forbidden_865a0(void)    { return s_wm_ctp0_forbidden_865a0; }
int  wm_ctp0_get_forbidden_85fe0(void)    { return s_wm_ctp0_forbidden_85fe0; }
int  wm_ctp0_get_forbidden_75228(void)    { return s_wm_ctp0_forbidden_75228; }

void wm_common_tail_p0_reset(void)
{
    s_wm_ctp0_entry = 0;
    s_wm_ctp0_c610_zero = 0;
    s_wm_ctp0_c610_nonzero = 0;
    s_wm_ctp0_89160_calls = 0;
    s_wm_ctp0_last_cut = 0;
    s_wm_ctp0_forbidden_978fc = 0;
    s_wm_ctp0_forbidden_scheduler = 0;
    s_wm_ctp0_forbidden_world_loop = 0;
    s_wm_ctp0_forbidden_8901c = 0;
    s_wm_ctp0_forbidden_865a0 = 0;
    s_wm_ctp0_forbidden_85fe0 = 0;
    s_wm_ctp0_forbidden_75228 = 0;
    wm_89160_reset();
}

/* ---- Forbidden-path stubs ---- */

void wm_800978fc_should_not_run(void)  { s_wm_ctp0_forbidden_978fc++; }
void wm_80097800_should_not_run(void)  { s_wm_ctp0_forbidden_scheduler++; }
void wm_p0_800712D0_should_not_run(void) { s_wm_ctp0_forbidden_world_loop++; }
void wm_8008901c_should_not_run(void)  { s_wm_ctp0_forbidden_8901c++; }
void wm_800865a0_should_not_run(void)  { s_wm_ctp0_forbidden_865a0++; }
void wm_80085fe0_should_not_run(void)  { s_wm_ctp0_forbidden_85fe0++; }
void wm_80075228_should_not_run(void)  { s_wm_ctp0_forbidden_75228++; }

/* ---- 0x80089160 production implementation ---- */

/* Exact native transcription of retail [0x80089160,0x800893E0).
 *
 * Leaf function: no direct calls, no stack frame.
 * Record stride: 672 bytes (0x2A0).
 * Table base: *(0x8009BCC0).
 * Inner loop: 8 sub-records, stride 0x54.
 * Flag: bit 0x80 at record+0x4F.
 *
 * Four dispatch paths based on a0/a1/a2:
 *   Path A: t6 & v1 != 0 → record+0x18 loop, aligned stores
 *   Path B: (0<a1) & v1 != 0 → record+0x20 loop, lwl/lwr+swl/swr
 *   Path C: t6 & (0<a2) != 0 → record+0x20 loop, lhu+negu+sh
 *   Path D: none of above → record+0x20 loop, lwl/lwr+swl/swr+lhu+negu+sh
 *
 * Natural call (14,0,0): Path A (t6=1, v1=1, and=1≠0).
 *
 * Dirty-path parity: when Phase 1 finds an existing 0x80 flag, retail
 * sets t2=1 and CONTINUES (never returns early).  In ALL four paths,
 * t2!=0 skips only the flag OR/store block but ALWAYS performs the
 * remaining stores. */
void wm_80089160(u32 a0, u32 a1, u32 a2)
{
    u32 table_base;
    u32 record_addr;
    u32 flag_byte;
    u32 a3;         /* record + 0x20 pointer (Path D) */
    u32 src_word0, src_word1;
    u16 hw0, hw1, hw2;
    int t0, i;
    int t2;         /* dirty-flag: 0 = clean, 1 = pre-existing 0x80 found */
    int t6, v1_cond;
    u32 t3;         /* saved a1 (source pointer) */

    s_wm_89160_calls++;
    t3 = a1;  /* retail: move $t3, $a1 at 0x80089160 */

    /* Compute record address: table_base + a0 * 672 */
    {
        u32 v0 = a0;
        v0 = (v0 << 2) + a0;   /* a0*5 */
        v0 = (v0 << 2) + a0;   /* a0*21 */
        v0 = v0 << 5;           /* a0*672 */
        table_base = wm_89160_lw(0x80089180u, WM_89160_TABLE_BASE_PTR);
        record_addr = table_base + v0;
    }

    fprintf(stderr,
            "[wm-80089160] entry a0=%u a1=0x%08x a2=0x%08x "
            "table_base=0x%08x record=0x%08x\n",
            a0, a1, a2, table_base, record_addr);

    /* Phase 1: check bit 0x80 at record+0x4F (0x8008918C–0x800891A4).
     * Retail: t2=0, t0=7.  Reads same byte each iteration.
     * If bit 0x80 found: t2=1, branch to Phase 2 (0x80089238→0x800891A8).
     * If bit clear: decrement t0, loop until t0==-1.
     * Key: retail NEVER returns here — it always continues to dispatch. */
    t2 = 0;
    flag_byte = wm_89160_lbu(0x8008918Cu,
                             record_addr + WM_89160_FLAG_BYTE_OFFSET);
    for (t0 = 7; t0 >= 0; t0--) {
        if (flag_byte & WM_89160_FLAG_BIT) {
            /* 0x80089198: bnez → 0x80089238: t2++, j 0x800891A8 */
            t2 = 1;
            fprintf(stderr,
                    "[wm-80089160] bit 0x80 found, t2=1, continuing\n");
            break;
        }
    }

    /* Phase 2: conditional dispatch (0x800891A8–0x80089348).
     *
     * Recompute record (retail does this after the first loop). */
    {
        u32 v0 = a0;
        v0 = (v0 << 2) + a0;
        v0 = (v0 << 2) + a0;
        v0 = v0 << 5;
        table_base = wm_89160_lw(0x800891BCu, WM_89160_TABLE_BASE_PTR);
        record_addr = table_base + v0;
    }

    /* 0x800891C4: sltiu $a0, $t3, 1 — NOTE: $a0 is OVERWRITTEN here.
     * The subsequent and at 0x800891D0 uses this overwritten $a0, NOT
     * the original record index. */
    t6 = (t3 < 1) ? 1 : 0;    /* retail: sltiu $a0, $t3, 1 */
    v1_cond = (a2 < 1) ? 1 : 0; /* retail: sltiu $v1, $a2, 1 */

    /* 0x800891D0: and $v0, $a0, $v1; beqz → 0x80089240
     * Uses the OVERWRITTEN $a0 (= t6), NOT the original record index.
     * For natural (14,0,0): t6=1, v1=1 → and=1 → NOT taken → Path A. */
    if ((t6 & v1_cond) == 0) {
        goto dispatch_check_2;
    }

    /* ---- Path A (0x800891DC–0x80089234) ----
     * Natural path for (a1 < 1) & (a2 < 1).
     * Uses $a0 = record + 0x18 as loop pointer, $a1 = -1 as sentinel.
     * Flag block + 4 unconditional aligned stores per iteration. */
    {
        u32 a0_loop = record_addr + 0x18;  /* addiu $a0, $t1, 0x18 */
        fprintf(stderr,
                "[wm-80089160] Path A t2=%d\n", t2);

        for (i = 0; i < WM_89160_SUBRECORD_COUNT; i++) {
            s_wm_89160_iterations++;

            if (t2 == 0) {
                /* Flag block (0x800891EC–0x8008920C):
                 * Flag byte at a0+0x37, hw from a0-8, word from (t1),
                 * zero hw at a0-0x0E, hw at a0-0x06, word at a0-0x14 */
                u8 byte_4f = wm_89160_lbu(0x800891ECu,
                                           a0_loop + 0x37u);
                byte_4f |= WM_89160_FLAG_BIT;
                wm_89160_sb(0x800891F8u, a0_loop + 0x37u, byte_4f);

                /* 0x800891FC/0x80089200: retail completes both loads
                 * before the 0x80089204/08/0C store sequence. */
                hw0 = wm_89160_lhu(0x800891FCu, a0_loop - 0x08u);
                src_word0 = wm_89160_lw(0x80089200u, record_addr);
                wm_89160_sh(0x80089204u, a0_loop - 0x0Eu, 0u);
                wm_89160_sh(0x80089208u, a0_loop - 0x06u, hw0);
                wm_89160_sw(0x8008920Cu, a0_loop - 0x14u, src_word0);
            }

            /* Unconditional stores (0x80089210–0x8008921C) */
            wm_89160_sw(0x80089210u, a0_loop + 4u, 0u);
            wm_89160_sw(0x80089214u, a0_loop - 4u, 0u);
            wm_89160_sh(0x80089218u, a0_loop + 8u, 0u);
            wm_89160_sh(0x8008921Cu, a0_loop, 0u);

            a0_loop += WM_89160_SUBRECORD_STRIDE;
            record_addr += WM_89160_SUBRECORD_STRIDE;
        }
    }

    fprintf(stderr,
            "[wm-80089160] exit t2=%d iterations=%d\n",
            t2, s_wm_89160_iterations);
    return;

dispatch_check_2:
    /* 0x80089240: sltu $v0, $zero, $t3; and $v0, $v0, $v1; beqz → 0x800892C0 */
    if (((0 < t3) ? 1 : 0) & v1_cond) {
        /* ---- Path B (0x80089250–0x800892BC) ---- */
        u32 a2_loop = record_addr + 0x20;

        fprintf(stderr,
                "[wm-80089160] Path B t2=%d\n", t2);

        for (i = 0; i < WM_89160_SUBRECORD_COUNT; i++) {
            s_wm_89160_iterations++;

            if (t2 == 0) {
                u8 byte_4f = wm_89160_lbu(0x8008925Cu,
                                           a2_loop + 0x2Fu);
                byte_4f |= WM_89160_FLAG_BIT;
                wm_89160_sb(0x80089268u, a2_loop + 0x2Fu, byte_4f);

                /* 0x8008926C/70 loads precede the retail
                 * 0x80089274/78/7C store sequence. */
                hw0 = wm_89160_lhu(0x8008926Cu, a2_loop - 0x10u);
                src_word0 = wm_89160_lw(0x80089270u, record_addr);
                wm_89160_sh(0x80089274u, a2_loop - 0x16u, 0u);
                wm_89160_sh(0x80089278u, a2_loop - 0x0Eu, hw0);
                wm_89160_sw(0x8008927Cu, a2_loop - 0x1Cu, src_word0);
            }

            /* lwl/lwr from t3, swl/swr to a2_loop.
             * MIPS retail convention: lwl at addr+3, lwr at addr+0. */
            src_word0 = wm_89160_lwl(0x80089280u, t3 + 3u);
            src_word0 |= wm_89160_lwr(0x80089284u, t3);
            src_word1 = wm_89160_lwl(0x80089288u, t3 + 7u);
            src_word1 |= wm_89160_lwr(0x8008928Cu, t3 + 4u);
            wm_89160_swl(0x80089290u, a2_loop - 9u, src_word0);
            wm_89160_swr(0x80089294u, a2_loop - 12u, src_word0);
            wm_89160_swl(0x80089298u, a2_loop - 5u, src_word1);
            wm_89160_swr(0x8008929Cu, a2_loop - 8u, src_word1);

            wm_89160_sw(0x800892A0u, a2_loop - 4u, 0u);
            wm_89160_sh(0x800892A4u, a2_loop, 0u);

            a2_loop += WM_89160_SUBRECORD_STRIDE;
            record_addr += WM_89160_SUBRECORD_STRIDE;
        }

        fprintf(stderr,
                "[wm-80089160] exit t2=%d iterations=%d\n",
                t2, s_wm_89160_iterations);
        return;
    }

    /* 0x800892C0: sltu $v0, $zero, $a2; and $v0, $a0, $v0; beqz → 0x8008934C
     * $a0 at this point = t6 (the sltiu result from 0x800891C4). */
    if ((t6 & ((0 < a2) ? 1 : 0)) != 0) {
        /* ---- Path C (0x800892D0–0x80089344) ---- */
        u32 a0_loop = record_addr + 0x20;
        fprintf(stderr,
                "[wm-80089160] Path C t2=%d\n", t2);

        for (i = 0; i < WM_89160_SUBRECORD_COUNT; i++) {
            s_wm_89160_iterations++;

            if (t2 == 0) {
                u8 byte_4f = wm_89160_lbu(0x800892E0u,
                                           a0_loop + 0x2Fu);
                byte_4f |= WM_89160_FLAG_BIT;
                wm_89160_sb(0x800892ECu, a0_loop + 0x2Fu, byte_4f);

                /* 0x800892F0/F4 loads precede the retail
                 * 0x800892F8/FC/0x80089300 store sequence. */
                hw0 = wm_89160_lhu(0x800892F0u, a0_loop - 0x10u);
                src_word0 = wm_89160_lw(0x800892F4u, record_addr);
                wm_89160_sh(0x800892F8u, a0_loop - 0x16u, 0u);
                wm_89160_sh(0x800892FCu, a0_loop - 0x0Eu, hw0);
                wm_89160_sw(0x80089300u, a0_loop - 0x1Cu, src_word0);
            }

            /* Unconditional stores */
            wm_89160_sw(0x80089304u, a0_loop - 0x0Cu, 0u);
            wm_89160_sh(0x80089308u, a0_loop - 0x08u, 0u);

            hw0 = wm_89160_lhu(0x8008930Cu, a2);
            wm_89160_sh(0x80089318u, a0_loop - 4u,
                        (u16)(-((s16)hw0)));
            hw1 = wm_89160_lhu(0x8008931Cu, a2 + 2u);
            wm_89160_sh(0x80089328u, a0_loop - 2u,
                        (u16)(-((s16)hw1)));
            hw2 = wm_89160_lhu(0x8008932Cu, a2 + 4u);
            wm_89160_sh(0x80089338u, a0_loop,
                        (u16)(-((s16)hw2)));

            record_addr += WM_89160_SUBRECORD_STRIDE;
            a0_loop += WM_89160_SUBRECORD_STRIDE;
        }

        fprintf(stderr,
                "[wm-80089160] exit t2=%d iterations=%d\n",
                t2, s_wm_89160_iterations);
        return;
    }

    /* ---- Path D (0x8008934C–0x800893D4) ----
     * Uses $a3 = record+0x20 as loop pointer.
     * Flag block + lwl/lwr/swl/swr + lhu/negu/sh per iteration. */
    {
        a3 = record_addr + 0x20;
        fprintf(stderr,
                "[wm-80089160] Path D t2=%d\n", t2);

        for (i = 0; i < WM_89160_SUBRECORD_COUNT; i++) {
            s_wm_89160_iterations++;

            if (t2 == 0) {
                /* Flag block (0x8008935C–0x8008937C) */
                u8 byte_4f = wm_89160_lbu(0x8008935Cu,
                                           a3 + 0x2Fu);
                byte_4f |= WM_89160_FLAG_BIT;
                wm_89160_sb(0x80089368u, a3 + 0x2Fu, byte_4f);

                /* 0x8008936C/70 loads precede the retail
                 * 0x80089374/78/7C store sequence. */
                hw0 = wm_89160_lhu(0x8008936Cu,
                                    record_addr + 0x10u);
                src_word0 = wm_89160_lw(0x80089370u, record_addr);
                wm_89160_sh(0x80089374u, a3 - 0x16u, 0u);
                wm_89160_sh(0x80089378u, a3 - 0x0Eu, hw0);
                wm_89160_sw(0x8008937Cu, a3 - 0x1Cu, src_word0);
            }

            /* Unconditional: lwl/lwr from t3, swl/swr to a3.
             * MIPS retail convention: lwl at addr+3, lwr at addr+0. */
            src_word0 = wm_89160_lwl(0x80089380u, t3 + 3u);
            src_word0 |= wm_89160_lwr(0x80089384u, t3);
            src_word1 = wm_89160_lwl(0x80089388u, t3 + 7u);
            src_word1 |= wm_89160_lwr(0x8008938Cu, t3 + 4u);
            wm_89160_swl(0x80089390u, a3 - 9u, src_word0);
            wm_89160_swr(0x80089394u, a3 - 12u, src_word0);
            wm_89160_swl(0x80089398u, a3 - 5u, src_word1);
            wm_89160_swr(0x8008939Cu, a3 - 8u, src_word1);

            /* 0x800893A0 is LHU, not SW: retail has no zero-word store
             * at a3-4 between the unaligned copy and these halfwords. */
            /* 0x800893A0/B0/C0: a2 is the fixed incoming source on every
             * iteration; retail never advances it. */
            hw0 = wm_89160_lhu(0x800893A0u, a2);
            wm_89160_sh(0x800893ACu, a3 - 4u,
                        (u16)(-((s16)hw0)));
            hw1 = wm_89160_lhu(0x800893B0u, a2 + 2u);
            wm_89160_sh(0x800893BCu, a3 - 2u,
                        (u16)(-((s16)hw1)));
            hw2 = wm_89160_lhu(0x800893C0u, a2 + 4u);
            wm_89160_sh(0x800893CCu, a3,
                        (u16)(-((s16)hw2)));

            /* 0x800893D4: only a3 advances by the 0x54 subrecord stride,
             * in the 0x800893D0 loop-branch delay slot. */
            a3 += WM_89160_SUBRECORD_STRIDE;
            record_addr += WM_89160_SUBRECORD_STRIDE;
        }
    }

    fprintf(stderr,
            "[wm-80089160] exit t2=%d iterations=%d\n",
            t2, s_wm_89160_iterations);
}

/* Retail [0x800894C8, 0x80089514). Leaf, void, no calls, no bounds check.
 * Access widths/order match the listing: one lw of the table base, then
 * eight lbu/andi/sb pairs at +0x4F + k*0x54. */
void wm_800894C8(u32 record_index)
{
    u32 table_base;
    u32 cursor;
    u32 i;
    u32 count;
    u32 record_stride;
    u32 sub_stride;
    u32 flag_off;

#if defined(WM_894C8_MUTANT_BOUNDS_CHECK)
    if (record_index > 0x3Fu)
        return;
#endif

#if defined(WM_894C8_MUTANT_WRONG_RECORD_STRIDE)
    record_stride = 668u;
#else
    record_stride = WM_89160_RECORD_STRIDE;
#endif

#if defined(WM_894C8_MUTANT_WRONG_SUBRECORD_STRIDE)
    sub_stride = 0x50u;
#else
    sub_stride = WM_89160_SUBRECORD_STRIDE;
#endif

#if defined(WM_894C8_MUTANT_WRONG_FLAG_OFFSET)
    flag_off = 0x4Eu;
#else
    flag_off = WM_89160_FLAG_BYTE_OFFSET;
#endif

#if defined(WM_894C8_MUTANT_COUNT_7)
    count = 7u;
#elif defined(WM_894C8_MUTANT_COUNT_9)
    count = 9u;
#else
    count = (u32)WM_89160_SUBRECORD_COUNT;
#endif

    table_base = wm_89160_lw(0x800894E4u, WM_89160_TABLE_BASE_PTR);
    cursor = table_base + record_index * record_stride + flag_off;

    for (i = 0; i < count; i++) {
        u8 flag = wm_89160_lbu(0x800894F4u, cursor);
#if defined(WM_894C8_MUTANT_CLEAR_WHOLE_BYTE)
        flag = 0u;
#elif defined(WM_894C8_MUTANT_CLEAR_WRONG_BIT)
        flag = (u8)(flag & (u8)~0x40u);
#else
        flag = (u8)(flag & (u8)~WM_89160_FLAG_BIT);
#endif
        wm_89160_sb(0x80089500u, cursor, flag);
        cursor += sub_stride;
    }
}

/* ---- Common-tail P0 production implementation ---- */

/* Bounded common-tail prefix starting at 0x8007290C.
 *
 * Reads C610 selector, dispatches:
 *   C610=0: calls wm_80089160(14,0,0), sets D_80059179=1
 *   C610≠0: skips wm_80089160, leaves D_80059179=0
 *
 * Returns the exact reconvergence cut PC: 0x8007293C.
 * Does NOT execute any instruction at or after 0x8007293C. */
u32 wm_8007290C_common_tail_p0(void)
{
    u32 c610;
    u32 cut;

    s_wm_ctp0_entry++;

    /* 0x8007290C–0x80072910: load C610 selector. */
    c610 = WM_U32(WM_SLOT_C610_ABS);
    fprintf(stderr,
            "[worldmap-common-tail-p0] entry C610=%u\n", c610);

    /* 0x80072914–0x80072918: D_80059179 = 0 (unconditional).
     * This main-executable symbol is a native authority in the port; compiled
     * field/menu consumers read the same host object. */
#if defined(W34N19_MUTANT_COMMON_TAIL_GUEST_TWIN)
    WM_U8(WM_D_80059179_ABS) = 0;
#else
    D_80059179 = 0;
#endif

    /* 0x8007291C: bnez $v0, 0x8007293C */
    if (c610 != 0) {
        /* C610 ≠ 0: skip 0x80089160 call, go to reconvergence. */
        s_wm_ctp0_c610_nonzero++;
        cut = WM_COMMON_TAIL_P0_CUT;
        s_wm_ctp0_last_cut = cut;
        fprintf(stderr,
                "[worldmap-common-tail-p0] C610!=0 → skip 0x80089160, "
                "cut=0x%08x\n", cut);
        return cut;
    }

    /* C610 == 0: natural path. */
    s_wm_ctp0_c610_zero++;

    /* 0x80072920: a0 = 14 [delay slot]
     * 0x80072924: a1 = 0
     * 0x80072928: jal 0x80089160
     * 0x8007292C: a2 = 0 [delay slot] */
    fprintf(stderr,
            "[worldmap-common-tail-p0] C610=0 → calling wm_80089160(14,0,0)\n");
    wm_80089160(14, 0, 0);
    s_wm_ctp0_89160_calls++;

    /* 0x80072930: v0 = 1
     * 0x80072934–0x80072938: D_80059179 = 1 */
#if defined(W34N19_MUTANT_COMMON_TAIL_GUEST_TWIN)
    WM_U8(WM_D_80059179_ABS) = 1;
#else
    D_80059179 = 1;
#endif

    /* Reconvergence at 0x8007293C. */
    cut = WM_COMMON_TAIL_P0_CUT;
    s_wm_ctp0_last_cut = cut;
    fprintf(stderr,
            "[worldmap-common-tail-p0] exit cut=0x%08x\n", cut);
    return cut;
}


/* ---- 0x800978FC: world-map graphics buffer allocator ---- */

/* HeapAlloc declared in port_main.c / world_map_init.c */
extern void* HeapAlloc(u_int allocSize, u_int allocFlags);

/* Constants from retail decode. */
#define WM_978FC_ALLOC_SIZE     0x10000u   /* 64 KB */
#define WM_978FC_RECORD_COUNT   2048
#define WM_978FC_RECORD_STRIDE  32
#define WM_978FC_COPY_CHUNK     16

/* Global addresses written by this helper. */
#define WM_D_8009BC3C_ABS       0x8009BC3Cu
#define WM_D_8009BCB4_ABS       0x8009BCB4u

/* Convert host pointer (from HeapAlloc) to PSX KUSEG address. */
static u32 wm_host_to_psx(void *p)
{
    if (!p) return 0;
    uintptr_t host = (uintptr_t)p;
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (host >= base && host < base + PSX_RAM_SIZE)
        return 0x80000000u | (u32)(host - base);
    return (u32)host;
}

void wm_800978FC(void)
{
    /* Retail 0x800978FC–0x800979C4.
     * Allocates two 64 KB buffers, initializes the first with a
     * repeating byte pattern (2048 records × 32 bytes), then copies
     * the first buffer to the second.
     *
     * Record initialization (per 32-byte record):
     *   byte[-3] = 7
     *   byte[-2] = 0x80
     *   byte[-1] = 0x80
     *   byte[+0] = 0x80
     *   byte[+1] = 0x24
     *   (remaining 27 bytes left as HeapAlloc zero-fill) */

    void *p1_host, *p2_host;
    u32 ptr1, ptr2;
    u32 v1;
    int i;

    /* 0x80097900–0x80097910: first HeapAlloc(0x10000, 1) */
    p1_host = HeapAlloc(WM_978FC_ALLOC_SIZE, 1);
    ptr1 = wm_host_to_psx(p1_host);

    /* 0x80097914–0x80097928: second HeapAlloc(0x10000, 1)
     * Delay slot stores ptr1 before the second call. */
    WM_U32(WM_D_8009BC3C_ABS) = ptr1;

    p2_host = HeapAlloc(WM_978FC_ALLOC_SIZE, 1);
    ptr2 = wm_host_to_psx(p2_host);
    WM_U32(WM_D_8009BCB4_ABS) = ptr2;

    fprintf(stderr,
            "[wm-800978FC] alloc ptr1=0x%08x ptr2=0x%08x\n",
            ptr1, ptr2);

    /* 0x8009792C–0x8009796C: initialize ptr1 records.
     * v1 starts at ptr1 + 6, increments by 32 each iteration.
     * 2048 iterations. */
    v1 = ptr1 + 6;
    for (i = 0; i < WM_978FC_RECORD_COUNT; i++) {
        WM_U8(v1 - 3) = 7;
        WM_U8(v1 - 2) = 0x80;
        WM_U8(v1 - 1) = 0x80;
        WM_U8(v1 + 0) = 0x80;
        WM_U8(v1 + 1) = 0x24;
        v1 += WM_978FC_RECORD_STRIDE;
    }

    /* 0x80097970–0x800979B0: copy ptr1 → ptr2.
     * 65536 / 16 = 4096 iterations, 16 bytes per iteration. */
    {
        u32 src = ptr1;
        u32 dst = ptr2;
        u32 end = ptr1 + WM_978FC_ALLOC_SIZE;
        while (src != end) {
            WM_U32(dst + 0)  = WM_U32(src + 0);
            WM_U32(dst + 4)  = WM_U32(src + 4);
            WM_U32(dst + 8)  = WM_U32(src + 8);
            WM_U32(dst + 12) = WM_U32(src + 12);
            src += WM_978FC_COPY_CHUNK;
            dst += WM_978FC_COPY_CHUNK;
        }
    }

    fprintf(stderr,
            "[wm-800978FC] init+copy done\n");
}


/* ---- Common-tail P1 instrumentation ---- */

static int  s_wm_ctp1_entry;
static int  s_wm_ctp1_978fc_calls;
static u32  s_wm_ctp1_last_cut;

int  wm_ctp1_get_entry(void)        { return s_wm_ctp1_entry; }
int  wm_ctp1_get_978fc_calls(void)  { return s_wm_ctp1_978fc_calls; }
u32  wm_ctp1_get_last_cut(void)     { return s_wm_ctp1_last_cut; }

void wm_common_tail_p1_reset(void)
{
    s_wm_ctp1_entry = 0;
    s_wm_ctp1_978fc_calls = 0;
    s_wm_ctp1_last_cut = 0;
}


/* ---- Common-tail P1: caller slice ---- */

u32 wm_8007293C_common_tail_p1(void)
{
    /* P1 slice: 0x8007293C..0x80072940
     * jal 0x800978FC + delay slot (nop).
     * First excluded: 0x80072944 (jal 0x8008901C).
     *
     * Requires P0 to have executed and returned 0x8007293C. */
    u32 cut;

    s_wm_ctp1_entry++;

    /* 0x8007293C: jal 0x800978FC */
    fprintf(stderr,
            "[worldmap-common-tail-p1] calling wm_800978FC\n");
    wm_800978FC();
    s_wm_ctp1_978fc_calls++;

    /* 0x80072940: nop (delay slot — no effect)
     * Return cut at 0x80072944 (first excluded = next helper). */
    cut = WM_COMMON_TAIL_P1_CUT;
    s_wm_ctp1_last_cut = cut;

    fprintf(stderr,
            "[worldmap-common-tail-p1] exit cut=0x%08x\n", cut);
    return cut;
}


/* ---- Common-tail P2 instrumentation ---- */

static int  s_wm_ctp2_entry;
static int  s_wm_ctp2_8901c_calls;
static u32  s_wm_ctp2_last_cut;
static int  s_wm_ctp2_alloc_calls;
static int  s_wm_ctp2_forbidden_865a0;
static int  s_wm_ctp2_forbidden_85fe0;
static int  s_wm_ctp2_forbidden_scheduler;
static int  s_wm_ctp2_forbidden_world_loop;

int  wm_ctp2_get_entry(void)              { return s_wm_ctp2_entry; }
int  wm_ctp2_get_8901c_calls(void)        { return s_wm_ctp2_8901c_calls; }
u32  wm_ctp2_get_last_cut(void)           { return s_wm_ctp2_last_cut; }
int  wm_ctp2_get_alloc_calls(void)        { return s_wm_ctp2_alloc_calls; }
int  wm_ctp2_get_forbidden_865a0(void)    { return s_wm_ctp2_forbidden_865a0; }
int  wm_ctp2_get_forbidden_85fe0(void)    { return s_wm_ctp2_forbidden_85fe0; }
int  wm_ctp2_get_forbidden_scheduler(void){ return s_wm_ctp2_forbidden_scheduler; }
int  wm_ctp2_get_forbidden_world_loop(void){ return s_wm_ctp2_forbidden_world_loop; }

void wm_common_tail_p2_reset(void)
{
    s_wm_ctp2_entry = 0;
    s_wm_ctp2_8901c_calls = 0;
    s_wm_ctp2_last_cut = 0;
    s_wm_ctp2_alloc_calls = 0;
    s_wm_ctp2_forbidden_865a0 = 0;
    s_wm_ctp2_forbidden_85fe0 = 0;
    s_wm_ctp2_forbidden_scheduler = 0;
    s_wm_ctp2_forbidden_world_loop = 0;
}

/* ---- P2 forbidden-path stubs ---- */

void wm_p2_800865a0_should_not_run(void) { s_wm_ctp2_forbidden_865a0++; }
void wm_p2_80085fe0_should_not_run(void) { s_wm_ctp2_forbidden_85fe0++; }
void wm_p2_80097800_should_not_run(void) { s_wm_ctp2_forbidden_scheduler++; }
void wm_p2_800712D0_should_not_run(void) { s_wm_ctp2_forbidden_world_loop++; }


/* ---- 0x8008901C production implementation ---- */

/* Exact native transcription of retail 0x8008901C–0x80089128.
 *
 * Allocates two 10240-byte (0x2800) buffers via HeapAlloc.
 * Stores pointers at D_8009BE1C and D_8009BE20.
 *
 * Initializes 256 records (40 bytes each) in the first buffer.
 * Record base = alloc_ptr + 7.  Per-record writes:
 *   byte[-4] = 9    (record type marker)
 *   byte[+0] = 0x2C (44, then OR'd with 0x02 → 0x2E after first pass)
 *   hw[+7]   = GetTPage(1, 1, 0x340, 0x100) = 0x00BD
 *   hw[+15]  = GetClut(0x100, 0x1FF) = 0x7FD0
 *
 * Then copies first buffer → second buffer (10240 bytes, 16-byte chunks).
 *
 * Only calls: HeapAlloc, PsyQ GetTPage, PsyQ GetClut. */
void wm_8008901C(void)
{
    void *p1_host, *p2_host;
    u32 ptr1, ptr2;
    u16 tpage_val, clut_val;
    u32 base;
    int i;

    /* 0x80089020–0x80089040: first HeapAlloc(0x2800, 1) */
    p1_host = HeapAlloc(WM_8901C_ALLOC_SIZE, 1);
    ptr1 = wm_host_to_psx(p1_host);
    s_wm_ctp2_alloc_calls++;

    /* 0x80089044–0x80089058: second HeapAlloc(0x2800, 1)
     * Delay slot stores ptr1 at D_8009BE1C before the call. */
    WM_U32(WM_D_8009BE1C_ABS) = ptr1;

    p2_host = HeapAlloc(WM_8901C_ALLOC_SIZE, 1);
    ptr2 = wm_host_to_psx(p2_host);
    s_wm_ctp2_alloc_calls++;

    /* 0x80089070–0x80089074: store ptr2 at D_8009BE20. */
    WM_U32(WM_D_8009BE20_ABS) = ptr2;

    fprintf(stderr,
            "[wm-8008901C] alloc ptr1=0x%08x ptr2=0x%08x\n",
            ptr1, ptr2);

    /* 0x80089084–0x80089088: GetTPage(1, 1, 0x340, 0x100) */
    tpage_val = GetTPage(1, 1, 0x340, 0x100);

    /* 0x80089098–0x8008909C: GetClut(0x100, 0x1FF) */
    clut_val = GetClut(0x100, 0x1FF);

    fprintf(stderr,
            "[wm-8008901C] tpage=0x%04x clut=0x%04x\n",
            tpage_val, clut_val);

    /* 0x80089078–0x800890C0: initialize 256 records.
     * Base = ptr1 + 7.  Stride = 40.  Count = 256. */
    base = ptr1 + WM_8901C_RECORD_BASE_OFFSET;
    for (i = 0; i < WM_8901C_RECORD_COUNT; i++) {
        /* 0x8008908C: sb $s4, -4($s0) → byte[-4] = 9 */
        WM_U8(base - 4) = 9;

        /* 0x80089094: sb $s3, 0($s0) → byte[0] = 0x2C */
        WM_U8(base + 0) = 0x2C;

        /* 0x800890A4 (delay of jal GetClut): sh $v0, 15($s0)
         * $v0 still holds GetTPage result at this point, so packet+0x16
         * receives tpage.  GetClut then overwrites $v0 and 0x800890B0
         * stores it at 7($s0), which is packet+0x0e. */
#if defined(WM_8901C_MUTANT_SWAPPED_TEXTURE_FIELDS)
        WM_U16(base + 7)  = tpage_val;
        WM_U16(base + 15) = clut_val;
#else
        WM_U16(base + 15) = tpage_val;
        WM_U16(base + 7)  = clut_val;
#endif

        /* 0x800890B4–0x800890B8: lbu/ori/sb → byte[0] |= 0x02 */
        WM_U8(base + 0) = WM_U8(base + 0) | 0x02;

        base += WM_8901C_RECORD_STRIDE;
    }

    /* 0x800890D4–0x80089100: copy ptr1 → ptr2 (10240 bytes, 16-byte chunks). */
    {
        u32 src = ptr1;
        u32 dst = ptr2;
        u32 end = ptr1 + WM_8901C_ALLOC_SIZE;
        while (src != end) {
            WM_U32(dst + 0)  = WM_U32(src + 0);
            WM_U32(dst + 4)  = WM_U32(src + 4);
            WM_U32(dst + 8)  = WM_U32(src + 8);
            WM_U32(dst + 12) = WM_U32(src + 12);
            src += WM_8901C_COPY_CHUNK;
            dst += WM_8901C_COPY_CHUNK;
        }
    }

    fprintf(stderr,
            "[wm-8008901C] init+copy done\n");
}


/* ---- Common-tail P2: caller slice ---- */

u32 wm_80072944_common_tail_p2(void)
{
    /* P2 slice: 0x80072944..0x80072948
     * jal 0x8008901C + delay slot (nop).
     * First excluded: 0x8007294C (jal 0x800865A0).
     *
     * Requires P1 to have executed and returned 0x80072944. */
    u32 cut;

    s_wm_ctp2_entry++;

    /* 0x80072944: jal 0x8008901C */
    fprintf(stderr,
            "[worldmap-common-tail-p2] calling wm_8008901C\n");
    wm_8008901C();
    s_wm_ctp2_8901c_calls++;

    /* 0x80072948: nop (delay slot — no effect)
     * Return cut at 0x8007294C (first excluded = next helper). */
    cut = WM_COMMON_TAIL_P2_CUT;
    s_wm_ctp2_last_cut = cut;

    fprintf(stderr,
            "[worldmap-common-tail-p2] exit cut=0x%08x\n", cut);
    return cut;
}


/* ---- Common-tail P3 instrumentation ---- */

static int  s_wm_ctp3_entry;
static int  s_wm_ctp3_865a0_calls;
static u32  s_wm_ctp3_last_cut;
static int  s_wm_ctp3_alloc_calls;
static int  s_wm_ctp3_forbidden_85fe0;
static int  s_wm_ctp3_forbidden_scheduler;
static int  s_wm_ctp3_forbidden_world_loop;
static int  s_wm_ctp3_forbidden_75228;

int  wm_ctp3_get_entry(void)              { return s_wm_ctp3_entry; }
int  wm_ctp3_get_865a0_calls(void)        { return s_wm_ctp3_865a0_calls; }
u32  wm_ctp3_get_last_cut(void)           { return s_wm_ctp3_last_cut; }
int  wm_ctp3_get_alloc_calls(void)        { return s_wm_ctp3_alloc_calls; }
int  wm_ctp3_get_forbidden_85fe0(void)    { return s_wm_ctp3_forbidden_85fe0; }
int  wm_ctp3_get_forbidden_scheduler(void){ return s_wm_ctp3_forbidden_scheduler; }
int  wm_ctp3_get_forbidden_world_loop(void){ return s_wm_ctp3_forbidden_world_loop; }
int  wm_ctp3_get_forbidden_75228(void)    { return s_wm_ctp3_forbidden_75228; }

void wm_common_tail_p3_reset(void)
{
    s_wm_ctp3_entry = 0;
    s_wm_ctp3_865a0_calls = 0;
    s_wm_ctp3_last_cut = 0;
    s_wm_ctp3_alloc_calls = 0;
    s_wm_ctp3_forbidden_85fe0 = 0;
    s_wm_ctp3_forbidden_scheduler = 0;
    s_wm_ctp3_forbidden_world_loop = 0;
    s_wm_ctp3_forbidden_75228 = 0;
}

/* ---- P3 forbidden-path stubs ---- */

void wm_p3_80085fe0_should_not_run(void) { s_wm_ctp3_forbidden_85fe0++; }
void wm_p3_80097800_should_not_run(void) { s_wm_ctp3_forbidden_scheduler++; }
void wm_p3_800712D0_should_not_run(void) { s_wm_ctp3_forbidden_world_loop++; }
void wm_p3_80075228_should_not_run(void) { s_wm_ctp3_forbidden_75228++; }


/* ---- 0x800865A0 production implementation ---- */

/* Exact native transcription of retail 0x800865A0–0x800866C8.
 *
 * Allocates two 11520-byte (0x2D00) buffers via HeapAlloc.
 * Stores pointers at D_8009D7F8 and D_8009D7FC.
 *
 * Initializes 288 records (40 bytes each) in the first buffer.
 * Retail keeps s0 = record + 14 as its CLUT anchor, but the packet passed to
 * SetSemiTrans and consumed later by wm_80086798 starts at record.  Per-record
 * writes relative to that packet start are:
 *   byte[+3]  = 9    (type marker)
 *   byte[+4]  = 38 (0x26)
 *   byte[+5]  = 38 (0x26)
 *   byte[+6]  = 38 (0x26)
 *   byte[+7]  = 44 (0x2C), then OR'd with 0x02 → 0x2E (SetSemiTrans)
 *   hw[+14]   = GetClut(304, 510)
 *   hw[+22]   = GetTPage(0, 1, 960, 256)
 *
 * Then copies first buffer → second buffer (11520 bytes, 16-byte chunks).
 *
 * Calls: HeapAlloc, PsyQ GetTPage, PsyQ GetClut, PsyQ SetSemiTrans. */
void wm_800865A0(void)
{
    void *p1_host, *p2_host;
    u32 ptr1, ptr2;
    u16 tpage_val, clut_val;
    u32 record;
    int i;

    /* 0x800865A4–0x800865C4: first HeapAlloc(0x2D00, 1) */
    p1_host = HeapAlloc(WM_865A0_ALLOC_SIZE, 1);
    ptr1 = wm_host_to_psx(p1_host);
    s_wm_ctp3_alloc_calls++;

    /* 0x800865CC–0x800865DC: second HeapAlloc(0x2D00, 1)
     * Delay slot stores ptr1 at D_8009D7F8 before the call. */
    WM_U32(WM_D_8009D7F8_ABS) = ptr1;

    p2_host = HeapAlloc(WM_865A0_ALLOC_SIZE, 1);
    ptr2 = wm_host_to_psx(p2_host);
    s_wm_ctp3_alloc_calls++;

    /* 0x800865F8–0x800865FC: store ptr2 at D_8009D7FC. */
    WM_U32(WM_D_8009D7FC_ABS) = ptr2;

    fprintf(stderr,
            "[wm-800865A0] alloc ptr1=0x%08x ptr2=0x%08x\n",
            ptr1, ptr2);

    /* 0x80086624: GetTPage(0, 1, 960, 256) */
    tpage_val = GetTPage(0, 1, 960, 256);

    /* 0x80086634: GetClut(304, 510) */
    clut_val = GetClut(304, 510);

    fprintf(stderr,
            "[wm-800865A0] tpage=0x%04x clut=0x%04x\n",
            tpage_val, clut_val);

    /* 0x80086600–0x8008665C: initialize 288 records.
     * s1 is the packet start and s0 is s1 + 14.  The negative stores through
     * s0 therefore land at packet offsets +3..+7; SetSemiTrans receives s1.
     * Stride = 40.  Count = 288. */
    record = ptr1;
    for (i = 0; i < WM_865A0_RECORD_COUNT; i++) {
#if defined(WM_865A0_MUTANT_SHIFTED_HEADER)
        u32 header = record + WM_865A0_CLUT_OFFSET;
#else
        u32 header = record;
#endif
        /* 0x80086614: sb $s5, -11($s0) → byte[3] = 9 */
        WM_U8(header + 3u) = 9;

        /* 0x8008661C: sb $s3, -10($s0) → byte[4] = 38 */
        WM_U8(header + 4u) = 38;

        /* 0x80086620: sb $s3, -9($s0) → byte[5] = 38 */
        WM_U8(header + 5u) = 38;

        /* 0x80086628 (delay of jal GetTPage): sb $s3, -8($s0) → byte[6] = 38 */
        WM_U8(header + 6u) = 38;

        /* 0x80086618: sb $s4, -7($s0) → byte[7] = 44 (code byte) */
        WM_U8(header + 7u) = 44;

        /* 0x80086638 (delay of jal GetClut): sh $v0, 8($s0) → tpage
         * $v0 still holds GetTPage result at this point;
         * s0+8 = record+22.  GetClut then overwrites $v0.
         * 0x80086648 (delay of jal SetSemiTrans): sh $v0, 0($s0) → clut
         * at record+14. */
        WM_U16(record + 22u) = tpage_val;
        WM_U16(record + WM_865A0_CLUT_OFFSET) = clut_val;

        /* 0x80086644: SetSemiTrans(record, 1) → byte[7] |= 0x02
         * Retail SetSemiTrans with abe=1 sets bit 1 of the code byte.
         * Inline: code byte at record+7, OR with 0x02. */
        WM_U8(header + 7u) = WM_U8(header + 7u) | 0x02u;

        record += WM_865A0_RECORD_STRIDE;
    }

    /* 0x80086660–0x8008669C: copy ptr1 → ptr2 (11520 bytes, 16-byte chunks). */
    {
        u32 src = ptr1;
        u32 dst = ptr2;
        u32 end = ptr1 + WM_865A0_ALLOC_SIZE;
        while (src != end) {
            WM_U32(dst + 0)  = WM_U32(src + 0);
            WM_U32(dst + 4)  = WM_U32(src + 4);
            WM_U32(dst + 8)  = WM_U32(src + 8);
            WM_U32(dst + 12) = WM_U32(src + 12);
            src += WM_865A0_COPY_CHUNK;
            dst += WM_865A0_COPY_CHUNK;
        }
    }

    fprintf(stderr,
            "[wm-800865A0] init+copy done\n");
}


/* ---- Common-tail P3: caller slice ---- */

u32 wm_8007294C_common_tail_p3(void)
{
    /* P3 slice: 0x8007294C..0x80072950
     * jal 0x800865A0 + delay slot (nop).
     * First excluded: 0x80072954 (jal 0x80085FE0).
     *
     * Requires P2 to have executed and returned 0x8007294C. */
    u32 cut;

    s_wm_ctp3_entry++;

    /* 0x8007294C: jal 0x800865A0 */
    fprintf(stderr,
            "[worldmap-common-tail-p3] calling wm_800865A0\n");
    wm_800865A0();
    s_wm_ctp3_865a0_calls++;

    /* 0x80072950: nop (delay slot — no effect)
     * Return cut at 0x80072954 (first excluded = next helper 0x80085FE0). */
    cut = WM_COMMON_TAIL_P3_CUT;
    s_wm_ctp3_last_cut = cut;

    fprintf(stderr,
            "[worldmap-common-tail-p3] exit cut=0x%08x\n", cut);
    return cut;
}


/* ---- Common-tail P4 instrumentation ---- */

static int  s_wm_ctp4_entry;
static int  s_wm_ctp4_85fe0_calls;
static u32  s_wm_ctp4_last_cut;
static int  s_wm_ctp4_alloc_calls;
static int  s_wm_ctp4_forbidden_75228;
static int  s_wm_ctp4_forbidden_scheduler;
static int  s_wm_ctp4_forbidden_world_loop;

int  wm_ctp4_get_entry(void)              { return s_wm_ctp4_entry; }
int  wm_ctp4_get_85fe0_calls(void)        { return s_wm_ctp4_85fe0_calls; }
u32  wm_ctp4_get_last_cut(void)           { return s_wm_ctp4_last_cut; }
int  wm_ctp4_get_alloc_calls(void)        { return s_wm_ctp4_alloc_calls; }
int  wm_ctp4_get_forbidden_75228(void)    { return s_wm_ctp4_forbidden_75228; }
int  wm_ctp4_get_forbidden_scheduler(void){ return s_wm_ctp4_forbidden_scheduler; }
int  wm_ctp4_get_forbidden_world_loop(void){ return s_wm_ctp4_forbidden_world_loop; }

void wm_common_tail_p4_reset(void)
{
    s_wm_ctp4_entry = 0;
    s_wm_ctp4_85fe0_calls = 0;
    s_wm_ctp4_last_cut = 0;
    s_wm_ctp4_alloc_calls = 0;
    s_wm_ctp4_forbidden_75228 = 0;
    s_wm_ctp4_forbidden_scheduler = 0;
    s_wm_ctp4_forbidden_world_loop = 0;
}

/* ---- P4 forbidden-path stubs ---- */

void wm_p4_80075228_should_not_run(void)  { s_wm_ctp4_forbidden_75228++; }
void wm_p4_80097800_should_not_run(void)  { s_wm_ctp4_forbidden_scheduler++; }
void wm_p4_800712D0_should_not_run(void)  { s_wm_ctp4_forbidden_world_loop++; }


/* ---- 0x80085FE0 production implementation ---- */

/* Exact native transcription of retail 0x80085FE0–0x80086124.
 *
 * Allocates two 20480-byte (0x5000) buffers via HeapAlloc.
 * Stores pointers at D_8009D7E8 and D_8009D7EC.
 *
 * Initializes 512 records (40 bytes each) in the first buffer.
 * Record base = alloc_ptr + 22.  Per-record writes:
 *   byte[-19] = 9    (type marker)
 *   byte[-18] = 128  (R)
 *   byte[-17] = 128  (G)
 *   byte[-16] = 128  (B)
 *   byte[-15] = 44   (code byte)
 *   byte[-10] = 0
 *   byte[-9]  = 64   (0x40)
 *   byte[-2]  = 31   (0x1F)
 *   byte[-1]  = 64   (0x40)
 *   byte[+6]  = 0
 *   byte[+7]  = 111  (0x6F)
 *   byte[+14] = 31   (0x1F)
 *   byte[+15] = 111  (0x6F)
 *   hw[-8]    = GetClut(240, 511)
 *   hw[+0]    = GetTPage(0, 0, 896, 256)
 *
 * Then copies first buffer → second buffer (20480 bytes, 16-byte chunks).
 *
 * Calls: HeapAlloc, PsyQ GetTPage, PsyQ GetClut. */
void wm_80085FE0(void)
{
    void *p1_host, *p2_host;
    u32 ptr1, ptr2;
    u16 tpage_val, clut_val;
    u32 base;
    int i;

    /* 0x80085FE4–0x8008600C: first HeapAlloc(0x5000, 1) */
    p1_host = HeapAlloc(WM_85FE0_ALLOC_SIZE, 1);
    ptr1 = wm_host_to_psx(p1_host);
    s_wm_ctp4_alloc_calls++;

    /* 0x8008600C–0x80086020: second HeapAlloc(0x5000, 1)
     * Delay slot stores ptr1 at D_8009D7E8 before the call. */
    WM_U32(WM_D_8009D7E8_ABS) = ptr1;

    p2_host = HeapAlloc(WM_85FE0_ALLOC_SIZE, 1);
    ptr2 = wm_host_to_psx(p2_host);
    s_wm_ctp4_alloc_calls++;

    /* 0x80086040: store ptr2 at D_8009D7EC. */
    WM_U32(WM_D_8009D7EC_ABS) = ptr2;

    fprintf(stderr,
            "[wm-80085FE0] alloc ptr1=0x%08x ptr2=0x%08x\n",
            ptr1, ptr2);

    /* 0x80086048–0x800860B8: initialize 512 records.
     * Base = ptr1 + 22.  Stride = 40.  Count = 512. */
    base = ptr1 + WM_85FE0_RECORD_BASE_OFFSET;
    for (i = 0; i < WM_85FE0_RECORD_COUNT; i++) {
        /* 0x80086054: sb $s2, -19($s0) → byte[-19] = 9 */
        WM_U8(base - 19) = 9;

        /* 0x80086060: sb $s2, -18($s0) → byte[-18] = 128 */
        WM_U8(base - 18) = 128;

        /* 0x80086064: sb $s2, -17($s0) → byte[-17] = 128 */
        WM_U8(base - 17) = 128;

        /* 0x80086068: sb $s2, -16($s0) → byte[-16] = 128 */
        WM_U8(base - 16) = 128;

        /* 0x8008605C: sb $s2, -15($s0) → byte[-15] = 44 */
        WM_U8(base - 15) = 44;

        /* 0x8008606C: sb $zero, -10($s0) → byte[-10] = 0 */
        WM_U8(base - 10) = 0;

        /* 0x80086070: sb $s5, -9($s0) → byte[-9] = 64 */
        WM_U8(base - 9) = 64;

        /* 0x80086074: sb $s4, -2($s0) → byte[-2] = 31 */
        WM_U8(base - 2) = 31;

        /* 0x80086078: sb $s5, -1($s0) → byte[-1] = 64 */
        WM_U8(base - 1) = 64;

        /* 0x8008607C: sb $zero, 6($s0) → byte[6] = 0 */
        WM_U8(base + 6) = 0;

        /* 0x80086080: sb $s3, 7($s0) → byte[7] = 111 */
        WM_U8(base + 7) = 111;

        /* 0x80086084: sb $s4, 14($s0) → byte[14] = 31 */
        WM_U8(base + 14) = 31;

        /* 0x8008608C: sb $s3, 15($s0) → byte[15] = 111 */
        WM_U8(base + 15) = 111;

        /* 0x80086088: jal GetClut(240, 511) */
        clut_val = GetClut(240, 511);

        /* 0x800860A4 (delay of jal GetTPage): sh $v0, -8($s0)
         * $v0 = GetClut result.  Store at hw[-8]. */
        *(u16*)PSX_ADDR(base - 8) = clut_val;

        /* 0x80086090..0x800860A0: a0=0, a1=0, a2=896, a3=256. */
        tpage_val = GetTPage(0, 0, 896, 256);

        /* 0x800860AC (after GetTPage returns): sh $v0, 0($s0)
         * $v0 = GetTPage result.  Store at hw[0]. */
        *(u16*)PSX_ADDR(base + 0) = tpage_val;

        base += WM_85FE0_RECORD_STRIDE;
    }

    /* 0x800860BC–0x800860F8: copy ptr1 → ptr2 (20480 bytes, 16-byte chunks). */
    {
        u32 src = ptr1;
        u32 dst = ptr2;
        u32 end = ptr1 + WM_85FE0_ALLOC_SIZE;
        while (src != end) {
            WM_U32(dst + 0)  = WM_U32(src + 0);
            WM_U32(dst + 4)  = WM_U32(src + 4);
            WM_U32(dst + 8)  = WM_U32(src + 8);
            WM_U32(dst + 12) = WM_U32(src + 12);
            src += WM_85FE0_COPY_CHUNK;
            dst += WM_85FE0_COPY_CHUNK;
        }
    }

    fprintf(stderr,
            "[wm-80085FE0] init+copy done\n");
}


/* ---- Common-tail P4: caller slice ---- */

u32 wm_80072954_common_tail_p4(void)
{
    /* P4 slice: 0x80072954..0x80072958
     * jal 0x80085FE0 + delay slot (nop).
     * First excluded: 0x8007295C (next instruction in caller).
     *
     * Requires P3 to have executed and returned 0x80072954. */
    u32 cut;

    s_wm_ctp4_entry++;

    /* 0x80072954: jal 0x80085FE0 */
    fprintf(stderr,
            "[worldmap-common-tail-p4] calling wm_80085FE0\n");
    wm_80085FE0();
    s_wm_ctp4_85fe0_calls++;

    /* 0x80072958: nop (delay slot — no effect)
     * Return cut at 0x8007295C (first excluded = next instruction). */
    cut = WM_COMMON_TAIL_P4_CUT;
    s_wm_ctp4_last_cut = cut;

    fprintf(stderr,
            "[worldmap-common-tail-p4] exit cut=0x%08x\n", cut);
    return cut;
}


/* ---- Common-tail P5 instrumentation ---- */

static int  s_wm_ctp5_entry;
static int  s_wm_ctp5_c894_zero;
static int  s_wm_ctp5_c894_nonzero;
static int  s_wm_ctp5_75228_calls;
static int  s_wm_ctp5_palette_calls;
static u32  s_wm_ctp5_last_cut;
static int  s_wm_ctp5_forbidden_scheduler;
static int  s_wm_ctp5_forbidden_world_loop;
static int  s_wm_ctp5_forbidden_drawotag;

int  wm_ctp5_get_entry(void)              { return s_wm_ctp5_entry; }
int  wm_ctp5_get_c894_zero(void)          { return s_wm_ctp5_c894_zero; }
int  wm_ctp5_get_c894_nonzero(void)       { return s_wm_ctp5_c894_nonzero; }
int  wm_ctp5_get_75228_calls(void)        { return s_wm_ctp5_75228_calls; }
int  wm_ctp5_get_palette_calls(void)      { return s_wm_ctp5_palette_calls; }
u32  wm_ctp5_get_last_cut(void)           { return s_wm_ctp5_last_cut; }
int  wm_ctp5_get_forbidden_scheduler(void){ return s_wm_ctp5_forbidden_scheduler; }
int  wm_ctp5_get_forbidden_world_loop(void){ return s_wm_ctp5_forbidden_world_loop; }
int  wm_ctp5_get_forbidden_drawotag(void) { return s_wm_ctp5_forbidden_drawotag; }

void wm_common_tail_p5_reset(void)
{
    s_wm_ctp5_entry = 0;
    s_wm_ctp5_c894_zero = 0;
    s_wm_ctp5_c894_nonzero = 0;
    s_wm_ctp5_75228_calls = 0;
    s_wm_ctp5_palette_calls = 0;
    s_wm_ctp5_last_cut = 0;
    s_wm_ctp5_forbidden_scheduler = 0;
    s_wm_ctp5_forbidden_world_loop = 0;
    s_wm_ctp5_forbidden_drawotag = 0;
}

/* ---- P5 forbidden-path stubs ---- */

void wm_p5_80097800_should_not_run(void)  { s_wm_ctp5_forbidden_scheduler++; }
void wm_p5_800712D0_should_not_run(void)  { s_wm_ctp5_forbidden_world_loop++; }
void wm_p5_DrawOTag_should_not_run(void)  { s_wm_ctp5_forbidden_drawotag++; }


/* ---- 0x80075228 production implementation ---- */

/* Exact native transcription of retail 0x80075228–0x80075288.
 *
 * Zeroes 16 halfwords at D_8009C872 (loop: 16 iterations, stride -2).
 * Reads button state at 0x8006EE68.
 * Sets D_8009D64C = 1.
 * Sets D_8009BE40 = 768 (if button 0x4000 set) or 384 (if clear).
 * Sets D_8009BCC4 = 1.
 * Sets D_8009D80C = 0.
 *
 * Leaf function — no external calls. */
void wm_80075228(void)
{
    int i;
    u32 addr;
    u16 buttons;

    /* 0x80075228–0x80075240: zero 16 halfwords at D_8009C872.
     * Loop: v1=15 down to 0, v0 starts at D_8009C872, decrements by 2. */
    addr = WM_D_8009C872_ABS;
    for (i = 0; i < WM_75228_HALFWORD_COUNT; i++) {
        WM_U16(addr) = 0;
        addr -= 2;
    }

    /* 0x80075244–0x80075248: load button state. */
    buttons = WM_U16(WM_BUTTONS_ABS);

    /* 0x8007524C–0x80075254: D_8009D64C = 1. */
    WM_U32(WM_D_8009D64C_ABS) = 1;

    /* 0x80075258–0x80075264: check button 0x4000, set D_8009BE40. */
    if (buttons & WM_75228_FLAG_CROSS) {
        WM_U32(WM_D_8009BE40_ABS) = WM_75228_VALUE_CROSS;   /* 768 */
    } else {
        WM_U32(WM_D_8009BE40_ABS) = WM_75228_VALUE_NO_CROSS; /* 384 */
    }

    /* 0x80075270–0x80075278: D_8009BCC4 = 1. */
    WM_U32(WM_D_8009BCC4_ABS) = 1;

    /* 0x8007527C–0x80075280: D_8009D80C = 0. */
    WM_U32(WM_D_8009D80C_ABS) = 0;

    /* 0x80075284: jr $ra / 0x80075288: nop (return). */
}


/* ---- Common-tail P5: caller slice ---- */

/* SystemTransferPaletteToVRAM: declared in src/slus_006.64/system/system.c. */
extern void SystemTransferPaletteToVRAM(short xDest, short yDest);

u32 wm_8007295C_common_tail_p5(void)
{
    /* P5 slice: 0x8007295C..0x80072998
     * Reads C894 ready flag, branches:
     *   C894 == 0: calls wm_80075228, then falls through.
     *   C894 != 0: skips wm_80075228, falls through.
     * Both paths: call SystemTransferPaletteToVRAM(0x130, 0x1E0), epilogue.
     * Slot-1 callback completes at 0x80072998 (jr $ra).
     * Returns overlay-local sentinel 0x8007299C (slot-2 entry).
     * Actual post-slot-1 return PC: 0x80071064 (WorldMapMain resumes).
     *
     * Requires P4 to have executed and returned 0x8007295C. */
    u32 c894;
    u32 cut;

    s_wm_ctp5_entry++;

    /* 0x8007295C–0x80072960: load C894 ready flag. */
    c894 = WM_U32(WM_FLAG_C894_ABS);
    fprintf(stderr,
            "[worldmap-common-tail-p5] entry C894=%u\n", c894);

    /* 0x80072968: bne $v0, $zero, 0x8007297C */
    if (c894 != 0) {
        /* C894 != 0: skip wm_80075228, go directly to convergence. */
        s_wm_ctp5_c894_nonzero++;
        fprintf(stderr,
                "[worldmap-common-tail-p5] C894!=0 → skip wm_80075228\n");
    } else {
        /* C894 == 0: natural Lahan path. Call wm_80075228. */
        s_wm_ctp5_c894_zero++;
        fprintf(stderr,
                "[worldmap-common-tail-p5] C894=0 → calling wm_80075228\n");
        wm_80075228();
        s_wm_ctp5_75228_calls++;
    }

    /* Convergence: both paths call SystemTransferPaletteToVRAM.
     * 0x8007297C: jal 0x80033698
     * 0x80072980: addiu $a1, $zero, 0x1E0 (delay slot) */
    fprintf(stderr,
            "[worldmap-common-tail-p5] calling SystemTransferPaletteToVRAM"
            "(0x%x, 0x%x)\n",
            WM_PALETTE_ARCHIVE_IDX, WM_PALETTE_Y_POS);
    SystemTransferPaletteToVRAM((short)WM_PALETTE_ARCHIVE_IDX,
                                (short)WM_PALETTE_Y_POS);
    s_wm_ctp5_palette_calls++;

    /* 0x80072984–0x80072998: epilogue (restore $ra/$s1/$s0, deallocate, return).
     * Return overlay-local sentinel at 0x8007299C (slot-2 entry / first excluded).
     * Actual retail return PC is 0x80071064. */
    cut = WM_COMMON_TAIL_P5_CUT;
    s_wm_ctp5_last_cut = cut;

    fprintf(stderr,
            "[worldmap-common-tail-p5] exit cut=0x%08x\n", cut);
    return cut;
}

/* Entry for the matched C in src/world_map/main.c (0x80089160 is not
 * matched yet): the vector and the optional third argument arrive as host
 * pointers into emulated RAM (NULL stays 0). */
void func_80089160(s32 id, void* vector, void* flags)
{
    wm_80089160((u32)id, PsxMemory_GuestAddr(vector),
                flags != NULL ? PsxMemory_GuestAddr(flags) : 0u);
}
