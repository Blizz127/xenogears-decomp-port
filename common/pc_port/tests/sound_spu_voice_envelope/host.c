#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef union { volatile u16 raw[256]; } SpuUnion;
_Static_assert(sizeof(SpuUnion) == 512, "native modeled SPU page");
extern SpuUnion *g_pSoundSpuRegisters;

#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_spu_voice_envelope.inc"
#else
void SoundSetVoiceAdsrAttackModeAndRate(s32, s32, s32);
void SoundSetVoiceAdsrDecayShift(s32, s32);
void SoundSetVoiceAdsrSustainLevel(s32, s32);
void SoundSetVoiceAdsrSustainRateAndDirection(s32, s32, s32);
SpuUnion *g_pSoundSpuRegisters;

struct Bank { u16 lead[8]; SpuUnion page; u16 tail[8]; };
struct Pages { struct Bank banks[2]; };
struct Event { volatile void *address; unsigned bytes, write; };
static struct Event expectedEvents[3];
static unsigned expectedCount, consumed, calls, outside, equalStores;
static SpuUnion *alternate;
static int redirect;

static void expect(volatile void *address, unsigned bytes, unsigned write)
{
    assert(expectedCount < 3);
    expectedEvents[expectedCount++] = (struct Event){address, bytes, write};
}

#ifdef OBSERVE_ACCESSES
static void observe(void *address, unsigned bytes, unsigned write)
{
    assert(consumed < expectedCount);
    const struct Event *event = &expectedEvents[consumed++];
    assert(event->address == address && event->bytes == bytes && event->write == write);
    /* At this point the candidate has fetched the bank. Redirect subsequent
     * lookups while keeping the first register access on its original bank. */
    if (consumed == 2 && redirect)
        g_pSoundSpuRegisters = alternate;
}

void __tsan_init(void) {}
void __tsan_func_entry(void *pc) { (void)pc; }
void __tsan_func_exit(void) {}
#define DEFINE_HOOKS(n) \
    void __tsan_read##n(void *p) { observe(p, n, 0); } \
    void __tsan_write##n(void *p) { observe(p, n, 1); } \
    void __tsan_volatile_read##n(void *p) { observe(p, n, 0); } \
    void __tsan_volatile_write##n(void *p) { observe(p, n, 1); }
DEFINE_HOOKS(1)
DEFINE_HOOKS(2)
DEFINE_HOOKS(4)
DEFINE_HOOKS(8)
#undef DEFINE_HOOKS
#endif

static void prepare(struct Pages *pages, unsigned which, unsigned serial)
{
    for (unsigned b = 0; b < 2; ++b) {
        struct Bank *bank = &pages->banks[b];
        memset(bank, b ? 0x56 : 0x97, sizeof *bank);
        for (unsigned i = 0; i < 256; ++i)
            bank->page.raw[i] = (u16)(i * 349u + serial * 37u + b * 0x5757u);
    }
    g_pSoundSpuRegisters = &pages->banks[which].page;
    alternate = &pages->banks[1 - which].page;
    if (sizeof(void *) > sizeof(u32)) {
        assert((uintptr_t)g_pSoundSpuRegisters > UINT32_MAX);
        assert((uintptr_t)alternate > UINT32_MAX);
    }
    redirect = (serial & 2u) != 0;
    expectedCount = consumed = 0;
    expect(&g_pSoundSpuRegisters, sizeof g_pSoundSpuRegisters, 0);
}

static void finish(const struct Pages *pages, const struct Pages *wanted, unsigned which)
{
    const SpuUnion *wantPointer = &pages->banks[which].page;
#ifdef OBSERVE_ACCESSES
    assert(consumed == expectedCount);
    if (redirect && expectedCount > 1)
        wantPointer = alternate;
#endif
    assert(g_pSoundSpuRegisters == wantPointer);
    assert(!memcmp(pages, wanted, sizeof *pages));
    g_pSoundSpuRegisters = NULL;
    alternate = NULL;
}


