/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_PC_PORT
#error "This kernel menu translation unit requires the native PC port."
#endif

#include <psx/libetc.h>
#include "system/kernel.h"

extern unsigned short g_C1ButtonStatePressedOnce;
extern unsigned short g_C1ButtonStateReleased;

/* The menu text is supplied through external game-data symbols. */
extern char D_800182B0, D_800182C0, D_800182E8, D_800182F0, D_800182F8;

void HeapChangeCurrentUser(unsigned int user, char **names);
void *FontLoadFont(int x, int y, int width, int height, int capacity,
                   unsigned int flags, int page_x, int page_y,
                   int clut_x, int clut_y, void *compressed);
void FontDrawLetters(void *ordering_table);
void FontPrintf(char *format, ...);
int Sprintf(char *destination, char *format, ...);
void ChangeGameState(unsigned int state);
_Noreturn void MainLoop(int state);

#include "../../../pc_port/src/cleanroom/kernel_menu_cursor.inc"
#include "../../../pc_port/src/cleanroom/kernel_menu_init.inc"
#include "../../../pc_port/src/cleanroom/kernel_menu_update.inc"
#include "../../../pc_port/src/cleanroom/kernel_menu_main.inc"
