#ifndef PC_PORT_BATTLE_TARGET_ELIGIBILITY_RAM_H
#define PC_PORT_BATTLE_TARGET_ELIGIBILITY_RAM_H
#include <stddef.h>
#include <stdint.h>

/* Isolated data helper, NOT a dispatch/identity gate. Caller owns a stable RAM
 * image throughout the call. Returns 0 with result 0/1, or -1 on invalid input
 * or accessed address, leaving result unchanged. Never writes RAM. Only
 * KSEG0/KSEG1 aliases of the first 2 MiB are supported. No host pointer slots.
 * result must point to separate writable host storage, outside ram. */
int PcPortBattleTargetEligibilityRam(const uint8_t *ram, size_t size,
                                    uint32_t actor, uint32_t target,
                                    uint32_t *result);
#endif
