#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int16_t s16;
typedef int32_t s32;
typedef uint32_t u32;
typedef unsigned int uint;
typedef struct {
    int16_t assignedVoice;
    uint16_t modeFlags;
    int16_t priority;
    uint16_t flags;
    uint8_t remaining[36];
} SoundVoiceData;
_Static_assert(sizeof(SoundVoiceData) == 44, "sound voice record layout");
_Static_assert(offsetof(SoundVoiceData, priority) == 4 && offsetof(SoundVoiceData, flags) == 6, "voice members");

struct State {
    SoundVoiceData *slots[24];
    s32 processing;
    u32 keyOn, keyOff;
    SoundVoiceData low, voices[3];
};

extern struct State live;
#define g_SoundChannels live.slots
#define g_unk_VoicesNeedingProcessing live.processing
#define g_SoundKeyOnFlags live.keyOn
#define g_SoundKeyOffFlags live.keyOff
#define PSX_ADDR(address) ((unsigned char *)&live.low + (address))

#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_voice_channel_lifecycle.inc"
#else
typedef void Command(SoundVoiceData *, u32);
Command SoundAssignVoiceToChannelAndPlay, SoundAssignVoiceToChannelAndStop;
Command SoundReleaseVoiceFromChannel, SoundStopVoiceOnChannel;
enum VoiceCommand { ASSIGN_STOP, ASSIGN_PLAY, RELEASE, STOP };
static Command *const commands[] = {
    SoundAssignVoiceToChannelAndStop, SoundAssignVoiceToChannelAndPlay,
    SoundReleaseVoiceFromChannel, SoundStopVoiceOnChannel
};
struct State live;
static struct State shadow;
static struct Access { void *address; size_t width; int write; } trace[16];
static size_t events, seen;
static unsigned runs;

static void *shadow_at(void *address, size_t width, int write)
{
    uintptr_t offset = (uintptr_t)address - (uintptr_t)&live;
    assert(width <= sizeof live && offset <= sizeof live - width);
    assert(events < sizeof trace / sizeof *trace);
    trace[events++] = (struct Access){address, width, write};
    return (unsigned char *)&shadow + offset;
}

#define WORD_ACCESS(bits) \
    static uint##bits##_t rd##bits(void *address) { \
        uint##bits##_t value; \
        memcpy(&value, shadow_at(address, sizeof value, 0), sizeof value); \
        return value; \
    } \
    static void wr##bits(void *address, uint##bits##_t value) { \
        memcpy(shadow_at(address, sizeof value, 1), &value, sizeof value); \
    }
WORD_ACCESS(16)
WORD_ACCESS(32)
#undef WORD_ACCESS

static SoundVoiceData *rdslot(uint32_t index)
{
    SoundVoiceData *value;
    memcpy(&value, shadow_at(&live.slots[index], sizeof value, 0), sizeof value);
    return value;
}
static void wrslot(uint32_t index, SoundVoiceData *value)
{
    memcpy(shadow_at(&live.slots[index], sizeof value, 1), &value, sizeof value);
}

#ifdef OBSERVE_ACCESSES
static void observe(void *address, size_t width, int write)
{
    assert(seen < events);
    const struct Access *expected = &trace[seen++];
    assert(address == expected->address && width == expected->width && write == expected->write);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *caller) { (void)caller; }
void __tsan_func_exit(void) {}
#define DESCRIBE_ACCESS(n) \
    void __tsan_read##n(void *p) { observe(p, n, 0); } \
    void __tsan_volatile_read##n(void *p) { observe(p, n, 0); } \
    void __tsan_write##n(void *p) { observe(p, n, 1); } \
    void __tsan_volatile_write##n(void *p) { observe(p, n, 1); }
