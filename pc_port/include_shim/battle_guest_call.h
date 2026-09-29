#ifndef XENO_SHIM_BATTLE_GUEST_CALL_H
#define XENO_SHIM_BATTLE_GUEST_CALL_H
/* Port-only.  A native battle-overlay C body that calls an overlay routine
 * which has no C body yet must still run that routine: a generated no-op stub
 * would silently drop its effect.  BATTLE_GUEST_CALL runs the retail bytes of
 * the callee through the battle MIPS runtime instead (the same service the
 * native file-1 controller uses, PcPort_BattleMipsCallGuest), so the host body
 * behaves exactly as the interpreted caller would have.
 *
 * Arguments are raw guest words: convert host pointers with
 * PsxMemory_GuestAddr() before passing them.  The result is the callee's v0.
 * A failed guest call is fatal -- retail would have faulted there too, and
 * resuming as if the helper had run would corrupt battle state quietly.
 *
 * Replace a call site with the direct C call once the callee has an adopted
 * host body. */
#if defined(XENO_PC_PORT)
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int PcPort_BattleMipsCallGuest(uint32_t target, const uint32_t *args,
                              unsigned argc, uint32_t *result);

static inline uint32_t BattleGuestCallN(uint32_t target, unsigned argc,
                                        const uint32_t *args)
{
    uint32_t result = 0;
    if (PcPort_BattleMipsCallGuest(target, args, argc, &result) != 0) {
        fprintf(stderr,
                "[xeno-port][battle-guest-call] guest call 0x%08x failed\n",
                (unsigned)target);
        abort();
    }
    return result;
}

#define BATTLE_GUEST_CALL0(target) BattleGuestCallN((target), 0u, NULL)
#define BATTLE_GUEST_CALL(target, ...)                                      \
    ({                                                                      \
        const uint32_t battle_guest_args_[] = { __VA_ARGS__ };              \
        BattleGuestCallN((target),                                          \
                         (unsigned)(sizeof(battle_guest_args_) /            \
                                    sizeof(battle_guest_args_[0])),         \
                         battle_guest_args_);                               \
    })

/* Guest-address accessors for bodies that follow a guest word (a pointer that
 * retail keeps as a 32-bit address in overlay data) the way the MIPS code does:
 * compute the address in 32-bit arithmetic, then touch g_PsxRam through it. */
#define BATTLE_G8(addr)  (*(u8*)PSX_ADDR((u32)(addr)))
#define BATTLE_G16(addr) (*(u16*)PSX_ADDR((u32)(addr)))
#define BATTLE_G32(addr) (*(u32*)PSX_ADDR((u32)(addr)))
/* A guest word handed to a native main-executable routine that takes a host
 * pointer (HeapFree, LoadImage, ...): translated exactly as the bridge
 * translates an interpreted caller's argument (0 and other non-addresses pass
 * through raw; shared main-exe data resolves to its native copy). */
void *PcPort_BattleGuestWordToHost(uint32_t value);
#define BATTLE_HOST_PTR(word) PcPort_BattleGuestWordToHost((u32)(word))
/* MIPS divu/div never trap: a zero divisor leaves LO = all ones (divu) or
 * +-1 by sign (div).  x86 would raise SIGFPE instead. */
static inline u32 BattleDivu(u32 n, u32 d) { return d != 0 ? n / d : 0xFFFFFFFFu; }
static inline u32 BattleRemu(u32 n, u32 d) { return d != 0 ? n % d : n; }
static inline s32 BattleDiv(s32 n, s32 d)
{
    if (d == 0) return n < 0 ? 1 : -1;
    if (n == (s32)0x80000000 && d == -1) return n;
    return n / d;
}
static inline s32 BattleRem(s32 n, s32 d)
{
    if (d == 0) return n;
    if (n == (s32)0x80000000 && d == -1) return 0;
    return n % d;
}
#endif

#endif
