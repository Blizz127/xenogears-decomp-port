/*
 * data_slus_rodata.c - retail main-exe .rodata tables for the Xenogears PC port.
 *
 * No retail bytes live here: each object is zero storage that the load table
 * at the end fills at startup from the user's disc/SLUS_006.64 (sha256
 * dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119) at file
 * offset 0x800 + vaddr - 0x80010000 (PS-X EXE: 0x800-byte header, t_addr
 * 0x80010000); see retail_data.h.  Sizes come from the splat listing
 * asm/slus_006.64/data/<name>.rodata.s (.size of each dlabel).
 *
 * The port generator (tools/scripts/gen_port_stubs.py) emits a zero-filled
 * .bss stub for every symbol that is still undefined when it runs. Defining
 * the real symbol here removes it from that set, so native readers no longer
 * see an all-zero object. After loading, every object holds the exact retail
 * byte stream, so a raw memcmp against the image holds.
 *
 * NOTE on the three big head objects: D_80010000 is the game's build/mode word
 * (0xFFFFFFFF); pc_port/src/port_main.c previously patched the zero stub to -1.
 * D_80010004 / D_80018004 are the archive index/header that PC-port ArchiveInit
 * re-reads from the disc (pDebugTable == 0 path); loading them is harmless because
 * the port still supplies the disc-backed table.
 */

#include "psx_memory.h"

/* D_80010000: PSX 0x80010000 (rodata), 4 bytes at file offset 0x800. */
unsigned char D_80010000[4] __attribute__((aligned(8)));

/* D_80010004: PSX 0x80010004 (rodata), 32768 bytes at file offset 0x804. */
unsigned char D_80010004[32768] __attribute__((aligned(8)));

/* D_80018004: PSX 0x80018004 (rodata), 128 bytes at file offset 0x8804. */
unsigned char D_80018004[128] __attribute__((aligned(8)));

/* D_800180FC: PSX 0x800180FC (rodata), 177 bytes at file offset 0x88FC. */
unsigned char D_800180FC[177] __attribute__((aligned(8)));

/* D_800181B8: PSX 0x800181B8 (rodata), 16 bytes at file offset 0x89B8. */
unsigned char D_800181B8[16] __attribute__((aligned(8)));

/* D_800181C8: PSX 0x800181C8 (rodata), 20 bytes at file offset 0x89C8. */
unsigned char D_800181C8[20] __attribute__((aligned(8)));

/* D_800181DC: PSX 0x800181DC (rodata), 12 bytes at file offset 0x89DC. */
unsigned char D_800181DC[12] __attribute__((aligned(8)));

/* D_800181E8: PSX 0x800181E8 (rodata), 10 bytes at file offset 0x89E8. */
unsigned char D_800181E8[10] __attribute__((aligned(8)));

/* D_800181F4: PSX 0x800181F4 (rodata), 10 bytes at file offset 0x89F4. */
unsigned char D_800181F4[10] __attribute__((aligned(8)));

/* D_80018200: PSX 0x80018200 (rodata), 27 bytes at file offset 0x8A00. */
unsigned char D_80018200[27] __attribute__((aligned(8)));

/* D_8001821C: PSX 0x8001821C (rodata), 2 bytes at file offset 0x8A1C. */
unsigned char D_8001821C[2] __attribute__((aligned(8)));

/* D_80018220: PSX 0x80018220 (rodata), 4 bytes at file offset 0x8A20. */
unsigned char D_80018220[4] __attribute__((aligned(8)));

/* D_80018224: PSX 0x80018224 (rodata), 19 bytes at file offset 0x8A24. */
unsigned char D_80018224[19] __attribute__((aligned(8)));

/* D_80018238: PSX 0x80018238 (rodata), 29 bytes at file offset 0x8A38. */
unsigned char D_80018238[29] __attribute__((aligned(8)));

/* D_80018258: PSX 0x80018258 (rodata), 82 bytes at file offset 0x8A58. */
unsigned char D_80018258[82] __attribute__((aligned(8)));

/* D_8001833C: PSX 0x8001833C (rodata), 20 bytes at file offset 0x8B3C. */
unsigned char D_8001833C[20] __attribute__((aligned(8)));

/* D_80018350: PSX 0x80018350 (rodata), 18 bytes at file offset 0x8B50. */
unsigned char D_80018350[18] __attribute__((aligned(8)));

/* D_80018364: PSX 0x80018364 (rodata), 19 bytes at file offset 0x8B64. */
unsigned char D_80018364[19] __attribute__((aligned(8)));

/* D_80018378: PSX 0x80018378 (rodata), 21 bytes at file offset 0x8B78. */
unsigned char D_80018378[21] __attribute__((aligned(8)));

/* D_80018390: PSX 0x80018390 (rodata), 19 bytes at file offset 0x8B90. */
unsigned char D_80018390[19] __attribute__((aligned(8)));

/* D_80018644: PSX 0x80018644 (rodata), 32 bytes at file offset 0x8E44. */
unsigned char D_80018644[32] __attribute__((aligned(8)));

/* D_800188EC: PSX 0x800188EC (rodata), 4 bytes at file offset 0x90EC. */
unsigned char D_800188EC[4] __attribute__((aligned(8)));

