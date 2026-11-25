#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <stddef.h>
/* Synthetic host records for this isolated test. */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;

typedef struct {
    s16 assignedVoice;
    u8 unused[2];
    s16 priority;
    u8 guard[26];
} SoundVoiceData;
typedef struct {
    u16 active_flag;
    u8 prefix[0x27 - 2];
    u8 voice_number;
    u8 gap[8];
    SoundVoiceData voice_data;
    u8 tail[0x158 - 0x30 - sizeof(SoundVoiceData)];
} AudioElement;
typedef struct {
    u8 prefix[0x14];
    u8 elementCount;
    u8 remainder[0x94 - 0x15];
    AudioElement elements[24];
} AudioManager;
_Static_assert(offsetof(SoundVoiceData, priority) == 4, "voice priority");
_Static_assert(offsetof(AudioElement, voice_number) == 0x27, "channel number");
_Static_assert(offsetof(AudioElement, voice_data) == 0x30, "voice member");
_Static_assert(sizeof(AudioElement) == 0x158, "row stride");
_Static_assert(offsetof(AudioManager, elements) == 0x94, "first row");

extern SoundVoiceData *g_SoundChannels[24];
extern s32 g_unk_VoicesNeedingProcessing;
extern u32 g_SoundKeyOnFlags;


void SoundAssignVoiceToChannel(SoundVoiceData *, u32);
void SoundAbortVoiceOnChannel(SoundVoiceData *, u32);
void SoundReleaseVoiceFromChannel(SoundVoiceData *, u32);
void SoundAbortAllVoices(AudioManager *);
void SoundReleaseAllVoices(AudioManager *);
extern SoundVoiceData low_ram;

#ifdef BUILD_COMMANDS
#define PSX_ADDR(n) ((void *)((uintptr_t)&low_ram + (uintptr_t)(n)))
#ifndef VOICE_FRAGMENT
#define VOICE_FRAGMENT "../../src/cleanroom/sound_voice_management.inc"
#endif
#include VOICE_FRAGMENT
#else
SoundVoiceData low_ram;
SoundVoiceData *g_SoundChannels[24];
s32 g_unk_VoicesNeedingProcessing;
u32 g_SoundKeyOnFlags;

struct Event { const void *ptr; unsigned width; char kind; };
static struct Event expected[2048];
static unsigned planned, observed, runs;
static void expect_event(char kind, const void *p, unsigned width)
{
    assert(planned < 2048);
    expected[planned++] = (struct Event){p,width,kind};
}
static void verify_event(char kind, const void *p, unsigned width)
{
    assert(observed < planned);
    struct Event e = expected[observed++];
    assert(e.kind==kind && e.ptr==p && e.width==width);
}
static void begin(void) { planned=observed=0; }
static void finish(void) {
    assert(observed==planned);
    ++runs;
}
#ifdef OBSERVE_ACCESSES
#define PLAN(k,p,w) expect_event(k,p,w)
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define ACCESS(n) \
  void __tsan_read##n(void *p) { verify_event('R',p,n); } \
  void __tsan_write##n(void *p) { verify_event('W',p,n); } \
  void __tsan_volatile_read##n(void *p) { verify_event('R',p,n); } \
  void __tsan_volatile_write##n(void *p) { verify_event('W',p,n); }
ACCESS(1) ACCESS(2) ACCESS(4) ACCESS(8) ACCESS(16)
#else
#define PLAN(k,p,w) ((void)0)
#endif

#ifndef CLEAR_GROUPS
void SoundReleaseVoiceFromChannel(SoundVoiceData *v,u32 i) { (void)v; (void)i; abort(); }
static void leaves(void)
{
    SoundVoiceData records[2], copies[2], low_copy;
    SoundVoiceData *slots[24];
    const u32 channels[]={0,1,12,23,24,25,0x80000000u,~0u};
    const s16 pairs[][2]={{-32768,32767},{32767,-32768},{0,0},{-1,0},{0,-1},{32767,32767}};
    for (unsigned which=0; which<2; ++which)
    for (unsigned ch=0;ch<sizeof channels/sizeof channels[0];++ch)
    for (unsigned old=0;old<3;++old)
    for (unsigned request=0;request<3;++request)
    for (unsigned p=0;p<sizeof pairs/sizeof pairs[0];++p)
    for (unsigned masks=0;masks<2;++masks) {
        u32 index=channels[ch], pending=masks?~0u:0x55240u, key=masks?0:0x1ffffau;
        memset(records,0x5a,sizeof records);memset(&low_ram,0x38,sizeof low_ram);
        SoundVoiceData *v=request?&records[request-1]:NULL,*owner=old?&records[old-1]:NULL;
        SoundVoiceData *access=v?v:&low_ram;
        access->priority=pairs[p][0]; if(owner) owner->priority=pairs[p][1];
        memset(g_SoundChannels,0,sizeof g_SoundChannels);
        if(index<24) g_SoundChannels[index]=owner;
        g_unk_VoicesNeedingProcessing=(s32)pending;g_SoundKeyOnFlags=key;
        memcpy(copies,records,sizeof records);low_copy=low_ram;memcpy(slots,g_SoundChannels,sizeof slots);
        begin();
        if(index<24) {
            PLAN('R',&g_SoundChannels[index],sizeof v);
            if(which==0 && owner!=v) {
                if(owner) { PLAN('R',&owner->priority,2); PLAN('R',&access->priority,2); }
                if(!owner || access->priority>=owner->priority) {
                    PLAN('W',&access->assignedVoice,2); PLAN('W',&g_SoundChannels[index],sizeof v);
                    (v?&copies[request-1]:&low_copy)->assignedVoice=(s16)index;
                    slots[index]=v;
                }
            }
            if(which==1 && owner==v) {
                PLAN('R',&g_unk_VoicesNeedingProcessing,4);PLAN('W',&g_unk_VoicesNeedingProcessing,4);
                PLAN('R',&g_SoundKeyOnFlags,4);PLAN('W',&g_SoundKeyOnFlags,4);
                pending|=1u<<index;key&=~(1u<<index);
            }
        }
        if(which==0) SoundAssignVoiceToChannel(v,index); else SoundAbortVoiceOnChannel(v,index);
        finish();
        assert(!memcmp(records,copies,sizeof records) && !memcmp(&low_copy,&low_ram,sizeof low_copy));
        assert(!memcmp(slots,g_SoundChannels,sizeof slots));
        assert((u32)g_unk_VoicesNeedingProcessing==pending && g_SoundKeyOnFlags==key);
    }
}
#else
static unsigned char *live, *model;
static size_t storage;
static unsigned visits, dynamic;

