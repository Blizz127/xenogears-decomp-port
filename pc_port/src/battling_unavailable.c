/*
 * TEMPORARY STOPGAP -- Battling (game state 4) is not ported yet.
 *
 * The native-port plan replaces this file: once the Battling overlay
 * (battling.bin, archive 0x0D, retail entry BattlingMain 0x80088E90) has a
 * native host entry bound in g_MainGameStates[4] (behind the
 * PC_PORT_BATTLING_ENTRY_READY gate, pc_port/tests/
 * battling_overlay_bootstrap_test.sh), this guard no longer fires and should
 * be deleted together with its MainLoop call.
 *
 * Without it, g_MainGameStates[4] is all-zero in the port
 * (PcPort_InitGameStates leaves it unbound), so MainLoop would call
 * HeapRelocate((void*)0 + 0x800) and write through a host pointer of 0x800
 * (SIGSEGV), and failing that would call a NULL pFnMain.
 *
 * Instead of entering the minigame we do what retail does when the player
 * leaves Battling, without playing a match:
 *   - battling setup func_80079B44 clears the result byte read back by the
 *     field opcode func_80087800 (D_80050622 = 0 @0x80079C04): no match was
 *     played, so the field sees the "no result" value, never a fabricated
 *     win/loss (0x81..0x87, func_80075060) or the fade-out exit 0x7F
 *     (func_80072170);
 *   - the battling exit func_800851D4 does ChangeGameState(1),
 *     D_8005061C = 1 and MainLoop(0): back to the field, which reloads the
 *     current map.
 * The battling-owned work in that exit (sound teardown func_8003852C/
 * func_80039C4C on battling's own handles, and func_80088A40 storing the
 * player's battling options to D_8006F980) is skipped: the overlay never
 * ran, so there is nothing to tear down and the stored options stay as they
 * were.  Nothing here touches the save game, the memory card or the
 * quick-checkpoint file.
 */
#include <stdio.h>

#include "battling_unavailable.h"
#include "quick_checkpoint.h"

extern void ChangeGameState(unsigned int state);
extern unsigned char D_8005061C[]; /* battling mode/flag byte (sdata) */
extern unsigned char D_80050622[]; /* battling result, read by func_80087800 */

const char *const PcPort_BattlingUnavailableNotice =
    "BATTLING ARENA NOT YET AVAILABLE IN THIS BUILD";

unsigned int PcPort_BattlingUnavailableCount;

int PcPort_BattlingUnavailableGuard(unsigned int state, int entryBound)
{
    if (state != PC_PORT_GAME_STATE_BATTLING || entryBound)
        return 0;

    PcPort_BattlingUnavailableCount++;
    fprintf(stderr,
            "[xeno-port][battling] state 4 (Battling minigame) is not ported "
            "in this build: skipped, result=0 (no match), returning to field "
            "(state 1) as retail func_800851D4 does [stopgap #%u]\n",
            PcPort_BattlingUnavailableCount);
    PcPort_QuickCheckpointNoticeSet(PcPort_BattlingUnavailableNotice);

    D_80050622[0] = 0;                        /* func_80079B44: no result */
    ChangeGameState(PC_PORT_GAME_STATE_FIELD); /* func_800851D4 */
    D_8005061C[0] = 1;                        /* func_800851D4 */
    return 1;
}
