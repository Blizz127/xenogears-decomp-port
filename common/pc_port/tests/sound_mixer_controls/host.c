#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* Synthetic host records for this isolated test. */
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;

typedef struct { u8 identity; } SoundFile;
typedef struct {
    u8 before[0x10];
    s16 unk_Flags;
    u8 after[32];
} AudioManager;
_Static_assert(offsetof(AudioManager, unk_Flags) == 0x10, "manager control offset");

extern short g_SoundControlFlags;
extern u32 g_unk_SoundEvent;
extern SoundFile *g_SoundSedsLinkedList;

extern long EnableEvent(long);
extern long DisableEvent(long);
extern void func_80039FF8(void);
extern void func_80039CC4(void);

#ifdef BUILD_COMMANDS
#ifndef CANDIDATE_INCLUDE
#define CANDIDATE_INCLUDE "../../src/cleanroom/sound_mixer_controls.inc"
#endif
#include CANDIDATE_INCLUDE
#else
void func_80037F44(void);
void func_80037F88(void);
void func_80038624(void);
void func_8003869C(void);
int func_80038824(void);
void func_800388D4(s32);
void func_8003890C(AudioManager *,s32);

enum { Flags, Token, Head, ManagerFlags };
enum { Read, Write, Service };
enum { Enable, Disable, Flush, Prepare };
struct State { u32 flags, token, head, manager; };
struct Vector { struct State initial; u32 arg, profile, select; };
struct Entry { u32 kind, target, width, value; struct State boundary; };
struct Result { struct State after; u32 result, count; struct Entry entries[8]; };

short g_SoundControlFlags;
u32 g_unk_SoundEvent;
SoundFile *g_SoundSedsLinkedList;
static SoundFile nodes[3];
static AudioManager managers[2], initialManagers[2];
static AudioManager *active;
static struct Result *trace;
static unsigned profile;
static void *pending;
static struct Entry *pendingEntry;

