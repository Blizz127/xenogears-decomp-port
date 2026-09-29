/* State-1 command handler versus retail bytes. Callees are controlled test
 * boundaries (trace and optional context replacement), not combat execution. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void func_8008115C(uint32_t) __attribute__((weak));
struct BattleCommandContext;
struct BattleCommandContext *D_800C3EAC;
uint8_t D_800D3014, D_800D366C, D_800C3E29;
enum { CONTEXT = 0x100000, CONTEXT_SIZE = 0x4100 };
static uint8_t ram[0x200000], initial[CONTEXT_SIZE];
static _Alignas(4) uint8_t native_context[CONTEXT_SIZE];
static _Alignas(4) uint8_t replacement_context[CONTEXT_SIZE];
enum { REPLACEMENT = 0x110000 };
static uint32_t calls[4], arguments[4];
static unsigned call_count, rebind_at, native_phase;
static void fail(const char *why) {
    fprintf(stderr, "ATTACK RING FAIL %s\n", why); exit(1);
}
static void record(uint32_t target, uint32_t arg) {
    if (call_count == 4) fail("excess calls");
    calls[call_count] = target; arguments[call_count++] = arg;
    if (rebind_at && call_count == rebind_at) {
        if (native_phase) D_800C3EAC = (struct BattleCommandContext *)replacement_context;
        else {
            const uint32_t pointer = 0x80000000u + REPLACEMENT;
            for (unsigned i = 0; i < 4; ++i)
                ram[0xc3eac+i] = (uint8_t)(pointer >> (8*i));
        }
    }
}
void func_80087A38(uint32_t a) { record(0x80087a38, a); }
void func_80084A7C(uint32_t a) { record(0x80084a7c, a); }
void func_80077698(void) { record(0x80077698, 0); }
void func_8008AA74(uint32_t a) { record(0x8008aa74, a); }
static int read_bus(void *unused, uint32_t a, unsigned w, uint32_t *v) {
    (void)unused; a &= 0x1fffffff;
    if ((uint64_t)a + w > sizeof(ram)) return -1;
    *v = 0;
    for (unsigned i = 0; i < w; ++i) *v |= (uint32_t)ram[a+i] << (8*i);
    return 0;
}
static int write_bus(void *unused, uint32_t a, unsigned w, uint32_t v) {
    (void)unused; a &= 0x1fffffff;
    if ((uint64_t)a + w > sizeof(ram)) return -1;
    for (unsigned i = 0; i < w; ++i) ram[a+i] = (uint8_t)(v >> (8*i));
    return 0;
}
static int bridge(void *unused, PcPortMipsCpu *cpu, uint32_t target) {
    (void)unused;
    switch (target) {
    case 0x80087a38: case 0x80084a7c: case 0x8008aa74:
        record(target, cpu->gpr[4]); break;
    case 0x80077698: record(target, 0); break;
    default: return 0;
    }
    /* o32 caller-saved GPRs may be destroyed by every callee. */
    for (unsigned r = 2; r <= 15; ++r) cpu->gpr[r] = 0xa55a0000u + r;
    cpu->gpr[24] = 0xa55a0018; cpu->gpr[25] = 0xa55a0019;
    return 1;
}
static void half(unsigned offset, unsigned value) {
    initial[offset] = (uint8_t)value;
    initial[offset+1] = (uint8_t)(value >> 8);
}
static void check_rebinding(int oracle_only) {
    for (unsigned mode = 1; mode <= 4; ++mode) {
        /* Acceptance can rebind in any of its three calls. The repeat-Up
         * rejection call must clear the flag in the replacement owner. */
        unsigned rejecting = mode == 4;
        rebind_at = rejecting ? 1 : mode;
        native_phase = 0;
        memset(initial, 0xa5, sizeof(initial));
        initial[0x3c] = 0;
        initial[0x2dd] = 1; initial[0x2f6] = 255;
        half(0x30, 1); half(0x2a, 1);
        memcpy(ram+CONTEXT, initial, sizeof(initial));
        memset(ram+REPLACEMENT, 0x6b, CONTEXT_SIZE);
        write_bus(NULL, 0x800c3eac, 4, 0x80000000u+CONTEXT);
        ram[0xd3014] = rejecting ? 3 : 4;
        ram[0xc3e29] = 3; ram[0xd366c] = 0x5a;
        call_count = 0;
        PcPortMipsBus bus = {.read=read_bus, .write=write_bus, .bridge=bridge};
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = 0x100; cpu.gpr[29] = 0x801ff000;
        cpu.gpr[31] = 0xfffffffcu;
        if (PcPortMipsRun(&cpu, 0x8008115c, 0xfffffffcu, 1000)) fail(cpu.error);
        if (memcmp(ram+CONTEXT, initial, sizeof(initial))) fail("retail stale owner write");
        for (unsigned i = 0; i < CONTEXT_SIZE; ++i) {
            unsigned expected = i == (rejecting ? 0x2f6u : 0x2ddu) ?
                (rejecting ? 0 : 5) : 0x6b;
            if (ram[REPLACEMENT+i] != expected) fail("retail replacement write");
        }
        if (call_count != (rejecting ? 1u : 3u) ||
            ram[0xd366c] != (rejecting ? 0x5a : 0)) fail("retail rebind calls");
        uint32_t expected_calls[4], expected_args[4];
        unsigned expected_count = call_count;
        memcpy(expected_calls, calls, sizeof(calls));
        memcpy(expected_args, arguments, sizeof(arguments));
        if (!oracle_only && func_8008115C) {
            memcpy(native_context, initial, sizeof(initial));
            memset(replacement_context, 0x6b, sizeof(replacement_context));
            D_800C3EAC = (struct BattleCommandContext *)native_context;
            D_800D3014 = rejecting ? 3 : 4;
            D_800C3E29 = 3; D_800D366C = 0x5a;
            native_phase = 1; call_count = 0;
            func_8008115C(0x100);
            if ((void *)D_800C3EAC != replacement_context ||
                memcmp(native_context, ram+CONTEXT, sizeof(initial)) ||
                memcmp(replacement_context, ram+REPLACEMENT, sizeof(initial)) ||
                D_800D366C != ram[0xd366c] || call_count != expected_count ||
                memcmp(calls, expected_calls, call_count*sizeof(*calls)) ||
                memcmp(arguments, expected_args, call_count*sizeof(*arguments)))
                fail("native context reload differs from retail");
        }
    }
    rebind_at = 0; native_phase = 0;
    puts("ATTACK RING callback replacement PASS 4 cases");
}
int main(int argc, char **argv) {
    int oracle_only = argc == 2 && !strcmp(argv[1], "--oracle-only");
    FILE *f = fopen("disc/battle.bin", "rb");
    if (!f) fail("retail file missing");
    size_t n = fread(ram + 0x6faf0, 1, 0x60000, f);
    if (n <= 0x11824 || !feof(f) || fclose(f)) fail("retail file read");
    const uint32_t actors[] = {0, 1, 255, 0x101};
    unsigned cases = 0;
    for (unsigned ai = 0; ai < 4; ++ai)
    for (unsigned event = 0; event < 256; ++event)
    for (unsigned mask = 0; mask < 16; ++mask)
    for (unsigned repeat = 0; repeat < 2; ++repeat)
    for (unsigned last = 0; last < 2; ++last)
    for (unsigned invalid = 0; invalid < 2; ++invalid) {
        unsigned actor = actors[ai] & 255, row = actor * 64;
        memset(initial, 0xa5, sizeof(initial));
        half(row+0x26, mask & 1 ? 0x100 : 0);
        half(row+0x32, mask & 2 ? 0x8000 : 0);
        half(row+0x30, mask & 4 ? 0xffff : 0);
        half(row+0x2a, mask & 8 ? 1 : 0);
        initial[row+0x3c] = invalid ? 255 : 0;
        initial[0x2dd] = 1; initial[0x2f6] = repeat ? 255 : 0;
        memcpy(ram+CONTEXT, initial, sizeof(initial));
        write_bus(NULL, 0x800c3eac, 4, 0x80100000);
        ram[0xd3014] = (uint8_t)event;
        ram[0xc3e29] = last ? 3 : 255;
        ram[0xd366c] = 0x5a;
        call_count = 0;
        PcPortMipsBus bus = {.read=read_bus, .write=write_bus, .bridge=bridge};
        PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = actors[ai]; cpu.gpr[29] = 0x801ff000;
        cpu.gpr[31] = 0xfffffffcu;
        if (PcPortMipsRun(&cpu, 0x8008115c, 0xfffffffcu, 1000)) fail(cpu.error);
        /* Independently stated ring contract, including invalid event no-op. */
        unsigned state = 1, flag = repeat ? 255 : 0, clear = 0, error = 0;
        if (event == 0) { if (mask & 1) error = 1; else state = 7; }
        if (event == 1) { if (mask & 2) error = 1; else state = 2; }
        if (event == 2) state = 3;
        if (event == 3) {
            if (!(mask & 4)) state = 4;
            else if (repeat && last) {
                flag = 0;
                if (mask & 8) error = 1; else state = 10;
            } else { flag = 1; error = 1; }
        }
        if (event == 4 || event == 6 || event == 7) {
            if (invalid) error = 1; else { state = 5; clear = 1; }
        }
        initial[0x2dd] = (uint8_t)state; initial[0x2f6] = (uint8_t)flag;
        if (memcmp(initial, ram+CONTEXT, sizeof(initial)) ||
            ram[0xd366c] != (clear ? 0 : 0x5a)) fail("retail state contract");
        if (call_count != (clear ? 3u : error)) fail("retail call count");
        if (error && (calls[0] != 0x8008aa74 || arguments[0] != 0x4f))
            fail("retail rejection call");
        if (clear && (calls[0] != 0x80087a38 || calls[1] != 0x80084a7c ||
            calls[2] != 0x80077698 || arguments[0] != actor || arguments[1] != actor))
            fail("retail accept calls");
        if (!oracle_only && func_8008115C) {
            uint32_t expected_calls[4], expected_args[4];
            unsigned expected_count = call_count;
            memcpy(expected_calls, calls, sizeof(calls));
            memcpy(expected_args, arguments, sizeof(arguments));
            memcpy(native_context, initial, sizeof(initial));
            native_context[0x2dd] = 1; native_context[0x2f6] = repeat ? 255 : 0;
            D_800C3EAC = (struct BattleCommandContext *)native_context;
            D_800D3014 = (uint8_t)event;
            D_800C3E29 = last ? 3 : 255; D_800D366C = 0x5a;
            call_count = 0;
            func_8008115C(actors[ai]);
            if (memcmp(native_context, ram+CONTEXT, sizeof(initial)) ||
                D_800D366C != ram[0xd366c] || call_count != expected_count ||
                memcmp(calls, expected_calls, call_count*sizeof(*calls)) ||
                memcmp(arguments, expected_args, call_count*sizeof(*arguments)))
                fail("native differs from retail");
        }
        ++cases;
    }
    printf("ATTACK RING retail contract PASS %u cases\n", cases);
    check_rebinding(oracle_only);
    if (!oracle_only && !func_8008115C) fail("missing native owner 8008115C");
    if (!oracle_only) puts("ATTACK RING native differential PASS");
    return 0;
}
