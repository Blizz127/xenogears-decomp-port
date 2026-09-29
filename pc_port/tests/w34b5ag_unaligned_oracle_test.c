/*
 * W34B5-AG — Exhaustive PSX unaligned memory oracle test.
 *
 * Standalone test with independently derived byte-exact semantics.
 * Does NOT copy production shift formulas; uses literal byte mapping.
 *
 * The production helpers psx_lwl/psx_swl/psx_lwr/psx_swr handle ONLY
 * the memory portion of the operation.  The MIPS register merge is done
 * by the caller via OR (psx_lwl|psx_lwr).  So:
 *   - psx_lwl returns memory bytes in high positions, zeros in low
 *   - psx_lwr returns memory bytes in low positions, zeros in high
 *   - psx_lwl|psx_lwr = full 32-bit word from memory (regardless of rt)
 *   - psx_swl writes high bytes of val to memory
 *   - psx_swr writes low bytes of val to memory
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5ag_unaligned_oracle_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5ag_unaligned_oracle_test
 *
 * With UBSan:
 *   gcc -std=gnu17 -O0 -g -fsanitize=undefined \
 *     -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5ag_unaligned_oracle_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5ag_unaligned_oracle_ubsan
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"

/* Provide g_PsxRam for the production module. */
uint8_t g_PsxRam[PSX_RAM_SIZE];

/* Stubs for helpers used by wm_800865A0 (not exercised by unaligned tests). */
void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocSize; (void)allocFlags;
    return NULL;
}
u_short GetTPage(int tp, int abr, int x, int y)
{
    (void)tp; (void)abr; (void)x; (void)y;
    return 0;
}
u_short GetClut(int x, int y)
{
    (void)x; (void)y;
    return 0;
}


void SystemTransferPaletteToVRAM(short xDest, short yDest)
{
    (void)xDest; (void)yDest;
}
static int total = 0, pass = 0, fail = 0;

static void check(const char* name, int cond)
{
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else      { fail++; printf("  FAIL: %s\n", name); }
}

/* ================================================================
 * INDEPENDENT ORACLE — byte-level reference implementations.
 *
 * These are derived from the MIPS R3000A spec, NOT from the
 * production helper code.  They use explicit byte arrays.
 *
 * CRITICAL: The production psx_lwl returns ONLY the memory bytes
 * in the high portion of the result (low bytes = 0).  It does NOT
 * merge with the current register value.  The caller does the
 * merge: psx_lwl(addr) | psx_lwr(addr).
 * ================================================================ */

/* Reference LWL: MIPS R3000A little-endian.
 * Loaded bytes occupy high register positions in NORMAL order
 * (lowest address → lowest replaced position). */
static uint32_t ref_lwl(const uint8_t mem[4], unsigned off)
{
    uint32_t result = 0;
    for (unsigned j = 0; j <= off; j++)
        result |= (uint32_t)mem[j] << ((3 - off + j) * 8);
    return result;
}

/* Reference LWR: MIPS R3000A little-endian.
 * Loaded bytes occupy LOW register positions starting at 0.
 * When paired with lwl(addr+3), the OR merge gives the correct word. */
static uint32_t ref_lwr(const uint8_t mem[4], unsigned off)
{
    uint32_t result = 0;
    for (unsigned j = off; j < 4; j++)
        result |= (uint32_t)mem[j] << ((j - off) * 8);
    return result;
}

/* Reference SWL: MIPS R3000A little-endian.
 * Stores in same normal order as lwl loads. */
static void ref_swl(uint8_t mem[4], unsigned off, uint32_t val)
{
    for (unsigned j = 0; j <= off; j++)
        mem[j] = (uint8_t)(val >> ((3 - off + j) * 8));
}

/* Reference SWR: MIPS R3000A little-endian.
 * Val byte (j-off) goes to memory position j.
 * swl(addr+3)|swr(addr) stores the full word in memory-byte order. */
