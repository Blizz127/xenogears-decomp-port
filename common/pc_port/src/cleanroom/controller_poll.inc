struct PollAxes {
    unsigned char *rightX;
    unsigned char *rightY;
    unsigned char *leftX;
    unsigned char *leftY;
};

struct PollPort {
    unsigned short *held;
    unsigned short *rising;
    unsigned short *repeat;
    int *previous;
    int *delay;
    struct PollAxes output;
    struct PollAxes packet;
};

static short PollSignedButtons(unsigned short buttons)
{
    int value = (int)buttons;
    return (short)(value < 32768 ? value : value - 65536);
}

static void PollSetAxes(const struct PollAxes *out, unsigned char rightX,
                        unsigned char rightY, unsigned char leftX,
                        unsigned char leftY)
{
    *out->rightX = rightX;
    *out->rightY = rightY;
    *out->leftX = leftX;
    *out->leftY = leftY;
}

void ControllerPoll(void)
{
    static const struct PollPort ports[2] = {
        {
            &g_C1ButtonState, &g_C1ButtonStateReleased, &g_C1ButtonStatePressedOnce,
            &g_C1PrevButtonState, &D_8005022C,
            { &g_C1RightStickXAxis, &g_C1RightStickYAxis, &g_C1LeftStickXAxis, &g_C1LeftStickYAxis },
            { &D_80062600, &D_80062601, &D_80062602, &D_80062603 }
        },
        {
            &g_C2ButtonState, &g_C2ButtonStateReleased, &g_C2ButtonStatePressedOnce,
            &g_C2PrevButtonState, &D_80050230,
            { &g_C2RightStickXAxis, &g_C2RightStickYAxis, &g_C2LeftStickXAxis, &g_C2LeftStickYAxis },
            { &D_80062622, &D_80062623, &D_80062624, &D_80062625 }
        }
    };
    int index;

    for (index = 0; index < 2; ++index) {
        const struct PollPort *port = &ports[index];
        unsigned char type;
        short mapped;
        unsigned short rising;

        *port->held = (unsigned short)ControllerGetButtonState(index);
        mapped = ControllerRemapButtonState(PollSignedButtons(*port->held));
        type = g_ControllerType;
        *port->held = (unsigned short)mapped;

        if (type == 0x50 || type == 0x70) {
            if (type == 0x50) {
                *port->held = (unsigned short)ControllerRemapAnalogJoystickState(mapped);
            }
            PollSetAxes(&port->output, *port->packet.rightX, *port->packet.rightY,
                        *port->packet.leftX, *port->packet.leftY);
        } else if (type != 0) {
            unsigned int direction = (unsigned int)*port->held >> 12;
            PollSetAxes(&port->output, 0, 0, g_ControllerStickToAnalogX[direction],
                        g_ControllerStickToAnalogY[direction]);
        } else {
            PollSetAxes(&port->output, 0, 0, 0, 0);
        }

        rising = (unsigned short)(*port->held & ~(unsigned int)*port->previous);
        *port->rising = rising;
        *port->previous = (int)*port->held;
        if (rising != 0) {
            *port->delay = 0;
        }

        if (*port->delay < 32) {
            ++*port->delay;
            *port->repeat = rising;
        } else {
            *port->repeat = ((unsigned int)D_80059488 & 3u) == 0 ? *port->held : rising;
        }
    }
}
