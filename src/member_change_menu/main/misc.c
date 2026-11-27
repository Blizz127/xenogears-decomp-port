/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors
 */
#ifndef XENO_PC_PORT
#error "The member-change menu translation unit requires the native PC port."
#endif

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "system/memory.h"
#include "system/controller.h"
#include "system/menu.h"

/* Native decoded button values used by the menu's input queue. */
#ifndef MENU_INPUT_SQUARE
#define MENU_INPUT_SQUARE 6
#endif
#ifndef MENU_INPUT_TRIANGLE
#define MENU_INPUT_TRIANGLE 7
#endif
#ifndef MENU_INPUT_L1
#define MENU_INPUT_L1 10
#endif
#ifndef MENU_INPUT_R1
#define MENU_INPUT_R1 9
#endif

#ifdef _WIN32
void *bzero(void *destination, size_t length);
#else
#include <strings.h>
#endif

#include "member_data.h"
#include "member_calls.h"

#include "../../../pc_port/src/cleanroom/menu_system_palette.inc"
#include "../../../pc_port/src/cleanroom/member_menu_storage.inc"
#include "../../../pc_port/src/cleanroom/member_initialize.inc"
#include "../../../pc_port/src/cleanroom/member_label_pairs.inc"
#include "../../../pc_port/src/cleanroom/member_label_init.inc"
#include "../../../pc_port/src/cleanroom/member_window_atlas.inc"
#include "../../../pc_port/src/cleanroom/member_menu_window_graphics.inc"
#include "../../../pc_port/src/cleanroom/member_window_scroll_init.inc"
#include "../../../pc_port/src/cleanroom/member_window_corner_init.inc"
#include "../../../pc_port/src/cleanroom/member_name_strip.inc"
#include "../../../pc_port/src/cleanroom/member_menu_number.inc"
#include "../../../pc_port/src/cleanroom/member_roster_swap.inc"
#include "../../../pc_port/src/cleanroom/member_menu_selection.inc"
#include "../../../pc_port/src/cleanroom/member_change_state.inc"
#include "../../../pc_port/src/cleanroom/member_change_render.inc"
#include "../../../pc_port/src/cleanroom/member_change_main.inc"
#include "../../../pc_port/src/cleanroom/member_change_character.inc"
#include "../../../pc_port/src/cleanroom/member_change_services.inc"
#include "../../../pc_port/src/cleanroom/member_change_window.inc"
#include "../../../pc_port/src/cleanroom/member_window_edges.inc"
#include "../../../pc_port/src/cleanroom/member_window_parts.inc"
#include "../../../pc_port/src/cleanroom/member_menu_window_shape.inc"
#include "../../../pc_port/src/cleanroom/member_menu_window_edges.inc"
#include "../../../pc_port/src/cleanroom/member_menu_lifecycle.inc"
#include "../../../pc_port/src/cleanroom/member_menu_frame_input_windows.inc"
#include "../../../pc_port/src/cleanroom/member_change_character_selection.inc"
#include "../../../pc_port/src/cleanroom/member_change_render_characters.inc"
#include "../../../pc_port/src/cleanroom/member_change_character_strings.inc"
#include "../../../pc_port/src/cleanroom/member_change_character_labels.inc"
#include "../../../pc_port/src/cleanroom/member_menu_cursors.inc"
#include "../../../pc_port/src/cleanroom/member_menu_roster_windows.inc"
#include "../../../pc_port/src/cleanroom/member_change_initialize_background.inc"
#include "../../../pc_port/src/cleanroom/member_change_load_resources.inc"
#include "../../../pc_port/src/cleanroom/member_change_string_packets.inc"
