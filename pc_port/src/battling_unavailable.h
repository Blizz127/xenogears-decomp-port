#ifndef XENO_PC_PORT_BATTLING_UNAVAILABLE_H
#define XENO_PC_PORT_BATTLING_UNAVAILABLE_H

/* TEMPORARY STOPGAP until the Battling overlay (game state 4) is ported
 * natively; see battling_unavailable.c. */

#define PC_PORT_GAME_STATE_FIELD    1u
#define PC_PORT_GAME_STATE_BATTLING 4u

extern const char *const PcPort_BattlingUnavailableNotice;
extern unsigned int PcPort_BattlingUnavailableCount;

/* Called by MainLoop before it touches g_MainGameStates[state].  Returns 1
 * when it redirected an unbound state 4 back to the field (g_CurGameState is
 * then 1), 0 when the state is left alone. */
int PcPort_BattlingUnavailableGuard(unsigned int state, int entryBound);

#endif