/* D_800188F0: PSX 0x800188F0 (rodata), 4 bytes at file offset 0x90F0. */
unsigned char D_800188F0[4] __attribute__((aligned(8)));

/* D_80018944: PSX 0x80018944 (rodata), 16 bytes at file offset 0x9144. */
unsigned char D_80018944[16] __attribute__((aligned(8)));

/* D_80018954: PSX 0x80018954 (rodata), 13 bytes at file offset 0x9154. */
unsigned char D_80018954[13] __attribute__((aligned(8)));

/* D_80018964: PSX 0x80018964 (rodata), 14 bytes at file offset 0x9164. */
unsigned char D_80018964[14] __attribute__((aligned(8)));

/* D_80018974: PSX 0x80018974 (rodata), 10 bytes at file offset 0x9174. */
unsigned char D_80018974[10] __attribute__((aligned(8)));

/* End of generated retail .rodata data. */

/* Retail data, loaded from the user's disc before main() (retail_data.h):
 * symbol, retail file, file offset (from the guest address), size. */
#include "retail_data.h"

_Static_assert(sizeof(D_80010000) == 0x4, "D_80010000 size");
_Static_assert(sizeof(D_80010004) == 0x8000, "D_80010004 size");
_Static_assert(sizeof(D_80018004) == 0x80, "D_80018004 size");
_Static_assert(sizeof(D_800180FC) == 0xB1, "D_800180FC size");
_Static_assert(sizeof(D_800181B8) == 0x10, "D_800181B8 size");
_Static_assert(sizeof(D_800181C8) == 0x14, "D_800181C8 size");
_Static_assert(sizeof(D_800181DC) == 0xC, "D_800181DC size");
_Static_assert(sizeof(D_800181E8) == 0xA, "D_800181E8 size");
_Static_assert(sizeof(D_800181F4) == 0xA, "D_800181F4 size");
_Static_assert(sizeof(D_80018200) == 0x1B, "D_80018200 size");
_Static_assert(sizeof(D_8001821C) == 0x2, "D_8001821C size");
_Static_assert(sizeof(D_80018220) == 0x4, "D_80018220 size");
_Static_assert(sizeof(D_80018224) == 0x13, "D_80018224 size");
_Static_assert(sizeof(D_80018238) == 0x1D, "D_80018238 size");
_Static_assert(sizeof(D_80018258) == 0x52, "D_80018258 size");
_Static_assert(sizeof(D_8001833C) == 0x14, "D_8001833C size");
_Static_assert(sizeof(D_80018350) == 0x12, "D_80018350 size");
_Static_assert(sizeof(D_80018364) == 0x13, "D_80018364 size");
_Static_assert(sizeof(D_80018378) == 0x15, "D_80018378 size");
_Static_assert(sizeof(D_80018390) == 0x13, "D_80018390 size");
_Static_assert(sizeof(D_80018644) == 0x20, "D_80018644 size");
_Static_assert(sizeof(D_800188EC) == 0x4, "D_800188EC size");
_Static_assert(sizeof(D_800188F0) == 0x4, "D_800188F0 size");
_Static_assert(sizeof(D_80018944) == 0x10, "D_80018944 size");
_Static_assert(sizeof(D_80018954) == 0xD, "D_80018954 size");
_Static_assert(sizeof(D_80018964) == 0xE, "D_80018964 size");
_Static_assert(sizeof(D_80018974) == 0xA, "D_80018974 size");

XENO_RETAIL_DATA_BEGIN(slus_rodata)
    XENO_RD(D_80010000, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80010000u), 0x4),
    XENO_RD(D_80010004, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80010004u), 0x8000),
    XENO_RD(D_80018004, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018004u), 0x80),
    XENO_RD(D_800180FC, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800180FCu), 0xB1),
    XENO_RD(D_800181B8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800181B8u), 0x10),
    XENO_RD(D_800181C8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800181C8u), 0x14),
    XENO_RD(D_800181DC, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800181DCu), 0xC),
    XENO_RD(D_800181E8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800181E8u), 0xA),
    XENO_RD(D_800181F4, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800181F4u), 0xA),
    XENO_RD(D_80018200, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018200u), 0x1B),
    XENO_RD(D_8001821C, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8001821Cu), 0x2),
    XENO_RD(D_80018220, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018220u), 0x4),
    XENO_RD(D_80018224, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018224u), 0x13),
    XENO_RD(D_80018238, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018238u), 0x1D),
    XENO_RD(D_80018258, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018258u), 0x52),
    XENO_RD(D_8001833C, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8001833Cu), 0x14),
    XENO_RD(D_80018350, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018350u), 0x12),
    XENO_RD(D_80018364, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018364u), 0x13),
    XENO_RD(D_80018378, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018378u), 0x15),
    XENO_RD(D_80018390, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018390u), 0x13),
    XENO_RD(D_80018644, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018644u), 0x20),
    XENO_RD(D_800188EC, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800188ECu), 0x4),
    XENO_RD(D_800188F0, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800188F0u), 0x4),
    XENO_RD(D_80018944, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018944u), 0x10),
    XENO_RD(D_80018954, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018954u), 0xD),
    XENO_RD(D_80018964, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018964u), 0xE),
    XENO_RD(D_80018974, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80018974u), 0xA),
XENO_RETAIL_DATA_END(slus_rodata)
