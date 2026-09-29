/* Focused production-linked oracle for retail world helper 0x80093F18. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93e8c.h"
#include "world_map_helper_93f18.h"

#define STRIDE_ADDR   0x8009D160u
#define TABLE_ADDR    0x8009C184u
#define VEC_ADDR      0x800A2000u
#define CELL_ADDR     0x800A4000u
#define RECORD_OFF    0x510u

#define TBL_A_ADDR    0x8009B464u
#define TBL_B_ADDR    0x8009B364u
#define TBL_C_ADDR    0x8009B46Cu
#define TBL_D_ADDR    0x8009B36Cu

typedef struct LoadEvent {
    u32 address;
    u32 width;
    u32 value;
} LoadEvent;

/* Trace for 93E8C */
static LoadEvent s_loads_e8c[16];
static u32 s_load_count_e8c;
/* Trace for 93F18 */
static LoadEvent s_loads_f18[16];
static u32 s_load_count_f18;

static int s_failures;

void wm_93e8c_test_load(u32 address, u32 width, u32 value)
{
    if (s_load_count_e8c < 16u) {
        s_loads_e8c[s_load_count_e8c].address = address;
        s_loads_e8c[s_load_count_e8c].width = width;
        s_loads_e8c[s_load_count_e8c].value = value;
    }
    s_load_count_e8c++;
}

void wm_93f18_test_load(u32 address, u32 width, u32 value)
{
    if (s_load_count_f18 < 16u) {
        s_loads_f18[s_load_count_f18].address = address;
        s_loads_f18[s_load_count_f18].width = width;
        s_loads_f18[s_load_count_f18].value = value;
    }
    s_load_count_f18++;
}

static void check_s32(const char *name, s32 got, s32 expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=%d (0x%08x) expected=%d (0x%08x)\n",
                name, got, (u32)got, expected, (u32)expected);
        s_failures++;
    }
}

static void check_u32(const char *name, u32 got, u32 expected)
{
    if (got != expected) {
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n", name, got, expected);
        s_failures++;
    }
}

