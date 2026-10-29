/* TEST TOOLING ONLY -- never linked into the port.  LD_PRELOAD probe that
 * replays battle_command_file1 func_801E77E4's post-battle movie request
 * through the production battle bus of an UNMODIFIED port binary and logs
 * what the game makes of it:
 *
 *  - fflush() (battle_state_hash.c flushes once per battle frame): with an
 *    active battle runtime, once the battle's outcome is decided (guest
 *    D_800C48EA turns non-zero; retail's after-controller runs then, in
 *    main.c after the battle loop), make the after-controller's stores with
 *    the runtime's own runtime_write -- 0x8004FE44 = type | 0x80, 0x8004FE45
 *    = movie number, 0x8004FE46 = 1 (state after: Field), 0x8004FE47 =
 *    no-skip flag, 0x80062514 = end frame (u16) and 0x800D3338 = 1, the flag
 *    the battle exit func_8001B6C4 tests to choose ChangeGameState(6) -- and
 *    log the native globals MovieMain reads; again on a later battle frame
 *    if the guest flag no longer reads 1;
 *  - fprintf() "retail battle returned" (battle_mips_runtime.c, after the
 *    runtime is released and before func_8001B6C4 reads the result byte):
 *    log guest D_800C48EA and D_800D3338.  The test battle is a random
 *    battle; retail's after-controller only runs in event battles, and those
 *    leave D_800C48EA = 1 (main2.c: no enemy left; main.c: a scripted loss
 *    becomes 1).  A result outside func_8001B6C4's win set {1, 0x40, 0x21}
 *    (0x81: party wiped, the game-over path) is set to 1, so the exit tests
 *    D_800D3338 and itself calls ChangeGameState(6);
 *  - a polling thread (every 0.5 ms): log each g_CurGameState change,
 *    MovieMain's type / number / end frame (D_80077448 / D_8007711C /
 *    D_8007739C), the guest D_800D3338 byte, and the request the battle exit
 *    sees when the battle runtime is released.  MainLoop puts the state back
 *    to 0 before a state's main runs, so a chosen state shows only while
 *    MainLoop loads it: after the stores, a 6 is the battle exit's
 *    ChangeGameState(6) and the next 1 MovieMain's ChangeGameState(D_8004FE46).
 *
 * Addresses come from nm of the binary under test (the port is non-PIE), all
 * hex: MRP_RUNTIME (g_ActiveBattleRuntime), MRP_WRITE (runtime_write),
 * MRP_STATE (g_CurGameState), MRP_RAM (g_PsxRam), MRP_TYPE, MRP_NUMBER,
 * MRP_END (MovieMain's three), MRP_FE44 .. MRP_FE47, MRP_F2514 (the native
 * request globals).  MRP_REQUEST=<type>:<number>:<end>:<no-skip>, MRP_LOG=<path>.
 * The probe stays inert in any process whose exported g_CurGameState is not
 * at MRP_STATE (LD_PRELOAD reaches child processes too). */
#define _GNU_SOURCE
#include <dlfcn.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static int log_fd = -1, active;
static uintptr_t a_runtime, a_write, a_state, a_ram, a_type, a_number, a_end;
static uintptr_t a_fe[4], a_f2514;
static unsigned req_type, req_number, req_end, req_skip;
static int written, saw_exit6, returned;   /* written: set by fflush */
static unsigned stores;
static int armed;                          /* saw the runtime with no result yet */
static struct timespec t0;

static void say(const char *fmt, ...)
{
    char line[512];
    struct timespec now;
    va_list ap;
    int n;
    clock_gettime(CLOCK_MONOTONIC, &now);
    n = snprintf(line, sizeof(line), "%8.3f ", (now.tv_sec - t0.tv_sec) + (now.tv_nsec - t0.tv_nsec) / 1e9);
    va_start(ap, fmt);
    n += vsnprintf(line + n, sizeof(line) - (size_t)n, fmt, ap);
    va_end(ap);
    if (n > (int)sizeof(line) - 2)
        n = (int)sizeof(line) - 2;
    line[n++] = '\n';
    if (log_fd >= 0 && write(log_fd, line, (size_t)n) < 0) {
        /* nothing to do */
    }
}

