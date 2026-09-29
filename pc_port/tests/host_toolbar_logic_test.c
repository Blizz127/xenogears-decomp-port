#include <assert.h>
#include <stdio.h>

#include "../src/psycross_host_toolbar_logic.h"

int main(void)
{
    assert(PcPort_HostToolbarHitTest(8, 4) == PC_PORT_TOOLBAR_QUICK_SAVE);
    assert(PcPort_HostToolbarHitTest(87, 29) == PC_PORT_TOOLBAR_QUICK_SAVE);
    assert(PcPort_HostToolbarHitTest(96, 4) == PC_PORT_TOOLBAR_QUICK_LOAD);
    assert(PcPort_HostToolbarHitTest(175, 29) == PC_PORT_TOOLBAR_QUICK_LOAD);
    assert(PcPort_HostToolbarHitTest(184, 4) == PC_PORT_TOOLBAR_RECORD);
    assert(PcPort_HostToolbarHitTest(279, 29) == PC_PORT_TOOLBAR_RECORD);

    assert(PcPort_HostToolbarHitTest(7, 4) == PC_PORT_TOOLBAR_NONE);
    assert(PcPort_HostToolbarHitTest(88, 4) == PC_PORT_TOOLBAR_NONE);
    assert(PcPort_HostToolbarHitTest(176, 4) == PC_PORT_TOOLBAR_NONE);
    assert(PcPort_HostToolbarHitTest(280, 4) == PC_PORT_TOOLBAR_NONE);
    assert(PcPort_HostToolbarHitTest(20, -1) == PC_PORT_TOOLBAR_NONE);
    assert(PcPort_HostToolbarHitTest(20, PC_PORT_HOST_TOOLBAR_HEIGHT) ==
           PC_PORT_TOOLBAR_NONE);

    assert(PcPort_HostToolbarHitTest(400, 4) == PC_PORT_TOOLBAR_FEI_HD2D);
    assert(PcPort_HostToolbarHitTest(555, 29) == PC_PORT_TOOLBAR_FEI_HD2D);
    assert(PcPort_HostToolbarHitTest(399, 4) == PC_PORT_TOOLBAR_NONE);
    assert(PcPort_HostToolbarHitTest(556, 4) == PC_PORT_TOOLBAR_NONE);

    assert(PcPort_HostToolbarHitTest(564, 4) == PC_PORT_TOOLBAR_GOD_MODE);
    assert(PcPort_HostToolbarHitTest(655, 29) == PC_PORT_TOOLBAR_GOD_MODE);
    assert(PcPort_HostToolbarHitTest(563, 4) == PC_PORT_TOOLBAR_NONE);
    assert(PcPort_HostToolbarHitTest(656, 4) == PC_PORT_TOOLBAR_NONE);
    assert(PcPort_HostToolbarHitTest(600, 30) == PC_PORT_TOOLBAR_NONE);
    puts("host toolbar hit-zone regression: PASS");
    return 0;
}
