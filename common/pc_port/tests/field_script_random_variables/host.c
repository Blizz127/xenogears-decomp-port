#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>

typedef struct { uint16_t scriptInstructionPointer; } ActorData;
ActorData *g_FieldScriptVMCurActor;
void *g_FieldScriptVMCurScriptData;

enum Service { DESTINATION, DRAW, LIMIT, STORE };
static enum Service services[4];
static unsigned callCount;
static int drawValue, limitValue, indexValue;
static int writtenIndex;
static uint16_t writtenValue, storeCursor;
static ActorData actors[5];

static void record(enum Service service)
{
    assert(callCount < 4);
    services[callCount++] = service;
    g_FieldScriptVMCurActor = &actors[callCount];
}

int FieldScriptVMGetInstructionArgument(int offset)
{
    assert(offset == 1);
    record(DESTINATION);
    return indexValue;
}

int rand(void)
{
    record(DRAW);
    return drawValue;
}

int FieldScriptVMGetArgument(int offset)
{
    assert(offset == 3);
    record(LIMIT);
    return limitValue;
}

/* The approved unary fragment also defines increment and decrement commands. */
int FieldScriptVMGetVariableValue(int id)
{
    (void)id;
    assert(0);
    return 0;
}

extern void testRandomWrite(int index, unsigned short value) __asm__("FieldScriptMemoryWriteU16");

void testRandomWrite(int index, unsigned short value)
{
    record(STORE);
    writtenIndex = index;
    writtenValue = value;
    g_FieldScriptVMCurActor->scriptInstructionPointer = storeCursor;
}

#include "../../src/cleanroom/field_script_unary_variables.inc"
#include "../../src/cleanroom/field_script_random_variables.inc"

static void testReserved(void)
{
    g_FieldScriptVMCurActor = NULL;
    g_FieldScriptVMCurScriptData = NULL;
    func_8009CF70();
    assert(g_FieldScriptVMCurActor == NULL && g_FieldScriptVMCurScriptData == NULL);

    ActorData actor;
    unsigned char script = 0xd1;
    g_FieldScriptVMCurActor = &actor;
    g_FieldScriptVMCurScriptData = &script;
    for (unsigned cursor = 0; cursor <= UINT16_MAX; ++cursor) {
        actor.scriptInstructionPointer = (uint16_t)cursor;
        func_8009CF70();
        assert(g_FieldScriptVMCurActor == &actor && g_FieldScriptVMCurScriptData == &script);
        assert(actor.scriptInstructionPointer == cursor && script == 0xd1);
    }
    g_FieldScriptVMCurActor = NULL;
    g_FieldScriptVMCurScriptData = NULL;
    assert(callCount == 0);
    puts("Reserved field command passed 65537 host calls");
}

static int64_t expectedScaled(int draw, int limit)
{
    uint32_t bits = (uint32_t)((int64_t)draw * ((int64_t)limit + 1));
    int64_t signedProduct = bits;
    if (bits > INT32_MAX) {
        signedProduct -= INT64_C(4294967296);
    }
    return signedProduct / 32768 - (signedProduct % 32768 < 0);
}

static void setupRandom(void)
{
    for (unsigned i = 0; i < 5; ++i) {
        actors[i].scriptInstructionPointer = (uint16_t)(100 + i);
    }
    g_FieldScriptVMCurActor = actors;
    callCount = 0;
    writtenIndex = -1;
}

static void checkWrite(unsigned length, int64_t expected)
{
    assert(writtenIndex == (uint16_t)indexValue && writtenValue == (uint16_t)expected);
    assert(g_FieldScriptVMCurActor == &actors[callCount]);
    for (unsigned i = 0; i < 5; ++i) {
        uint16_t cursor = i == callCount ? (uint16_t)(storeCursor + length) : (uint16_t)(100 + i);
        assert(actors[i].scriptInstructionPointer == cursor);
    }
}

static void testRandom(void)
{
    const int draws[] = { INT_MIN, -32769, -1, 0, 1, 2, 12345, 32767, 32768, INT_MAX };
    const int limits[] = { INT_MIN, -65537, -32768, -2, -1, 0, 1, 32767, 65535, 65536, INT_MAX };
    const int indices[] = { 0x12340000, -1, -65534, 0x1234ffff };
    const uint16_t cursors[] = { 0, 0x1234, 65530, 65532, 65535 };
    unsigned plainCases = 0, scaledCases = 0;
    for (unsigned c = 0; c < sizeof cursors / sizeof *cursors; ++c) {
        storeCursor = cursors[c];
        for (unsigned i = 0; i < sizeof indices / sizeof *indices; ++i) {
            indexValue = indices[i];
            for (unsigned d = 0; d < sizeof draws / sizeof *draws; ++d) {
                drawValue = draws[d];
                setupRandom();
                FieldScriptVMHandlerRandVariable();
                assert(callCount == 3 && services[0] == DESTINATION && services[1] == DRAW && services[2] == STORE);
                checkWrite(3, drawValue);
                ++plainCases;

                for (unsigned l = 0; l < sizeof limits / sizeof *limits; ++l) {
                    limitValue = limits[l];
                    setupRandom();
                    FieldScriptVMHandlerMulVariableWithRand();
                    assert(callCount == 4 && services[0] == DRAW && services[1] == LIMIT);
                    assert(services[2] == DESTINATION && services[3] == STORE);
                    checkWrite(5, expectedScaled(drawValue, limitValue));
                    ++scaledCases;
                }
            }
        }
    }
    printf("Random variable commands passed %u unscaled and %u scaled host calls\n", plainCases, scaledCases);
}

int main(void)
{
    testReserved();
    testRandom();
    return 0;
}
