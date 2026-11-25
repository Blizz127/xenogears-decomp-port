#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef uint16_t u16;
typedef uint32_t u32;
typedef int32_t s32;
typedef union { volatile u16 raw[256]; } SpuUnion;
extern SpuUnion *g_pSoundSpuRegisters;
#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_spu_voice_registers.inc"
#else
void SoundSetVoiceKeyOn(u32);
void SoundSetReverbVoices(u32);
void SoundSetVoiceStartAddress(s32,s32);
void SoundSetVoiceVolume(s32,s32,s32);
void SoundSetVoicePitch(s32,s32);
SpuUnion *g_pSoundSpuRegisters;
struct Write { u32 offset; u16 value; };
static struct Window {
    u32 guard1[4];
    SpuUnion bank;
    u32 guard2[4];
    SpuUnion alternate;
    u32 guard3[4];
} state, expected;
static struct Write stores[2];
static unsigned writeCount, seen, redirect, cases;
static const volatile u16 *lastAddress;
static u16 lastValue;
static void finishStore(void)
{
    if (lastAddress) { assert(*lastAddress==lastValue); lastAddress=NULL; }
}
#ifdef OBSERVE_ACCESSES
static void observe(void *addr,unsigned size,int write)
{
    finishStore();
    if (!seen) {
        assert(!write && addr==&g_pSoundSpuRegisters && size==sizeof g_pSoundSpuRegisters);
    } else {
        assert(seen<=writeCount && write && size==2);
        struct Write w=stores[seen-1];
        assert(addr==(void *)&state.bank.raw[w.offset/2]);
        lastAddress=(const volatile u16 *)addr; lastValue=w.value;
        if (redirect && seen==1) g_pSoundSpuRegisters=&state.alternate;
    }
    ++seen;
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define HOOK(n) \
    void __tsan_read##n(void *p) { observe(p,n,0); } \
    void __tsan_write##n(void *p) { observe(p,n,1); } \
    void __tsan_volatile_read##n(void *p) { observe(p,n,0); } \
    void __tsan_volatile_write##n(void *p) { observe(p,n,1); }
HOOK(1)
HOOK(2)
HOOK(4)
HOOK(8)
#undef HOOK
#endif
static void store(u32 offset,u16 value)
{
    if (offset>=sizeof state.bank) return;
    assert(writeCount<2);
    stores[writeCount++]=(struct Write){offset,value};
    expected.bank.raw[offset/2]=value;
}
static void test(unsigned op,u32 a,u32 b,u32 c)
{
    memset(&state, (int)(a*73u+b+c), sizeof state);
    expected=state; writeCount=seen=0; lastAddress=NULL;
    u32 voiceOffset=a*16u;
    switch (op) {
    case 0: store(0x188,(u16)a);store(0x18a,(u16)(a>>16));break;
    case 1: store(0x198,(u16)a);store(0x19a,(u16)(a>>16));break;
    case 2: store(voiceOffset+6u,(u16)(b>>3));break;
    case 3: store(voiceOffset,(u16)b);store(voiceOffset+2u,(u16)c);break;
    case 4: store(voiceOffset+4u,(u16)b);break;
    default:assert(0);
    }
    redirect=cases&1;
    /* The repeated case must still emit each store in the instrumented build. */
    if (cases&2) for (unsigned k=0;k<writeCount;k++)
        state.bank.raw[stores[k].offset/2]=stores[k].value;
    g_pSoundSpuRegisters=&state.bank;
    switch (op) {
    case 0: SoundSetVoiceKeyOn(a);break;
    case 1: SoundSetReverbVoices(a);break;
    case 2: SoundSetVoiceStartAddress((s32)a,(s32)b);break;
    case 3: SoundSetVoiceVolume((s32)a,(s32)b,(s32)c);break;
    case 4: SoundSetVoicePitch((s32)a,(s32)b);break;
    }
    finishStore();
    assert(!memcmp(&expected,&state,sizeof state));
#ifdef OBSERVE_ACCESSES
    assert(seen==writeCount+1);
#endif
    ++cases;
}
int main(void)
{
    static const u32 indices[]={0,1,22,23,24,25,30,31,32,33,0xfffffffe,
        0xffffffff,0x10000000,0x10000001,0x10000017,0x1000001f,0x10000020,0x80000000,0xf000001f};
    static const u32 words[]={0,1,7,8,9,0xffff,0x10000,0x7ffff,0x80000,
        0xffffff,0xff000000,0x7fffffff,0x80000000,0xfffffff9,0xffffffff};
    for(unsigned n=0;n<65536;++n) {
        for(unsigned op=0;op<2;++op) {
            test(op,(n<<16)|0x7561u,0,0); test(op,0x8f240000u|n,0,0);
        }
        u32 v=0xad7b0000u|n,w=0x18e50000u|(n^0x91b7u);
        test(2,n%32,(n<<3)|(n&7)|((n&0x1fffu)<<19),0);
        test(3,n%32,v,w);test(4,n%32,v,0);
    }
    for (unsigned i=0;i<sizeof indices/sizeof *indices;i++) {
        for(unsigned j=0;j<sizeof words/sizeof *words;j++) {
            for(unsigned op=2;op<=4;op++) test(op,indices[i],words[j],~words[j]);
            test(0,words[j],0,0);test(1,words[j],0,0);
        }
    }
    printf("PASS %u five-function register-window cases\n",cases);
    return 0;
}
#endif
