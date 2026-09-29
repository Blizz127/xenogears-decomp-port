/* data_controller.c -- the BIOS pad buffers g_C1Buffer / g_C2Buffer as one
 * contiguous host blob.
 *
 * Retail BSS (asm/slus_006.64/data/49AC0.bss.s):
 *
 *   0x800625FC  g_C1Buffer    (label size 0x01; real extent 0x22)
 *   0x800625FD  D_800625FD    0x03   \
 *   0x80062600  D_80062600    0x01    |  splat-split labels that are
 *   0x80062601  D_80062601    0x01    |  interior bytes of controller 1's
 *   0x80062602  D_80062602    0x01    |  0x22-byte buffer
 *   0x80062603  D_80062603    0x1B   /
 *   0x8006261E  g_C2Buffer    (label size 0x04; real extent 0x22)
 *   0x80062622  D_80062622    0x01   \
 *   0x80062623  D_80062623    0x01    |  interior bytes of controller 2's
 *   0x80062624  D_80062624    0x01    |  buffer
 *   0x80062625  D_80062625    0x1B   /
 *
 * CONTROLLER_BUFFER_SIZE is 0x22 (include/system/controller.h) and the game
 * indexes ONE array across both slots: controller.c does
 * `&g_C1Buffer[controllerIndex * CONTROLLER_BUFFER_SIZE]`, so slot 1 is
 * g_C1Buffer + 0x22 -- which is exactly where retail placed g_C2Buffer.
 *
 * The auto-generated stubs sized g_C1Buffer at 0x20 (the label is 0x01 and the
 * generator's minimum reservation is 32 bytes) and gave g_C2Buffer separate
 * storage. Two consequences, both real bugs rather than cosmetic ones:
 *
 *   - port_main.c registers slot 1 at &g_C1Buffer[0x22], i.e. past the end of
 *     a 0x20-byte object, so PsyX_Pad_InitPad's writes to pad->id/buttons[]
 *     landed in whatever global followed (ASan: global-buffer-overflow at
 *     PsyX_pad.cpp:133, 3 bytes after g_C1Buffer, 29 before g_C1ButtonState).
 *   - the game's own `controllerIndex * 0x22` indexing read slot 1 from that
 *     same out-of-bounds region instead of from g_C2Buffer.
 *
 * One 0x44 blob with every symbol as a .set alias at its retail offset
 * reproduces retail's aliasing exactly (same idiom as data_game_state.c /
 * data_main_menu.c). */

unsigned char g_C1Buffer[0x44] __attribute__((aligned(8)));

__asm__(
    ".globl D_800625FD\n.set D_800625FD, g_C1Buffer + 0x01\n"
    ".globl D_80062600\n.set D_80062600, g_C1Buffer + 0x04\n"
    ".globl D_80062601\n.set D_80062601, g_C1Buffer + 0x05\n"
    ".globl D_80062602\n.set D_80062602, g_C1Buffer + 0x06\n"
    ".globl D_80062603\n.set D_80062603, g_C1Buffer + 0x07\n"
    ".globl g_C2Buffer\n.set g_C2Buffer, g_C1Buffer + 0x22\n"
    ".globl D_80062622\n.set D_80062622, g_C1Buffer + 0x26\n"
    ".globl D_80062623\n.set D_80062623, g_C1Buffer + 0x27\n"
    ".globl D_80062624\n.set D_80062624, g_C1Buffer + 0x28\n"
    ".globl D_80062625\n.set D_80062625, g_C1Buffer + 0x29\n"
);
