#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* Synthetic records expose only the fields used by this fixture. */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;

typedef struct { u8 canary[0x94]; } AudioManager;

typedef struct {
    u16 active_flag;
    u16 status_flags;
    u16 unk_0x04;
    u8 before_voice_mode[0x32 - 6];
    struct {
        u16 modeFlags;
        u16 untouched;
        u16 flags;
    } voice_data;
    u8 before_unsigned_parameter[0x62 - 0x38];
    u16 unk_0x62;
    u8 before_pitch_parameter[0x6e - 0x64];
    s16 unk_0x6E;
    u8 remainder[0x158 - 0x70];
} AudioElement;

_Static_assert(sizeof(AudioElement) == 0x158, "element extent");
_Static_assert(offsetof(AudioElement, active_flag) == 0x00, "percussion flag");
_Static_assert(offsetof(AudioElement, status_flags) == 0x02, "element status");
_Static_assert(offsetof(AudioElement, unk_0x04) == 0x04, "instrument flags");
_Static_assert(offsetof(AudioElement, voice_data.modeFlags) == 0x32, "voice mode");
_Static_assert(offsetof(AudioElement, voice_data.flags) == 0x36, "voice flags");
_Static_assert(offsetof(AudioElement, unk_0x62) == 0x62, "unsigned parameter");
_Static_assert(offsetof(AudioElement, unk_0x6E) == 0x6e, "scaled signed parameter");

#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_script_voice_flags.inc"
#else
typedef u8 *Command(u8 *, AudioManager *, AudioElement *);
Command SoundScriptPercussionOff, SoundScriptClearUnk04Bit1, SoundScriptToggleUnk04Bit2;
Command SoundScriptSetVoiceFlags2000AndMode, SoundScriptSetVoiceFlags2000ClearMode;
Command SoundScriptSetVoiceFlags4000ClearMode, SoundScriptSetUnk62, SoundScriptSetUnk6E;

enum Kind { CLEAR_BIT, TOGGLE_BIT, SET_MODE, CLEAR_MODE, UNSIGNED_BYTE, SIGNED_SCALE };
struct Spec { Command *run; enum Kind kind; unsigned offset, bits, mode; };
static const struct Spec specs[] = {
    {SoundScriptPercussionOff, CLEAR_BIT, 0, 0x10, 0},
    {SoundScriptClearUnk04Bit1, CLEAR_BIT, 4, 1, 0},
    {SoundScriptToggleUnk04Bit2, TOGGLE_BIT, 4, 2, 0},
    {SoundScriptSetVoiceFlags2000AndMode, SET_MODE, 0x36, 0x2000, 0x20},
    {SoundScriptSetVoiceFlags2000ClearMode, CLEAR_MODE, 0x36, 0x2000, 0x20},
    {SoundScriptSetVoiceFlags4000ClearMode, CLEAR_MODE, 0x36, 0x4000, 0x40},
    {SoundScriptSetUnk62, UNSIGNED_BYTE, 0x62, 0, 0},
    {SoundScriptSetUnk6E, SIGNED_SCALE, 0x6e, 0, 0},
};

struct Guarded { u8 lead[16]; AudioElement element; u8 tail[16]; };
struct Access { void *address; unsigned bytes, write; };
static struct Access sequence[4];
static unsigned count, observed, tested, equalStores, aliasedOperands;
static const void *firstDestination;
static u16 firstValue;

static u16 half(const void *p)
{
    u16 value;
    memcpy(&value, p, sizeof value);
    return value;
}

static void storeHalf(void *p, u16 value) { memcpy(p, &value, sizeof value); }

static void expect(void *address, unsigned bytes, unsigned write)
{
    assert(count < 4);
    sequence[count++] = (struct Access){address, bytes, write};
}

#ifdef OBSERVE_ACCESSES
static void checkAccess(void *address, unsigned bytes, unsigned write)
{
    assert(observed < count);
    const struct Access *event = &sequence[observed++];
    assert(event->address == address && event->bytes == bytes && event->write == write);
    if (observed == count && firstDestination)
        assert(half(firstDestination) == firstValue);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *pc) { (void)pc; }
void __tsan_func_exit(void) {}
#define OBSERVE(n) \
    void __tsan_read##n(void *p) { checkAccess(p, n, 0); } \
    void __tsan_write##n(void *p) { checkAccess(p, n, 1); } \
    void __tsan_volatile_read##n(void *p) { checkAccess(p, n, 0); } \
    void __tsan_volatile_write##n(void *p) { checkAccess(p, n, 1); }
OBSERVE(1)
OBSERVE(2)
OBSERVE(4)
OBSERVE(8)
#undef OBSERVE
#endif

