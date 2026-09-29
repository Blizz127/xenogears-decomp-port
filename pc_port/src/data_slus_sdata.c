/*
 * data_slus_sdata.c - retail main-exe .sdata tables for the Xenogears PC port.
 *
 * No retail bytes live here: the load table at the end fills each object at
 * startup from the user's disc/SLUS_006.64 (retail_data.h),
 * (sha256 dc0b2dd786203d4cce5927c5a3fc85a18f39a3f7406078860076ebb0bbae7119)
 * at file offset 0x800 + vaddr - 0x80010000 (PS-X EXE: 0x800-byte header,
 * t_addr 0x80010000). Sizes and element kinds come from the splat listing
 * asm/slus_006.64/data/3F290.sdata.s.
 *
 * The port generator (tools/scripts/gen_port_stubs.py) emits a zero-filled
 * .bss stub for every symbol that is still undefined when it runs. Defining
 * the real symbol here removes it from that set, so native readers no longer
 * see an all-zero object. Pointer tables are loaded as host pointers into
 * g_PsxRam at the retail guest address (XENO_RD_PTRS); every other symbol
 * receives the exact retail byte stream so a raw memcmp against the image holds.
 *
 * Symbols intentionally NOT defined here:
 *   - D_8004FD40 (0x8004FD40, 16 function pointers): the live path uses the
 *     file-local table in game_overrides.c; temp1.c's extern read is dead.
 *     Defining a global function-pointer table here would only duplicate the
 *     known split-brain and cannot be checked byte-for-byte against retail.
 */

#include "psx_memory.h"

/* D_8004F0C0: PSX 0x8004F0C0 (sdata), 508 bytes at file offset 0x3F8C0. */
unsigned char D_8004F0C0[508] __attribute__((aligned(8)));

/* g_KernelMenuCurChoice: PSX 0x8004F2D8 (sdata), 28 bytes at file offset 0x3FAD8. */
unsigned char g_KernelMenuCurChoice[28] __attribute__((aligned(8)));

/* D_8004F304: PSX 0x8004F304 (sdata), 4 bytes at file offset 0x3FB04. */
unsigned char D_8004F304[4] __attribute__((aligned(8))) = {
    0x00,0x00,0x00,0x00,
};

/* D_8004F364: PSX 0x8004F364 (sdata), 4 bytes at file offset 0x3FB64. */
unsigned char D_8004F364[4] __attribute__((aligned(8)));

/* D_8004F384: PSX 0x8004F384 (sdata), 4 bytes at file offset 0x3FB84. */
unsigned char D_8004F384[4] __attribute__((aligned(8))) = {
    0x00,0x00,0x00,0x00,
};

/* D_8004FBB8: PSX 0x8004FBB8 (sdata), 32 bytes at file offset 0x403B8. */
unsigned char D_8004FBB8[32] __attribute__((aligned(8)));

/* D_8004FD80: PSX 0x8004FD80 (sdata), 32 bytes at file offset 0x40580. */
unsigned char D_8004FD80[32] __attribute__((aligned(8)));

/* D_8004FDA0: PSX 0x8004FDA0 (sdata), 32 bytes at file offset 0x405A0. */
unsigned char D_8004FDA0[32] __attribute__((aligned(8)));

/* D_8004FE44: PSX 0x8004FE44 (sdata), 1 bytes at file offset 0x40644. */
unsigned char D_8004FE44[1] __attribute__((aligned(8)));

/* D_8005010C: PSX 0x8005010C (sdata), 4 bytes at file offset 0x4090C. */
unsigned char D_8005010C[4] __attribute__((aligned(8)));

/* D_800501D0: PSX 0x800501D0 (sdata), 24 bytes at file offset 0x409D0. */
unsigned char D_800501D0[24] __attribute__((aligned(8)));

/* g_ControllerStickToAnalogX: PSX 0x8005020C (sdata), 16 bytes at file offset 0x40A0C. */
unsigned char g_ControllerStickToAnalogX[16] __attribute__((aligned(8)));

/* g_ControllerStickToAnalogY: PSX 0x8005021C (sdata), 16 bytes at file offset 0x40A1C. */
unsigned char g_ControllerStickToAnalogY[16] __attribute__((aligned(8)));

/* g_FontClutData: PSX 0x80050598 (sdata), 128 bytes at file offset 0x40D98. */
unsigned char g_FontClutData[128] __attribute__((aligned(8)));

