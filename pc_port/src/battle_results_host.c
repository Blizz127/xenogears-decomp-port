/* battle_results_host.c -- native bodies for adopted routines of the retail
 * battle RESULTS module (0x801DE000..0x801E4500).
 *
 * A body here runs only when battle_mips_runtime.c's identity gate has
 * proved that the retail results module is the code resident at its address
 * (battle_results_gate.inc), and only for routines listed in
 * battle_results_host_leaves.inc.  Every other results routine is interpreted.
 *
 * Each body is a 1:1 rendering of the retail instruction stream -- same
 * loads and stores, same order, same widths -- through the guest accessors in
 * battle_results_host.h (never PSX_ADDR; see the address rule there).  The
 * listing references are to the module linked at 0x801DE000.
 *
 * Proof: pc_port/tests/run_battle_results_host_differential_test.sh.
 */

#include "battle_results_host.h"

/* func_801E24B0 (0x801E24B0..0x801E2528, leaf, 31 instructions).
 * Called once, from the results entry routine at 0x801E25F0 (jal, nop delay
 * slot); no branch, J or data word targets it.
 *
 * For each of the 48 result-id bytes D_800D2FE4[j] that is nonzero, every one
 * of the 150 entries g_GameState+0x2026 (0x8006F65A) equal to it gets
 * D_800D2CB0[j] stored to the parallel byte at g_GameState+0x1F90
 * (0x8006F5C4).  A later j overwrites an earlier one.  0x8006F5C4/0x8006F65A
 * are inside the native g_GameState copy; D_800D2FE4/D_800D2CB0 are battle
 * BSS in guest RAM -- which is why the accessors, not PSX_ADDR, are required.
 *
 *   24B0  a2 = 0 ; a1 = 0x800D2FE4
 *   24BC  v0 = lbu 0(a1) ; beq v0, 0 -> 2514
 *   24CC  a0 = 0
 *   24D0  v1 = lbu 0(a1)               (re-read every inner iteration)
 *   24DC  v0 = lbu 0x8006F65A(a0) ; bne v1, v0 -> 2504
 *   24F4  v0 = lbu 0x800D2CB0(a2) ; sb v0, 0x8006F5C4(a0)
 *   2504  a0++ ; slti a0 < 150 -> 24D0
 *   2514  a2++ ; slti a2 < 48 -> 24BC (delay: a1++)
 *   2524  jr ra
 *
 * The id is re-read inside the inner loop exactly as retail does; the store
 * range (0x8006F5C4..0x8006F659) never overlaps the id table, so this changes
 * no value, but it keeps the access sequence identical.  Reads are pinned to
 * temporaries so C's unspecified operand order cannot reorder them.
 *
 * v0 on return: the final `slti v0, a2, 48` with a2 == 48, i.e. always 0.
 * Other exit registers, reproduced exactly (the caller's victory path goes on
 * to jal 0x800BFBA0 / 0x801E2280 without setting arguments, so they are not
 * provably dead):
 *   a1 = 0x800D2FE4 + 48, a2 = 48                       always;
 *   a0 = 150, v1 = the last nonzero id, at = 0x80070000 + 149
 *                                  only if some id was nonzero (else as on entry). */
uint32_t battle_results_func_801E24B0(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    int32_t j;
    int32_t i;
    int inner_ran = 0;
    uint8_t last_id = 0;

    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    for (j = 0; j < 48; j++) {
        if (BR_LBU(0x800D2FE4u + (uint32_t)j) == 0)
            continue;
        inner_ran = 1;
        for (i = 0; i < 150; i++) {
            uint8_t id = BR_LBU(0x800D2FE4u + (uint32_t)j);
            last_id = id;
            uint8_t entry = BR_LBU(0x8006F65Au + (uint32_t)i);
            if (id == entry) {
                uint8_t value = BR_LBU(0x800D2CB0u + (uint32_t)j);
                BR_SB(0x8006F5C4u + (uint32_t)i, value);
            }
        }
    }
    BattleResults_SetExitReg(5, 0x800D2FE4u + 48u);
    BattleResults_SetExitReg(6, 48u);
    if (inner_ran) {
        BattleResults_SetExitReg(1, 0x80070000u + 149u);
        BattleResults_SetExitReg(3, last_id);
        BattleResults_SetExitReg(4, 150u);
    }
    return 0;
}

/* ---- MIPS arithmetic, exactly as the R3000A (and the interpreter's
 * battle_mips_adapter.c) define it.  No host divide by zero, no signed
 * overflow, no implementation-defined signed shifts. ---------------------- */

/* slt/slti: signed 32-bit compare without converting to a signed type. */
static int br_slt(uint32_t a, uint32_t b)
{
    return (a ^ 0x80000000u) < (b ^ 0x80000000u);
}

/* sra rd, rt, n (n in 1..31). */
static uint32_t br_sra(uint32_t value, unsigned n)
{
    uint32_t shifted = value >> n;
    if (value & 0x80000000u)
        shifted |= ~(0xFFFFFFFFu >> n);
    return shifted;
}

/* Two's-complement value of a guest word, as int64 (no conversion of an
 * out-of-range unsigned value to a signed type). */
static int64_t br_s64(uint32_t value)
{
    return (value & 0x80000000u) ? (int64_t)value - INT64_C(0x100000000)
                                 : (int64_t)value;
}

/* mult rs, rt: signed 32x32 -> 64 into HI:LO. */
static void br_mult(uint32_t rs, uint32_t rt, uint32_t *hi, uint32_t *lo)
{
    uint64_t product = (uint64_t)(br_s64(rs) * br_s64(rt));
    *lo = (uint32_t)product;
    *hi = (uint32_t)(product >> 32);
}

/* multu rs, rt: unsigned 32x32 -> 64 into HI:LO. */
static void br_multu(uint32_t rs, uint32_t rt, uint32_t *hi, uint32_t *lo)
{
    uint64_t product = (uint64_t)rs * (uint64_t)rt;
    *lo = (uint32_t)product;
    *hi = (uint32_t)(product >> 32);
}

/* div rs, rt (signed).  R3000A results the interpreter reproduces: rt == 0
 * gives LO = (rs >= 0 ? 0xFFFFFFFF : 1), HI = rs; 0x80000000 / -1 gives
 * LO = 0x80000000, HI = 0; otherwise C99 truncating quotient/remainder. */
static void br_div(uint32_t rs, uint32_t rt, uint32_t *hi, uint32_t *lo)
{
    int64_t dividend = br_s64(rs);
    int64_t divisor = br_s64(rt);

    if (divisor == 0) {
        *lo = dividend >= 0 ? 0xFFFFFFFFu : 1u;
        *hi = rs;
    } else if (rs == 0x80000000u && rt == 0xFFFFFFFFu) {
        *lo = 0x80000000u;
        *hi = 0;
    } else {
        /* In int64 neither the quotient nor the remainder can overflow. */
        *lo = (uint32_t)(uint64_t)(dividend / divisor);
        *hi = (uint32_t)(uint64_t)(dividend % divisor);
    }
}

/* divu rs, rt.  R3000A results the interpreter reproduces: rt == 0 gives
 * LO = 0xFFFFFFFF, HI = rs; otherwise the unsigned quotient/remainder. */
static void br_divu(uint32_t rs, uint32_t rt, uint32_t *hi, uint32_t *lo)
{
    if (rt == 0) {
        *lo = 0xFFFFFFFFu;
        *hi = rs;
    } else {
        *lo = rs / rt;
        *hi = rs % rt;
    }
}

/* srav rd, rt, rs: arithmetic shift by the low 5 bits of rs. */
static uint32_t br_srav(uint32_t value, uint32_t amount)
{
    amount &= 31u;
    return amount == 0 ? value : br_sra(value, amount);
}

/* func_801DF910 (0x801DF910..0x801DFA34, leaf, 74 instructions, no frame).
 * Called once, from 0x801E04C4 (jal, nop delay slot) in 0x801E03FC's row
 * loop, with a0/a1 = two lbu'd bytes and a2 = 0x801E03FC's saved copy of
 * func_801DFA38's result (0..255 there; the body takes any 32-bit a2).  No
 * branch, J or data word targets it.
 *
 * Percent-bar setup into the mutable module words (guest RAM):
 *   44CC = a0&0xFF   44D0 = a1&0xFF   44D4 = |a1-a0| (a1-a0 first, then
 *   rewritten when negative)   44D8/44DC = q*6400/10000 via the magic
 *   0x68DB8BAD (mult; HI >> 12, minus the sign), q = 100*x / a2 by DIV
 *   44E0 = 2/3   44E4 = 0xE3/0xE5   (u8, by sign of a1-a0)
 *
 *   F910  a3 = a0 & 0xFF ; v0 = a3*100 ; div v0, a2 ; v0 = lo
 *   F930  a0 = 0x68DB8BAD ; v1 = v0*6400 ; mult v1, a0 ; a1 &= 0xFF
 *   F954  sw a3 -> 44CC ; sw a1 -> 44D0 ; a0 = a1-a3 ; v1 = sra v1,31
 *   F970  sw a0 -> 44D4 ; v0 = sra(hi,12) - v1 ; sw v0 -> 44D8
 *   F988  bltz a0 -> F9AC (delay v0 = 2)
 *   F990    sb 2 -> 44E0 ; sb 0xE3 -> 44E4 ; j F9D0
 *   F9AC    sb 3 -> 44E0 ; sb 0xE5 -> 44E4 ; sw a3-a1 -> 44D4
 *   F9D0  v0 = lw 44D4 (re-read) ; v1 = v0*100 ; div v1, a2 ; v1 = lo
 *   F9F8  a0 = 0x68DB8BAD ; v0 = v1*6400 ; mult v0, a0 ; v0 = sra v0,31
 *   FA1C  v1 = sra(hi,12) - v0 ; sw v1 -> 44DC ; jr ra
 * There is no divide-by-zero trap check in the bytes: a2 == 0 yields the
 * R3000A quotient (0xFFFFFFFF for these non-negative dividends), reproduced
 * by br_div; 44D4 is re-read from guest RAM exactly as retail does.
 *
 * v0 on return: sra(second v0*6400 product, 31).
 * Exit registers, always: v1 = the 44DC value, a0 = 0x68DB8BAD,
 * a1 = a1&0xFF, a3 = a0&0xFF, at = 0x801E0000, HI:LO = the second mult's
 * product.  a2 and every other register are unchanged. */
