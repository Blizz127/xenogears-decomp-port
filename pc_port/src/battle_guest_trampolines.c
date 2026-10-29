/* Typed guest-call trampolines for battle-overlay functions that adopted
 * native bodies call directly but that have no native body yet.
 *
 * Without a definition the link would get a generated stub that logs once
 * and returns 0, silently skipping the retail work.  A trampoline instead
 * makes the retail call through the battle runtime
 * (PcPort_BattleMipsCallGuest): an adopted guest-ABI body stays native, and
 * anything else needs the CPU interpreter.  In a native-only build that is
 * absent, so the call fails and this aborts naming the target -- a visible
 * native frontier, never a silent skip.  Arguments are scalar words, or
 * host pointers converted to the guest word explicitly per entry.
 *
 * Remove an entry once its function has a native body. */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "psx_memory.h"

int PcPort_BattleMipsCallGuest(uint32_t target, const uint32_t* args,
                               unsigned argc, uint32_t* result);

static uint32_t battle_guest_trampoline(const char* name, uint32_t target,
                                        const uint32_t* args, unsigned argc)
{
    uint32_t result = 0;

    if (PcPort_BattleMipsCallGuest(target, args, argc, &result) != 0) {
        fprintf(stderr, "[xeno-port][battle-trampoline] %s (0x%08x) has no "
                "native body and its retail call failed\n", name, target);
        abort();
    }
    return result;
}

/* func_800B89FC (mainc104_port.c) callees without a linked native body.
 * func_800BFBA0 has retail C in mainc132.c, which is not an adopted host
 * TU; drop its entry if that TU is ever linked (duplicate symbol). */
void func_800BE790(void)
{
    battle_guest_trampoline("func_800BE790", 0x800BE790u, NULL, 0);
}

void func_800BEE2C(uint32_t slot, uint32_t arg, uint32_t mode)
{
    const uint32_t args[3] = { slot, arg, mode };

    battle_guest_trampoline("func_800BEE2C", 0x800BEE2Cu, args, 3);
}

void func_800BFBA0(void)
{
    battle_guest_trampoline("func_800BFBA0", 0x800BFBA0u, NULL, 0);
}

/* Battle frame-pump phase draws (80076544 -> 800764B4/80076418/800764EC,
 * main2.c/main3.c): argument-less overlay steps whose byte-exact C is
 * retail-only in main2.c. */
