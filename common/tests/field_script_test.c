/* Standalone behavior checks for the public field-script source. The service
 * adapters use synthetic inputs and only the state read by these routines. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef struct { u16 scriptInstructionPointer; } ActorData;

static s32 D_8005A444, D_800ADC18, D_800B00C0;
static ActorData actor, jumpActor, *g_FieldScriptVMCurActor;
static int actorCalls, projectionCalls, destinationCalls, expectedOffset;
static u32 selection;
static s32 partyActor, projectedX, projectedY, override;

static void check(int condition, const char *description)
{
    if (!condition) {
        fprintf(stderr, "Failed %s\n", description);
        exit(EXIT_FAILURE);
    }
}

static u32 FieldScriptVMGetActorIndex(int offset)
{
    ++actorCalls;
    check(offset == expectedOffset, "selector operand offset");
    D_8005A444 = partyActor;
    return selection;
}

static void FieldProjectActorOriginToScreen(s32 *x, s32 *y)
{
    ++projectionCalls;
    *x = projectedX;
    *y = projectedY;
    D_800ADC18 = override;
    g_FieldScriptVMCurActor = &actor;
}

static int FieldScriptVMGetInstructionArgument(int offset)
{
    ++destinationCalls;
    check(offset == 2, "branch operand offset");
    g_FieldScriptVMCurActor = &jumpActor;
    return 0x10037;
}

#include "field_script_resolve_actor.inc"
#include "field_script_check_actor_on_screen.inc"

static void check_selection(int offset, u32 supplied, s32 lead, u32 expected)
{
    expectedOffset = offset;
    selection = supplied;
    partyActor = lead;
    D_8005A444 = 0;
    actorCalls = 0;
    check(func_8009CD7C(offset) == expected, "selected actor");
    check(actorCalls == 1, "selector call count");
}

static void check_projection(s32 x, s32 y, s32 forced, int advances)
{
    projectedX = x;
    projectedY = y;
    override = forced;
    D_800ADC18 = !forced;
    D_800B00C0 = -7;
    actor.scriptInstructionPointer = UINT16_MAX;
    jumpActor.scriptInstructionPointer = 10;
    g_FieldScriptVMCurActor = &jumpActor;
    projectionCalls = destinationCalls = 0;
    FieldScriptCheckActorOnScreen();
    check(projectionCalls == 1, "projection call count");
    check(destinationCalls == !advances, "destination call count");
    check(actor.scriptInstructionPointer == (advances ? 3 : UINT16_MAX),
          "projected actor cursor and wraparound");
    check(jumpActor.scriptInstructionPointer == (advances ? 10 : 0x37),
          "jump destination and actor after argument service");
    check(D_800B00C0 == (forced ? -7 : 1), "script break state");
}

int main(void)
{
    check_selection(-1, 0, 19, 0);
    check_selection(2, 255, -1, UINT32_MAX);
    check_selection(3, 255, 255, 255);
    check_selection(4, 511, 21, 511);
    check_projection(1, 1, 0, 1);
    check_projection(319, 223, 0, 1);
    check_projection(0, 1, 0, 0);
    check_projection(320, 223, 0, 0);
    check_projection(319, 0, 0, 0);
    check_projection(1, 224, 0, 0);
    check_projection(INT32_MIN, INT32_MAX, 0, 0);
    check_projection(INT32_MIN, INT32_MAX, -1, 1);
    puts("PASS: four actor selections and eight viewport branches");
    return 0;
}
