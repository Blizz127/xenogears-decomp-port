#include "battle_target_eligibility_ram.h"
#include <setjmp.h>

typedef uint8_t u8;
typedef uint32_t u32;
struct TargetRam {
    const uint8_t *bytes;
    size_t size;
    jmp_buf failure;
};

static uint32_t offset(struct TargetRam *ram, uint32_t address) {
    if (address >= 0x80000000u && address < 0x80200000u)
        return address - 0x80000000u;
    if (address >= 0xA0000000u && address < 0xA0200000u)
        return address - 0xA0000000u;
    longjmp(ram->failure, 1);
}

static uint32_t read_ram(struct TargetRam *ram, uint32_t address, unsigned width) {
    uint32_t at = offset(ram, address);
    if (address % width || at > ram->size || width > ram->size - at ||
        width > 0x200000u - at) longjmp(ram->failure, 1);
    uint32_t value = 0;
    for (unsigned i = 0; i < width; i++)
        value |= (uint32_t)ram->bytes[at+i] << (8*i);
#ifdef XBT_TEST_RAM_TRACE
    /* Test-only observation of completed accesses, absent from normal builds. */
    extern void PcPortBattleTargetRamObserve(uint32_t address, unsigned width);
    PcPortBattleTargetRamObserve(address, width);
#endif
    return value;
}

static uint32_t matrix_value(struct TargetRam *ram, uint32_t actor, uint32_t target) {
    /* Retail order: target group, actor group, four-byte owner, matrix byte. */
    uint32_t tg = read_ram(ram, 0x800C3EB4u + target*28, 1);
    uint32_t ag = read_ram(ram, 0x800C3EB4u + actor*28, 1);
    uint32_t base = read_ram(ram, 0x800D3364u, 4);
    uint32_t delta = 0x140u + ag*64 + tg*8;
    (void)offset(ram, base); /* Validate owner before adding/alias conversion. */
    if (base > UINT32_MAX - delta) longjmp(ram->failure, 1);
    return read_ram(ram, base + delta, 1);
}

#define XBT_ELIGIBILITY_CUSTOM_BINDINGS
#define XBT_ELIGIBILITY_SIGNATURE \
    static u32 target_eligibility_body(struct TargetRam *ram, u32 arg0, u32 arg1)
#define XBT_PRESENT(t) read_ram(ram, 0x800D2DCCu + (t), 1)
#define XBT_BLOCKED(t) read_ram(ram, 0x800C3EB7u + (t)*28, 1)
#define XBT_ALTERNATE(t) read_ram(ram, 0x800D32A1u + (t)*8, 1)
#define XBT_MATRIX(a, t) matrix_value(ram, a, t)
#define XBT_NORMAL_STATUS(t) read_ram(ram, 0x800CCD64u + (t)*0x170, 2)
#define XBT_ALTERNATE_STATUS(t) read_ram(ram, 0x800CCE08u + (t)*0x170, 2)
#include "../../src/battle/target_eligibility_impl.inc"

int PcPortBattleTargetEligibilityRam(const uint8_t *bytes, size_t size,
                                    uint32_t actor, uint32_t target,
                                    uint32_t *result) {
    if (!bytes || !result) return -1;
    struct TargetRam ram = {.bytes = bytes, .size = size};
    if (setjmp(ram.failure)) return -1;
    uint32_t value = target_eligibility_body(&ram, actor, target);
    *result = value;
    return 0;
}
