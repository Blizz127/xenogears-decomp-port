#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint16_t u16, u_short;
typedef struct {
    u16 before[4], scriptInstructionPointer, after[8];
} ActorData;
extern ActorData *g_FieldScriptVMCurActor;
extern u16 D_800AFC6C, D_800AFE9C;
int FieldScriptVMGetInstructionArgument(int index);

#ifdef BUILD_COMMANDS
#ifndef INPUT_IMPLEMENTATION
#define INPUT_IMPLEMENTATION "../../src/cleanroom/field_input_script_tests.inc"
#endif
#include INPUT_IMPLEMENTATION
#else
void FieldScriptVMCheckControllerInput(u_short);
void FieldScriptCheckControllerInputExact(u_short);
void func_80096178(void), func_800961A0(void);
ActorData *g_FieldScriptVMCurActor;
u16 D_800AFC6C, D_800AFE9C;

static ActorData actors[3];
static unsigned int calls, expectedCalls;
static int maskWord, jumpWord;
static unsigned count;

#ifdef OBSERVE_ACCESSES
struct Access { const void *address; unsigned size; int write; };
static struct Access events[8];
static unsigned expectedCount, consumed;
static void expect(const void *p, unsigned width, int write)
{
    assert(expectedCount < sizeof events / sizeof events[0]);
    events[expectedCount++] = (struct Access){p, width, write};
}
static void observe(const void *p, unsigned width, int write)
{
    assert(consumed < expectedCount);
    const struct Access *next = &events[consumed++];
    assert(next->address == p && next->size == width && next->write == write);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define DECLARE_ACCESS(n) \
    void __tsan_read##n(void *p) { observe(p,n,0); } \
    void __tsan_volatile_read##n(void *p) { observe(p,n,0); } \
    void __tsan_write##n(void *p) { observe(p,n,1); } \
    void __tsan_volatile_write##n(void *p) { observe(p,n,1); }
DECLARE_ACCESS(1)
DECLARE_ACCESS(2)
DECLARE_ACCESS(4)
DECLARE_ACCESS(8)
#endif

int FieldScriptVMGetInstructionArgument(int index)
{
    ++calls;
    assert(calls <= expectedCalls && index == (calls == 1 ? 1 : 3));
#ifdef OBSERVE_ACCESSES
    observe(&calls, 0, 0);
#endif
    g_FieldScriptVMCurActor = &actors[calls];
    return calls == 1 ? maskWord : jumpWord;
}

static void check(unsigned route, u16 state, u16 required, u16 initial, u16 jump)
{
    ActorData wanted[3];
    int exact = route == 1 || route == 2;
    int matched = exact ? state == required : (state & required) != 0;
    unsigned int target = matched ? 1 : 2;
    u16 cursor = matched ? (u16)(initial + 5) : jump;
    memset(actors, 0x69, sizeof actors);
    for (unsigned i = 0; i < 3; ++i)
        actors[i].scriptInstructionPointer = initial;
    memcpy(wanted, actors, sizeof wanted);
    wanted[target].scriptInstructionPointer = cursor;
    g_FieldScriptVMCurActor = &actors[0];
    D_800AFC6C = route == 2 ? state : (u16)~state;
    D_800AFE9C = route == 3 ? state : (u16)~state;
    const u16 startExact = D_800AFC6C, startAny = D_800AFE9C;
    maskWord = 0x11220000 | required;
    jumpWord = 0x32100000 | jump;
    calls = 0;
    expectedCalls = target;
#ifdef OBSERVE_ACCESSES
    consumed = expectedCount = 0;
    if (route == 2) expect(&D_800AFC6C, 2, 0);
    if (route == 3) expect(&D_800AFE9C, 2, 0);
    for (unsigned i = 0; i < target; ++i) expect(&calls, 0, 0);
    expect(&g_FieldScriptVMCurActor, sizeof g_FieldScriptVMCurActor, 0);
    if (matched) expect(&actors[target].scriptInstructionPointer, 2, 0);
    expect(&actors[target].scriptInstructionPointer, 2, 1);
#endif
    switch (route) {
        case 0: FieldScriptVMCheckControllerInput(state); break;
        case 1: FieldScriptCheckControllerInputExact(state); break;
        case 2: func_80096178(); break;
        case 3: func_800961A0(); break;
    }
    assert(calls == expectedCalls && g_FieldScriptVMCurActor == &actors[target]);
    assert(D_800AFC6C == startExact && D_800AFE9C == startAny);
    assert(!memcmp(actors,wanted,sizeof actors));
#ifdef OBSERVE_ACCESSES
    assert(consumed == expectedCount);
#endif
    ++count;
}

int main(void)
{
    for (unsigned state = 0; state <= UINT16_MAX; ++state) {
        for (unsigned route = 0; route < 4; ++route) {
            check(route, state, state, state, (u16)~state);
            check(route, state, (u16)~state, (u16)(state+3), (u16)(state*5));
            check(route, state, (u16)(1u << (state % 16)), 0xffff, state);
        }
    }
    printf("%u script input calls passed\n",count);
    return 0;
}
#endif