enum Control { ATTACK, DECAY, LEVEL, SUSTAIN };
static u16 result(enum Control control, u16 old, u32 first, u32 second)
{
    /* Use only the input bits that can survive the final 16-bit reduction.
     * Additions intentionally retain carries between adjacent register fields. */
    switch (control) {
    case ATTACK:  return (u16)((old % 256u) + (first % 256u) * 256u + (second & 4u) * 8192u);
    case DECAY:   return (u16)((old & 0xff0fu) + (first % 4096u) * 16u);
    case LEVEL:   return (u16)((old - old % 16u) + (u16)first);
    case SUSTAIN: return (u16)((old % 64u) + (first % 1024u) * 64u + (second & 6u) * 8192u);
    }
    assert(0);
    return 0;
}

static s32 signedBits(u32 value)
{
    s32 result;
    memcpy(&result, &value, sizeof result);
    return result;
}

static void sample(enum Control control, u32 selectorBits, u32 rate, u32 mode, u16 old)
{
    struct Pages pages, wanted;
    unsigned which = calls & 1u;
    prepare(&pages, which, calls);
    SpuUnion *bank = &pages.banks[which].page;
    u32 slot = selectorBits & 0x0fffffffu;
    /* This selector reduction is independent of the candidate's 32-bit byte
     * offset calculation. A host pointer is formed only after the range test. */
    unsigned index = 0;
    if (slot < 32) {
        index = 8u * slot + (control == SUSTAIN ? 5u : 4u);
        bank->raw[index] = old;
    }
    memcpy(&wanted, &pages, sizeof wanted);
    if (slot < 32) {
        u16 value = result(control, old, rate, mode);
        equalStores += value == old;
        wanted.banks[which].page.raw[index] = value;
        expect(&bank->raw[index], control == ATTACK ? 1u : 2u, 0);
        expect(&bank->raw[index], 2, 1);
    } else {
        ++outside;
    }
    s32 voice = signedBits(selectorBits);
    s32 first = signedBits(rate);
    s32 second = signedBits(mode);
    switch (control) {
    case ATTACK:  SoundSetVoiceAdsrAttackModeAndRate(voice, first, second); break;
    case DECAY:   SoundSetVoiceAdsrDecayShift(voice, first); break;
    case LEVEL:   SoundSetVoiceAdsrSustainLevel(voice, first); break;
    case SUSTAIN: SoundSetVoiceAdsrSustainRateAndDirection(voice, first, second); break;
    }
    finish(&pages, &wanted, which);
    ++calls;
}

int main(void)
{
    const u32 parameters[] = {0,1,2,3,4,7,15,16,31,32,63,64,127,128,255,256,
                              1023,1024,4095,4096,65535,65536,0x80000000u,UINT32_MAX};
    const u32 modes[] = {0,1,2,3,4,5,6,7,8,UINT32_MAX};
    const u16 previous[] = {0,0xffff,0xaaa7,0x5555,0x12ff,0xff0f,0x803f};
    for (enum Control control = ATTACK; control <= SUSTAIN; ++control) {
        for (unsigned slot = 0; slot < 32; ++slot) {
            for (unsigned hi = 0; hi < 16; ++hi) {
                u32 selector = ((u32)hi << 28) | slot;
                for (unsigned i = 0; i < sizeof parameters / sizeof *parameters; ++i) {
                    u32 mode = modes[(slot + hi + i) % (sizeof modes / sizeof *modes)];
                    u16 old = previous[(slot + hi + i) % (sizeof previous / sizeof *previous)];
                    sample(control, selector, parameters[i], mode, old);
                }
                sample(control, selector, 0, 0, 0);
            }
        }
        for (unsigned bit = 0; bit < 32; ++bit) {
            u32 single = (u32)1 << bit;
            sample(control, 0, single, 0, 0x52f0);
            sample(control, 1, ~single, 0, 0x52f0);
            sample(control, 22, 0x26, single, 0x5aaa);
            sample(control, 23, 0x26, ~single, 0x5aaa);
        }
        const u32 invalid[] = {32,33,255,0x0fffffffu,0x10000020u,0xffffffe0u,UINT32_MAX};
        for (unsigned k = 0; k < sizeof invalid / sizeof *invalid; ++k)
            sample(control, invalid[k], UINT32_MAX, 6, 0x0fff);
    }
    assert(equalStores > 0 && outside == 28);
    printf("%u envelope calls, %u unchanged stores, %u outside the modeled page, %zu-bit pointer\n",
           calls, equalStores, outside, sizeof(void *) * 8u);
    return 0;
}
#endif