static uintptr_t hexenv(const char *name)
{
    const char *v = getenv(name);
    return v ? (uintptr_t)strtoull(v, NULL, 16) : 0;
}

static uint8_t guest_d3338(void)
{
    return *(volatile uint8_t *)(a_ram + 0xD3338u);
}

static void *poll_thread(void *arg)
{
    unsigned last_state = 0xffffffffu;
    int32_t last_type = -1, last_number = -1, last_end = -1;
    int last_flag = -1, flag_changes = 0;
    void *last_rt = NULL;
    (void)arg;
    for (;;) {
        struct timespec nap = {0, 500000};
        unsigned state = *(volatile uint32_t *)a_state;
        int32_t type = *(volatile int32_t *)a_type, number = *(volatile int32_t *)a_number;
        int32_t end = *(volatile int32_t *)a_end;
        int flag = guest_d3338();
        int stored = __atomic_load_n(&written, __ATOMIC_ACQUIRE);
        void *rt = *(void *volatile *)a_runtime;
        if (last_rt != NULL && rt == NULL && stored)
            say("E battle runtime released after %u store rounds: native D_8004FE44=0x%02x "
                "FE45=%u FE46=%u FE47=%u D_80062514=%u; guest D_800D3338=%u",
                __atomic_load_n(&stores, __ATOMIC_ACQUIRE), *(volatile uint8_t *)a_fe[0],
                *(volatile uint8_t *)a_fe[1], *(volatile uint8_t *)a_fe[2],
                *(volatile uint8_t *)a_fe[3], *(volatile uint16_t *)a_f2514, flag);
        last_rt = rt;
        if (type != last_type || number != last_number || end != last_end) {
            say("M MovieMain type=%d number=%d end_frame=%d (state %u)", type, number, end, state);
            last_type = type;
            last_number = number;
            last_end = end;
        }
        if (flag != last_flag) {
            if (++flag_changes <= 20)
                say("G guest D_800D3338=%d", flag);
            last_flag = flag;
        }
        if (state != last_state) {
            say("S game state %u -> %u", last_state, state);
            if (stored && state == 6u && !saw_exit6) {
                saw_exit6 = 1;
                say("C battle exit chose ChangeGameState(6)");
            } else if (saw_exit6 && state == 1u && !returned) {
                returned = 1;
                say("R returned to field state 1 after the movie: MovieMain type=%d number=%d "
                    "end_frame=%d", type, number, end);
            }
            last_state = state;
        }
        nanosleep(&nap, NULL);
    }
    return NULL;
}

__attribute__((constructor)) static void configure(void)
{
    const char *req, *path;
    pthread_t thread;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    a_runtime = hexenv("MRP_RUNTIME"); a_write = hexenv("MRP_WRITE");
    a_state = hexenv("MRP_STATE"); a_ram = hexenv("MRP_RAM");
    a_type = hexenv("MRP_TYPE"); a_number = hexenv("MRP_NUMBER"); a_end = hexenv("MRP_END");
    a_fe[0] = hexenv("MRP_FE44"); a_fe[1] = hexenv("MRP_FE45");
    a_fe[2] = hexenv("MRP_FE46"); a_fe[3] = hexenv("MRP_FE47");
    a_f2514 = hexenv("MRP_F2514");
    if (!a_state || dlsym(RTLD_DEFAULT, "g_CurGameState") != (void *)a_state)
        return;   /* not the binary under test */
    path = getenv("MRP_LOG");
    if (path)
        log_fd = open(path, O_WRONLY | O_CREAT | O_TRUNC | O_APPEND, 0644);
    req = getenv("MRP_REQUEST");
    if (req)
        sscanf(req, "%u:%u:%u:%u", &req_type, &req_number, &req_end, &req_skip);
    say("I request type=%u number=%u end=%u skip=%u", req_type, req_number, req_end, req_skip);
    if (!a_runtime || !a_write || !a_ram || !a_type || !a_number || !a_end || !a_f2514 ||
        !a_fe[0] || !a_fe[1] || !a_fe[2] || !a_fe[3]) {
        say("I missing an address: probe inert");
        return;
    }
    active = 1;
    if (pthread_create(&thread, NULL, poll_thread, NULL) == 0)
        pthread_detach(thread);
    else
        say("I no polling thread");
}

