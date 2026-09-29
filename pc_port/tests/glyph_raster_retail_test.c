#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battle_mips_adapter.h"
#include "common.h"

extern void func_80034FFC(s32 lead, s32 trail, void *work,
                          s32 stride_words, s32 page);
extern u32 D_8005934C;
extern u32 D_80059350;
extern u32 D_8005935C;
extern u32 D_80059364;

/* The ordinary path is selected in this audit.  Keep the special-glyph
 * symbol real so the production TU's declaration remains linkable. */
u16 D_800501D0[11];

enum {
    kFunctionAddress = 0x80034ffcu,
    kFunctionSize = 1696u,
    kHaltAddress = 0x801ff000u,
    kStackAddress = 0x801ff800u,
    kWorkAddress = 0x80130000u,
    kGlyphAddress = 0x80110000u,
    kD5934C = 0x8005934cu,
    kD59350 = 0x80059350u,
    kD5935C = 0x8005935cu,
    kD59364 = 0x80059364u,
    kRows = 13,
    kMaxStride = 42,
    kGuardWords = 17,
    kGlyphBackingBytes = 0x100,
    kGuestRamSize = 0x300000,
};

typedef struct Guest {
    uint8_t ram[kGuestRamSize];
} Guest;

static uint16_t s_native_work[kGuardWords + kMaxStride * kRows + kGuardWords];
static uint16_t s_native_glyph[kGlyphBackingBytes / 2];

static uint16_t get_le16(const uint8_t *p)
{
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}

