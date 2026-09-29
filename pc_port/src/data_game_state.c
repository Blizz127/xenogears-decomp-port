/* data_game_state.c -- g_GameState and the field-transition tuple that
 * retail keeps IMMEDIATELY after it, as one contiguous host blob.
 *
 * Retail BSS (asm/slus_006.64/data/49AC0.bss.s, config/symbol_addrs):
 *
 *   0x8006D634  g_GameState      0x2300
 *   0x8006F934  D_8006F934       0x1A   (unreferenced pad)
 *   0x8006F94E  D_8006F94E       u16    field map selector
 *   0x8006F950  D_8006F950       u16    camera approach heading
 *   0x8006F952  D_8006F952       u16    transition arg 2
 *   0x8006F954  D_8006F954       0x3C   entrance (u16), D_8006F956 (u16),
 *                                        D_8006F958[16] (u16), ...
 *
 * The game addresses this tuple BOTH ways: through the standalone symbols
 * (FieldMain reads D_8006F94E/50/54/56 at entry, main.c:443-450; the
 * return-to-title path func_8001B6C4 writes them) and through g_GameState
 * offsets (CHANGE_FIELD writes g_pGameState+0x231A..0x2320, misc11.c; the
 * post-menu New Game fixup writes +0x231A/+0x2320, misc4.c:717-724; the
 * new-game template func_8001B970 memmove()s 0x2358 bytes over g_GameState,
 * which on retail lands +0x231A = 0x01EA in D_8006F94E -- the disc-1
 * template's first map, 490).
 *
 * Auto-generated stubs gave each D_ symbol its own storage, so the two views
 * disagreed: the template copy and every +0x231A store went into padding
 * while FieldMain read a separate, still-zero D_8006F94E (-> debug Map 0).
 * Following data_main_menu.c, every symbol is a .set alias at its retail
 * offset inside the one blob so the aliasing is exactly retail's.
 *
 * Size stays at the generator's 2x reservation (0x4600) so nothing else
 * changes; only the first 0x235C bytes carry retail meaning. */

unsigned char g_GameState[0x4600] __attribute__((aligned(8)));

__asm__(
    ".globl D_8006F934\n.set D_8006F934, g_GameState + 0x2300\n"
    ".globl D_8006F94E\n.set D_8006F94E, g_GameState + 0x231A\n"
    ".globl D_8006F950\n.set D_8006F950, g_GameState + 0x231C\n"
    ".globl D_8006F952\n.set D_8006F952, g_GameState + 0x231E\n"
    ".globl D_8006F954\n.set D_8006F954, g_GameState + 0x2320\n"
    ".globl D_8006F956\n.set D_8006F956, g_GameState + 0x2322\n"
    ".globl D_8006F958\n.set D_8006F958, g_GameState + 0x2324\n"
);
