/* Regression test for PcPort_BattleMipsDispatchCallback's main-executable path.
 *
 * The interpreted battle overlay's func_800B6438 (src/battle/mainc88.c) packs
 * the retail main-executable address 0x80025A88 (D_80025A88, func_80025A88)
 * into a work-list callback slot.  pc_port/src/work_list_port.c then asks
 * PcPort_BattleMipsDispatchCallback to invoke it.  Before the production fix
 * the dispatcher returned 0 for every callback outside the battle-overlay
 * guest range, so the work list aborted with
 *   [xeno-port][work-list] unresolved guest callback 0x80025a88
 * The fix routes main-executable callbacks through run_main_exe_callback:
 * bridge table first, then the func_%08X dlsym symbol, refusing generated
 * stubs and staying silent (0) for genuinely unknown addresses.
 *
 * Modelled on battle_guest_call_test.c: this file #includes the production
 * translation unit directly and supplies the external stubs it needs.  The
 * strong xeno_port_is_generated_stub override lives in a separate TU emitted
 * by run_battle_main_exe_callback_dispatch_test.sh because the production file
 * already carries a weak definition and one TU cannot define it twice.
 *
 * The test only prints "MAIN EXE CALLBACK PASS ..." once every assertion holds;
 * any failure prints "MAIN EXE CALLBACK FAIL <label> line=N" and exits 1.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psyq/libgpu.h"
#include "psyq/libgte.h"

#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

/* ---- external stubs for the production TU (see battle_guest_call_test.c) ---- */
uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
void CTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
int doCOP2(int op) { (void)op; abort(); }
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PsyX_UpdateInput(void) {}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
char PsyX_BeginScene(void) { return 1; }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }

/* Added to battle_mips_runtime.c after battle_guest_call_test.c was written;
 * the dispatcher never reaches it on this path, but the TU needs the symbol. */
void PcPort_GodModeBeforeGuest(PcPortMipsCpu *cpu, uint32_t target)
{
    (void)cpu;
    (void)target;
}

/* Strong xeno_port_is_generated_stub override (runner-emitted TU consults
 * these tables).  Production declares the symbol weak; the strong definition
 * in the other TU wins across translation units at O0/O2. */
const char *g_test_generated_stub_names[8];
unsigned g_test_generated_stub_count;

static void set_stub(const char *name)
{
    g_test_generated_stub_names[0] = name;
    g_test_generated_stub_count = 1;
}

static void clear_stubs(void)
{
    g_test_generated_stub_count = 0;
}

/* Retail callback stored by func_800B6438: D_80025A88 in
 * linker/undefined_syms_auto.battle.txt. */
#define MAIN_EXE_CALLBACK 0x80025a88u
/* No bridge entry and no func_80026FFC symbol exists: genuinely unknown. */
#define UNKNOWN_CALLBACK  0x80026ffcu

static unsigned checks;
static unsigned bridge_calls;
static unsigned dlsym_calls;
static unsigned stub_host_calls;
static unsigned canary_calls;
static uintptr_t bridge_last_arg;
static uintptr_t dlsym_last_arg;
static uintptr_t canary_seen_arg;

/* Host owner of 0x80025A88 when the bridge table carries an entry. */
static void host_bridge(void *argument)
{
    bridge_calls++;
    bridge_last_arg = (uintptr_t)argument;
}

/* Distinctive dlsym fallback owner for func_80025A88.  Global so that
 * dlsym(RTLD_DEFAULT, "func_80025A88") finds it; the runner links -rdynamic
 * and this test forces a reference so --gc-sections cannot drop it. */
void func_80025A88(void *argument)
{
    dlsym_calls++;
    dlsym_last_arg = (uintptr_t)argument;
}

/* Must never be reached while its name is reported as a generated stub. */
static void host_stub_refused(void *argument)
{
    (void)argument;
    stub_host_calls++;
}

