#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
/* Only the public sound fields relevant to these handlers are projected. */
typedef struct {
    u8 reserved[6];
    u16 flags;
    u8 state[0x1c];
    u8 unkAdsr1, unkAdsr2, unkAdsr3, unkAdsr4, adsrDR, unkAdsr5, unkAdsr6, adsrSR;
} SoundVoiceData;
typedef struct {
    u8 prefix[0x28];
    u8 unk_0x28;
    u8 pad[7];
    SoundVoiceData voice_data;
    u8 tail[0x158-0x5c];
} AudioElement;
typedef struct { u8 sentinel[0x94]; } AudioManager;
_Static_assert(sizeof(AudioElement) == 0x158, "sound element size");
_Static_assert(offsetof(AudioElement, voice_data.flags) == 0x36, "flags offset");
_Static_assert(offsetof(AudioElement, voice_data.unkAdsr1) == 0x54, "attack mode offset");
_Static_assert(offsetof(AudioElement, voice_data.adsrSR) == 0x5b, "sustain level offset");

#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_script_envelope_ops.inc"
#else
typedef u8 *Command(u8 *, AudioManager *, AudioElement *);
Command SoundScriptSetDecayTime, SoundScriptSetRelease, SoundScriptSetSustainLevel;
Command SoundScriptSetAdsrDRAndSR, SoundScriptSetAttackMode, SoundScriptSetSustainMode, SoundScriptSetReleaseMode;
struct Operation {
    Command *handler;
    unsigned opcode, length, mask, first, second;
};
static const struct Operation ops[] = {
    {SoundScriptSetDecayTime,   0xc3,1,0x020,0x58,0   },
    {SoundScriptSetRelease,     0xc5,1,0x080,0x28,0x5a},
    {SoundScriptSetSustainLevel,0xc6,1,0x100,0x5b,0   },
    {SoundScriptSetAdsrDRAndSR, 0xc7,2,0x120,0x58,0x5b},
    {SoundScriptSetAttackMode,  0xc8,1,0x010,0x54,0   },
    {SoundScriptSetSustainMode, 0xc9,1,0x040,0x55,0   },
    {SoundScriptSetReleaseMode, 0xca,1,0x080,0x56,0   }
};
#define COUNT(a) (sizeof(a) / sizeof((a)[0]))
struct Arena {
    u8 guard[64];
    AudioElement element;
    u8 gap[32];
    AudioManager manager;
    u8 script[128];
    u8 tail[64];
};
static struct Arena arena, expected;
static size_t base(void) { return offsetof(struct Arena,element); }
static u8 *raw(void) { return (u8 *)&arena; }
struct Event { size_t offset, width; int write; };
static struct Event events[6];
static unsigned count, seen, tested;
static unsigned char *expectedBytes(void) { return (unsigned char *)&expected; }