uint32_t battle_results_func_801DF910(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t hi, lo;
    uint32_t v0, v1, r_a0, r_a1, r_a3;

    (void)a3;
    r_a3 = a0 & 0xFFu;
    v0 = r_a3 * 100u;
    br_div(v0, a2, &hi, &lo);
    v0 = lo;
    r_a0 = 0x68DB8BADu;
    v1 = v0 * 6400u;
    br_mult(v1, r_a0, &hi, &lo);
    r_a1 = a1 & 0xFFu;
    BR_SW(0x801E44CCu, r_a3);
    BR_SW(0x801E44D0u, r_a1);
    r_a0 = r_a1 - r_a3;
    v1 = br_sra(v1, 31);
    BR_SW(0x801E44D4u, r_a0);
    v0 = br_sra(hi, 12) - v1;
    BR_SW(0x801E44D8u, v0);
    if (r_a0 & 0x80000000u) {
        BR_SB(0x801E44E0u, 3u);
        BR_SB(0x801E44E4u, 0xE5u);
        BR_SW(0x801E44D4u, r_a3 - r_a1);
    } else {
        BR_SB(0x801E44E0u, 2u);
        BR_SB(0x801E44E4u, 0xE3u);
    }
    v0 = BR_LW(0x801E44D4u);
    v1 = v0 * 100u;
    br_div(v1, a2, &hi, &lo);
    v1 = lo;
    r_a0 = 0x68DB8BADu;
    v0 = v1 * 6400u;
    br_mult(v0, r_a0, &hi, &lo);
    v0 = br_sra(v0, 31);
    v1 = br_sra(hi, 12) - v0;
    BR_SW(0x801E44DCu, v1);

    BattleResults_SetExitReg(1, 0x801E0000u);
    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitHiLo(hi, lo);
    return v0;
}

/* func_801DFA38 (0x801DFA38..0x801DFAA4, leaf, 28 instructions, no frame).
 * Called once, from 0x801E042C (jal; delay slot sw s1 to 0x801E03FC's own
 * frame); no branch, J or data word targets it.
 *
 * Returns the maximum of the 14 bytes D_800CDD10[8*i .. 8*i+6] and
 * D_800CDD28[8*i .. 8*i+6], i = a0 & 0xFF (battle BSS, guest RAM), taken
 * pairwise in the retail order with the winning byte re-read:
 *
 *   FA38  a2 = 0 ; a0 = (a0&0xFF) << 3 ; v1 = 0x800CDD10
 *   FA4C  a1 = a0 + v1 + 24 ; a0 += v1 ; v1 = a1 + 7
 *   FA60  v0 = lbu 0(a0) ; if !(v0 <u a2) a2 = lbu 0(a0)
 *   FA78  v0 = lbu 0(a1) ; if !(v0 <u a2) a2 = lbu 0(a1)   (delay: a0++)
 *   FA90  a1++ ; slt a1 < v1 -> FA60 ; jr ra (delay: v0 = a2)
 *
 * v0 on return: the maximum (0..255).
 * Exit registers, always: a0 = 0x800CDD10 + 8*i + 7, a1 = v1 =
 * 0x800CDD28 + 8*i + 7, a2 = the maximum. */
uint32_t battle_results_func_801DFA38(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t r_a0, r_a1, r_a2, v0, v1;

    (void)a1;
    (void)a2;
    (void)a3;
    r_a2 = 0;
    r_a0 = (a0 & 0xFFu) << 3;
    v1 = 0x800CDD10u;
    r_a1 = r_a0 + (v1 + 24u);
    r_a0 = r_a0 + v1;
    v1 = r_a1 + 7u;
    do {
        v0 = BR_LBU(r_a0);
        if (!(v0 < r_a2))
            r_a2 = BR_LBU(r_a0);
        v0 = BR_LBU(r_a1);
        if (!(v0 < r_a2))
            r_a2 = BR_LBU(r_a1);
        r_a0++;
        r_a1++;
    } while (br_slt(r_a1, v1));

    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    return r_a2;
}

/* func_801E1590 (0x801E1590..0x801E168C, leaf, 64 instructions, no frame).
 * Called once, from 0x801E16C4 (jal; delay slot sw s0 to 0x801E1690's
 * frame), with a0 = sp+24 (ids[8]), a1 = sp+40 (counts[8]), a2 = sp+32
 * (kinds[8]) in 0x801E1690's guest frame; no branch, J or data word targets
 * it.  The body takes the pointers as raw guest words and reaches them only
 * through the accessors, so the caller's guest frame is what it reads and
 * writes.
 *
 * Clears ids[0..7] and counts[0..7] (interleaved, ids first; kinds is NOT
 * cleared), then for each k in 0..7 with D_800CDCFC[k] (id) != 0 searches all
 * 8 slots for kinds[j] == D_800CDCF4[k] && ids[j] == id: a hit increments
 * counts[j]; a miss appends (kind, id) at the cursor (t6/t7) and increments
 * counts at the count cursor t5.  Stale kinds[] bytes are compared (retail
 * reads them; the ids[] test then rejects them since ids[] were zeroed and the
 * id is nonzero) -- reproduced read for read.
 *
 *   1590  v1 = a1 ; a3 = a0 ; t0 = a1+8
 *   159C  sb 0,(a3) ; sb 0,(v1) ; v1++ ; slt v1 < t0 -> 159C (delay a3++)
 *   15B4  t9 = 8 ; t3 = 0x800CDCFC ; t4 = t3-8 ; t8 = t3+8
 *         t5 = a1 ; t7 = a0 ; t6 = a2
 *   15D4  v0 = lbu (t3) ; beq v0,0 -> 1678 (delay t2 = 0)
 *   15E4  t1 = a1 ; t0 = a0 ; a3 = a2
 *   15F0  v1 = lbu (t4) ; v0 = lbu (a3) ; bne -> 162C
 *         v1 = lbu (t3) ; v0 = lbu (t0) ; bne -> 162C
 *         v0 = lbu (t1) + 1 ; j 1644 (delay sb v0,(t1))
 *   162C  t1++ ; t0++ ; t2++ ; slti t2 < 8 -> 15F0 (delay a3++)
 *   1644  bne t2, t9 -> 1678
 *         sb lbu(t4),(t6) ; sb lbu(t3),(t7) ; t6++ ; t7++
 *         sb lbu(t5)+1,(t5) ; t5++
 *   1678  t3++ ; slt t3 < t8 -> 15D4 (delay t4++) ; jr ra
 * The pointer compares are signed (slt), as reproduced by br_slt.
 *
 * v0 on return: the final slt, i.e. always 0.
 * Exit registers, always: v1, a3, t0 (the last values the stream above left
 * in them), t2 (0 if D_800CDCFC[7] == 0, else the last slot index reached),
 * t3 = t8 = 0x800CDD04, t4 = 0x800CDCFC, t5/t6/t7 = the cursors, t9 = 8;
 * t1 only if some id was nonzero (else as on entry). */
uint32_t battle_results_func_801E1590(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t v0, v1, r_a3, t0, t1 = 0, t2, t3, t4, t5, t6, t7, t8, t9;
    int t1_written = 0;

    (void)a3;
    v1 = a1;
    r_a3 = a0;
    t0 = a1 + 8u;
    do {
        BR_SB(r_a3, 0u);
        BR_SB(v1, 0u);
        v1++;
        v0 = (uint32_t)br_slt(v1, t0);
        r_a3++;
    } while (v0 != 0);
    t9 = 8u;
    t3 = 0x800CDCFCu;
    t4 = t3 - 8u;
    t8 = t3 + 8u;
    t5 = a1;
    t7 = a0;
    t6 = a2;
    do {
        v0 = BR_LBU(t3);
        t2 = 0;
        if (v0 != 0) {
            t1 = a1;
            t0 = a0;
            r_a3 = a2;
            t1_written = 1;
            for (;;) {
                v1 = BR_LBU(t4);
                v0 = BR_LBU(r_a3);
                if (v1 == v0) {
                    v1 = BR_LBU(t3);
                    v0 = BR_LBU(t0);
                    if (v1 == v0) {
                        v0 = BR_LBU(t1) + 1u;
                        BR_SB(t1, v0);
                        break;
                    }
                }
                t1++;
                t0++;
                t2++;
                v0 = (uint32_t)br_slt(t2, 8u);
                r_a3++;
                if (v0 == 0)
                    break;
            }
            if (t2 == t9) {
                v0 = BR_LBU(t4);
                BR_SB(t6, v0);
                v0 = BR_LBU(t3);
                t6++;
                BR_SB(t7, v0);
                v0 = BR_LBU(t5);
                t7++;
                v0 = v0 + 1u;
                BR_SB(t5, v0);
                t5++;
            }
        }
        t3++;
        v0 = (uint32_t)br_slt(t3, t8);
        t4++;
    } while (v0 != 0);

    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitReg(8, t0);
    if (t1_written)
        BattleResults_SetExitReg(9, t1);
    BattleResults_SetExitReg(10, t2);
    BattleResults_SetExitReg(11, t3);
    BattleResults_SetExitReg(12, t4);
    BattleResults_SetExitReg(13, t5);
    BattleResults_SetExitReg(14, t6);
    BattleResults_SetExitReg(15, t7);
    BattleResults_SetExitReg(24, t8);
    BattleResults_SetExitReg(25, t9);
    return v0;
}

