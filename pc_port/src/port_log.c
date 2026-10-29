/* port_log.c -- durable, traceable diagnostics for the owner's device.
 *
 * 1. Log mirroring.  PsyCross writes its own eprint lines to
 *    "<window title>.log" in the working directory (the launcher's data dir),
 *    but the port's own lines ([DEV_MENU], [CHEATS], [xeno-port], pad:) go to
 *    stderr and never reached that file.  PcPort_LogInit() (first thing in
 *    main) rotates the previous runs (.log -> .log.1 -> .log.2 -> .log.3, the
 *    last three runs are kept), then replaces fd 2 with a pipe whose reader
 *    thread copies every byte to the real stderr AND appends it to the same
 *    log file (O_APPEND; the PsyCross patch psycross_log_append opens its
 *    stream in append mode too, so the two writers interleave safely).  The
 *    file part stops at LOG_CAP_BYTES (a marker line says so); stderr is never
 *    throttled.  XENO_LOG_TEE=0 turns the mirror off.
 *
 * 2. Heartbeat.  Every XENO_HEARTBEAT_SECS (default 10) seconds a separate
 *    thread logs one line: presented frames, field-loop passes, map, the
 *    field-safe flag, dev-menu input state / drain age, and how long ago the
 *    game thread last ticked.  A freeze then reads directly off the log: which
 *    counter stopped and what state the input gate was in.  The snapshot is
 *    taken on the game thread (frame_tick hook); the thread only reads it, so
 *    it never touches game state that may be mid-teardown. */
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include "port_dev_menu.h"
#include "quick_checkpoint.h"
#include "../include/xg_plat/mods.h"

#define LOG_NAME "Xenogears (PC port).log"
#define LOG_KEEP 3
#define LOG_CAP_BYTES (4u * 1024u * 1024u)

extern int g_GameSceneMapNum;

static int s_realErr = -1, s_pipeRead = -1, s_logFd = -1;
static pthread_t s_teeThread;
static int s_teeStarted;

static void rotate_logs(void)
{
    char from[128], to[128];
    int i;
    snprintf(to, sizeof to, "%s.%d", LOG_NAME, LOG_KEEP);
    (void)unlink(to);
    for (i = LOG_KEEP - 1; i >= 1; i--) {
        snprintf(from, sizeof from, "%s.%d", LOG_NAME, i);
        snprintf(to, sizeof to, "%s.%d", LOG_NAME, i + 1);
        (void)rename(from, to);
    }
    snprintf(to, sizeof to, "%s.1", LOG_NAME);
    (void)rename(LOG_NAME, to);
}

static void write_all(int fd, const char *p, size_t n)
{
    while (n > 0) {
        ssize_t w = write(fd, p, n);
        if (w < 0) {
            if (errno == EINTR) continue;
            return;
        }
        p += w; n -= (size_t)w;
    }
}

static void *tee_main(void *arg)
{
    char buf[4096];
    size_t logged = 0;
    int capped = 0;
    ssize_t n;
    (void)arg;
    while ((n = read(s_pipeRead, buf, sizeof buf)) != 0) {
        if (n < 0) {
            if (errno == EINTR) continue;
            break;
        }
        write_all(s_realErr, buf, (size_t)n);
        if (s_logFd >= 0 && !capped) {
            if (logged + (size_t)n > LOG_CAP_BYTES) {
                static const char m[] = "[xeno-port][log] size cap reached; this file stops here "
                                        "(stderr is unaffected)\n";
                write_all(s_logFd, m, sizeof m - 1);
                capped = 1;
            } else {
                write_all(s_logFd, buf, (size_t)n);
                logged += (size_t)n;
            }
        }
    }
    return NULL;
}

static void log_shutdown(void)
{
    if (!s_teeStarted) return;
    fflush(stderr);
    /* Restore the real stderr: closes the pipe's write end, the reader drains
     * what is left and sees EOF. */
    (void)dup2(s_realErr, 2);
    (void)pthread_join(s_teeThread, NULL);
    s_teeStarted = 0;
}

