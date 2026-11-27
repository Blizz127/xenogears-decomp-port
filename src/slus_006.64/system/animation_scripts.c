/* SPDX-License-Identifier: MIT
 * Native sprite animation, argument decoding and animation package services.
 */
#include "../../../pc_port/src/animation_declarations.h"
#include "animation_data.inc"
#include "../../../pc_port/src/cleanroom/animation/dispatch/dispatch.inc"
#include "../../../pc_port/src/cleanroom/resident_sprite_var_slot.inc"
#include "../../../pc_port/src/cleanroom/resident_color_add_clamp.inc"
#include "../../../pc_port/src/cleanroom/animation/colors_vectors.inc"
#include "../../../pc_port/src/cleanroom/animation/stack.inc"
#include "../../../pc_port/src/cleanroom/animation/animation_checkpoint.inc"
#include "../../../pc_port/src/cleanroom/animation/tail.inc"
#include "../../../pc_port/src/cleanroom/resident_sprite_transforms.inc"
#include "../../../pc_port/src/cleanroom/animation/package_motion.inc"
#include "../../../pc_port/src/cleanroom/resident_sprite_bind_resource.inc"
#include "../../../pc_port/src/cleanroom/animation/direction_seek.inc"
