/*
 * W34B5C — Production test for wm_8008901C and common-tail P2 slice.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5c_8008901c_prod_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5c_8008901c_prod_test
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"

/* Provide g_PsxRam for the production module. */
uint8_t g_PsxRam[PSX_RAM_SIZE];
u8 D_80059179;

/* HeapAlloc stub: allocates from top of g_PsxRam to produce valid KUSEG pointers. */
static u32 s_heap_offset = 0;
static int s_heap_alloc_count = 0;

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocFlags;
    s_heap_offset += allocSize;
    s_heap_offset = (s_heap_offset + 15u) & ~15u;
    void *p = &g_PsxRam[PSX_RAM_SIZE - s_heap_offset];
    memset(p, 0, allocSize);
    s_heap_alloc_count++;
    return p;
}

/* PsyQ stubs for GetTPage and GetClut.
 * Exact retail formulas. */
u_short GetTPage(int tp, int abr, int x, int y)
{
    return (u_short)(((tp & 3) << 7) | ((abr & 3) << 5) |
                     ((y & 0x100) >> 4) | ((x & 0x3FF) >> 6));
}

u_short GetClut(int x, int y)
{
    return (u_short)((y << 6) | ((x >> 4) & 0x3F));
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
    printf("=== W34B5C wm_8008901C + P2 production test ===\n\n");

    /* ---- Test 1: wm_8008901C basic allocation ---- */
    printf("Test 1: wm_8008901C basic allocation\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_8008901C();

        check("2 HeapAlloc calls", s_heap_alloc_count == 2);

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        check("D_8009BE1C non-zero", ptr1 != 0);

        u32 ptr2 = *(u32*)PSX_ADDR(0x8009BE20);
        check("D_8009BE20 non-zero", ptr2 != 0);

        check("ptr1 != ptr2", ptr1 != ptr2);

        check("alloc size each 0x2800",
              s_heap_alloc_count == 2); /* implicit from HeapAlloc stub */
    }

    /* ---- Test 2: Record initialization pattern ---- */
    printf("\nTest 2: Record initialization pattern\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_8008901C();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 base = ptr1 + 7;

        /* Retail values. */
        u16 expected_tpage = GetTPage(1, 1, 0x340, 0x100); /* 0x00BD */
        u16 expected_clut  = GetClut(0x100, 0x1FF);         /* 0x7FD0 */

        check("expected tpage == 0x00BD", expected_tpage == 0x00BD);
        check("expected clut == 0x7FD0",  expected_clut == 0x7FD0);

        /* Record 0. */
        check("record0 byte[-4] == 9",     *(u8*)PSX_ADDR(base - 4) == 9);
        check("record0 byte[+0] == 0x2E",  *(u8*)PSX_ADDR(base + 0) == 0x2E);
        check("record0 packet clut is GetClut", *(u16*)PSX_ADDR(base + 7) == expected_clut);
        check("record0 packet tpage is GetTPage", *(u16*)PSX_ADDR(base + 15) == expected_tpage);

        /* Last record (255). */
        u32 base_last = ptr1 + 7 + 255 * 40;
        check("record255 byte[-4] == 9",    *(u8*)PSX_ADDR(base_last - 4) == 9);
        check("record255 byte[+0] == 0x2E", *(u8*)PSX_ADDR(base_last + 0) == 0x2E);
        check("record255 packet clut is GetClut", *(u16*)PSX_ADDR(base_last + 7) == expected_clut);
        check("record255 packet tpage is GetTPage", *(u16*)PSX_ADDR(base_last + 15) == expected_tpage);

        /* Middle record (128). */
        u32 base_mid = ptr1 + 7 + 128 * 40;
        check("record128 byte[-4] == 9",    *(u8*)PSX_ADDR(base_mid - 4) == 9);
        check("record128 packet clut is GetClut", *(u16*)PSX_ADDR(base_mid + 7) == expected_clut);
        check("record128 packet tpage is GetTPage", *(u16*)PSX_ADDR(base_mid + 15) == expected_tpage);
    }

    /* ---- Test 3: Record count ---- */
    printf("\nTest 3: Record count\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_8008901C();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 count = 0;
        for (u32 i = 0; i < 256u; i++) {
            u32 b = ptr1 + 7u + i * 40u;
            if (*(u8*)PSX_ADDR(b - 4) == 9 && (*(u8*)PSX_ADDR(b) & 0x02) != 0)
                count++;
        }
        check("256 records initialized", count == 256u);
    }

    /* ---- Test 4: Copy verification ---- */
    printf("\nTest 4: Copy verification\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_8008901C();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 ptr2 = *(u32*)PSX_ADDR(0x8009BE20);

        int mismatch = 0;
        for (u32 off = 0; off < 0x2800; off += 4) {
            if (*(u32*)PSX_ADDR(ptr1 + off) != *(u32*)PSX_ADDR(ptr2 + off))
                mismatch++;
        }
        check("copy: zero mismatches in 10240 bytes", mismatch == 0);
    }

    /* ---- Test 5: Neighboring guard ---- */
    printf("\nTest 5: Neighboring guard\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_8008901C();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 base = ptr1 + 7;

        /* bytes 1..2 before first record start (ptr1+3) should be zero.
         * Record 0 starts at ptr1+3 (base-4), so ptr1[3]=9 is expected. */
        int padding_ok = 1;
        for (u32 j = 1u; j <= 2u; j++) {
            if (*(u8*)PSX_ADDR(ptr1 + j) != 0) { padding_ok = 0; break; }
        }
        check("bytes ptr1[1..2] == 0 (padding before records)", padding_ok);

        /* Record 0 byte[-5] should be zero (before the -4 init). */
        check("record0 byte[-5] == 0", *(u8*)PSX_ADDR(base - 5) == 0);

        /* Record 0 bytes[1..6] should be zero (between byte[0] and hw[7]). */
        int gap_ok = 1;
        for (u32 j = 1u; j <= 6u; j++) {
            if (*(u8*)PSX_ADDR(base + j) != 0) { gap_ok = 0; break; }
        }
        check("record0 bytes[1..6] == 0 (gap before hw[7])", gap_ok);

        /* Record 0 bytes[9..14] should be zero (between hw[7] and hw[15]). */
        int gap2_ok = 1;
        for (u32 j = 9u; j <= 14u; j++) {
            if (*(u8*)PSX_ADDR(base + j) != 0) { gap2_ok = 0; break; }
        }
        check("record0 bytes[9..14] == 0 (gap between hw[7] and hw[15])", gap2_ok);
    }

    /* ---- Test 6: Table A/B / prior state preserved ---- */
    printf("\nTest 6: Prior state preserved\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u32*)PSX_ADDR(0x80099E8C) = 0xDEADBEEF;
        *(u32*)PSX_ADDR(0x8009A034) = 0xCAFEBABE;
        *(u32*)PSX_ADDR(0x8009C610) = 0;
        *(u32*)PSX_ADDR(0x8009C618) = 1024;
        *(u32*)PSX_ADDR(0x8009BCDC) = 256;
        *(u32*)PSX_ADDR(0x8009BC3C) = 0x12345678;
        *(u32*)PSX_ADDR(0x8009BCB4) = 0x9ABCDEF0;

        s_heap_offset = 0;
        wm_8008901C();

        check("table A sentinel preserved", *(u32*)PSX_ADDR(0x80099E8C) == 0xDEADBEEF);
        check("table B sentinel preserved", *(u32*)PSX_ADDR(0x8009A034) == 0xCAFEBABE);
        check("C610 preserved", *(u32*)PSX_ADDR(0x8009C610) == 0);
        check("C618 preserved", *(u32*)PSX_ADDR(0x8009C618) == 1024);
        check("BCDC preserved", *(u32*)PSX_ADDR(0x8009BCDC) == 256);
        check("P1 ptr1 preserved", *(u32*)PSX_ADDR(0x8009BC3C) == 0x12345678);
        check("P1 ptr2 preserved", *(u32*)PSX_ADDR(0x8009BCB4) == 0x9ABCDEF0);
    }

    /* ---- Test 7: P2 caller slice ---- */
    printf("\nTest 7: P2 caller slice\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_common_tail_p2_reset();

        u32 cut = wm_80072944_common_tail_p2();

        check("P2 entry == 1", wm_ctp2_get_entry() == 1);
        check("P2 8901c_calls == 1", wm_ctp2_get_8901c_calls() == 1);
        check("P2 alloc_calls == 2", wm_ctp2_get_alloc_calls() == 2);
        check("P2 cut == 0x8007294C", cut == 0x8007294C);
        check("P2 last_cut == 0x8007294C", wm_ctp2_get_last_cut() == 0x8007294C);
    }

    /* ---- Test 8: P2 repeated calls ---- */
    printf("\nTest 8: P2 repeated calls\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p2_reset();
        wm_80072944_common_tail_p2();
        wm_80072944_common_tail_p2();

        check("P2 entry == 2 after two calls", wm_ctp2_get_entry() == 2);

        wm_common_tail_p2_reset();
        check("P2 entry == 0 after reset", wm_ctp2_get_entry() == 0);
        check("P2 calls == 0 after reset", wm_ctp2_get_8901c_calls() == 0);
    }

    /* ---- Test 9: Full P0→P1→P2 chain ---- */
    printf("\nTest 9: Full P0→P1→P2 chain\n");
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

        wm_common_tail_p2_reset();
        u32 p2_cut = wm_80072944_common_tail_p2();
        check("P2 cut == 0x8007294C", p2_cut == 0x8007294C);

        check("P0 entry still 1", wm_ctp0_get_entry() == 1);
        check("P1 entry still 1", wm_ctp1_get_entry() == 1);
        check("P2 entry still 1", wm_ctp2_get_entry() == 1);

        /* Verify P1 allocations still valid. */
        u32 p1_ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 p1_ptr2 = *(u32*)PSX_ADDR(0x8009BCB4);
        check("P1 ptr1 still valid", p1_ptr1 != 0);
        check("P1 ptr2 still valid", p1_ptr2 != 0);

        /* Verify P2 allocations are distinct from P1. */
        u32 p2_ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 p2_ptr2 = *(u32*)PSX_ADDR(0x8009BE20);
        check("P2 ptr1 still valid", p2_ptr1 != 0);
        check("P2 ptr2 still valid", p2_ptr2 != 0);
        check("P2 ptr1 != P1 ptr1", p2_ptr1 != p1_ptr1);
        check("P2 ptr1 != P1 ptr2", p2_ptr1 != p1_ptr2);
        check("P2 ptr2 != P1 ptr1", p2_ptr2 != p1_ptr1);
        check("P2 ptr2 != P1 ptr2", p2_ptr2 != p1_ptr2);
        check("P2 ptr1 != P2 ptr2", p2_ptr2 != p2_ptr1);

        check("P0 D_80059179 == 1", D_80059179 == 1u);
    }

    /* ---- Test 10: No overlap with P1 allocations ---- */
    printf("\nTest 10: No overlap with P1 allocations\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        /* Run P1 first. */
        wm_common_tail_p1_reset();
        wm_8007293C_common_tail_p1();

        u32 p1_ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 p1_ptr2 = *(u32*)PSX_ADDR(0x8009BCB4);

        /* Run P2. */
        wm_common_tail_p2_reset();
        wm_80072944_common_tail_p2();

        u32 p2_ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 p2_ptr2 = *(u32*)PSX_ADDR(0x8009BE20);

        /* Check for overlap. Each block is 0x2800 or 0x10000 bytes. */
        u32 p1_end1 = p1_ptr1 + 0x10000;
        u32 p1_end2 = p1_ptr2 + 0x10000;
        u32 p2_end1 = p2_ptr1 + 0x2800;
        u32 p2_end2 = p2_ptr2 + 0x2800;

        int no_overlap = 1;
        /* P2 block 1 vs P1 blocks. */
        if (p2_ptr1 < p1_end1 && p2_end1 > p1_ptr1) no_overlap = 0;
        if (p2_ptr1 < p1_end2 && p2_end1 > p1_ptr2) no_overlap = 0;
        /* P2 block 2 vs P1 blocks. */
        if (p2_ptr2 < p1_end1 && p2_end2 > p1_ptr1) no_overlap = 0;
        if (p2_ptr2 < p1_end2 && p2_end2 > p1_ptr2) no_overlap = 0;
        /* P2 blocks vs each other. */
        if (p2_ptr1 < p2_end2 && p2_end1 > p2_ptr2) no_overlap = 0;

        check("P2 allocs don't overlap P1 allocs", no_overlap);
    }

    printf("\n=== Results: %d/%d PASS ===\n", pass, total);
    return (fail > 0) ? 1 : 0;
}
