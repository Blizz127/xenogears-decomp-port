/* port_soft_reset.c -- the soft reset (START+SELECT+L1+R1) as a re-exec.
 *
 * Retail: GameCheckAndHandleSoftReset (src/slus_006.64/main/main_loop.c)
 * calls GameSoftReset when exactly START+SELECT+L1+R1 are held. GameSoftReset
 * tears down graphics, archive, sound, SPU and CD, then calls the crt0 entry
 * `start` (0x80019524, main_loop.c:199). That clears .bss, resets the stack
 * and boots to the title again. It never returns.
 *
 * start() below is the port's equivalent of that crt0 restart. It leaves
 * through the normal quit path, exit(0), so the atexit chain runs as it does
 * when the window is closed:
 *   - PsyX_Shutdown joins the CD worker and the vblank thread, then releases
 *     the GL context, the window and SDL;
 *   - port_log restores the real stderr and drains the log mirror.
 * PcPort_SoftResetInit() registers its handler first thing in main(), so that
 * handler runs last. It replaces the process image with /proc/self/exe, using
 * the argv saved at boot. By then GameSoftReset's own SpuQuit has already
 * closed the OpenAL device.
 *
 * The new image is the port's .bss clear and stack reset: every native
 * global, driver, cache and thread starts over. It keeps the same PID,
 * environment (inherited as-is) and working directory. Unlike retail, the
 * port's .data equivalents and the config file are loaded again too; retail
 * keeps .data across a soft reset.
 *
 * What crosses the exec, and what does not:
 *   - Descriptors open when main() started (stdin/stdout/stderr and anything
 *     the launcher passed) are inherited.
 *   - Every descriptor opened later (disc image, logs, audio/GL device nodes,
 *     sockets) is marked close-on-exec just before execv.
 *   - The boot signal mask is restored, and signals ignored since boot go back
 *     to SIG_DFL.
 *   - stdout and stderr are flushed. fflush(NULL) is not used, because it
 *     takes every stream's lock and the cheat console's reader thread sits in
 *     fgets(stdin). Any other still-open stdio stream (env-gated trace files)
 *     loses its unflushed tail.
 *   - The kernel names a task after the exec path, so the new image would show
 *     up as "exe". It renames itself after argv[0] at boot.
 * If execv fails, one line is logged and the exit finishes with status 0. The
 * frame loop is never re-entered.
 *
 * Linux only (/proc/self/exe, /proc/self/fd). Windows and macOS need their own
 * relaunch (CreateProcess, or posix_spawn of the executable, then exit). On
 * those platforms start() logs one line and exits cleanly instead.
 *
 * CdFlush (GameSoftReset, main_loop.c:196) stays a generated stub. libcd's
 * flush has nothing left to do once the process image is replaced, and a port
 * definition of an SDK name belongs in the guarded compat layer. */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void PcPort_SoftResetInit(int argc, char** argv);
void start(void) __attribute__((noreturn));

#if defined(__linux__)
#include <dirent.h>
#include <fcntl.h>
#include <limits.h>
#include <pthread.h>
#include <sys/prctl.h>
#include <unistd.h>

#define BOOT_FD_LIMIT 1024

static char** s_argv;                           /* NULL-terminated copy of main's argv */
static unsigned char s_bootFd[BOOT_FD_LIMIT / 8]; /* descriptors open at main() entry */
static int s_bootFdKnown;
static sigset_t s_bootMask;
static unsigned char s_bootIgnored[NSIG];
static int s_restart;

/* A /proc/self/fd entry name as a descriptor number; -1 for "." and "..". */
static int fd_from_name(const char* s)
{
    char* end;
    long v;
    if (*s < '0' || *s > '9')
        return -1;
    v = strtol(s, &end, 10);
    return (*end == '\0' && v <= INT_MAX) ? (int)v : -1;
}

static int fd_inherited(int fd)
{
    if (!s_bootFdKnown)
        return fd <= 2;
    return fd < BOOT_FD_LIMIT && (s_bootFd[fd >> 3] & (1u << (fd & 7))) != 0;
}

static void mark_port_fds_cloexec(void)
{
    DIR* d = opendir("/proc/self/fd");
    struct dirent* e;
    if (d == NULL)
        return;
    while ((e = readdir(d)) != NULL) {
        int fd = fd_from_name(e->d_name), flags;
        if (fd < 0 || fd == dirfd(d) || fd_inherited(fd))
            continue;
        flags = fcntl(fd, F_GETFD);
        if (flags >= 0 && (flags & FD_CLOEXEC) == 0)
            (void)fcntl(fd, F_SETFD, flags | FD_CLOEXEC);
    }
    closedir(d);
}

