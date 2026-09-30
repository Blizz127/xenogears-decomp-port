#include "common.h"
#include "system/controller.h"

int ControllerGetButtonState(int controllerIndex) {
    int nType;
    u_char* pController;

    pController = &g_C1Buffer[controllerIndex * CONTROLLER_BUFFER_SIZE];
    g_ControllerType = 0;
    if (pController[CONTROLLER_STATUS] == CONTROLLER_STATUS_SUCCESS) {
        g_ControllerType = pController[CONTROLLER_TYPE] & 0xF0;
        if (g_ControllerType == CONTROLLER_INTERNAL_TYPE_DIGITAL_PAD || 
            g_ControllerType == CONTROLLER_INTERNAL_TYPE_ANALOG_PAD  || 
            g_ControllerType == CONTROLLER_INTERNAL_TYPE_ANALOG_STICK
        ) {
            return (
                ~pController[CONTROLLER_BUTTONS_2] & 0xFF) | 
                ((pController[CONTROLLER_BUTTONS_1] << 8) ^ 0xFF00);
        }
    }
    return 0;
}

int ControllerGetType(int controllerIndex) {
    int nController;
    int nType;

    nController = controllerIndex * CONTROLLER_BUFFER_SIZE;
    if (g_C1Buffer[nController] != CONTROLLER_STATUS_ERROR) {
        nType = D_800625FD[nController] & 0xF0;
        switch (nType) {
            case CONTROLLER_INTERNAL_TYPE_DIGITAL_PAD:
                return CONTROLLER_TYPE_DIGITAL_PAD;
            case CONTROLLER_INTERNAL_TYPE_MOUSE:
                return CONTROLLER_TYPE_MOUSE;
            case CONTROLLER_INTERNAL_TYPE_ANALOG_PAD:
                return CONTROLLER_TYPE_ANALOG_PAD;
            case CONTROLLER_INTERNAL_TYPE_ANALOG_STICK:
                return CONTROLLER_TYPE_ANALOG_STICK;
            default:
                return CONTROLLER_TYPE_ERROR;
        }
    }
    
    return CONTROLLER_TYPE_NONE;
}

short ControllerRemapButtonState(short buttonState) {
    u_short nMaskedState;
    int i;

    nMaskedState = (u_short) buttonState & (
        CTRL_BTN_SELECT | CTRL_BTN_L3 | CTRL_BTN_R3 | CTRL_BTN_START |
        CTRL_BTN_UP | CTRL_BTN_RIGHT | CTRL_BTN_DOWN | CTRL_BTN_LEFT
    );
    
    for (i = 0; i < 8; i++) {
        if (g_ControllerButtonMasks[i] & (u_short) buttonState) {
            nMaskedState |= g_ControllerButtonMasks[g_ControllerButtonMappings[i]];
        }
    };
    
    return nMaskedState;
}

// Analog Stick Controller (0x5A53) has some buttons rearranged.
// This function remaps these buttons so we have a uniform state
short ControllerRemapAnalogJoystickState(short buttonState) {
    short nMaskedState;

    // Mask out buttons which are rearranged on analog joystick controllers
    nMaskedState = buttonState & ~(
        CTRL_BTN_R2 | CTRL_BTN_L1 | CTRL_BTN_R1 | CTRL_BTN_TRIANGLE | CTRL_BTN_SQUARE
    );

    // Remap them
    if (buttonState & CTRL_ANALOG_STICK_BTN_TRIANGLE) {
        nMaskedState |= nMaskedState | CTRL_BTN_TRIANGLE;
    }
    if (buttonState & CTRL_ANALOG_STICK_BTN_L1) {
        nMaskedState |= CTRL_BTN_L1;
    }
    if (buttonState & CTRL_ANALOG_STICK_BTN_R1) {
        nMaskedState |= CTRL_BTN_R1;
    }
    if (buttonState & CTRL_ANALOG_STICK_BTN_R2) {
        nMaskedState |= CTRL_BTN_R2;
    }
    if (buttonState & CTRL_ANALOG_STICK_BTN_SQUARE) {
        nMaskedState |= CTRL_BTN_SQUARE;
    }
    return nMaskedState;
}

