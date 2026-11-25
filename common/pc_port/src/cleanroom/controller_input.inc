int ControllerGetButtonState(int controllerIndex)
{
    const unsigned char *slot = &g_C1Buffer[34 * controllerIndex];

    g_ControllerType = 0;
    if (slot[0] != 0) {
        return 0;
    }

    g_ControllerType = slot[1] & 0xf0;
    switch (g_ControllerType) {
    case 0x40:
    case 0x50:
    case 0x70:
        /* The pad reports a pressed button as a cleared bit. */
        return ((slot[2] << 8) | slot[3]) ^ 0xffff;
    default:
        return 0;
    }
}

int ControllerGetType(int controllerIndex)
{
    const unsigned char *slot = &g_C1Buffer[34 * controllerIndex];

    if (slot[0] == 0xff) {
        return 0;
    }

    switch (slot[1] & 0xf0) {
    case 0x40:
        return 1;
    case 0x10:
        return 2;
    case 0x50:
        return 3;
    case 0x70:
        return 4;
    default:
        return -1;
    }
}

short ControllerRemapButtonState(short buttonState)
{
    unsigned short buttons = (unsigned short)buttonState;
    unsigned short remapped = buttons & 0xff00;
    int position;

    for (position = 0; position < 8; ++position) {
        if ((buttons & g_ControllerButtonMasks[position]) != 0) {
            remapped |= g_ControllerButtonMasks[g_ControllerButtonMappings[position]];
        }
    }

    return (short)remapped;
}
