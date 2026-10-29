/* TEST TOOLING ONLY -- never linked into the port.  LD_PRELOAD probe for the
 * battle-1 "write16 fault at 0x00000394 (pc=0x8007e7e0)" investigation.
 *
 * It observes (and optionally seeds) an UNMODIFIED port binary by
 * interposing two libc imports:
 *
 *  - getenv(): the port reads XENO_DETERMINISTIC_VBLANK_BATTLE exactly once,
 *    in battle entry (func_80070F40 -> PcPort_DetVblankBattleEnter), after
 *    the field is gone and before the guest battle runs.  There the probe
 *    logs the retail rand state (g_RandomSeed) and the watched words, and
 *    with PROBE_SEED=<hex> replaces the rand state, so one battle can be
 *    replayed from a chosen RNG state.  getenv is answered from environ
 *    directly (no dlsym in this path).
 *  - fflush(): battle_state_hash.c flushes at least once per battle frame;
 *    after the real fflush the probe samples the watched words and the
 *    state-hash frame counter and logs every change, plus the rand state
 *    once per new battle frame.
 *
 * Addresses come from `nm` of the binary under test (the port is non-PIE):
 *   PROBE_LOG=<path>          output (required)
 *   PROBE_PSXRAM=<hex>        g_PsxRam (guest RAM base; guest a -> base + (a & 0x1fffff))
 *   PROBE_SEED_ADDR=<hex>     g_RandomSeed
 *   PROBE_FRAME_ADDR=<hex>    frame static of PcPort_BattleStateHashBeforeGuest
 *   PROBE_GUEST=<label>:<guest hex>:<width>,...   guest words to watch
 *   PROBE_HOST=<label>:<host hex>:<width>,...     host globals to watch
 *   PROBE_SEED=<hex>          optional forced rand state at battle entry
 *   PROBE_POLL_US=<n>         optional: also poll the watches every n us on
 *                             a probe thread (times changes between flushes)
 *   PROBE_DUMP=<label>:<host hex>:<len>,...  hex dumps at battle entry
 *   PROBE_RT_ADDR=<hex> PROBE_RREAD_ADDR=<hex>  &g_BattleRuntime and
 *                             runtime_read: with both, every S line is followed
 *                             by "N <frame> <steps>", the step count of the
 *                             outermost interpreter PcPortMipsCpu, found on the
 *                             game thread's stack by its bus signature
 *                             (bus.opaque == &g_BattleRuntime, bus.read ==
 *                             runtime_read; offsets of battle_mips_adapter.h)
 * Reads only, except the optional seed store.  Output lines:
 *   I <t> <label>=<v> ...          first sample
 *   E <t> battle-entry seed=<v> [forced=<v>] <label>=<v> ...
 *   S <frame> <seed>               rand state when a new battle frame is seen
 *   C <t> frame=<f> <label> <old> -> <new> [entry|poll]
 * S lines come only from the thread that entered the battle (the game
 * thread), at its first flush after a frame change.
 */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

extern char **environ;

#define MAX_WATCH 16

typedef struct {
    char label[24];
    uintptr_t host;
    unsigned width;
    uint32_t last;
} Watch;

static pthread_mutex_t s_lock = PTHREAD_MUTEX_INITIALIZER;
static int s_inited, s_fd = -1;
static uintptr_t s_ram, s_seed, s_frame;
static Watch s_watch[MAX_WATCH];
static unsigned s_nwatch;
static uint32_t s_last_frame = 0xffffffffu;
static int s_have_forced;
static uint32_t s_forced;
static struct timespec s_t0;
static int (*s_real_fflush)(FILE *);
static __thread int s_busy;
static pthread_t s_game_thread;
static int s_have_game_thread;
static uintptr_t s_rt_addr, s_rread_addr;
static uintptr_t s_stack_lo, s_stack_hi;

/* PcPortMipsCpu layout (pc_port/src/battle_mips_adapter.h, LP64). */
#define CPU_STEPS_OFF 272u
#define CPU_BUS_OFF 296u

/* Step count of the outermost interpreter cpu on this thread's stack, or
 * ~0 when none is found.  Nested cpus (bridge calls back into guest code)
 * sit at lower addresses; the outermost one is func_80070F40's. */
static uint64_t probe_outer_steps(void)
{
    uintptr_t sp = (uintptr_t)__builtin_frame_address(0), a, found = 0;
    if (s_rt_addr == 0 || s_rread_addr == 0)
        return ~(uint64_t)0;
    if (s_stack_hi == 0) {
        pthread_attr_t at;
        void *lo;
        size_t size;
        if (pthread_getattr_np(pthread_self(), &at) != 0)
            return ~(uint64_t)0;
        pthread_attr_getstack(&at, &lo, &size);
        pthread_attr_destroy(&at);
        s_stack_lo = (uintptr_t)lo;
        s_stack_hi = (uintptr_t)lo + size;
    }
    if (sp < s_stack_lo || sp >= s_stack_hi)
        return ~(uint64_t)0;
    for (a = (sp + 7u) & ~(uintptr_t)7u; a + 16u <= s_stack_hi; a += 8u) {
        if (*(const uintptr_t *)a == s_rt_addr && *(const uintptr_t *)(a + 8u) == s_rread_addr)
            found = a;
    }
    if (found == 0 || found < CPU_BUS_OFF)
        return ~(uint64_t)0;
    return *(const uint64_t *)(found - CPU_BUS_OFF + CPU_STEPS_OFF);
}

