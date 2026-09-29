#include "battle_mips_adapter.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern void func_80023538(void *, void *);

#define SLUS_BASE 0x8000f800u
#define RETAIL_PC 0x80023538u
#define RETAIL_END 0x80023804u
#define SPRITE_PC 0x80180000u
#define ANIM_PC 0x80181000u
#define BASE_PC 0x80182000u
#define STACK_PC 0x801ff000u
#define D_FLAG_PC 0x800591adu
#define HALT_PC 0xfffffffcu

static uint8_t retail_ram[0x200000];
static uint8_t native_sprite[0xb4];
static uint8_t native_anim[0x40];
static uint8_t native_base[0x80];
static uint8_t native_state[0x20];
uint8_t D_800591AD;
int32_t D_800591A8;
int32_t D_80059198;

typedef struct {
    unsigned kind;
    uint32_t flag;
    int32_t arg;
} Event;
static Event retail_events[4], native_events[4];
static unsigned retail_event_count, native_event_count;
static int scale_zero;

static uint8_t *guest_ptr(uint32_t address, unsigned width)
{
    if (address < 0x80000000u ||
        (uint64_t)address + width > 0x80200000u)
        return NULL;
    return retail_ram + (address & 0x1fffffu);
}

static int retail_read(void *unused, uint32_t address, unsigned width,
                       uint32_t *value)
{
    (void)unused;
    uint8_t *p = guest_ptr(address, width);
    if (p == NULL)
        return -1;
    *value = 0;
    for (unsigned i = 0; i < width; ++i)
        *value |= (uint32_t)p[i] << (8u * i);
    return 0;
}

static int retail_write(void *unused, uint32_t address, unsigned width,
                        uint32_t value)
{
    (void)unused;
    uint8_t *p = guest_ptr(address, width);
    if (p == NULL)
        return -1;
    for (unsigned i = 0; i < width; ++i)
        p[i] = (uint8_t)(value >> (8u * i));
    return 0;
}

static int retail_bridge(void *unused, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)unused;
    if (target == 0x80022000u || target == 0x80022090u) {
        uint8_t *sprite = guest_ptr(cpu->gpr[4], 0xb4);
        uint32_t base_address;
        uint8_t *base;
        assert(sprite != NULL);
        memcpy(&base_address, sprite + 0x20, sizeof(base_address));
        base = guest_ptr(base_address, 0x20);
        assert(base != NULL);
        assert(retail_event_count < 4);
        retail_events[retail_event_count++] =
            (Event){target == 0x80022000u ? 1u : 2u,
                    retail_ram[D_FLAG_PC & 0x1fffffu],
                    target == 0x80022000u ? (int32_t)cpu->gpr[5] : 0};
        if (target == 0x80022000u) {
            base[0x10] = 0x11;
            base[0x11] = 0x11;
            retail_ram[D_FLAG_PC & 0x1fffffu] = scale_zero ? 0 : 2;
        } else {
            base[0x14] = (uint8_t)retail_ram[D_FLAG_PC & 0x1fffffu];
        }
        return 1;
    }
    if (target == 0x800234acu)
        return 1;
    return 0;
}

void SpriteSetScale(void *sprite, short scale)
{
    uint8_t *p = sprite;
    uint8_t *base;
    uint32_t packed_base;
    (void)scale;
    assert(native_event_count < 4);
    memcpy(&packed_base, p + 0x20, sizeof(packed_base));
    base = (uint8_t *)(uintptr_t)packed_base;
    native_events[native_event_count++] = (Event){1u, D_800591AD, scale};
    base[0x10] = 0x11;
    base[0x11] = 0x11;
    D_800591AD = scale_zero ? 0 : 2;
}

void SpriteComputeTransformMatrix(void *sprite)
{
    uint8_t *p = sprite;
    uint8_t *base;
    uint32_t packed_base;
    assert(native_event_count < 4);
    memcpy(&packed_base, p + 0x20, sizeof(packed_base));
    base = (uint8_t *)(uintptr_t)packed_base;
    native_events[native_event_count++] = (Event){2u, D_800591AD, 0};
    base[0x14] = D_800591AD;
}

void func_800234AC(void *sprite) { (void)sprite; }

static void put32(uint8_t *p, uint32_t value) { memcpy(p, &value, 4); }
static void put16(uint8_t *p, uint16_t value) { memcpy(p, &value, 2); }

static void load_retail_function(void)
{
    const char *path = getenv("TRANSFORM_RETAIL_IMAGE");
    FILE *file;
    assert(path != NULL);
    file = fopen(path, "rb");
    assert(file != NULL);
    assert(fread(guest_ptr(RETAIL_PC, RETAIL_END - RETAIL_PC), 1,
                RETAIL_END - RETAIL_PC, file) == RETAIL_END - RETAIL_PC);
    assert(!fclose(file));
}