u_char ControllerStickToAnalogX(int buttonState) {
    return g_ControllerStickToAnalogX[((buttonState >> 0xC) & 0xF)];
}

u_char ControllerStickToAnalogY(int buttonState) {
    return g_ControllerStickToAnalogY[((buttonState >> 0xC) & 0xF)];
}

void ControllerPoll(void) {
    // Controller 1
    g_C1ButtonState =  ControllerGetButtonState(0);
    g_C1ButtonState = ControllerRemapButtonState(g_C1ButtonState); // Masking buttons

    if (g_ControllerType) {
        if (g_ControllerType == CONTROLLER_INTERNAL_TYPE_ANALOG_PAD) {
            g_C1ButtonState = ControllerRemapAnalogJoystickState(g_C1ButtonState);
            goto controller1_analog_stick;
        } else if (g_ControllerType == CONTROLLER_INTERNAL_TYPE_ANALOG_STICK) {
            controller1_analog_stick:
            g_C1RightStickXAxis = D_80062600;
            g_C1RightStickYAxis = D_80062601;
            g_C1LeftStickXAxis = D_80062602;
            g_C1LeftStickYAxis = D_80062603;
        } else {
            g_C1RightStickYAxis = 0;
            g_C1RightStickXAxis = 0;
            g_C1LeftStickXAxis = g_ControllerStickToAnalogX[(u16) g_C1ButtonState >> 0xC];
            g_C1LeftStickYAxis = g_ControllerStickToAnalogY[(u16) g_C1ButtonState >> 0xC];
        }
    } else {
        g_C1RightStickYAxis = 0;
        g_C1RightStickXAxis = 0;
        g_C1LeftStickYAxis = 0;
        g_C1LeftStickXAxis = 0;        
    }

    g_C1ButtonStateReleased = g_C1ButtonState ^ g_C1PrevButtonState;
    g_C1ButtonStateReleased &= g_C1ButtonState;
    g_C1PrevButtonState = g_C1ButtonState;

    // Were any buttons released?
    if (g_C1ButtonStateReleased)
        D_8005022C = 0;
    
    g_C1ButtonStatePressedOnce = g_C1ButtonState;
    if (D_8005022C < 0x20) {
        D_8005022C += 1;
        g_C1ButtonStatePressedOnce = g_C1ButtonStateReleased;
    } else if (D_80059488 & 3) {
        g_C1ButtonStatePressedOnce = g_C1ButtonStateReleased;
    }


    // Controller 2
    g_C2ButtonState = ControllerGetButtonState(1);
    g_C2ButtonState = ControllerRemapButtonState(g_C2ButtonState);

    if (g_ControllerType) {
        if (g_ControllerType == CONTROLLER_INTERNAL_TYPE_ANALOG_PAD) {
            g_C2ButtonState = ControllerRemapAnalogJoystickState(g_C2ButtonState);
            goto controller2_analog_stick;
        } else if (g_ControllerType == CONTROLLER_INTERNAL_TYPE_ANALOG_STICK) {
            controller2_analog_stick:
            g_C2RightStickXAxis = D_80062622;
            g_C2RightStickYAxis = D_80062623;
            g_C2LeftStickXAxis = D_80062624;
            g_C2LeftStickYAxis = D_80062625;
        } else {
            g_C2RightStickYAxis = 0;
            g_C2RightStickXAxis = 0;
            g_C2LeftStickXAxis = g_ControllerStickToAnalogX[(u16) g_C2ButtonState >> 0xC];
            g_C2LeftStickYAxis = g_ControllerStickToAnalogY[(u16) g_C2ButtonState >> 0xC];
        }
    } else {
        g_C2RightStickYAxis = 0;
        g_C2RightStickXAxis = 0;
        g_C2LeftStickYAxis = 0;
        g_C2LeftStickXAxis = 0;   
    }
    
    g_C2ButtonStateReleased = g_C2ButtonState ^ g_C2PrevButtonState;
    g_C2ButtonStateReleased &= g_C2ButtonState;
    g_C2PrevButtonState = g_C2ButtonState;
    
    if (g_C2ButtonStateReleased)
        D_80050230 = 0;
    
    g_C2ButtonStatePressedOnce = g_C2ButtonState;
    if (D_80050230 < 0x20) {
        D_80050230 += 1;
        g_C2ButtonStatePressedOnce = g_C2ButtonStateReleased;
    } else if (D_80059488 & 3) {
        g_C2ButtonStatePressedOnce = g_C2ButtonStateReleased;
    }
}

