#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "../src/battle_mips_runtime.c"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

/* This suite tests marshaling through controlled SDK boundaries. Real GTE
 * execution is covered separately by battle_gte_retail_test.c. */
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int value, int reg) { (void)value; (void)reg; abort(); }
void CTC2(unsigned int value, int reg) { (void)value; (void)reg; abort(); }
int doCOP2(int op) { (void)op; abort(); }

static int expect_word(uint32_t address, uint32_t expected)
{
    uint32_t actual = load_le(PSX_ADDR(address), 4);
    if (actual != expected) {
        fprintf(stderr, "guest %08x: got %08x expected %08x\n", address, actual, expected);
        return 0;
    }
    return 1;
}

static int check_tim(int clut)
{
    const uint32_t input = 0x80180000u, output = 0x801f0000u;
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};
    uint32_t *tim = PSX_ADDR(input);
    unsigned pixel_block = clut ? 6 : 2;
    memset(tim, 0, 128);
    tim[0] = 0x10;
    tim[1] = clut ? 8 : 2;
    if (clut) {
        tim[2] = 16; tim[3] = 0x01e00000; tim[4] = 0x00010002; tim[5] = 0x22221111;
    }
    tim[pixel_block] = 16;
    tim[pixel_block + 1] = 0x01000200;
    tim[pixel_block + 2] = 0x00010002;
    tim[pixel_block + 3] = 0x44443333;
    memset(PSX_ADDR(output - 4), 0xa5, 64);
    OpenTIM((u_long *)tim);
    runtime.functions[0] = (ResolvedFunction){0x800471c4u, ReadTIM, "ReadTIM"};
    runtime.function_count = 1;
    cpu.gpr[4] = output;
    cpu.gpr[29] = 0x801ff000u;
    if (runtime_bridge(&runtime, &cpu, 0x800471c4u) != 1)
        return 0;
    if (!expect_word(output - 4, 0xa5a5a5a5) ||
        !expect_word(output + 20, 0xa5a5a5a5) ||
        !expect_word(output, clut ? 8 : 2) ||
        !expect_word(output + 4, clut ? input + 12 : 0) ||
        !expect_word(output + 8, clut ? input + 20 : 0) ||
        !expect_word(output + 12, input + (pixel_block + 1) * 4) ||
        !expect_word(output + 16, input + (pixel_block + 3) * 4))
        return 0;
    if (cpu.gpr[2] != output) {
        fprintf(stderr, "ReadTIM returned %08x instead of guest output %08x\n", cpu.gpr[2], output);
        return 0;
    }
    return 1;
}

static void native_offsets(long *x, long *y)
{
    *x = -123;
    *y = 567;
}

static int check_offsets(void)
{
    const uint32_t output = 0x801f0200u;
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};
    memset(PSX_ADDR(output - 4), 0xa5, 32);
    runtime.functions[0] = (ResolvedFunction){0x8004a0bcu, native_offsets, "ReadGeomOffset"};
    runtime.function_count = 1;
    cpu.gpr[4] = output;
    cpu.gpr[5] = output + 4;
    cpu.gpr[2] = 0xdeadbeefu;
    cpu.gpr[29] = 0x801ff000u;
    return runtime_bridge(&runtime, &cpu, 0x8004a0bcu) == 1 &&
        cpu.gpr[2] == 0xdeadbeefu &&
        expect_word(output - 4, 0xa5a5a5a5) &&
        expect_word(output, (uint32_t)-123) && expect_word(output + 4, 567) &&
        expect_word(output + 8, 0xa5a5a5a5);
}

static TIM_IMAGE *failed_tim(TIM_IMAGE *output)
{
    (void)output;
    return NULL;
}

static int check_failed_tim(void)
{
    const uint32_t output = 0x801f0000u;
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};
    memset(PSX_ADDR(output), 0xa5, 24);
    runtime.functions[0] = (ResolvedFunction){0x800471c4u, failed_tim, "ReadTIM"};
    runtime.function_count = 1;
    cpu.gpr[4] = output;
    cpu.gpr[29] = 0x801ff000u;
    if (runtime_bridge(&runtime, &cpu, 0x800471c4u) != 1 || cpu.gpr[2] != 0)
        return 0;
    for (unsigned i = 0; i < 6; i++)
        if (!expect_word(output + i * 4, 0xa5a5a5a5)) return 0;
    return 1;
}