static unsigned readAt(size_t off, unsigned width)
{
    unsigned value = expectedBytes()[off];
    if (width == 2) value |= (unsigned)expectedBytes()[off+1] << 8;
    assert(count < COUNT(events));
    events[count++] = (struct Event){off,width,0};
    return value;
}
static void writeAt(size_t off, unsigned width, unsigned value)
{
    expectedBytes()[off] = (u8)value;
    if (width == 2) expectedBytes()[off+1] = (u8)(value >> 8);
    assert(count < COUNT(events));
    events[count++] = (struct Event){off,width,1};
}
#ifdef OBSERVE_ACCESSES
static void check(void *p, size_t width, int write)
{
    assert(seen < count);
    const struct Event *e = &events[seen++];
    assert(p == raw()+e->offset && width == e->width && write == e->write);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define OBSERVE(n) \
    void __tsan_read##n(void *p) { check(p,n,0); } \
    void __tsan_volatile_read##n(void *p) { check(p,n,0); } \
    void __tsan_write##n(void *p) { check(p,n,1); } \
    void __tsan_volatile_write##n(void *p) { check(p,n,1); }
OBSERVE(1)
OBSERVE(2)
OBSERVE(4)
OBSERVE(8)
#undef OBSERVE
#endif

static u8 *execute(const struct Operation *op, size_t cursor, AudioManager *manager)
{
    size_t f=base()+0x36, a=base()+op->first, b=base()+op->second;
    count=seen=0;
    if (op->length==2) {
        unsigned first=readAt(cursor,1);
        writeAt(a,1,first);
        unsigned old=readAt(f,2);
        unsigned second=readAt(cursor+1,1);
        writeAt(f,2,old|op->mask);
        writeAt(b,1,second);
    } else {
        unsigned old=readAt(f,2);
        unsigned first=readAt(cursor,1);
        writeAt(f,2,old|op->mask);
        writeAt(a,1,first);
        if (op->second) writeAt(b,1,first);
    }
    u8 *next=op->handler(raw()+cursor, manager, &arena.element);
    assert(next==raw()+cursor+op->length);
    assert(!memcmp(&expected,&arena,sizeof arena));
#ifdef OBSERVE_ACCESSES
    assert(seen==count);
#endif
    ++tested;
    return next;
}
static void operands(void)
{
    size_t script=offsetof(struct Arena,script);
    size_t starts[]={script,base()+0x35,base()+0x36,base()+0x37,base()+0x27,
        base()+0x28,base()+0x53,base()+0x54,base()+0x55,base()+0x56,
        base()+0x57,base()+0x58,base()+0x59,base()+0x5a,base()+0x5b,
        base()+0x157,offsetof(struct Arena,manager)+7};
    for (size_t i=0; i<COUNT(ops); ++i) {
        const struct Operation *op=&ops[i];
        for (unsigned byte=0; byte<256; ++byte) {
            for (size_t j=0; j<COUNT(starts); ++j) {
                size_t cur=starts[j];
                memset(&arena,(int)(i*59+j+byte),sizeof arena);
                const u16 flags[]={0,0xffff,0xaaaa,0x5555,(u16)op->mask,
                    (u16)~op->mask,(u16)(1u<<(byte%16)),(u16)(byte*7919u)};
                arena.element.voice_data.flags=flags[(byte+j)%COUNT(flags)];
                raw()[cur]=(u8)byte;
                raw()[cur+1]=(u8)(byte^0x98);
                if (!j && (byte&1)) {
                    arena.element.voice_data.flags |= (u16)op->mask;
                    raw()[base()+op->first]=(u8)byte;
                    if (op->second) raw()[base()+op->second]=(u8)(op->length==2 ? (byte^0x98):byte);
                }
                expected=arena;
                execute(op,cur,(j&1) ? &arena.manager : NULL);
            }
        }
        if (op->length==2) {
            for (unsigned first=0; first<256; ++first) {
                for (unsigned second=0; second<256; ++second) {
                    memset(&arena,(int)(first+second+91),sizeof arena);
                    arena.element.voice_data.flags=(u16)(first*313+second*101);
                    arena.script[0]=(u8)first; arena.script[1]=(u8)second;
                    expected=arena;
                    execute(op,script,&arena.manager);
                }
            }
        }
    }
}
static void sequence(void)
{
    memset(&arena,0xa6,sizeof arena);
    size_t length=0;
    for (unsigned j=0; j<7; ++j) {
        for (size_t i=0; i<COUNT(ops); ++i) {
            const struct Operation *op=&ops[(i+j)%COUNT(ops)];
            arena.script[length++]=(u8)op->opcode;
            arena.script[length++]=(u8)(0xf1-i*3+j);
            if (op->length==2) arena.script[length++]=(u8)(0x41+j);
        }
    }
    expected=arena;
    u8 *cursor=arena.script;
    while (cursor<arena.script+length) {
        unsigned opcode=*cursor++;
        size_t i=0;
        while (i<COUNT(ops) && ops[i].opcode!=opcode) ++i;
        assert(i<COUNT(ops));
        cursor=execute(&ops[i],(size_t)(cursor-raw()),&arena.manager);
    }
    assert(cursor==arena.script+length);
}
int main(void)
{
    operands();
    sequence();
    printf("PASS %u ordered envelope command cases across seven functions\n", tested);
    return 0;
}
#endif
