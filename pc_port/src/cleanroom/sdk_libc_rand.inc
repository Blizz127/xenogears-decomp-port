#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

/* The port's checkpoints, scripts and guest bridge share this 32-bit word. */
uint32_t g_RandomSeed;

/* The field-step vblank log consumes these counters and marks its game thread. */
static uint32_t s_rand_calls, s_rand_calls_off_main;
static void *s_rand_last_off_main;
static _Thread_local int s_rand_on_main;
extern int g_GameSceneMapNum __attribute__((weak));
extern int PcPort_FieldStepPadArmedStep(unsigned *step) __attribute__((weak));

static void record_random_draw(void *caller)
{
    static FILE *trace;
    static int initialized;

    ++s_rand_calls;
    if (!s_rand_on_main) {
        ++s_rand_calls_off_main;
        s_rand_last_off_main = caller;
    }

    if (!initialized) {
        initialized = 1;
        const char *path = getenv("XENO_RAND_TRACE");
        if (path && *path && &g_GameSceneMapNum && PcPort_FieldStepPadArmedStep)
            trace = fopen(path, "w");
    }
    if (trace) {
        unsigned step;
        if (PcPort_FieldStepPadArmedStep(&step)) {
            fclose(trace);
            trace = NULL;
        } else {
            fprintf(trace, "%u %d %p\n", (unsigned)s_rand_calls, g_GameSceneMapNum, caller);
        }
    }
}

int rand(void)
{
    /* The C11 7.22.2.2 example's LCG parameters match the recorded game draws.
     * Unsigned arithmetic retains all 32 state bits across checkpoint restores. */
    uint32_t state = UINT32_C(1103515245) * g_RandomSeed + UINT32_C(12345);
    record_random_draw(__builtin_return_address(0));
    g_RandomSeed = state;
    return (int)((state >> 16) & UINT32_C(0x7fff));
}
