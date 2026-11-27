#include "common.h"
#include "system/menu.h"

void MenuInitializeGfxEnvironment(GfxEnvironment *environment)
{
    DRAWENV *draw = &environment->drawEnv;
    DISPENV *display = &environment->dispEnv;

    draw->dtd = 1;
    draw->isbg = 0;
    draw->r0 = 0;
    draw->g0 = 0;
    draw->b0 = 0;

    display->screen.x = 0;
    display->screen.y = 10;
    display->screen.w = 256;
    display->screen.h = 216;
}

void MenuInitializeGfxEnvironments(void)
{
    enum { width = 320, height = 224 };

    SetGeomOffset(width / 2, height / 2);
    SetGeomScreen(512);

    SetDefDispEnv(&g_Menu->gfxEnvs[0].dispEnv, 0, height, width, height);
    SetDefDrawEnv(&g_Menu->gfxEnvs[0].drawEnv, 0, 0, width, height);
    SetDefDispEnv(&g_Menu->gfxEnvs[1].dispEnv, 0, 0, width, height);
    SetDefDrawEnv(&g_Menu->gfxEnvs[1].drawEnv, 0, height, width, height);

    MenuInitializeGfxEnvironment(&g_Menu->gfxEnvs[0]);
    MenuInitializeGfxEnvironment(&g_Menu->gfxEnvs[1]);
}