/* D_8005061C: PSX 0x8005061C (sdata), 1 bytes at file offset 0x40E1C. */
unsigned char D_8005061C[1] __attribute__((aligned(8)));

/* D_8005061F: PSX 0x8005061F (sdata), 1 bytes at file offset 0x40E1F. */
unsigned char D_8005061F[1] __attribute__((aligned(8)));

/* D_80050620: PSX 0x80050620 (sdata), 1 bytes at file offset 0x40E20. */
unsigned char D_80050620[1] __attribute__((aligned(8)));

/* g_ReverbWorkAreaSizes: PSX 0x800508E8 (sdata), 40 bytes at file offset 0x410E8. */
unsigned char g_ReverbWorkAreaSizes[40] __attribute__((aligned(8)));

/* D_80050910: PSX 0x80050910 (sdata(SoundFile)), 32 bytes at file offset 0x41110. */
unsigned char D_80050910[32] __attribute__((aligned(8)));

/* D_80050924: PSX 0x80050924 (sdata), 28 bytes at file offset 0x41124. */
unsigned char D_80050924[28] __attribute__((aligned(8)));

/* D_80050940: PSX 0x80050940 (sdata(SoundWDSEntry)), 112 bytes at file offset 0x41140. */
unsigned char D_80050940[112] __attribute__((aligned(8)));

/* D_80059171: PSX 0x80059171 (sdata), 7 bytes at file offset 0x49971. */
unsigned char D_80059171[7] __attribute__((aligned(8)));

/* g_MenuDebugEnabled: PSX 0x80059178 (sdata), 4 bytes at file offset 0x49978. */
unsigned char g_MenuDebugEnabled[4] __attribute__((aligned(8)));

/* D_80059179: PSX 0x80059179 (sdata), 3 bytes at file offset 0x49979. */
unsigned char D_80059179[3] __attribute__((aligned(8))) = {
    0x00,0x00,0x00,
};

/* D_800591A8: PSX 0x800591A8 (sdata), 4 bytes at file offset 0x499A8. */
unsigned char D_800591A8[4] __attribute__((aligned(8)));

/* D_800591B8: PSX 0x800591B8 (sdata), 12 bytes at file offset 0x499B8. */
unsigned char D_800591B8[12] __attribute__((aligned(8)));

/* D_8004FA9C: PSX 0x8004FA9C (sdata), 7 guest pointers at file offset 0x4029C.
 * Retail values: 0x8004F39C, 0x8004F49C, 0x8004F59C, 0x8004F69C, 0x8004F79C, 0x8004F89C, 0x8004F99C. Stored as host pointers into g_PsxRam so native
 * readers (which dereference the table) get valid host addresses. */
void* D_8004FA9C[7] __attribute__((aligned(8)));

/* D_80050110: PSX 0x80050110 (sdata), 12 guest pointers at file offset 0x40910.
 * Retail values: 0x80059224, 0x8005921C, 0x80059214, 0x8005920C, 0x80059204, 0x800591FC, 0x800591F4, 0x800591EC, 0x800591E4, 0x800591DC, 0x800591D4, 0x800591CC. Stored as host pointers into g_PsxRam so native
 * readers (which dereference the table) get valid host addresses. */
void* D_80050110[12] __attribute__((aligned(8)));

/* g_HeapContentTypeNames: PSX 0x80050140 (sdata), 20 guest pointers at file offset 0x40940.
 * Retail values: 0x80059244, 0x80018A4C, 0x80018A40, 0x80018A34, 0x80018A28, 0x80018A1C, 0x80018A10, 0x80018A04, 0x800189F8, 0x800189EC, 0x8005923C, 0x800189E0, 0x800189D4, 0x800189C8, 0x800189BC, 0x80059234, 0x800189B0, 0x800189A4, 0x8005922C, 0x80018998. Stored as host pointers into g_PsxRam so native
 * readers (which dereference the table) get valid host addresses. */
void* g_HeapContentTypeNames[20] __attribute__((aligned(8)));

/* End of generated retail .sdata data. */

/* Retail data, loaded from the user's disc before main() (retail_data.h):
 * symbol, retail file, file offset (from the guest address), size. */
#include "retail_data.h"