static void log_native_request(const char *when)
{
    say("N %s native D_8004FE44=0x%02x FE45=%u FE46=%u FE47=%u D_80062514=%u; guest D_800D3338=%u",
        when, *(uint8_t *)a_fe[0], *(uint8_t *)a_fe[1], *(uint8_t *)a_fe[2], *(uint8_t *)a_fe[3],
        *(uint16_t *)a_f2514, guest_d3338());
}

static void battle_returned(void)
{
    volatile uint8_t *result = (volatile uint8_t *)(a_ram + 0xC48EAu);
    uint8_t was = *result;
    say("B battle returned: guest D_800C48EA=0x%02x D_800D3338=%u", was, guest_d3338());
    if (was != 1u && was != 0x40u && was != 0x21u) {
        *result = 1u;
        say("X result 0x%02x is outside the exit's win set: set to 1, as retail's event "
            "battles leave it", was);
    }
}

int fprintf(FILE *stream, const char *fmt, ...)
{
    static int (*real)(FILE *, const char *, va_list);
    va_list ap;
    int rc;
    if (!real)
        real = (int (*)(FILE *, const char *, va_list))dlsym(RTLD_NEXT, "vfprintf");
    va_start(ap, fmt);
    rc = real(stream, fmt, ap);
    va_end(ap);
    if (active && __atomic_load_n(&written, __ATOMIC_ACQUIRE) &&
        strstr(fmt, "retail battle returned") != NULL)
        battle_returned();
    return rc;
}

int fflush(FILE *stream)
{
    static int (*real)(FILE *);
    int rc;
    if (!real)
        real = (int (*)(FILE *))dlsym(RTLD_NEXT, "fflush");
    rc = real(stream);
    if (active && *(void **)a_runtime != NULL) {
        void *rt = *(void **)a_runtime;
        uint8_t outcome = *(volatile uint8_t *)(a_ram + 0xC48EAu);
        if (outcome == 0)
            armed = 1;
        if (armed && outcome != 0 && (!written || guest_d3338() != 1u)) {
            int (*rw)(void *, uint32_t, unsigned, uint32_t) =
                (int (*)(void *, uint32_t, unsigned, uint32_t))a_write;
            int r0 = rw(rt, 0x8004FE44u, 1, (req_type & 0x7fu) | 0x80u);
            int r1 = rw(rt, 0x8004FE45u, 1, req_number & 0xffu);
            int r2 = rw(rt, 0x8004FE46u, 1, 1u);
            int r3 = rw(rt, 0x8004FE47u, 1, req_skip & 0xffu);
            int r4 = rw(rt, 0x80062514u, 2, req_end & 0xffffu);
            int r5 = rw(rt, 0x800D3338u, 1, 1u);
            unsigned round = __atomic_add_fetch(&stores, 1, __ATOMIC_ACQ_REL);
            __atomic_store_n(&written, 1, __ATOMIC_RELEASE);
            if (round <= 5 || round % 100 == 0) {
                say("W store round %u (battle outcome D_800C48EA=0x%02x) through runtime_write: "
                    "FE44=0x%02x FE45=%u FE46=1 FE47=%u 80062514=%u 800D3338=1 "
                    "(rc %d %d %d %d %d %d)", round, outcome, (req_type & 0x7fu) | 0x80u,
                    req_number, req_skip, req_end, r0, r1, r2, r3, r4, r5);
                log_native_request("after the stores:");
            }
        }
    }
    return rc;
}
