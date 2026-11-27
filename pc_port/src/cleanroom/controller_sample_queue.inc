/* The queue publishes complete samples between the controller poll and game. */
typedef struct {
    u_short held1, held2, released1, released2, repeated1, repeated2;
} ControllerQueueSample;

#define QUEUE_FIELD(field) (*(volatile __typeof__(field) *)&(field))

void ControllerPushState(void)
{
    u_int count = QUEUE_FIELD(g_ControllerNumStates);
    if (count >= CONTROLLER_MAX_NUM_STATES) {
        QUEUE_FIELD(g_ControllerIsStateStackFull) = 1;
        return;
    }
    u_int next = (u_int)QUEUE_FIELD(g_ControllerCurStateWriteIndex);
    u_int slot = next & (CONTROLLER_MAX_NUM_STATES - 1u);
    ControllerQueueSample sample;
    sample.held1 = QUEUE_FIELD(g_C1ButtonState);
    sample.held2 = QUEUE_FIELD(g_C2ButtonState);
    sample.released1 = QUEUE_FIELD(g_C1ButtonStateReleased);
    sample.released2 = QUEUE_FIELD(g_C2ButtonStateReleased);
    sample.repeated1 = QUEUE_FIELD(g_C1ButtonStatePressedOnce);
    sample.repeated2 = QUEUE_FIELD(g_C2ButtonStatePressedOnce);
    QUEUE_FIELD(g_ControllerNumStates) = count + 1u;
    QUEUE_FIELD(g_C1ButtonStatesPressed[slot]) = sample.held1;
    QUEUE_FIELD(g_C2ButtonStatesPressed[slot]) = sample.held2;
    QUEUE_FIELD(g_C1ButtonStatesReleased[slot]) = sample.released1;
    QUEUE_FIELD(g_C2ButtonStatesReleased[slot]) = sample.released2;
    QUEUE_FIELD(g_C1ButtonStatesPressedOnce[slot]) = sample.repeated1;
    QUEUE_FIELD(g_C2ButtonStatesPressedOnce[slot]) = sample.repeated2;
    QUEUE_FIELD(g_ControllerCurStateWriteIndex) = (int)(next + 1u);
}

int ControllerPopState(void)
{
    u_int count = QUEUE_FIELD(g_ControllerNumStates);
    if (count == 0) return 0;
    u_int next = (u_int)QUEUE_FIELD(g_ControllerCurStateReadIndex);
    u_int slot = next & (CONTROLLER_MAX_NUM_STATES - 1u);
    QUEUE_FIELD(g_ControllerNumStates) = count - 1u;
    ControllerQueueSample sample;
    sample.held1 = QUEUE_FIELD(g_C1ButtonStatesPressed[slot]);
    sample.held2 = QUEUE_FIELD(g_C2ButtonStatesPressed[slot]);
    sample.released1 = QUEUE_FIELD(g_C1ButtonStatesReleased[slot]);
    sample.released2 = QUEUE_FIELD(g_C2ButtonStatesReleased[slot]);
    sample.repeated1 = QUEUE_FIELD(g_C1ButtonStatesPressedOnce[slot]);
    sample.repeated2 = QUEUE_FIELD(g_C2ButtonStatesPressedOnce[slot]);
    QUEUE_FIELD(g_ControllerCurStateReadIndex) = (int)(next + 1u);
    QUEUE_FIELD(g_C1ButtonState) = sample.held1;
    QUEUE_FIELD(g_C2ButtonState) = sample.held2;
    QUEUE_FIELD(g_C1ButtonStateReleased) = sample.released1;
    QUEUE_FIELD(g_C2ButtonStateReleased) = sample.released2;
    QUEUE_FIELD(g_C1ButtonStatePressedOnce) = sample.repeated1;
    QUEUE_FIELD(g_C2ButtonStatePressedOnce) = sample.repeated2;
    return (int)count;
}

void ControllerResetState(void)
{
    QUEUE_FIELD(g_ControllerNumStates) = 0;
    QUEUE_FIELD(g_ControllerCurStateWriteIndex) = 0;
    QUEUE_FIELD(g_ControllerCurStateReadIndex) = 0;
    QUEUE_FIELD(g_ControllerIsStateStackFull) = 0;
    QUEUE_FIELD(D_80050200) = 1;
    QUEUE_FIELD(D_800594EC) = 0;
    QUEUE_FIELD(D_800594E8) = 0;
    QUEUE_FIELD(D_800594E0) = 0;
    QUEUE_FIELD(D_800594DC) = 0;
    QUEUE_FIELD(D_800595CC) = 0;
    QUEUE_FIELD(D_800595C8) = 0;
    QUEUE_FIELD(g_C2ButtonStatePressedOnce) = 0;
    QUEUE_FIELD(g_C1ButtonStatePressedOnce) = 0;
    QUEUE_FIELD(g_C2ButtonStateReleased) = 0;
    QUEUE_FIELD(g_C1ButtonStateReleased) = 0;
    QUEUE_FIELD(g_C2ButtonState) = 0;
    QUEUE_FIELD(g_C1ButtonState) = 0;
    if (PsyX_Pad_ClearDeferredSamples) {
        PsyX_Pad_ClearDeferredSamples();
    }
}

#undef QUEUE_FIELD