/* func_801E2888 (0x801E2888..0x801E2AC8, leaf, 145 instructions).
 * Called once, from 0x801E2834 (jal, nop delay slot) in the results flow
 * routine 0x801E2794; no branch, J or data word targets it.  Its 8-byte
 * frame (addiu sp,-8 / +8) holds nothing: no instruction addresses sp.
 *
 * For the three party slots k (D_800D2D24[k] != 0xFF), with
 *   S  = *(u32 *)0x801E44C8 + 0x170*k            (a2; t1 = S + 0xA4)
 *   A  = *(u32 *)0x801E44C4 + 620  + 164*S[0x56] (a1)
 *   B  = *(u32 *)0x801E44C4 + 2424 + 164*S[0xA0] (a3)
 * it copies/clamps the displayed values:
 *   S[0x56] == 7 && (D_800CCE42[0x170*k] & 0x80):
 *     S.u16[0x4C] = (S.u32[0x104] + 1) / 100  (multu 0x51EB851F, HI >> 4),
 *     re-read, 1 if it came out 0
 *   A.u16[0x4C] = S.u16[0x4C] ; A.u16[0x50] = S.u16[0x50]  (store in the
 *     beq delay slot, after the 0x4C/0x4E/0x50 loads)
 *   A.u16[0x4C] = min(.., A.u16[0x4E]) ; A.u16[0x50] = min(.., A.u16[0x52])
 *   A.u16[0x90 + 2j] = S.u16[0x90 + 2j], j = 0..6
 *   A.u16[0x3A] = S.u16[0x3A] ; S.u16[0x7C] & 0xC000 -> A.u16[0x4C] = 1
 *   S[0xA0] (re-read) in 0..6 or 8..16:
 *     B.u32[0x60] = T.u32[0x60] (T = S+0xA4) ; B.u16[0x38] = T.u16[0x38]
 *     B.u32[0x60] = min(B.u32[0x60] (re-read), B.u32[0x64]) (a0 = 0x64,
 *       loaded before the 0x60 store)
 *     B.u16[0x38] = min(.., B.u16[0x3A])
 *     T.u16[0x7C] & 0x8000 -> B.u32[0x60] = B.u32[0x64] / 10
 *       (multu 0xCCCCCCCD, HI >> 3)
 * 0x801E44C4 is read once, before the loop; 0x801E44C8 once per slot.  The
 * records are reached through whatever the two words hold, so A, B and S may
 * alias (A == B when S[0x56] == S[0xA0] + 11) -- every load and store is in
 * the retail order, so aliasing behaves as retail.  The bltz on the lbu'd
 * S[0xA0] at 0x2A18 can never be taken.
 *
 * v0 on return: the final `sltiu v0, t2&0xFF, 3` with t2 == 3, i.e. 0.
 * Exit registers:
 *   a0 = 3, t2 = 3, t3 = *(u32 *)0x801E44C4, t4 = 1          always;
 *   at = 0x800D0000 + t0 if slot 2 reached the 0x800CCE42 test, else
 *        0x800D0002;  v1 = the last value the stream left in it (slot 2's
 *        0xFF id if slot 2 is empty)                          always;
 *   a1, a2, a3, t0 (= 0x170*k), t1 for the last non-empty slot,
 *                          only if some slot was non-empty (else as on entry);
 *   HI:LO = the last multu's product, only if a multu ran (else as on entry). */
uint32_t battle_results_func_801E2888(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t hi = 0, lo = 0;
    uint32_t v0, v1, at, r_a0, r_a1 = 0, r_a2 = 0, r_a3 = 0;
    uint32_t t0 = 0, t1 = 0, t2, t3, t4;
    int slot_ran = 0, multiplied = 0, more;

    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    t2 = 0;
    t3 = BR_LW(0x801E44C4u);
    t4 = 1;
    r_a0 = t2 & 0xFFu;
    do {
        at = 0x800D0000u + r_a0;
        v1 = BR_LBU(at + 0x2D24u);
        v0 = 0xFFu;
        if (v1 != v0) {
            slot_ran = 1;
            v0 = r_a0 << 1;
            v0 = v0 + r_a0;
            v0 = v0 << 3;
            v0 = v0 - r_a0;
            v1 = BR_LW(0x801E44C8u);
            t0 = v0 << 4;
            r_a2 = t0 + v1;
            r_a0 = BR_LBU(r_a2 + 0x56u);
            v1 = BR_LBU(r_a2 + 0xA0u);
            r_a1 = r_a0 * 164u + 620u + t3;
            r_a3 = v1 * 164u + 2424u + t3;
            v0 = 7u;
            t1 = r_a2 + 164u;
            if (r_a0 == v0) {
                at = 0x800D0000u + t0;
                v0 = BR_LBU(at - 0x31BEu) & 0x80u;
                v1 = 0x51EB0000u;
                if (v0 != 0) {
                    v0 = BR_LW(r_a2 + 0x104u);
                    v1 = 0x51EB851Fu;
                    v0 = v0 + 1u;
                    br_multu(v0, v1, &hi, &lo);
                    multiplied = 1;
                    v0 = hi >> 4;
                    BR_SH(r_a2 + 0x4Cu, v0);
                    v0 = BR_LHU(r_a2 + 0x4Cu);
                    if (v0 == 0)
                        BR_SH(r_a2 + 0x4Cu, t4);
                }
            }
            v0 = BR_LHU(r_a2 + 0x4Cu);
            BR_SH(r_a1 + 0x4Cu, v0);
            r_a0 = BR_LHU(r_a1 + 0x4Cu);
            v0 = BR_LHU(r_a1 + 0x4Eu);
            v1 = BR_LHU(r_a2 + 0x50u);
            v0 = v0 < r_a0;
            BR_SH(r_a1 + 0x50u, v1);
            if (v0 != 0) {
                v0 = BR_LHU(r_a1 + 0x4Eu);
                BR_SH(r_a1 + 0x4Cu, v0);
            }
            v1 = BR_LHU(r_a1 + 0x50u);
            v0 = BR_LHU(r_a1 + 0x52u);
            v0 = v0 < v1;
            r_a0 = 0;
            if (v0 != 0) {
                v0 = BR_LHU(r_a1 + 0x52u);
                BR_SH(r_a1 + 0x50u, v0);
            }
            v0 = r_a0 & 0xFFu;
            do {
                r_a0++;
                v0 = v0 << 1;
                v1 = BR_LHU(v0 + r_a2 + 0x90u);
                v0 = v0 + r_a1;
                BR_SH(v0 + 0x90u, v1);
                more = (r_a0 & 0xFFu) < 7u;
                v0 = r_a0 & 0xFFu;
            } while (more);
            v0 = BR_LHU(r_a2 + 0x3Au);
            BR_SH(r_a1 + 0x3Au, v0);
            v0 = BR_LHU(r_a2 + 0x7Cu) & 0xC000u;
            if (v0 != 0)
                BR_SH(r_a1 + 0x4Cu, t4);
            v1 = BR_LBU(r_a2 + 0xA0u);
            if (v1 < 7u || (v1 >= 8u && v1 < 17u)) {
                v0 = BR_LW(t1 + 0x60u);
                r_a0 = BR_LW(r_a3 + 0x64u);
                BR_SW(r_a3 + 0x60u, v0);
                v0 = BR_LW(r_a3 + 0x60u);
                v1 = BR_LHU(t1 + 0x38u);
                v0 = r_a0 < v0;
                BR_SH(r_a3 + 0x38u, v1);
                if (v0 != 0)
                    BR_SW(r_a3 + 0x60u, r_a0);
                v1 = BR_LHU(r_a3 + 0x38u);
                v0 = BR_LHU(r_a3 + 0x3Au);
                v0 = v0 < v1;
                if (v0 != 0) {
                    v0 = BR_LHU(r_a3 + 0x3Au);
                    BR_SH(r_a3 + 0x38u, v0);
                }
                v0 = BR_LHU(t1 + 0x7Cu) & 0x8000u;
                v1 = 0xCCCC0000u;
                if (v0 != 0) {
                    v0 = BR_LW(r_a3 + 0x64u);
                    v1 = 0xCCCCCCCDu;
                    br_multu(v0, v1, &hi, &lo);
                    multiplied = 1;
                    v0 = hi >> 3;
                    BR_SW(r_a3 + 0x60u, v0);
                }
            }
        }
        t2++;
        more = (t2 & 0xFFu) < 3u;
        r_a0 = t2 & 0xFFu;
    } while (more);

    BattleResults_SetExitReg(1, at);
    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(10, t2);
    BattleResults_SetExitReg(11, t3);
    BattleResults_SetExitReg(12, t4);
    if (slot_ran) {
        BattleResults_SetExitReg(5, r_a1);
        BattleResults_SetExitReg(6, r_a2);
        BattleResults_SetExitReg(7, r_a3);
        BattleResults_SetExitReg(8, t0);
        BattleResults_SetExitReg(9, t1);
    }
    if (multiplied)
        BattleResults_SetExitHiLo(hi, lo);
    return 0;
}

/* func_801E403C (0x801E403C..0x801E41B0, leaf, 94 instructions, no frame).
 * Called once, from 0x801E2824 (jal, nop delay slot) in 0x801E2794; no
 * branch, J or data word targets it.
 *
 * Promotes the 11 record states R_i = W + 0x16C0 + 32*i + 0x17 (W = the
 * word at 0x801E44C4, re-read every iteration) one step each, with
 * L_i = (W + 620 + 164*i)[0x62] and T_i = *(u32 *)0x801E44E8 + 0x110*i:
 *   state 3 -> 4 if !(L_i < T_i[0xCC])     state 4 -> 5 if !(L_i < T_i[0xCD])
 *   state 5 -> 6 if !(L_i < T_i[0xCE])
 *   state 6 -> 7 if L_i >= 50 and (g_GameState u16 at 0x8006F8EA & 0x4000)
 *   any other state (7 included) unchanged.
 * 0x8006F8EA is inside the native g_GameState copy; W's records and the
 * 44E8 table are guest RAM -- both only through the accessors.
 *
 *   403C  a3 = 0 ; t3 = 7 ; t2 = 4 ; t1 = 5 ; t0 = 6 ; a1 = a3 & 0xFF
 *   4054  v0 = 164*a1 + 620 ; v1 = 32*a1 + 0x16C0 ; a0 = lw 44C4
 *   407C  a2 = v1 + a0 ; v1 = lbu 0x17(a2) ; (delay) a0 = v0 + a0
 *   4088  7 -> 4198 ; 4 -> 40FC ; >=5: 5 -> 4134, 6 -> 4168 ; 3 -> 40C8
 *   40C8/40FC/4134  v1 = lw 44E8 + 0x110*a1 ; v0 = lbu 0x62(a0) ;
 *                   v1 = lbu 0xCC/0xCD/0xCE(v1) ; sltu -> else sb 4/5/6
 *   4168  v0 = lbu 0x62(a0) ; sltiu 50 ; v0 = lhu 0x8006F8EA & 0x4000 ; sb 7
 *   4198  a3++ ; sltiu (a3&0xFF) < 11 -> 4054 (delay a1 = a3 & 0xFF) ; jr ra
 * (the "j 419C" exits carry the same a3++ in their delay slot).
 *
 * v0 on return: the final sltiu with a3 == 11, i.e. 0.
 * Exit registers, always: a1 = a3 = 11, t0 = 6, t1 = 5, t2 = 4, t3 = 7,
 * a0/a2 = slot 10's L/R base (W + 2260, W + 0x1800), v1 = slot 10's state
 * byte, or its T[0xCC/0xCD/0xCE] threshold if the state was 3/4/5. */