static void change_next(unsigned char *bytes, unsigned i, u32 channel)
{
    AudioManager *manager=(AudioManager *)(void *)bytes;
    manager->elementCount=119;
    size_t offset=0x94+(size_t)(i+1)*0x158;
    if(offset+0x158<=storage) {
        AudioElement *e=(AudioElement *)(void *)(bytes+offset);
        e->active_flag=(u16)(i%2?0:0xfedc); e->voice_number=(u8)(channel+251);
    }
}
static void service(char kind, SoundVoiceData *v, u32 channel)
{
    uintptr_t pos=(uintptr_t)v-(uintptr_t)live;
    assert(pos>=0xc4 && (pos-0xc4)%0x158==0 && pos<storage);
    unsigned i=(unsigned)((pos-0xc4)/0x158);
    verify_event(kind,v,channel);++visits;
    if(dynamic) change_next(live,i,channel);
}
void SoundAbortVoiceOnChannel(SoundVoiceData *v,u32 channel) { service('A',v,channel); }
void SoundReleaseVoiceFromChannel(SoundVoiceData *v,u32 channel) { service('L',v,channel); }

static void group_case(unsigned count, unsigned active_mode, unsigned mutate, int revoke)
{
    assert(count>0);
    unsigned maximum=count;
    storage=0x94+(size_t)(maximum<24?24:maximum)*0x158;
    live=malloc(storage);model=malloc(storage);
    assert(live && model);
    for(size_t i=0;i<storage;++i) live[i]=(u8)(19u+i*73u);
    AudioManager *manager=(AudioManager *)(void *)live;
    manager->elementCount=(u8)count;
    for(unsigned i=0;i<maximum;++i) {
        AudioElement *e=(AudioElement *)(void *)(live+0x94+(size_t)i*0x158);
        e->active_flag=(u16)(active_mode==0?0:active_mode==1 || i%3?0x8001:0);
        e->voice_number=(const u8[]){0,23,24,255,3}[i%5];
    }
    memcpy(model,live,storage);begin();visits=0;dynamic=mutate;
    unsigned calls=0;
    PLAN('R',&manager->elementCount,1);
    for(unsigned i=0;i<maximum;++i) {
        size_t offset=0x94+(size_t)i*0x158;
        AudioElement *e=(AudioElement *)(void *)(live+offset), *m=(AudioElement *)(void *)(model+offset);
        if(revoke) PLAN('R',&e->active_flag,2);
        if(!revoke || m->active_flag) {
            PLAN('R',&e->voice_number,1);
            expect_event(revoke?'A':'L',&e->voice_data,m->voice_number);
            ++calls;
            if(dynamic) change_next(model,i,m->voice_number);
        }
    }
    if(revoke) SoundAbortAllVoices(manager); else SoundReleaseAllVoices(manager);
    finish();
    assert(calls==visits && !memcmp(live,model,storage));
    free(model);free(live);
}
static void empty_header(void)
{
    const size_t page=(size_t)sysconf(_SC_PAGESIZE);
    const size_t header=__builtin_offsetof(AudioManager,elements);
    assert(header==0x94 && header<page);
    unsigned char *allocation=mmap(NULL,page*2,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);
    assert(allocation!=MAP_FAILED);
    live=allocation+page-header;storage=header;visits=0;dynamic=0;
    AudioManager *manager=(AudioManager *)(void *)live;
    memset(live,0xa5,header);manager->elementCount=0;
    /* Any element read faults. Even a redundant header write faults. */
    assert(mprotect(allocation+page,page,PROT_NONE)==0);
    assert(mprotect(allocation,page,PROT_READ)==0);
    for(unsigned i=0;i<2;++i) {
        begin(); PLAN('R',&manager->elementCount,1);
        if(i==0) SoundAbortAllVoices(manager); else SoundReleaseAllVoices(manager);
        finish(); assert(visits==0);
    }
    assert(munmap(allocation,page*2)==0);
}
static void groups(void)
{
    const unsigned counts[]={1,2,3,7,23,24,25,127,255};
    for(unsigned k=0;k<sizeof counts/sizeof counts[0];++k)
        for(unsigned active=0;active<3;++active)
            for(unsigned modify=0;modify<2;++modify)
                for(unsigned mode=0;mode<2;++mode)
                    group_case(counts[k],active,modify,(int)mode);
    empty_header();
}
#endif

int main(void)
{
    (void)expect_event;(void)verify_event;
#ifdef CLEAR_GROUPS
    groups();
#else
    leaves();
#endif
    printf("%u voice-management cases passed\n",runs);
    return 0;
}
#endif
