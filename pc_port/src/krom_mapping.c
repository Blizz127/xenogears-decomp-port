#include "krom_mapping.h"

static uint32_t read_halfword(const uint8_t *p) {
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8);
}

uint32_t PcPortKromMapAddress(const uint8_t *bios, uint32_t code,
                             uint8_t scratch_selector) {
    /* RAM 6670..6A44 selects table records. These interval boundaries
     * are instruction operands, not copied ROM font/table payloads. */
    static const struct { uint8_t lead, first, limit; } ranges[] = {
        {0x81,0x40,0x7F}, {0x81,0x80,0xAD}, {0x81,0xB8,0xC0},
        {0x81,0xC8,0xCF}, {0x81,0xDA,0xE9}, {0x81,0xF0,0xF8},
        {0x81,0xFC,0xFD}, {0x82,0x4F,0x59}, {0x82,0x60,0x7A},
        {0x82,0x81,0x9B}, {0x82,0x9F,0xF2}, {0x83,0x40,0x7F},
        {0x83,0x80,0x97}, {0x83,0x9F,0xB7}, {0x83,0xBF,0xD7},
        {0x84,0x40,0x61}, {0x84,0x70,0x7F}, {0x84,0x80,0x92},
        {0x84,0x9F,0xBF}
    };
    uint32_t base, table, record, index;
    code &= 0xFFFF;
    if (code >= 0x8140 && code < 0x84BF) base = 0xBFC66000;
    else if (code >= 0x889F && code < 0x9873) base = 0xBFC69D68;
    else return UINT32_MAX;

    unsigned lead = code >> 8, trail = code & 255;
    if (lead >= 0x81 && lead <= 0x84) {
        record = scratch_selector;
        for (unsigned i = 0; i < sizeof ranges / sizeof *ranges; ++i) {
            if (lead == ranges[i].lead && trail >= ranges[i].first && trail < ranges[i].limit) {
                record = i;
                break;
            }
        }
        table = 0x7300;
    } else {
        record = (lead * 2 + (trail >= 0x7F) - 0x111) & 255;
        table = 0x734C;
    }
    /* Tables were relocated from ROM+FB00 into low RAM by this BIOS.
     * Preserve modulo-32-bit arithmetic, including gap-selected records. */
    const uint8_t *entry = bios + 0xFB00 + table + record * 4;
    index = code - read_halfword(entry) + read_halfword(entry + 2);
    return base + index * 30;
}

PcPortKromResult PcPortKromResolveRomSpan(const uint8_t *bios, uint32_t code,
                                       const uint8_t *selector, size_t length,
                                       const uint8_t **span) {
    *span = NULL;
    uint32_t address = PcPortKromMapAddress(bios, code, selector ? *selector : 0);
    if (!selector) {
        /* Zero is only the comparison anchor, never assumed caller state. */
        for (unsigned candidate = 1; candidate < 256; ++candidate)
            if (PcPortKromMapAddress(bios, code, candidate) != address)
                return PC_PORT_KROM_NEEDS_SELECTOR;
    }
    if (address == UINT32_MAX) return PC_PORT_KROM_REJECTED;
    if (address < 0xBFC00000 || address > 0xBFC80000 ||
        length > 0xBFC80000 - address)
        return PC_PORT_KROM_OUTSIDE_ROM;
    *span = bios + (address - 0xBFC00000);
    return PC_PORT_KROM_OK;
}
