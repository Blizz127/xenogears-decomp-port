/* SPDX-License-Identifier: MIT
 * Copyright (c) 2026 Blizz127 and contributors.
 */
#include <stdatomic.h>
#include <stdio.h>
#include <string.h>
#include "helpers/animation_dispatch_helpers.inc"
#include "control.inc"
#include "data/opcodes.inc"
#include "data/anchor.inc"
#include "arithmetic/opcodes.inc"
#include "arithmetic/transform.inc"
#include "opening/opcodes.inc"
#include "opening/map_motion.inc"
#include "events/opcodes.inc"
#include "lineage/relations.inc"
#include "stack_control.inc"

static atomic_uint_least32_t unhandledSpriteCommands;

uint32_t PcPort_UnhandledSpriteCommandCount(void)
{
    return atomic_load_explicit(&unhandledSpriteCommands, memory_order_relaxed);
}

static void reportUnhandledSpriteCommand(uint8_t command, const uint8_t *operands)
{
    static atomic_uint_least32_t reported[16];
    static atomic_uint distinct;
    unsigned key = (command == 0xbc && operands) ? 256u + operands[0] : command;
    uint_least32_t bit = UINT32_C(1) << (key % 32u);
    atomic_fetch_add_explicit(&unhandledSpriteCommands, 1, memory_order_relaxed);
    if (atomic_fetch_or_explicit(&reported[key / 32u], bit, memory_order_relaxed) & bit)
        return;
    unsigned index = atomic_fetch_add_explicit(&distinct, 1, memory_order_relaxed);
    if (index == 16u)
        fputs("Sprite animation: further unsupported command warnings suppressed.\n", stderr);
    if (index >= 16u)
        return;
    if (key >= 256u)
        fprintf(stderr, "Sprite animation: skipped unsupported command 0xBC, selector 0x%02X.\n",
                (unsigned)operands[0]);
    else
        fprintf(stderr, "Sprite animation: skipped unsupported command 0x%02X.\n",
                (unsigned)command);
}

void func_8001FBE4(void *actor, uint32_t command, void *operands)
{
    uint8_t id = (uint8_t)command;
    if (id < 0x8a || id > 0xfc || AnimationScriptOpcodeIsNoop(id))
        return;
    if (animationControl(actor, id, operands) || animationData(actor, id, operands) ||
        animationAnchor(actor, id, operands) || animationArithmetic(actor, id, operands) ||
        animationTransform(actor, id, operands) || animationOpening(actor, id, operands) ||
        animationMapOpening(actor, id, operands) || animationEvents(actor, id, operands) ||
        animationRelations(actor, id, operands) ||
        animationStackControl(actor, id, operands))
        return;
    reportUnhandledSpriteCommand(id, operands);
}
