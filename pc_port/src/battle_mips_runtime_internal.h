#ifndef XENO_BATTLE_MIPS_RUNTIME_INTERNAL_H
#define XENO_BATTLE_MIPS_RUNTIME_INTERNAL_H

#include <stdint.h>

/* Internal call service used by the identity-gated native file-1 controller.
 * Requires an initialized active battle and its current guest-to-native
 * bridge CPU. It does not select/validate a loaded dynamic module: the caller
 * must establish that identity before using this service for an overlay.
 *
 * Supply 0..7 raw guest words (no host-pointer conversion). A 0x50-byte frame,
 * matching retail controller 801E6CE8, is reserved below the current guest SP;
 * arguments use a0..a3 and SP+0x10/14/18. The initial frame must be 8-byte
 * aligned, within physical KSEG0 RAM and below the runtime's stack top.
 * Target must be an aligned guest-code address within physical RAM.
 *
 * Nonargument integer registers, GP, HI/LO and CP0 are inherited. Unspecified
 * register arguments are zero; unused stack bytes are retained. SP/RA and
 * the instruction/load-delay pipeline belong to the nested call. The caller
 * CPU and bridge context are restored unchanged; the public one-argument
 * callback interface retains its separate existing initialization rules.
 *
 * Returns 0 on halt, -1 on rejection or guest failure. A non-NULL result gets
 * v0 only on success. Argument words are snapshotted before outgoing writes,
 * so args/result may alias that guest frame. The service changes the caller
 * CPU only if the caller explicitly directs result storage into it. Guest
 * memory can be modified by the callee. Guest/frame writes before a failure
 * are not rolled back; callers must propagate failure, not resume as
 * if the guest helper succeeded. This does not bound a callee's own frames.
 */
int PcPort_BattleMipsCallGuest(uint32_t target, const uint32_t *args,
                              unsigned argc, uint32_t *result);

#endif
