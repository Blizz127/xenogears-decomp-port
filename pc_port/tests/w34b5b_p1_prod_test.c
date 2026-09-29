/*
 * W34B5B — Production test for wm_800978FC and common-tail P1 slice.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5b_p1_prod_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5b_p1_prod_test
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"

/* Provide g_PsxRam for the production module. */
uint8_t g_PsxRam[PSX_RAM_SIZE];

/* HeapAlloc stub: allocates from top of g_PsxRam to produce valid KUSEG pointers. */
static int s_heap_offset = 0;
static int s_heap_alloc_count = 0;

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocFlags;
    /* Allocate from the top of PSX RAM, growing downward. */
    s_heap_offset += allocSize;
    s_heap_offset = (s_heap_offset + 15) & ~15;
    void *p = &g_PsxRam[PSX_RAM_SIZE - s_heap_offset];
    memset(p, 0, allocSize);  /* HeapAlloc zero-fills. */
    s_heap_alloc_count++;
    return p;
}

/* Stubs for helpers used by wm_800865A0 (not exercised by P1 tests). */
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

int main(void)
{
    printf("=== W34B5B wm_800978FC + P1 production test ===\n\n");

    /* ---- Test 1: wm_800978FC basic allocation ---- */
    printf("Test 1: wm_800978FC basic allocation\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_800978FC();

        check("2 HeapAlloc calls", s_heap_alloc_count == 2);

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        check("D_8009BC3C non-zero", ptr1 != 0);

        u32 ptr2 = *(u32*)PSX_ADDR(0x8009BCB4);
        check("D_8009BCB4 non-zero", ptr2 != 0);

        check("ptr1 != ptr2", ptr1 != ptr2);
    }

    /* ---- Test 2: Record initialization pattern ---- */
    printf("\nTest 2: Record initialization pattern\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_800978FC();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 v1 = ptr1 + 6;

        check("record0 byte[-3] == 7",    *(u8*)PSX_ADDR(v1 - 3) == 7);
        check("record0 byte[-2] == 0x80", *(u8*)PSX_ADDR(v1 - 2) == 0x80);
        check("record0 byte[-1] == 0x80", *(u8*)PSX_ADDR(v1 - 1) == 0x80);
        check("record0 byte[+0] == 0x80", *(u8*)PSX_ADDR(v1 + 0) == 0x80);
        check("record0 byte[+1] == 0x24", *(u8*)PSX_ADDR(v1 + 1) == 0x24);

        /* Last record. */
        u32 v1_last = ptr1 + 6 + 2047 * 32;
        check("record2047 byte[-3] == 7",    *(u8*)PSX_ADDR(v1_last - 3) == 7);
        check("record2047 byte[-2] == 0x80", *(u8*)PSX_ADDR(v1_last - 2) == 0x80);
        check("record2047 byte[-1] == 0x80", *(u8*)PSX_ADDR(v1_last - 1) == 0x80);
        check("record2047 byte[+0] == 0x80", *(u8*)PSX_ADDR(v1_last + 0) == 0x80);
        check("record2047 byte[+1] == 0x24", *(u8*)PSX_ADDR(v1_last + 1) == 0x24);

        /* Middle record. */
        u32 v1_mid = ptr1 + 6 + 1024 * 32;
        check("record1024 byte[-3] == 7",    *(u8*)PSX_ADDR(v1_mid - 3) == 7);
        check("record1024 byte[+1] == 0x24", *(u8*)PSX_ADDR(v1_mid + 1) == 0x24);
    }

    /* ---- Test 3: Copy verification ---- */
    printf("\nTest 3: Copy verification\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_800978FC();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 ptr2 = *(u32*)PSX_ADDR(0x8009BCB4);

        int mismatch = 0;
        for (u32 off = 0; off < 0x10000; off += 4) {
            if (*(u32*)PSX_ADDR(ptr1 + off) != *(u32*)PSX_ADDR(ptr2 + off))
                mismatch++;
        }
        check("copy: zero mismatches in 64KB", mismatch == 0);
    }

    /* ---- Test 4: Record count ---- */
    printf("\nTest 4: Record count\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_800978FC();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        int count = 0;
        for (int i = 0; i < 2048; i++) {
            u32 v = ptr1 + 6 + i * 32;
            if (*(u8*)PSX_ADDR(v - 3) == 7 && *(u8*)PSX_ADDR(v + 1) == 0x24)
                count++;
        }
        check("2048 records initialized", count == 2048);
    }

    /* ---- Test 5: Neighboring guard ---- */
    printf("\nTest 5: Neighboring guard\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_800978FC();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 v1 = ptr1 + 6;

        check("byte v1-4 == 0 (before pattern)", *(u8*)PSX_ADDR(v1 - 4) == 0);
        check("byte v1+2 == 0 (after pattern)",  *(u8*)PSX_ADDR(v1 + 2) == 0);

        /* Bytes v1+3..v1+28 should be zero (within current record).
         * v1+29..v1+31 are the start of the NEXT record's init pattern
         * (records are stride 32, so v1+29 = next record's v1-3). */
        int all_zero = 1;
        for (int j = 3; j <= 28; j++) {
            if (*(u8*)PSX_ADDR(v1 + j) != 0) { all_zero = 0; break; }
        }
        check("record0 bytes[3..28] == 0 (within record)", all_zero);

        /* Verify v1+29..v1+31 are the NEXT record's init bytes. */
        check("record1 byte[-3] at v1+29 == 7",    *(u8*)PSX_ADDR(v1 + 29) == 7);
        check("record1 byte[-2] at v1+30 == 0x80", *(u8*)PSX_ADDR(v1 + 30) == 0x80);
        check("record1 byte[-1] at v1+31 == 0x80", *(u8*)PSX_ADDR(v1 + 31) == 0x80);
    }

    /* ---- Test 6: Table A/B preserved ---- */
    printf("\nTest 6: Table A/B preserved\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u32*)PSX_ADDR(0x80099E8C) = 0xDEADBEEF;
        *(u32*)PSX_ADDR(0x8009A034) = 0xCAFEBABE;

        s_heap_offset = 0;
        wm_800978FC();

        check("table A sentinel preserved", *(u32*)PSX_ADDR(0x80099E8C) == 0xDEADBEEF);
        check("table B sentinel preserved", *(u32*)PSX_ADDR(0x8009A034) == 0xCAFEBABE);
    }

    /* ---- Test 7: State preservation ---- */
    printf("\nTest 7: State preservation\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u32*)PSX_ADDR(0x8009C610) = 0;
        *(u32*)PSX_ADDR(0x8009C618) = 1024;
        *(u32*)PSX_ADDR(0x8009BCDC) = 256;

        s_heap_offset = 0;
        wm_800978FC();

        check("C610 preserved", *(u32*)PSX_ADDR(0x8009C610) == 0);
        check("C618 preserved", *(u32*)PSX_ADDR(0x8009C618) == 1024);
        check("BCDC preserved", *(u32*)PSX_ADDR(0x8009BCDC) == 256);
    }

    /* ---- Test 8: P1 caller slice ---- */
    printf("\nTest 8: P1 caller slice\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_common_tail_p1_reset();

        u32 cut = wm_8007293C_common_tail_p1();

        check("P1 entry == 1", wm_ctp1_get_entry() == 1);
        check("P1 978fc_calls == 1", wm_ctp1_get_978fc_calls() == 1);
        check("P1 cut == 0x80072944", cut == 0x80072944);
        check("P1 last_cut == 0x80072944", wm_ctp1_get_last_cut() == 0x80072944);
        check("HeapAlloc called 2x", s_heap_alloc_count == 2);
    }

    /* ---- Test 9: P1 repeated calls ---- */
    printf("\nTest 9: P1 repeated calls\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p1_reset();
        wm_8007293C_common_tail_p1();
        wm_8007293C_common_tail_p1();

        check("P1 entry == 2 after two calls", wm_ctp1_get_entry() == 2);

        wm_common_tail_p1_reset();
        check("P1 entry == 0 after reset", wm_ctp1_get_entry() == 0);
        check("P1 calls == 0 after reset", wm_ctp1_get_978fc_calls() == 0);
    }

    /* ---- Test 10: P0 state preserved through P1 ---- */
    printf("\nTest 10: P0 state preserved through P1\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p0_reset();
        wm_8007290C_common_tail_p0();
        u32 p0_cut = wm_ctp0_get_last_cut();
        check("P0 cut == 0x8007293C", p0_cut == 0x8007293C);

        wm_common_tail_p1_reset();
        u32 p1_cut = wm_8007293C_common_tail_p1();
        check("P1 cut == 0x80072944", p1_cut == 0x80072944);

        check("P0 entry still 1", wm_ctp0_get_entry() == 1);
        check("P0 D_80059179 == 1", *(u8*)PSX_ADDR(0x80059179) == 1);
    }

    printf("\n=== Results: %d/%d PASS ===\n", pass, total);
    return (fail > 0) ? 1 : 0;
}
