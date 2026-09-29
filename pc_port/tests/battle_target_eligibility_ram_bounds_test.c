#include "battle_target_eligibility_ram.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static uint8_t ram[0x200000], saved[0x200000];
static uint32_t target_arg = 0x103;
void PcPortBattleTargetRamObserve(uint32_t address, unsigned width) {
    (void)address; (void)width;
}
static void owner(uint32_t value) {
    for (unsigned i = 0; i < 4; i++) ram[0xD3364+i] = value >> (8*i);
}
static void check(size_t size, int valid, uint32_t expected) {
    uint32_t result = 0xDEADBEEF;
    memcpy(saved, ram, sizeof(ram));
    int rc = PcPortBattleTargetEligibilityRam(ram, size, 0x100, target_arg, &result);
    assert(rc == (valid ? 0 : -1));
    assert(result == (valid ? expected : 0xDEADBEEF));
    assert(memcmp(saved, ram, sizeof(ram)) == 0);
}
int main(void) {
    /* The unused invalid matrix must not prevent an early/alternate result. */
    owner(0);
    check(sizeof(ram), 1, 0);
    ram[0xD2DCC+3] = 1;
    ram[0xC3EB7+3*28] = 1;
    check(sizeof(ram), 1, 0);
    ram[0xC3EB7+3*28] = 0;
    ram[0xD32A1+3*8] = 1;
    check(sizeof(ram), 1, 1);
    ram[0xD32A1+3*8] = 0;
    const uint32_t bad[] = {0, 0x00100000, 0x7FFFFFFF, 0x80200000,
                            0xA0200000, 0xC0100000, 0xFFFFFFFF};
    for (unsigned i=0; i<sizeof(bad)/sizeof(bad[0]); i++) {
        owner(bad[i]); check(sizeof(ram), 0, 0);
    }
    /* Exact last-byte access succeeds; requiring a whole matrix would fail. */
    owner(0x801FFEBF); check(sizeof(ram), 1, 1);
    owner(0xA01FFEBF); check(sizeof(ram), 1, 1);
    owner(0x801FFEC0); check(sizeof(ram), 0, 0);
    owner(0x80100000); check(0x100140, 0, 0);
    check(0x100141, 1, 1);
    check(0xD3367, 0, 0); /* truncated packed pointer */
    check(0, 0, 0);
    check(0xD2DCC+3, 0, 0); /* initial presence byte */
    check(0xD32A1+3*8, 0, 0); /* alternate flag */
    /* High target puts both status halfwords beyond all earlier fields. */
    target_arg = 0x1FF;
    ram[0xD2DCC+255] = 1;
    owner(0x800E0000);
    for (unsigned alt=0; alt<2; alt++) {
        ram[0xD32A1+255*8] = alt;
        unsigned status_at = (alt ? 0xCCE08 : 0xCCD64) + 255*0x170;
        check(status_at+1, 0, 0);
        check(status_at+2, 1, 1);
    }
    uint32_t result = 42;
    assert(PcPortBattleTargetEligibilityRam(NULL, sizeof(ram), 0, 3, &result) == -1);
    assert(result == 42);
    assert(PcPortBattleTargetEligibilityRam(ram, sizeof(ram), 0, 3, NULL) == -1);
    puts("ELIGIBILITY RAM bounds/aliases/no-write PASS");
}
