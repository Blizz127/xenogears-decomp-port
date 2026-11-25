#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

typedef struct { uint16_t scriptInstructionPointer; } ActorData;
static ActorData actors[3];
static unsigned char streams[2][65541];
ActorData *g_FieldScriptVMCurActor;
void *g_FieldScriptVMCurScriptData;

enum Action { INDEX, LEFT, RIGHT, WRITE };
struct Call { enum Action action; int address; int value; };
static struct Call calls[5];
static unsigned count, nindices;
static int left, right;
static uint16_t readCursor, writeCursor;
static const int firstIndex = 0x12340007;
static const int nextIndex = -135802;

static void record(enum Action action, int address, int value)
{
    assert(count < 5);
    calls[count++] = (struct Call){ action, address, value };
}

int FieldScriptVMGetInstructionArgument(int offset)
{
    record(INDEX, offset, 0);
    g_FieldScriptVMCurActor = &actors[0];
    actors[0].scriptInstructionPointer = readCursor;
    g_FieldScriptVMCurScriptData = streams[0];
    return ++nindices == 1 ? firstIndex : nextIndex;
}

int FieldScriptVMGetVariableValue(int id)
{
    record(LEFT, id, 0);
    g_FieldScriptVMCurActor = &actors[1];
    actors[1].scriptInstructionPointer = readCursor;
    g_FieldScriptVMCurScriptData = streams[1];
    return left;
}

int FieldScriptArgument2(int offset, int flags)
{
    record(RIGHT, offset, flags);
    g_FieldScriptVMCurActor = &actors[1];
    g_FieldScriptVMCurScriptData = streams[0];
    return right;
}

extern void FieldScriptMemoryWriteU16(int address, int value);
void smokeVariableWrite(int address, unsigned short value) __asm__("FieldScriptMemoryWriteU16");

void smokeVariableWrite(int address, unsigned short value)
{
    record(WRITE, address, value);
    g_FieldScriptVMCurActor = &actors[2];
    actors[2].scriptInstructionPointer = writeCursor;
}

#include "../../src/cleanroom/field_script_bitwise_variables.inc"
#include "../../src/cleanroom/field_script_unary_variables.inc"

static void expect(unsigned i, enum Action action, int address, int value)
{
    assert(calls[i].action == action);
    assert(calls[i].address == address);
    assert(calls[i].value == value);
}

int main(void)
{
    const int words[] = { INT_MIN, -1, 0, 0x3505a553, INT_MAX };
    const uint16_t cursors[] = { 0, 1, 297, 65530, 65534, 65535 };
    void (*const commands[])(void) = {
        FieldScriptVMHandlerVariableAssign, FieldScriptVMHandlerVariableAND,
        FieldScriptVMHandlerVariableOR, FieldScriptVMHandlerVariableXOR
    };
    unsigned cases = 0;
    for (unsigned cmd = 0; cmd < 4; ++cmd) {
        for (unsigned c = 0; c < sizeof cursors / sizeof *cursors; ++c) {
            for (unsigned i = 0; i < sizeof words / sizeof *words; ++i) {
                for (unsigned j = 0; j < sizeof words / sizeof *words; ++j) {
                    for (unsigned flags = 0; flags < 256; ++flags) {
                        readCursor = cursors[c];
                        writeCursor = (uint16_t)(cursors[c] - 1u);
                        unsigned commandOffset = (unsigned)readCursor + 5u;
                        if (commandOffset > UINT16_MAX) {
                            streams[0][(uint16_t)commandOffset] = (unsigned char)(flags ^ 37u);
                            streams[1][(uint16_t)commandOffset] = (unsigned char)(flags ^ 83u);
                        }
                        streams[0][commandOffset] = (unsigned char)flags;
                        streams[1][commandOffset] = (unsigned char)(255u - flags);
                        left = words[i];
                        right = words[j];
                        g_FieldScriptVMCurActor = &actors[2];
                        actors[0].scriptInstructionPointer = 55;
                        actors[1].scriptInstructionPointer = 11;
                        actors[2].scriptInstructionPointer = 77;
                        g_FieldScriptVMCurScriptData = streams[1];
                        count = nindices = 0;
                        commands[cmd]();
                        expect(0, INDEX, 1, 0);
                        if (cmd == 0) {
                            assert(count == 3 && nindices == 1);
                            expect(1, RIGHT, 3, (int)flags);
                            expect(2, WRITE, (uint16_t)firstIndex, (uint16_t)right);
                        } else {
                            int result = cmd == 1 ? left & right : cmd == 2 ? left | right : left ^ right;
                            assert(count == 5 && nindices == 2);
                            expect(1, LEFT, (uint16_t)firstIndex, 0);
                            expect(2, RIGHT, 3, (int)(255u - flags));
                            expect(3, INDEX, 1, 0);
                            expect(4, WRITE, (uint16_t)nextIndex, (uint16_t)result);
                        }
                        assert(actors[0].scriptInstructionPointer == readCursor);
                        assert(actors[1].scriptInstructionPointer == (cmd == 0 ? 11 : readCursor));
                        assert(actors[2].scriptInstructionPointer == (uint16_t)(writeCursor + 6u));
                        ++cases;
                    }
                }
            }
        }
    }
    printf("Field variable assignment and bitwise handlers passed %u ordered cases\n", cases);
    return 0;
}