static int native_project(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2,
                          long *xy0, long *xy1, long *xy2, long *p, long *flag)
{
    assert(v0 == PSX_ADDR(0x80180100u) && v1 == v0 + 1 && v2 == v0 + 2);
    *xy0 = 0x11223344;
    *xy1 = 0x55667788;
    *xy2 = (int32_t)0x99aabbccu;
    *p = 0x1000;
    *flag = (int32_t)0x80000001u;
    return 4096;
}

static int check_projection(void)
{
    const uint32_t out = 0x801f0400u, sp = 0x801ff000u;
    const uint32_t offsets[] = {0, 8, 16, 24, 28};
    const uint32_t values[] = {0x11223344, 0x55667788, 0x99aabbcc, 0x1000, 0x80000001};
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};
    memset(PSX_ADDR(out), 0xa5, 40);
    runtime.functions[0] = (ResolvedFunction){0x8004a67cu, native_project, "RotTransPers3"};
    runtime.function_count = 1;
    cpu.gpr[4] = 0x80180100u;
    cpu.gpr[5] = cpu.gpr[4] + 8;
    cpu.gpr[6] = cpu.gpr[4] + 16;
    cpu.gpr[7] = out;
    cpu.gpr[29] = sp;
    for (unsigned i = 1; i < 5; i++) store_le(PSX_ADDR(sp + 12 + i * 4), 4, out + offsets[i]);
    if (runtime_bridge(&runtime, &cpu, 0x8004a67cu) != 1 || cpu.gpr[2] != 4096) return 0;
    for (unsigned i = 0; i < 5; i++)
        if (!expect_word(out + offsets[i], values[i])) return 0;
    return expect_word(out + 4, 0xa5a5a5a5) && expect_word(out + 12, 0xa5a5a5a5) &&
        expect_word(out + 20, 0xa5a5a5a5) && expect_word(out + 32, 0xa5a5a5a5);
}

static int native_project1(SVECTOR *vertex, int *xy, long *p, long *flag)
{
    assert(vertex == PSX_ADDR(0x80180100u));
    *xy = 0x11223344;
    *p = 0x1000;
    *flag = (int32_t)0x80000001u;
    return 123;
}

static int native_project4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                           long *xy0, long *xy1, long *xy2, long *xy3, long *p, long *flag)
{
    assert(v0 == PSX_ADDR(0x80180100u) && v1 == v0 + 1 && v2 == v0 + 2 && v3 == v0 + 3);
    *xy0 = 0x11223344; *xy1 = 0x55667788;
    *xy2 = (int32_t)0x99aabbccu; *xy3 = (int32_t)0xddeeff00u;
    *p = 0x1000; *flag = (int32_t)0x80000001u;
    return 456;
}

static int check_other_projections(void)
{
    const uint32_t out = 0x801f0400u, sp = 0x801ff000u;
    const uint32_t expected[] = {0x11223344, 0x55667788, 0x99aabbcc, 0xddeeff00, 0x1000, 0x80000001};
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};
    memset(PSX_ADDR(out), 0xa5, 48);
    runtime.functions[0] = (ResolvedFunction){0x8004a64cu, native_project1, "RotTransPers"};
    runtime.function_count = 1;
    cpu.gpr[4] = 0x80180100u; cpu.gpr[5] = out; cpu.gpr[6] = out + 8; cpu.gpr[7] = out + 16;
    cpu.gpr[29] = sp;
    if (runtime_bridge(&runtime, &cpu, 0x8004a64cu) != 1 || cpu.gpr[2] != 123 ||
        !expect_word(out, 0x11223344) || !expect_word(out + 8, 0x1000) ||
        !expect_word(out + 16, 0x80000001) || !expect_word(out + 4, 0xa5a5a5a5) ||
        !expect_word(out + 12, 0xa5a5a5a5) || !expect_word(out + 20, 0xa5a5a5a5)) return 0;
    runtime.functions[0] = (ResolvedFunction){0x8004a73cu, native_project4, "RotTransPers4"};
    memset(PSX_ADDR(out), 0xa5, 48);
    for (unsigned i = 0; i < 4; i++) cpu.gpr[4 + i] = 0x80180100u + i * 8;
    for (unsigned i = 0; i < 6; i++) store_le(PSX_ADDR(sp + 16 + i * 4), 4, out + i * 8);
    if (runtime_bridge(&runtime, &cpu, 0x8004a73cu) != 1 || cpu.gpr[2] != 456) return 0;
    for (unsigned i = 0; i < 6; i++)
        if (!expect_word(out + i * 8, expected[i]) ||
            !expect_word(out + i * 8 + 4, 0xa5a5a5a5)) return 0;
    return 1;
}