uint32_t battle_results_func_801E403C(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t v0, v1, r_a0, r_a1, r_a2, r_a3, w, table;

    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    r_a3 = 0;
    r_a1 = r_a3 & 0xFFu;
    do {
        v0 = r_a1 * 164u + 620u;
        v1 = (r_a1 << 5) + 0x16C0u;
        w = BR_LW(0x801E44C4u);
        r_a2 = v1 + w;
        v1 = BR_LBU(r_a2 + 0x17u);
        r_a0 = v0 + w;
        if (v1 == 3u || v1 == 4u || v1 == 5u) {
            uint32_t offset = 0xC9u + v1; /* 0xCC, 0xCD, 0xCE */
            table = BR_LW(0x801E44E8u) + r_a1 * 0x110u;
            v0 = BR_LBU(r_a0 + 0x62u);
            v1 = BR_LBU(table + offset);
            if (!(v0 < v1))
                BR_SB(r_a2 + 0x17u, offset - 0xC8u); /* 4, 5, 6 */
        } else if (v1 == 6u) {
            v0 = BR_LBU(r_a0 + 0x62u);
            if (!(v0 < 50u)) {
                v0 = BR_LHU(0x8006F8EAu) & 0x4000u;
                if (v0 != 0)
                    BR_SB(r_a2 + 0x17u, 7u);
            }
        }
        r_a3++;
        v0 = (r_a3 & 0xFFu) < 11u;
        r_a1 = r_a3 & 0xFFu;
    } while (v0 != 0);

    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitReg(8, 6u);
    BattleResults_SetExitReg(9, 5u);
    BattleResults_SetExitReg(10, 4u);
    BattleResults_SetExitReg(11, 7u);
    return v0;
}

/* func_801E41B4 (0x801E41B4..0x801E42C0, leaf, 68 instructions, no frame).
 * Called once, from 0x801E282C (jal, nop delay slot) in 0x801E2794; no
 * branch, J or data word targets it.
 *
 * For the three battle rows k, c = D_800CCD3E[0x170*k] (guest BSS), level
 * byte L = g_GameState byte 0x8006D902 + 164*c (re-read before each test):
 * L >= 50 sets bit 0x8, L >= 60 bit 0x4, L >= 70 bit 0x2 of the u16 at
 * 0x8006ECF8 + 32*c (lhu/ori/sh each, in that order).  Both tables are in the
 * native g_GameState copy for the real character ids.  Every address is
 * formed as lui 0x800D/0x8007 + register + negative offset (wrapping uint32).
 *
 *   41B4  a2 = 0 ; v1 = a2 & 0xFF
 *   41BC  v0 = 0x170*v1 ; at = 0x800D0000 + v0 ; a0 = lbu -0x32C2(at)
 *   41E0  a1 = 164*a0 ; at = 0x80070000 + a1 ; v0 = lbu -0x26FE(at)
 *   4204  sltiu 50 -> 4230 (delay v1 = a0 << 5) ; at = 0x80070000 + v1 ;
 *         lhu/ori 8/sh -0x1308(at)
 *   4230  same for 60 / bit 4 (delay v1 = a0 << 5)
 *   426C  same for 70 / bit 2 (delay a2++ ; v1 = a0 << 5 inside)
 *   42AC  sltiu (a2&0xFF) < 3 -> 41BC (delay v1 = a2 & 0xFF) ; jr ra
 *
 * v0 on return: the final sltiu with a2 == 3, i.e. 0.
 * Exit registers, always: a2 = v1 = 3, a0 = row 2's c, a1 = 164*c,
 * at = 0x80070000 + 32*c if row 2's L >= 70, else 0x80070000 + 164*c. */
uint32_t battle_results_func_801E41B4(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    static const uint8_t kThreshold[3] = {50u, 60u, 70u};
    static const uint16_t kBit[3] = {0x8u, 0x4u, 0x2u};
    uint32_t v0, v1, at, r_a0, r_a1, r_a2;
    unsigned step;

    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    r_a2 = 0;
    v1 = r_a2 & 0xFFu;
    do {
        v0 = v1 * 0x170u;
        at = 0x800D0000u + v0;
        r_a0 = BR_LBU(at - 0x32C2u);
        r_a1 = r_a0 * 164u;
        for (step = 0; step < 3; step++) {
            at = 0x80070000u + r_a1;
            v0 = BR_LBU(at - 0x26FEu);
            if (step == 2)
                r_a2++;
            v1 = r_a0 << 5;
            if (!(v0 < kThreshold[step])) {
                at = 0x80070000u + v1;
                v0 = BR_LHU(at - 0x1308u) | kBit[step];
                at = 0x80070000u + v1;
                BR_SH(at - 0x1308u, v0);
            }
        }
        v0 = (r_a2 & 0xFFu) < 3u;
        v1 = r_a2 & 0xFFu;
    } while (v0 != 0);

    BattleResults_SetExitReg(1, at);
    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    return v0;
}

/* ---- leaves batch 3 ----------------------------------------------------- */

/* func_801E1370 (0x801E1370..0x801E1440, leaf, 53 instructions).
 * Called once, from 0x801E1540 (jal, nop delay slot) in 0x801E1444, with
 * a0 = id byte, a1 = count byte, a2 = key table, a3 = value table (one of
 * the five inventory table pairs in the native g_GameState copy), and a
 * FIFTH argument n, the table length, in the caller's outgoing argument word
 * at entry sp+0x10 (stored by 0x801E14E0/0x801E1520/0x801E153C); it is read
 * with lbu, so only its low byte counts.  No branch, J or data word targets
 * the entry.  The 8-byte frame (addiu sp,-8 / +8) holds nothing.
 *
 *   1370  sp -= 8 ; t5 = lbu 0x18(sp) (= entry sp+0x10) ; v0 = t5 & 0xFF
 *   1380  blez v0 -> 13FC (delay t1 = 0)
 *   1388  t3 = a0&FF ; t4 = a1&FF ; t6 = 99 ; t2 = v0 ; v1 = a3 ; t0 = a2
 *   13A0  v0 = lbu (t0) ; bne v0, t3 -> 13E4
 *   13B0    v0 = lbu (v1) + t4 ; slti v0 < 100 -> 13D0
 *   13C8      j 13F8 (delay sb 99, (v1))
 *   13D0      v0 = a1 + lbu (v1) ; j 13F8 (delay sb v0, (v1))
 *   13E4  v1++ ; t1++ ; slt t1 < t2 -> 13A0 (delay t0++)
 *   13F8  v0 = t5 & 0xFF
 *   13FC  bne t1, v0 -> 1438 ; beq v0, 0 -> 1438 (delay v1 = v0 + a3)
 *   140C  v0 = lbu (a2) ; bne v0, 0 -> 1428
 *   141C    sb a0, (a2) ; j 1438 (delay sb a1, (a3))
 *   1428  a3++ ; slt a3 < v1 -> 140C (delay a2++)
 *   1438  sp += 8 ; jr ra
 * A hit adds the FULL a1 (not a1&FF) to the stored byte when the masked sum
 * is below 100, else stores 99; a miss stores a0/a1 into the first zero key
 * (sb, low bytes).  The loop compares are signed (slt), as br_slt does.
 *
 * v0 on return: n on a hit; 0 otherwise (n == 0, the free key's 0 byte, or
 * the last slt).
 * Exit registers: t1 (0 if n == 0, else the hit index or n), t5 = n, v1 (a3
 * if n == 0, a3 + hit index on a hit, a3 + n on a miss) always; t0
 * (a2 + hit index, or a2 + n), t2 = n, t3 = a0&FF, t4 = a1&FF, t6 = 99 only
 * if n != 0 (else as on entry); a2/a3 advanced by the free-slot scan on a
 * miss (unchanged otherwise). */
uint32_t battle_results_func_801E1370(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t sp = BattleResults_EntrySp() - 8u;
    uint32_t v0, v1 = 0, t0 = 0, t1, t2 = 0, t3 = 0, t4 = 0, t5, t6 = 0;
    uint32_t r_a2 = a2, r_a3 = a3;
    int scanned = 0;

    t5 = BR_LBU(sp + 0x18u);
    v0 = t5 & 0xFFu;
    t1 = 0;
    if (v0 != 0) { /* blez: v0 is 0..255 */
        scanned = 1;
        t3 = a0 & 0xFFu;
        t4 = a1 & 0xFFu;
        t6 = 0x63u;
        t2 = v0;
        v1 = a3;
        t0 = a2;
        for (;;) {
            v0 = BR_LBU(t0);
            if (v0 == t3) {
                v0 = BR_LBU(v1);
                v0 = v0 + t4;
                v0 = (uint32_t)br_slt(v0, 100u);
                if (v0 == 0) {
                    BR_SB(v1, t6);
                } else {
                    v0 = BR_LBU(v1);
                    v0 = a1 + v0;
                    BR_SB(v1, v0);
                }
                break;
            }
            v1++;
            t1++;
            v0 = (uint32_t)br_slt(t1, t2);
            t0++;
            if (v0 == 0)
                break;
        }
        v0 = t5 & 0xFFu;
    }
    if (t1 == v0) {
        v1 = v0 + r_a3;
        if (v0 != 0) {
            do {
                v0 = BR_LBU(r_a2);
                if (v0 == 0) {
                    BR_SB(r_a2, a0);
                    BR_SB(r_a3, a1);
                    break;
                }
                r_a3++;
                v0 = (uint32_t)br_slt(r_a3, v1);
                r_a2++;
            } while (v0 != 0);
        }
    }

    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(6, r_a2);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitReg(9, t1);
    BattleResults_SetExitReg(13, t5);
    if (scanned) {
        BattleResults_SetExitReg(8, t0);
        BattleResults_SetExitReg(10, t2);
        BattleResults_SetExitReg(11, t3);
        BattleResults_SetExitReg(12, t4);
        BattleResults_SetExitReg(14, t6);
    }
    return v0;
}

