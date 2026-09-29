/*
 * Production-linked certificate for the retail encounter-section object
 * D_800658DC / D_80065ADC.
 *
 * Retail stores the per-map encounter section as ONE contiguous blob at
 * 0x800658DC: sixteen formation records of 0x20 bytes (0x200 total), then the
 * sixteen per-formation encounter WEIGHT bytes at +0x200 -- an address that is
 * exactly 0x80065ADC -- then a 2-byte tail (0x212 decoded for map 2).
 * FieldLoad decompresses that whole stream to the unshifted base in a single
 * call (src/field/main/misc3.c:831-833).
 *
 * The defect this pins: the port used to split that single retail object into
 * two unrelated native objects (an auto-generated stub for D_800658DC plus a
 * standalone `u8 D_80065ADC[16]` in pc_port/src/game_overrides.c), so the
 * whole decode landed inside the first object, the weight table stayed zero,
 * `sum` was 0, `selected` stayed -1 in func_80079288, and no random encounter
 * could ever fire.
 *
 * This test links the REAL production owners:
 *   pc_port/src/data_field.c   -- the aliased storage under test
 *   src/field/main/misc4.c     -- the real func_80079288 weighted roll
 * and drives them with explicit spies. It does NOT re-implement the roll.
 *
 * Case 1 proves the write lands correctly and a non-zero weight set now
 * selects a formation and reaches the battle handoff.
 * Case 2 proves retail semantics are preserved for an all-zero weight set:
 * sum 0 -> no selection -> early return, nothing committed. (Guards against
 * "helpfully" seeding non-zero weights.)
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ---- storage under test (production pc_port/src/data_field.c) ------------ */
extern unsigned char D_800658DC[];
extern unsigned char D_80065ADC[];

/* ---- field state the roll reads (production data_field.c BSS aliases) ---- */
extern int32_t D_800B2294;   /* step counter */
extern int32_t D_800B2298;   /* enable gate */
extern int32_t D_800B229C;   /* live cooldown-timer count */
extern int16_t D_800B22A0[]; /* cooldown timers */
extern int16_t D_800B2270[]; /* per-formation payload copied to D_800B2290 */
extern int16_t D_800B2290;   /* committed payload */
extern int16_t g_FieldControl;      /* roll reads *(s16 *)&g_FieldControl */
extern int32_t g_FieldSystemMode;

/* ---- main-exe BSS normally supplied by stubs.c / game_overrides.c -------- */
/* Weak so a real production definition (if one is ever linked in) wins. */
__attribute__((weak)) int32_t D_8004F308;
__attribute__((weak)) int32_t D_8004F370;
__attribute__((weak)) int32_t D_800ADBDC;
__attribute__((weak)) int32_t D_800ADBE4;
__attribute__((weak)) int32_t D_800ADBEC;
__attribute__((weak)) int32_t D_800ADB2C;
__attribute__((weak)) int32_t D_800ADBD0;
__attribute__((weak)) unsigned char D_800ADB04;
__attribute__((weak)) unsigned char D_80059508;
__attribute__((weak)) unsigned char D_800594F8;

/* ---- function under test ------------------------------------------------ */
extern void func_80079288(void);

/* ---- explicit spies ----------------------------------------------------- */
static int s_refresh_calls;
static int s_battle_calls;
static int32_t s_battle_arg;
static int s_overlay_calls;
static unsigned int s_overlay_arg;
static int s_rand_calls;
static int s_rand_value;

void func_8008E718(void) { s_refresh_calls++; }

void func_80281204(int32_t formationIndex)
{
    s_battle_calls++;
    s_battle_arg = formationIndex;
}

void *LoadGameStateOverlay(unsigned int which)
{
    s_overlay_calls++;
    s_overlay_arg = which;
    return NULL;
}

/* Deterministic replacement for libc rand(); the roll is the only caller on
 * this path. Interposes over libc because a definition in the executable wins. */
