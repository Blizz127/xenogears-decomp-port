#include <assert.h>
#include <stdio.h>

#include "../src/quick_checkpoint_request.h"

int main(void)
{
    PcPortQuickRequestState state = {0};

    assert(PcPort_QuickRequestQueue(&state, PC_PORT_QUICK_REQUEST_SAVE, 1));
    assert(state.action == PC_PORT_QUICK_REQUEST_SAVE);
    assert(state.ui_state == PC_PORT_QUICK_UI_SAVE_PENDING);

    assert(PcPort_QuickRequestTakeIfSafe(&state, 0) ==
           PC_PORT_QUICK_REQUEST_NONE);
    assert(state.action == PC_PORT_QUICK_REQUEST_SAVE);
    assert(state.ui_state == PC_PORT_QUICK_UI_SAVE_PENDING);

    assert(PcPort_QuickRequestTakeIfSafe(&state, 1) ==
           PC_PORT_QUICK_REQUEST_SAVE);
    assert(state.action == PC_PORT_QUICK_REQUEST_NONE);
    PcPort_QuickRequestSetResult(&state, PC_PORT_QUICK_UI_SAVE_OK);
    assert(state.ui_state == PC_PORT_QUICK_UI_SAVE_OK);

    assert(PcPort_QuickRequestQueue(&state, PC_PORT_QUICK_REQUEST_LOAD, 1));
    assert(state.action == PC_PORT_QUICK_REQUEST_LOAD);
    assert(state.ui_state == PC_PORT_QUICK_UI_LOAD_PENDING);
    PcPort_QuickRequestCancel(&state);
    assert(state.action == PC_PORT_QUICK_REQUEST_NONE);
    assert(state.ui_state == PC_PORT_QUICK_UI_LOAD_ERROR);

    assert(!PcPort_QuickRequestQueue(&state, PC_PORT_QUICK_REQUEST_SAVE, 0));
    assert(state.action == PC_PORT_QUICK_REQUEST_NONE);
    assert(state.ui_state == PC_PORT_QUICK_UI_SAVE_ERROR);

    puts("quick checkpoint request regression: PASS");
    return 0;
}