/* Match the current native OT_TAG layout. The guest must never be passed to
 * this eight-byte-stride implementation: the retail DMA table is words. */
static u_long *native_clear_otag_r(u_long *ot, int count)
{
    OT_TAG *tags = (OT_TAG *)ot;
    if (count <= 0) return NULL;
    tags[0].addr = 0xffffffu;
    setlen(&tags[0], 0);
    for (int i = 1; i < count; i++) {
        tags[i].addr = (uintptr_t)&tags[i - 1] & 0xffffffu;
        setlen(&tags[i], 0);
    }
    return NULL;
}

static int check_retail_ot_clear(void)
{
    const uint32_t ot = 0x800c4a90u;
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};
    runtime.functions[0] = (ResolvedFunction){0x80044ad8u, native_clear_otag_r, "ClearOTagR"};
    runtime.function_count = 1;
    cpu.gpr[29] = 0x801ff000u;
    for (unsigned count = 0x1000; count != 0; count /= 2) {
        memset(PSX_ADDR(ot - 4), 0xa5, count * 8 + 16);
        cpu.gpr[4] = ot;
        cpu.gpr[5] = count;
        if (runtime_bridge(&runtime, &cpu, 0x80044ad8u) != 1 ||
            !expect_word(ot - 4, 0xa5a5a5a5) ||
            !expect_word(ot + count * 4, 0xa5a5a5a5) ||
            !expect_word(ot, 0x0005698cu) || cpu.gpr[2] != ot) return 0;
        for (unsigned i = 1; i < count; i++)
            if (!expect_word(ot + i * 4, (ot + (i - 1) * 4) & 0xffffffu)) return 0;
    }
    return 1;
}

int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
/* compat.o's real pump is the sole definition; with a frozen vblank count it
 * early-returns before touching pad state (a conflicting no-op stub here
 * breaks the link with multiple-definition). */
int PsyX_Sys_GetVBlankCount(void) { return 0; }
void PsyX_UpdateInput(void) {}
void ControllerPoll(void) {}
void ControllerPushState(void) {}
static int draw_begins, draw_clears, draw_flushes, draw_packets, raw_ot_calls;
static uint32_t captured_packets[4][256];

char PsyX_BeginScene(void) { draw_begins++; return 1; }
void ClearSplits(void) { draw_clears++; }
void DrawAllSplits(void) { draw_flushes++; }
void ParsePrimitivesLinkedList(u_long *packet, int single)
{
    uint32_t tag = load_le((uint8_t *)packet, 4);
    assert(single == 0 && draw_packets < 4);
    memcpy(captured_packets[draw_packets++], packet, ((tag >> 24) + 1) * 4);
}
static void native_draw_otag(u_long *ot) { (void)ot; raw_ot_calls++; }

