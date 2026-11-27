/* Host implementation of the field-490 title picker. MenuMain owns entry and
 * func_800799D4 applies the result when the overlay returns. Disc archive
 * reads and the two video pages belong to preview_title_drawing.inc. */
#include <stdio.h>
#include "common.h"
#include "system/controller.h"
#include "system/menu.h"
#include "preview_title_drawing.inc"

extern int PcPort_QuickCheckpointPoll(void);
extern void PcPort_QuickCheckpointNoticeSet(const char*);
extern int ArchiveGetDiscNumber(void);
extern int ControllerPopState(void);
extern void ControllerResetState(void);
extern s32 func_80036410(void);
extern void func_8001B970(void);
extern u8 D_80059460;
extern u8 D_800594D0;

enum {
    PC_TITLE_BOTTOM_ROW = 0,
    PC_TITLE_CONTINUE = 1,
    PC_TITLE_NEW_GAME = 2
};

void func_801C7D78(void)
{
    g_Menu->input = MENU_INPUT_IDLE;
    if (func_80036410()) {
        ControllerResetState();
        return;
    }

    /* ControllerPopState supplies game-layout buttons. The FIFO's one-shot
     * face-button word is distinct from its repeating direction word. */
    while (ControllerPopState()) {
        const u16 direction = g_C1ButtonStatePressedOnce;
        const u16 action = g_C1ButtonStateReleased;

        if (direction & CTRL_BTN_RIGHT)
            g_Menu->input = MENU_INPUT_RIGHT;
        else if (direction & CTRL_BTN_DOWN)
            g_Menu->input = MENU_INPUT_DOWN;
        else if (direction & CTRL_BTN_LEFT)
            g_Menu->input = MENU_INPUT_LEFT;
        else if (direction & CTRL_BTN_UP)
            g_Menu->input = MENU_INPUT_UP;
        else if (action & CTRL_BTN_CIRCLE)
            g_Menu->input = MENU_INPUT_CONFIRM;
        else if (action & CTRL_BTN_CROSS)
            g_Menu->input = MENU_INPUT_BACK;
        else
            continue;
        break;
    }
}

void func_801C7BF4(void)
{
    PcPreviewTitleDrawFrame(g_Menu->menu1Choice);
    func_801C7D78();
    if (g_Menu->input == MENU_INPUT_IDLE)
        ++g_Menu->unk2D8;
    else
        g_Menu->unk2D8 = 0;
}

void func_801C58EC(void)
{
    const int disc = ArchiveGetDiscNumber();

    g_Menu->unk2D8 = 0;
    PcPreviewTitleDrawBegin();
    /* Discard the field's queued opener, preserving ControllerPoll's previous
     * held level. A held confirm cannot produce another one-shot. */
    ControllerResetState();
    printf("[xeno-port][menu] func_801C58EC title loop enter choice=%u\n",
           (unsigned)g_Menu->menu1Choice);
    fflush(stdout);

    for (;;) {
        func_801C7BF4();
        if (PcPort_QuickCheckpointPoll())
            break;
        if (disc == 1 && g_Menu->unk2D8 > 600u) {
            D_800594D0 = 1;
            break;
        }
        switch (g_Menu->input) {
        case MENU_INPUT_UP:
            g_Menu->menu1Choice = g_Menu->menu1Choice == PC_TITLE_NEW_GAME
                ? PC_TITLE_BOTTOM_ROW : g_Menu->menu1Choice + 1;
            break;
        case MENU_INPUT_DOWN:
            g_Menu->menu1Choice = g_Menu->menu1Choice == PC_TITLE_BOTTOM_ROW
                ? PC_TITLE_NEW_GAME : g_Menu->menu1Choice - 1;
            break;
        case MENU_INPUT_CONFIRM:
            if (g_Menu->menu1Choice == PC_TITLE_NEW_GAME && disc == 1) {
                func_8001B970();
                D_800594D0 = 0;
                printf("[xeno-port][menu] title confirm choice=2 keep=0 D_800594D0=0\n");
                fflush(stdout);
                goto finish;
            }
            if (g_Menu->menu1Choice == PC_TITLE_CONTINUE)
                PcPort_QuickCheckpointNoticeSet("MEMORY CARD LOAD UNAVAILABLE IN THIS PREVIEW");
            else if (g_Menu->menu1Choice == PC_TITLE_NEW_GAME)
                PcPort_QuickCheckpointNoticeSet("NEW GAME REQUIRES DISC 1");
            else
                PcPort_QuickCheckpointNoticeSet("TITLE OPTION UNAVAILABLE IN THIS PREVIEW");
            printf("[xeno-port][menu] title confirm choice=%u keep=1 D_800594D0=%u\n",
                   (unsigned)g_Menu->menu1Choice, (unsigned)D_800594D0);
            fflush(stdout);
            break;
        }
    }
finish:
    PcPreviewTitleDrawEnd();
}

void PcPort_RunTitleMenu(void)
{
    /* The field opener calls selector 6 separately, with D_80059171 = 1.
     * Only selector 2 may turn a zero return into a New Game field handoff. */
    if (D_80059460 == 6) {
        PcPreviewTitleReleaseArchive();
        PcPort_QuickCheckpointNoticeSet("MEMORY CARD MENU UNAVAILABLE IN THIS PREVIEW");
        return;
    }

    g_Menu->menu1Choice = PC_TITLE_CONTINUE;
    g_Menu->unk337 = 0xFF;
    func_801C58EC();
}