static uint32_t get_le32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static void put_le16(uint8_t *p, uint16_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static void put_le32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static uint8_t *guest_ptr(Guest *guest, uint32_t address, unsigned width)
{
    uint64_t offset;

    if (address < 0x80000000u)
        return NULL;
    offset = (uint64_t)address - 0x80000000u;
    if (offset + width > sizeof(guest->ram))
        return NULL;
    return guest->ram + (size_t)offset;
}

static int guest_read(void *opaque, uint32_t address, unsigned width,
                      uint32_t *value)
{
    Guest *guest = opaque;
    uint8_t *p = guest_ptr(guest, address, width);

    if (p == NULL || (width != 1u && width != 2u && width != 4u))
        return -1;
    *value = width == 1u ? p[0] : width == 2u ? get_le16(p) : get_le32(p);
    return 0;
}

static int guest_write(void *opaque, uint32_t address, unsigned width,
                       uint32_t value)
{
    Guest *guest = opaque;
    uint8_t *p = guest_ptr(guest, address, width);

    if (p == NULL || (width != 1u && width != 2u && width != 4u))
        return -1;
    if (width == 1u)
        p[0] = (uint8_t)value;
    else if (width == 2u)
        put_le16(p, (uint16_t)value);
    else
        put_le32(p, value);
    return 0;
}

static void guest_store16(Guest *guest, uint32_t address, uint16_t value)
{
    uint8_t *p = guest_ptr(guest, address, 2u);
    if (p == NULL) abort();
    put_le16(p, value);
}

static uint16_t guest_load16(const Guest *guest, uint32_t address)
{
    uint8_t *p = guest_ptr((Guest *)guest, address, 2u);
    if (p == NULL) abort();
    return get_le16(p);
}

static int load_retail(Guest *guest, const char *path)
{
    FILE *file = fopen(path, "rb");
    uint8_t *destination = guest_ptr(guest, kFunctionAddress,
                                     kFunctionSize);
    size_t got;

    if (file == NULL || destination == NULL)
        return 0;
    got = fread(destination, 1u, kFunctionSize, file);
    fclose(file);
    return got == kFunctionSize;
}

static void fill_native_words(uint16_t *p, size_t count, uint16_t value)
{
    size_t i;
    for (i = 0; i < count; i++)
        p[i] = value;
}

static void fill_guest_words(Guest *guest, uint32_t address, size_t count,
                             uint16_t value)
{
    size_t i;
    for (i = 0; i < count; i++)
        guest_store16(guest, address + (uint32_t)(i * 2u), value);
}

static void make_glyph(uint16_t *native, Guest *guest, unsigned glyph_kind)
{
    static const uint16_t mixed[11] = {
        0x8001, 0x0240, 0x1818, 0x4002, 0x00f0, 0x0f00,
        0xaaaa, 0x5555, 0x1001, 0x2084, 0x7e81,
    };
    unsigned i;

    for (i = 0; i < kGlyphBackingBytes / 2u; i++) {
        native[i] = 0x1357u;
        guest_store16(guest, kGlyphAddress - 0x80u + i * 2u, 0x1357u);
    }
    for (i = 0; i < 11u; i++) {
        uint16_t value;
        if (glyph_kind == 0u)
            value = 0;
        else if (glyph_kind == 1u)
            value = 0xffffu;
        else if (glyph_kind == 2u)
            value = (uint16_t)(1u << (i % 16u));
        else
            value = mixed[i];
        native[0x40u / 2u + i] = value;
        native[0x40u / 2u - 0x16u / 2u + i] = value;
        guest_store16(guest, kGlyphAddress + i * 2u, value);
        guest_store16(guest, kGlyphAddress - 0x16u + i * 2u, value);
    }
}

static void set_native_globals(void)
{
    uintptr_t glyph = (uintptr_t)&s_native_glyph[0x40u / 2u];
    if (glyph > UINT32_MAX)
        abort();
    D_8005934C = 0xfeu;
    D_80059350 = 0u;
    D_8005935C = (u32)glyph;
    D_80059364 = 0x41u;
}

static void set_guest_globals(Guest *guest)
{
    put_le32(guest_ptr(guest, kD5934C, 4u), 0xfeu);
    put_le32(guest_ptr(guest, kD59350, 4u), 0u);
    put_le32(guest_ptr(guest, kD5935C, 4u), kGlyphAddress);
    put_le32(guest_ptr(guest, kD59364, 4u), 0x41u);
}

static int check_case(const char *retail_path, unsigned page,
                      unsigned stride, unsigned glyph_kind, uint16_t fill,
                      uint16_t trail, unsigned case_number)
{
    Guest guest;
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {0};
    size_t work_words = (size_t)stride * kRows;
    size_t storage_words = kGuardWords + work_words + kGuardWords;
    uint16_t native_glyph_before[kGlyphBackingBytes / 2u];
    uint8_t guest_glyph_before[kGlyphBackingBytes];
    uint16_t *native_work = &s_native_work[kGuardWords];
    unsigned i;
    int rc;

    memset(&guest, 0, sizeof(guest));
    memset(s_native_work, 0, sizeof(s_native_work));
    fill_native_words(s_native_work, sizeof(s_native_work) / sizeof(*s_native_work),
                      0xd00du);
    fill_native_words(native_work, work_words, fill);
    fill_guest_words(&guest, kWorkAddress - kGuardWords * 2u,
                     storage_words, 0xd00du);
    fill_guest_words(&guest, kWorkAddress, work_words, fill);
    make_glyph(s_native_glyph, &guest, glyph_kind);
    memcpy(native_glyph_before, s_native_glyph, sizeof(native_glyph_before));
    memcpy(guest_glyph_before,
           guest_ptr(&guest, kGlyphAddress - 0x80u, kGlyphBackingBytes),
           sizeof(guest_glyph_before));
    set_native_globals();
    set_guest_globals(&guest);

    func_80034FFC(0, trail, native_work, (s32)stride, (s32)page);

    bus.opaque = &guest;
    bus.read = guest_read;
    bus.write = guest_write;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = 0u;
    cpu.gpr[5] = trail;
    cpu.gpr[6] = kWorkAddress;
    cpu.gpr[7] = stride;
    cpu.gpr[29] = kStackAddress;
    cpu.gpr[31] = kHaltAddress;
    guest_store16(&guest, kStackAddress + 16u, (uint16_t)page);
    rc = load_retail(&guest, retail_path) ?
        PcPortMipsRun(&cpu, kFunctionAddress, kHaltAddress, 200000u) :
        PC_PORT_MIPS_FAULT;
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "DIFF case=%u retail-run rc=%d error=%s\n",
                case_number, rc, cpu.error);
        return 0;
    }

    for (i = 0; i < work_words; i++) {
        uint16_t actual = guest_load16(&guest, kWorkAddress + i * 2u);
        if (actual != native_work[i]) {
            fprintf(stderr,
                    "DIFF case=%u output word=%u got=%04x native=%04x page=%u stride=%u glyph=%u fill=%04x trail=%04x\n",
                    case_number, i, actual, native_work[i], page, stride,
                    glyph_kind, fill, trail);
            return 0;
        }
    }
    for (i = 0; i < kGuardWords; i++) {
        uint16_t before = 0xd00du;
        uint16_t left = guest_load16(&guest,
                                     kWorkAddress - (kGuardWords - i) * 2u);
        uint16_t right = guest_load16(&guest,
                                      kWorkAddress + work_words * 2u + i * 2u);
        if (s_native_work[i] != before ||
            s_native_work[kGuardWords + work_words + i] != before ||
            left != before || right != before) {
            fprintf(stderr, "DIFF case=%u redzone page=%u stride=%u\n",
                    case_number, page, stride);
            return 0;
        }
    }
    {
        uint16_t inactive = page ? 0x3333u : 0xccccu;
        for (i = 0; i < work_words; i++) {
            uint16_t initial = fill;
            uint16_t actual = guest_load16(&guest, kWorkAddress + i * 2u);
            if ((actual & inactive) != (initial & inactive) ||
                (native_work[i] & inactive) != (initial & inactive)) {
                fprintf(stderr, "DIFF case=%u inactive-bits word=%u\n",
                        case_number, i);
                return 0;
            }
        }
    }
    if (memcmp(s_native_glyph, native_glyph_before,
               sizeof(native_glyph_before)) != 0 ||
        memcmp(guest_ptr(&guest, kGlyphAddress - 0x80u, kGlyphBackingBytes),
               guest_glyph_before, sizeof(guest_glyph_before)) != 0) {
        fprintf(stderr, "DIFF case=%u glyph-input-mutated\n", case_number);
        return 0;
    }
    return 1;
}