_Static_assert(sizeof(D_8004F0C0) == 0x1FC, "D_8004F0C0 size");
_Static_assert(sizeof(g_KernelMenuCurChoice) == 0x1C, "g_KernelMenuCurChoice size");
_Static_assert(sizeof(D_8004F364) == 0x4, "D_8004F364 size");
_Static_assert(sizeof(D_8004FBB8) == 0x20, "D_8004FBB8 size");
_Static_assert(sizeof(D_8004FD80) == 0x20, "D_8004FD80 size");
_Static_assert(sizeof(D_8004FDA0) == 0x20, "D_8004FDA0 size");
_Static_assert(sizeof(D_8004FE44) == 0x1, "D_8004FE44 size");
_Static_assert(sizeof(D_8005010C) == 0x4, "D_8005010C size");
_Static_assert(sizeof(D_800501D0) == 0x18, "D_800501D0 size");
_Static_assert(sizeof(g_ControllerStickToAnalogX) == 0x10, "g_ControllerStickToAnalogX size");
_Static_assert(sizeof(g_ControllerStickToAnalogY) == 0x10, "g_ControllerStickToAnalogY size");
_Static_assert(sizeof(g_FontClutData) == 0x80, "g_FontClutData size");
_Static_assert(sizeof(D_8005061C) == 0x1, "D_8005061C size");
_Static_assert(sizeof(D_8005061F) == 0x1, "D_8005061F size");
_Static_assert(sizeof(D_80050620) == 0x1, "D_80050620 size");
_Static_assert(sizeof(g_ReverbWorkAreaSizes) == 0x28, "g_ReverbWorkAreaSizes size");
_Static_assert(sizeof(D_80050910) == 0x20, "D_80050910 size");
_Static_assert(sizeof(D_80050924) == 0x1C, "D_80050924 size");
_Static_assert(sizeof(D_80050940) == 0x70, "D_80050940 size");
_Static_assert(sizeof(D_80059171) == 0x7, "D_80059171 size");
_Static_assert(sizeof(g_MenuDebugEnabled) == 0x4, "g_MenuDebugEnabled size");
_Static_assert(sizeof(D_800591A8) == 0x4, "D_800591A8 size");
_Static_assert(sizeof(D_800591B8) == 0xC, "D_800591B8 size");

XENO_RETAIL_DATA_BEGIN(slus_sdata)
    XENO_RD(D_8004F0C0, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F0C0u), 0x1FC),
    XENO_RD(g_KernelMenuCurChoice, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F2D8u), 0x1C),
    XENO_RD(D_8004F364, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004F364u), 0x4),
    XENO_RD(D_8004FBB8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004FBB8u), 0x20),
    XENO_RD(D_8004FD80, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004FD80u), 0x20),
    XENO_RD(D_8004FDA0, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004FDA0u), 0x20),
    XENO_RD(D_8004FE44, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004FE44u), 0x1),
    XENO_RD(D_8005010C, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8005010Cu), 0x4),
    XENO_RD(D_800501D0, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800501D0u), 0x18),
    XENO_RD(g_ControllerStickToAnalogX, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8005020Cu), 0x10),
    XENO_RD(g_ControllerStickToAnalogY, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8005021Cu), 0x10),
    XENO_RD(g_FontClutData, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050598u), 0x80),
    XENO_RD(D_8005061C, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8005061Cu), 0x1),
    XENO_RD(D_8005061F, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8005061Fu), 0x1),
    XENO_RD(D_80050620, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050620u), 0x1),
    XENO_RD(g_ReverbWorkAreaSizes, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800508E8u), 0x28),
    XENO_RD(D_80050910, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050910u), 0x20),
    XENO_RD(D_80050924, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050924u), 0x1C),
    XENO_RD(D_80050940, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050940u), 0x70),
    XENO_RD(D_80059171, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80059171u), 0x7),
    XENO_RD(g_MenuDebugEnabled, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80059178u), 0x4),
    XENO_RD(D_800591A8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800591A8u), 0x4),
    XENO_RD(D_800591B8, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x800591B8u), 0xC),
    XENO_RD_PTRS(D_8004FA9C, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8004FA9Cu), 7),
    XENO_RD_PTRS(D_80050110, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050110u), 12),
    XENO_RD_PTRS(g_HeapContentTypeNames, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x80050140u), 20),
XENO_RETAIL_DATA_END(slus_sdata)
