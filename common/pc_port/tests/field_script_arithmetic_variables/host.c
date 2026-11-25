/* Exercise the same services and actor/script mutations as the bitwise suite. */
#define main runPreviousBitwiseCases
#include "../field_script_bitwise_variables/host.c"
#undef main

#include "../../src/cleanroom/field_script_arithmetic_variables.inc"

static int64_t expectedArithmetic(unsigned command, int a, int b)
{
    if (command == 0) {
        return (int64_t)a + b;
    }
    if (command == 1) {
        return (int64_t)a - b;
    }
    if (command == 2) {
        return (int64_t)a * b;
    }
    if (b == 0) {
        return a;
    }
    int64_t magnitudeA = a < 0 ? -(int64_t)a : a;
    int64_t magnitudeB = b < 0 ? -(int64_t)b : b;
    int64_t magnitude = magnitudeA / magnitudeB;
    return (a < 0) == (b < 0) ? magnitude : -magnitude;
}

int main(void)
{
    const int words[] = { INT_MIN, -65536, -7, -3, -1, 0, 1, 3, 7, 65535, 65536, INT_MAX };
    const uint16_t cursors[] = { 0, 0x1234, 65530, 65535 };
    void (*const commands[])(void) = {
        FieldScriptVMHandlerVariableAdd, FieldScriptVMHandlerVariableSub,
        FieldScriptVMHandlerVariableMul, FieldScriptVMHandlerVariableDiv
    };
    unsigned cases = 0;
    for (unsigned cmd = 0; cmd < 4; ++cmd) {
        for (unsigned c = 0; c < sizeof cursors / sizeof *cursors; ++c) {
            for (unsigned i = 0; i < sizeof words / sizeof *words; ++i) {
                for (unsigned j = 0; j < sizeof words / sizeof *words; ++j) {
                    for (unsigned flags = 0; flags < 256; ++flags) {
                        left = words[i];
                        right = words[j];
                        readCursor = cursors[c];
                        writeCursor = (uint16_t)(cursors[c] + 1u);
                        unsigned offset = (unsigned)readCursor + 5u;
                        if (offset > UINT16_MAX) {
                            streams[1][(uint16_t)offset] = (unsigned char)(flags ^ 0x55u);
                        }
                        streams[0][offset] = (unsigned char)(flags ^ 0xffu);
                        streams[1][offset] = (unsigned char)flags;
                        g_FieldScriptVMCurActor = &actors[2];
                        g_FieldScriptVMCurScriptData = streams[0];
                        count = nindices = 0;
                        commands[cmd]();
                        assert(count == 5 && nindices == 2);
                        expect(0, INDEX, 1, 0);
                        expect(1, LEFT, (uint16_t)firstIndex, 0);
                        expect(2, RIGHT, 3, (int)flags);
                        expect(3, INDEX, 1, 0);
                        expect(4, WRITE, (uint16_t)nextIndex,
                               (uint16_t)expectedArithmetic(cmd, left, right));
                        assert(actors[0].scriptInstructionPointer == readCursor);
                        assert(actors[1].scriptInstructionPointer == readCursor);
                        assert(actors[2].scriptInstructionPointer == (uint16_t)(writeCursor + 6u));
                        ++cases;
                    }
                }
            }
        }
    }
    printf("%u arithmetic calls passed\n", cases);
    return 0;
}
