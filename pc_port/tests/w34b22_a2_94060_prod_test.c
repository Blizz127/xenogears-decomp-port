/* Focused production-linked oracle for retail world helper 0x80094060. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_94060.h"

#define TABLE_ADDR   0x8009BAC8u
#define IMAGE_FILE   "disc/world_map.bin"
#define IMAGE_SIZE   180422u
#define CODE_OFFSET  0x24570u /* 0x80094060 - 0x8006FAF0 */
#define CODE_BYTES   40u
#define TABLE_OFFSET 0x2BFD8u /* 0x8009BAC8 - 0x8006FAF0 */
#define TABLE_BYTES  128u

typedef struct LoadEvent {
    u32 address;
    u32 width;
    u32 value;
} LoadEvent;

static LoadEvent s_loads[64];
static u32 s_load_count;
static int s_failures;

/* Retail instruction words at [0x80094060, 0x80094088), little-endian. */
static const u32 s_retail_code[10] = {
    0x00042400u, /* sll $a0,$a0,16      */
    0x00052C00u, /* sll $a1,$a1,16      */
    0x00042303u, /* sra $a0,$a0,12      */
    0x00052BC3u, /* sra $a1,$a1,15      */
    0x00852021u, /* addu $a0,$a0,$a1    */
    0x3C01800Au, /* lui $at,0x800A      */
    0x00240821u, /* addu $at,$at,$a0    */
    0x8422BAC8u, /* lh $v0,-17720($at)  */
    0x03E00008u, /* jr $ra              */
    0x00000000u  /* nop                 */
};

/* Retail walkability table, 8 rows x 16-byte stride (64 s16). */
static const s16 s_retail_table[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    1, 0, 0, 1, 1, 0, 0, 0,
    1, 0, 0, 1, 0, 0, 0, 0,
    1, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 1, 1, 0, 0, 0, 0,
    1, 1, 1, 1, 1, 0, 0, 0,
    1, 1, 1, 1, 1, 0, 0, 0
};

void wm_94060_test_load(u32 address, u32 width, u32 value)
{
    if (s_load_count < 64u) {
        s_loads[s_load_count].address = address;
        s_loads[s_load_count].width = width;
        s_loads[s_load_count].value = value;
    }
    s_load_count++;
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
        fprintf(stderr, "ASSERTION %s: got=0x%08x expected=0x%08x\n",
                name, got, expected);
        s_failures++;
    }
}

static void poke_s16(u32 address, s16 value)
{
    memcpy(PSX_ADDR(address), &value, sizeof(value));
}

static u32 peek_u32(u32 address)
{
    u32 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return value;
}

static s32 load_s16(u32 address)
{
    s16 value;

    memcpy(&value, PSX_ADDR(address), sizeof(value));
    return (s32)value;
}

/* Independent s16 restore: mask + subtract, not a cast chain. */
static s32 spec_s16(u32 bits)
{
    s32 half = (s32)(bits & 0xFFFFu);

    if (half >= 0x8000)
        half -= 0x10000;
    return half;
}

/* Independent byte offset for $a0: sign-extended half scaled by 16. */
static u32 spec_offset_a0(u32 reg)
{
    return (u32)(spec_s16(reg) * 16);
}

/* Independent byte offset for $a1: sign-extended half scaled by 2. */
static u32 spec_offset_a1(u32 reg)
{
    return (u32)(spec_s16(reg) * 2);
}

static u32 spec_half_addr(u32 a0, u32 a1)
{
    return TABLE_ADDR + spec_offset_a0(a0) + spec_offset_a1(a1);
}

/* Independent oracle return: sign-extend the halfword in planted RAM. */
static s32 spec_return(u32 a0, u32 a1)
{
    u32 addr = spec_half_addr(a0, a1);
    u32 lo = (u32)g_PsxRam[addr & 0x1FFFFFu];
    u32 hi = (u32)g_PsxRam[(addr + 1u) & 0x1FFFFFu];

    return spec_s16(lo | (hi << 8));
}

static void reset_trace(void)
{
    memset(s_loads, 0, sizeof(s_loads));
    s_load_count = 0u;
}

static void reset_ram(void)
{
    memset(g_PsxRam, 0xA5, PSX_RAM_SIZE);
    reset_trace();
}

