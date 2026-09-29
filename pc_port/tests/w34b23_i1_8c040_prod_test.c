/* Focused oracle for world helper 0x8008C040 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_93534.h"
#include "world_map_helper_8c040.h"


void wm_8c040_test_load(u32 addr, u32 width, u32 value){ (void)addr;(void)width;(void)value; }
void wm_8c040_test_store(u32 addr, u32 width, u32 value){ (void)addr;(void)width;(void)value; }
void wm_93534_test_load(u32 addr, u32 value){ (void)addr;(void)value; }
void wm_93534_test_store(u32 addr, u32 value){ (void)addr;(void)value; }

#define COUNT_ADDR 0x8009BD04u
#define BASE_ADDR  0x8009BE6Cu
#define WRAP_X_ADDR 0x8009D160u
#define WRAP_Z_ADDR 0x8009D2B4u

#define VEC_ADDR   0x800A2000u
#define OUT1_ADDR  0x800A3000u
#define OUT2_ADDR  0x800A3001u
#define TMP_ADDR   0x801C0300u

static int s_failures;

static void check_u8(const char *name, u8 got, u8 exp)
{
    if (got != exp) {
        fprintf(stderr, "ASSERTION %s: got %u exp %u\n", name, got, exp);
        s_failures++;
    }
}

static void __attribute__((unused)) check_s32(const char *name, s32 got, s32 exp)
{
    if (got != exp) {
        fprintf(stderr, "ASSERTION %s: got %d exp %d\n", name, got, exp);
        s_failures++;
    }
}

static void poke_u32(u32 addr, u32 v){ memcpy(PSX_ADDR(addr), &v, 4); }
static void poke_s16(u32 addr, s16 v){ memcpy(PSX_ADDR(addr), &v, 2); }
static void poke_u16(u32 addr, u16 v){ memcpy(PSX_ADDR(addr), &v, 2); }
static void poke_s32(u32 addr, s32 v){ u32 b; memcpy(&b,&v,4); poke_u32(addr,b); }
static u32 peek_u32(u32 addr){ u32 v; memcpy(&v, PSX_ADDR(addr),4); return v; }
static u8 peek_u8(u32 addr){ u8 v; memcpy(&v, PSX_ADDR(addr),1); return v; }

static s32 bits_to_s32(u32 b){ s32 v; memcpy(&v,&b,4); return v; }
static u32 s32_to_bits(s32 v){ u32 b; memcpy(&b,&v,4); return b; }
static u32 sra_bits(u32 b,u32 amt){ u32 v=b>>amt; if(b&0x80000000u) v|=0xFFFFFFFFu << (32-amt); return v; }

/* Retail GTE sqrt table exploited by SquareRoot0 (0x80048C4C): 192 s16
 * entries at 0x80056A00, sliced from the SLUS image like the libsn oracle.
 * Without it every nonzero distance sqrt reads the 0xA5 fill and no
 * boundary tier can match. */
#define SQRT_TABLE_GUEST 0x80056A00u
#define SQRT_TABLE_FILE_OFF (0x800u + (0x80056A00u - 0x80010000u))
#define SQRT_TABLE_SIZE 384u

static void reset_ram(void)
{
    FILE *f;
    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    f = fopen("disc/SLUS_006.64", "rb");
    if (f == NULL) {
        fprintf(stderr, "FATAL w34b23: cannot open disc/SLUS_006.64\n");
        exit(2);
    }
    if (fseek(f, SQRT_TABLE_FILE_OFF, SEEK_SET) != 0 ||
        fread(PSX_ADDR(SQRT_TABLE_GUEST), 1, SQRT_TABLE_SIZE, f) != SQRT_TABLE_SIZE) {
        fprintf(stderr, "FATAL w34b23: cannot load sqrt table\n");
        exit(2);
    }
    fclose(f);
}

/* Independent wrap like 93534 */
static s32 spec_wrap(s32 val, u32 period_global)
{
    u32 period = peek_u32(period_global) << 11;
    s32 p = bits_to_s32(period);
    if (val < -16384) {
        return val + p;
    } else if (val >= 16385) {
        return val - p;
    }
    return val;
}

/* Independent integer sqrt floor like SquareRoot0 */
static s32 spec_sqrt(s32 x)
{
    if (x <= 0) return 0;
    s32 r = (s32)sqrt((double)x);
    while ((s64)(r+1)*(r+1) <= x) r++;
    while ((s64)r*r > x) r--;
    return r;
}

/* Independent Y range check replicating retail */
static int spec_y_pass(s32 y_in, s32 y_low, s32 y_high, s32 s5)
{
    s32 v0, v1;
    int lt = (y_in < y_low);
    if (lt) {
        v0 = y_in - s5*4096;
        v1 = y_low;
    } else {
        s32 tmp = y_high*4096;
        v0 = y_low - tmp;
        v1 = y_in;
    }
    v1 = v1 - v0;
    s32 y_high_reload = y_high;
    s32 v1_sra = bits_to_s32(sra_bits(s32_to_bits(v1),12));
    s32 cmp = s5 + y_high_reload;
    return v1_sra < cmp;
}

