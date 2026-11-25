#include "common.h"
#include "field/actor.h"
#include "field/main.h"
#include "field/script_vm.h"

extern s32 D_800AFFEC;
extern s32 D_800AFD1C;
extern char D_8006FD44;
extern void func_800379C8(char *format, ...);

static u16 jump_operand(int offset, int literal)
{
    if (literal)
        return (u16)FieldScriptVMGetInstructionArgumentS16(offset);

    return (u16)FieldScriptVMGetVariableValue((u16)FieldScriptVMGetInstructionArgument(offset));
}

static int jump_predicate(int code, int left, int right)
{
    switch (code) {
    case 0:  return left == right;
    case 1:
    case 7:  return left != right;
    case 2:  return left > right;
    case 3:  return left < right;
    case 4:  return left >= right;
    case 5:  return left <= right;
    case 6:
    case 9:  return (left & right) != 0;
    case 8:  return left != 0 || right != 0;
    case 10: return (~left & right) != 0;
    default: return 0;
    }
}

#define JUMP_CONTROL_BYTE \
    (((const u8 *)g_FieldScriptVMCurScriptData)[g_FieldScriptVMCurActor->scriptInstructionPointer + 5])

void FieldScriptVMHandlerConditionalJmp(void)
{
    int mode = JUMP_CONTROL_BYTE & 0xf0;
    int left = 0;
    int right = 0;

    if ((mode & 0x30) == 0) {
        int sign = FIELD_SCRIPT_VM_VAR_SIGNED;

        left = jump_operand(1, mode & 0x80);
        right = jump_operand(3, mode & 0x40);
        if (mode != 0xc0) {
            int variableOffset = (mode & 0x80) ? 3 : 1;
            u16 variableId = (u16)FieldScriptVMGetInstructionArgument(variableOffset);
            sign = FieldScriptVMGetVariableSign(variableId);
        }
        if (sign == FIELD_SCRIPT_VM_VAR_SIGNED) {
            left = (left & 0x7fff) - (left & 0x8000);
            right = (right & 0x7fff) - (right & 0x8000);
        }
    }
    if (jump_predicate(JUMP_CONTROL_BYTE & 0x0f, left, right))
        g_FieldScriptVMCurActor->scriptInstructionPointer += 8;
    else
        g_FieldScriptVMCurActor->scriptInstructionPointer = (u16)FieldScriptVMGetInstructionArgument(6);
}
#undef JUMP_CONTROL_BYTE

/* Encoded depths 5 through 7 refer to halfwords beyond the four declared
 * slots. Use the containing actor's byte representation for all eight. */
static u8 *packed_return_slot(ActorData *actor, unsigned int depth)
{
    return (u8 *)actor + __builtin_offsetof(ActorData, scriptPointersStack) + 2u * depth;
}

static void report_stack_error(void)
{
    if (g_FieldSystemMode == 0)
        func_800379C8(&D_8006FD44, D_800AFD1C);
}

void func_800A17F4(void)
{
    ActorData *actor = g_FieldScriptVMCurActor;
    unsigned int depth = actor->flags12C_0x6;
    u16 returnAddress;

    if (depth == SCRIPT_MAX_STACK_SIZE) {
        report_stack_error();
        D_800B00C0 = 1;
        return;
    }
    returnAddress = (u16)(actor->scriptInstructionPointer + 3);
    __builtin_memcpy(packed_return_slot(actor, depth), &returnAddress, sizeof(returnAddress));
    actor->scriptInstructionPointer = (u16)FieldScriptVMGetInstructionArgument(1);
    actor->flags12C_0x6 = (depth + 1u) & 7u;
}

void func_800A18B8(void)
{
    ActorData *actor = g_FieldScriptVMCurActor;
    unsigned int depth = actor->flags12C_0x6;
    u16 returnAddress;

    if (depth == 0) {
        report_stack_error();
        if (actor->curScriptIndex < ACTOR_MAX_NUM_SCRIPTS) {
            ActorScriptSlot *slot = &actor->scripts[actor->curScriptIndex];
            slot->scriptId = ACTOR_ID_INVALID;
            slot->flags_0x12 = 0xf;
        }
        D_800AFFEC = 1;
        D_800B00C0 = 1;
        return;
    }
    --depth;
    actor->flags12C_0x6 = depth & 7u;
    __builtin_memcpy(&returnAddress, packed_return_slot(actor, depth), sizeof(returnAddress));
    actor->scriptInstructionPointer = returnAddress;
}