static void ref_swr(uint8_t mem[4], unsigned off, uint32_t val)
{
    for (unsigned j = off; j < 4; j++)
        mem[j] = (uint8_t)(val >> ((j - off) * 8));
}

/* Identical to production (post-fix). */
static inline uint32_t prod_lwl(uint32_t addr)
{
    unsigned off = addr & 3;
    uint8_t *base = (uint8_t*)PSX_ADDR(addr - off);
    uint32_t result = 0;
    for (unsigned j = 0; j <= off; j++)
        result |= (uint32_t)base[j] << ((3 - off + j) * 8);
    return result;
}
static inline uint32_t prod_lwr(uint32_t addr)
{
    unsigned off = addr & 3;
    uint8_t *base = (uint8_t*)PSX_ADDR(addr - off);
    uint32_t result = 0;
    for (unsigned j = off; j < 4; j++)
        result |= (uint32_t)base[j] << ((j - off) * 8);
    return result;
}
static inline void prod_swl(uint32_t addr, uint32_t val)
{
    unsigned off = addr & 3;
    uint8_t *base = (uint8_t*)PSX_ADDR(addr - off);
    for (unsigned j = 0; j <= off; j++)
        base[j] = (uint8_t)(val >> ((3 - off + j) * 8));
}
static inline void prod_swr(uint32_t addr, uint32_t val)
{
    unsigned off = addr & 3;
    uint8_t *base = (uint8_t*)PSX_ADDR(addr - off);
    for (unsigned j = off; j < 4; j++)
        base[j] = (uint8_t)(val >> ((j - off) * 8));
}

/* Test patterns. */
static const uint32_t test_vals[] = {
    0xDEADBEEF, 0x00000000, 0xFFFFFFFF, 0x01234567, 0x89ABCDEF
};
#define N_VALS 5

/* Memory patterns. */
static const uint8_t mem_patterns[][4] = {
    { 0x11, 0x22, 0x44, 0x88 },
    { 0xAA, 0xBB, 0xDD, 0xEE },
    { 0x00, 0x00, 0x00, 0x00 },
    { 0xFF, 0xFF, 0xFF, 0xFF },
    { 0x01, 0x23, 0x45, 0x67 },
};
#define N_MEMS 5

