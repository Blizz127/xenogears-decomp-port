#ifndef XENO_BATTLE_RESULTS_HOST_H
#define XENO_BATTLE_RESULTS_HOST_H

/* Native bodies for routines of the retail battle RESULTS module (disc
 * archive 0x10/file 4, linked at guest 0x801DE000, 0x6500 bytes).  Everything
 * here is the contract between pc_port/src/battle_results_host.c and the
 * identity-gated dispatch in battle_mips_runtime.c (battle_results_gate.inc).
 *
 * Status: only the routines named in battle_results_host_leaves.inc run as
 * native C, and only while the module's immutable code bytes in guest RAM
 * hash to the pinned retail identity.  Every other results routine -- and
 * every adopted one whenever the gate is in any doubt -- is executed by the
 * MIPS interpreter (the disclosed CPU fallback).
 *
 * Address rule.  An adopted body never touches guest memory through
 * PSX_ADDR.  The interpreter binds main-executable symbols below the battle
 * overlay (0x8006FAF0) to their native host copies -- g_GameState, for one, is
 * mirrored for 0x2358 bytes, through 0x8006F98C -- while overlay addresses
 * stay in guest RAM.  A raw PSX_ADDR would read and write the stale guest
 * bytes behind those native copies, i.e. different bytes than the retail code
 * it replaces.  Every access therefore goes through PcPort_BattleMipsResolve,
 * which is exactly the interpreter's resolve_memory(): same mapping, same
 * KSEG1 canonicalisation, same store-side invalidation of identity-gated
 * modules.
 */

#include <stdint.h>
#include <string.h>

/* Host pointer for `width` (1, 2 or 4) bytes at guest `address`, resolved the
 * way the interpreter's bus resolves it.  `write` != 0 marks a store and runs
 * the interpreter's store-side invalidation before returning.  Fails closed:
 * an address the interpreter could not access (or a misaligned 2/4-byte
 * access, which the interpreter faults on) never returns NULL to the body --
 * inside an adopted call the call is stopped and reported to the bridge as a
 * failed call, exactly where interpretation would have faulted; outside one it
 * aborts. */
uint8_t *PcPort_BattleMipsResolve(uint32_t address, unsigned width, int write);

/* Adopted-body ABI.  Arguments are the raw guest a0..a3 words (no host
 * pointer translation; a body that takes a guest pointer resolves it through
 * the accessors below).  The return value is stored to v0 as the full 32-bit
 * register: a body must return exactly the v0 the retail bytes leave, which
 * for a void routine is the value its last instruction leaves there (each body
 * documents it).  Any other register the retail bytes leave changed is
 * reproduced through BattleResults_SetExitReg() (HI/LO through
 * BattleResults_SetExitHiLo()), so the caller resumes on the same register
 * file it would after interpretation; each body documents its exit set. */
typedef uint32_t (*BattleResultsHostBody)(uint32_t a0, uint32_t a1,
                                          uint32_t a2, uint32_t a3);

/* Exit registers besides v0, applied by the dispatch after the body returns.
 * The dispatch clears `mask` and `hilo` before each call; bit n set means
 * gpr[n] is stored, `hilo` != 0 means HI and LO are (a body whose retail
 * bytes execute a MULT/MULTU/DIV/DIVU or MTHI/MTLO leaves HI/LO changed, so it
 * reproduces them exactly like a GPR).  Defined by battle_mips_runtime.c
 * (battle_results_gate.inc). */
typedef struct BattleResultsExitRegs {
    uint32_t mask;
    uint32_t gpr[32];
    int hilo;
    uint32_t hi;
    uint32_t lo;
} BattleResultsExitRegs;
extern BattleResultsExitRegs g_BattleResultsExit;

/* Entry registers a body may need besides a0..a3, set by the dispatch before
 * each call.  `sp` is the guest SP at the jal target: a routine that takes a
 * fifth (or later) o32 argument reads it from the caller's outgoing argument
 * area at sp+0x10.., exactly as its retail bytes do (func_801E1370).  `gpr`
 * is the whole guest register file at the jal target (gpr[29] == sp).  The
 * one adopted non-leaf, func_801E3A18, stores the caller's s0-s2 and ra into
 * its guest frame below sp, as its retail prologue does, through the
 * ordinary accessors; leaves never touch the guest stack. */
typedef struct BattleResultsEntryRegs {
    uint32_t sp;
    uint32_t gpr[32];
} BattleResultsEntryRegs;
extern BattleResultsEntryRegs g_BattleResultsEntry;

static inline uint32_t BattleResults_EntrySp(void)
{
    return g_BattleResultsEntry.sp;
}

static inline void BattleResults_SetExitReg(unsigned reg, uint32_t value)
{
    g_BattleResultsExit.mask |= 1u << reg;
    g_BattleResultsExit.gpr[reg] = value;
}

static inline void BattleResults_SetExitHiLo(uint32_t hi, uint32_t lo)
{
    g_BattleResultsExit.hilo = 1;
    g_BattleResultsExit.hi = hi;
    g_BattleResultsExit.lo = lo;
}

/* Typed guest accessors for adopted bodies.  Little-endian like the guest; the
 * multi-byte forms go through memcpy so the host access has no alignment
 * requirement of its own (alignment is enforced by the resolver, as retail's
 * address-error exception would). */
static inline uint8_t BattleResults_ReadU8(uint32_t address)
{
    return *PcPort_BattleMipsResolve(address, 1, 0);
}

static inline uint16_t BattleResults_ReadU16(uint32_t address)
{
    uint16_t value;
    memcpy(&value, PcPort_BattleMipsResolve(address, 2, 0), sizeof(value));
    return value;
}

static inline uint32_t BattleResults_ReadU32(uint32_t address)
{
    uint32_t value;
    memcpy(&value, PcPort_BattleMipsResolve(address, 4, 0), sizeof(value));
    return value;
}

static inline void BattleResults_WriteU8(uint32_t address, uint8_t value)
{
    *PcPort_BattleMipsResolve(address, 1, 1) = value;
}

static inline void BattleResults_WriteU16(uint32_t address, uint16_t value)
{
    memcpy(PcPort_BattleMipsResolve(address, 2, 1), &value, sizeof(value));
}

static inline void BattleResults_WriteU32(uint32_t address, uint32_t value)
{
    memcpy(PcPort_BattleMipsResolve(address, 4, 1), &value, sizeof(value));
}

/* lbu/lb/lhu/lh/lw and sb/sh/sw by guest address. */
#define BR_LBU(address) BattleResults_ReadU8((uint32_t)(address))
#define BR_LB(address) ((int8_t)BattleResults_ReadU8((uint32_t)(address)))
#define BR_LHU(address) BattleResults_ReadU16((uint32_t)(address))
#define BR_LH(address) ((int16_t)BattleResults_ReadU16((uint32_t)(address)))
#define BR_LW(address) BattleResults_ReadU32((uint32_t)(address))
#define BR_SB(address, value) \
    BattleResults_WriteU8((uint32_t)(address), (uint8_t)(value))
#define BR_SH(address, value) \
    BattleResults_WriteU16((uint32_t)(address), (uint16_t)(value))
#define BR_SW(address, value) \
    BattleResults_WriteU32((uint32_t)(address), (uint32_t)(value))

#endif