/* func_801E2EB0 (0x801E2EB0..0x801E3088, leaf, 119 instructions, no frame).
 * Called once, from 0x801E2CB4 (jal, nop delay slot) in 0x801E2ACC, either
 * with a2 = 0, a1 = party slot 0..2, a0 = a divu quotient, or with a2 = 1,
 * a1 = 0xFF, a0 = s3 >> 1.  No branch, J or data word targets it.
 *
 * Splits a0 into the EXP/SP pair at the mutable module words 0x801E44F0 /
 * 0x801E44F4 (guest RAM):
 *   (s16)a2 == 1:  both = (3*a0) >> 2 (wrapping 32-bit), nothing else.
 *   otherwise, k = (s16)a1, R = word 0x801E44C8 + 0x170*k (wrapping):
 *     w0 = max(R[0x158], 1), w1 = max(R[0x159], 1) (slti < 2 -> 1)
 *     44F0 = lo(a0*w0) / (w0+w1), 44F4 = lo(a0*w1) / (w0+w1)   (mult, divu)
 *     (word 0x801E44C4)->u16[0x22B6] & 0x8000: both = a0
 *     a zero word -> 1 (each re-read)
 *     (word 0x801E44EC)->u16[0x32] & 0x2000: both *= 1.5 (x + x>>1)
 *     same u16 re-read & 0x1000: 44F4 *= 1.5
 *     word 0x801E44C8 + 8*k + 0xFE8/0xFEC = 44F0/44F4 (re-read)
 * 0x801E44C4 normally holds g_GameState (0x8006D634), so the 0x22B6 flag is
 * read from the native copy -- through the accessor.  The divisor is >= 2,
 * but br_divu reproduces the R3000A zero-divisor result anyway.
 *
 * v0 on return: 3*a0 on the a2 == 1 path; else word 0x801E44C8 + 8*k.
 * Exit registers: at = 0x801E0000, a2 = (s16) of the deciding register,
 * a3, t0 = 1 always (a3 = (3*a0)>>2 on the early path, a0 otherwise);
 * on the main path also v1 = 44F0, a0 = 44F4 (the final re-reads),
 * a1 = word 0x801E44EC, a2 = k, t1 = word 0x801E44C8 and HI:LO = the second
 * divu; the early path leaves v1, a0, a1, t1 and HI/LO as on entry. */
uint32_t battle_results_func_801E2EB0(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t hi, lo, v0, v1, r_a0, r_a1, r_a2, r_a3, t0, t1, c;

    (void)a3;
    r_a2 = br_sra(a2 << 16, 16);
    t0 = 1u;
    r_a3 = a0;
    if (r_a2 == t0) {
        v0 = r_a3 << 1;
        v0 = v0 + r_a3;
        r_a3 = v0 >> 2;
        BR_SW(0x801E44F0u, r_a3);
        BR_SW(0x801E44F4u, r_a3);
        BattleResults_SetExitReg(1, 0x801E0000u);
        BattleResults_SetExitReg(6, r_a2);
        BattleResults_SetExitReg(7, r_a3);
        BattleResults_SetExitReg(8, t0);
        return v0;
    }
    v0 = a1 << 16;
    r_a2 = br_sra(v0, 16);
    v0 = r_a2 << 1;
    v0 = v0 + r_a2;
    v0 = v0 << 3;
    v0 = v0 - r_a2;
    t1 = BR_LW(0x801E44C8u);
    v0 = v0 << 4;
    v0 = t1 + v0;
    r_a0 = BR_LBU(v0 + 0x158u);
    v1 = BR_LBU(v0 + 0x159u);
    c = (uint32_t)br_slt(r_a0, 2u);
    v0 = (uint32_t)br_slt(v1, 2u); /* beq delay slot */
    if (c != 0)
        r_a0 = 1u;
    c = v0;
    v0 = r_a0;                     /* beq delay slot */
    if (c != 0)
        v1 = 1u;
    br_mult(r_a3, v0, &hi, &lo);
    r_a0 = lo;
    v0 = v0 + v1;
    br_divu(r_a0, v0, &hi, &lo);
    r_a0 = lo;
    br_mult(r_a3, v1, &hi, &lo);
    v1 = lo;
    br_divu(v1, v0, &hi, &lo);
    v1 = lo;
    v0 = BR_LW(0x801E44C4u);
    v0 = BR_LHU(v0 + 0x22B6u) & 0x8000u;
    BR_SW(0x801E44F0u, r_a0);
    BR_SW(0x801E44F4u, v1);
    if (v0 != 0) {
        BR_SW(0x801E44F0u, r_a3);
        BR_SW(0x801E44F4u, r_a3);
    }
    v0 = BR_LW(0x801E44F0u);
    if (v0 == 0)
        BR_SW(0x801E44F0u, t0);
    v0 = BR_LW(0x801E44F4u);
    if (v0 == 0)
        BR_SW(0x801E44F4u, t0);
    r_a1 = BR_LW(0x801E44ECu);
    v0 = BR_LHU(r_a1 + 0x32u) & 0x2000u;
    if (v0 != 0) {
        v1 = BR_LW(0x801E44F0u);
        r_a0 = BR_LW(0x801E44F4u);
        v0 = (v1 >> 1) + v1;
        BR_SW(0x801E44F0u, v0);
        v0 = (r_a0 >> 1) + r_a0;
        BR_SW(0x801E44F4u, v0);
    }
    v0 = BR_LHU(r_a1 + 0x32u) & 0x1000u;
    c = v0;
    v0 = r_a2 << 3;                /* beq delay slot */
    if (c != 0) {
        v1 = BR_LW(0x801E44F4u);
        v0 = (v1 >> 1) + v1;
        BR_SW(0x801E44F4u, v0);
        v0 = r_a2 << 3;
    }
    v1 = BR_LW(0x801E44F0u);
    r_a0 = BR_LW(0x801E44F4u);
    v0 = t1 + v0;
    BR_SW(v0 + 0xFE8u, v1);
    BR_SW(v0 + 0xFECu, r_a0);

    BattleResults_SetExitReg(1, 0x801E0000u);
    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitReg(8, t0);
    BattleResults_SetExitReg(9, t1);
    BattleResults_SetExitHiLo(hi, lo);
    return v0;
}

/* func_801E3BE0 (0x801E3BE0..0x801E3D50, leaf, 93 instructions, no frame).
 * Called once, from 0x801E3A8C (jal, nop delay slot) in 0x801E3A18, with
 * a0 = the record's kind byte; no branch, J or data word targets it.
 *
 * Skill-learn scan for character k = a0 & 0xFF.  M = u16 0x8006ECF4 + 32k
 * (learned mask, read once), L = byte 0x8006D902 + 164k (level, re-read per
 * candidate), D = word 0x801E44E8 + 0x110k, E = word 0x801E44EC (re-read per
 * requirement).  For candidate i = 0 .. n-1 (n = 13 if u16 0x8006F8EA has
 * 0x4000, else 7):
 *   bit 0x8000>>i of M set -> next i
 *   L < D[0x100+i] -> stop, nothing learned
 *   all j in 0..6: !(E.u16[0x90+2j] < D.u16[14i+2j]) -> learn i, stop
 *   else next i
 * Learning ORs 0x8000>>i into the u16 at 0x8006ECF4 + 32k (re-read).
 * Both 0x8006 tables are in the native g_GameState copy for k <= 100.
 *
 *   3BE0  t5 = a0 ; t1 = 7 or 13 ; t4 = 0xFF ; a3 = 0 ; t6 = 0x8000
 *   3C10  v1 = k ; t3 = 164k ; at = 0x80070000+32k ; t2 = lhu -0x130C(at)
 *         t0 = lw 44E8 + 0x110k ; a0 = 0
 *   3C54  v0 = t2 & (t6 srav a0) ; bne -> 3CE8 (delay v0 = t0 + a0)
 *         v1 = lbu 0x100(v0) ; at = 0x80070000+t3 ; v0 = lbu -0x26FE(at)
 *         sltu v0 < v1 -> 3CFC (delay a1 = 0) ; a2 = 14*a0 + t0
 *   3C98  v0 = 2*(a1&FF) ; v1 = lw 44EC ; a0 = v0 + a2 ; v1 = lhu (a0) ;
 *         v0 = lhu 0x90(v0 + E) ; sltu v0 < v1 -> 3CD8 ;
 *         a1++ ; sltiu (a1&FF) < 7 -> 3C98 (delay v0 = a1&FF)
 *   3CD8  (a1&FF) == 7 -> 3D40: j 3CFC (delay t4 = a3)
 *   3CE8  a3++ ; sltu (a3&FF) < t1 -> 3C54 (delay a0 = a3&FF)
 *   3CFC  v1 = t4&FF ; == 0xFF -> 3D48 (delay v0 = v1) ;
 *         at = 0x80070000+32k ; a1 = lhu -0x130C(at) | (0x8000 srav v0) ;
 *         sh a1 ; j 3D4C
 *   3D48  v0 = 0 ;  3D4C jr ra
 *
 * v0 on return: the learned index i (so 0 both for "none" and for i == 0,
 * the retail quirk 0x801E3A18 lives with), or 0.
 * Exit registers: t0, t1, t2, t3, t4, t5 = a0, t6 = 0x8000, a0, a3, v1, at
 * always; a1 if any candidate reached its level test or something was
 * learned; a2 if any candidate reached its requirement loop. */
