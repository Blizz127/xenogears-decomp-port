/*
 * W34B5D — Production test for wm_800865A0 and common-tail P3 slice.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5d_800865a0_prod_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5d_800865a0_prod_test
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
static u32 s_heap_offset = 0u;
static int s_heap_alloc_count = 0;

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocFlags;
    s_heap_offset += allocSize;
    s_heap_offset = (s_heap_offset + 15u) & ~UINT32_C(15);
    void *p = &g_PsxRam[PSX_RAM_SIZE - s_heap_offset];
    memset(p, 0, allocSize);
    s_heap_alloc_count++;
    return p;
}

/* PsyQ stubs for GetTPage, GetClut, and SetSemiTrans. */
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
void SetSemiTrans(void *p, int abe)
{
    /* PS1 semantics: set or clear bit 1 of the code byte (offset +7). */
    u8 *base = (u8*)p;
    if (abe)
        base[7] |= 0x02;
    else
        base[7] = (u8)(base[7] & UINT8_C(0xFD));
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
    printf("=== W34B5D wm_800865A0 + P3 production test ===\n\n");

    /* ---- Test 1: wm_800865A0 basic allocation ---- */
    printf("Test 1: wm_800865A0 basic allocation\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_800865A0();

        check("2 HeapAlloc calls", s_heap_alloc_count == 2);

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7F8);
        check("D_8009D7F8 non-zero", ptr1 != 0);

        u32 ptr2 = *(u32*)PSX_ADDR(0x8009D7FC);
        check("D_8009D7FC non-zero", ptr2 != 0);

        check("ptr1 != ptr2", ptr1 != ptr2);
    }

    /* ---- Test 2: Record initialization pattern ---- */
    printf("\nTest 2: Record initialization pattern\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_800865A0();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7F8);
        u32 base = ptr1;  /* packet start; retail s0 is packet + 14 */

        /* Retail values. */
        u16 expected_tpage = GetTPage(0, 1, 960, 256);
        u16 expected_clut  = GetClut(304, 510);

        printf("  INFO: tpage=0x%04x clut=0x%04x\n", expected_tpage, expected_clut);

        /* Record 0. */
        check("record0 byte[3] == 9",      *(u8*)PSX_ADDR(base + 3) == 9);
        check("record0 byte[4] == 38",     *(u8*)PSX_ADDR(base + 4) == 38);
        check("record0 byte[5] == 38",     *(u8*)PSX_ADDR(base + 5) == 38);
        check("record0 byte[6] == 38",     *(u8*)PSX_ADDR(base + 6) == 38);
        check("record0 byte[7] == 0x2E",   *(u8*)PSX_ADDR(base + 7) == 0x2E);
        check("record0 hw[14] == clut",    *(u16*)PSX_ADDR(base + 14) == expected_clut);
        check("record0 hw[22] == tpage",   *(u16*)PSX_ADDR(base + 22) == expected_tpage);

        /* Last record (287). */
        u32 base_last = ptr1 + 287 * 40;
        check("record287 byte[3] == 9",    *(u8*)PSX_ADDR(base_last + 3) == 9);
        check("record287 byte[7] == 0x2E", *(u8*)PSX_ADDR(base_last + 7) == 0x2E);
        check("record287 hw[14] == clut",  *(u16*)PSX_ADDR(base_last + 14) == expected_clut);
        check("record287 hw[22] == tpage", *(u16*)PSX_ADDR(base_last + 22) == expected_tpage);

        /* Middle record (144). */
        u32 base_mid = ptr1 + 144 * 40;
        check("record144 byte[3] == 9",    *(u8*)PSX_ADDR(base_mid + 3) == 9);
        check("record144 byte[7] == 0x2E", *(u8*)PSX_ADDR(base_mid + 7) == 0x2E);
    }

    /* ---- Test 3: Record count ---- */
    printf("\nTest 3: Record count\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_800865A0();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7F8);
        int count = 0;
        for (u32 i = 0u; i < 288u; i++) {
            u32 b = ptr1 + i * 40u;
            if (*(u8*)PSX_ADDR(b + 3) == 9 && (*(u8*)PSX_ADDR(b + 7) & 0x02) != 0)
                count++;
        }
        check("288 records initialized", count == 288);
    }

    /* ---- Test 4: Copy verification ---- */
    printf("\nTest 4: Copy verification\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_800865A0();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7F8);
        u32 ptr2 = *(u32*)PSX_ADDR(0x8009D7FC);

        int mismatch = 0;
        for (u32 off = 0; off < 0x2D00; off += 4) {
            if (*(u32*)PSX_ADDR(ptr1 + off) != *(u32*)PSX_ADDR(ptr2 + off))
                mismatch++;
        }
        check("copy: zero mismatches in 11520 bytes", mismatch == 0);
    }

    /* ---- Test 5: Neighboring guard ---- */
    printf("\nTest 5: Neighboring guard\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_800865A0();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7F8);
        u32 base = ptr1;

        u16 expected_clut_guard = GetClut(304, 510);

        /* Retail's s0 anchor is packet+14, where the CLUT halfword lives.
         * Byte 2 remains the gap before the DMA length byte. */
        check("record0 hw[14] == clut", *(u16*)PSX_ADDR(base + 14) == expected_clut_guard);
        check("record0 byte[2] == 0 (gap before type marker)", *(u8*)PSX_ADDR(base + 2) == 0);

        /* Bytes 8..13 remain the gap between code and CLUT. */
        int gap_ok = 1;
        for (u32 j = 8u; j <= 13u; j++) {
            if (*(u8*)PSX_ADDR(base + j) != 0) { gap_ok = 0; break; }
        }
        check("record0 bytes[8..13] == 0 (gap before clut hw[14])", gap_ok);

        /* Bytes 16..21 remain the gap between CLUT and TPage. */
        int gap2_ok = 1;
        for (u32 j = 16u; j <= 21u; j++) {
            if (*(u8*)PSX_ADDR(base + j) != 0) { gap2_ok = 0; break; }
        }
        check("record0 bytes[16..21] == 0 (gap before tpage hw[22])", gap2_ok);
    }

    /* ---- Test 6: Prior state preserved ---- */
    printf("\nTest 6: Prior state preserved\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u32*)PSX_ADDR(0x8009BC3C) = 0x12345678;   /* P1 ptr1 */
        *(u32*)PSX_ADDR(0x8009BCB4) = 0x9ABCDEF0;   /* P1 ptr2 */
        *(u32*)PSX_ADDR(0x8009BE1C) = 0xDEADBEEF;   /* P2 ptr1 */
        *(u32*)PSX_ADDR(0x8009BE20) = 0xCAFEBABE;   /* P2 ptr2 */
        *(u32*)PSX_ADDR(0x8009C610) = 0;
        *(u32*)PSX_ADDR(0x8009C618) = 1024;
        *(u32*)PSX_ADDR(0x8009BCDC) = 256;

        s_heap_offset = 0;
        wm_800865A0();

        check("P1 ptr1 preserved", *(u32*)PSX_ADDR(0x8009BC3C) == 0x12345678);
        check("P1 ptr2 preserved", *(u32*)PSX_ADDR(0x8009BCB4) == 0x9ABCDEF0);
        check("P2 ptr1 preserved", *(u32*)PSX_ADDR(0x8009BE1C) == 0xDEADBEEF);
        check("P2 ptr2 preserved", *(u32*)PSX_ADDR(0x8009BE20) == 0xCAFEBABE);
        check("C610 preserved", *(u32*)PSX_ADDR(0x8009C610) == 0);
        check("C618 preserved", *(u32*)PSX_ADDR(0x8009C618) == 1024);
        check("BCDC preserved", *(u32*)PSX_ADDR(0x8009BCDC) == 256);
    }

    /* ---- Test 7: P3 caller slice ---- */
    printf("\nTest 7: P3 caller slice\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_common_tail_p3_reset();

        u32 cut = wm_8007294C_common_tail_p3();

        check("P3 entry == 1", wm_ctp3_get_entry() == 1);
        check("P3 865a0_calls == 1", wm_ctp3_get_865a0_calls() == 1);
        check("P3 alloc_calls == 2", wm_ctp3_get_alloc_calls() == 2);
        check("P3 cut == 0x80072954", cut == 0x80072954);
        check("P3 last_cut == 0x80072954", wm_ctp3_get_last_cut() == 0x80072954);
    }

    /* ---- Test 8: P3 repeated calls ---- */
    printf("\nTest 8: P3 repeated calls\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p3_reset();
        wm_8007294C_common_tail_p3();
        wm_8007294C_common_tail_p3();

        check("P3 entry == 2 after two calls", wm_ctp3_get_entry() == 2);

        wm_common_tail_p3_reset();
        check("P3 entry == 0 after reset", wm_ctp3_get_entry() == 0);
        check("P3 calls == 0 after reset", wm_ctp3_get_865a0_calls() == 0);
    }

    /* ---- Test 9: Full P0→P1→P2→P3 chain ---- */
    printf("\nTest 9: Full P0→P1→P2→P3 chain\n");
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

        wm_common_tail_p3_reset();
        u32 p3_cut = wm_8007294C_common_tail_p3();
        check("P3 cut == 0x80072954", p3_cut == 0x80072954);

        check("P0 entry still 1", wm_ctp0_get_entry() == 1);
        check("P1 entry still 1", wm_ctp1_get_entry() == 1);
        check("P2 entry still 1", wm_ctp2_get_entry() == 1);
        check("P3 entry still 1", wm_ctp3_get_entry() == 1);

        /* Verify P1 allocations still valid. */
        u32 p1_ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 p1_ptr2 = *(u32*)PSX_ADDR(0x8009BCB4);
        check("P1 ptr1 still valid", p1_ptr1 != 0);
        check("P1 ptr2 still valid", p1_ptr2 != 0);

        /* Verify P2 allocations valid and distinct from P1. */
        u32 p2_ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 p2_ptr2 = *(u32*)PSX_ADDR(0x8009BE20);
        check("P2 ptr1 still valid", p2_ptr1 != 0);
        check("P2 ptr2 still valid", p2_ptr2 != 0);
        check("P2 ptr1 != P1 ptr1", p2_ptr1 != p1_ptr1);
        check("P2 ptr1 != P1 ptr2", p2_ptr1 != p1_ptr2);
        check("P2 ptr2 != P1 ptr1", p2_ptr2 != p1_ptr1);
        check("P2 ptr2 != P1 ptr2", p2_ptr2 != p1_ptr2);
        check("P2 ptr1 != P2 ptr2", p2_ptr2 != p2_ptr1);

        /* Verify P3 allocations valid and distinct from P1/P2. */
        u32 p3_ptr1 = *(u32*)PSX_ADDR(0x8009D7F8);
        u32 p3_ptr2 = *(u32*)PSX_ADDR(0x8009D7FC);
        check("P3 ptr1 still valid", p3_ptr1 != 0);
        check("P3 ptr2 still valid", p3_ptr2 != 0);
        check("P3 ptr1 != P1 ptr1", p3_ptr1 != p1_ptr1);
        check("P3 ptr1 != P1 ptr2", p3_ptr1 != p1_ptr2);
        check("P3 ptr1 != P2 ptr1", p3_ptr1 != p2_ptr1);
        check("P3 ptr1 != P2 ptr2", p3_ptr1 != p2_ptr2);
        check("P3 ptr2 != P1 ptr1", p3_ptr2 != p1_ptr1);
        check("P3 ptr2 != P1 ptr2", p3_ptr2 != p1_ptr2);
        check("P3 ptr2 != P2 ptr1", p3_ptr2 != p2_ptr1);
        check("P3 ptr2 != P2 ptr2", p3_ptr2 != p2_ptr2);
        check("P3 ptr1 != P3 ptr2", p3_ptr2 != p3_ptr1);

        check("P0 D_80059179 == 1", D_80059179 == 1u);
    }

    /* ---- Test 10: No overlap with P1/P2 allocations ---- */
    printf("\nTest 10: No overlap with P1/P2 allocations\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p1_reset();
        wm_8007293C_common_tail_p1();
        u32 p1_ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 p1_ptr2 = *(u32*)PSX_ADDR(0x8009BCB4);

        wm_common_tail_p2_reset();
        wm_80072944_common_tail_p2();
        u32 p2_ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 p2_ptr2 = *(u32*)PSX_ADDR(0x8009BE20);

        wm_common_tail_p3_reset();
        wm_8007294C_common_tail_p3();
        u32 p3_ptr1 = *(u32*)PSX_ADDR(0x8009D7F8);
        u32 p3_ptr2 = *(u32*)PSX_ADDR(0x8009D7FC);

        u32 p1_end1 = p1_ptr1 + 0x10000;
        u32 p1_end2 = p1_ptr2 + 0x10000;
        u32 p2_end1 = p2_ptr1 + 0x2800;
        u32 p2_end2 = p2_ptr2 + 0x2800;
        u32 p3_end1 = p3_ptr1 + 0x2D00;
        u32 p3_end2 = p3_ptr2 + 0x2D00;

        int no_overlap = 1;
        /* P3 vs P1 */
        if (p3_ptr1 < p1_end1 && p3_end1 > p1_ptr1) no_overlap = 0;
        if (p3_ptr1 < p1_end2 && p3_end1 > p1_ptr2) no_overlap = 0;
        if (p3_ptr2 < p1_end1 && p3_end2 > p1_ptr1) no_overlap = 0;
        if (p3_ptr2 < p1_end2 && p3_end2 > p1_ptr2) no_overlap = 0;
        /* P3 vs P2 */
        if (p3_ptr1 < p2_end1 && p3_end1 > p2_ptr1) no_overlap = 0;
        if (p3_ptr1 < p2_end2 && p3_end1 > p2_ptr2) no_overlap = 0;
        if (p3_ptr2 < p2_end1 && p3_end2 > p2_ptr1) no_overlap = 0;
        if (p3_ptr2 < p2_end2 && p3_end2 > p2_ptr2) no_overlap = 0;
        /* P3 blocks vs each other */
        if (p3_ptr1 < p3_end2 && p3_end1 > p3_ptr2) no_overlap = 0;

        check("P3 allocs don't overlap P1/P2 allocs", no_overlap);
    }

    printf("\n=== Results: %d/%d PASS ===\n", pass, total);
    return (fail > 0) ? 1 : 0;
}