static void plant_table(const s16 *rows64)
{
    memcpy(PSX_ADDR(TABLE_ADDR), rows64, TABLE_BYTES);
}

static void test_fixture_binding(void)
{
    FILE *fp = fopen(IMAGE_FILE, "rb");
    unsigned char code[CODE_BYTES];
    unsigned char table[TABLE_BYTES];

    if (fp == NULL) {
        fprintf(stderr, "ASSERTION fixture %s missing\n", IMAGE_FILE);
        s_failures++;
        return;
    }
    fseek(fp, 0, SEEK_END);
    if ((u32)ftell(fp) != IMAGE_SIZE) {
        fprintf(stderr, "ASSERTION fixture size: got=%ld expected=%u\n",
                ftell(fp), IMAGE_SIZE);
        s_failures++;
        fclose(fp);
        return;
    }
    fseek(fp, (long)CODE_OFFSET, SEEK_SET);
    if (fread(code, 1, CODE_BYTES, fp) != CODE_BYTES ||
        memcmp(code, s_retail_code, CODE_BYTES) != 0) {
        fprintf(stderr, "ASSERTION code slice [0x80094060,0x80094088) "
                        "does not match frozen retail words\n");
        s_failures++;
    }
    fseek(fp, (long)TABLE_OFFSET, SEEK_SET);
    if (fread(table, 1, TABLE_BYTES, fp) != TABLE_BYTES ||
        memcmp(table, s_retail_table, TABLE_BYTES) != 0) {
        fprintf(stderr, "ASSERTION table slice 0x8009BAC8+128 does not "
                        "match frozen retail bytes\n");
        s_failures++;
    }
    fclose(fp);
}

static void test_retail_sweep(void)
{
    s32 row;
    s32 col;
    u32 before = wm_80094060_get_exec_count();

    reset_ram();
    plant_table(s_retail_table);
    poke_s16(TABLE_ADDR - 2u, (s16)0x5AA5u);
    poke_s16(TABLE_ADDR + TABLE_BYTES, (s16)0x5AA5u);

    for (row = 0; row < 8; row++) {
        for (col = 0; col < 2; col++) {
            s32 a0 = row;
            s32 a1 = col;
            s32 got;
            s32 expect = spec_return((u32)a0, (u32)a1);
            char label[96];

            reset_trace();
            got = wm_80094060(a0, a1);

            snprintf(label, sizeof(label), "sweep a0=%d a1=%d return", row, col);
            check_s32(label, got, expect);
            snprintf(label, sizeof(label), "sweep a0=%d a1=%d flag in {0,1}",
                     row, col);
            check_s32(label, got == 0 || got == 1 ? 1 : 0, 1);
            snprintf(label, sizeof(label), "sweep a0=%d a1=%d retail value",
                     row, col);
            check_s32(label, got, (s32)s_retail_table[row * 8 + col]);
            snprintf(label, sizeof(label), "sweep a0=%d a1=%d load count",
                     row, col);
            check_u32(label, s_load_count, 1u);
            snprintf(label, sizeof(label), "sweep a0=%d a1=%d load addr",
                     row, col);
            check_u32(label, s_loads[0].address, spec_half_addr((u32)a0, (u32)a1));
            snprintf(label, sizeof(label), "sweep a0=%d a1=%d load width",
                     row, col);
            check_u32(label, s_loads[0].width, 2u);
        }
    }

    /* The sweep must not disturb neighbours: read canaries back raw. */
    check_s32("sweep pre-table canary intact",
              load_s16(TABLE_ADDR - 2u), (s32)(s16)0x5AA5u);
    check_s32("sweep post-table canary intact",
              load_s16(TABLE_ADDR + TABLE_BYTES), (s32)(s16)0x5AA5u);
    check_u32("sweep exec delta",
              wm_80094060_get_exec_count() - before, 16u);
}