uint32_t battle_results_func_801E3BE0(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t v0, v1, at, r_a0, r_a1 = a1, r_a2 = a2, r_a3;
    uint32_t t0, t1, t2, t3, t4, t5, t6, c;

    (void)a3;
    t5 = a0;
    v0 = BR_LHU(0x8006F8EAu) & 0x4000u;
    t1 = 7u;
    if (v0 != 0)
        t1 = 13u;
    t4 = 0xFFu;
    r_a3 = 0;
    /* beq t1, zero: t1 is 7 or 13, never taken. */
    t6 = 0x8000u;
    v1 = t5 & 0xFFu;
    v0 = v1 << 2;
    v0 = v0 + v1;
    v0 = v0 << 3;
    v0 = v0 + v1;
    t3 = v0 << 2;
    v0 = v1 << 5;
    at = 0x80070000u + v0;
    t2 = BR_LHU(at - 0x130Cu);
    v0 = v1 << 4;
    v0 = v0 + v1;
    v1 = BR_LW(0x801E44E8u);
    v0 = v0 << 4;
    t0 = v1 + v0;
    r_a0 = r_a3 & 0xFFu;
    for (;;) {
        v0 = br_srav(t6, r_a0);
        v0 = t2 & v0;
        c = v0;
        v0 = t0 + r_a0;            /* bne delay slot */
        if (c == 0) {
            v1 = BR_LBU(v0 + 0x100u);
            at = 0x80070000u + t3;
            v0 = BR_LBU(at - 0x26FEu);
            v0 = v0 < v1;
            r_a1 = 0;              /* bne delay slot */
            if (v0 != 0)
                break;             /* -> 3CFC */
            v0 = r_a0 << 3;
            v0 = v0 - r_a0;
            v0 = v0 << 1;
            r_a2 = v0 + t0;
            v0 = r_a1 & 0xFFu;
            for (;;) {
                v0 = v0 << 1;
                v1 = BR_LW(0x801E44ECu);
                r_a0 = v0 + r_a2;
                v0 = v0 + v1;
                v1 = BR_LHU(r_a0);
                v0 = BR_LHU(v0 + 0x90u);
                v0 = v0 < v1;
                if (v0 != 0)
                    break;         /* -> 3CD8 */
                r_a1++;
                c = (r_a1 & 0xFFu) < 7u;
                v0 = r_a1 & 0xFFu; /* bne delay slot */
                if (c == 0)
                    break;
            }
            v1 = r_a1 & 0xFFu;
            v0 = 7u;
            if (v1 == v0) {
                t4 = r_a3;         /* 3D40: j 3CFC, delay slot */
                break;
            }
        }
        r_a3++;
        v0 = (r_a3 & 0xFFu) < t1;
        r_a0 = r_a3 & 0xFFu;       /* bne delay slot */
        if (v0 == 0)
            break;
    }
    v1 = t4 & 0xFFu;
    c = v1 == 0xFFu;
    v0 = v1;                       /* beq delay slot */
    if (c) {
        v0 = 0;
    } else {
        uint32_t bit;
        v1 = t5 & 0xFFu;
        v1 = v1 << 5;
        bit = 0x8000u;
        at = 0x80070000u + v1;
        r_a1 = BR_LHU(at - 0x130Cu);
        r_a0 = br_srav(bit, v0);
        r_a1 = r_a1 | r_a0;
        at = 0x80070000u + v1;
        BR_SH(at - 0x130Cu, r_a1);
    }

    BattleResults_SetExitReg(1, at);
    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitReg(8, t0);
    BattleResults_SetExitReg(9, t1);
    BattleResults_SetExitReg(10, t2);
    BattleResults_SetExitReg(11, t3);
    BattleResults_SetExitReg(12, t4);
    BattleResults_SetExitReg(13, t5);
    BattleResults_SetExitReg(14, t6);
    return v0;
}

/* func_801E3D54 (0x801E3D54..0x801E3E10, leaf, 48 instructions, no frame).
 * Called once, from 0x801E3AD4 (jal, nop delay slot) in 0x801E3A18, with
 * a0 = the record's kind byte; no branch, J or data word targets it.
 *
 * For k = a0 & 0xFF, D = word 0x801E44E8 + 0x110k, F = word 0x801E44C4 +
 * 32k + 0x16C2 (u16), L = byte 0x8006D902 + 164k (re-read per entry): walks
 * the list D[0xF0 + i], i = 0..11, until a 0xFF entry (-> return 0); the
 * first entry with !(L < e) whose bit 0x8000>>i is clear in F (re-read) gets
 * the bit set (sh) and returns i+1.  Entries that fail either test are
 * skipped; twelve entries without 0xFF return 0.
 *
 *   3D54  a2 = 0x8000 ; a1 = 0 ; a0 &= 0xFF ; t0 = 164k
 *         v1 = lw 44E8 + 0x110k ; a3 = lw 44C4 + 32k
 *   3D9C  a0 = lbu 0xF0(v1 + (a1&FF)) ; == 0xFF -> 3E0C (delay v0 = 0)
 *         at = 0x80070000 + t0 ; v0 = lbu -0x26FE(at) ; sltu v0 < a0 -> 3DF4
 *         a0 = lhu 0x16C2(a3) ; a2 & a0 -> 3DF4 (delay v0 = a2 | a0)
 *         sh v0, 0x16C2(a3) ; v0 = (a1 + 1) & 0xFF ; j 3E0C
 *   3DF4  a1++ ; sltiu (a1&FF) < 12 -> 3D9C (delay a2 >>= 1) ; v0 = 0
 *
 * v0 on return: i+1 (1..12) for a newly set bit, else 0.
 * Exit registers: a0 (the last byte or mask loaded), a1, a2, a3, t0, v1
 * always; at = 0x80070000 + 164k only if some entry reached its level test. */
uint32_t battle_results_func_801E3D54(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t v0, v1, r_a0, r_a1, r_a2, r_a3, t0;
    int level_read = 0;

    (void)a1;
    (void)a2;
    (void)a3;
    r_a2 = 0x8000u;
    r_a1 = 0;
    r_a0 = a0 & 0xFFu;
    v0 = r_a0 << 2;
    v0 = v0 + r_a0;
    v0 = v0 << 3;
    v0 = v0 + r_a0;
    t0 = v0 << 2;
    v0 = r_a0 << 4;
    v0 = v0 + r_a0;
    v1 = BR_LW(0x801E44E8u);
    v0 = v0 << 4;
    v1 = v1 + v0;
    v0 = BR_LW(0x801E44C4u);
    r_a0 = r_a0 << 5;
    r_a3 = v0 + r_a0;
    for (;;) {
        v0 = r_a1 & 0xFFu;
        v0 = v1 + v0;
        r_a0 = BR_LBU(v0 + 0xF0u);
        v0 = 0;                    /* beq delay slot */
        if (r_a0 == 0xFFu)
            break;
        level_read = 1;
        v0 = BR_LBU(0x80070000u + t0 - 0x26FEu);
        v0 = v0 < r_a0;
        if (v0 == 0) {
            uint32_t c;
            r_a0 = BR_LHU(r_a3 + 0x16C2u);
            c = r_a2 & r_a0;
            v0 = r_a2 | r_a0;      /* bne delay slot */
            if (c == 0) {
                BR_SH(r_a3 + 0x16C2u, v0);
                v0 = r_a1 + 1u;
                v0 = v0 & 0xFFu;
                break;
            }
        }
        r_a1++;
        v0 = (r_a1 & 0xFFu) < 12u;
        r_a2 = r_a2 >> 1;          /* bne delay slot */
        if (v0 == 0) {
            v0 = 0;
            break;
        }
    }

    if (level_read)
        BattleResults_SetExitReg(1, 0x80070000u + t0);
    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitReg(8, t0);
    return v0;
}

/* func_801E3E14 (0x801E3E14..0x801E3EA0, leaf, 36 instructions, no frame).
 * Called once, from 0x801E3B30 (jal, nop delay slot) in 0x801E3A18, with
 * a0 = the record's kind byte; no branch, J or data word targets it.
 *
 * For k = a0 & 0xFF, D = word 0x801E44E8 + 0x110k, G = word 0x801E44C4 +
 * 32k: for i = 0..8 until D[0xD0 + i] == 0xFF, e = D[0xD0+i]: if
 * u16 G[0x16C0] & (0x8000 srav (e-1)) then u16 G[0x16C4] |= 0x8000 srav
 * (i+3) (each a fresh lhu).  srav uses the low 5 bits: e == 0 shifts by 31
 * (bit 0), e > 16 shifts the bit out.
 *
 *   3E14  a1 = 0 ; t1 = 0xFF ; a3 = 0x8000 ; a0 &= 0xFF
 *         t0 = lw 44E8 + 0x110k ; a2 = lw 44C4 + 32k ; a0 = 0
 *   3E50  v0 = lbu 0xD0(t0 + a0) ; == t1 -> 3E9C (delay v0 -= 1)
 *         v1 = lhu 0x16C0(a2) & (a3 srav v0) ; == 0 -> 3E8C (delay a1++)
 *         v0 = a0 + 3 ; v1 = lhu 0x16C4(a2) | (a3 srav v0) ; sh v1
 *   3E8C  sltiu (a1&FF) < 9 -> 3E50 (delay a0 = a1&FF) ; jr ra
 *
 * v0 on return: 0xFE on a 0xFF terminator, else the final sltiu (0).
 * Exit registers, always: a0, a1, a2 = G, a3 = 0x8000, t0 = D, t1 = 0xFF,
 * v1 (the last value loaded/combined; word 44E8 if the first entry is 0xFF). */
