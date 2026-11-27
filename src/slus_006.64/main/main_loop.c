/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_PC_PORT
#error "This main loop requires the native PC port."
#endif

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "main_native_abi.h"
#include "../../../pc_port/src/psx_memory.h"
#include "../../../pc_port/include_shim/lzss_bounds.h"
#include "../../../pc_port/include_shim/game_state_overlay.h"

enum {
    HEAP_ERROR_HANDLER_OFF = 1,
    HEAP_USER_SUGI = 6,
    HEAP_USER_UNKNOWN = 10,
    CTRL_BTN_L1 = 4,
    CTRL_BTN_R1 = 8,
    CTRL_BTN_SELECT = 256,
    CTRL_BTN_START = 2048,
};

/* Track the owned input size and decoded image size for the cached overlay. */
static size_t cachedSourceSize;
static size_t cachedExpectedImageSize;
static int overlayLoadInProgress;

#include "../../../pc_port/src/cleanroom/game_owned_overlay_layout.inc"
#include "../../../pc_port/src/cleanroom/game_overlay_decode.inc"
#include "../../../pc_port/src/cleanroom/game_change_state.inc"
#include "../../../pc_port/src/cleanroom/game_overlay_loader.inc"
#include "../../../pc_port/src/cleanroom/game_dispatch.inc"
#include "../../../pc_port/src/cleanroom/main_loop_controls.inc"
#include "../../../pc_port/src/cleanroom/game_soft_reset.inc"
#include "../../../pc_port/src/cleanroom/main_loop_splash.inc"
#include "../../../pc_port/src/cleanroom/game_diagnostic_display.inc"
