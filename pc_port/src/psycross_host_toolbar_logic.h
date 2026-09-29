#ifndef XENO_PC_PORT_HOST_TOOLBAR_LOGIC_H
#define XENO_PC_PORT_HOST_TOOLBAR_LOGIC_H

#define PC_PORT_HOST_TOOLBAR_HEIGHT 34

typedef enum PcPortHostToolbarAction {
    PC_PORT_TOOLBAR_NONE = 0,
    PC_PORT_TOOLBAR_QUICK_SAVE,
    PC_PORT_TOOLBAR_QUICK_LOAD,
    PC_PORT_TOOLBAR_RECORD,
    PC_PORT_TOOLBAR_SPEED,
    PC_PORT_TOOLBAR_FEI_HD2D,
    PC_PORT_TOOLBAR_GOD_MODE,
    PC_PORT_TOOLBAR_RANDOM_BATTLES
} PcPortHostToolbarAction;

static inline PcPortHostToolbarAction PcPort_HostToolbarHitTest(int x, int y)
{
    if (y < 4 || y >= 30)
        return PC_PORT_TOOLBAR_NONE;
    if (x >= 8 && x < 88)
        return PC_PORT_TOOLBAR_QUICK_SAVE;
    if (x >= 96 && x < 176)
        return PC_PORT_TOOLBAR_QUICK_LOAD;
    if (x >= 184 && x < 280)
        return PC_PORT_TOOLBAR_RECORD;
    if (x >= 288 && x < 392)
        return PC_PORT_TOOLBAR_SPEED;
    if (x >= 400 && x < 468)
        return PC_PORT_TOOLBAR_FEI_HD2D;
    if (x >= 472 && x < 570)
        return PC_PORT_TOOLBAR_RANDOM_BATTLES;
    if (x >= 578 && x < 656)
        return PC_PORT_TOOLBAR_GOD_MODE;
    return PC_PORT_TOOLBAR_NONE;
}

#endif
