/* Function-coverage recorder for XENO_COVERAGE=1 port builds (build_port.sh).
 *
 * Every TU of a coverage build is compiled with -finstrument-functions, so
 * each function entry calls __cyg_profile_func_enter(fn, site).  This file is
 * compiled without instrumentation.  It keeps a lock-free table of
 * {function address, call count} and a helper thread writes it every two
 * seconds (and at exit) to $XENO_COVERAGE_OUT (default coverage.txt), one
 * "<hex address> <calls>" line per executed function, replaced atomically.
 * Periodic writes survive runs that end by timeout or signal (SDL installs
 * its own SIGINT/SIGTERM handlers).  tools/analysis/port_preservation.py
 * turns the file into the executed-code classification. */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NI __attribute__((no_instrument_function))
#define SLOTS (1u << 17)

typedef struct { uintptr_t fn; uint64_t calls; } Slot;
static Slot s_table[SLOTS];
static volatile int s_stop;

NI void __cyg_profile_func_enter(void* fn, void* site);
NI void __cyg_profile_func_exit(void* fn, void* site);

NI void __cyg_profile_func_enter(void* fn, void* site)
{
    uintptr_t key = (uintptr_t)fn;
    uint32_t i = (uint32_t)((key >> 4) * 2654435761u) & (SLOTS - 1);
    uint32_t n;
    (void)site;
    for (n = 0; n < SLOTS; n++, i = (i + 1) & (SLOTS - 1)) {
        uintptr_t cur = __atomic_load_n(&s_table[i].fn, __ATOMIC_RELAXED);
        if (cur == 0) {
            uintptr_t expected = 0;
            if (__atomic_compare_exchange_n(&s_table[i].fn, &expected, key, 0,
                                            __ATOMIC_RELAXED, __ATOMIC_RELAXED))
                cur = key;
            else
                cur = expected;
        }
        if (cur == key) {
            __atomic_fetch_add(&s_table[i].calls, 1, __ATOMIC_RELAXED);
            return;
        }
    }
}

NI void __cyg_profile_func_exit(void* fn, void* site) { (void)fn; (void)site; }

NI static void dump(void)
{
    const char* out = getenv("XENO_COVERAGE_OUT");
    char tmp[1024];
    FILE* f;
    uint32_t i;
    if (out == NULL || *out == '\0')
        out = "coverage.txt";
    snprintf(tmp, sizeof tmp, "%s.tmp", out);
    if ((f = fopen(tmp, "w")) == NULL)
        return;
    for (i = 0; i < SLOTS; i++) {
        uintptr_t fn = __atomic_load_n(&s_table[i].fn, __ATOMIC_RELAXED);
        if (fn)
            fprintf(f, "%lx %llu\n", (unsigned long)fn,
                    (unsigned long long)__atomic_load_n(&s_table[i].calls, __ATOMIC_RELAXED));
    }
    fclose(f);
    rename(tmp, out);
}

NI static void* writer(void* arg)
{
    (void)arg;
    while (!s_stop) {
        sleep(2);
        dump();
    }
    return NULL;
}

NI __attribute__((constructor)) static void coverage_start(void)
{
    pthread_t t;
    if (pthread_create(&t, NULL, writer, NULL) == 0)
        pthread_detach(t);
}

NI __attribute__((destructor)) static void coverage_stop(void)
{
    s_stop = 1;
    dump();
}