uint32_t battle_results_func_801E3E14(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t v0, v1, r_a0, r_a1, r_a2, r_a3, t0, t1;

    (void)a1;
    (void)a2;
    (void)a3;
    r_a1 = 0;
    t1 = 0xFFu;
    r_a3 = 0x8000u;
    r_a0 = a0 & 0xFFu;
    v0 = r_a0 << 4;
    v0 = v0 + r_a0;
    v1 = BR_LW(0x801E44E8u);
    v0 = v0 << 4;
    t0 = v1 + v0;
    v0 = BR_LW(0x801E44C4u);
    r_a0 = r_a0 << 5;
    r_a2 = v0 + r_a0;
    r_a0 = r_a1 & 0xFFu;
    for (;;) {
        uint32_t e;
        v0 = t0 + r_a0;
        e = BR_LBU(v0 + 0xD0u);
        v0 = e - 1u;               /* beq delay slot */
        if (e == t1)
            break;
        v1 = BR_LHU(r_a2 + 0x16C0u);
        v0 = br_srav(r_a3, v0);
        v1 = v1 & v0;
        r_a1++;                    /* beq delay slot */
        if (v1 != 0) {
            v0 = r_a0 + 3u;
            v1 = BR_LHU(r_a2 + 0x16C4u);
            v0 = br_srav(r_a3, v0);
            v1 = v1 | v0;
            BR_SH(r_a2 + 0x16C4u, v1);
        }
        v0 = (r_a1 & 0xFFu) < 9u;
        r_a0 = r_a1 & 0xFFu;       /* bne delay slot */
        if (v0 == 0)
            break;
    }

    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitReg(8, t0);
    BattleResults_SetExitReg(9, t1);
    return v0;
}

/* func_801E3EA4 (0x801E3EA4..0x801E3F24, leaf, 33 instructions, no frame).
 * Called once, from 0x801E3B84 (jal, nop delay slot) in 0x801E3A18, only
 * for kind 8; it takes no argument (a0 is not read).  No branch, J or data
 * word targets it.
 *
 * D = word 0x801E44E8 (row 0: no kind offset), G = word 0x801E44C4 (no 32k
 * offset), L = byte 0x8006DE22 (native g_GameState, re-read per entry): for
 * i = 0..8 until D[0x950 + i] == 0xFF, if !(L < D[0x950+i]) and the bit
 * 0x1000 srav i is clear in u16 G[0x17C4] (re-read), set it (sh).
 *
 *   3EA4  a2 = 0 ; t3 = 0xFF ; t2 = 0x8006DE22 ; t1 = 0x1000
 *         t0 = lw 44E8 ; a3 = lw 44C4 ; a1 = 0
 *   3ECC  v1 = lbu 0x950(t0 + a1) ; == t3 -> 3F20 (delay nop)
 *         v0 = lbu (t2) ; sltu v0 < v1 -> 3F10 (delay a2++)
 *         a0 = lhu 0x17C4(a3) ; v1 = t1 srav a1 ; a0 & v1 -> 3F14
 *           (delay v0 = a2&FF) ; sh (v1 | a0), 0x17C4(a3)
 *   3F10  v0 = a2&FF
 *   3F14  sltiu v0 < 9 -> 3ECC (delay a1 = a2&FF) ; jr ra
 *
 * v0 on return: t0 + i (the terminator's row address) on a 0xFF entry, else
 * the final sltiu (0).
 * Exit registers: a1, a2, a3 = G, t0 = D, t1 = 0x1000, t2 = 0x8006DE22,
 * t3 = 0xFF, v1 always; a0 = the last mask loaded, only if some entry passed
 * its level test (else as on entry). */
uint32_t battle_results_func_801E3EA4(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t v0, v1, r_a0 = a0, r_a1, r_a2, r_a3, t0, t1, t2, t3, c;

    (void)a1;
    (void)a2;
    (void)a3;
    r_a2 = 0;
    t3 = 0xFFu;
    t2 = 0x8006DE22u;
    t1 = 0x1000u;
    t0 = BR_LW(0x801E44E8u);
    r_a3 = BR_LW(0x801E44C4u);
    r_a1 = r_a2 & 0xFFu;
    for (;;) {
        v0 = t0 + r_a1;
        v1 = BR_LBU(v0 + 0x950u);
        if (v1 == t3)
            break;
        v0 = BR_LBU(t2);
        v0 = v0 < v1;
        r_a2++;                    /* bne delay slot */
        if (v0 == 0) {
            r_a0 = BR_LHU(r_a3 + 0x17C4u);
            v1 = br_srav(t1, r_a1);
            v0 = r_a0 & v1;
            c = v0;
            v0 = r_a2 & 0xFFu;     /* bne delay slot */
            if (c == 0) {
                v0 = v1 | r_a0;
                BR_SH(r_a3 + 0x17C4u, v0);
                v0 = r_a2 & 0xFFu;
            }
        } else {
            v0 = r_a2 & 0xFFu;
        }
        v0 = v0 < 9u;
        r_a1 = r_a2 & 0xFFu;       /* bne delay slot */
        if (v0 == 0)
            break;
    }

    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitReg(8, t0);
    BattleResults_SetExitReg(9, t1);
    BattleResults_SetExitReg(10, t2);
    BattleResults_SetExitReg(11, t3);
    return v0;
}

/* func_801E3F28 (0x801E3F28..0x801E3FAC, leaf, 34 instructions, no frame).
 * Called once, from 0x801E3B60 (jal, nop delay slot) in 0x801E3A18, with
 * a0 = the record's kind byte; no branch, J or data word targets it.
 *
 * For k = a0 & 0xFF, D = word 0x801E44E8 + 0x110k, G = word 0x801E44C4 +
 * 32k: for i = 0..12 until D[0xE0 + i] == 0, e = D[0xE0+i]: if
 * u16 G[0x16C2] & (0x8000 srav (e-1)) then u16 G[0x16C6] |= 0x8000 srav i
 * (each a fresh lhu; srav takes the low 5 bits).
 *
 *   3F28  a1 = 0 ; a3 = 0x8000 ; a0 &= 0xFF ; t0 = lw 44E8 + 0x110k ;
 *         a2 = lw 44C4 + 32k ; a0 = 0
 *   3F60  v0 = lbu 0xE0(t0 + a0) ; == 0 -> 3FA8 (delay v0 -= 1)
 *         v1 = lhu 0x16C2(a2) & (a3 srav v0) ; == 0 -> 3F98 (delay a1++)
 *         v0 = lhu 0x16C6(a2) | (a3 srav a0) ; sh v0
 *   3F98  sltiu (a1&FF) < 13 -> 3F60 (delay a0 = a1&FF) ; jr ra
 *
 * v0 on return: 0xFFFFFFFF on a 0 terminator, else the final sltiu (0).
 * Exit registers, always: a0, a1, a2 = G, a3 = 0x8000, t0 = D, v1 (the last
 * value loaded/combined; word 44E8 if the first entry is 0). */
uint32_t battle_results_func_801E3F28(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t v0, v1, r_a0, r_a1, r_a2, r_a3, t0;

    (void)a1;
    (void)a2;
    (void)a3;
    r_a1 = 0;
    r_a3 = 0x8000u;
    r_a0 = a0 & 0xFFu;
    v0 = r_a0 << 4;
    v0 = v0 + r_a0;
    v1 = BR_LW(0x801E44E8u);
    v0 = v0 << 4;
    t0 = v1 + v0;
    v0 = BR_LW(0x801E44C4u);
    r_a0 = r_a0 << 5;
    r_a2 = v0 + r_a0;
    r_a0 = r_a1 & 0xFFu;
    for (;;) {
        uint32_t e;
        v0 = t0 + r_a0;
        e = BR_LBU(v0 + 0xE0u);
        v0 = e - 1u;               /* beq delay slot */
        if (e == 0)
            break;
        v1 = BR_LHU(r_a2 + 0x16C2u);
        v0 = br_srav(r_a3, v0);
        v1 = v1 & v0;
        r_a1++;                    /* beq delay slot */
        if (v1 != 0) {
            v0 = BR_LHU(r_a2 + 0x16C6u);
            v1 = br_srav(r_a3, r_a0);
            v0 = v0 | v1;
            BR_SH(r_a2 + 0x16C6u, v0);
        }
        v0 = (r_a1 & 0xFFu) < 13u;
        r_a0 = r_a1 & 0xFFu;       /* bne delay slot */
        if (v0 == 0)
            break;
    }

    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    BattleResults_SetExitReg(7, r_a3);
    BattleResults_SetExitReg(8, t0);
    return v0;
}

/* func_801E3FB0 (0x801E3FB0..0x801E4038, leaf, 35 instructions, no frame).
 * Called once, from 0x801E3BA8 (jal, nop delay slot) in 0x801E3A18, only
 * for kind 7; it takes no argument.  No branch, J or data word targets it.
 *
 * E = word 0x801E44EC (read twice), G = word 0x801E44C4 (read once):
 *   G.u32[0xE58] = 200 * E.u16[0x4E]      (sll/addu chain, 32-bit)
 *   G.u8[0xE30]  = (E[0x58] * 0xCCCCCCCD hi >> 2) + 1   (multu; = x/5 + 1)
 *   G.u16[0xE64] = 10 * E.u16[0x4E] ; G.u16[0xE66] = 10 * E.u16[0x4E]
 *     (E re-read from 44EC before 0xE64, the u16 re-read before each store;
 *     the 0xE66 store is in the jr delay slot)
 *
 * v0 on return: 10 * the last u16 read.
 * Exit registers, always: a0 = G, a1 = E (the re-read), a2 = 0xCCCCCCCD,
 * v1 = the last u16 read, HI:LO = the multu's product. */
