/* Retail-differential test for the port's TransposeMatrix
 * (pc_port/src/psyq_compat_transpose_matrix.c).
 *
 * Runs the retail bytes at 0x8004A8EC (read from the user's SLUS_006.64) in
 * the port's MIPS interpreter and compares, for every case, all 128 bytes of a
 * window around both matrices plus the returned pointer. Cases: random
 * matrices in place (m1 == m0, the world map's call shape) and fully disjoint
 * (m1 at +40 and -32 bytes from m0); then fixed matrices whose transpose is
 * checked by value, including that t[] and the pad halfword are never written.
 *
 * Clean-room restriction (see the harness README): the original run also
 * compared m1 at every 4-byte offset from -16 to +16 bytes of m0. Matrices
 * that overlap without being identical are outside the public interface and
 * the game never passes them, so a body written from the interface alone is
 * checked on the two shapes the game uses. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <libgte.h>
#include "battle_mips_adapter.h"

#define ENTRY 0x8004A8ECu
#define CODE_SIZE 0x40u
#define EXE_LOAD_BIAS 0x8000F800u /* PS-X EXE: 0x800 header, text at 0x80010000 */
#define HALT 0xFFFFFFFCu
#define RAM_BASE 0x80000000u
#define WINDOW 0x80100000u
#define WINDOW_SIZE 128u
#define M0_OFFSET 32u

extern MATRIX* TransposeMatrix(MATRIX* m0, MATRIX* m1);

static uint8_t ram[0x200000];
static unsigned checks;

static uint8_t* guest(uint32_t a)
{
    return ram + (a - RAM_BASE);
}

static int read_bus(void* opaque, uint32_t a, unsigned w, uint32_t* v)
{
    unsigned i;
    (void)opaque;
    if (a < RAM_BASE || (uint64_t)a + w > (uint64_t)RAM_BASE + sizeof(ram)) return -1;
    *v = 0;
    for (i = 0; i < w; i++) *v |= (uint32_t)guest(a)[i] << (8 * i);
    return 0;
}

static int write_bus(void* opaque, uint32_t a, unsigned w, uint32_t v)
{
    unsigned i;
    (void)opaque;
    if (a < RAM_BASE || (uint64_t)a + w > (uint64_t)RAM_BASE + sizeof(ram)) return -1;
    for (i = 0; i < w; i++) guest(a)[i] = (uint8_t)(v >> (8 * i));
    return 0;
}

static void fail(const char* what, unsigned trial, int delta)
{
    fprintf(stderr, "TRANSPOSE MATRIX FAIL %s trial=%u delta=%d\n", what, trial, delta);
    exit(1);
}

/* Retail execution: returns $v0; the window holds the result. */
static uint32_t run_retail(uint32_t a0, uint32_t a1)
{
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;
    memset(&bus, 0, sizeof bus);
    bus.read = read_bus;
    bus.write = write_bus;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a0;
    cpu.gpr[5] = a1;
    cpu.gpr[29] = 0x801FF000u;
    cpu.gpr[31] = HALT;
    if (PcPortMipsRun(&cpu, ENTRY, HALT, 1000) != PC_PORT_MIPS_HALTED) fail("retail run", 0, 0);
    return cpu.gpr[2];
}

static void compare_case(const uint8_t* initial, int delta, unsigned trial)
{
    static uint8_t host[WINDOW_SIZE] __attribute__((aligned(16)));
    uint8_t expected[WINDOW_SIZE];
    uint32_t a0 = WINDOW + M0_OFFSET;
    uint32_t a1 = (uint32_t)((int32_t)a0 + delta);
    uint32_t v0;
    MATRIX* ret;

    memcpy(guest(WINDOW), initial, WINDOW_SIZE);
    v0 = run_retail(a0, a1);
    memcpy(expected, guest(WINDOW), WINDOW_SIZE);
    if (v0 != a1) fail("retail return", trial, delta);

    memcpy(host, initial, WINDOW_SIZE);
    ret = TransposeMatrix((MATRIX*)(host + M0_OFFSET), (MATRIX*)(host + M0_OFFSET + delta));
    if ((uint8_t*)ret != host + M0_OFFSET + delta) fail("native return", trial, delta);
    if (memcmp(host, expected, WINDOW_SIZE) != 0) fail("bytes", trial, delta);
    checks++;
}

static void fixed_cases(void)
{
    static const short in[3][3] = {{1, 2, 3}, {4, 5, 6}, {7, 8, -9}};
    MATRIX a, b;
    int i, j;

    memset(&a, 0, sizeof a);
    memcpy(a.m, in, sizeof in);
    a.t[0] = 0x11111111; a.t[1] = 0x22222222; a.t[2] = 0x33333333;
    memset(&b, 0x5A, sizeof b);
    if (TransposeMatrix(&a, &b) != &b) fail("fixed return", 0, 32);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            if (b.m[i][j] != in[j][i]) fail("fixed transpose", 0, 32);
    for (i = 18; i < (int)sizeof b; i++)
        if (((uint8_t*)&b)[i] != 0x5A) fail("fixed untouched pad/t", 0, 32);

    if (TransposeMatrix(&a, &a) != &a) fail("fixed in-place return", 0, 0);
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            if (a.m[i][j] != in[j][i]) fail("fixed in-place transpose", 0, 0);
    if (a.t[0] != 0x11111111 || a.t[1] != 0x22222222 || a.t[2] != 0x33333333)
        fail("fixed in-place t", 0, 0);
    checks += 2;
}

int main(int argc, char** argv)
{
    const char* exe = argc > 1 ? argv[1] : "disc/SLUS_006.64";
    static const uint8_t first_word[4] = {0x21, 0x10, 0xA0, 0x00}; /* addu v0,a1,zero */
    uint8_t initial[WINDOW_SIZE];
    uint32_t seed = 0x13579BDFu;
    unsigned trial, i;
    FILE* f = fopen(exe, "rb");

    if (f == NULL || fseek(f, (long)(ENTRY - EXE_LOAD_BIAS), SEEK_SET) != 0 ||
        fread(guest(ENTRY), 1, CODE_SIZE, f) != CODE_SIZE) {
        fprintf(stderr, "TRANSPOSE MATRIX FAIL cannot read retail code from %s\n", exe);
        return 1;
    }
    fclose(f);
    if (memcmp(guest(ENTRY), first_word, sizeof first_word) != 0) {
        fprintf(stderr, "TRANSPOSE MATRIX FAIL unexpected retail bytes at 0x%08X\n", ENTRY);
        return 1;
    }

    for (trial = 0; trial < 2048; trial++) {
        for (i = 0; i < WINDOW_SIZE; i++) {
            seed = seed * 1664525u + 1013904223u;
            initial[i] = (uint8_t)(seed >> 24);
        }
        compare_case(initial, 0, trial);   /* in place */
        compare_case(initial, 40, trial);  /* disjoint, above m0 */
        compare_case(initial, -32, trial); /* disjoint, below m0 */
    }
    fixed_cases();
    printf("TRANSPOSE MATRIX PASS %u comparisons: retail bytes 0x%08X vs native, offsets 0 "
           "(in place), +40 and -32 (disjoint), full 128-byte window and return pointer\n",
           checks, ENTRY);
    return 0;
}