/* Independent oracle for whole 8C040 */
static void spec_oracle(u32 vec, s32 arg1, s32 arg2, u32 out1 __attribute__((unused)), u32 out2 __attribute__((unused)),
                        s32 *out_tier, u8 *out_id)
{
    /* Initialize */
    *out_tier = 0;
    *out_id = 0;

    s16 count = (s16)(peek_u32(COUNT_ADDR) & 0xFFFF);
    if (count <=0) return;

    s32 y_in = bits_to_s32(peek_u32(vec+4));
    s32 x_in = bits_to_s32(peek_u32(vec+0));
    s32 z_in = bits_to_s32(peek_u32(vec+8));

    u32 base = BASE_ADDR;
    for (int idx=0; idx<count; idx++) {
        u32 entry = base + (u32)idx*0x18u;
        s32 y_low = bits_to_s32(peek_u32(entry+8));
        s32 y_high = bits_to_s32(peek_u32(entry+16));
        if (!spec_y_pass(y_in, y_low, y_high, arg2)) continue;

        s32 x_center = bits_to_s32(peek_u32(entry+4));
        s32 z_center = bits_to_s32(peek_u32(entry+12));
        s32 dx = x_center - x_in;
        s32 dz = z_center - z_in;
        s32 dx_sra = bits_to_s32(sra_bits(s32_to_bits(dx),12));
        s32 dz_sra = bits_to_s32(sra_bits(s32_to_bits(dz),12));

        /* wrap */
        s32 wx = spec_wrap(dx_sra, WRAP_X_ADDR);
        s32 wz = spec_wrap(dz_sra, WRAP_Z_ADDR);

        s64 sq = (s64)wx*wx + (s64)wz*wz;
        s32 sqrt_res = spec_sqrt((s32)sq);

        s16 thresh = (s16)(peek_u32(entry+20) & 0xFFFF);
        if (thresh & 0x8000) {} /* keep signed */
        s32 v1 = (s32)thresh + arg1;
        if (sqrt_res < v1) {
            *out_tier = 2;
            *out_id = (u8)(peek_u32(entry) & 0xFF);
            return;
        }
        if (sqrt_res < v1+16) {
            *out_tier = 1;
            *out_id = (u8)(peek_u32(entry) & 0xFF);
            return;
        }
    }
}

static void plant_entry(int idx, u32 id, s32 x, s32 y_low, s32 z, s32 y_high, s16 thresh)
{
    u32 entry = BASE_ADDR + (u32)idx*0x18u;
    poke_u16(entry+0, (u16)id);
    poke_s32(entry+4, x);
    poke_s32(entry+8, y_low);
    poke_s32(entry+12, z);
    poke_s32(entry+16, y_high);
    poke_s16(entry+20, thresh);
    poke_u16(entry+22, 0);
}

static void run_case(const char *name, s32 vec_x, s32 vec_y, s32 vec_z,
                     s32 arg1, s32 arg2,
                     int entry_count,
                     s32 exp_tier, u8 exp_id)
{
    /* Caller has already reset and planted entries, count, wraps */
    poke_s32(VEC_ADDR+0, vec_x);
    poke_s32(VEC_ADDR+4, vec_y);
    poke_s32(VEC_ADDR+8, vec_z);
    poke_s16(COUNT_ADDR, (s16)entry_count);
    poke_u32(WRAP_X_ADDR, 100u);
    poke_u32(WRAP_Z_ADDR, 100u);

    wm_8008C040(VEC_ADDR, arg1, arg2, OUT1_ADDR, OUT2_ADDR);

    u8 got1 = peek_u8(OUT1_ADDR);
    u8 got2 = peek_u8(OUT2_ADDR);

    char n1[128], n2[128];
    snprintf(n1,sizeof(n1),"%s out1",name);
    snprintf(n2,sizeof(n2),"%s out2",name);
    check_u8(n1, got1, (u8)exp_tier);
    check_u8(n2, got2, exp_id);

    /* Also run independent oracle */
    s32 oracle_tier; u8 oracle_id;
    spec_oracle(VEC_ADDR, arg1, arg2, OUT1_ADDR, OUT2_ADDR, &oracle_tier, &oracle_id);
    char n3[128], n4[128];
    snprintf(n3,sizeof(n3),"%s oracle out1",name);
    snprintf(n4,sizeof(n4),"%s oracle out2",name);
    check_u8(n3, (u8)oracle_tier, (u8)exp_tier);
    check_u8(n4, oracle_id, exp_id);
}

