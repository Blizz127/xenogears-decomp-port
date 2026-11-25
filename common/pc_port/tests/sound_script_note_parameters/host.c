#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef struct { u8 guard[16]; } AudioManager;
typedef struct {
    u8 prefix[6];
    u16 flags;
    u8 middle[31];
    u8 unkAdsr4;
    u8 adjacent;
    u8 unkAdsr5;
    u8 suffix[2];
} Voice;
typedef struct {
    u16 active_flag;
    u8 prefix[46];
    Voice voice_data;
    int16_t fermataDuration;
    u8 middle[8];
    u16 octave;
    u8 suffix[240];
} AudioElement;

_Static_assert(sizeof(AudioElement) == 0x158, "element size");
_Static_assert(offsetof(AudioElement, voice_data.flags) == 0x36, "voice flags");
_Static_assert(offsetof(AudioElement, voice_data.unkAdsr4) == 0x57, "attack");
_Static_assert(offsetof(AudioElement, voice_data.unkAdsr5) == 0x59, "sustain");
_Static_assert(offsetof(AudioElement, fermataDuration) == 0x5c, "duration");
_Static_assert(offsetof(AudioElement, octave) == 0x66, "octave");

#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_script_note_parameters.inc"
#else
typedef u8 *Handler(u8 *, AudioManager *, AudioElement *);
Handler SoundScriptSetOctave, SoundScriptFermata, SoundScriptSetAttackTime, SoundScriptSetSustain;

struct Access { void *address; unsigned size; int write; };
static struct Access expectedAccess[4];
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
    assert(accesses < 4);
    expectedAccess[accesses++] = (struct Access){address, size, write};
}

struct Command {
    Handler *run;
    unsigned flags, mask, destination, width, scale;
};

static void test(struct Command cmd, unsigned input, unsigned variant)
{
    struct Guarded { u8 prefix[8]; AudioElement element; u8 suffix[8]; } guarded, expected;
    AudioManager manager, previousManager;
    u8 script[5] = {0xed, 0, 0xd1, 0xcc, 0xaa}, previousScript[sizeof script];

    memset(&guarded, (int)(input * 37 + variant * 19), sizeof guarded);
    memset(&manager, (int)(input + variant), sizeof manager);
    previousManager = manager;
    u8 *raw = (u8 *)&guarded.element;
    u8 *locations[] = {script + 1, raw + cmd.destination, raw + cmd.destination + 1,
                       raw + cmd.flags, raw + cmd.flags + 1, raw + 0x6e};
    u8 *cursor = locations[variant];
    *cursor = (u8)input;
    memcpy(previousScript, script, sizeof script);
    memcpy(&expected, &guarded, sizeof guarded);

    u8 *newBytes = (u8 *)&expected.element;
    accesses = consumed = 0;
    if (cmd.mask)
        expect(raw + cmd.flags, 2, 0);
    expect(cursor, 1, 0);
    if (cmd.mask) {
        u16 updated;
        memcpy(&updated, raw + cmd.flags, 2);
        updated |= (u16)cmd.mask;
        memcpy(newBytes + cmd.flags, &updated, 2);
        expect(raw + cmd.flags, 2, 1);
    }
    if (cmd.width == 1) {
        newBytes[cmd.destination] = (u8)input;
    } else {
        u16 value = (u16)(input * cmd.scale);
        memcpy(newBytes + cmd.destination, &value, 2);
    }
    expect(raw + cmd.destination, cmd.width, 1);
    AudioManager *context = variant == 0 ? &manager : NULL;
    assert(cmd.run(cursor, context, &guarded.element) == cursor + 1);
    assert(!memcmp(&guarded, &expected, sizeof guarded));
    assert(!memcmp(&manager, &previousManager, sizeof manager));
    assert(!memcmp(script, previousScript, sizeof script));
#ifdef OBSERVE_ACCESSES
    assert(consumed == accesses);
#endif
}

int main(void)
{
    const struct Command commands[] = {
        {SoundScriptSetOctave, 0, 0, 0x66, 2, 12},
        {SoundScriptFermata, 0, 0x100, 0x5c, 2, 1},
        {SoundScriptSetAttackTime, 0x36, 0x10, 0x57, 1, 1},
        {SoundScriptSetSustain, 0x36, 0x40, 0x59, 1, 1},
    };
    for (size_t c = 0; c < sizeof commands / sizeof *commands; ++c)
        for (unsigned input = 0; input < 256; ++input)
            for (unsigned variant = 0; variant < 6; ++variant)
                test(commands[c], input, variant);
    puts("6144 note-parameter cases passed");
    return 0;
}
#endif
