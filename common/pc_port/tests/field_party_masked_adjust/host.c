#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Synthetic records for the command and its service doubles. */
typedef uint16_t u16;
typedef struct {
    uint32_t prefixGuard;
    u16 scriptInstructionPointer;
    uint16_t suffixGuard;
} ActorData;
extern ActorData *g_FieldScriptVMCurActor;

typedef uint8_t u8;
typedef int16_t s16;
enum { MAX_PARTY_MEMBERS = 3 };
extern void *g_FieldScriptVMCurScriptData;
extern int g_GamePartyMembers[MAX_PARTY_MEMBERS];

extern s16 g_FieldNumPartyMembersMasks[4];
int FieldScriptArgument1(int offset, int instruction_byte);
void FieldPartyMemberDecreaseHp(int slot, int amount);
void FieldPartyMemberIncreaseMp(int slot, int amount);
void FieldPartyMemberDecreaseMp(int slot, int amount);

#ifdef BUILD_COMMANDS
#ifndef PARTY_IMPLEMENTATION
#define PARTY_IMPLEMENTATION "../../src/cleanroom/field_party_script_adjust_amount.inc"
#endif
#include PARTY_IMPLEMENTATION
#else
ActorData *g_FieldScriptVMCurActor;
void *g_FieldScriptVMCurScriptData;
int g_GamePartyMembers[MAX_PARTY_MEMBERS];
s16 g_FieldNumPartyMembersMasks[4];
void FieldScriptVMHandlerDecreasePartyHp(void);
void FieldScriptVMHandlerIncreasePartyMp(void);
void FieldScriptVMHandlerDecreasePartyMp(void);

struct Bank {
    ActorData actor[3];
    u8 script[2][65539];
};
static struct Bank actual, reference;
static unsigned handler, test, actor, script, cursor, operand, amount_calls, leaf_calls, next_slot;
static unsigned waiting_slot, latched_mask;
static int amount, roster[MAX_PARTY_MEMBERS];
static s16 masks[4];

#ifdef OBSERVE_ACCESSES
struct Access { const void *address; unsigned bytes, write; };
static struct Access ordered[32];
static unsigned queued, seen;
static unsigned long total_accesses;
static void expect(const void *address, unsigned bytes, unsigned write)
{
    assert(queued < sizeof(ordered) / sizeof(ordered[0]));
    ordered[queued++] = (struct Access){address, bytes, write};
}
static void observed(void *address, unsigned bytes, unsigned write)
{
    assert(seen < queued);
    struct Access a = ordered[seen++];
    assert(a.address == address && a.bytes == bytes && a.write == write);
    total_accesses++;
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) {(void)p;}
void __tsan_func_exit(void) {}
#define SIZE_CALLBACKS(N) \
    void __tsan_read##N(void *p) {observed(p,N,0);} \
    void __tsan_write##N(void *p) {observed(p,N,1);} \
    void __tsan_volatile_read##N(void *p) {observed(p,N,0);} \
    void __tsan_volatile_write##N(void *p) {observed(p,N,1);}
SIZE_CALLBACKS(1) SIZE_CALLBACKS(2) SIZE_CALLBACKS(4) SIZE_CALLBACKS(8)
#define READ(P) expect((P),sizeof(*(P)),0)
#define WRITE(P) expect((P),sizeof(*(P)),1)
#define FINISHED() assert(seen==queued)
#else
#define READ(P) ((void)0)
#define WRITE(P) ((void)0)
#define FINISHED() ((void)0)
#endif

static void change_context(unsigned a, unsigned s, unsigned ip, unsigned byte)
{
    actor=a; script=s; cursor=ip; operand=byte;
    g_FieldScriptVMCurActor=&actual.actor[actor];
    g_FieldScriptVMCurScriptData=actual.script[script];
    actual.actor[actor].scriptInstructionPointer=reference.actor[actor].scriptInstructionPointer=(u16)cursor;
    actual.script[script][cursor+3]=reference.script[script][cursor+3]=(u8)operand;
}

static void check_state(void)
{
    assert(!memcmp(&actual,&reference,sizeof(actual)));
    assert(!memcmp(masks,g_FieldNumPartyMembersMasks,sizeof(masks)));
    assert(!memcmp(roster,g_GamePartyMembers,sizeof(roster)));
    assert(g_FieldScriptVMCurActor==&actual.actor[actor]);
    assert(g_FieldScriptVMCurScriptData==actual.script[script]);
}

