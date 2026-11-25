#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Synthetic host records for this isolated test. */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;

typedef unsigned char u_char;
typedef struct {
    u8 guard;
    u8 unk1;
    u8 prefix[6];
    s32 size;
    u8 tail[4];
} SoundSpuMemoryBlock;
typedef struct {
    _Alignas(u32) u16 active_flag;
    u16 status_flags;
    u8 tail[0x158 - 4];
} AudioElement;
typedef struct {
    u8 prefix[0x14];
    u8 elementCount;
    u8 remainder[0x94 - 0x15];
    AudioElement elements[];
} AudioManager;
SoundSpuMemoryBlock *SoundSpuMemoryFindBlock(s32);


extern short g_SoundControlFlags;
extern int D_80059404;
void func_80039FF8(void);
void func_8003B644(s32, s32, s32, s32);
#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_sequencer_control_services.inc"
#else
void func_80039748(int, u_char);
void func_80039D2C(s32);
void func_80039E18(s32);
void func_80039EC4(s32, s32);
void func_8003AFA0(AudioManager *);
short g_SoundControlFlags;
int D_80059404;
_Static_assert(sizeof(AudioElement) == 344 && offsetof(AudioManager, elements) == 148, "variable-sized manager ABI");
_Static_assert(offsetof(AudioManager, elementCount) == 20 && offsetof(AudioElement, status_flags) == 2, "record offsets");
_Static_assert(offsetof(SoundSpuMemoryBlock, unk1) == 1, "block byte");

enum Kind { READ, WRITE, LOOKUP, DISABLE, SEND };
struct Event { enum Kind kind; const void *address; unsigned bytes; };
static struct Event queue[1100];
static unsigned count, seen, callbacks, expectedCallbacks, servicesTested, walks, rereads;
static unsigned initialFlags, mutationFlags;
static int initialPending, changedPending, wantAddress, args[4];
static SoundSpuMemoryBlock *record, *returnedBlock;
static u8 *injectAt;

static void begin(void)
{
    count = seen = callbacks = expectedCallbacks = 0;
    injectAt = NULL;
}
static void add(enum Kind kind, const void *address, unsigned bytes)
{
    assert(count < sizeof queue / sizeof *queue);
    queue[count++] = (struct Event){kind,address,bytes};
}
static void event(enum Kind kind, const void *address, unsigned bytes)
{
#ifdef OBSERVE_ACCESSES
    assert(seen < count);
    const struct Event *next = &queue[seen++];
    assert(next->kind == kind && next->address == address && next->bytes == bytes);
    if (injectAt && address == injectAt && kind == READ && bytes == 2) {
        u16 changed = 0x10;
        memcpy(injectAt, &changed, 2);
        injectAt = NULL;
        ++rereads;
    }
#else
    (void)kind; (void)address; (void)bytes;
#endif
}
static void finish(void)
{
    assert(callbacks == expectedCallbacks);
#ifdef OBSERVE_ACCESSES
    assert(seen == count && injectAt == NULL);
#endif
}
#ifdef OBSERVE_ACCESSES
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define HIT(n) \
    void __tsan_read##n(void *p) { event(READ,p,n); } \
    void __tsan_write##n(void *p) { event(WRITE,p,n); } \
    void __tsan_volatile_read##n(void *p) { event(READ,p,n); } \
    void __tsan_volatile_write##n(void *p) { event(WRITE,p,n); }
HIT(1)
HIT(2)
HIT(4)
HIT(8)
#undef HIT
#endif

static void provider(enum Kind kind)
{
    event(kind,NULL,0);
    assert(++callbacks == expectedCallbacks && (u16)g_SoundControlFlags == initialFlags);
    assert(D_80059404 == (kind == SEND ? 2 : initialPending));
    g_SoundControlFlags = (short)mutationFlags;
    D_80059404 = changedPending;
    if (record) {
        record->unk1 = (u8)mutationFlags;
        record->size = changedPending;
    }
}
SoundSpuMemoryBlock *SoundSpuMemoryFindBlock(s32 target)
{
    assert(target == wantAddress);
    provider(LOOKUP);
    return returnedBlock;
}
void func_80039FF8(void) { provider(DISABLE); }
void func_8003B644(s32 a, s32 b, s32 c, s32 d)
{
    assert(a == args[0] && b == args[1] && c == args[2] && d == args[3]);
    provider(SEND);
}
static s32 signedBits(u32 v) { s32 result; memcpy(&result,&v,sizeof result); return result; }