#define BATTLE_VOID_TRAMPOLINE(addr)                                        \
    void func_##addr(void)                                                  \
    {                                                                       \
        battle_guest_trampoline("func_" #addr, 0x##addr##u, NULL, 0);       \
    }
BATTLE_VOID_TRAMPOLINE(80073538)
BATTLE_VOID_TRAMPOLINE(80073A58)
BATTLE_VOID_TRAMPOLINE(80073B64)
BATTLE_VOID_TRAMPOLINE(80073E88)
BATTLE_VOID_TRAMPOLINE(80073F08)
BATTLE_VOID_TRAMPOLINE(80074EEC)
BATTLE_VOID_TRAMPOLINE(80074F70)
BATTLE_VOID_TRAMPOLINE(8007500C)
BATTLE_VOID_TRAMPOLINE(8008FAD8)
BATTLE_VOID_TRAMPOLINE(801DE594)

/* Callees of the native command-step bodies (func_800B905C mainc107_port.c,
 * func_800BF4F0 mainc127_port.c, func_800B7E94 mainc100_port.c) that have no
 * linked native body yet.  Sprite pointers go to the guest as their KSEG0
 * word or raw compact host address, as bg_addr() converts them. */
void func_800BF1EC(uint8_t* sprite, int32_t anim)
{
    const uint32_t args[2] = { PsxMemory_GuestAddr(sprite), (uint32_t)anim };

    battle_guest_trampoline("func_800BF1EC", 0x800BF1ECu, args, 2);
}

/* Registers the battle file's SED/WDS entries; returns the SED entry. */
uint32_t func_800C0FAC(uint8_t* base)
{
    const uint32_t args[1] = { PsxMemory_GuestAddr(base) };

    return battle_guest_trampoline("func_800C0FAC", 0x800C0FACu, args, 1);
}

/* Stubs reached from the native command-step bodies through legacy host
 * bodies: func_800BF600 -> func_800B7C34 (battle file loader, id as the
 * word), func_800BC404 -> func_800BC2F0 (mode word) and func_800C0314 ->
 * func_800B136C -> func_800B0060 (sprite record).  Typed here so no
 * generated stub is reachable from the adopted leaves. */
void func_800B7C34(uint32_t id)
{
    const uint32_t args[1] = { id };

    battle_guest_trampoline("func_800B7C34", 0x800B7C34u, args, 1);
}

void func_800BC2F0(uint32_t mode)
{
    const uint32_t args[1] = { mode };

    battle_guest_trampoline("func_800BC2F0", 0x800BC2F0u, args, 1);
}

void func_800B0060(uint8_t* sprite)
{
    const uint32_t args[1] = { PsxMemory_GuestAddr(sprite) };

    battle_guest_trampoline("func_800B0060", 0x800B0060u, args, 1);
}

/* func_80080160 (main34_q1_port.c) callees with no linked native body: the
 * frame wait (hosted by the runtime's frame-wait bridge, not the CPU), the
 * Magic/Item-ring entry func_8007FF14 and the seventeen command-menu state
 * handlers func_80081318..func_80083948 (asm-only today; func_80081B58 has
 * its native body in mainasm_12068_port.c).  Each handler takes the actor
 * byte (the caller passes it masked); func_80083948 returns a word the driver
 * ignores. */
BATTLE_VOID_TRAMPOLINE(800716D8)

#define BATTLE_ACTOR_TRAMPOLINE(addr)                                       \
    void func_##addr(uint32_t actor)                                        \
    {                                                                       \
        const uint32_t args[1] = { actor };                                 \
        battle_guest_trampoline("func_" #addr, 0x##addr##u, args, 1);       \
    }
#define BATTLE_ACTOR_RESULT_TRAMPOLINE(addr)                                \
    int32_t func_##addr(uint32_t actor)                                     \
    {                                                                       \
        const uint32_t args[1] = { actor };                                 \
        return (int32_t)battle_guest_trampoline("func_" #addr, 0x##addr##u, \
                                                args, 1);                   \
    }
BATTLE_ACTOR_TRAMPOLINE(8007FF14)
BATTLE_ACTOR_TRAMPOLINE(80081318)
BATTLE_ACTOR_TRAMPOLINE(80081504)
BATTLE_ACTOR_TRAMPOLINE(800816F8)
BATTLE_ACTOR_TRAMPOLINE(800820A4)
BATTLE_ACTOR_TRAMPOLINE(800822C4)
BATTLE_ACTOR_TRAMPOLINE(80082504)
BATTLE_ACTOR_TRAMPOLINE(80082820)
BATTLE_ACTOR_TRAMPOLINE(800829F4)
BATTLE_ACTOR_TRAMPOLINE(80082BB0)
BATTLE_ACTOR_TRAMPOLINE(80082D4C)
BATTLE_ACTOR_TRAMPOLINE(80082F7C)
BATTLE_ACTOR_TRAMPOLINE(800830A8)
BATTLE_ACTOR_TRAMPOLINE(80083340)
BATTLE_ACTOR_TRAMPOLINE(80083580)
BATTLE_ACTOR_TRAMPOLINE(80083748)
BATTLE_ACTOR_RESULT_TRAMPOLINE(80083948)

/* func_80081B58 (mainasm_12068_port.c) callees with no linked native body:
 * func_8008189C / func_800819A4 (retail C in mainasm_11828.c, retail-only),
 * func_800861D0 / the battle func_800877E0 (main38_p2.c, retail-only; named
 * xeno_battle_guest_800877E0 because the world-map overlay's own
 * func_800877E0, src/world_map/main.c, is linked into the port), func_800B8DA4
 * (main106.c, retail-only) and func_80084854 (port arm in main35.c, which is
 * not an adopted host TU; drop its entry if that TU is ever linked).  The
 * word-returning ones are used by retail (func_80084854 and the ignored
 * func_800877E0 result are read as full words; the caller stores the low
 * byte of the first). */
BATTLE_ACTOR_TRAMPOLINE(8008189C)
BATTLE_ACTOR_TRAMPOLINE(800819A4)
BATTLE_VOID_TRAMPOLINE(800B8DA4)

void func_800861D0(uint32_t state, uint32_t actor)
{
    const uint32_t args[2] = { state, actor };

    battle_guest_trampoline("func_800861D0", 0x800861D0u, args, 2);
}

uint32_t xeno_battle_guest_800877E0(uint32_t actor, uint32_t target)
{
    const uint32_t args[2] = { actor, target };

    return battle_guest_trampoline("func_800877E0", 0x800877E0u, args, 2);
}

uint32_t func_80084854(uint32_t target, uint32_t direction)
{
    const uint32_t args[2] = { target, direction };

    return battle_guest_trampoline("func_80084854", 0x80084854u, args, 2);
}

/* Command-menu state-5 handler func_80081B58 reach (main37/39 bodies). */
void func_80079840(uint8_t actor, uint8_t target)
{
    const uint32_t args[2] = { actor, target };

    battle_guest_trampoline("func_80079840", 0x80079840u, args, 2);
}

uint32_t func_80079AB0(uint8_t actor)
{
    const uint32_t args[1] = { actor };

    return battle_guest_trampoline("func_80079AB0", 0x80079AB0u, args, 1);
}

uint32_t func_800941A4(void)
{
    return battle_guest_trampoline("func_800941A4", 0x800941A4u, NULL, 0);
}

/* Battle-range names that another overlay's game TU also defines
 * (world map func_800877E0, field misc7 func_8009A1AC).  build_port.sh
 * renames battle host objects' references to them to battle_<name>, so a
 * battle caller lands here instead of in the other overlay's body. */
uint32_t battle_func_800877E0(uint8_t actor, uint8_t target)
{
    const uint32_t args[2] = { actor, target };

    return battle_guest_trampoline("func_800877E0", 0x800877E0u, args, 2);
}

uint32_t battle_func_8009A1AC(uint32_t index)
{
    const uint32_t args[1] = { index };

    return battle_guest_trampoline("func_8009A1AC", 0x8009A1ACu, args, 1);
}
