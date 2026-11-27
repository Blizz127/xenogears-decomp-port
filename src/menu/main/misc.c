/* Native preview of the ordinary menu overlay, assembled from host sources. */
#ifdef XENO_PC_PORT
#include "preview/equipment/gear_parts.c"
#include "preview/equipment/gear_effects.c"
#include "preview/equipment/gear_weapon.c"
#include "preview/title/preview_title_menu.inc"

void func_801C62A8(void)
{
    if (D_80059460 == 2 || D_80059460 == 6) {
        PcPort_RunTitleMenu();
    } else {
        PcPort_QuickCheckpointNoticeSet("FIELD MENU UNAVAILABLE");
    }

    /* This overlay owns the common menu file and the MenuMain allocation.
     * The field opener releases its separate overlay and backup buffers. */
    if (D_8005945C && HeapFree(D_8005945C) == 0)
        D_8005945C = NULL;
    if (g_Menu && HeapFree(g_Menu) == 0)
        g_Menu = NULL;
}
#endif