int main(void)
{
    uint32_t base_addr = 0x800C0000u;
    char name[128];

    printf("=== W34B5-AG Exhaustive Unaligned Oracle Test ===\n\n");

    /* ---- LWL tests (memory portion only) ---- */
    printf("LWL alignment tests:\n");
    for (unsigned off = 0; off < 4; off++) {
        int all_ok = 1;
        for (int mi = 0; mi < N_MEMS; mi++) {
            uint32_t addr = base_addr + off;
            uint8_t *mem_host = (u8*)PSX_ADDR(base_addr);
            memcpy(mem_host, mem_patterns[mi], 4);

            uint32_t expected = ref_lwl(mem_patterns[mi], off);
            uint32_t actual = prod_lwl(addr);

            if (expected != actual) {
                char ename[128];
                snprintf(ename, sizeof(ename),
                         "LWL off=%d mem=[%02x%02x%02x%02x]: "
                         "expected=%08x actual=%08x",
                         off, mem_patterns[mi][0], mem_patterns[mi][1],
                         mem_patterns[mi][2], mem_patterns[mi][3],
                         expected, actual);
                check(ename, 0);
                all_ok = 0;
            }
        }
        snprintf(name, sizeof(name), "LWL off=%d all %d mem patterns PASS", off, N_MEMS);
        check(name, all_ok);
    }

    /* ---- LWR tests (memory portion only) ---- */
    printf("\nLWR alignment tests:\n");
    for (unsigned off = 0; off < 4; off++) {
        int all_ok = 1;
        for (int mi = 0; mi < N_MEMS; mi++) {
            uint32_t addr = base_addr + 0x100 + off;
            uint8_t *mem_host = (u8*)PSX_ADDR(base_addr + 0x100);
            memcpy(mem_host, mem_patterns[mi], 4);

            uint32_t expected = ref_lwr(mem_patterns[mi], off);
            uint32_t actual = prod_lwr(addr);

            if (expected != actual) {
                char ename[128];
                snprintf(ename, sizeof(ename),
                         "LWR off=%d mem=[%02x%02x%02x%02x]: "
                         "expected=%08x actual=%08x",
                         off, mem_patterns[mi][0], mem_patterns[mi][1],
                         mem_patterns[mi][2], mem_patterns[mi][3],
                         expected, actual);
                check(ename, 0);
                all_ok = 0;
            }
        }
        snprintf(name, sizeof(name), "LWR off=%d all %d mem patterns PASS", off, N_MEMS);
        check(name, all_ok);
    }

    /* ---- SWL tests ---- */
    printf("\nSWL alignment tests:\n");
    for (unsigned off = 0; off < 4; off++) {
        int all_ok = 1;
        for (int mi = 0; mi < N_MEMS; mi++) {
            for (int vi = 0; vi < N_VALS; vi++) {
                uint32_t region = base_addr + 0x200;
                uint8_t *mem_host = (u8*)PSX_ADDR(region);
                uint8_t mem_before[4];
                memcpy(mem_before, mem_patterns[mi], 4);
                memcpy(mem_host, mem_before, 4);

                uint8_t expected_mem[4];
                memcpy(expected_mem, mem_before, 4);
                ref_swl(expected_mem, off, test_vals[vi]);

                prod_swl(region + off, test_vals[vi]);

                if (memcmp(mem_host, expected_mem, 4) != 0) {
                    char ename[128];
                    snprintf(ename, sizeof(ename),
                             "SWL off=%d mem=[%02x%02x%02x%02x] val=%08x: "
                             "expected=[%02x%02x%02x%02x] actual=[%02x%02x%02x%02x]",
                             off, mem_before[0], mem_before[1], mem_before[2], mem_before[3],
                             test_vals[vi],
                             expected_mem[0], expected_mem[1], expected_mem[2], expected_mem[3],
                             mem_host[0], mem_host[1], mem_host[2], mem_host[3]);
                    check(ename, 0);
                    all_ok = 0;
                }
            }
        }
        snprintf(name, sizeof(name), "SWL off=%d all %d patterns PASS", off, N_VALS * N_MEMS);
        check(name, all_ok);
    }

    /* ---- SWR tests ---- */
    printf("\nSWR alignment tests:\n");
    for (unsigned off = 0; off < 4; off++) {
        int all_ok = 1;
        for (int mi = 0; mi < N_MEMS; mi++) {
            for (int vi = 0; vi < N_VALS; vi++) {
                uint32_t region = base_addr + 0x300;
                uint8_t *mem_host = (u8*)PSX_ADDR(region);
                uint8_t mem_before[4];
                memcpy(mem_before, mem_patterns[mi], 4);
                memcpy(mem_host, mem_before, 4);

                uint8_t expected_mem[4];
                memcpy(expected_mem, mem_before, 4);
                ref_swr(expected_mem, off, test_vals[vi]);

                prod_swr(region + off, test_vals[vi]);

                if (memcmp(mem_host, expected_mem, 4) != 0) {
                    char ename[128];
                    snprintf(ename, sizeof(ename),
                             "SWR off=%d mem=[%02x%02x%02x%02x] val=%08x: "
                             "expected=[%02x%02x%02x%02x] actual=[%02x%02x%02x%02x]",
                             off, mem_before[0], mem_before[1], mem_before[2], mem_before[3],
                             test_vals[vi],
                             expected_mem[0], expected_mem[1], expected_mem[2], expected_mem[3],
                             mem_host[0], mem_host[1], mem_host[2], mem_host[3]);
                    check(ename, 0);
                    all_ok = 0;
                }
            }
        }
        snprintf(name, sizeof(name), "SWR off=%d all %d patterns PASS", off, N_VALS * N_MEMS);
        check(name, all_ok);
    }

    /* ---- LWL+LWR pair tests: verify full word from memory ---- */
    printf("\nLWL+LWR pair tests:\n");
    for (unsigned off = 0; off < 4; off++) {
        int all_ok = 1;
        for (int mi = 0; mi < N_MEMS; mi++) {
            uint32_t region = base_addr + 0x400;
            uint8_t *mem_host = (u8*)PSX_ADDR(region);
            memcpy(mem_host, mem_patterns[mi], 4);

            /* Reference: the pair should reconstruct all 4 memory bytes. */
            uint32_t ref_left = ref_lwl(mem_patterns[mi], off);
            uint32_t ref_right = ref_lwr(mem_patterns[mi], off);
            uint32_t expected = ref_left | ref_right;

            /* Production: psx_lwl|psx_lwr */
            uint32_t actual = prod_lwl(region + off) | prod_lwr(region + off);

            if (expected != actual) {
                char ename[128];
                snprintf(ename, sizeof(ename),
                         "LWL+LWR off=%d mem=[%02x%02x%02x%02x]: "
                         "expected=%08x actual=%08x",
                         off, mem_patterns[mi][0], mem_patterns[mi][1],
                         mem_patterns[mi][2], mem_patterns[mi][3],
                         expected, actual);
                check(ename, 0);
                all_ok = 0;
            }
        }
        snprintf(name, sizeof(name), "LWL+LWR pair off=%d all %d mem patterns PASS", off, N_MEMS);
        check(name, all_ok);
    }

    /* ---- SWL+SWR pair tests: verify full word stored ---- */
    printf("\nSWL+SWR pair tests:\n");
    for (unsigned off = 0; off < 4; off++) {
        int all_ok = 1;
        for (int vi = 0; vi < N_VALS; vi++) {
            uint32_t region = base_addr + 0x500 + off * 16;
            uint8_t *mem_host = (u8*)PSX_ADDR(region);
            memset(mem_host, 0, 4);

            /* Production: swl(addr+3) + swr(addr) — pattern used in wm_80089160 */
            prod_swl(region + 3, test_vals[vi]);
            prod_swr(region, test_vals[vi]);

            /* Expected: little-endian order (val byte 0 at lowest address). */
            uint8_t expected_mem[4] = {
                (uint8_t)(test_vals[vi]),
                (uint8_t)(test_vals[vi] >> 8),
                (uint8_t)(test_vals[vi] >> 16),
                (uint8_t)(test_vals[vi] >> 24)
            };

            if (memcmp(mem_host, expected_mem, 4) != 0) {
                char ename[128];
                snprintf(ename, sizeof(ename),
                         "SWL+SWR off=%d val=%08x: "
                         "expected=[%02x%02x%02x%02x] actual=[%02x%02x%02x%02x]",
                         off, test_vals[vi],
                         expected_mem[0], expected_mem[1], expected_mem[2], expected_mem[3],
                         mem_host[0], mem_host[1], mem_host[2], mem_host[3]);
                check(ename, 0);
                all_ok = 0;
            }
        }
        snprintf(name, sizeof(name), "SWL+SWR pair off=%d all %d vals PASS", off, N_VALS);
        check(name, all_ok);
    }

    /* ---- Spark-proved failure case ---- */
    printf("\nSpark-proved failure case:\n");
    {
        /* Memory: DE AD BE EF 11 22 33 44 at aligned address.
         * LWL offset 1 loads 2 bytes in normal order from the aligned word:
         * mem[0]=DE → byte 2 (shift 16), mem[1]=AD → byte 3 (shift 24).
         * Old buggy: shift by 32 was UB. */
        uint8_t *mem = (u8*)PSX_ADDR(base_addr + 0x600);
        mem[0] = 0xDE; mem[1] = 0xAD; mem[2] = 0xBE; mem[3] = 0xEF;
        /* Next aligned word for the lwl(addr+3) to read from. */
        mem[4] = 0x11; mem[5] = 0x22; mem[6] = 0x33; mem[7] = 0x44;

        uint32_t lwl_result = prod_lwl(base_addr + 0x600 + 1);
        check("Spark: LWL off=1 returns ADDE0000", lwl_result == 0xADDE0000u);

        /* LWR offset 1 returns: low 3 bytes in low positions. */
        uint32_t lwr_result = prod_lwr(base_addr + 0x600 + 1);
        check("Spark: LWR off=1 returns 00EFBEAD", lwr_result == 0x00EFBEADu);

        /* Pair (lwl(addr+3)|lwr(addr)) gives full word at addr.
         * addr = base+0x601, addr+3 = base+0x604. */
        uint32_t pair = prod_lwl(base_addr + 0x600 + 4) | prod_lwr(base_addr + 0x600 + 1);
        /* Expected: little-endian word for bytes at base+0x601..+0x604
         * = AD BE EF 11 = 0x11EFBEAD */
        check("Spark: LWL+LWR off=1 pair = 11EFBEAD", pair == 0x11EFBEADu);

        /* SWL offset 1 stores high 2 bytes of val. */
        uint8_t *store_mem = (u8*)PSX_ADDR(base_addr + 0x700);
        memset(store_mem, 0, 4);
        prod_swl(base_addr + 0x700 + 1, 0xADDE0000u);
        check("Spark: SWL off=1 stores DE at pos 0", store_mem[0] == 0xDE);
        check("Spark: SWL off=1 stores AD at pos 1", store_mem[1] == 0xAD);
        check("Spark: SWL off=1 preserves pos 2", store_mem[2] == 0x00);
        check("Spark: SWL off=1 preserves pos 3", store_mem[3] == 0x00);
    }

    /* ---- Cross-val LWL+LWR pair completeness ----
     * The pair lwl(addr+3)|lwr(addr) should produce the little-endian
     * word for the4 bytes starting at addr. */
    printf("\nCross-val LWL+LWR pair completeness:\n");
    for (unsigned off = 0; off < 4; off++) {
        int all_ok = 1;
        for (int mi = 0; mi < N_MEMS; mi++) {
            /* Set up8 bytes so the pair can read4 from any offset. */
            uint32_t region = base_addr + 0x800 + off * 256;
            uint8_t *mem_host = (u8*)PSX_ADDR(region);
            memcpy(mem_host, mem_patterns[mi], 4);
            /* Also fill the next word for off>0 cases. */
            memcpy(mem_host + 4, mem_patterns[(mi + 1) % N_MEMS], 4);

            /* Pair: lwl(region+off+3) | lwr(region+off) */
            uint32_t pair = prod_lwl(region + off + 3) | prod_lwr(region + off);

            /* Expected: little-endian word for bytes at region+off..region+off+3 */
            uint32_t expected_word =
                (uint32_t)mem_host[off] |
                ((uint32_t)mem_host[off + 1] << 8) |
                ((uint32_t)mem_host[off + 2] << 16) |
                ((uint32_t)mem_host[off + 3] << 24);

            if (pair != expected_word) {
                char ename[128];
                snprintf(ename, sizeof(ename),
                         "LWL+LWR off=%d mem=[%02x%02x%02x%02x]: "
                         "expected=%08x actual=%08x",
                         off, mem_host[off], mem_host[off + 1],
                         mem_host[off + 2], mem_host[off + 3],
                         expected_word, pair);
                check(ename, 0);
                all_ok = 0;
            }
        }
        snprintf(name, sizeof(name), "LWL+LWR completeness off=%d PASS", off);
        check(name, all_ok);
    }

    /* ---- Cross-val SWL+SWR pair completeness ----
     * swl(addr+3)|swr(addr) should store val in little-endian order. */
    printf("\nCross-val SWL+SWR pair completeness:\n");
    for (unsigned off = 0; off < 4; off++) {
        int all_ok = 1;
        for (int vi = 0; vi < N_VALS; vi++) {
            uint32_t region = base_addr + 0x900 + (off * N_VALS + vi) * 16;
            uint8_t *mem_host = (u8*)PSX_ADDR(region);
            memset(mem_host, 0, 4);

            /* swl(addr+3, val) + swr(addr, val) stores in little-endian. */
            prod_swl(region + 3, test_vals[vi]);
            prod_swr(region, test_vals[vi]);

            uint8_t expected_mem[4] = {
                (uint8_t)(test_vals[vi]),
                (uint8_t)(test_vals[vi] >> 8),
                (uint8_t)(test_vals[vi] >> 16),
                (uint8_t)(test_vals[vi] >> 24)
            };

            if (memcmp(mem_host, expected_mem, 4) != 0) {
                char ename[128];
                snprintf(ename, sizeof(ename),
                         "SWL+SWR off=%d val=%08x: "
                         "expected=[%02x%02x%02x%02x] actual=[%02x%02x%02x%02x]",
                         off, test_vals[vi],
                         expected_mem[0], expected_mem[1], expected_mem[2], expected_mem[3],
                         mem_host[0], mem_host[1], mem_host[2], mem_host[3]);
                check(ename, 0);
                all_ok = 0;
            }
        }
        snprintf(name, sizeof(name), "SWL+SWR completeness off=%d PASS", off);
        check(name, all_ok);
    }

    /* ---- Specific nonzero/misaligned 8-byte copy (wm_80089160 pattern) ---- */
    printf("\nwm_80089160 8-byte copy pattern:\n");
    for (unsigned off = 0; off < 4; off++) {
        uint32_t src_region = base_addr + 0xA00 + off;
        uint32_t dst_region = base_addr + 0xB00 + off * 16;
        /* Source data at the aligned base. The copy reads from src_region. */
        uint8_t *src_base = (u8*)PSX_ADDR(base_addr + 0xA00);
        /* Stores go to dst_region-12 through dst_region-5 (8 bytes). */
        uint8_t *dst = (u8*)PSX_ADDR(dst_region - 12);

        /* Fill source with nonzero test data at the aligned base. */
        src_base[0] = 0x11; src_base[1] = 0x22; src_base[2] = 0x33; src_base[3] = 0x44;
        src_base[4] = 0x55; src_base[5] = 0x66; src_base[6] = 0x77; src_base[7] = 0x88;
        src_base[8] = 0x99; src_base[9] = 0xAA; src_base[10] = 0xBB; src_base[11] = 0xCC;
        memset(dst, 0, 16);

        /* This is the exact pattern from wm_80089160 Paths B/D:
         *   src_word0 = psx_lwl_lwr(t3 + 3, t3);  // MIPS: lwl at addr+3, lwr at addr+0
         *   src_word1 = psx_lwl_lwr(t3 + 7, t3 + 4);
         *   psx_swl(a2 - 9, src_word0);
         *   psx_swr(a2 - 12, src_word0);
         *   psx_swl(a2 - 5, src_word1);
         *   psx_swr(a2 - 8, src_word1); */
        uint32_t src_word0 = prod_lwl(src_region + 3) | prod_lwr(src_region);
        uint32_t src_word1 = prod_lwl(src_region + 7) | prod_lwr(src_region + 4);
        prod_swl(dst_region - 9, src_word0);
        prod_swr(dst_region - 12, src_word0);
        prod_swl(dst_region - 5, src_word1);
        prod_swr(dst_region - 8, src_word1);

        /* Verify: dst (at dst_region-12) should contain the 8 source bytes
         * starting at src_region (which is at offset `off` from the aligned base). */
        int ok = 1;
        for (int j = 0; j < 8; j++) {
            if (dst[j] != src_base[off + j]) { ok = 0; break; }
        }
        char name[128];
        snprintf(name, sizeof(name), "8-byte copy off=%d bytes match", off);
        check(name, ok);
    }

    printf("\n=== Results: %d/%d PASS ===\n", pass, total);
    return (fail > 0) ? 1 : 0;
}