int main(int argc, char **argv)
{
    static const unsigned strides[] = {4u, 28u, 42u};
    static const uint16_t fills[] = {0u, 0xffffu, 0xa55au};
    static const uint16_t trails[] = {0x41u, 0x40u};
    unsigned page;
    unsigned stride_index;
    unsigned glyph_kind;
    unsigned fill_index;
    unsigned trail_index;
    unsigned case_number = 0;

    if (argc != 2) {
        fprintf(stderr, "usage: %s retail-glyph.bin\n", argv[0]);
        return EXIT_FAILURE;
    }
    for (page = 0; page <= 1u; page++) {
        for (stride_index = 0; stride_index < 3u; stride_index++) {
            for (glyph_kind = 0; glyph_kind < 4u; glyph_kind++) {
                for (fill_index = 0; fill_index < 3u; fill_index++) {
                    for (trail_index = 0; trail_index < 2u; trail_index++) {
                        case_number++;
                        if (!check_case(argv[1], page, strides[stride_index],
                                        glyph_kind, fills[fill_index],
                                        trails[trail_index], case_number))
                            return EXIT_FAILURE;
                    }
                }
            }
        }
    }
    printf("GLYPH RETAIL DIFFERENTIAL PASS cases=%u pages=2 strides=3 glyphs=4 fills=3 trails=2\n",
           case_number);
    return EXIT_SUCCESS;
}
