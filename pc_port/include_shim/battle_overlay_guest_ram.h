#ifndef XENO_SHIM_BATTLE_OVERLAY_GUEST_RAM_H
#define XENO_SHIM_BATTLE_OVERLAY_GUEST_RAM_H
/* Native-only game translation units reach the generated battle-overlay
 * guest-RAM aliases through include/common.h, which every port TU includes.
 * The generator writes the real header next to battle_mips_runtime.c, so this
 * shim keeps the two spellings equivalent. */
#include "../src/battle_overlay_guest_ram.h"
#endif
