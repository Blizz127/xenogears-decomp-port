#ifndef PC_PORT_KROM_MAPPING_H
#define PC_PORT_KROM_MAPPING_H
#include <stdint.h>
#include <stddef.h>
/* SCPH-5500 B0:51 mapping, not a host pointer. bios must reference the
 * verified 512KB ROM (SHA256 11052b64...cf8a1fef). Caller validates it.
 * UINT32_MAX is the BIOS out-of-range sentinel. Gap inputs consume the
 * mapping helper's uninitialized sp+0x1C byte: callers must supply real
 * guest state, not invent a default. Only the low 16 code bits are used.
 * No ROM ownership, I/O or address translation is performed here. */
uint32_t PcPortKromMapAddress(const uint8_t *bios, uint32_t code,
                             uint8_t scratch_selector);

typedef enum {
    PC_PORT_KROM_OK,
    PC_PORT_KROM_REJECTED,
    PC_PORT_KROM_NEEDS_SELECTOR,
    PC_PORT_KROM_OUTSIDE_ROM
} PcPortKromResult;

/* Same verified 512KB ROM precondition as the mapper. A NULL selector means
 * unknown guest state: resolve only if all 256 possible mappings agree.
 * For OK, *span borrows length readable bytes from bios, with its lifetime.
 * Otherwise *span is NULL. Only REJECTED denotes the BIOS -1 sentinel;
 * other errors must not be converted into a replacement glyph or sentinel.
 * Does not load/verify/own ROM, substitute RAM, or choose caller stack state.
 * span must be non-NULL; length is in bytes (the title reader requires 32). */
PcPortKromResult PcPortKromResolveRomSpan(const uint8_t *bios, uint32_t code,
                                       const uint8_t *selector, size_t length,
                                       const uint8_t **span);
#endif
