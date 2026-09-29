/* Retail world-map restore-entry helper 0x80073398. */
#ifndef WORLD_MAP_HELPER_73398_H
#define WORLD_MAP_HELPER_73398_H

#include "common.h"

void wm_80073398(void);

#if defined(WM_73398_TEST_HOOKS)
enum Wm73398TestEvent {
    WM_73398_TEST_READ = 1,
    WM_73398_TEST_WRITE = 2,
    WM_73398_TEST_DFF4 = 3
};
void wm_73398_test_event(enum Wm73398TestEvent event,
                         u32 address,
                         u32 value);
#endif

#endif