/* For paths that leave with _Exit (no atexit): push what is still in the
 * stderr pipe out to the real stderr and the log before the process goes.
 * Bounded (~0.5 s) so a stuck reader cannot hold an exit. */
void PcPort_LogFlush(void)
{
    int pending = 0, i;
    if (!s_teeStarted) return;
    fflush(stderr);
    for (i = 0; i < 50; i++) {
        if (ioctl(s_pipeRead, FIONREAD, &pending) != 0 || pending <= 0) break;
        usleep(10000);
    }
    usleep(20000);   /* the reader's last write after its final read */
}

void PcPort_LogInit(void)
{
    int fds[2];
    const char *off = getenv("XENO_LOG_TEE");
    if (off && !strcmp(off, "0")) return;
    rotate_logs();
    s_logFd = open(LOG_NAME, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (s_logFd < 0)
        fprintf(stderr, "[xeno-port][log] cannot open '%s' (%s); stderr is not mirrored\n", LOG_NAME,
                strerror(errno));
    if (s_logFd < 0) return;
    if (pipe(fds) != 0) return;
    s_realErr = dup(2);
    if (s_realErr < 0) { close(fds[0]); close(fds[1]); return; }
    s_pipeRead = fds[0];
    if (pthread_create(&s_teeThread, NULL, tee_main, NULL) != 0) {
        close(fds[0]); close(fds[1]); close(s_realErr);
        return;
    }
    s_teeStarted = 1;
    (void)dup2(fds[1], 2);
    close(fds[1]);
    setvbuf(stderr, NULL, _IONBF, 0);
    atexit(log_shutdown);
    fprintf(stderr, "[xeno-port][log] stderr mirrored to ./%s (keeps the last %d runs, cap %u KB each)\n",
            LOG_NAME, LOG_KEEP, LOG_CAP_BYTES / 1024u);
}

/* ---- heartbeat ---------------------------------------------------------- */
static volatile unsigned s_ticks, s_loop, s_drainMs;
static volatile int s_map, s_safe, s_input;
static volatile long long s_lastTickMs;

static long long now_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static void heartbeat_tick(const XgPlatModEventData *e, void *u)
{
    unsigned ms = 0;
    (void)e; (void)u;
    s_ticks++;
    s_lastTickMs = now_ms();
    if ((s_ticks & 7u) == 1u) {   /* the safety read derefs actors: sample sparsely */
        s_loop = PcPort_FieldLoopCount();
        s_map = (int)(g_GameSceneMapNum & 0x3FFF);
        s_safe = PcPort_QuickCheckpointFieldIsSafe();
        s_input = PcPort_DevMenuInputState(&ms);
        s_drainMs = ms;
    }
}

static void *heartbeat_main(void *arg)
{
    long long start = now_ms();
    unsigned secs = (unsigned)(uintptr_t)arg;
    for (;;) {
        unsigned slept;
        for (slept = 0; slept < secs; slept++) sleep(1);
        fprintf(stderr,
                "[xeno-port][heartbeat] t=%llus frames=%u fieldLoop=%u map=%d fieldSafe=%d "
                "input=%s drainMs=%u lastTickAgoMs=%lld\n",
                (now_ms() - start) / 1000, s_ticks, s_loop, s_map, s_safe,
                s_input == 0 ? "game" : s_input == 1 ? "devmenu-open" : "drain", s_drainMs,
                s_lastTickMs ? now_ms() - s_lastTickMs : -1);
    }
    return NULL;
}

void PcPort_HeartbeatInit(void)
{
    static int done;
    const char *e = getenv("XENO_HEARTBEAT_SECS");
    unsigned secs = e && atoi(e) > 0 ? (unsigned)atoi(e) : 10u;
    pthread_t t;
    if (done) return;
    done = 1;
    xg_plat_mods_subscribe(XG_PLAT_EVENT_FRAME_TICK, heartbeat_tick, NULL);
    if (pthread_create(&t, NULL, heartbeat_main, (void *)(uintptr_t)secs) == 0)
        pthread_detach(t);
}