static void setup_fixture(unsigned flags, unsigned flag_value,
                          unsigned base_present)
{
    memset(retail_ram, 0xa5, sizeof(retail_ram));
    memset(native_sprite, 0xa5, sizeof(native_sprite));
    memset(native_anim, 0xa5, sizeof(native_anim));
    memset(native_base, 0xa5, sizeof(native_base));
    memset(native_state, 0xa5, sizeof(native_state));
    put32(native_sprite + 0x20, base_present ?
          (uint32_t)(uintptr_t)native_base : 0);
    put32((uint8_t *)guest_ptr(SPRITE_PC, 0x100) + 0x20,
          base_present ? BASE_PC : 0);
    put32(native_sprite + 0x7c, 0);
    put32(guest_ptr(SPRITE_PC, 0x100) + 0x7c, 0);
    put32(native_sprite + 0xac, 256u << 7);
    put32(guest_ptr(SPRITE_PC, 0x100) + 0xac, 256u << 7);
    put16(native_sprite + 0x82, 0);
    put16(guest_ptr(SPRITE_PC, 0x100) + 0x82, 0);
    put16(native_anim, (uint16_t)flags);
    put16(guest_ptr(ANIM_PC, 0x40), (uint16_t)flags);
    put16(native_anim + 2, 0);
    put16(native_anim + 4, 0);
    D_80059198 = 0;
    D_800591A8 = 1234;
    D_800591AD = (uint8_t)flag_value;
    put32(guest_ptr(0x80059198u, 4), (uint32_t)D_80059198);
    put32(guest_ptr(0x800591a8u, 4), (uint32_t)D_800591A8);
    retail_ram[D_FLAG_PC & 0x1fffffu] = (uint8_t)flag_value;
    retail_event_count = native_event_count = 0;
}

static int run_retail(void)
{
    PcPortMipsBus bus = {.read = retail_read, .write = retail_write,
                         .bridge = retail_bridge};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = SPRITE_PC;
    cpu.gpr[5] = ANIM_PC;
    cpu.gpr[29] = STACK_PC;
    cpu.gpr[31] = HALT_PC;
    int result = PcPortMipsRun(&cpu, RETAIL_PC, HALT_PC, 4000);
    if (result != PC_PORT_MIPS_HALTED)
        fprintf(stderr, "TRANSFORM retail adapter: %s\n", cpu.error);
    return result;
}

static int same_events(void)
{
    if (retail_event_count != native_event_count)
        return 0;
    for (unsigned i = 0; i < retail_event_count; ++i)
        if (retail_events[i].kind != native_events[i].kind ||
            retail_events[i].flag != native_events[i].flag ||
            retail_events[i].arg != native_events[i].arg)
            return 0;
    return 1;
}

static int run_case(unsigned flags, unsigned flag_value, unsigned base_present)
{
    Event expected_events[4];
    unsigned expected_event_count;
    uint8_t expected_base10, expected_base14;
    int result;
    setup_fixture(flags, flag_value, base_present);
    load_retail_function();
    result = run_retail();
    if (result != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "TRANSFORM FAIL retail pc=%08x\n", result);
        return 1;
    }
    expected_event_count = retail_event_count;
    memcpy(expected_events, retail_events, sizeof(expected_events));
    expected_base10 = guest_ptr(BASE_PC, 0x80)[0x10];
    expected_base14 = guest_ptr(BASE_PC, 0x80)[0x14];
    setup_fixture(flags, flag_value, base_present);
    func_80023538(native_sprite, native_anim);
    retail_event_count = expected_event_count;
    memcpy(retail_events, expected_events, sizeof(retail_events));
    if (!same_events() || expected_base10 != native_base[0x10] ||
        expected_base14 != native_base[0x14]) {
        fprintf(stderr, "TRANSFORM RED branch/order flags=%04x flag=%u\n",
                flags, flag_value);
        return 1;
    }
    return 0;
}

static int run_matrix(void)
{
    for (unsigned bit12 = 0; bit12 != 2; ++bit12)
        for (unsigned bit13 = 0; bit13 != 2; ++bit13)
            for (unsigned flag = 0; flag != 2; ++flag)
                for (unsigned base = 0; base != 2; ++base)
                    if (run_case((bit12 << 12) | (bit13 << 13), flag,
                                 base) != 0)
                        return 1;
    return 0;
}

int main(void)
{
    assert((uintptr_t)native_sprite + sizeof(native_sprite) <= UINT32_MAX);
    assert((uintptr_t)native_anim + sizeof(native_anim) <= UINT32_MAX);
    assert((uintptr_t)native_base + sizeof(native_base) <= UINT32_MAX);
    scale_zero = 0;
    if (run_matrix() != 0)
        return 1;
    scale_zero = 1;
    if (run_matrix() != 0)
        return 1;
    puts("TRANSFORM PASS 32 retail/native 23538 cases, including Scale->0 "
         "fresh-flag read; helper boundary only, not matrix algorithm parity");
    return 0;
}
