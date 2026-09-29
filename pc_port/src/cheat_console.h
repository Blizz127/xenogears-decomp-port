#ifndef XENO_PC_PORT_CHEAT_CONSOLE_H
#define XENO_PC_PORT_CHEAT_CONSOLE_H

/* Cheat console (cheat_console.c), on the xg_plat frame_tick hook. */
void PcPort_CheatConsoleInit(void);
/* Queue a command from any thread; it runs on the next presented frame. */
void PcPort_CheatQueue(const char* line);
/* Run a command now (game thread only). 1 ok, 0 empty, -1 unknown. */
int PcPort_CheatExec(const char* line);

#endif