static int check_retail_ot_draw(void)
{
    const uint32_t ot = 0x800c4a90u, prim = 0x80180000u, sentinel = 0x8005698cu;
    const uint32_t merged[] = {0x04000000, 0x20112233, 0x00010002, 0x00030004, 0x00050006,
                               0, 0x20556677, 0x00070008, 0x0009000a, 0x000b000c};
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu = {0};
    uint32_t original[16];
    runtime.host_ranges[0] = (HostRange){(uintptr_t)g_PsxRam, (uintptr_t)g_PsxRam + PSX_RAM_SIZE, 1};
    runtime.host_range_count = 1;
    runtime.functions[0] = (ResolvedFunction){0x80044bd0u, native_draw_otag, "DrawOTag"};
    runtime.function_count = 1;
    cpu.gpr[29] = 0x801ff000u;
    cpu.gpr[4] = ot;
    cpu.gpr[5] = 4;
    if (bridge_clear_otag_r(&runtime, &cpu) != 1) return 0;
    memset(PSX_ADDR(sentinel), 0, 20);
    store_le(PSX_ADDR(sentinel), 4, 0x04ffffff);
    memset(PSX_ADDR(prim), 0xa5, sizeof(original));
    memcpy(PSX_ADDR(prim), merged, sizeof(merged));
    store_le(PSX_ADDR(prim), 4, 0x09000000u | (ot & 0xffffffu));
    store_le(PSX_ADDR(ot + 4), 4, prim & 0xffffffu);
    /* An SDK-returned host pointer may also appear in an inline guest tag.
     * It is unambiguous here: native RAM mapping does not overlap PSX RAM. */
    assert((uintptr_t)PSX_ADDR(prim + 40) > 0x200000u && (uintptr_t)PSX_ADDR(prim + 40) < 0x1000000u);
    store_le(PSX_ADDR(ot + 8), 4, (uint32_t)(uintptr_t)PSX_ADDR(prim + 40));
    store_le(PSX_ADDR(prim + 40), 4, 0x01000000u | ((ot + 4) & 0xffffffu));
    store_le(PSX_ADDR(prim + 44), 4, 0xe1000123);
    memcpy(original, PSX_ADDR(prim), sizeof(original));
    cpu.gpr[4] = ot + 12;
    cpu.gpr[2] = 0x12345678;
    if (runtime_bridge(&runtime, &cpu, 0x80044bd0u) != 1 || raw_ot_calls != 0 ||
        draw_begins != 1 || draw_clears != 1 || draw_flushes != 1 || draw_packets != 2 ||
        cpu.gpr[2] != 0x12345678 || memcmp(original, PSX_ADDR(prim), sizeof(original)) != 0) {
        fprintf(stderr, "DMA draw mismatch raw=%d begin=%d clear=%d flush=%d packets=%d\n",
                raw_ot_calls, draw_begins, draw_clears, draw_flushes, draw_packets);
        return 0;
    }
    if (captured_packets[0][0] != 0x01ffffff || captured_packets[0][1] != 0xe1000123 ||
        captured_packets[1][0] != 0x09ffffff ||
        memcmp(captured_packets[1] + 1, merged + 1, sizeof(merged) - 4) != 0) return 0;
    return expect_word(sentinel, 0x04ffffff) && expect_word(ot, 0x0005698c);
}

static int check_dma_boundaries(void)
{
    const uint32_t guest = 0x801f0800u;
    PcPortMipsCpu cpu = {0};
    uint32_t addresses[] = {guest, guest + 0x20000000u, guest & 0xffffffu,
                            (uint32_t)(uintptr_t)PSX_ADDR(guest)};
    for (unsigned i = 0; i < sizeof(addresses) / sizeof(addresses[0]); i++) {
        if (dma_packet_memory(addresses[i], 4) != PSX_ADDR(guest) ||
            dma_packet_memory(addresses[i] + 1, 4) != NULL) return 0;
        store_le(PSX_ADDR(guest), 4, 0xffffffu);
        cpu.gpr[4] = addresses[i];
        draw_flushes = draw_packets = 0;
        if (bridge_draw_otag(&cpu) != 1 || draw_flushes != 1 || draw_packets != 0) return 0;
    }
    if (dma_packet_memory(0x80200000u, 4) != NULL ||
        dma_packet_memory(0x801ffffcu, 8) != NULL) return 0;
    g_GPUDisabledState = 1;
    draw_begins = draw_clears = draw_flushes = 0;
    cpu.gpr[4] = 0xdeadbeef;
    if (bridge_draw_otag(&cpu) != 1 || draw_begins != 0 || draw_clears != 1 || draw_flushes != 0) return 0;
    g_GPUDisabledState = 0;
    if (bridge_draw_otag(&cpu) != -1 || draw_flushes != 0) return 0;
    /* Non-terminating and truncated chains must fail, never pretend success. */
    cpu.gpr[4] = guest;
    store_le(PSX_ADDR(guest), 4, guest & 0xffffffu);
    if (bridge_draw_otag(&cpu) != -1 || draw_flushes != 0) return 0;
    cpu.gpr[4] = 0x801ffffcu;
    store_le(PSX_ADDR(cpu.gpr[4]), 4, 0x01ffffffu);
    return bridge_draw_otag(&cpu) == -1 && draw_flushes == 0;
}

static unsigned callback_depth, callback_limit;

