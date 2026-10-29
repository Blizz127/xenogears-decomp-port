/* Synthetic mechanism test: no retail inputs and no gameplay claim. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void PcPort_BattleResultsTraceBeforeGuest(PcPortMipsCpu*, uint32_t);
static unsigned char gs[0x235C];
static unsigned reads;
static int read_failure;
static const char *out;

static int read_bus(void *opaque, uint32_t address, unsigned width, uint32_t *value)
{
    (void)opaque;
    assert(width == 1);
    reads++;
    if (read_failure && address == 0x8006D634u + 0x2358u) return -1;
    if (address - 0x8006D634u < sizeof(gs)) *value = gs[address - 0x8006D634u];
    else *value = address & 255u;
    return 0;
}

static void path(char *buffer, size_t size, const char *leaf)
{
    assert(snprintf(buffer, size, "%s/%s", out, leaf) < (int)size);
}

int main(int argc, char **argv)
{
    PcPortMipsCpu cpu = {0};
    char filename[4096], log[4096];
    FILE *f;
    size_t bytes;
    assert(argc == 3);
    out = argv[2];
    cpu.bus.read = read_bus;
    cpu.gpr[31] = 0x80071698u;
    cpu.gpr[29] = 0x801FFFF0u;
    if (strcmp(argv[1], "off") == 0) {
        unsetenv("XENO_BATTLE_RESULTS_SNAPSHOT_DIR");
        PcPort_BattleResultsTraceBeforeGuest(&cpu, 0x801E252Cu);
        assert(reads == 0);
        puts("RESULTS TRACE off PASS");
        return 0;
    }
    assert(setenv("XENO_BATTLE_RESULTS_SNAPSHOT_DIR", out, 1) == 0);
    read_failure = strcmp(argv[1], "fault") == 0;
    PcPort_BattleResultsTraceBeforeGuest(&cpu, 0x80070F40u);
    PcPort_BattleResultsTraceBeforeGuest(&cpu, 0x801E252Cu);
    gs[0x2358] = 42; /* native write, no interpreted-store hook */
    PcPort_BattleResultsTraceBeforeGuest(&cpu, 0x80044BD0u);
    cpu.gpr[29] -= 16; /* same RA inside a callee is not a RESULTS return */
    PcPort_BattleResultsTraceBeforeGuest(&cpu, cpu.gpr[31]);
    path(filename, sizeof(filename), "b1-r1-return-1.bin");
    f = fopen(filename, "rb");
    assert(!f);
    cpu.gpr[29] += 16;
    PcPort_BattleResultsTraceBeforeGuest(&cpu, cpu.gpr[31]);
    path(filename, sizeof(filename), "snapshots.log");
    f = fopen(filename, "rb");
    assert(f);
    bytes = fread(log, 1, sizeof(log)-1, f);
    log[bytes] = 0;
    assert(fclose(f) == 0);
    if (read_failure) {
        assert(strstr(log, "FAIL snapshot-read-or-write"));
        assert(!strstr(log, "status=OK"));
    } else {
        assert(!strstr(log, "FAIL"));
        assert(strstr(log, "S 1 1 entry 0"));
        assert(strstr(log, "S 1 1 render 0"));
        assert(strstr(log, "S 1 1 return 1"));
        path(filename, sizeof(filename), "b1-r1-return-1.bin");
        f = fopen(filename, "rb");
        assert(f && fseek(f, 0x2358, SEEK_SET) == 0);
        assert(fgetc(f) == 42);
        assert(fclose(f) == 0);
    }
    puts(read_failure ? "RESULTS TRACE read-failure PASS" : "RESULTS TRACE native-write/return PASS");
    return 0;
}
