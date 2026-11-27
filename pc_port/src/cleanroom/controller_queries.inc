unsigned int ControllerGetNumStates(void)
{
    return *(const volatile u_int *)&g_ControllerNumStates;
}

short ControllerRemapAnalogJoystickState(short buttons)
{
    unsigned int input = (u_short)buttons;
    unsigned int remapped = CTRL_ANALOG_STICK_BTN_L1 | CTRL_ANALOG_STICK_BTN_SQUARE |
                            CTRL_ANALOG_STICK_BTN_TRIANGLE | CTRL_ANALOG_STICK_BTN_R1 |
                            CTRL_ANALOG_STICK_BTN_R2;
    unsigned int output = input & ~remapped;
    output |= (input & (CTRL_ANALOG_STICK_BTN_L1 | CTRL_ANALOG_STICK_BTN_TRIANGLE)) << 1;
    output |= (input & CTRL_ANALOG_STICK_BTN_SQUARE) << 5;
    output |= (input & CTRL_ANALOG_STICK_BTN_R1) >> 1;
    output |= (input & CTRL_ANALOG_STICK_BTN_R2) >> 6;
    return (short)(u_short)output;
}

u_char ControllerStickToAnalogX(int buttons)
{
    unsigned int direction = ((unsigned int)buttons >> 12) & 15u;
    return ((const volatile u_char *)g_ControllerStickToAnalogX)[direction];
}

u_char ControllerStickToAnalogY(int buttons)
{
    unsigned int direction = ((unsigned int)buttons >> 12) & 15u;
    return ((const volatile u_char *)g_ControllerStickToAnalogY)[direction];
}