void ControllerPushState(void) {
    int i;

    if (g_ControllerNumStates < CONTROLLER_MAX_NUM_STATES) {
        g_ControllerNumStates += 1;
        i = g_ControllerCurStateWriteIndex & (CONTROLLER_MAX_NUM_STATES - 1);
        g_C1ButtonStatesPressed[i] = g_C1ButtonState;
        g_C2ButtonStatesPressed[i] = g_C2ButtonState;
        g_C1ButtonStatesReleased[i] = g_C1ButtonStateReleased;
        g_C2ButtonStatesReleased[i] = g_C2ButtonStateReleased;
        g_C1ButtonStatesPressedOnce[i] = g_C1ButtonStatePressedOnce;
        g_C2ButtonStatesPressedOnce[i] = g_C2ButtonStatePressedOnce;
        /* Retail ADDIU wraps the 32-bit index without signed overflow. */
        g_ControllerCurStateWriteIndex = (int)((u_int)g_ControllerCurStateWriteIndex + 1u);
    } else {
        /* Retail 80035CC8: preserve queued states and report overflow.
         * Input scheduling must not be compensated by modifying the FIFO. */
        g_ControllerIsStateStackFull = 1;
    }
}

int ControllerPopState(void) {
    int i;

    if (g_ControllerNumStates == 0) {
        return 0;
    }
    
    i = g_ControllerCurStateReadIndex & (CONTROLLER_MAX_NUM_STATES - 1);
    g_ControllerCurStateReadIndex = (int)((u_int)g_ControllerCurStateReadIndex + 1u);
    g_C1ButtonState =  g_C1ButtonStatesPressed[i];
    g_C2ButtonState = g_C2ButtonStatesPressed[i];
    g_C1ButtonStateReleased = g_C1ButtonStatesReleased[i];
    g_C2ButtonStateReleased = g_C2ButtonStatesReleased[i];
    g_C1ButtonStatePressedOnce = g_C1ButtonStatesPressedOnce[i];
    g_C2ButtonStatePressedOnce = g_C2ButtonStatesPressedOnce[i];
    g_ControllerNumStates--;
    return g_ControllerNumStates + 1;
}

unsigned int ControllerGetNumStates() {
    return g_ControllerNumStates;
}

void ControllerResetState(void) {
    g_ControllerNumStates = 0;
    g_ControllerCurStateWriteIndex = 0;
    g_ControllerCurStateReadIndex = 0;
    g_ControllerIsStateStackFull = 0;
    D_80050200 = 1;
    D_800594EC = 0;
    D_800594E8 = 0;
    D_800594E0 = 0;
    D_800594DC = 0;
    D_800595CC = 0;
    D_800595C8 = 0;
    g_C2ButtonStatePressedOnce = 0;
    g_C1ButtonStatePressedOnce = 0;
    g_C2ButtonStateReleased = 0;
    g_C1ButtonStateReleased = 0;
    g_C2ButtonState = 0;
    g_C1ButtonState = 0;
}

/* func_80036410, func_8003611C and ControllerInit: one body for both builds
 * in system/controller2.c. */