typedef struct CanaryArgument {
    uint32_t guard_head;
    uint8_t payload[64];
    uint32_t guard_tail;
} CanaryArgument;

static CanaryArgument g_canary_sent;
static CanaryArgument g_canary_received;
static int g_canary_guards_ok;

static void host_canary(void *argument)
{
    CanaryArgument *canary = (CanaryArgument *)argument;
    canary_calls++;
    canary_seen_arg = (uintptr_t)argument;
    g_canary_received = *canary;
    g_canary_guards_ok = canary->guard_head == 0xA5A5A5A5u &&
                         canary->guard_tail == 0x5A5A5A5Au;
}

static BattleMipsRuntime rt;

#define CHECK(label, condition) do { checks++; if (!(condition)) { \
    fprintf(stderr, "MAIN EXE CALLBACK FAIL %s line=%d\n", (label), __LINE__); \
    return 0; \
} } while (0)

static void setup(void)
{
    memset(&rt, 0, sizeof(rt));
    rt.initialized = 1;
    g_ActiveBattleRuntime = &rt;
    bridge_calls = dlsym_calls = stub_host_calls = canary_calls = 0;
    bridge_last_arg = dlsym_last_arg = canary_seen_arg = 0;
    g_canary_guards_ok = 0;
    clear_stubs();
}

static void install_bridge_entry(uint32_t address, void *host, const char *name)
{
    rt.functions[0].address = address;
    rt.functions[0].host = host;
    rt.functions[0].name = name;
    rt.function_count = 1;
}

/* 1. Bridge-table hit: called exactly once, identical argument, returns 1,
 *    and the dlsym owner of the same name is NOT used. */
static int bridge_table_dispatch(void)
{
    uint32_t argument = 0x11223344u;

    setup();
    install_bridge_entry(MAIN_EXE_CALLBACK, (void *)host_bridge, "func_80025A88");

    CHECK("bridge-table callback returns 1",
          PcPort_BattleMipsDispatchCallback(MAIN_EXE_CALLBACK, &argument) == 1);
    CHECK("bridge-table host called exactly once", bridge_calls == 1);
    CHECK("bridge-table host got the identical argument pointer",
          bridge_last_arg == (uintptr_t)&argument);
    CHECK("bridge-table entry wins over the func_%08X dlsym owner",
          dlsym_calls == 0);
    CHECK("bridge-table scalar argument bytes unchanged", argument == 0x11223344u);
    return 1;
}

/* 2. No bridge entry: the func_%08X dlsym fallback runs the host owner. */
static int dlsym_fallback_dispatch(void)
{
    uint32_t argument = 0x55667788u;

    setup();
    rt.function_count = 0;

    CHECK("dlsym fallback callback returns 1",
          PcPort_BattleMipsDispatchCallback(MAIN_EXE_CALLBACK, &argument) == 1);
    CHECK("dlsym fallback func_80025A88 called exactly once", dlsym_calls == 1);
    CHECK("dlsym fallback host got the identical argument pointer",
          dlsym_last_arg == (uintptr_t)&argument);
    CHECK("dlsym fallback did not touch the bridge host", bridge_calls == 0);
    return 1;
}

/* 3a. Bridge entry whose name is a generated stub must be refused. */
static int bridge_stub_refused(void)
{
    uint32_t argument = 0xdeadbeefu;

    setup();
    set_stub("func_80026FFC");
    install_bridge_entry(UNKNOWN_CALLBACK, (void *)host_stub_refused,
                         "func_80026FFC");

    CHECK("generated-stub bridge entry returns 0",
          PcPort_BattleMipsDispatchCallback(UNKNOWN_CALLBACK, &argument) == 0);
    CHECK("generated-stub bridge host is not called", stub_host_calls == 0);
    return 1;
}

