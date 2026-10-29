/* Opt-in test tooling, not a native conversion or release acceptance gate.
 * XENO_BATTLE_RESULTS_SNAPSHOT_DIR must be a pre-created PRIVATE output dir.
 * Captures canonical bus-visible memory at RESULTS entry, every DrawOTag while
 * inside RESULTS, and its observed return (saved RA and SP in the same CPU).
 * Native and interpreted writes are both observed. Never compare decoded
 * instruction-store logs as parity: they omit native writes.
 * Snapshots can contain retail module bytes; keep them outside source/public
 * artifacts. Read/I/O failures and a 512-render bound fail the evidence closed.
 */
#include "battle_mips_adapter.h"
#include <stdio.h>
#include <stdlib.h>

void PcPort_BattleResultsTraceBeforeGuest(PcPortMipsCpu*, uint32_t);

static const struct { uint32_t address, size; } ranges[] = {
    {0x8006D634u, 0x235Cu}, /* game state INCLUDING field-transition tuple */
    {0x801DE000u, 0x6500u}, /* RESULTS immutable code + mutable workspace */
    {0x800CCCE8u, 11u * 0x170u}, /* battlers */
    {0x800C3FE8u, 32u * 72u},    /* actions */
    {0x800D3400u, 8u * 64u},    /* enemy AI */
    {0x800D301Cu, 0x40u},       /* enemy groups */
};
static const char *directory;
static FILE *log_file;
static int initialized, active, failed;
static unsigned battle, invocation, render;
static uint32_t return_pc, entry_sp;
static PcPortMipsCpu *entry_cpu;

static void snapshot(PcPortMipsCpu *cpu, const char *phase)
{
    char path[4096];
    FILE *f;
    unsigned i;
    int n = snprintf(path, sizeof(path), "%s/b%u-r%u-%s-%u.bin", directory,
                     battle, invocation, phase, render);
    if (n < 0 || (size_t)n >= sizeof(path) || !(f = fopen(path, "wb"))) {
        fprintf(log_file, "FAIL snapshot-open phase=%s\n", phase);
        failed = 1;
        return;
    }
    for (i = 0; i < sizeof(ranges) / sizeof(ranges[0]); i++) {
        uint32_t j;
        for (j = 0; j < ranges[i].size; j++) {
            uint32_t value;
            if (cpu->bus.read(cpu->bus.opaque, ranges[i].address + j, 1, &value) != 0 ||
                fputc((int)(value & 255u), f) == EOF) {
                fprintf(log_file, "FAIL snapshot-read-or-write phase=%s address=%08x\n",
                        phase, ranges[i].address + j);
                failed = 1;
                goto finish;
            }
        }
    }
finish:
    if (fclose(f) != 0) {
        fprintf(log_file, "FAIL snapshot-close phase=%s\n", phase);
        failed = 1;
    }
    fprintf(log_file, "S %u %u %s %u ra=%08x sp=%08x status=%s\n",
            battle, invocation, phase, render, return_pc, entry_sp,
            failed ? "FAIL" : "OK");
    fflush(log_file);
}

void PcPort_BattleResultsTraceBeforeGuest(PcPortMipsCpu *cpu, uint32_t target)
{
    if (!initialized) {
        char path[4096];
        int n;
        initialized = 1;
        directory = getenv("XENO_BATTLE_RESULTS_SNAPSHOT_DIR");
        if (!directory || !*directory) return;
        n = snprintf(path, sizeof(path), "%s/snapshots.log", directory);
        if (n < 0 || (size_t)n >= sizeof(path) || !(log_file = fopen(path, "w"))) {
            fprintf(stderr, "[battle-results-trace] FAIL opening private snapshot log\n");
            return;
        }
        fputs("FORMAT 1 gs=235c results=6500 battlers=fd0 actions=900 ai=200 groups=40\n", log_file);
    }
    if (!log_file || failed) return;
    if (target == 0x80070F40u) {
        if (active) {
            fputs("FAIL battle-reentry-before-results-return\n", log_file);
            fflush(log_file);
            failed = 1;
            return;
        }
        battle++;
        active = 0;
        invocation = 0;
    }
    if (active && cpu == entry_cpu && target == return_pc && cpu->gpr[29] == entry_sp) {
        snapshot(cpu, "return");
        active = 0;
    }
    if (target == 0x801E252Cu) {
        if (active) {
            fputs("FAIL nested-results-entry\n", log_file);
            failed = 1;
        } else {
            invocation++;
            render = 0;
            entry_cpu = cpu;
            return_pc = cpu->gpr[31];
            entry_sp = cpu->gpr[29];
            active = 1;
            snapshot(cpu, "entry");
        }
    } else if (active && target == 0x80044BD0u) {
        if (render >= 512) {
            fputs("FAIL results-render-limit\n", log_file);
            failed = 1;
        } else {
            snapshot(cpu, "render");
            render++;
        }
    }
    fflush(log_file);
}