int main(void)
{
    PsxMemory_Init();

    /* no candidates - plant matching entry but count 0 so should not match; OFF_BY_ONE_COUNT mutant would match */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 0);
        poke_s32(VEC_ADDR+0, 0);
        poke_s32(VEC_ADDR+4, 0);
        poke_s32(VEC_ADDR+8, 0);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 7, 0, 0, 0, 10, 5);
        wm_8008C040(VEC_ADDR, 16, 32, OUT1_ADDR, OUT2_ADDR);
        check_u8("no_cand out1", peek_u8(OUT1_ADDR), 0);
        check_u8("no_cand out2", peek_u8(OUT2_ADDR), 0);
    }

    /* one candidate match first tier */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 7, 0, 0, 0, 10, 5); /* id 7, center 0,0,0, y_high 10, thresh 5 */
        run_case("one_match", 0, 0, 0, 16, 32, 1, 2, 7);
    }

    /* multiple candidates, first matches */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 2);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 3, 0, 0, 0, 10, 5);
        plant_entry(1, 9, 500000, 0, 500000, 10, 5);
        run_case("multi_first_match", 0,0,0, 16,32, 2, 2, 3);
    }

    /* later candidate matches */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 2);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 3, 500000, 0, 500000, 10, 5); /* far */
        plant_entry(1, 9, 0, 0, 0, 10, 5); /* near */
        run_case("later_match", 0,0,0, 16,32, 2, 2, 9);
    }

    /* no candidate matches */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 5, 500000, 0, 500000, 10, 5);
        run_case("no_match", 0,0,0, 16,32, 1, 0, 0);
    }

    /* exact distance boundary: thresh 10, arg1 0, distance 9 -> tier2, distance 10 -> not tier2 but tier1 if <26 */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        /* X center 9<<12 = 36864, vec 0, dx=36864>>12=9, wrapped 9, sqrt 9 */
        plant_entry(0, 1, 9<<12, 0, 0, 10, 10);
        run_case("exact_boundary_9", 0,0,0, 0,32, 1, 2, 1); /* sqrt 9 <10 => tier2 */
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 1, 10<<12, 0, 0, 10, 10);
        run_case("exact_boundary_10", 0,0,0, 0,32, 1, 1, 1); /* sqrt 10 ==10 not <10, but <26 => tier1 */
    }

    /* just below/above boundary */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 2, 15<<12, 0, 0, 10, 10);
        run_case("just_below_16", 0,0,0, 0,32, 1, 1, 2); /* 15<26 tier1 */
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 2, 26<<12, 0, 0, 10, 10);
        run_case("just_above_26", 0,0,0, 0,32, 1, 0, 0); /* 26 not <26 */
    }

    /* wrapped coordinates: X delta large, wraps to small */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        /* period = 100<<11=204800, X 200000 should wrap? Actually wrap logic: if val<-16384 add period, if >=16385 sub period */
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        /* dx = 20000, which >=16385, so wraps to 20000-204800=-184800? That's large negative, sqrt large -> no match? Need case where wrap makes it small */
        /* Use dx = 204800+5 = 204805, then wrapped = 5 */
        plant_entry(0, 3, (204800+5)<<12, 0, 0, 10, 10);
        run_case("wrapped_match", 0,0,0, 0,32, 1, 2, 3);
    }

    /* negative coordinates */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 4, ((-5)*4096), 0, 0, 10, 10);
        run_case("neg_coord", 0,0,0, 0,32, 1, 2, 4);
    }

    /* large positive */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 5, (1000)<<12, 0, 0, 10, 10);
        /* vec far */
        run_case("large_pos_no_match", (2000)<<12, 0, (2000)<<12, 0,32, 1, 0,0);
    }

    /* zero delta */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 6, 0, 0, 0, 10, 0);
        run_case("zero_delta", 0,0,0, 0,32, 1, 1, 6);
    }

    /* max count: use count 10 */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 10);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        for (int i=0;i<9;i++) plant_entry(i, (u32)(i+1), 500000, 0, 500000, 10, 5);
        plant_entry(9, 99, 0,0,0,10,5);
        run_case("max_count_last_match", 0,0,0, 16,32, 10, 2, 99);
    }


    /* unsigned delta check: y_high negative */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 1);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 8, 0, 0, 0, -10, 5);
        run_case("neg_y_high", 0, 0, 0, 16, 32, 1, 2, 8);
    }


    /* first vs last match: two near entries, first should win */
    {
        reset_ram();
        poke_s16(COUNT_ADDR, 2);
        poke_u32(WRAP_X_ADDR, 100);
        poke_u32(WRAP_Z_ADDR, 100);
        plant_entry(0, 11, 0, 0, 0, 10, 5);
        plant_entry(1, 22, 0, 0, 0, 10, 5);
        run_case("first_vs_last", 0,0,0, 16,32, 2, 2, 11);
    }

    if (s_failures) {
        fprintf(stderr, "FAILURES %d\n", s_failures);
        return 1;
    }
    printf("W34B23-I1 0x8008C040 focused oracle PASS\n");
    return 0;
}