static void expect_operand(void)
{
    READ(&g_FieldScriptVMCurActor);
    READ(&g_FieldScriptVMCurScriptData);
    READ(&actual.actor[actor].scriptInstructionPointer);
    READ(&actual.script[script][cursor+3]);
}

static void expect_next_call_or_completion(void)
{
    while (next_slot < MAX_PARTY_MEMBERS) {
        unsigned slot=next_slot++;
        READ(&g_GamePartyMembers[slot]);
        if (roster[slot] != 255 && ((latched_mask >> slot) & 1)) {
            waiting_slot=slot;
            return;
        }
    }
    waiting_slot=MAX_PARTY_MEMBERS;
    READ(&g_FieldScriptVMCurActor);
    READ(&actual.actor[actor].scriptInstructionPointer);
    WRITE(&actual.actor[actor].scriptInstructionPointer);
    reference.actor[actor].scriptInstructionPointer=(u16)(cursor+4);
}

int FieldScriptArgument1(int offset, int instruction_byte)
{
    FINISHED();
    check_state();
    assert(amount_calls++==0 && offset==1 && instruction_byte==(int)operand);
    if (test & 1)
        change_context(1,1,(cursor+17)&65535,(operand+91)&255);
    expect_operand();
    READ(&g_FieldNumPartyMembersMasks[operand&3]);
    latched_mask = (u16)masks[operand&3];
    expect_next_call_or_completion();
    return amount;
}

static void leaf(unsigned which, int slot, int value)
{
    FINISHED();
    check_state();
    assert(amount_calls==1 && handler==which && waiting_slot<MAX_PARTY_MEMBERS);
    assert(slot==(int)waiting_slot && value==amount);
    leaf_calls++;
    if (test & 2) {
        for (unsigned j=next_slot; j<MAX_PARTY_MEMBERS; ++j)
            roster[j]=g_GamePartyMembers[j]=roster[j]==255 ? 0x1ff : 255;
    }
    if (test & 4) {
        masks[operand&3]=g_FieldNumPartyMembersMasks[operand&3]=(s16)(masks[operand&3]^0x7);
        change_context(2,script^1,(cursor+0xffe1)&65535,(operand+41)&255);
    }
    expect_next_call_or_completion();
}
void FieldPartyMemberDecreaseHp(int slot, int value) {leaf(0,slot,value);}
void FieldPartyMemberIncreaseMp(int slot, int value) {leaf(1,slot,value);}
void FieldPartyMemberDecreaseMp(int slot, int value) {leaf(2,slot,value);}

int main(void)
{
    static void (*const run[])(void)={FieldScriptVMHandlerDecreasePartyHp,
        FieldScriptVMHandlerIncreasePartyMp, FieldScriptVMHandlerDecreasePartyMp};
    static const unsigned ips[]={0,1,100,32768,65531,65532,65533,65535};
    static const int returned[]={INT_MIN,-65537,-1,0,1,32768,65535,65536,INT_MAX};
    static const int ids[]={0,1,255,256,511,INT_MIN,-1};
    unsigned calls=0, total_leaves=0;
    memset(&actual,0xa5,sizeof(actual));
    memcpy(&reference,&actual,sizeof(actual));
    for (handler=0;handler<3;++handler) for (test=0;test<2048;++test) {
        unsigned ip=ips[(test/17)%8];
        unsigned key=(test*53 + test/256)&255;
        change_context(0,0,ip,key);
        for (unsigned j=0;j<4;++j) {
            s16 mask=(s16)((test*911u+j*21u)^0x9119u);
            if (test % 16 == 0) mask = 0;
            if (test % 16 == 8) mask = -1;
            masks[j]=g_FieldNumPartyMembersMasks[j]=mask;
        }
        for (unsigned j=0;j<MAX_PARTY_MEMBERS;++j)
            roster[j]=g_GamePartyMembers[j]=ids[(test/(j+1)+j)%7];
        amount=returned[(test/3)%9];
        amount_calls=leaf_calls=next_slot=0;
        waiting_slot=MAX_PARTY_MEMBERS;
#ifdef OBSERVE_ACCESSES
        queued=seen=0;
#endif
        expect_operand();
        run[handler]();
        FINISHED(); check_state();
        assert(amount_calls==1 && waiting_slot==MAX_PARTY_MEMBERS);
        calls++;
        total_leaves+=leaf_calls;
    }
    printf("%u masked party adjustment cases, %u member calls",calls,total_leaves);
#ifdef OBSERVE_ACCESSES
    printf(", %lu traced memory accesses",total_accesses);
#endif
    putchar('\n');
    return 0;
}
#endif