static uintptr_t nested_callback_driver(void)
{
    if (callback_depth < callback_limit) {
        callback_depth++;
        assert(PcPort_BattleMipsDispatchCallback(0x80080100u, NULL) == 1);
        callback_depth--;
    }
    return 0;
}

/* Synthetic MIPS prologues deliberately save a word at SP(top)-8 in each
 * frame. This is an adapter stack-isolation test, not retail scene content. */
static void write_callback_fixture(uint32_t pc, unsigned frame, uint16_t marker)
{
    const uint32_t instructions[] = {
        0x27bd0000u | ((0x10000u - frame) & 0xffffu), /* addiu sp,-frame */
        0xafbf0000u | (frame - 4),                  /* sw ra,frame-4(sp) */
        0x24080000u | marker,                      /* li t0,marker */
        0xafa80000u | (frame - 8),                  /* sw t0,frame-8(sp) */
        0x0c004440u, 0,                            /* jal native 80011100 */
        0x8fa80000u | (frame - 8),                  /* lw t0,frame-8(sp) */
        0x3c01801fu, 0xac281000u,                   /* sw t0,801f1000 */
        0x8fbf0000u | (frame - 4), 0,
        0x27bd0000u | frame, 0x03e00008u, 0,
    };
    memcpy(PSX_ADDR(pc), instructions, sizeof(instructions));
}

static int check_nested_callback_stack(void)
{
    const unsigned frames[] = {0x20, 0x50, 0xa0};
    unsigned cases = 0;
    for (unsigned parent = 0; parent < 3; parent++)
        for (unsigned child = 0; child < 3; child++)
            for (callback_limit = 0; callback_limit <= 4; callback_limit++, cases++) {
                BattleMipsRuntime runtime = {0};
                PcPortMipsCpu cpu;
                runtime.functions[0] = (ResolvedFunction){0x80011100u, nested_callback_driver, "test_nested_callback"};
                runtime.function_count = 1;
                write_callback_fixture(0x80080000u, frames[parent], 0x1234);
                write_callback_fixture(0x80080100u, frames[child], 0x5678);
                memset(PSX_ADDR(BATTLE_STACK_TOP - 0x800), 0xa5, 0x804);
                initialize_cpu(&cpu, &runtime);
                g_ActiveBattleRuntime = &runtime;
                callback_depth = 0;
                int rc = PcPortMipsRun(&cpu, 0x80080000u, BATTLE_HALT_PC, 1000);
                g_ActiveBattleRuntime = NULL;
                if (rc != PC_PORT_MIPS_HALTED || runtime.bridge_cpu != NULL ||
                    cpu.gpr[29] != BATTLE_STACK_TOP ||
                    !expect_word(0x801f1000u, 0x1234) ||
                    !expect_word(BATTLE_STACK_TOP, 0xa5a5a5a5)) {
                    fprintf(stderr, "BATTLE CALLBACK STACK FAIL parent=%u child=%u depth=%u: %s\n",
                            frames[parent], frames[child], callback_limit, cpu.error);
                    return 0;
                }
                for (unsigned depth = 0; depth < callback_limit; depth++)
                    if (!expect_word(BATTLE_STACK_TOP - frames[parent] - depth * frames[child] - 8, 0x5678))
                        return 0;
            }
    printf("BATTLE CALLBACK STACK PASS cases=%u, nested frames/return/SP/guards\n", cases);
    return 1;
}

