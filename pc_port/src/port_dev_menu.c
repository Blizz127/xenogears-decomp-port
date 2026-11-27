/* Public builds leave keyboard and controller input with the game. */
#include "port_dev_menu.h"

#include <stdio.h>

void PcPort_DevMenuInit(void)
{
    fputs("[DEV_MENU] disabled: F8 and Select+Start do nothing\n", stderr);
}

int PcPort_DevMenuEnabled(void)
{
    return 0;
}

int PcPort_DevMenuOpen(void)
{
    return 0;
}

int PcPort_DevMenuInputState(unsigned *ms)
{
    if (ms)
        *ms = 0;
    return 0;
}

int PcPort_DevMenuKey(int key, int pressed)
{
    (void)key;
    (void)pressed;
    return 0;
}

unsigned PcPort_DevMenuPad(unsigned bits, int instance, int held_any, unsigned now_ms)
{
    (void)bits;
    (void)instance;
    (void)held_any;
    (void)now_ms;
    return PC_DEV_PASS;
}

void PcPort_DevMenuSetHost(const PcPortDevMenuHost *host)
{
    (void)host;
}

void PcPort_DevMenuRegisterEnhancement(const char *label, int (*get)(void), void (*toggle)(void))
{
    (void)label;
    (void)get;
    (void)toggle;
}

void PcPort_DevMenuRegisterEnhancementValue(const char *label, int (*get)(void),
    void (*set)(int), int min, int max, int step, const char *unit)
{
    (void)label;
    (void)get;
    (void)set;
    (void)min;
    (void)max;
    (void)step;
    (void)unit;
}

int PcPort_DevMenuEnhancementCount(void)
{
    return 0;
}

int PcPort_DevMenuRows(void)
{
    return 0;
}

int PcPort_DevMenuCursor(void)
{
    return 0;
}

const char *PcPort_DevMenuTitle(void)
{
    return "";
}

const char *PcPort_DevMenuRow(int row)
{
    (void)row;
    return "";
}

int PcPort_DevMenuRowSelectable(int row)
{
    (void)row;
    return 0;
}

int PcPort_DevMenuRowAdjustable(int row)
{
    (void)row;
    return 0;
}

void PcPort_DevMenuGetLayout(int width, int height, PcPortDevMenuLayout *layout)
{
    (void)width;
    (void)height;
    if (layout)
        *layout = (PcPortDevMenuLayout){0};
}

const char *PcPort_DevMenuStatus(void)
{
    return "";
}

void PcPort_DevMenuSetStatus(const char *text)
{
    (void)text;
}

void PcPort_DevMenuQuickSaveResult(const char *status)
{
    (void)status;
}

int PcPort_DevMenuCheatsActive(void)
{
    return 0;
}