static SoundFile *headPointer(u32 slot) { assert(slot<4); return slot?&nodes[slot-1]:NULL; }
static u32 headIndex(SoundFile *p)
{
    if (!p) return 0;
    for (unsigned i=0;i<3;i++) if (p==&nodes[i]) return i+1;
    assert(0);
    return 0;
}
static struct State current(void)
{
    return (struct State){(u16)g_SoundControlFlags,g_unk_SoundEvent,
                          headIndex(g_SoundSedsLinkedList),(u16)active->unk_Flags};
}
static void install(struct State s)
{
    g_SoundControlFlags=(short)s.flags; g_unk_SoundEvent=s.token;
    g_SoundSedsLinkedList=headPointer(s.head); active->unk_Flags=(s16)s.manager;
}
/* Each service can replace the event, the list, and both control words. */
static struct State serviceEffect(struct State s,unsigned id,unsigned mode)
{
    if (mode&1) s.flags=(s.flags^(0xa57bu+id))&65535u;
    if (mode&2) s.flags&=0xfffeu;
    if (mode&4) s.token=(s.token*33u)^(0xcde4f1a9u+id);
    if (mode&8) s.head=(s.head+1u+id)&3u;
    if (mode&16) s.manager=(s.manager^(0x84fdu+id))&65535u;
    return s;
}
static struct Entry *append(struct Result *r,unsigned kind,unsigned target,unsigned width,unsigned value)
{
    assert(r->count<8);
    struct Entry *e=&r->entries[r->count++];
    *e=(struct Entry){kind,target,width,value,{0,0,0,0}};
    return e;
}
static u32 sample(const void *p,unsigned target)
{
    if (target==Head) return headIndex(g_SoundSedsLinkedList);
    if (target==Token) return g_unk_SoundEvent;
    u16 word; memcpy(&word,p,2);return word;
}
static void finishWrite(void)
{
    if (pending) { pendingEntry->value=sample(pending,pendingEntry->target);pending=NULL; }
}
#ifdef OBSERVE_ACCESSES
static void observe(void *p,unsigned width,unsigned isWrite)
{
    finishWrite();
    unsigned target;
    if (p==&g_SoundControlFlags) { assert(width==2);target=Flags; }
    else if (p==&g_unk_SoundEvent) { assert(width==4);target=Token; }
    else if (p==&g_SoundSedsLinkedList) {
        assert(width==sizeof g_SoundSedsLinkedList);width=4;target=Head;
    } else { assert(p==&active->unk_Flags && width==2);target=ManagerFlags; }
    struct Entry *e=append(trace,isWrite?Write:Read,target,width,isWrite?0:sample(p,target));
    if (isWrite) { pending=p;pendingEntry=e; }
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define HOOK(n) \
    void __tsan_read##n(void *p) { observe(p,n,0); } \
    void __tsan_write##n(void *p) { observe(p,n,1); } \
    void __tsan_volatile_read##n(void *p) { observe(p,n,0); } \
    void __tsan_volatile_write##n(void *p) { observe(p,n,1); }
HOOK(1) HOOK(2) HOOK(4) HOOK(8)
#undef HOOK
#endif
static long invoke(unsigned id,long arg)
{
    finishWrite();
    assert((unsigned long)arg<=UINT32_MAX);
    struct Entry *e=append(trace,Service,id,0,(u32)arg);
    e->boundary=current();
    install(serviceEffect(e->boundary,id,profile));
    return -12345678L+(long)id;
}
long EnableEvent(long handle) { return invoke(Enable,handle); }
long DisableEvent(long handle) { return invoke(Disable,handle); }
void func_80039FF8(void) { (void)invoke(Flush,0); }
void func_80039CC4(void) { (void)invoke(Prepare,0); }
void mixer_native(unsigned op,const struct Vector *v,struct Result *r)
{
    memset(r,0,sizeof *r);
    memset(managers,(int)(v->arg^v->initial.token),sizeof managers);
    active=&managers[v->select&1];trace=r;profile=v->profile;pending=NULL;
    install(v->initial);
    memcpy(initialManagers,managers,sizeof managers);
    switch (op) {
    case 0: func_80037F44();break;
    case 1: func_80037F88();break;
    case 2: func_80038624();break;
    case 3: func_8003869C();break;
    case 4: r->result=(u32)func_80038824();break;
    case 5: func_800388D4((s32)v->arg);break;
    case 6: func_8003890C(active,(s32)v->arg);break;
    default: assert(0);
    }
    finishWrite();r->after=current();
    initialManagers[v->select&1].unk_Flags=active->unk_Flags;
    assert(memcmp(initialManagers,managers,sizeof managers)==0);
}
static void expectedAccess(struct Result *r,unsigned kind,unsigned target,unsigned width,unsigned value)
{
#ifdef OBSERVE_ACCESSES
    append(r,kind,target,width,value);
#else
    (void)r;(void)kind;(void)target;(void)width;(void)value;
#endif
}
static void expectedService(struct Result *r,unsigned service,unsigned arg,unsigned mode)
{
    append(r,Service,service,0,arg)->boundary=r->after;
    r->after=serviceEffect(r->after,service,mode);
}
static void check(unsigned op,struct Vector v)
{
    struct Result expected={0},actual;
    expected.after=v.initial;
    struct State *s=&expected.after;
    if (op!=2 && op!=3) {
        if (op==6) expectedAccess(&expected,Read,ManagerFlags,2,s->manager);
        else expectedAccess(&expected,Read,Flags,2,s->flags);
    }
    if (op==0 && !(s->flags&1)) {
        unsigned token=s->token;
        expectedAccess(&expected,Read,Token,4,token);
        s->flags|=1;
        expectedAccess(&expected,Write,Flags,2,s->flags);
        expectedService(&expected,Enable,token,v.profile);
    } else if (op==1 && (s->flags&1)) {
        expectedAccess(&expected,Read,Token,4,s->token);
        expectedService(&expected,Disable,s->token,v.profile);
        expectedAccess(&expected,Read,Flags,2,s->flags);
        s->flags&=0xfffeu;
        expectedAccess(&expected,Write,Flags,2,s->flags);
    } else if (op==2) {
        expectedService(&expected,Flush,0,v.profile);
        s->head=0;expectedAccess(&expected,Write,Head,4,0);
    } else if (op==3) {
        expectedService(&expected,Prepare,0,v.profile);
        expectedService(&expected,Flush,0,v.profile);
    } else if (op==4) {
        expected.result=(s->flags&0x600)?2:!!(s->flags&0x100);
    } else if (op==5) {
        s->flags=(s->flags&0xefffu)|(v.arg?0x1000u:0u);
        expectedAccess(&expected,Write,Flags,2,s->flags);
    } else if (op==6) {
        s->manager=(s->manager&0xfffeu)|!v.arg;
        expectedAccess(&expected,Write,ManagerFlags,2,s->manager);
    }
    mixer_native(op,&v,&actual);
    assert(memcmp(&actual,&expected,sizeof actual)==0);
}
int main(void)
{
    static const u32 args[]={0,1,0xffffffff,0x80000000,0x10000,0x100,0x7fffffff};
    unsigned count=0;
    for (unsigned i=0;i<65536;i++) {
        struct Vector v={{i,(0x85be32d1u*i)^0xccf9e553u,(i>>1)&3,(i*131u)&0xffffu},
                         args[(i>>1)%7],(i>>2)&31,i&1};
        for (unsigned op=0;op<7;op++) { check(op,v);++count; }
        v.arg=0;check(5,v);check(6,v);count+=2;
    }
    printf("PASS %u mixer control cases\n",count);
    return 0;
}
#endif
