#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


typedef uint8_t u8;
typedef int8_t s8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef struct { u8 guard[16]; } AudioManager;
typedef struct {
    u16 active_flag;
    u16 status_flags;
    u8 prefix[98];
    u16 octave;
    u8 middle[6];
    s16 unk_0x6E;
    u8 between[4];
    u16 unk_0x74;
    u8 suffix[226];
} AudioElement;

_Static_assert(sizeof(AudioElement) == 0x158, "element size");
_Static_assert(offsetof(AudioElement, octave) == 0x66, "octave offset");
_Static_assert(offsetof(AudioElement, unk_0x6E) == 0x6e, "sequence accumulator offset");
_Static_assert(offsetof(AudioElement, unk_0x74) == 0x74, "sequence parameter offset");

#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_script_pitch_controls.inc"
#else
typedef u8 *Handler(u8 *, AudioManager *, AudioElement *);
Handler SoundScriptRaiseOctave, SoundScriptLowerOctave, SoundScriptSetUnk74, SoundScriptAddUnk6E;
struct Access { void *address; unsigned size; int write; };
static struct Access expectedAccess[8];
static unsigned accesses, consumed;

#ifdef OBSERVE_ACCESSES
static void observe(void *address, unsigned size, int write)
{
    assert(consumed < accesses);
    const struct Access *next = &expectedAccess[consumed++];
    assert(address == next->address && size == next->size && write == next->write);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *caller) { (void)caller; }
void __tsan_func_exit(void) {}
#define DEFINE_ACCESS(size) \
    void __tsan_read##size(void *p) { observe(p, size, 0); } \
    void __tsan_volatile_read##size(void *p) { observe(p, size, 0); } \
    void __tsan_write##size(void *p) { observe(p, size, 1); } \
    void __tsan_volatile_write##size(void *p) { observe(p, size, 1); }
DEFINE_ACCESS(1)
DEFINE_ACCESS(2)
DEFINE_ACCESS(4)
DEFINE_ACCESS(8)
#undef DEFINE_ACCESS
#endif

static void expect(void *address, unsigned size, int write)
{
    assert(accesses < sizeof expectedAccess / sizeof *expectedAccess);
    expectedAccess[accesses++] = (struct Access){address, size, write};
}

struct Guarded {
    u8 before[8];
    AudioElement element;
    u8 after[8];
};
static unsigned cases;

static void finish(const struct Guarded *actual, const struct Guarded *expected)
{
    assert(memcmp(actual, expected, sizeof *actual) == 0);
#ifdef OBSERVE_ACCESSES
    assert(consumed == accesses);
#endif
    ++cases;
}

static void test_parameter(int add, unsigned input, unsigned variant, unsigned seed)
{
    static const u16 starting[] = {0, 1, 0xfff0, 0x7ff0, 0x8000, 0xffff};
    struct Guarded fixture, expected;
    AudioManager manager = {{0}}, managerBefore;
    u8 script[] = {0x51, 0x31, 0x6c, 0xdb};
    u8 savedScript[sizeof script];

    memset(&fixture, (int)(input + variant * 31 + seed * 19), sizeof fixture);
    memset(&manager, (int)(77 + input), sizeof manager);
    u16 initial = starting[seed];
    if (seed == 0 && !add) initial = (u16)(input * 256);
    unsigned target = add ? offsetof(AudioElement, unk_0x6E) : offsetof(AudioElement, unk_0x74);
    unsigned flag = add ? 0x200 : 0x100;
    fixture.element.status_flags = (u16)((input * 109 + variant * 19) | ((seed & 1) ? flag : 0));
    u8 *raw = (u8 *)&fixture.element;
    memcpy(raw + target, &initial, sizeof initial);
    u8 *cursors[] = {
        script + 1, raw + target, raw + target + 1,
        raw + 2, raw + 3, raw + 0x5a, manager.guard + 4
    };
    u8 *cursor = cursors[variant];
    *cursor = (u8)input;
    managerBefore = manager;
    memcpy(savedScript, script, sizeof script);
    memcpy(&expected, &fixture, sizeof fixture);

    accesses = consumed = 0;
    expect(cursor, 1, 0);
    u16 value;
    if (add) {
        expect(raw + target, 2, 0);
        u16 old;
        memcpy(&old, raw + target, sizeof old);
        int delta = (input >= 128 ? (int)input - 256 : (int)input) * 32;
        value = (u16)((int32_t)old + delta);
    } else {
        value = (u16)(input * 256);
    }
    expect(&fixture.element.status_flags, 2, 0);
    expect(raw + target, 2, 1);
    expect(&fixture.element.status_flags, 2, 1);
    memcpy((u8 *)&expected.element + target, &value, sizeof value);
    expected.element.status_flags |= (u16)flag;
    AudioManager *context = variant & 1 ? NULL : &manager;
    Handler *handler = add ? SoundScriptAddUnk6E : SoundScriptSetUnk74;
    assert(handler(cursor, context, &fixture.element) == cursor + 1);
    assert(!memcmp(script, savedScript, sizeof script));
    assert(!memcmp(&manager, &managerBefore, sizeof manager));
    finish(&fixture, &expected);
}

static void test_octave(int raise, unsigned initial, unsigned context)
{
    struct Guarded fixture, expected;
    AudioManager manager = {{0x87}}, oldManager = manager;
    u8 script[] = {0xb7, 0x4a}, oldScript[sizeof script];
    memcpy(oldScript, script, sizeof script);
    memset(&fixture, (int)((initial >> 3) + 73), sizeof fixture);
    fixture.element.octave = (u16)initial;
    memcpy(&expected, &fixture, sizeof fixture);
    expected.element.octave = (u16)(raise ? initial + 12 : initial - 12);

    accesses = consumed = 0;
    expect(&fixture.element.octave, 2, 0);
    expect(&fixture.element.octave, 2, 1);
    u8 *cursors[] = {NULL, script, (u8 *)&fixture.element.octave + 1,
                     (u8 *)&fixture.element.status_flags};
    Handler *handler = raise ? SoundScriptRaiseOctave : SoundScriptLowerOctave;
    u8 *cursor = cursors[context];
    assert(handler(cursor, context & 1 ? &manager : NULL, &fixture.element) == cursor);
    assert(memcmp(&manager, &oldManager, sizeof manager) == 0);
    assert(memcmp(script, oldScript, sizeof script) == 0);
    finish(&fixture, &expected);
}

int main(void)
{
    for (int direction = 0; direction < 2; ++direction) {
        for (unsigned initial = 0; initial <= UINT16_MAX; ++initial)
            test_octave(direction, initial, initial % 4);
    }
    for (int add = 0; add < 2; ++add) {
        for (unsigned operand = 0; operand <= UINT8_MAX; ++operand) {
            for (unsigned variant = 0; variant < 7; ++variant) {
                for (unsigned seed = 0; seed < 6; ++seed)
                    test_parameter(add, operand, variant, seed);
            }
        }
    }
    printf("%u sequence control cases passed\n", cases);
    return 0;
}
#endif