static unsigned host_callback_calls;
static uintptr_t known_host_callback(uintptr_t a0, uintptr_t a1, uintptr_t a2,
    uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6, uintptr_t a7,
    uintptr_t a8, uintptr_t a9, uintptr_t a10, uintptr_t a11)
{
    (void)a1; (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    (void)a7; (void)a8; (void)a9; (void)a10; (void)a11;
    assert(a0 == (uintptr_t)PSX_ADDR(0x80180000u));
    host_callback_calls++;
    return 0x12345678u;
}

static int check_registered_host_callback(void)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu;
    uintptr_t host = (uintptr_t)known_host_callback;
    assert(host <= UINT32_MAX);
    runtime.functions[0] = (ResolvedFunction){0x80011100u, known_host_callback,
                                             "test_known_host_callback"};
    runtime.function_count = 1;
    initialize_cpu(&cpu, &runtime);
    cpu.gpr[4] = 0x80180000u;
    cpu.gpr[29] = 0x801ff000u;
    host_callback_calls = 0;
    if (runtime_bridge(&runtime, &cpu, (uint32_t)host) != 1 ||
        host_callback_calls != 1 || cpu.gpr[2] != 0x12345678u) {
        fputs("REGISTERED HOST CALLBACK FAIL\n", stderr);
        return 0;
    }
    /* Unknown and truncated pointers must never become callable merely
     * because the address lies in an executable host mapping.  Use a fresh
     * runtime: the bridge caches each resolved target, and the port's
     * function table never changes after start-up. */
    BattleMipsRuntime truncated = {0};
    truncated.functions[0] = (ResolvedFunction){0x80011100u,
        (void *)(host | (UINT64_C(1) << 32)), "test_known_host_callback"};
    truncated.function_count = 1;
    initialize_cpu(&cpu, &truncated);
    cpu.gpr[4] = 0x80180000u;
    cpu.gpr[29] = 0x801ff000u;
    if (runtime_bridge(&truncated, &cpu, (uint32_t)host) != -1) {
        fputs("REGISTERED HOST CALLBACK FAIL truncated-pointer was accepted\n",
              stderr);
        return 0;
    }
    BattleMipsRuntime unknown = {0};
    initialize_cpu(&cpu, &unknown);
    cpu.gpr[4] = 0x80180000u;
    cpu.gpr[29] = 0x801ff000u;
    if (runtime_bridge(&unknown, &cpu, (uint32_t)host) != -1 ||
        host_callback_calls != 1) {
        fputs("REGISTERED HOST CALLBACK FAIL unknown-function was accepted\n",
              stderr);
        return 0;
    }
    puts("REGISTERED HOST CALLBACK PASS exact/unknown/truncated");
    return 1;
}

static uintptr_t lookup_owner_arg, lookup_callback_arg;
static uintptr_t lookup_boundary(uintptr_t owner, uintptr_t callback,
    uintptr_t a2, uintptr_t a3, uintptr_t a4, uintptr_t a5, uintptr_t a6,
    uintptr_t a7, uintptr_t a8, uintptr_t a9, uintptr_t a10, uintptr_t a11)
{
    (void)a2; (void)a3; (void)a4; (void)a5; (void)a6;
    (void)a7; (void)a8; (void)a9; (void)a10; (void)a11;
    lookup_owner_arg = owner;
    lookup_callback_arg = callback;
    return (uintptr_t)PSX_ADDR(0x80180100u);
}

static int check_lookup_callback_argument(void)
{
    const uint32_t callbacks[] = {0, 0x800bcbb4u, 0x800bcb54u, 0x00412340u, 0x800bcfacu};
    BattleMipsRuntime runtime = {0};
    runtime.functions[0] = (ResolvedFunction){0x8001d0a4u, lookup_boundary, "func_8001D0A4"};
    runtime.function_count = 1;
    for (unsigned n = 0; n < sizeof(callbacks) / sizeof(*callbacks); n++) {
        PcPortMipsCpu cpu;
        initialize_cpu(&cpu, &runtime);
        cpu.gpr[4] = 0x80180000u;
        cpu.gpr[5] = callbacks[n];
        cpu.gpr[29] = 0x801ff000u;
        if (runtime_bridge(&runtime, &cpu, 0x8001d0a4u) != 1 ||
            lookup_owner_arg != (uintptr_t)PSX_ADDR(0x80180000u) ||
            lookup_callback_arg != callbacks[n] ||
            cpu.gpr[2] != (uintptr_t)PSX_ADDR(0x80180100u)) {
            fprintf(stderr, "TIMER LOOKUP ABI FAIL callback=%08x received=%lx\n",
                callbacks[n], (unsigned long)lookup_callback_arg);
            return 0;
        }
    }
    runtime.functions[0] = (ResolvedFunction){0x8001d164u, lookup_boundary, "func_8001D164"};
    for (unsigned n = 0; n < sizeof(callbacks) / sizeof(*callbacks); n++) {
        PcPortMipsCpu cpu;
        initialize_cpu(&cpu, &runtime);
        cpu.gpr[4] = callbacks[n];
        cpu.gpr[29] = 0x801ff000u;
        if (runtime_bridge(&runtime, &cpu, 0x8001d164u) != 1 ||
            lookup_owner_arg != callbacks[n] ||
            cpu.gpr[2] != (uintptr_t)PSX_ADDR(0x80180100u)) {
            fprintf(stderr, "TIMER CALLBACK LOOKUP ABI FAIL callback=%08x received=%lx\n",
                    callbacks[n], (unsigned long)lookup_owner_arg);
            return 0;
        }
    }
    puts("TIMER LOOKUP ABI PASS both lookups preserve callback identity/result");
    return 1;
}