static const char *env_raw(const char *name)
{
    size_t n = strlen(name);
    for (char **e = environ; e != NULL && *e != NULL; e++)
        if (strncmp(*e, name, n) == 0 && (*e)[n] == '=')
            return *e + n + 1;
    return NULL;
}

static double now_s(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (double)(t.tv_sec - s_t0.tv_sec) + (double)(t.tv_nsec - s_t0.tv_nsec) / 1e9;
}

static void out(const char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void out(const char *fmt, ...)
{
    char buf[1024];
    va_list ap;
    int n;
    if (s_fd < 0)
        return;
    va_start(ap, fmt);
    n = vsnprintf(buf, sizeof buf, fmt, ap);
    va_end(ap);
    if (n > (int)sizeof buf - 1)
        n = (int)sizeof buf - 1;
    if (n > 0 && write(s_fd, buf, (size_t)n) < 0) {
        /* diagnostics only */
    }
}

static uint32_t read_width(uintptr_t p, unsigned width)
{
    if (width == 1) return *(volatile uint8_t *)p;
    if (width == 2) return *(volatile uint16_t *)p;
    return *(volatile uint32_t *)p;
}

static void add_watches(const char *spec, int guest)
{
    char tmp[1024];
    char *save = NULL, *item;
    if (spec == NULL)
        return;
    snprintf(tmp, sizeof tmp, "%s", spec);
    for (item = strtok_r(tmp, ",", &save); item != NULL && s_nwatch < MAX_WATCH;
         item = strtok_r(NULL, ",", &save)) {
        char *a = strchr(item, ':'), *w;
        uintptr_t addr;
        Watch *x;
        if (a == NULL) continue;
        *a++ = '\0';
        w = strchr(a, ':');
        if (w != NULL) *w++ = '\0';
        addr = (uintptr_t)strtoull(a, NULL, 16);
        x = &s_watch[s_nwatch++];
        snprintf(x->label, sizeof x->label, "%s", item);
        x->width = w != NULL ? (unsigned)strtoul(w, NULL, 0) : 4u;
        if (x->width != 1 && x->width != 2) x->width = 4;
        x->host = guest ? s_ram + (addr & 0x1fffffu) : addr;
    }
}

static void probe_sample(int poll);

static void *probe_poller(void *arg)
{
    struct timespec t;
    long us = (long)(intptr_t)arg;
    t.tv_sec = us / 1000000;
    t.tv_nsec = (us % 1000000) * 1000;
    for (;;) {
        nanosleep(&t, NULL);
        pthread_mutex_lock(&s_lock);
        probe_sample(1);
        pthread_mutex_unlock(&s_lock);
    }
    return NULL;
}

static void probe_init(void)
{
    const char *p;
    if (s_inited)
        return;
    s_inited = 1;
    clock_gettime(CLOCK_MONOTONIC, &s_t0);
    p = env_raw("PROBE_LOG");
    if (p == NULL || p[0] == '\0')
        return;
    s_fd = open(p, O_WRONLY | O_CREAT | O_APPEND | O_CLOEXEC, 0600);
    if ((p = env_raw("PROBE_PSXRAM")) != NULL) s_ram = (uintptr_t)strtoull(p, NULL, 16);
    if ((p = env_raw("PROBE_SEED_ADDR")) != NULL) s_seed = (uintptr_t)strtoull(p, NULL, 16);
    if ((p = env_raw("PROBE_FRAME_ADDR")) != NULL) s_frame = (uintptr_t)strtoull(p, NULL, 16);
    if ((p = env_raw("PROBE_RT_ADDR")) != NULL) s_rt_addr = (uintptr_t)strtoull(p, NULL, 16);
    if ((p = env_raw("PROBE_RREAD_ADDR")) != NULL) s_rread_addr = (uintptr_t)strtoull(p, NULL, 16);
    if ((p = env_raw("PROBE_SEED")) != NULL && p[0] != '\0') {
        s_have_forced = 1;
        s_forced = (uint32_t)strtoul(p, NULL, 16);
    }
    if (s_ram != 0)
        add_watches(env_raw("PROBE_GUEST"), 1);
    add_watches(env_raw("PROBE_HOST"), 0);
    out("# probe pid=%d ram=%#lx seed@%#lx frame@%#lx forced=%s%08x watches=%u\n",
        (int)getpid(), (unsigned long)s_ram, (unsigned long)s_seed, (unsigned long)s_frame,
        s_have_forced ? "" : "none:", s_forced, s_nwatch);
    {
        unsigned i;
        out("I %.3f", now_s());
        for (i = 0; i < s_nwatch; i++) {
            s_watch[i].last = read_width(s_watch[i].host, s_watch[i].width);
            out(" %s=%#x", s_watch[i].label, s_watch[i].last);
        }
        out("\n");
    }
    if ((p = env_raw("PROBE_POLL_US")) != NULL && atol(p) > 0) {
        pthread_t th;
        sigset_t all, old;
        /* The poller must not take the game's signals. */
        sigfillset(&all);
        pthread_sigmask(SIG_SETMASK, &all, &old);
        if (pthread_create(&th, NULL, probe_poller, (void *)(intptr_t)atol(p)) == 0)
            pthread_detach(th);
        pthread_sigmask(SIG_SETMASK, &old, NULL);
    }
}

static void probe_battle_entry(void)
{
    unsigned i;
    uint32_t seed = s_seed ? *(volatile uint32_t *)s_seed : 0;
    pthread_mutex_lock(&s_lock);
    probe_init();
    out("E %.3f battle-entry seed=%08x", now_s(), seed);
    if (s_have_forced && s_seed) {
        *(volatile uint32_t *)s_seed = s_forced;
        out(" forced=%08x", s_forced);
    }
    s_game_thread = pthread_self();
    s_have_game_thread = 1;
    for (i = 0; i < s_nwatch; i++)
        out(" %s=%#x", s_watch[i].label, read_width(s_watch[i].host, s_watch[i].width));
    out("\n");
    for (i = 0; i < s_nwatch; i++) {
        uint32_t v = read_width(s_watch[i].host, s_watch[i].width);
        if (v != s_watch[i].last)
            out("C %.3f frame=%u %s %#x -> %#x entry\n", now_s(),
                s_frame ? *(volatile uint32_t *)s_frame : 0u, s_watch[i].label, s_watch[i].last, v);
        s_watch[i].last = v;
    }
    /* PROBE_DUMP=<label>:<host hex>:<len>,...: hex dump of host memory at
     * battle entry (e.g. the per-map battle descriptor table). */
    {
        const char *spec = env_raw("PROBE_DUMP");
        char tmp[512], *save = NULL, *item;
        if (spec != NULL) {
            snprintf(tmp, sizeof tmp, "%s", spec);
            for (item = strtok_r(tmp, ",", &save); item != NULL; item = strtok_r(NULL, ",", &save)) {
                char *a = strchr(item, ':'), *l;
                const uint8_t *p;
                unsigned len, k;
                if (a == NULL || (l = strchr(a + 1, ':')) == NULL) continue;
                *a++ = '\0';
                *l++ = '\0';
                p = (const uint8_t *)(uintptr_t)strtoull(a, NULL, 16);
                len = (unsigned)strtoul(l, NULL, 0);
                for (k = 0; k < len; k += 32) {
                    unsigned j;
                    out("D %s +%03x", item, k);
                    for (j = k; j < k + 32 && j < len; j++) out("%s%02x", (j & 3) ? "" : " ", p[j]);
                    out("\n");
                }
            }
        }
    }
    pthread_mutex_unlock(&s_lock);
}

static void probe_sample(int poll)
{
    unsigned i;
    uint32_t frame;
    if (s_fd < 0)
        return;
    frame = s_frame ? *(volatile uint32_t *)s_frame : 0;
    for (i = 0; i < s_nwatch; i++) {
        uint32_t v = read_width(s_watch[i].host, s_watch[i].width);
        if (v != s_watch[i].last) {
            out("C %.3f frame=%u %s %#x -> %#x%s\n", now_s(), frame, s_watch[i].label,
                s_watch[i].last, v, poll ? " poll" : "");
            s_watch[i].last = v;
        }
    }
    if (poll || !s_have_game_thread || !pthread_equal(pthread_self(), s_game_thread))
        return;
    if (frame != s_last_frame) {
        s_last_frame = frame;
        if (s_seed)
            out("S %u %08x\n", frame, *(volatile uint32_t *)s_seed);
        if (s_rt_addr != 0) {
            uint64_t steps = probe_outer_steps();
            out("N %u %llu\n", frame, (unsigned long long)steps);
        }
    }
}

char *getenv(const char *name)
{
    const char *v = env_raw(name);
    if (strcmp(name, "XENO_DETERMINISTIC_VBLANK_BATTLE") == 0 && !s_busy) {
        s_busy = 1;
        probe_battle_entry();
        s_busy = 0;
    }
    return (char *)v;
}

int fflush(FILE *stream)
{
    int rc;
    if (s_real_fflush == NULL)
        s_real_fflush = (int (*)(FILE *))dlsym(RTLD_NEXT, "fflush");
    rc = s_real_fflush != NULL ? s_real_fflush(stream) : 0;
    if (!s_busy) {
        s_busy = 1;
        pthread_mutex_lock(&s_lock);
        probe_init();
        probe_sample(0);
        pthread_mutex_unlock(&s_lock);
        s_busy = 0;
    }
    return rc;
}