static void test(const struct Spec *spec, unsigned input, unsigned alias)
{
    struct Guarded state, wanted;
    AudioManager manager, savedManager;
    u8 stream[] = {0xbc, (u8)input, 0x72}, savedStream[sizeof stream];
    u8 *record = (u8 *)&state.element;
    u8 *expected = (u8 *)&wanted.element;
    for (unsigned i = 0; i < sizeof state; ++i)
        ((u8 *)&state)[i] = (u8)(i * 53u + input * 31u + alias * 19u);
    memset(&manager, (u8)(input + alias), sizeof manager);
    memcpy(&savedManager, &manager, sizeof manager);
    const u16 patterns[] = {0, 0xffff, 0xaaaa, 0x5555, (u16)(1u << (input % 16)),
                            (u16)~(1u << (input % 16)), (u16)(input * 3251u)};
    const unsigned n = sizeof patterns / sizeof *patterns;
    storeHalf(record + spec->offset, patterns[(input + alias) % n]);
    storeHalf(record + 0x32, patterns[(input * 3u + alias + 1u) % n]);
    storeHalf(record + 2, patterns[(input * 5u + alias + 2u) % n]);
    int readsScript = spec->kind == UNSIGNED_BYTE || spec->kind == SIGNED_SCALE;
    u8 *choices[] = {stream + 1, record, record + 2, record + 3, record + 4,
                     record + 0x32, record + 0x33, record + 0x36, record + 0x37,
                     record + spec->offset, record + spec->offset + 1, NULL};
    u8 *script = choices[alias];
    if (readsScript) {
        if (!script)
            script = stream + 1;
        *script = (u8)input;
        aliasedOperands += script != stream + 1;
    }
    if (sizeof(void *) > sizeof(u32)) {
        assert((uintptr_t)record > UINT32_MAX);
        assert(!script || (uintptr_t)script > UINT32_MAX);
    }
    memcpy(&wanted, &state, sizeof wanted);
    memcpy(savedStream, stream, sizeof stream);
    count = observed = 0;
    firstDestination = NULL;
    u16 old = half(record + spec->offset);
    u16 next = old;
    if (readsScript) {
        expect(script, 1, 0);
        next = *script;
        if (spec->kind == SIGNED_SCALE) {
            expect(record + 2, 2, 0);
            int value = (int)*script * 32;
            if (*script >= 128)
                value -= 8192;
            next = (u16)value;
        }
    } else {
        expect(record + spec->offset, 2, 0);
        if (spec->kind == CLEAR_BIT)
            next = (u16)(old & ~spec->bits);
        else if (spec->kind == TOGGLE_BIT)
            next = (u16)(old ^ spec->bits);
        else {
            expect(record + 0x32, 2, 0);
            next = (u16)(old | spec->bits);
        }
    }
    expect(record + spec->offset, 2, 1);
    storeHalf(expected + spec->offset, next);
    equalStores += next == old;
    if (spec->kind == SET_MODE || spec->kind == CLEAR_MODE || spec->kind == SIGNED_SCALE) {
        unsigned second = spec->kind == SIGNED_SCALE ? 2 : 0x32;
        u16 previous = half(record + second);
        u16 result = spec->kind == SIGNED_SCALE ? (u16)(previous | 0x200) :
                     spec->kind == SET_MODE ? (u16)(previous | spec->mode) :
                                              (u16)(previous & ~spec->mode);
        expect(record + second, 2, 1);
        storeHalf(expected + second, result);
        firstDestination = record + spec->offset;
        firstValue = next;
        equalStores += result == previous;
    }
    AudioManager *context = NULL;
    if (alias % 3 == 1)
        context = &manager;
    if (alias % 3 == 2)
        context = (AudioManager *)(void *)&state.element;
    u8 *ret = spec->run(script, context, &state.element);
    assert(ret == (readsScript ? script + 1 : script));
    assert(!memcmp(&state, &wanted, sizeof state));
    assert(!memcmp(stream, savedStream, sizeof stream));
    assert(!memcmp(&manager, &savedManager, sizeof manager));
#ifdef OBSERVE_ACCESSES
    assert(observed == count);
#endif
    ++tested;
}

int main(void)
{
    for (unsigned command = 0; command < sizeof specs / sizeof *specs; ++command)
        for (unsigned value = 0; value < 256; ++value)
            for (unsigned alias = 0; alias < 12; ++alias)
                test(&specs[command], value, alias);
    assert(equalStores && aliasedOperands == 5120);
    printf("%u calls across eight commands, %u equal stores and %u aliased operands\n",
           tested, equalStores, aliasedOperands);
    return 0;
}
#endif