/* 3b. The dlsym fallback name is a generated stub and must be refused too. */
static int dlsym_stub_refused(void)
{
    uint32_t argument = 0x0badf00du;

    setup();
    set_stub("func_80025A88");
    rt.function_count = 0;

    CHECK("generated-stub dlsym fallback returns 0",
          PcPort_BattleMipsDispatchCallback(MAIN_EXE_CALLBACK, &argument) == 0);
    CHECK("generated-stub dlsym host is not called", dlsym_calls == 0);
    return 1;
}

/* 4. Unknown address: nothing resolves and the dispatcher returns 0, the value
 *    that makes pc_port/src/work_list_port.c report an unresolved callback. */
static int unknown_callback(void)
{
    uint32_t argument = 0x01234567u;

    setup();
    rt.function_count = 0;

    CHECK("unknown callback returns 0",
          PcPort_BattleMipsDispatchCallback(UNKNOWN_CALLBACK, &argument) == 0);
    CHECK("unknown callback calls no host", bridge_calls == 0 && dlsym_calls == 0);
    return 1;
}

/* 5. No active battle runtime: unchanged return 0. */
static int inactive_runtime(void)
{
    uint32_t argument = 0x89abcdefu;

    setup();
    rt.function_count = 0;
    g_ActiveBattleRuntime = NULL;

    CHECK("no active runtime returns 0",
          PcPort_BattleMipsDispatchCallback(MAIN_EXE_CALLBACK, &argument) == 0);
    CHECK("no active runtime calls no host", dlsym_calls == 0);
    g_ActiveBattleRuntime = &rt;
    return 1;
}

/* 6. The argument pointer is forwarded byte-for-byte with its guard bytes. */
static int canary_argument_passthrough(void)
{
    unsigned i;

    setup();
    install_bridge_entry(MAIN_EXE_CALLBACK, (void *)host_canary, "func_80025A88");
    memset(&g_canary_sent, 0, sizeof(g_canary_sent));
    memset(&g_canary_received, 0, sizeof(g_canary_received));
    g_canary_sent.guard_head = 0xA5A5A5A5u;
    g_canary_sent.guard_tail = 0x5A5A5A5Au;
    for (i = 0; i < sizeof(g_canary_sent.payload); i++)
        g_canary_sent.payload[i] = (uint8_t)(i * 7u + 3u);

    CHECK("canary callback returns 1",
          PcPort_BattleMipsDispatchCallback(MAIN_EXE_CALLBACK, &g_canary_sent) == 1);
    CHECK("canary host called exactly once", canary_calls == 1);
    CHECK("canary host received the exact argument pointer",
          canary_seen_arg == (uintptr_t)&g_canary_sent);
    CHECK("canary guard bytes were intact in the host",
          g_canary_guards_ok);
    CHECK("canary argument passed through byte-for-byte",
          memcmp(&g_canary_sent, &g_canary_received, sizeof(g_canary_sent)) == 0);
    CHECK("canary live object untouched after dispatch",
          g_canary_sent.guard_head == 0xA5A5A5A5u &&
          g_canary_sent.guard_tail == 0x5A5A5A5Au &&
          g_canary_sent.payload[0] == 3u &&
          g_canary_sent.payload[63] == (uint8_t)(63u * 7u + 3u));
    return 1;
}

int main(void)
{
    /* Keep the dlsym-only host reachable for --gc-sections. */
    void *volatile keep_func_80025A88 = (void *)func_80025A88;
    (void)keep_func_80025A88;

    if (!bridge_table_dispatch()) return 1;
    if (!dlsym_fallback_dispatch()) return 1;
    if (!bridge_stub_refused()) return 1;
    if (!dlsym_stub_refused()) return 1;
    if (!unknown_callback()) return 1;
    if (!inactive_runtime()) return 1;
    if (!canary_argument_passthrough()) return 1;

    printf("MAIN EXE CALLBACK PASS checks=%u "
           "bridge/dlsym/stub-refused/unknown/inactive/canary\n",
           checks);
    return 0;
}
