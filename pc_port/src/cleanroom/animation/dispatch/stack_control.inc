/* SPDX-License-Identifier: MIT
 * Animation bytecode uses the actor's packed byte stack.
 */
static int animationStackControl(uint8_t *actor, uint8_t opcode, uint8_t *args)
{
    if (opcode == 0xb4) {
        AnimScriptStackPushU8((SpriteData *)actor, args[0]);
        return 1;
    }
    return 0;
}
