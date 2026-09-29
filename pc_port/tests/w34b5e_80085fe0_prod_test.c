/*
 * W34B5E — Production test for wm_80085FE0 and common-tail P4 slice.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5e_80085fe0_prod_test.c \
 *     pc_port/src/world_map_common_tail.c \
 *     -o pc_port/build_native/w34b5e_80085fe0_prod_test
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_common_tail.h"

/* Provide g_PsxRam for the production module. */
uint8_t g_PsxRam[PSX_RAM_SIZE];
/* Native main-executable gate referenced by the linked P0 slice. */
u8 D_80059179;

/* HeapAlloc stub: allocates from top of g_PsxRam to produce valid KUSEG pointers. */
static int s_heap_offset = 0;
static int s_heap_alloc_count = 0;

void* HeapAlloc(u_int allocSize, u_int allocFlags)
{
    (void)allocFlags;
    s_heap_offset += allocSize;
    s_heap_offset = (s_heap_offset + 15) & ~15;
    void *p = &g_PsxRam[PSX_RAM_SIZE - s_heap_offset];
    memset(p, 0, allocSize);
    s_heap_alloc_count++;
    return p;
}

/* PsyQ stubs for GetTPage and GetClut. */
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
    printf("=== W34B5E wm_80085FE0 + P4 production test ===\n\n");

    /* ---- Test 1: wm_80085FE0 basic allocation ---- */
    printf("Test 1: wm_80085FE0 basic allocation\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_80085FE0();

        check("2 HeapAlloc calls", s_heap_alloc_count == 2);

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7E8);
        check("D_8009D7E8 non-zero", ptr1 != 0);

        u32 ptr2 = *(u32*)PSX_ADDR(0x8009D7EC);
        check("D_8009D7EC non-zero", ptr2 != 0);

        check("ptr1 != ptr2", ptr1 != ptr2);
    }

    /* ---- Test 2: Record initialization pattern ---- */
    printf("\nTest 2: Record initialization pattern\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_80085FE0();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7E8);
        u32 base = ptr1 + 22;  /* record base = ptr1 + 22 */

        /* Retail values. */
        /* Retail 80086090..800860A0: a0=0,a1=0,a2=896,a3=256. */
        u16 expected_tpage = 0x001E;
        u16 expected_clut  = GetClut(240, 511);

        printf("  INFO: tpage=0x%04x clut=0x%04x\n", expected_tpage, expected_clut);

        /* Record 0. */
        check("record0 byte[-19] == 9",    *(u8*)PSX_ADDR(base - 19) == 9);
        check("record0 byte[-18] == 128",  *(u8*)PSX_ADDR(base - 18) == 128);
        check("record0 byte[-17] == 128",  *(u8*)PSX_ADDR(base - 17) == 128);
        check("record0 byte[-16] == 128",  *(u8*)PSX_ADDR(base - 16) == 128);
        check("record0 byte[-15] == 44",   *(u8*)PSX_ADDR(base - 15) == 44);
        check("record0 byte[-10] == 0",    *(u8*)PSX_ADDR(base - 10) == 0);
        check("record0 byte[-9] == 64",    *(u8*)PSX_ADDR(base - 9) == 64);
        check("record0 byte[-2] == 31",    *(u8*)PSX_ADDR(base - 2) == 31);
        check("record0 byte[-1] == 64",    *(u8*)PSX_ADDR(base - 1) == 64);
        check("record0 byte[6] == 0",      *(u8*)PSX_ADDR(base + 6) == 0);
        check("record0 byte[7] == 111",    *(u8*)PSX_ADDR(base + 7) == 111);
        check("record0 byte[14] == 31",    *(u8*)PSX_ADDR(base + 14) == 31);
        check("record0 byte[15] == 111",   *(u8*)PSX_ADDR(base + 15) == 111);
        check("record0 hw[-8] == clut",    *(u16*)PSX_ADDR(base - 8) == expected_clut);
        check("record0 hw[0] == tpage",    *(u16*)PSX_ADDR(base + 0) == expected_tpage);

        /* Last record (511). */
        u32 base_last = ptr1 + 22 + 511 * 40;
        check("record511 byte[-19] == 9",  *(u8*)PSX_ADDR(base_last - 19) == 9);
        check("record511 byte[-15] == 44", *(u8*)PSX_ADDR(base_last - 15) == 44);
        check("record511 hw[-8] == clut",  *(u16*)PSX_ADDR(base_last - 8) == expected_clut);
        check("record511 hw[0] == tpage",  *(u16*)PSX_ADDR(base_last + 0) == expected_tpage);

        /* Middle record (256). */
        u32 base_mid = ptr1 + 22 + 256 * 40;
        check("record256 byte[-19] == 9",  *(u8*)PSX_ADDR(base_mid - 19) == 9);
        check("record256 byte[-15] == 44", *(u8*)PSX_ADDR(base_mid - 15) == 44);
        check("record256 hw[-8] == clut",  *(u16*)PSX_ADDR(base_mid - 8) == expected_clut);
        check("record256 hw[0] == tpage",  *(u16*)PSX_ADDR(base_mid + 0) == expected_tpage);
    }

    /* ---- Test 3: Record count ---- */
    printf("\nTest 3: Record count\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_80085FE0();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7E8);
        int count = 0;
        for (int i = 0; i < 512; i++) {
            u32 b = ptr1 + 22 + i * 40;
            if (*(u8*)PSX_ADDR(b - 19) == 9)
                count++;
        }
        check("512 records initialized", count == 512);
        {
            int pages_ok = 1;
            u32 ptr2 = *(u32*)PSX_ADDR(0x8009D7EC);
            for (int i = 0; i < 512; i++) {
                if (*(u16*)PSX_ADDR(ptr1 + i * 40 + 22) != 0x001E ||
                    *(u16*)PSX_ADDR(ptr2 + i * 40 + 22) != 0x001E)
                    pages_ok = 0;
            }
            check("all 1024 buffered records use retail page 0x001E", pages_ok);
        }
    }

    /* ---- Test 4: Copy verification ---- */
    printf("\nTest 4: Copy verification\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_80085FE0();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7E8);
        u32 ptr2 = *(u32*)PSX_ADDR(0x8009D7EC);

        int mismatch = 0;
        for (u32 off = 0; off < 0x5000; off += 4) {
            if (*(u32*)PSX_ADDR(ptr1 + off) != *(u32*)PSX_ADDR(ptr2 + off))
                mismatch++;
        }
        check("copy: zero mismatches in 20480 bytes", mismatch == 0);
    }

    /* ---- Test 5: Neighboring guard ---- */
    printf("\nTest 5: Neighboring guard\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_80085FE0();

        u32 ptr1 = *(u32*)PSX_ADDR(0x8009D7E8);
        u32 base = ptr1 + 22;

        /* Record layout: 40 bytes per record. base = ptr1 + 22 + N*40.
         * base points at byte[22] of the record, so base-22 = byte[0].
         *
         * Written fields:
         *   byte[3..7]   = base[-19..-15]  ← tag, RGB, code
         *   byte[8..9]   = base[-14..-13]  ← (gap, but byte[8] has code effect)
         *   byte[10..11] = base[-12..-11]  ← gap
         *   byte[12]     = base[-10]       ← zero write
         *   byte[13]     = base[-9]        ← 64
         *   byte[14..15] = base[-8..-7]    ← clut halfword
         *   byte[16..19] = base[-6..-3]    ← gap (4 bytes between clut and byte[20])
         *   byte[20]     = base[-2]        ← 31
         *   byte[21]     = base[-1]        ← 64
         *   byte[22..23] = base[0..1]      ← tpage halfword
         *   byte[24..27] = base[2..5]      ← gap
         *   byte[28]     = base[6]         ← zero write
         *   byte[29]     = base[7]         ← 111
         *   byte[30..35] = base[8..13]     ← gap
         *   byte[36]     = base[14]        ← 31
         *   byte[37]     = base[15]        ← 111
         *   byte[38..39] = base[16..17]    ← gap
         *
         * Gap bytes (should be zero from HeapAlloc zero-fill): */

        /* Gap: bytes 0..2 (before tag). */
        check("record0 byte[-22] == 0 (gap)", *(u8*)PSX_ADDR(base - 22) == 0);
        check("record0 byte[-21] == 0 (gap)", *(u8*)PSX_ADDR(base - 21) == 0);
        check("record0 byte[-20] == 0 (gap)", *(u8*)PSX_ADDR(base - 20) == 0);

        /* Gap: bytes 10..11 (between code byte and clut). */
        check("record0 byte[-12] == 0 (gap)", *(u8*)PSX_ADDR(base - 12) == 0);
        check("record0 byte[-11] == 0 (gap)", *(u8*)PSX_ADDR(base - 11) == 0);

        /* Gap: bytes 16..19 (between clut and byte[20] write). */
        int gap_ok = 1;
        for (int j = -6; j <= -3; j++) {
            if (*(u8*)PSX_ADDR(base + j) != 0) { gap_ok = 0; break; }
        }
        check("record0 bytes[-6..-3] == 0 (gap clut-to-byte20)", gap_ok);

        /* Gap: bytes 24..27 (between tpage and byte[28] write). */
        int gap2_ok = 1;
        for (int j = 2; j <= 5; j++) {
            if (*(u8*)PSX_ADDR(base + j) != 0) { gap2_ok = 0; break; }
        }
        check("record0 bytes[2..5] == 0 (gap tpage-to-byte28)", gap2_ok);

        /* Gap: bytes 30..35 (between byte[29] and byte[36] writes). */
        int gap3_ok = 1;
        for (int j = 8; j <= 13; j++) {
            if (*(u8*)PSX_ADDR(base + j) != 0) { gap3_ok = 0; break; }
        }
        check("record0 bytes[8..13] == 0 (gap byte29-to-byte36)", gap3_ok);
    }

    /* ---- Test 6: Prior state preserved ---- */
    printf("\nTest 6: Prior state preserved\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        *(u32*)PSX_ADDR(0x8009BC3C) = 0x12345678;   /* P1 ptr1 */
        *(u32*)PSX_ADDR(0x8009BCB4) = 0x9ABCDEF0;   /* P1 ptr2 */
        *(u32*)PSX_ADDR(0x8009BE1C) = 0xDEADBEEF;   /* P2 ptr1 */
        *(u32*)PSX_ADDR(0x8009BE20) = 0xCAFEBABE;   /* P2 ptr2 */
        *(u32*)PSX_ADDR(0x8009D7F8) = 0x11111111;   /* P3 ptr1 */
        *(u32*)PSX_ADDR(0x8009D7FC) = 0x22222222;   /* P3 ptr2 */
        *(u32*)PSX_ADDR(0x8009C610) = 0;
        *(u32*)PSX_ADDR(0x8009C618) = 1024;
        *(u32*)PSX_ADDR(0x8009BCDC) = 256;

        s_heap_offset = 0;
        wm_80085FE0();

        check("P1 ptr1 preserved", *(u32*)PSX_ADDR(0x8009BC3C) == 0x12345678);
        check("P1 ptr2 preserved", *(u32*)PSX_ADDR(0x8009BCB4) == 0x9ABCDEF0);
        check("P2 ptr1 preserved", *(u32*)PSX_ADDR(0x8009BE1C) == 0xDEADBEEF);
        check("P2 ptr2 preserved", *(u32*)PSX_ADDR(0x8009BE20) == 0xCAFEBABE);
        check("P3 ptr1 preserved", *(u32*)PSX_ADDR(0x8009D7F8) == 0x11111111);
        check("P3 ptr2 preserved", *(u32*)PSX_ADDR(0x8009D7FC) == 0x22222222);
        check("C610 preserved", *(u32*)PSX_ADDR(0x8009C610) == 0);
        check("C618 preserved", *(u32*)PSX_ADDR(0x8009C618) == 1024);
        check("BCDC preserved", *(u32*)PSX_ADDR(0x8009BCDC) == 256);
    }

    /* ---- Test 7: P4 caller slice ---- */
    printf("\nTest 7: P4 caller slice\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;
        s_heap_alloc_count = 0;

        wm_common_tail_p4_reset();

        u32 cut = wm_80072954_common_tail_p4();

        check("P4 entry == 1", wm_ctp4_get_entry() == 1);
        check("P4 85fe0_calls == 1", wm_ctp4_get_85fe0_calls() == 1);
        check("P4 alloc_calls == 2", wm_ctp4_get_alloc_calls() == 2);
        check("P4 cut == 0x8007295C", cut == 0x8007295C);
        check("P4 last_cut == 0x8007295C", wm_ctp4_get_last_cut() == 0x8007295C);
    }

    /* ---- Test 8: P4 repeated calls ---- */
    printf("\nTest 8: P4 repeated calls\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p4_reset();
        wm_80072954_common_tail_p4();
        wm_80072954_common_tail_p4();

        check("P4 entry == 2 after two calls", wm_ctp4_get_entry() == 2);

        wm_common_tail_p4_reset();
        check("P4 entry == 0 after reset", wm_ctp4_get_entry() == 0);
        check("P4 calls == 0 after reset", wm_ctp4_get_85fe0_calls() == 0);
    }

    /* ---- Test 9: Full P0→P1→P2→P3→P4 chain ---- */
    printf("\nTest 9: Full P0→P1→P2→P3→P4 chain\n");
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

        wm_common_tail_p4_reset();
        u32 p4_cut = wm_80072954_common_tail_p4();
        check("P4 cut == 0x8007295C", p4_cut == 0x8007295C);

        check("P0 entry still 1", wm_ctp0_get_entry() == 1);
        check("P1 entry still 1", wm_ctp1_get_entry() == 1);
        check("P2 entry still 1", wm_ctp2_get_entry() == 1);
        check("P3 entry still 1", wm_ctp3_get_entry() == 1);
        check("P4 entry still 1", wm_ctp4_get_entry() == 1);

        /* Verify all allocations still valid. */
        u32 p1_ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 p1_ptr2 = *(u32*)PSX_ADDR(0x8009BCB4);
        u32 p2_ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 p2_ptr2 = *(u32*)PSX_ADDR(0x8009BE20);
        u32 p3_ptr1 = *(u32*)PSX_ADDR(0x8009D7F8);
        u32 p3_ptr2 = *(u32*)PSX_ADDR(0x8009D7FC);
        u32 p4_ptr1 = *(u32*)PSX_ADDR(0x8009D7E8);
        u32 p4_ptr2 = *(u32*)PSX_ADDR(0x8009D7EC);

        check("P1 ptr1 still valid", p1_ptr1 != 0);
        check("P1 ptr2 still valid", p1_ptr2 != 0);
        check("P2 ptr1 still valid", p2_ptr1 != 0);
        check("P2 ptr2 still valid", p2_ptr2 != 0);
        check("P3 ptr1 still valid", p3_ptr1 != 0);
        check("P3 ptr2 still valid", p3_ptr2 != 0);
        check("P4 ptr1 still valid", p4_ptr1 != 0);
        check("P4 ptr2 still valid", p4_ptr2 != 0);

        /* All 8 allocations must be distinct. */
        check("P4 ptr1 != P4 ptr2", p4_ptr1 != p4_ptr2);
        check("P4 ptr1 != P1 ptr1", p4_ptr1 != p1_ptr1);
        check("P4 ptr1 != P1 ptr2", p4_ptr1 != p1_ptr2);
        check("P4 ptr1 != P2 ptr1", p4_ptr1 != p2_ptr1);
        check("P4 ptr1 != P2 ptr2", p4_ptr1 != p2_ptr2);
        check("P4 ptr1 != P3 ptr1", p4_ptr1 != p3_ptr1);
        check("P4 ptr1 != P3 ptr2", p4_ptr1 != p3_ptr2);
        check("P4 ptr2 != P1 ptr1", p4_ptr2 != p1_ptr1);
        check("P4 ptr2 != P1 ptr2", p4_ptr2 != p1_ptr2);
        check("P4 ptr2 != P2 ptr1", p4_ptr2 != p2_ptr1);
        check("P4 ptr2 != P2 ptr2", p4_ptr2 != p2_ptr2);
        check("P4 ptr2 != P3 ptr1", p4_ptr2 != p3_ptr1);
        check("P4 ptr2 != P3 ptr2", p4_ptr2 != p3_ptr2);

        check("P0 native D_80059179 == 1", D_80059179 == 1);
        check("P0 leaves guest twin untouched", *(u8*)PSX_ADDR(0x80059179) == 0);
    }

    /* ---- Test 10: No overlap with P1/P2/P3 allocations ---- */
    printf("\nTest 10: No overlap with P1/P2/P3 allocations\n");
    {
        memset(g_PsxRam, 0, PSX_RAM_SIZE);
        s_heap_offset = 0;

        wm_common_tail_p1_reset();
        wm_8007293C_common_tail_p1();

        wm_common_tail_p2_reset();
        wm_80072944_common_tail_p2();

        wm_common_tail_p3_reset();
        wm_8007294C_common_tail_p3();

        wm_common_tail_p4_reset();
        wm_80072954_common_tail_p4();

        u32 p1_ptr1 = *(u32*)PSX_ADDR(0x8009BC3C);
        u32 p1_ptr2 = *(u32*)PSX_ADDR(0x8009BCB4);
        u32 p2_ptr1 = *(u32*)PSX_ADDR(0x8009BE1C);
        u32 p2_ptr2 = *(u32*)PSX_ADDR(0x8009BE20);
        u32 p3_ptr1 = *(u32*)PSX_ADDR(0x8009D7F8);
        u32 p3_ptr2 = *(u32*)PSX_ADDR(0x8009D7FC);
        u32 p4_ptr1 = *(u32*)PSX_ADDR(0x8009D7E8);
        u32 p4_ptr2 = *(u32*)PSX_ADDR(0x8009D7EC);

        u32 p1_end1 = p1_ptr1 + 0x10000;
        u32 p1_end2 = p1_ptr2 + 0x10000;
        u32 p2_end1 = p2_ptr1 + 0x2800;
        u32 p2_end2 = p2_ptr2 + 0x2800;
        u32 p3_end1 = p3_ptr1 + 0x2D00;
        u32 p3_end2 = p3_ptr2 + 0x2D00;
        u32 p4_end1 = p4_ptr1 + 0x5000;
        u32 p4_end2 = p4_ptr2 + 0x5000;

        int no_overlap = 1;
        /* P4 vs P1 */
        if (p4_ptr1 < p1_end1 && p4_end1 > p1_ptr1) no_overlap = 0;
        if (p4_ptr1 < p1_end2 && p4_end1 > p1_ptr2) no_overlap = 0;
        if (p4_ptr2 < p1_end1 && p4_end2 > p1_ptr1) no_overlap = 0;
        if (p4_ptr2 < p1_end2 && p4_end2 > p1_ptr2) no_overlap = 0;
        /* P4 vs P2 */
        if (p4_ptr1 < p2_end1 && p4_end1 > p2_ptr1) no_overlap = 0;
        if (p4_ptr1 < p2_end2 && p4_end1 > p2_ptr2) no_overlap = 0;
        if (p4_ptr2 < p2_end1 && p4_end2 > p2_ptr1) no_overlap = 0;
        if (p4_ptr2 < p2_end2 && p4_end2 > p2_ptr2) no_overlap = 0;
        /* P4 vs P3 */
        if (p4_ptr1 < p3_end1 && p4_end1 > p3_ptr1) no_overlap = 0;
        if (p4_ptr1 < p3_end2 && p4_end1 > p3_ptr2) no_overlap = 0;
        if (p4_ptr2 < p3_end1 && p4_end2 > p3_ptr1) no_overlap = 0;
        if (p4_ptr2 < p3_end2 && p4_end2 > p3_ptr2) no_overlap = 0;
        /* P4 blocks vs each other */
        if (p4_ptr1 < p4_end2 && p4_end1 > p4_ptr2) no_overlap = 0;

        check("P4 allocs don't overlap P1/P2/P3 allocs", no_overlap);
    }

    /* ---- Test 11: TPage/CLUT exact values ---- */
    printf("\nTest 11: TPage/CLUT exact values\n");
    {
        u16 tpage = GetTPage(0, 0, 896, 256);
        u16 clut  = GetClut(240, 511);

        /* Retail GetTPage(0, 0, 896, 256): 0x10 | 14 = 0x001E.
         *
         * GetClut(240, 511):
         *   = (511<<6) | ((240>>4)&0x3F)
         *   = 0x7FC0 | 0x0F = 0x7FCF */
        check("GetTPage(0,0,896,256) == 0x001E", tpage == 0x001E);
        check("GetClut(240,511) == 0x7FCF",      clut == 0x7FCF);
    }

    printf("\n=== Results: %d/%d PASS ===\n", pass, total);
    return (fail > 0) ? 1 : 0;
}