static void restore_boot_signals(void)
{
    int sig;
    for (sig = 1; sig < NSIG; sig++) {
        struct sigaction sa;
        if (s_bootIgnored[sig] || sig == SIGKILL || sig == SIGSTOP)
            continue;
        if (sigaction(sig, NULL, &sa) != 0 || sa.sa_handler != SIG_IGN)
            continue;
        memset(&sa, 0, sizeof sa);
        sa.sa_handler = SIG_DFL;
        sigemptyset(&sa.sa_mask);
        (void)sigaction(sig, &sa, NULL);
    }
    (void)pthread_sigmask(SIG_SETMASK, &s_bootMask, NULL);
}

/* Registered first, so it runs after every other atexit handler. */
static void soft_reset_exec(void)
{
    if (!s_restart || s_argv == NULL)
        return;
    fflush(stdout);
    fflush(stderr);
    mark_port_fds_cloexec();
    restore_boot_signals();
    execv("/proc/self/exe", s_argv);
    fprintf(stderr, "[xeno-port][soft-reset] execv(/proc/self/exe) failed (%s); exiting\n",
            strerror(errno));
}

/* A restarted image is named "exe" (the basename of /proc/self/exe); take the
 * name the first launch had, argv[0]'s basename, back. */
static void restore_process_name(const char* argv0)
{
    char comm[32] = "";
    const char* base;
    FILE* f = fopen("/proc/self/comm", "r");
    if (f == NULL)
        return;
    if (fgets(comm, sizeof comm, f) == NULL)
        comm[0] = '\0';
    fclose(f);
    if (strcmp(comm, "exe\n") != 0 || argv0 == NULL || argv0[0] == '\0')
        return;
    base = strrchr(argv0, '/');
    (void)prctl(PR_SET_NAME, base != NULL ? base + 1 : argv0, 0, 0, 0);
}

void PcPort_SoftResetInit(int argc, char** argv)
{
    char list[128];
    size_t used = 0;
    unsigned extra = 0;
    DIR* d;
    int i;

    s_argv = calloc((size_t)(argc > 0 ? argc : 0) + 1, sizeof *s_argv);
    if (s_argv != NULL)
        for (i = 0; i < argc; i++)
            s_argv[i] = argv[i];

    d = opendir("/proc/self/fd");
    if (d != NULL) {
        struct dirent* e;
        while ((e = readdir(d)) != NULL) {
            int fd = fd_from_name(e->d_name);
            if (fd < 0 || fd == dirfd(d))
                continue;
            if (fd < BOOT_FD_LIMIT)
                s_bootFd[fd >> 3] |= (unsigned char)(1u << (fd & 7));
            else
                extra++;
        }
        closedir(d);
        s_bootFdKnown = 1;
    }
    list[0] = '\0';
    for (i = 0; i < BOOT_FD_LIMIT && s_bootFdKnown; i++)
        if (fd_inherited(i) && used + 12 < sizeof list)
            used += (size_t)snprintf(list + used, sizeof list - used, used ? ",%d" : "%d", i);

    (void)pthread_sigmask(SIG_BLOCK, NULL, &s_bootMask);
    for (i = 1; i < NSIG; i++) {
        struct sigaction sa;
        if (sigaction(i, NULL, &sa) == 0 && sa.sa_handler == SIG_IGN)
            s_bootIgnored[i] = 1;
    }
    restore_process_name(argc > 0 ? argv[0] : NULL);

    if (atexit(soft_reset_exec) != 0)
        s_argv = NULL;
    fprintf(stderr, "[xeno-port][soft-reset] armed: pid=%d inherited fds=%s%s%s\n", (int)getpid(),
            s_bootFdKnown ? list : "unknown (0-2 assumed)", extra ? " +more" : "",
            s_argv != NULL ? "" : " (restart unavailable)");
}

void start(void)
{
    if (s_argv != NULL)
        fprintf(stderr, "[xeno-port][soft-reset] START+SELECT+L1+R1: restarting pid=%d "
                        "(clean shutdown, then execv of /proc/self/exe)\n", (int)getpid());
    else
        fprintf(stderr, "[xeno-port][soft-reset] START+SELECT+L1+R1: restart unavailable "
                        "(boot state not saved); exiting\n");
    s_restart = 1;
    exit(0);
}

#else /* !__linux__ */

void PcPort_SoftResetInit(int argc, char** argv)
{
    (void)argc;
    (void)argv;
}

void start(void)
{
    fprintf(stderr, "[xeno-port][soft-reset] START+SELECT+L1+R1: restart is not implemented "
                    "on this platform; exiting\n");
    exit(0);
}

#endif