static void testService(unsigned kind, u32 input, u32 channel, u16 flags, int found)
{
    struct BlockEnvelope { u8 lead[8]; SoundSpuMemoryBlock item; u8 tail[8]; } block, wanted;
    memset(&block, (u8)channel, sizeof block);
    wanted = block;
    begin();
    record = &block.item;
    returnedBlock = found ? record : NULL;
    if (sizeof(void *) > 4) assert((uintptr_t)record > UINT32_MAX);
    initialFlags = flags;
    mutationFlags = (u16)(channel * 271u + flags + 0x1078u);
    initialPending = signedBits(0x9a54231du + input);
    changedPending = signedBits(0x473af070u + channel);
    g_SoundControlFlags = (short)initialFlags;
    D_80059404 = initialPending;
    wantAddress = signedBits(input);
    unsigned finalFlags = initialFlags;
    int finalPending = initialPending;
    if (kind == 0) {
        expectedCallbacks = 1;
        add(LOOKUP,NULL,0);
        if (found) add(WRITE,&record->unk1,1);
        func_80039748(wantAddress,(u_char)channel);
    } else if (kind == 1) {
        if (!input) {
            expectedCallbacks = 1;
            add(DISABLE,NULL,0);
        }
        add(READ,&g_SoundControlFlags,2);
        add(WRITE,&g_SoundControlFlags,2);
        func_80039D2C(wantAddress);
    } else {
        add(READ,&g_SoundControlFlags,2);
        if (flags & 0x800u) {
            expectedCallbacks = 1;
            add(WRITE,&D_80059404,4);
            add(SEND,NULL,0);
            args[0] = kind == 2 ? 0x600c : (int)(0x2000u + ((channel % 256u & 254u) ^ 8u));
            args[1] = wantAddress;
            args[2] = 0x6000;
            args[3] = 0x4000;
        }
        if (kind == 2) func_80039E18(wantAddress);
        else func_80039EC4(wantAddress,signedBits(channel));
    }
    if (expectedCallbacks) {
        finalPending = changedPending;
        finalFlags = mutationFlags;
        wanted.item.unk1 = (u8)mutationFlags;
        wanted.item.size = changedPending;
    }
    if (kind == 0 && found) wanted.item.unk1 = (u8)channel;
    if (kind == 1) finalFlags = input ? (initialFlags | 0x800u) : (mutationFlags & 0xf7ffu);
    finish();
    assert((u16)g_SoundControlFlags == finalFlags && D_80059404 == finalPending);
    assert(!memcmp(&block,&wanted,sizeof block));
    record = NULL;
    returnedBlock = NULL;
    ++servicesTested;
}
static void store2(u8 *p,u16 v) { memcpy(p,&v,2); }
static u16 read2(const u8 *p) { u16 v; memcpy(&v,p,2); return v; }

static void walk(unsigned n, unsigned pattern, int switchFlag)
{
    const size_t bytes = 32u + 148u + n * 344u + 32u;
    u8 *storage = malloc(bytes), *expected = malloc(bytes);
    assert(storage && expected);
    for (size_t j=0; j<bytes; ++j) storage[j] = (u8)(j * 37u + pattern * 15u);
    u8 *manager = storage + 32;
    manager[20] = (u8)n;
    begin();
    add(READ,manager+20,1);
    const u16 active[] = {0x101,0x110,0x111,0x121,0xffff,0x8101,0,1};
    for (unsigned i=0; i<n; ++i) {
        u8 *row = manager + 148 + 344*i;
        store2(row,active[(pattern+i)%8]);
        store2(row+2,(u16)(0xacedu^(i+pattern)));
    }
    memcpy(expected,storage,bytes);
    for (unsigned i=0; i<n; ++i) {
        u8 *row = manager + 148 + 344*i;
        u8 *out = expected + (row-storage);
        u16 bits = read2(row);
        add(READ,row,4);
        if ((bits & 0x101u) != 0x101u) continue;
        add(READ,row,2);
#ifdef OBSERVE_ACCESSES
        if (!i && switchFlag) {
            injectAt = row;
            bits = 0x10;
            store2(out,bits);
        }
#else
        (void)switchFlag;
#endif
        if (bits & 0x30u) continue;
        add(READ,row+2,2);
        add(WRITE,row+2,2);
        store2(out+2,(u16)(read2(row+2)|1u));
    }
    func_8003AFA0((AudioManager *)(void *)manager);
    finish();
    assert(!memcmp(storage,expected,bytes));
    free(expected);
    free(storage);
    ++walks;
}
int main(void)
{
    const u32 inputs[] = {0,1,0x80000000u,0x7fffffffu,0xffffffffu,0xfabcde01u};
    const u16 flags[] = {0,0x800,0x7ff,0xffff,0x5555,0xaaaa,0xf7ff};
    for (unsigned k=0; k<4; ++k)
        for (unsigned ch=0; ch<256; ++ch)
            for (unsigned j=0; j<sizeof inputs/sizeof *inputs; ++j)
                testService(k,inputs[j],ch | (j&1u ? 0x8001a300u : 0),flags[(ch+j)%7],(ch+j)&1);
    const unsigned counts[] = {0,1,2,3,8,23,24,25,32,127,128,254,255};
    for (unsigned k=0; k<sizeof counts/sizeof *counts; ++k)
        for (unsigned seed=0; seed<8; ++seed)
            walk(counts[k],seed,0);
    walk(1,0,1);
    walk(25,0,1);
    walk(255,0,1);
#ifdef OBSERVE_ACCESSES
    assert(rereads == 3);
#else
    (void)rereads;
#endif
    printf("%u service-path cases, %u complete manager scans\n",servicesTested,walks);
    return 0;
}
#endif
