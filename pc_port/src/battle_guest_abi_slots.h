/* Hand-maintained companion to the generated battle_overlay_guest_ram.h, for
 * the battle guest ABI TUs only (included by include/battle_guest_abi.h under
 * XENO_BATTLE_OVERLAY_HOST_BODIES).
 *
 *  - Scalar aliases the generator has not emitted yet for the prototype
 *    functions.  Each is guarded, so a regenerated header that adds the same
 *    symbol wins.
 *  - <sym>__slot: the u32 lvalue of a pointer-kind overlay global's guest
 *    word, used by GGET/GSTORE.  The generated alias of such a symbol is an
 *    rvalue (it loads and translates the word), so retail's
 *    `*(void **)&sym = v` cannot be written through it. */
#ifndef XENO_BATTLE_GUEST_ABI_SLOTS_H
#define XENO_BATTLE_GUEST_ABI_SLOTS_H

#include "psx_memory.h"
#include "types.h"

#ifndef D_800D3348
#define D_800D3348 (*((s32 *)PSX_ADDR(0x800D3348))) /* main69 triangle count */
#endif
#ifndef D_800D2F64
#define D_800D2F64 (*((u8 *)PSX_ADDR(0x800D2F64))) /* main69 pass counter */
#endif

#define D_800D2DB4__slot (*((u32 *)PSX_ADDR(0x800D2DB4)))

#endif