DESCRIBE_ACCESS(1)
DESCRIBE_ACCESS(2)
DESCRIBE_ACCESS(4)
DESCRIBE_ACCESS(8)
#undef DESCRIBE_ACCESS
#endif
/* An ownership request observes the full slot before any field or mask. */
static void expect_assignment(int play, SoundVoiceData *voice, uint32_t index)
{
    SoundVoiceData *incumbent = rdslot(index);
    const uint32_t bit = UINT32_C(1) << index;

    if (incumbent == voice) {
        if (play) {
            uint32_t key_on = rd32(&g_SoundKeyOnFlags);
            wr32(&g_SoundKeyOnFlags, key_on | bit);
        } else {
            uint32_t pending = rd32(&g_unk_VoicesNeedingProcessing);
            wr32(&g_unk_VoicesNeedingProcessing, pending | bit);
        }
        return;
    }

    /* A null guest request can still address the low physical RAM record.
       Keep its original identity for the channel-table store below. */
    SoundVoiceData *requested = voice ? voice : &live.low;
    if (incumbent != NULL) {
        int16_t old_priority = (int16_t)rd16(&incumbent->priority);
        int16_t new_priority = (int16_t)rd16(&requested->priority);
        if (old_priority > new_priority)
            return;
    }

    wr16(&requested->flags, UINT16_MAX);
    uint32_t pending = rd32(&g_unk_VoicesNeedingProcessing);
    wr16(&requested->assignedVoice, (uint16_t)index);
    wrslot(index, voice);
    wr32(&g_unk_VoicesNeedingProcessing, pending | bit);

    uint32_t key_on = rd32(&g_SoundKeyOnFlags);
    wr32(&g_SoundKeyOnFlags, play ? key_on | bit : key_on & ~bit);
}

static void check(enum VoiceCommand command, SoundVoiceData *voice, uint32_t index)
{
    memcpy(&shadow, &live, sizeof live);
    events = seen = 0;
    if (index < 24) {
        uint32_t bit = UINT32_C(1) << index;
        if (command <= ASSIGN_PLAY) {
            expect_assignment(command == ASSIGN_PLAY, voice, index);
        } else if (rdslot(index) == voice) {
            if (command == RELEASE) {
                wrslot(index, NULL);
                uint32_t pending = rd32(&live.processing);
                wr32(&live.processing, pending | bit);
                uint32_t on = rd32(&live.keyOn);
                wr32(&live.keyOn, on & ~bit);
            } else {
                uint32_t off = rd32(&live.keyOff);
                wr32(&live.keyOff, off | bit);
            }
        }
    }
    commands[command](voice, index);
    assert(memcmp(&live, &shadow, sizeof live) == 0);
#ifdef OBSERVE_ACCESSES
    assert(seen == events);
#endif
    ++runs;
}

static void test(enum VoiceCommand command, uint32_t index, unsigned variant, unsigned seed)
{
    static const uint32_t masks[] = {0, UINT32_MAX, UINT32_C(0xa4b7975c)};
    static const int16_t priorities[][2] = {
        {INT16_MIN, INT16_MAX}, {-1, 0}, {0, 0},
        {INT16_MAX, INT16_MIN}, {-4, -5}, {-13, -11}
    };
    memset(&live, (int)(seed * 79 + variant * 7), sizeof live);
    live.processing = (s32)masks[seed];
    live.keyOn = masks[(seed + 1) % 3];
    live.keyOff = masks[(seed + 2) % 3];
    SoundVoiceData *incumbent = &live.voices[0], *request = &live.voices[1];
    for (unsigned i = 0; i < 24; ++i)
        live.slots[i] = &live.voices[2];
    unsigned choice = variant % 6;
    incumbent->priority = priorities[choice][0];
    request->priority = live.low.priority = priorities[choice][1];
    if (variant == 6) incumbent = request;
    if (variant == 7) incumbent = request = NULL;
    if (variant == 8) incumbent = NULL;
    if (variant == 9 || variant == 10) {
        request = NULL;
        incumbent->priority = variant == 9 ? INT16_MIN : INT16_MAX;
        live.low.priority = 0;
    }
#if UINTPTR_MAX > UINT32_MAX
    if (variant == 11 && command >= RELEASE) {
        /* The two identity-only commands must distinguish the high pointer bits. */
        request = (SoundVoiceData *)((uintptr_t)incumbent ^ ((uintptr_t)1 << 32));
    }
#endif
    if (index < 24) live.slots[index] = incumbent;
    check(command, request, index);
    check(command, request, index);
}

int main(void)
{
    const uint32_t invalid[] = {24, 25, 255, 256, UINT32_C(0x40000000),
                                UINT32_C(0x80000017), UINT32_MAX};
    for (unsigned c = ASSIGN_STOP; c <= STOP; ++c) {
        for (unsigned variant = 0; variant < 12; ++variant) {
            for (unsigned seed = 0; seed < 3; ++seed) {
                for (uint32_t channel = 0; channel < 24; ++channel)
                    test((enum VoiceCommand)c, channel, variant, seed);
                for (size_t i = 0; i < sizeof invalid / sizeof *invalid; ++i)
                    test((enum VoiceCommand)c, invalid[i], variant, seed);
            }
        }
    }
    printf("%u voice lifecycle cases passed\n", runs);
    return 0;
}
#endif
