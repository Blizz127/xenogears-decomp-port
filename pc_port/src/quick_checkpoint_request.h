#ifndef XENO_PC_PORT_QUICK_CHECKPOINT_REQUEST_H
#define XENO_PC_PORT_QUICK_CHECKPOINT_REQUEST_H

typedef enum PcPortQuickRequestAction {
    PC_PORT_QUICK_REQUEST_NONE = 0,
    PC_PORT_QUICK_REQUEST_SAVE,
    PC_PORT_QUICK_REQUEST_LOAD
} PcPortQuickRequestAction;

typedef enum PcPortQuickUiState {
    PC_PORT_QUICK_UI_IDLE = 0,
    PC_PORT_QUICK_UI_SAVE_PENDING,
    PC_PORT_QUICK_UI_LOAD_PENDING,
    PC_PORT_QUICK_UI_SAVE_OK,
    PC_PORT_QUICK_UI_LOAD_OK,
    PC_PORT_QUICK_UI_SAVE_ERROR,
    PC_PORT_QUICK_UI_LOAD_ERROR
} PcPortQuickUiState;

typedef struct PcPortQuickRequestState {
    PcPortQuickRequestAction action;
    PcPortQuickUiState ui_state;
    int wait_reported;
} PcPortQuickRequestState;

static inline int PcPort_QuickRequestQueue(PcPortQuickRequestState* state,
                                           PcPortQuickRequestAction action,
                                           int field_active)
{
    if (!field_active) {
        state->action = PC_PORT_QUICK_REQUEST_NONE;
        state->ui_state = action == PC_PORT_QUICK_REQUEST_LOAD
            ? PC_PORT_QUICK_UI_LOAD_ERROR : PC_PORT_QUICK_UI_SAVE_ERROR;
        state->wait_reported = 0;
        return 0;
    }
    state->action = action;
    state->ui_state = action == PC_PORT_QUICK_REQUEST_LOAD
        ? PC_PORT_QUICK_UI_LOAD_PENDING : PC_PORT_QUICK_UI_SAVE_PENDING;
    state->wait_reported = 0;
    return 1;
}

static inline PcPortQuickRequestAction PcPort_QuickRequestTakeIfSafe(
    PcPortQuickRequestState* state, int safe)
{
    PcPortQuickRequestAction action;
    if (!safe || state->action == PC_PORT_QUICK_REQUEST_NONE)
        return PC_PORT_QUICK_REQUEST_NONE;
    action = state->action;
    state->action = PC_PORT_QUICK_REQUEST_NONE;
    state->wait_reported = 0;
    return action;
}

static inline void PcPort_QuickRequestSetResult(PcPortQuickRequestState* state,
                                                PcPortQuickUiState result)
{
    state->ui_state = result;
}

static inline void PcPort_QuickRequestCancel(PcPortQuickRequestState* state)
{
    if (state->action == PC_PORT_QUICK_REQUEST_LOAD)
        state->ui_state = PC_PORT_QUICK_UI_LOAD_ERROR;
    else if (state->action == PC_PORT_QUICK_REQUEST_SAVE)
        state->ui_state = PC_PORT_QUICK_UI_SAVE_ERROR;
    state->action = PC_PORT_QUICK_REQUEST_NONE;
    state->wait_reported = 0;
}

#endif
