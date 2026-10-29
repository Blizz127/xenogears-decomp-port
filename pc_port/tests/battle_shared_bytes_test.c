/* Shared single-byte battle bindings (on the native line the generated map's
 * exact entries; on the release line battle_mips_runtime.c
 * initialize_shared_byte_data), checked with the production runtime source
 * and the production objects that define the native globals:
 *
 *  1. each shared retail byte resolves through find_data to exactly &D_xxx
 *     (not into a wider symbol's host object), and a guest store through the
 *     production bus (runtime_write) reaches that native byte.  A dlsym miss
 *     fails here instead of being skipped;
 *  2. the field flag: after a guest store of 1 to 0x8005942C, misc4.c's field
 *     entry (func_80078D44, the production object) takes its D_8005942C == 1
 *     branches -- func_800A5884(0, 0) and the 33-step zoom-in
 *     func_800A5600(0x80 .. 0) -- and with 0 the != 1 ones: func_800A5884(1, 1)
 *     and the tail's 32-step fade (0x80 .. 4).
 *
 * The 0x80062514 end-frame word the movie request also writes has its own
 * map entry; it is checked the same way.
 *
 * usage: battle_shared_bytes_test   (run_battle_shared_bytes_test.sh) */
#include <dlfcn.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

/* misc4.c's field entry (production object) and the two callees whose
 * arguments tell the branch apart. */
extern void func_80078D44(void);
static int a5884_calls, a5884_first[2], a5600_calls, a5600_last;
void func_800A5884(int32_t a, int32_t b);
void func_800A5884(int32_t a, int32_t b)
{
    if (a5884_calls++ == 0) {
        a5884_first[0] = a;
        a5884_first[1] = b;
    }
}
void func_800A5600(int32_t level);
void func_800A5600(int32_t level)
{
    a5600_last = level;
    a5600_calls++;
}

static sigjmp_buf fault_jmp;
static volatile sig_atomic_t fault_armed;
static void on_fault(int sig)
{
    if (fault_armed) {
        fault_armed = 0;
        siglongjmp(fault_jmp, 1);
    }
    signal(sig, SIG_DFL);
    raise(sig);
}

/* Runs the field entry; 0 when it returned, 1 when it faulted after the
 * branches (the rest of the routine needs live field state). */
static int run_field_entry(void)
{
    a5884_calls = a5600_calls = 0;
    if (sigsetjmp(fault_jmp, 1) != 0)
        return 1;
    fault_armed = 1;
    func_80078D44();
    fault_armed = 0;
    return 0;
}

int main(void)
{
    static const struct { uint32_t address; unsigned width; const char *name; } k[] = {
        {0x800594d4u, 1, "D_800594D4"}, {0x800594d5u, 1, "D_800594D5"},
        {0x800594d6u, 1, "D_800594D6"}, {0x8004fe44u, 1, "D_8004FE44"},
        {0x8004fe45u, 1, "D_8004FE45"}, {0x8004fe46u, 1, "D_8004FE46"},
        {0x8004fe47u, 1, "D_8004FE47"}, {0x8005942cu, 1, "D_8005942C"},
        {0x80062514u, 2, "D_80062514"},
    };
    BattleMipsRuntime *rt = calloc(1, sizeof(*rt));
    unsigned i, failures = 0;
    int faulted;

    if (rt == NULL)
        return 2;
    g_ActiveBattleRuntime = rt;
    initialize_runtime(rt);
    for (i = 0; i < sizeof(k) / sizeof(k[0]); i++) {
        uint8_t *want = dlsym(RTLD_DEFAULT, k[i].name);
        ResolvedData *e;
        uint32_t value = k[i].address == 0x8005942cu ? 1u : (k[i].width == 2 ? 0x0123u + i : 0x50u + i);
        if (want == NULL) {
            printf("FAIL 0x%08x: no native %s (a dlsym miss would leave it unbound)\n",
                   k[i].address, k[i].name);
            failures++;
            continue;
        }
        e = find_data(rt, k[i].address, k[i].width);
        if (e == NULL || e->host + (k[i].address - e->address) != want) {
            printf("FAIL 0x%08x resolves to %s+0x%x, not &%s\n", k[i].address,
                   e && e->name ? e->name : "guest RAM", e ? k[i].address - e->address : 0u, k[i].name);
            failures++;
            continue;
        }
        memset(want, 0, k[i].width);
        if (runtime_write(rt, k[i].address, k[i].width, value) != 0 ||
            memcmp(want, &value, k[i].width) != 0) {
            printf("FAIL guest store to 0x%08x did not reach %s\n", k[i].address, k[i].name);
            failures++;
            continue;
        }
        printf("ok   0x%08x -> &%s (binding %s, %u byte%s); guest store 0x%x reached it\n",
               k[i].address, k[i].name, e->name, e->size, e->size == 1 ? "" : "s", value);
    }

    /* The field flag: D_8005942C now holds the guest store of 1. */
    signal(SIGSEGV, on_fault);
    signal(SIGBUS, on_fault);
    for (i = 0; i < 2; i++) {
        unsigned flag = *(uint8_t *)dlsym(RTLD_DEFAULT, "D_8005942C");
        int want_a = flag == 1 ? 0 : 1, want_calls = flag == 1 ? 33 : 32, want_last = flag == 1 ? 0 : 4;
        int ok;
        faulted = run_field_entry();
        ok = a5884_calls && a5884_first[0] == want_a && a5884_first[1] == want_a &&
             a5600_calls == want_calls && a5600_last == want_last;
        printf("%s field entry with D_8005942C=%u (%s branch): func_800A5884(%d, %d), "
               "func_800A5600 x%d ending at %d%s\n", ok ? "ok  " : "FAIL", flag,
               flag == 1 ? "== 1" : "!= 1", a5884_first[0], a5884_first[1], a5600_calls,
               a5600_last, faulted ? " (stopped at a later fault)" : "");
        if (!ok || (i == 0 && flag != 1))
            failures++;
        runtime_write(rt, 0x8005942cu, 1, 0);   /* second pass: the != 1 branch */
    }
    printf("BATTLE SHARED BYTES %s (%u failure%s)\n", failures ? "FAIL" : "PASS", failures,
           failures == 1 ? "" : "s");
    return failures ? 1 : 0;
}