uint32_t battle_results_func_801E3FB0(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    uint32_t hi, lo, v0, v1, r_a0, r_a1, r_a2;

    (void)a0;
    (void)a1;
    (void)a2;
    (void)a3;
    r_a1 = BR_LW(0x801E44ECu);
    r_a2 = 0xCCCC0000u;
    v1 = BR_LHU(r_a1 + 0x4Eu);
    r_a0 = BR_LW(0x801E44C4u);
    v0 = v1 << 1;
    v0 = v0 + v1;
    v0 = v0 << 3;
    v0 = v0 + v1;
    v0 = v0 << 3;
    BR_SW(r_a0 + 0xE58u, v0);
    v0 = BR_LBU(r_a1 + 0x58u);
    r_a2 = r_a2 | 0xCCCDu;
    br_multu(v0, r_a2, &hi, &lo);
    v0 = hi;
    v0 = v0 >> 2;
    v0 = v0 + 1u;
    BR_SB(r_a0 + 0xE30u, v0);
    r_a1 = BR_LW(0x801E44ECu);
    v1 = BR_LHU(r_a1 + 0x4Eu);
    v0 = v1 << 2;
    v0 = v0 + v1;
    v0 = v0 << 1;
    BR_SH(r_a0 + 0xE64u, v0);
    v1 = BR_LHU(r_a1 + 0x4Eu);
    v0 = v1 << 2;
    v0 = v0 + v1;
    v0 = v0 << 1;
    BR_SH(r_a0 + 0xE66u, v0);

    BattleResults_SetExitReg(3, v1);
    BattleResults_SetExitReg(4, r_a0);
    BattleResults_SetExitReg(5, r_a1);
    BattleResults_SetExitReg(6, r_a2);
    BattleResults_SetExitHiLo(hi, lo);
    return v0;
}

/* func_801E3A18 (0x801E3A18..0x801E3BDC, 0x1C8 bytes, the one adopted
 * non-leaf).  Skill/flag promotion over the three result slots; each gate
 * calls one of the six adopted helpers above directly.  Contract and proof:
 * docs/evidence/results-3a18-contract.md (retail bytes vs this exact
 * register-level transcription under scripted helpers, then composed with
 * the real helper bodies in the results differential).
 *
 *   frame  addiu sp,-32: s0 -> sp+16, s1 -> +20, s2 -> +24, ra -> +28 (the
 *          caller's values, from g_BattleResultsEntry.gpr), all restored, so
 *          s0-s2/sp/ra never appear in the exit set.
 *   loop   s1 = 0..2, slot = word 0x801E44C8 + 0x170*s0, stored to word
 *          0x801E44EC before the dead check (u16 +0x7C & 0x8000 skips).
 *   gates  the kind byte +0x56 is RE-READ through 0x801E44EC before every
 *          gate: 3BE0 if kind not 7/8 (nonzero low byte of v0 -> base+0x101C+s0),
 *          3D54 if kind != 10 (-> base+0x101F+s1), 3E14 if kind < 7, == 9 or
 *          >= 11, 3F28 if kind < 8 or >= 11, 3EA4 if kind == 8 and 3FB0 if
 *          kind == 7 (those two get whatever a0 is live).
 *   exit   at/v0/v1/a0 as this code last wrote them, otherwise whatever the
 *          last helper left (its exit set, HI/LO from 3FB0's multu): a helper
 *          call merges its exit registers into the local register file, the
 *          way the interpreted caller would see them after its jal. */
typedef struct BrRegs3A18 {
    uint32_t r[32];
    uint32_t written;
    int hilo;
    uint32_t hi, lo;
} BrRegs3A18;

static void br_3a18_call(BrRegs3A18 *s, BattleResultsHostBody body,
                         uint32_t return_address)
{
    unsigned reg;

    s->r[31] = return_address;
    g_BattleResultsExit.mask = 0;
    g_BattleResultsExit.hilo = 0;
    s->r[2] = body(s->r[4], s->r[5], s->r[6], s->r[7]);
    s->written |= 1u << 2;
    for (reg = 1; reg < 32; reg++) {
        if (reg != 2 && (g_BattleResultsExit.mask & (1u << reg))) {
            s->r[reg] = g_BattleResultsExit.gpr[reg];
            s->written |= 1u << reg;
        }
    }
    if (g_BattleResultsExit.hilo) {
        s->hilo = 1;
        s->hi = g_BattleResultsExit.hi;
        s->lo = g_BattleResultsExit.lo;
    }
}

#define BR3A18_SET(reg, value) (s.r[(reg)] = (value), s.written |= 1u << (reg))

uint32_t battle_results_func_801E3A18(uint32_t a0, uint32_t a1, uint32_t a2,
                                      uint32_t a3)
{
    BrRegs3A18 s;
    uint32_t sp;
    unsigned reg;

    memcpy(s.r, g_BattleResultsEntry.gpr, sizeof(s.r));
    s.r[4] = a0;
    s.r[5] = a1;
    s.r[6] = a2;
    s.r[7] = a3;
    s.written = 0;
    s.hilo = 0;
    s.hi = s.lo = 0;

    sp = s.r[29] - 32u;
    BR_SW(sp + 20u, s.r[17]);
    BR_SW(sp + 24u, s.r[18]);
    BR_SW(sp + 28u, s.r[31]);
    BR_SW(sp + 16u, s.r[16]);
    s.r[17] = 0;
    s.r[18] = 10;
    s.r[16] = s.r[17] & 0xFFu;
    for (;;) {
        uint32_t s0 = s.r[16];

        BR3A18_SET(3, BR_LW(0x801E44C8u));
        BR3A18_SET(2, ((s0 * 3u) * 8u - s0) << 4);
        BR3A18_SET(4, s.r[2] + s.r[3]);
        BR3A18_SET(2, BR_LHU(s.r[4] + 0x7Cu));
        BR3A18_SET(1, 0x801E0000u);
        BR_SW(0x801E44ECu, s.r[4]);
        s.r[2] &= 0x8000u;
        if (s.r[2] != 0)
            goto next_slot;

        BR3A18_SET(4, BR_LBU(s.r[4] + 0x56u));
        s.r[2] = (uint32_t)br_slt(s.r[4], 9u);
        if (s.r[2] == 0) {
            s.r[2] = (uint32_t)br_slt(s.r[4], 7u);
        } else {
            s.r[2] = (uint32_t)br_slt(s.r[4], 7u);
            if (s.r[2] == 0)
                goto after_3BE0;
        }
        br_3a18_call(&s, battle_results_func_801E3BE0, 0x801E3A94u);
        BR3A18_SET(4, s.r[2]);
        s.r[2] = s.r[4] & 0xFFu;
        if (s.r[2] != 0) {
            s.r[2] = BR_LW(0x801E44C8u) + s.r[16];
            BR_SB(s.r[2] + 0x101Cu, s.r[4]);
        }
after_3BE0:
        s.r[2] = BR_LW(0x801E44ECu);
        BR3A18_SET(4, BR_LBU(s.r[2] + 0x56u));
        if (s.r[4] == s.r[18]) {
            s.r[2] = (uint32_t)br_slt(s.r[4], 7u);
            goto check_3E14;
        }
        s.r[2] = (uint32_t)br_slt(s.r[4], 7u);
        br_3a18_call(&s, battle_results_func_801E3D54, 0x801E3ADCu);
        BR3A18_SET(4, s.r[2]);
        s.r[2] = s.r[4] & 0xFFu;
        BR3A18_SET(3, s.r[17] & 0xFFu);
        if (s.r[2] != 0) {
            s.r[2] = BR_LW(0x801E44C8u) + s.r[3];
            BR_SB(s.r[2] + 0x101Fu, s.r[4]);
        }
        s.r[2] = BR_LW(0x801E44ECu);
        BR3A18_SET(4, BR_LBU(s.r[2] + 0x56u));
        s.r[2] = (uint32_t)br_slt(s.r[4], 7u);
check_3E14:
        if (s.r[2] != 0) {
            s.r[2] = (uint32_t)br_slt(s.r[4], 9u);
        } else {
            s.r[2] = (uint32_t)br_slt(s.r[4], 9u);
            if (s.r[2] != 0 || s.r[4] == s.r[18])
                goto check_3F28;
        }
        br_3a18_call(&s, battle_results_func_801E3E14, 0x801E3B38u);
check_3F28:
        s.r[2] = BR_LW(0x801E44ECu);
        BR3A18_SET(4, BR_LBU(s.r[2] + 0x56u));
        s.r[2] = (uint32_t)br_slt(s.r[4], 11u);
        if (s.r[2] == 0) {
            s.r[2] = (uint32_t)br_slt(s.r[4], 8u);
        } else {
            s.r[2] = (uint32_t)br_slt(s.r[4], 8u);
            if (s.r[2] == 0)
                goto check_3EA4;
        }
        br_3a18_call(&s, battle_results_func_801E3F28, 0x801E3B68u);
check_3EA4:
        s.r[2] = BR_LW(0x801E44ECu);
        BR3A18_SET(3, BR_LBU(s.r[2] + 0x56u));
        s.r[2] = 8;
        if (s.r[3] == s.r[2])
            br_3a18_call(&s, battle_results_func_801E3EA4, 0x801E3B8Cu);
        s.r[2] = BR_LW(0x801E44ECu);
        BR3A18_SET(3, BR_LBU(s.r[2] + 0x56u));
        s.r[2] = 7;
        if (s.r[3] == s.r[2])
            br_3a18_call(&s, battle_results_func_801E3FB0, 0x801E3BB0u);
next_slot:
        s.r[17] += 1;
        s.r[2] = s.r[17] & 0xFFu;
        s.r[2] = s.r[2] < 3u;
        s.r[16] = s.r[17] & 0xFFu;
        if (s.r[2] == 0)
            break;
    }
    /* Epilogue: s0-s2/ra reloaded from the frame (the caller's values, as
     * stored above), sp back to the caller's.  None of them is an exit. */

    g_BattleResultsExit.mask = 0;
    g_BattleResultsExit.hilo = 0;
    for (reg = 1; reg < 32; reg++) {
        if (reg == 2 || (reg >= 16 && reg <= 18) || reg == 29 || reg == 31)
            continue;
        if (s.written & (1u << reg))
            BattleResults_SetExitReg(reg, s.r[reg]);
    }
    if (s.hilo)
        BattleResults_SetExitHiLo(s.hi, s.lo);
    return s.r[2];
}
#undef BR3A18_SET