static void test_index_packing(void)
{
    /* Synthetic table: entry (r,c) = 0x0A00 + r*0x20 + c*3. */
    s16 synth[64];
    s32 r;
    s32 c;

    for (r = 0; r < 8; r++)
        for (c = 0; c < 8; c++)
            synth[r * 8 + c] = (s16)(0x0A00 + r * 0x20 + c * 3);

    reset_ram();
    plant_table(synth);

    /* Upper register bits must be killed by the sll16 pair. */
    check_s32("hi-bits a0=0x12340005 equals a0=5",
              wm_80094060((s32)0x12340005u, 0), spec_return(0x12340005u, 0u));
    check_s32("hi-bits a1=0x7FFF0001 equals a1=1",
              wm_80094060(6, (s32)0x7FFF0001u), spec_return(6u, 0x7FFF0001u));

    /* Negative indices wrap below the table base. */
    poke_s16(TABLE_ADDR - 16u, (s16)0x6B10u);
    poke_s16(TABLE_ADDR - 2u, (s16)0x6B11u);
    poke_s16(TABLE_ADDR - 18u, (s16)0x6B12u);
    check_s32("neg a0=-1 reads base-16", wm_80094060(-1, 0), 0x6B10);
    check_s32("neg a1=-1 reads base-2", wm_80094060(0, -1), 0x6B11);
    check_s32("neg a0=-1 a1=-1 reads base-18", wm_80094060(-1, -1), 0x6B12);

    /* Extreme s16 magnitudes stay inside PSX RAM. */
    poke_s16(TABLE_ADDR + 524272u + 2u, (s16)0x6B13u);
    check_s32("max a0=32767 a1=1", wm_80094060(32767, 1), 0x6B13);
    poke_s16(TABLE_ADDR - 524288u, (s16)0x9C5Cu);
    check_s32("min a0=-32768 reads 0x8001BAC8", wm_80094060(-32768, 0),
              (s32)(s16)0x9C5Cu);

    /* Last in-stride slot: row 7, col 1 at byte offset 114. */
    reset_ram();
    plant_table(synth);
    check_s32("row7 col1 offset 114", wm_80094060(7, 1),
              (s32)(s16)(0x0A00 + 7 * 0x20 + 3));
    check_u32("row7 col1 load addr", s_loads[s_load_count - 1u].address,
              TABLE_ADDR + 114u);
}

static void test_signed_return(void)
{
    reset_ram();
    plant_table(s_retail_table);

    poke_s16(TABLE_ADDR, (s16)0x8001u);
    check_s32("neg-half is -32767", wm_80094060(0, 0), -32767);
    check_s32("neg-half is not 0x8001u",
              wm_80094060(0, 0) == (s32)0x8001u ? 1 : 0, 0);
    check_s32("neg-half is not boolean 1",
              wm_80094060(0, 0) == 1 ? 1 : 0, 0);

    poke_s16(TABLE_ADDR + 16u + 2u, (s16)0xFFFFu);
    check_s32("neg-one stays -1", wm_80094060(1, 1), -1);

    poke_s16(TABLE_ADDR + 2u * 16u + 2u, (s16)0x6C0Fu);
    check_s32("wide-half full", wm_80094060(2, 1), 0x6C0F);
}

static void test_no_stores(void)
{
    u32 i;

    reset_ram();
    plant_table(s_retail_table);
    poke_s16(TABLE_ADDR - 4u, (s16)0x1111u);
    poke_s16(TABLE_ADDR - 2u, (s16)0x2222u);
    poke_s16(TABLE_ADDR + TABLE_BYTES, (s16)0x3333u);
    poke_s16(TABLE_ADDR + TABLE_BYTES + 2u, (s16)0x4444u);

    for (i = 0u; i < 16u; i++)
        (void)wm_80094060((s32)(i & 7u), (s32)(i >> 3));

    check_u32("pre-table canary pair", peek_u32(TABLE_ADDR - 4u), 0x22221111u);
    check_u32("post-table canary pair",
              peek_u32(TABLE_ADDR + TABLE_BYTES), 0x44443333u);
    check_s32("table bytes unchanged",
              memcmp(PSX_ADDR(TABLE_ADDR), s_retail_table, TABLE_BYTES) == 0
                  ? 1
                  : 0,
              1);
}

int main(void)
{
    PsxMemory_Init();

    test_fixture_binding();
    test_retail_sweep();
    test_index_packing();
    test_signed_return();
    test_no_stores();

    if (s_failures != 0) {
        fprintf(stderr, "ASSERTION failures=%d\n", s_failures);
        return 1;
    }
    printf("W34B22-A2 0x80094060 focused oracle PASS\n");
    return 0;
}
