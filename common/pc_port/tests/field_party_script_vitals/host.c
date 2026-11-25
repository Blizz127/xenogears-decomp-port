#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
/* Synthetic state for testing the host interfaces. */
typedef uint16_t u16;
enum { MAX_PARTY_MEMBERS = 3, MAX_GAME_CHARACTERS = 11 };
typedef struct {
    uint32_t prefixGuard;
    u16 maxMp, hp, maxHp, mp;
    uint32_t suffixGuard;
} GameCharacter;
typedef struct { GameCharacter characters[MAX_GAME_CHARACTERS]; } GameState;
extern GameState *g_pGameState;
extern int g_GamePartyMembers[MAX_PARTY_MEMBERS];

typedef uint8_t u8;
typedef struct { uint32_t prefixGuard; u16 scriptInstructionPointer; } ActorData;
extern ActorData *g_FieldScriptVMCurActor;
extern void *g_FieldScriptVMCurScriptData;
int FieldScriptVMGetInstructionArgument(int offset);


#ifdef BUILD_COMMANDS
#ifndef PARTY_IMPLEMENTATION
#define PARTY_IMPLEMENTATION "../../src/cleanroom/field_party_script_vitals.inc"
#endif
#include PARTY_IMPLEMENTATION
#else
GameState *g_pGameState;
ActorData *g_FieldScriptVMCurActor;
void *g_FieldScriptVMCurScriptData;
int g_GamePartyMembers[MAX_PARTY_MEMBERS];
void FieldScriptVMHandlerWritePartyMemberHp(void);
void FieldScriptVMHandlerWritePartyMemberMp(void);
void FieldScriptVMHandlerSetPartyMemberHp(void);
void FieldScriptVMHandlerSetPartyMemberMp(void);

struct State {
    ActorData actors[3];
    unsigned char scripts[2][65540];
    GameState games[2];
};
static struct State live, expected;

static unsigned test, handler, actor, script, game, slot, destination, cursor;
static uint16_t base_current, base_max;
static int character_ids[3], getter_result, getter_arg, stored_address, stored_value;
static int getter_count, writer_count;

#ifdef OBSERVE_ACCESSES
struct Access { void *p; unsigned size, writing; };
static struct Access order[32];
static unsigned queued, consumed;
static void expect(void *p, unsigned size, unsigned write)
{
    assert(queued < sizeof(order) / sizeof(order[0]));
    order[queued++] = (struct Access){p, size, write};
}
static void observed(void *p, unsigned size, unsigned write)
{
    assert(consumed < queued);
    struct Access a = order[consumed++];
    assert(p == a.p && size == a.size && write == a.writing);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) {(void)p;}
void __tsan_func_exit(void) {}
#define TRACK_SIZE(s) \
  void __tsan_read##s(void *p) { observed(p,s,0); } \
  void __tsan_write##s(void *p) { observed(p,s,1); } \
  void __tsan_volatile_read##s(void *p) { observed(p,s,0); } \
  void __tsan_volatile_write##s(void *p) { observed(p,s,1); }
TRACK_SIZE(1) TRACK_SIZE(2) TRACK_SIZE(4) TRACK_SIZE(8)
#define DONE() assert(consumed == queued)
#else
#define expect(p,s,w) ((void)0)
#define DONE() ((void)0)
#endif
#define READ(p) expect((void *)(p), sizeof(*(p)), 0)
#define WRITE(p) expect((void *)(p), sizeof(*(p)), 1)

