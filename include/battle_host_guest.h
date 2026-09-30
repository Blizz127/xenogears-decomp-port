/* Guest-address accessors for port-only battle overlay bodies.
 *
 * A port body under XENO_PC_PORT runs natively while the battle overlay's
 * data stays in the loaded retail image (g_PsxRam).  Retail code keeps
 * 32-bit guest addresses in its structs and globals, so a body written from
 * the disassembly is most faithful when it does the same: hold every overlay
 * pointer as a u32 guest address and dereference it through these macros.
 * That keeps the retail 32-bit address arithmetic (and the RAM mirror mask)
 * instead of turning it into host pointer arithmetic.
 *
 * Boundary rules (see pc_port/src/battle_mips_runtime.c, runtime_bridge_call):
 *  - the bridge translates a KSEG0/KSEG1/scratchpad argument into a host
 *    pointer, so a parameter that can carry an address is declared as a
 *    pointer and converted back with BG_ADDR() on entry;
 *  - a callee's pointer parameter gets BG_PTR(guest address);
 *  - main-executable globals are NOT in guest RAM on the port (the runtime
 *    binds them to their native copies), so they are accessed through their
 *    C extern names, never through these macros.
 *
 * Only for XENO_PC_PORT; the matching build never includes this. */
#ifndef XENO_BATTLE_HOST_GUEST_H
#define XENO_BATTLE_HOST_GUEST_H

#ifdef XENO_PC_PORT
#include <stdint.h>
#include "psx_memory.h"
#include "types.h"

/* Mirrors the interpreter's resolve_memory(): KSEG0/KSEG1 addresses are
 * guest RAM (masked to the 2 MiB mirror), the scratchpad is its own buffer,
 * and anything else is a raw native address.  The last case is real: native
 * main-exe services (TimerWorkListAllocateTask, the work-list entries) hand
 * the overlay u32 host addresses -- the port is -no-pie, so they fit -- and
 * retail stores and later dereferences them like any other pointer. */
static inline void* bg_ptr(u32 address) {
    if ((address & 0xFF800000u) == 0x80000000u ||
        (address & 0xFF800000u) == 0xA0000000u) {
        return PSX_ADDR(address);
    }
    if ((address & 0xFFFFF000u) == 0x1F800000u) {
        return g_PsxScratchpad + (address & 0xFFFu);
    }
    return (void*)(uintptr_t)address;
}

static inline u32 bg_addr(const void* pointer) {
    uintptr_t host = (uintptr_t)pointer;
    uintptr_t pad = (uintptr_t)g_PsxScratchpad;
    if (pointer != NULL && host >= pad && host < pad + 0x1000u) {
        return 0x1F800000u | (u32)(host - pad);
    }
    return PsxMemory_GuestAddr(pointer);
}

#define BG_PTR(a) ((u8*)bg_ptr((u32)(a)))
#define BG_ADDR(p) bg_addr((const void*)(p))
#define BG_U8(a) (*(u8*)bg_ptr((u32)(a)))
#define BG_S8(a) (*(s8*)bg_ptr((u32)(a)))
#define BG_U16(a) (*(u16*)bg_ptr((u32)(a)))
#define BG_S16(a) (*(s16*)bg_ptr((u32)(a)))
#define BG_U32(a) (*(u32*)bg_ptr((u32)(a)))
#define BG_S32(a) (*(s32*)bg_ptr((u32)(a)))

/* JALR through a retail function pointer, dispatched the way the
 * interpreter's bridge would (pc_port/src/battle_mips_runtime.c):
 *  - overlay code runs in the interpreter with the raw guest argument words
 *    (PcPort_BattleMipsCallGuest);
 *  - a KSEG0 main-executable address goes through the runtime's callback
 *    dispatcher (one argument, as the work-list adapter uses it);
 *  - anything else is a native function address that a native allocator put
 *    in a packed callback slot (e.g. TimerWorkListDeleteTask); the bridge
 *    calls it with KSEG0-looking arguments translated to host pointers.
 * Returns v0. */
int PcPort_BattleMipsCallGuest(uint32_t target, const uint32_t* args,
                               unsigned argc, uint32_t* result);
int PcPort_BattleMipsDispatchCallback(uint32_t callback, void* argument);
static inline void* bg_arg(u32 value) {
    if ((value & 0xFF800000u) == 0x80000000u ||
        (value & 0xFF800000u) == 0xA0000000u ||
        (value & 0xFFFFF000u) == 0x1F800000u) {
        return bg_ptr(value);
    }
    return (void*)(uintptr_t)value;
}
static inline u32 bg_jalr(u32 target, u32 a0, u32 a1, u32 a2, u32 a3) {
    uint32_t args[4];
    uint32_t result = 0;

    args[0] = a0;
    args[1] = a1;
    args[2] = a2;
    args[3] = a3;
    if (target >= 0x8006FAF0u && target < 0x80200000u) {
        PcPort_BattleMipsCallGuest(target, args, 4, &result);
        return result;
    }
    if ((target & 0xFF800000u) == 0x80000000u) {
        PcPort_BattleMipsDispatchCallback(target, bg_arg(a0));
        return 0;
    }
    return (u32)((uintptr_t (*)(void*, void*, void*, void*))(uintptr_t)target)(
        bg_arg(a0), bg_arg(a1), bg_arg(a2), bg_arg(a3));
}

/* Retail trig entry points.  The decomp names 0x8003F8B0 `rcos` and
 * 0x8003F8CC `rsin`, opposite to PsyCross's conventional exports, and the
 * battle bridge calls PsyCross rsin/rcos for them after masking the angle
 * to 12 bits (runtime_bridge_call).  Bind the real PsyCross symbols by
 * assembler name so the pc_port/include_shim/psyq/libgte.h rsin/rcos macros
 * cannot swap them a second time; bg_rsin/bg_rcos mean the RETAIL entries. */
extern int bg_psycross_rsin_(int a) __asm__("rsin");
extern int bg_psycross_rcos_(int a) __asm__("rcos");
static inline int bg_rsin(int angle) { return bg_psycross_rcos_(angle & 0xFFF); }
static inline int bg_rcos(int angle) { return bg_psycross_rsin_(angle & 0xFFF); }

/* R3000 DIV/DIVU results for the cases C leaves undefined (the interpreter,
 * pc_port/src/battle_mips_adapter.c, models the same values).  Only for
 * divisions retail emits without its own `break 7` zero check. */
static inline u32 bg_divu(u32 a, u32 b) { return b != 0 ? a / b : 0xFFFFFFFFu; }
static inline u32 bg_remu(u32 a, u32 b) { return b != 0 ? a % b : a; }
static inline s32 bg_div(s32 a, s32 b) {
    if (b == 0) return a >= 0 ? -1 : 1;
    if (a == (s32)0x80000000 && b == -1) return a;
    return a / b;
}
static inline s32 bg_rem(s32 a, s32 b) {
    if (b == 0) return a;
    if (a == (s32)0x80000000 && b == -1) return 0;
    return a % b;
}
#endif

#endif