static void poke_u32(u32 address, u32 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static void poke_s16(u32 address, s16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 peek_u32(u32 address)
{
    u32 v;
    memcpy(&v, PSX_ADDR(address), sizeof(v));
    return v;
}

static u32 bits_from_s32(s32 v)
{
    u32 b;
    memcpy(&b, &v, sizeof(b));
    return b;
}

static s32 s32_from_bits(u32 b)
{
    s32 v;
    memcpy(&v, &b, sizeof(v));
    return v;
}

/* Arithmetic right shift without relying on C signed >> */
static u32 sra_bits(u32 bits, u32 amt)
{
    u32 v = bits >> amt;
    if (bits & 0x80000000u)
        v |= UINT32_MAX << (32u - amt);
    return v;
}

/* Independent coarse spec: (raw + (raw<0?7:0)) >>3 arithmetic */
static s32 spec_coarse(s32 raw)
{
    if (raw < 0)
        raw += 7;
    /* arithmetic */
    u32 bits = bits_from_s32(raw);
    bits = sra_bits(bits, 3u);
    return s32_from_bits(bits);
}

/* Independent packed for 93E8C: low 4 bits of x>>19 and z>>19 */
static u32 spec_93e8c_packed(u32 x_bits, u32 z_bits)
{
    return ((z_bits >> 19) & 0xFu) * 16u + ((x_bits >> 19) & 0xFu);
}

/* Independent table index for 93E8C cell lookup */


/* Independent oracle for 93F18 return */
static s32 spec_93f18_return(u32 x_bits, u32 z_bits, u32 packed_attr,
                             u32 tbl_a, u32 tbl_b, u32 tbl_c, u32 tbl_d)
{
    s32 x_raw = s32_from_bits(x_bits);
    s32 z_raw = s32_from_bits(z_bits);
    s32 cx_s32 = spec_coarse(x_raw);
    s32 cz_s32 = spec_coarse(z_raw);
    u32 cx = bits_from_s32(cx_s32) & 0xFFFFu;
    u32 cz = bits_from_s32(cz_s32) & 0xFFFFu;

    u32 diff_x_bits = cx - tbl_a;
    u32 diff_z_bits = cz - tbl_c;

    s32 diff_x = s32_from_bits(diff_x_bits);
    s32 diff_z = s32_from_bits(diff_z_bits);
    s32 b = s32_from_bits(tbl_b);
    s32 d = s32_from_bits(tbl_d);

    s64 prod_x = (s64)diff_x * (s64)b;
    s64 prod_z = (s64)diff_z * (s64)d;
    u32 lo_x = (u32)(prod_x & 0xFFFFFFFFLL);
    u32 lo_z = (u32)(prod_z & 0xFFFFFFFFLL);

    u32 sra_x = sra_bits(lo_x, 12u);
    u32 sra_z = sra_bits(lo_z, 12u);

    u32 sum_bits = sra_x + sra_z;
    s32 sum = s32_from_bits(sum_bits);

    u32 packed = packed_attr;
    u32 res;
    if (sum > 0)
        res = (packed >> 4) & 0x7u;
    else
        res = (packed >> 7) & 0x7u;
    return (s32)res;
}

static void reset_ram(void)
{
    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    memset(s_loads_e8c, 0, sizeof(s_loads_e8c));
    memset(s_loads_f18, 0, sizeof(s_loads_f18));
    s_load_count_e8c = 0u;
    s_load_count_f18 = 0u;
}

/* Plant a case where 93E8C returns `packed_attr` (halfword) */
static void plant_93e8c_case(u32 x_bits, u32 y_bits, u32 z_bits,
                             u32 stride, u32 cell, s16 half)
{
    /* Simplified: force table index 0 to keep half_addr = cell+RECORD_OFF+packed0*2
     * where packed0 = ((z>>19)&0xF)*16 + ((x>>19)&0xF)
     * We arrange x,z so packed0==0 for most cases, by keeping x and z < 1<<19.
     */
    u32 packed0 = spec_93e8c_packed(x_bits, z_bits);
    u32 half_addr = cell + RECORD_OFF + packed0 * 2u;

    reset_ram();
    poke_u32(VEC_ADDR, x_bits);
    poke_u32(VEC_ADDR + 4u, y_bits);
    poke_u32(VEC_ADDR + 8u, z_bits);
    poke_u32(STRIDE_ADDR, stride);
    /* Table entry 0 points to cell */
    poke_u32(TABLE_ADDR + 0u, cell);
    poke_s16(half_addr, half);
    /* Canaries */
    poke_u32(VEC_ADDR - 4u, 0x11111111u);
    poke_u32(VEC_ADDR + 12u, 0x22222222u);
}

/* Plant the four 93F18 tables for a given low_nibble */
static void plant_93f18_tables_for_nibble(u32 nibble, u32 a, u32 b, u32 c, u32 d)
{
    u32 a1 = nibble << 4;
    poke_u32(TBL_A_ADDR + a1, a);
    poke_u32(TBL_B_ADDR + a1, b);
    poke_u32(TBL_C_ADDR + a1, c);
    poke_u32(TBL_D_ADDR + a1, d);
}

static void plant_all_93f18_tables_default(void)
{
    /* Default simple tables: A=0, C=0, B=0x1000 (4096), D=0x2000 (8192)
     * After >>12, B contributes 1*diff, D contributes 2*diff, so swapping
     * B/D or swapping diffs changes sum when cx != cz.
     */
    for (u32 n = 0; n < 16u; n++) {
        u32 a1 = n << 4;
        poke_u32(TBL_A_ADDR + a1, 0u);
        poke_u32(TBL_B_ADDR + a1, 0x1000u);
        poke_u32(TBL_C_ADDR + a1, 0u);
        poke_u32(TBL_D_ADDR + a1, 0x2000u);
    }
}

static s32 run_single_case(const char *label,
                           u32 x_bits, u32 z_bits, s16 half,
                           u32 exp_tbl_a, u32 exp_tbl_b,
                           u32 exp_tbl_c, u32 exp_tbl_d,
                           s32 expected_ret)
{
    plant_93e8c_case(x_bits, 0x00C0FFEEu, z_bits, 1u, CELL_ADDR, half);
    plant_all_93f18_tables_default();
    /* Override the specific nibble entry */
    u32 nibble = ((u32)half) & 0xFu;
    plant_93f18_tables_for_nibble(nibble, exp_tbl_a, exp_tbl_b, exp_tbl_c, exp_tbl_d);

    s32 got = wm_80093F18(VEC_ADDR);
    char name[128];
    snprintf(name, sizeof(name), "%s return", label);
    check_s32(name, got, expected_ret);

    /* Canaries not clobbered */
    snprintf(name, sizeof(name), "%s canary pre", label);
    check_u32(name, peek_u32(VEC_ADDR - 4u), 0x11111111u);
    snprintf(name, sizeof(name), "%s canary post", label);
    check_u32(name, peek_u32(VEC_ADDR + 12u), 0x22222222u);

    /* At least verify that 93F18 performed 4 table loads */
    snprintf(name, sizeof(name), "%s f18 load count", label);
    /* We expect 4 loads from TBLs + 2 from x/z + whatever 93E8C does.
     * 93E8C itself does 5 loads, 93F18 does 6 loads (x,z + 4 tables).
     * But with trace we count only f18 loads.
     */
    if (s_load_count_f18 < 6u) {
        fprintf(stderr, "ASSERTION %s: f18 load count %u <6\n", label, s_load_count_f18);
        s_failures++;
    }
    return got;
}

/* Build expected using spec */
static s32 make_expected(u32 x_bits, u32 z_bits, s16 half,
                         u32 a, u32 b, u32 c, u32 d)
{
    return spec_93f18_return(x_bits, z_bits, (u32)(u16)half, a, b, c, d);
}

static void test_branch_classes(void)
{
    /* Class: x negative, z positive, sum <=0 -> low path */
    {
        u32 x = bits_from_s32(-8);
        u32 z = 0u;
        /* half: low_nibble=1, ret_gt=5, ret_le=2 => low=1, bits 4-6=101=5, bits7-9=010=2 */
        s16 half = (s16)(1u | (5u << 4) | (2u << 7)); /* 1 +80 +256 = 337 */
        u32 a = 0u, b = 0x1000u, c = 0u, d = 0x2000u;
        s32 exp = make_expected(x, z, half, a, b, c, d);
        run_single_case("negX_posZ_le", x, z, half, a, b, c, d, exp);
    }
    /* x positive large, z positive -> sum>0 */
    {
        u32 x = 64u; /* 64>>3=8 */
        u32 z = 0u;
        s16 half = (s16)(2u | (3u << 4) | (6u << 7)); /* low2, gt3, le6 */
        u32 a = 0u, b = 0x1000u, c = 0u, d = 0x2000u;
        s32 exp = make_expected(x, z, half, a, b, c, d);
        run_single_case("posX_gt", x, z, half, a, b, c, d, exp);
    }
    /* x negative -1, z negative -1 */
    {
        u32 x = bits_from_s32(-1);
        u32 z = bits_from_s32(-1);
        s16 half = (s16)(0x0Fu | (0x7u << 4) | (0x0u << 7));
        u32 a = 0u, b = 0x1000u, c = 0u, d = 0x2000u;
        s32 exp = make_expected(x, z, half, a, b, c, d);
        run_single_case("neg1_neg1", x, z, half, a, b, c, d, exp);
    }
    /* zero zero -> sum 0 -> le path */
    {
        u32 x = 0u, z = 0u;
        s16 half = (s16)(0u | (1u << 4) | (2u << 7));
        u32 a = 0u, b = 0x1000u, c = 0u, d = 0x2000u;
        s32 exp = make_expected(x, z, half, a, b, c, d);
        run_single_case("zero_zero", x, z, half, a, b, c, d, exp);
        /* This also verifies zero path returns (half>>7)&7 =2 */
        check_s32("zero_zero expects 2", exp, 2);
    }
}

static void test_all_nibbles(void)
{
    for (u32 nib = 0; nib < 16u; nib++) {
        char label[64];
        snprintf(label, sizeof(label), "nibble_%u", nib);
        /* X= 8*(nibble+1) to get varying coarse, Z=0 */
        u32 x = (nib + 1u) * 8u;
        u32 z = 0u;
        /* half encodes: low=nib, gt = nib%8, le = (7-(nib%8)) */
        s16 half = (s16)(nib | ((nib % 8u) << 4) | (((7u - (nib % 8u)) & 0x7u) << 7));
        u32 a = 0u, b = 0x1000u, c = 0u, d = 0x2000u;
        s32 exp = make_expected(x, z, half, a, b, c, d);
        run_single_case(label, x, z, half, a, b, c, d, exp);
    }
}

static void test_image_sites_coverage(void)
{
    /* 20 distinct scenarios mimicking the 20 jal sites:
     * We don't jump to those addresses, but we exercise the function with
     * 20 different vecs/halfwords covering all branch combos and nibbles.
     */
    const u32 xs[20] = {0u, 8u, 16u, 24u, 32u, 40u, 48u, 56u, 64u, 72u,
                        80u, 88u, 96u, 104u, 112u, 120u, 128u, 136u, 144u, 152u};
    const u32 zs[20] = {0u, 0u, 8u, 8u, 0u, 16u, 16u, 24u, 24u, 32u,
                        0u, 8u, 16u, 24u, 32u, 40u, 48u, 56u, 64u, 72u};
    for (u32 i = 0; i < 20u; i++) {
        char label[64];
        snprintf(label, sizeof(label), "site_%02u", i);
        u32 x = xs[i];
        u32 z = zs[i];
        /* Alternate negative for some */
        if ((i & 1u) && i >= 10u) {
            x = bits_from_s32(-(s32)(x + 1u));
        }
        s16 half = (s16)((i & 0xFu) | (((i * 2u) & 0x7u) << 4) | (((i * 3u) & 0x7u) << 7));
        u32 a = 0u, b = 0x1000u, c = 0u, d = 0x2000u;
        /* For some sites, make table A non-zero to affect sum */
        if (i % 3u == 0u) {
            a = 0xFFFFu; /* 65535 */
        }
        s32 exp = make_expected(x, z, half, a, b, c, d);
        run_single_case(label, x, z, half, a, b, c, d, exp);
    }
}

static void test_tables_with_realistic_values(void)
{
    /* Use realistic values from retail dump: pick nibble 0 where
     * T_A=0xFFFF, T_B=0x0E50 etc. Verify our implementation still matches spec.
     */
    u32 x = 0u;
    u32 z = 0u;
    s16 half = (s16)(0u | (4u << 4) | (2u << 7));
    /* For nibble 0, retail tables: A=FFFF, B=0E50 (3664), C=0, D=0728 (1832) */
    u32 a = 0xFFFFu;
    u32 b = 0x0E50u;
    u32 c = 0u;
    u32 d = 0x0728u;
    s32 exp = make_expected(x, z, half, a, b, c, d);
    run_single_case("realistic_n0", x, z, half, a, b, c, d, exp);

    /* Nibble 2: A=0x8000, B=0x0E50, C=0, D=0xF8D8(-1832) */
    x = 8u;
    z = 8u;
    half = (s16)(2u | (1u << 4) | (5u << 7));
    a = 0x8000u;
    b = 0x0E50u;
    c = 0u;
    d = bits_from_s32(-1832);
    exp = make_expected(x, z, half, a, b, c, d);
    run_single_case("realistic_n2", x, z, half, a, b, c, d, exp);
}

static void test_boundary_plus_minus_one(void)
{
    /* +/-1 boundary for coarse: values around 7,8 etc.
     * Retail adds 7 for negative then >>3: -8 -> -1, -7 -> 0
     */
    struct { s32 raw; s32 expected_coarse; } cases[] = {
        {-9, -1}, {-8, -1}, {-7, 0}, {-1, 0}, {0,0}, {1,0}, {7,0}, {8,1}, {9,1}
    };
    for (u32 i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        char label[64];
        snprintf(label, sizeof(label), "coarse_bdry_%d", cases[i].raw);
        s32 raw = cases[i].raw;
        s32 coarse = spec_coarse(raw);
        char cname[96];
        snprintf(cname, sizeof(cname), "%s coarse", label);
        check_s32(cname, coarse, cases[i].expected_coarse);

        /* Also test via full function: x=raw, z=0, half with distinct gt/le */
        u32 x = bits_from_s32(raw);
        u32 z = 0u;
        s16 half = (s16)(1u | (3u <<4) | (4u <<7));
        u32 a=0u,b=0x1000u,c=0u,d=0x1000u;
        s32 exp = make_expected(x,z,half,a,b,c,d);
        run_single_case(label, x, z, half, a,b,c,d, exp);
    }
}

int main(void)
{
    PsxMemory_Init();

    test_branch_classes();
    test_all_nibbles();
    test_image_sites_coverage();
    test_tables_with_realistic_values();
    test_boundary_plus_minus_one();

    if (s_failures != 0) {
        fprintf(stderr, "ASSERTION failures=%d\n", s_failures);
        return 1;
    }
    printf("W34B22-I4A 0x80093F18 focused oracle PASS (sites=20)\n");
    return 0;
}