static int check_battle_sprite_reentry(void)
{
    FILE* file = fopen("disc/battle.bin", "rb");
    assert(file);
    assert(fread(PSX_ADDR(BATTLE_BASE), 1, 0x53f80, file) == 0x53f80);
    assert(fclose(file) == 0);
    BattleMipsRuntime runtime = {0};
    runtime.host_ranges[0] = (HostRange){(uintptr_t)g_PsxRam,
        (uintptr_t)g_PsxRam + sizeof(g_PsxRam), 1};
    runtime.host_range_count = 1;
    uint8_t* sprite = PSX_ADDR(0x80180000u);
    uint8_t expected[256];
    for (unsigned i = 0; i < 3; ++i) {
        memset(sprite, 0, 256);
        store_le(sprite + 0x64, 4, 0x80181000u);
        store_le(sprite + 0xac, 4, 256u << 7);
        store_le(sprite + 0x9e, 2, i == 0 ? 5 : 0);
        *(uint8_t*)PSX_ADDR(0x80181000u) = i == 2 ? 0x3f : 0x30;
        PcPortMipsCpu cpu;
        initialize_cpu(&cpu, &runtime);
        cpu.gpr[4] = 0x80180000u;
        assert(PcPortMipsRun(&cpu, 0x800c11ccu, BATTLE_HALT_PC,
                            CALLBACK_STEP_LIMIT) == PC_PORT_MIPS_HALTED);
        memcpy(expected, sprite, sizeof(expected));
        assert(load_le(expected + 0x9e, 2) == (i == 0 ? 5u : i == 2 ? 16u : 1u));
        assert(load_le(expected + 0x64, 4) == 0x80181000u + (i != 0));
        memset(sprite, 0, 256);
        store_le(sprite + 0x64, 4, 0x80181000u);
        store_le(sprite + 0xac, 4, 256u << 7);
        store_le(sprite + 0x9e, 2, i == 0 ? 5 : 0);
        g_ActiveBattleRuntime = &runtime;
        func_800C11CC(sprite);
        g_ActiveBattleRuntime = NULL;
        if (memcmp(sprite, expected, sizeof(expected))) {
            fprintf(stderr,"BATTLE SPRITE REENTRY FAIL case=%u\n",i);
            return 0;
        }
    }
    puts("BATTLE SPRITE REENTRY PASS retail wait/nonzero-timer/duration paths");
    return 1;
}

/* Name the failing check.  This used to be one long `||` chain returning a
 * bare 1, so a failure printed NOTHING -- and because two earlier checks
 * print their own PASS lines first, the run looked like a pass that had
 * merely been cut short.  See docs/ai_context/HARNESS_REGIMES.md. */
#define REQUIRE(expr)                                                        \
    do {                                                                     \
        if (!(expr)) {                                                       \
            fprintf(stderr,                                                  \
                    "BATTLE GRAPHICS ABI FAIL check=%s (%s:%d)\n",           \
                    #expr, __FILE__, __LINE__);                              \
            return 1;                                                        \
        }                                                                    \
    } while (0)

int main(void)
{
    REQUIRE(check_battle_sprite_reentry());
    REQUIRE(check_lookup_callback_argument());
    REQUIRE(check_registered_host_callback());
    REQUIRE(check_nested_callback_stack());
    REQUIRE(check_tim(1));
    REQUIRE(check_tim(0));
    REQUIRE(check_failed_tim());
    REQUIRE(check_offsets());
    REQUIRE(check_projection());
    REQUIRE(check_other_projections());
    REQUIRE(check_retail_ot_clear());
    REQUIRE(check_retail_ot_draw());
    REQUIRE(check_dma_boundaries());
    puts("BATTLE GRAPHICS ABI PASS TIM, offsets, RTP1/3/4, OT stride/guards, DMA domains/order/payloads/rejections");
    return 0;
}
