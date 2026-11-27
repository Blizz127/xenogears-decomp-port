/* SPDX-License-Identifier: MIT */
/* One unsigned count per opcode. Load the counts from the player's executable. */
#include "../../../pc_port/src/retail_data.h"

uint8_t D_8004FC40[256];

XENO_RETAIL_DATA_BEGIN(anim_opcode_lengths)
    XENO_RD(D_8004FC40, XENO_RD_SLUS, 0x40440, sizeof D_8004FC40),
XENO_RETAIL_DATA_END(anim_opcode_lengths)
