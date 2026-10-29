/* One guest-address resolver for the battle runtime (M-a battle ABI).
 *
 * Both the MIPS interpreter's bus (resolve_memory in battle_mips_runtime.c)
 * and the native battle bodies (bg_ptr in include/battle_host_guest.h, and so
 * GPTR/GLOAD in include/battle_guest_abi.h) resolve a 32-bit guest word with
 * this function.  Before it existed the two disagreed on one class of
 * address: a KSEG0/KSEG1 address of a main-executable global that the runtime
 * binds to its native copy.  The interpreter read the native copy while
 * bg_ptr read the stale guest-RAM bytes behind it (design doc P8).
 *
 * Result:
 *  - BATTLE_GUEST_RESOLVED: *out is the host byte for `address`;
 *  - BATTLE_GUEST_INVALID: the address is guest-shaped but has no backing
 *    (a scratchpad access that runs past 4 KiB);
 *  - BATTLE_GUEST_NOT_GUEST: the word is not a guest address.  The caller
 *    decides: the interpreter validates it as a registered host range, a
 *    native body treats it as a raw host address (the port is -no-pie, and
 *    native main-exe services hand the overlay u32 host addresses).
 *
 * `shared` maps a canonical KSEG0 address below the battle overlay to its
 * native copy, or returns NULL; it may itself be NULL (no runtime active, or a
 * test that does not link the runtime).  It is only consulted below
 * BATTLE_GUEST_OVERLAY_BASE, so overlay data always stays in guest RAM. */
#ifndef XENO_BATTLE_GUEST_RESOLVER_H
#define XENO_BATTLE_GUEST_RESOLVER_H

#include <stddef.h>
#include <stdint.h>
#include "psx_memory.h"

#define BATTLE_GUEST_OVERLAY_BASE 0x8006faf0u

enum {
    BATTLE_GUEST_NOT_GUEST = 0,
    BATTLE_GUEST_RESOLVED = 1,
    BATTLE_GUEST_INVALID = 2
};

typedef uint8_t *(*BattleGuestSharedLookup)(void *context, uint32_t canonical,
                                            unsigned width);

static inline int battle_guest_resolve(uint32_t address, unsigned width,
                                       BattleGuestSharedLookup shared,
                                       void *context, uint8_t **out)
{
    uint32_t canonical = address;

    if ((address & 0xfffff000u) == 0x1f800000u) {
        unsigned offset = address & 0xfffu;
        if (offset + width > 4096u)
            return BATTLE_GUEST_INVALID;
        *out = g_PsxScratchpad + offset;
        return BATTLE_GUEST_RESOLVED;
    }
    if ((address & 0xff800000u) == 0x80000000u ||
        (address & 0xff800000u) == 0xa0000000u) {
        /* KSEG0 and KSEG1 alias the same physical RAM; the symbol table is
         * KSEG0-shaped, so canonicalise before the shared lookup. */
        if ((address & 0xff800000u) == 0xa0000000u)
            canonical = address - 0x20000000u;
        if (shared != NULL &&
            (canonical & 0x1fffffffu) < (BATTLE_GUEST_OVERLAY_BASE & 0x1fffffffu)) {
            uint8_t *host = shared(context, canonical, width);
            if (host != NULL) {
                *out = host;
                return BATTLE_GUEST_RESOLVED;
            }
        }
        *out = (uint8_t *)PSX_ADDR(canonical);
        return BATTLE_GUEST_RESOLVED;
    }
    return BATTLE_GUEST_NOT_GUEST;
}

/* The runtime's shared-data lookup for the active battle, exported by
 * battle_mips_runtime.c.  Weak, so a unit test that links battle host TUs
 * without the runtime still links; it then resolves exactly as PSX_ADDR. */
extern uint8_t *PcPort_BattleSharedData(void *context, uint32_t canonical,
                                        unsigned width) __attribute__((weak));

/* The resolver as a native body sees it: the active runtime's shared table,
 * raw host pass-through for any non-guest word. */
static inline void *battle_guest_resolve_native(uint32_t address)
{
    uint8_t *host = NULL;
    /* An undefined weak reference is NULL: no shared table. */
    BattleGuestSharedLookup shared = PcPort_BattleSharedData;

    switch (battle_guest_resolve(address, 1, shared, NULL, &host)) {
    case BATTLE_GUEST_RESOLVED:
        return host;
    case BATTLE_GUEST_INVALID:
        /* Same byte the pre-resolver bg_ptr returned: the masked pad. */
        return g_PsxScratchpad + (address & 0xfffu);
    default:
        return (void *)(uintptr_t)address;
    }
}

#endif
