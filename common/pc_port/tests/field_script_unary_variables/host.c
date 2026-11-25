#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    unsigned short scriptInstructionPointer;
} ActorData;

static ActorData actors[2];
ActorData *g_FieldScriptVMCurActor;

enum Action { FETCH, READ, WRITE };
struct Call { enum Action action; int address; };
static struct Call calls[4];
static unsigned callCount;
static int operands[2];
static unsigned operandCount;
static unsigned written;
static int currentValue;
static unsigned actorAfterWrite;
static unsigned short cursorAfterWrite;
static unsigned short beforeWrite[2];

static void record(enum Action action, int address)
{
    assert(callCount < 4);
    calls[callCount++] = (struct Call){ action, address };
}

int FieldScriptVMGetInstructionArgument(int index)
{
    record(FETCH, index);
    assert(operandCount < 2);
    return operands[operandCount++];
}

int FieldScriptVMGetVariableValue(int address)
{
    record(READ, address);
    return currentValue;
}

void FieldScriptMemoryWriteU16(int address, unsigned short value)
{
    record(WRITE, address);
    written = value;
    assert(actors[0].scriptInstructionPointer == beforeWrite[0]);
    assert(actors[1].scriptInstructionPointer == beforeWrite[1]);
    g_FieldScriptVMCurActor = &actors[actorAfterWrite];
    g_FieldScriptVMCurActor->scriptInstructionPointer = cursorAfterWrite;
}

#include "../../src/cleanroom/field_script_unary_variables.inc"

int main(void)
{
    const int values[] = { INT_MIN, INT_MIN + 1, -65537, -1, 0, 1, 65535, 65536, INT_MAX };
    const unsigned short cursors[] = { 0, 1, 0x1234, 65532, 65533, 65534, 65535 };
    void (*const commands[])(void) = {
        FieldScriptVMHandlerVariableSetFalse, FieldScriptVMHandlerVariableSetTrue,
        FieldScriptVMHandlerIncVariable, FieldScriptVMHandlerDecVariable
    };
    unsigned cases = 0;
    for (unsigned command = 0; command < 4; ++command) {
        for (unsigned i = 0; i < sizeof values / sizeof *values; ++i) {
            for (unsigned c = 0; c < sizeof cursors / sizeof *cursors; ++c) {
                for (unsigned target = 0; target < 2; ++target) {
                    for (unsigned r = 0; r < 2; ++r) {
                        beforeWrite[0] = actors[0].scriptInstructionPointer = 37;
                        beforeWrite[1] = actors[1].scriptInstructionPointer = 235;
                        g_FieldScriptVMCurActor = &actors[0];
                        actorAfterWrite = target;
                        cursorAfterWrite = cursors[c];
                        operands[0] = r == 0 ? -1 : 0x12340007;
                        operands[1] = r == 0 ? 0x22220405 : INT_MIN;
                        operandCount = callCount = 0;
                        currentValue = values[i];
                        commands[command]();
                        assert(calls[0].action == FETCH && calls[0].address == 1);
                        if (command < 2) {
                            assert(callCount == 2);
                            assert(calls[1].action == WRITE && calls[1].address == (uint16_t)operands[0]);
                            assert(written == command);
                        } else {
                            assert(callCount == 4);
                            assert(calls[1].action == READ && calls[1].address == (uint16_t)operands[0]);
                            assert(calls[2].action == FETCH && calls[2].address == 1);
                            assert(calls[3].action == WRITE && calls[3].address == (uint16_t)operands[1]);
                            int step = command == 2 ? 1 : -1;
                            assert(written == (uint16_t)((int64_t)currentValue + step));
                        }
                        assert(actors[target].scriptInstructionPointer == (uint16_t)(cursors[c] + 3u));
                        assert(actors[1 - target].scriptInstructionPointer == beforeWrite[1 - target]);
                        ++cases;
                    }
                }
            }
        }
    }
    printf("Field script unary variables passed %u ordered calls\n", cases);
    return 0;
}
