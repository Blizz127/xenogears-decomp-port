#define main runPreviousBitwiseCases
#include "../field_script_bitwise_variables/host.c"
#undef main

#include "../../src/cleanroom/field_script_arithmetic_variables.inc"

/* Model the legacy declaration and publish its link symbol through a separate name. */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstrict-prototypes"
extern int FieldScriptVMGetArgument();
#pragma GCC diagnostic pop

extern int smokeShiftOperand(int offset) __asm__("FieldScriptVMGetArgument");
int smokeShiftOperand(int offset)
{
    /* The shift service has no mode byte. Its trace marker is outside 0..255. */
    record(RIGHT, offset, -1);
    g_FieldScriptVMCurActor = &actors[1];
    return right;
}

#include "../../src/cleanroom/field_script_variable_shifts.inc"

static uint16_t reference(unsigned op, int word, int requested)
{
    unsigned countBits = (uint32_t)requested % 32u;
    int64_t factor = INT64_C(1) << countBits;
    int64_t unsignedWord = (uint32_t)word;
    int bit = (int)((unsignedWord / factor) % 2);
    if (op == 0) {
        return (uint16_t)(bit ? unsignedWord : unsignedWord + factor);
    }
    if (op == 1) {
        return (uint16_t)(bit ? unsignedWord - factor : unsignedWord);
    }
    if (op == 2) {
        return (uint16_t)((int64_t)word * factor);
    }
    int64_t quotient = word / factor;
    if (word < 0 && word % factor != 0) {
        --quotient;
    }
    return (uint16_t)quotient;
}

int main(void)
{
    void (*const commands[])(void) = {
        FieldScriptVMHandlerVariableSetBit, FieldScriptVMHandlerVariableUnsetBit,
        FieldScriptVMHandlerLShiftVariable, FieldScriptVMHandlerRShiftVariable
    };
    const int words[] = { INT_MIN, INT_MIN + 32767, -65537, -7, -1, 0, 1, 7, 65535, 65536, INT_MAX };
    const uint16_t cursors[] = { 0, 319, 65530, 65535 };
    const unsigned char modes[] = { 0, 1, 63, 64, 128, 255 };
    unsigned cases = 0;
    for (unsigned op = 0; op < 4; ++op) {
        for (unsigned c = 0; c < sizeof cursors / sizeof *cursors; ++c) {
            for (unsigned w = 0; w < sizeof words / sizeof *words; ++w) {
                for (int n = -65; n <= 65; ++n) {
                    for (unsigned m = 0; m < sizeof modes / sizeof *modes; ++m) {
                        readCursor = cursors[c];
                        writeCursor = (uint16_t)(cursors[c] - 1u);
                        unsigned offset = (unsigned)readCursor + 5u;
                        streams[0][offset] = (unsigned char)~modes[m];
                        streams[1][offset] = modes[m];
                        if (offset > UINT16_MAX) {
                            streams[1][(uint16_t)offset] = (unsigned char)~modes[m];
                        }
                        left = words[w];
                        right = n == -65 ? INT_MIN : n == 65 ? INT_MAX : n;
                        count = nindices = 0;
                        commands[op]();
                        assert(count == 5 && nindices == 2);
                        expect(0, INDEX, 1, 0);
                        expect(1, LEFT, (uint16_t)firstIndex, 0);
                        expect(2, RIGHT, 3, op < 2 ? (int)modes[m] : -1);
                        expect(3, INDEX, 1, 0);
                        expect(4, WRITE, (uint16_t)nextIndex, reference(op, left, right));
                        assert(actors[0].scriptInstructionPointer == readCursor);
                        assert(actors[1].scriptInstructionPointer == readCursor);
                        assert(actors[2].scriptInstructionPointer ==
                               (uint16_t)(writeCursor + (op < 2 ? 6u : 5u)));
                        ++cases;
                    }
                }
            }
        }
    }
    printf("%u calls passed\n", cases);
    return 0;
}