int rand(void)
{
    s_rand_calls++;
    return s_rand_value;
}

/* ---- assertion plumbing ------------------------------------------------- */
static int s_checks;
static int s_failures;

static void expect_true(const char *assertion, int condition)
{
    s_checks++;
    if (!condition) {
        s_failures++;
        fprintf(stderr, "ASSERTION %s\n", assertion);
    }
}

static void expect_int(const char *assertion, long expected, long actual)
{
    s_checks++;
    if (expected != actual) {
        s_failures++;
        fprintf(stderr, "ASSERTION %s expected=%ld actual=%ld\n",
                assertion, expected, actual);
    }
}

/*
 * A realistic 0x212-byte decoded encounter section, exactly the shape FieldLoad
 * hands to FieldLZSSDecompress: 0x200 bytes of sixteen 0x20-byte formation
 * records, sixteen weight bytes, then the 2-byte tail.
 */
#define SECTION_BYTES 0x212
#define WEIGHTS_OFFSET 0x200

static unsigned char s_section[SECTION_BYTES];

static const unsigned char k_weights[16] = {
    0x14, 0x00, 0x0A, 0x00, 0x00, 0x1E, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
/* sum = 0x14 + 0x0A + 0x1E = 60; cumulative[5] = 30 */
#define WEIGHT_SUM 60
#define EXPECT_SELECTED 5

static void build_section(const unsigned char *weights)
{
    int record, byte;

    /* Records: byte b of record r = 0x40 + r * 0x11 + b, never zero, so a
     * mis-aimed weight window lands on recognisably wrong data. */
    for (record = 0; record < 16; record++) {
        for (byte = 0; byte < 0x20; byte++) {
            s_section[record * 0x20 + byte] =
                (unsigned char)(0x40 + record * 0x11 + byte);
        }
    }
    memcpy(s_section + WEIGHTS_OFFSET, weights, 16);
    s_section[0x210] = 0xAB;
    s_section[0x211] = 0xCD;
}

/* Reset every gate the roll reads so each case starts from a known state. */
static void arm_field_state(void)
{
    int i;

    D_800ADBDC = 1;
    D_800ADBE4 = 1;
    D_800ADBEC = 1;
    D_8004F308 = 0;
    D_800B2298 = 1;
    g_FieldControl = 0;
    D_800ADB2C = 0;
    D_800ADB04 = 1;
    D_800ADBD0 = 0;
    D_8004F370 = 0;
    g_FieldSystemMode = 0;

    D_800B2294 = 5;   /* decrements to 4: no refresh call this step */
    D_800B229C = 4;   /* four live cooldown timers */
    D_800B22A0[0] = 1;      /* ages to 0 -> this is the expiring slot */
    D_800B22A0[1] = (int16_t)0xFFFF;
    D_800B22A0[2] = 5;
    D_800B22A0[3] = 7;

    for (i = 0; i < 16; i++) {
        D_800B2270[i] = (int16_t)(0x1000 + i);
    }
    D_800B2290 = 0;

    D_80059508 = 0xEE;   /* sentinel: must stay 0xEE when no encounter fires */
    D_800594F8 = 0xEE;

    s_refresh_calls = 0;
    s_battle_calls = 0;
    s_battle_arg = -1;
    s_overlay_calls = 0;
    s_overlay_arg = 0;
    s_rand_calls = 0;
}

static int weight_sum_now(void)
{
    int i, sum = 0;
    for (i = 0; i < 16; i++) {
        sum += D_80065ADC[i];
    }
    return sum;
}

int main(void)
{
    int i;
    int cases = 0;

    /* ---------------------------------------------------------------- */
    /* Case 1: realistic section write + non-zero weights -> encounter.  */
    /* ---------------------------------------------------------------- */
    cases++;
    build_section(k_weights);
    arm_field_state();

    /* The aliasing property itself: D_80065ADC must BE D_800658DC + 0x200. */
    expect_true("alias/D_80065ADC-is-D_800658DC+0x200",
                D_80065ADC == D_800658DC + WEIGHTS_OFFSET);

    /* Exactly what FieldLoad's FieldLZSSDecompress does: write the decoded
     * stream to the unshifted destination base. */
    memcpy(D_800658DC, s_section, SECTION_BYTES);

    /* The sixteen weight bytes must be readable through D_80065ADC. */
    for (i = 0; i < 16; i++) {
        char name[64];
        snprintf(name, sizeof(name), "weights/D_80065ADC[%d]", i);
        expect_int(name, s_section[WEIGHTS_OFFSET + i], D_80065ADC[i]);
    }

    /* The record area and the 2-byte tail must survive the same write. */
    expect_true("records/0x000-0x1FF-intact",
                memcmp(D_800658DC, s_section, WEIGHTS_OFFSET) == 0);
    expect_true("tail/0x210-0x211-intact",
                memcmp(D_800658DC + 0x210, s_section + 0x210, 2) == 0);

    /* The sum the roll computes must now be non-zero. */
    expect_int("roll/weight-sum", WEIGHT_SUM, weight_sum_now());

    /* rand() = 24500 -> roll = (24500 * 61) >> 15 = 45; scanning i=15..0 the
     * first non-zero weight whose cumulative (30) is < 45 is index 5. */
    s_rand_value = 24500;
    func_80079288();

    expect_int("roll/rand-calls", 1, s_rand_calls);
    expect_int("roll/selected-formation", EXPECT_SELECTED, D_80059508);
    expect_int("roll/D_800594F8-cleared", 0, D_800594F8);
    expect_int("roll/payload-D_800B2290", 0x1000 + EXPECT_SELECTED, D_800B2290);
    expect_int("roll/step-counter-decremented", 4, D_800B2294);
    expect_int("roll/no-refresh-call", 0, s_refresh_calls);
    expect_int("roll/expired-timer-consumed", (int16_t)0xFFFF, D_800B22A0[0]);
    expect_int("roll/overlay-calls", 1, s_overlay_calls);
    expect_int("roll/overlay-arg", 2, (long)s_overlay_arg);
    expect_int("roll/battle-handoff-calls", 1, s_battle_calls);
    expect_int("roll/battle-handoff-arg", EXPECT_SELECTED, s_battle_arg);
    expect_int("roll/D_800ADBDC-cleared", 0, D_800ADBDC);
    expect_int("roll/D_800ADBD0-set", 1, D_800ADBD0);

    /* ---------------------------------------------------------------- */
    /* Case 2: all-zero weights -> retail early return, nothing committed. */
    /* ---------------------------------------------------------------- */
    {
        static const unsigned char zero_weights[16] = { 0 };

        cases++;
        build_section(zero_weights);
        arm_field_state();
        memcpy(D_800658DC, s_section, SECTION_BYTES);

        expect_int("zero/weight-sum", 0, weight_sum_now());

        s_rand_value = 24500;
        func_80079288();

        expect_int("zero/no-formation-selected", 0xEE, D_80059508);
        expect_int("zero/D_800594F8-untouched", 0xEE, D_800594F8);
        expect_int("zero/no-battle-handoff", 0, s_battle_calls);
        expect_int("zero/no-overlay-load", 0, s_overlay_calls);
        expect_int("zero/D_800ADBDC-untouched", 1, D_800ADBDC);
        expect_int("zero/D_800ADBD0-untouched", 0, D_800ADBD0);
    }

    printf("ENCOUNTER WEIGHT CASES %d ASSERTIONS %d\n", cases, s_checks);
    if (s_failures != 0) {
        printf("ENCOUNTER WEIGHT FAIL %d\n", s_failures);
        return 1;
    }
    printf("ENCOUNTER WEIGHT PASS\n");
    return 0;
}
