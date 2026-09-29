#include "psyq/libgpu.h"
#include "psyq/libgte.h"

#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; abort(); }
void CTC2(unsigned int value, int reg) { (void)value; (void)reg; abort(); }
int doCOP2(int op) { (void)op; abort(); }

/* These symbols are reachable from runtime_bridge_call through the generic
 * dispatcher's other ABI branches. The constructor test never exercises them. */
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames; /* printf-only OT diag counter (runtime) */
/* compat.o's real pad pump (kept via the oracle bridge) early-returns on a
 * frozen vblank count before touching pad state. */
int PsyX_Sys_GetVBlankCount(void) { return 0; }
void PsyX_UpdateInput(void) {}
void ControllerPoll(void) {}
void ControllerPushState(void) {}
char PsyX_BeginScene(void) { return 1; }
void ClearSplits(void) {}
void DrawAllSplits(void) {}
void ParsePrimitivesLinkedList(u_long *packet, int single)
{
    (void)packet;
    (void)single;
}

static unsigned spy_calls;
static void *spy_window;
static int32_t spy_args[7];

/* Deliberately typed to the native constructor's eight-argument ABI. The
 * runtime's generic call site supplies the established twelve-word envelope;
 * UBSan's function-call check is disabled by the runner for this boundary. */
static uintptr_t window_constructor_spy(void *window, int32_t tpage_x,
                                        int32_t tpage_y, int32_t x,
                                        int32_t y, int32_t width,
                                        int32_t mode, int32_t height)
{
    spy_window = window;
    spy_args[0] = tpage_x;
    spy_args[1] = tpage_y;
    spy_args[2] = x;
    spy_args[3] = y;
    spy_args[4] = width;
    spy_args[5] = mode;
    spy_args[6] = height;
    spy_calls++;
    return (uintptr_t)0x1234abcdu;
}

static int expect(const char *label, int condition)
{
    if (!condition)
        fprintf(stderr, "BATTLE WINDOW ABI FAIL %s\n", label);
    return condition;
}

static void set_stack_words(uint32_t sp, uint32_t arg4, uint32_t arg5,
                            uint32_t raw_arg6, uint32_t poison_arg7)
{
    store_le(PSX_ADDR(sp + 0x10u), 4, arg4);
    store_le(PSX_ADDR(sp + 0x14u), 4, arg5);
    store_le(PSX_ADDR(sp + 0x18u), 4, raw_arg6);
    store_le(PSX_ADDR(sp + 0x1cu), 4, poison_arg7);
}

static int invoke_window(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu,
                         uint32_t sp, uint32_t raw_arg6)
{
    const uint32_t window = 0x801e5000u;

    memset(PSX_ADDR(sp), 0xa5, 0x40);
    set_stack_words(sp, 24, 24, raw_arg6, 0xdeadbeefu);
    cpu->gpr[4] = window;
    cpu->gpr[5] = 896;
    cpu->gpr[6] = 256;
    cpu->gpr[7] = 28;
    cpu->gpr[29] = sp;
    cpu->gpr[2] = 0;
    return runtime_bridge_call(runtime, cpu, 0x80032f54u);
}

static int check_observed_tuple(void)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};
    const uint32_t sp = 0x801ff000u;

    runtime.functions[0] = (ResolvedFunction){
        0x80032f54u, window_constructor_spy, "func_80032F54"};
    runtime.function_count = 1;
    spy_calls = 0;
    if (!expect("observed dispatch", invoke_window(&runtime, &cpu, sp, 4) == 1) ||
        !expect("observed one call", spy_calls == 1) ||
        !expect("observed return", cpu.gpr[2] == 0x1234abcdu) ||
        !expect("window pointer domain", spy_window == PSX_ADDR(0x801e5000u)) ||
        !expect("tpage x", spy_args[0] == 896) ||
        !expect("tpage y", spy_args[1] == 256) ||
        !expect("window x", spy_args[2] == 28) ||
        !expect("window y", spy_args[3] == 24) ||
        !expect("window width", spy_args[4] == 24) ||
        !expect("native dead mode", spy_args[5] == 0) ||
        !expect("height from guest seventh", spy_args[6] == 4))
        return 0;
    puts("BATTLE WINDOW ABI observed tuple PASS window/896/256/28/24/24/mode0/height4");
    return 1;
}

static int check_signed_low16_and_poison(void)
{
    static const uint32_t raw_values[] = {
        0x80000004u, 0x00008000u, 0x0000ffffu, 0xffff0004u};
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};
    const uint32_t sp = 0x801ff000u;

    runtime.functions[0] = (ResolvedFunction){
        0x80032f54u, window_constructor_spy, "func_80032F54"};
    runtime.function_count = 1;
    for (unsigned i = 0; i < sizeof(raw_values) / sizeof(raw_values[0]); i++) {
        spy_calls = 0;
        int32_t expected_height = (int16_t)(raw_values[i] & 0xffffu);
        if (!expect("highbit dispatch", invoke_window(&runtime, &cpu, sp, raw_values[i]) == 1) ||
            !expect("highbit one call", spy_calls == 1) ||
            !expect("highbit dead mode", spy_args[5] == 0) ||
            !expect("signed low16 height", spy_args[6] == expected_height) ||
            !expect("eighth-slot poison ignored", spy_args[6] != (int32_t)0xdeadbeef))
            return 0;
    }
    puts("BATTLE WINDOW ABI signed-low16 PASS highbit scalar/eighth-slot poison");
    return 1;
}

static int check_failed_stack_read(void)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};

    runtime.functions[0] = (ResolvedFunction){
        0x80032f54u, window_constructor_spy, "func_80032F54"};
    runtime.function_count = 1;
    spy_calls = 0;
    cpu.gpr[4] = 0x801e5000u;
    cpu.gpr[5] = 896;
    cpu.gpr[6] = 256;
    cpu.gpr[7] = 28;
    cpu.gpr[29] = 0x90000000u;
    if (!expect("bad seventh read fails closed",
                runtime_bridge_call(&runtime, &cpu, 0x80032f54u) == -1) ||
        !expect("bad seventh read does not invoke spy", spy_calls == 0))
        return 0;
    puts("BATTLE WINDOW ABI bad-read PASS fail-closed/no invocation");
    return 1;
}

static int check_unrelated_target(void)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};

    runtime.functions[0] = (ResolvedFunction){
        0x80032f50u, window_constructor_spy, "func_80032F50"};
    runtime.function_count = 1;
    spy_calls = 0;
    cpu.gpr[4] = 0x801e5000u;
    cpu.gpr[29] = 0x801ff000u;
    set_stack_words(cpu.gpr[29], 1, 2, 3, 4);
    if (!expect("unrelated target dispatch",
                runtime_bridge_call(&runtime, &cpu, 0x80032f50u) == 1) ||
        !expect("unrelated target invokes once", spy_calls == 1) ||
        !expect("unrelated target keeps generic stack arg", spy_args[5] == 3) ||
        !expect("unrelated target keeps eighth stack arg", spy_args[6] == 4))
        return 0;
    puts("BATTLE WINDOW ABI target scope PASS unrelated target unchanged");
    return 1;
}

int main(void)
{
    if (!check_observed_tuple() || !check_signed_low16_and_poison() ||
        !check_failed_stack_read() || !check_unrelated_target())
        return 1;
    puts("BATTLE WINDOW ABI PASS typed8arg/guest-pointer/low16/fail-closed/scope");
    return 0;
}
