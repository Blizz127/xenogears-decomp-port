/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#include <stdint.h>

extern uint8_t D_8004FE46;
extern void func_801D4318(void);
extern void ChangeGameState(unsigned int next);
extern void MainLoop(int reason);

void MovieMain(void)
{
    unsigned int next = D_8004FE46;
    func_801D4318();
    /* The legacy state table leaves slot four empty. Six re-enters this handler. */
    if (next == 4 || next >= 6)
        next = 1;
    ChangeGameState(next);
    MainLoop(0);
}
