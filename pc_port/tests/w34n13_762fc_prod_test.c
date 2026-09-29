#include <stdio.h>
#include <stdlib.h>

#include "world_map_helper_762fc.h"

enum Event {
    EVENT_DRAW_SYNC,
    EVENT_VSYNC,
    EVENT_ENTER_CRITICAL,
    EVENT_FLUSH_CACHE,
    EVENT_EXIT_CRITICAL
};

typedef struct {
    enum Event event;
    int argument;
} EventRecord;

static EventRecord g_events[8];
static int g_event_count;

static void record_event(enum Event event, int argument)
{
    if (g_event_count >= (int)(sizeof(g_events) / sizeof(g_events[0]))) {
        fprintf(stderr, "ASSERT_EVENT_CAPACITY\n");
        exit(1);
    }
    g_events[g_event_count].event = event;
    g_events[g_event_count].argument = argument;
    g_event_count++;
}

int DrawSync(int mode)
{
    record_event(EVENT_DRAW_SYNC, mode);
    return 0;
}

int VSync(int mode)
{
    record_event(EVENT_VSYNC, mode);
    return 0;
}

void EnterCriticalSection(void)
{
    record_event(EVENT_ENTER_CRITICAL, 0);
}

void FlushCache(void)
{
    record_event(EVENT_FLUSH_CACHE, 0);
}

void ExitCriticalSection(void)
{
    record_event(EVENT_EXIT_CRITICAL, 0);
}

static void require_event(int index, enum Event event, int argument)
{
    if (index >= g_event_count || g_events[index].event != event) {
        fprintf(stderr, "ASSERT_EVENT_SEQUENCE index=%d count=%d\n",
                index, g_event_count);
        exit(1);
    }
    if (g_events[index].argument != argument) {
        fprintf(stderr, "ASSERT_SYNC_ARGUMENT index=%d got=%d expected=%d\n",
                index, g_events[index].argument, argument);
        exit(1);
    }
}

int main(void)
{
    wm_800762FC();

    if (g_event_count != 7) {
        fprintf(stderr, "ASSERT_EVENT_COUNT got=%d expected=7\n", g_event_count);
        return 1;
    }
    require_event(0, EVENT_DRAW_SYNC, 0);
    require_event(1, EVENT_VSYNC, 0);
    require_event(2, EVENT_ENTER_CRITICAL, 0);
    require_event(3, EVENT_DRAW_SYNC, 0);
    require_event(4, EVENT_VSYNC, 0);
    require_event(5, EVENT_FLUSH_CACHE, 0);
    require_event(6, EVENT_EXIT_CRITICAL, 0);

    puts("W34N13 762FC PRODUCTION CERTIFICATE PASS");
    return 0;
}
