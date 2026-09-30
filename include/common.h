#ifndef _COMMON_H
#define _COMMON_H

#include "include_asm.h"
#include "types.h"

#ifdef XENO_PC_PORT
#include "psx_memory.h"
#endif

/* Battle-overlay D_* guest-RAM aliases are function-like name shadows, so they
 * can only be included by the battle host translation units that use them.
 * A TU that merely *declares* one of these symbols -- e.g. main-exe
 * animation_scripts.c's `extern u8 D_800C3EB0[];` -- would otherwise have its
 * declaration rewritten into nonsense, and would silently start reading guest
 * RAM instead of its own host global. pc_port/build_port.sh sets
 * XENO_BATTLE_OVERLAY_HOST_BODIES for src/battle TUs only. */
#if defined(XENO_PC_PORT) && defined(XENO_BATTLE_OVERLAY_HOST_BODIES)
#include "battle_overlay_guest_ram.h"
#endif

#define PSX_SCRATCH ((void*)0x1F800000)

#define ALIGN(x, a) \
    (((u32)(x) + ((a)-1)) & ~((a)-1))

#define SECTION(x) \
    __attribute__((section(x)))

#define STATIC_ASSERT(cond, msg) \
    typedef char static_assertion_##msg[(cond) ? 1 : -1]

#define STATIC_ASSERT_SIZEOF(type, size) \
    typedef char static_assertion_sizeof_##type[(sizeof(type) == (size)) ? 1 : -1]

#define ARRAY_COUNT(x) (sizeof(x) / sizeof(x[0]))

/* stddef.h (via -include assert.h or direct includes) already defines offsetof;
 * keep this TU's historical definition without tripping -Werror builds. */
#ifdef offsetof
#undef offsetof
#endif
#define offsetof(s,m) ((size_t)&(((s*)0)->m))

#endif