static void check_state(void)
{
    assert(memcmp(&expected,&live,sizeof(live)) == 0);
    assert(g_pGameState == &live.games[game]);
    assert(g_FieldScriptVMCurActor == &live.actors[actor]);
    assert(g_FieldScriptVMCurScriptData == live.scripts[script]);
    assert(memcmp(character_ids,g_GamePartyMembers,sizeof(character_ids)) == 0);
}
static void init_bank(struct State *s)
{
    s->actors[actor].scriptInstructionPointer = cursor;
    s->scripts[script][cursor+1] = destination;
    s->scripts[script][cursor+3] = slot;
    for (unsigned j=0; j<MAX_GAME_CHARACTERS; j++) {
        GameCharacter *c = &s->games[game].characters[j];
        c->hp = base_current ^ 0x575; c->mp = base_current;
        c->maxHp = base_max; c->maxMp = base_max ^ 0x2000;
    }
}
static void set_context(void)
{
    init_bank(&live); init_bank(&expected);
    g_pGameState=&live.games[game];
    g_FieldScriptVMCurActor=&live.actors[actor];
    g_FieldScriptVMCurScriptData=live.scripts[script];
    memcpy(g_GamePartyMembers,character_ids,sizeof(character_ids));
}
static void expect_lookup(unsigned operand, unsigned expected_slot)
{
    (void)operand; (void)expected_slot;
    READ(&g_FieldScriptVMCurActor); READ(&g_FieldScriptVMCurScriptData);
    READ(&live.actors[actor].scriptInstructionPointer);
    READ(&live.scripts[script][cursor+operand]);
    READ(&g_GamePartyMembers[expected_slot]);
}
static void expect_finish(void)
{
    READ(&g_FieldScriptVMCurActor);
    READ(&live.actors[actor].scriptInstructionPointer);
    WRITE(&live.actors[actor].scriptInstructionPointer);
    expected.actors[actor].scriptInstructionPointer = (uint16_t)(cursor+4);
}
static int service_get(int arg)
{
    DONE(); check_state(); assert(getter_count++ == 0);
    getter_arg=arg;
    if (test % 4) {
        if (test % 4 >= 2) actor=1;
        script=game=1; cursor=(cursor+9)&65535;
        slot=(slot+1)%3; destination=(destination+2)%3;
        for (unsigned j=0;j<3;j++) character_ids[j]=(character_ids[j]+1)%11;
        set_context();
    }
    unsigned resolved = handler<2 ? slot : destination;
    unsigned id=character_ids[resolved];
    GameCharacter *c=&live.games[game].characters[id];
    (void)c;
    GameCharacter *e=&expected.games[game].characters[id];
    expect_lookup(handler<2 ? 3:1, resolved);
    READ(&g_pGameState);
    if (handler<2) {
        if (handler == 0) READ(&c->hp); else READ(&c->mp);
    } else {
        int bound = handler==2 ? c->maxHp : c->maxMp;
        uint16_t result=(uint16_t)(getter_result < bound ? getter_result : bound);
        if (handler==2) {READ(&c->maxHp); WRITE(&c->hp); e->hp=result;}
        else {READ(&c->maxMp); WRITE(&c->mp); e->mp=result;}
        expect_finish();
    }
    return getter_result;
}
int FieldScriptVMGetArgument(int offset)
{
    assert(handler>=2 && offset==2);
    return service_get(offset);
}
int FieldScriptVMGetInstructionArgument(int offset)
{
    assert(handler<2 && offset==1);
    return service_get(offset);
}
void FieldScriptMemoryWriteU16(int address,int value)
{
    DONE(); check_state();
    assert(handler<2 && getter_count==1 && writer_count++==0);
    GameCharacter *c=&expected.games[game].characters[character_ids[slot]];
    assert(address==(uint16_t)getter_result && value==(handler==0 ? c->hp : c->mp));
    stored_address=address; stored_value=value;
    if (test%3) {
        if (test%3==2) actor=2;
        cursor=test%2 ? 65535:65534;
        g_FieldScriptVMCurActor=&live.actors[actor];
        live.actors[actor].scriptInstructionPointer=expected.actors[actor].scriptInstructionPointer=cursor;
    }
    expect_finish();
}
int main(void)
{
    static void (*const run[])(void)={FieldScriptVMHandlerWritePartyMemberHp,FieldScriptVMHandlerWritePartyMemberMp,
                                     FieldScriptVMHandlerSetPartyMemberHp,FieldScriptVMHandlerSetPartyMemberMp};
    static const unsigned ips[]={0,1,2,63,32767,65532,65533,65534,65535};
    static const uint16_t caps[]={0,1,0x7ffe,0x7fff,0x8000,0xfffe,0xffff,153};
    for (handler=0;handler<4;handler++) for (test=0;test<288;test++) {
        memset(&live,0xa5,sizeof(live)); memcpy(&expected,&live,sizeof(live));
        actor=script=game=0;
        cursor=ips[test%9];slot=(test/3)%3;destination=(test+2)%3;
        for (unsigned j=0;j<3;j++) character_ids[j]=(test*3+j*4)%11;
        if (test%12==0) character_ids[slot]=255;
        base_max=caps[(test/3)%8];
        const uint16_t currents[]={0,1,0x7fff,0x8000,0xffff,(uint16_t)(test*641)};
        base_current=currents[test%6];
        const int values[]={INT32_MIN,-65537,-65536,-65535,-4,-1,0,1,base_max-1,base_max,base_max+1,
                            0x7fff,0x8000,0xffff,0x10000,0x76543210,INT32_MAX};
        getter_result=values[test%17];set_context();
        getter_arg=stored_address=stored_value=-1;getter_count=writer_count=0;
#ifdef OBSERVE_ACCESSES
        consumed=queued=0;
#endif
        expect_lookup(3,slot);
        if (character_ids[slot]==255) expect_finish();
        run[handler]();
        DONE();check_state();
        assert(getter_count==(test%12!=0));
        assert(writer_count==(test%12!=0 && handler<2));
    }
    puts("1152 party-script stat cases passed");
    return 0;
}
#endif
