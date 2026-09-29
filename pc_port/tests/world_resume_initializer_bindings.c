/* Exercise the production resolver and its ABI adapters, not a test registry. */
#include <assert.h>
#include "../src/world_map_scheduler.c"
s32 test_world_resume_dispatch(u32 address, s32 slot)
{
    wm_sched_callback_fn callback = NULL;
    assert(wm_sched_resolve(address, &callback) == WM_SCHED_CB_IMPLEMENTED);
    assert(callback != NULL);
    return callback(slot);
}
