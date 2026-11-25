#include <assert.h>
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

enum { MAX_GAME_CHARACTERS = 11 };
typedef struct {
    uint32_t prefixGuard;
    u16 maxMp, hp, maxHp, mp;
    uint32_t suffixGuard;
} GameCharacter;
typedef struct { GameCharacter characters[MAX_GAME_CHARACTERS]; } GameState;
extern GameState *g_pGameState;

#ifdef BUILD_COMMANDS
int FieldScriptVMGetArgument(int offset);
#ifndef PARTY_IMPLEMENTATION
#define PARTY_IMPLEMENTATION "../../src/cleanroom/field_party_restore_vitals.inc"
#endif
#include PARTY_IMPLEMENTATION
#else
GameState *g_pGameState;
ActorData *g_FieldScriptVMCurActor;
struct Runtime { GameState states[2]; ActorData actors[2]; };
static struct Runtime actual, expected;
static unsigned handler, test, bank, period, getter_calls;
extern void FieldScriptVMHandlerRestoreCharacterHpAndMp(void);
extern void FieldScriptVMHandlerRestoreHp(void);
extern void FieldScriptVMHandlerRestoreMp(void);

#ifdef OBSERVE_ACCESSES
struct Access { void *p; unsigned width, writing; };
static struct Access order[32];
static unsigned queued, consumed;
static void expect(void *p, unsigned width, unsigned writing)
{
    assert(queued < 32);
    order[queued++] = (struct Access){p,width,writing};
}
static void observed(void *p, unsigned width, unsigned writing)
{
    assert(consumed < queued);
    struct Access a = order[consumed++];
    assert(p==a.p && width==a.width && writing==a.writing);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) {(void)p;}
void __tsan_func_exit(void) {}
#define CHECK_SIZE(size) \
  void __tsan_read##size(void *p) {observed(p,size,0);} \
  void __tsan_write##size(void *p) {observed(p,size,1);} \
  void __tsan_volatile_read##size(void *p) {observed(p,size,0);} \
  void __tsan_volatile_write##size(void *p) {observed(p,size,1);}
CHECK_SIZE(1) CHECK_SIZE(2) CHECK_SIZE(4) CHECK_SIZE(8)
#define DONE() assert(consumed == queued)
#else
#define expect(p,w,d) ((void)sizeof(p))
#define DONE() ((void)0)
#endif
#define READ(p) expect((void *)(p),sizeof(*(p)),0)
#define WRITE(p) expect((void *)(p),sizeof(*(p)),1)

static void select_bank(unsigned new_bank)
{
    bank = new_bank;
    g_pGameState=&actual.states[bank];
    g_FieldScriptVMCurActor=&actual.actors[bank];
}
static void plan_effect(void)
{
    READ(&g_pGameState);
    if (handler==0) READ(&g_FieldScriptVMCurActor);
    for (unsigned j=0;j<MAX_GAME_CHARACTERS;j++) {
        if (handler==0 && j != test%MAX_GAME_CHARACTERS) continue;
        GameCharacter *c=&actual.states[bank].characters[j];
        GameCharacter *e=&expected.states[bank].characters[j];
        if (handler!=2) READ(&c->maxHp);
        if (handler!=1) READ(&c->maxMp);
        if (handler!=2) {WRITE(&c->hp); e->hp=e->maxHp;}
        if (handler!=1) {WRITE(&c->mp); e->mp=e->maxMp;}
    }
    if (handler!=0) READ(&g_FieldScriptVMCurActor);
    READ(&actual.actors[bank].scriptInstructionPointer);
    WRITE(&actual.actors[bank].scriptInstructionPointer);
    expected.actors[bank].scriptInstructionPointer = period + (handler==0 ? 3:1);
}
int FieldScriptVMGetArgument(int arg)
{
    DONE(); assert(handler==0 && arg==1 && getter_calls++==0);
    assert(memcmp(&actual,&expected,sizeof(actual))==0);
    if (test%3) select_bank(1);
    plan_effect();
    return test%11;
}
int main(void)
{
    static void (*const run[])(void)={FieldScriptVMHandlerRestoreCharacterHpAndMp,FieldScriptVMHandlerRestoreHp,FieldScriptVMHandlerRestoreMp};
    for (handler=0;handler<3;handler++) for (test=0;test<165;test++) {
        const unsigned ips[]={0,1,0x7fff,0xfffd,0xfffe,0xffff,(test*337)&65535};
        period=ips[test%7]; getter_calls=0;
#ifdef OBSERVE_ACCESSES
        queued=consumed=0;
#endif
        memset(&actual,0xa5,sizeof(actual)); select_bank(0);
        for (unsigned b=0;b<2;b++) {
            actual.actors[b].scriptInstructionPointer=period;
            for (unsigned j=0;j<MAX_GAME_CHARACTERS;j++) {
                const unsigned v[]={0,1,0x7fff,0x8000,0xfffe,0xffff,(test*257+j*379)&65535,101};
                GameCharacter *c=&actual.states[b].characters[j];
                c->hp=v[(test+j)%8]; c->maxHp=v[(test+2*j+1)%8];
                c->mp=v[(test+3*j+2)%8]; c->maxMp=v[(test+5*j+3)%8];
                if (test%5==0) { c->hp=c->maxHp; c->mp=c->maxMp; }
            }
        }
        memcpy(&expected,&actual,sizeof(actual));
        if (handler!=0) plan_effect();
        run[handler]();
        DONE(); assert(getter_calls == (handler==0));
        assert(g_FieldScriptVMCurActor==&actual.actors[bank] && g_pGameState==&actual.states[bank]);
        assert(memcmp(&actual,&expected,sizeof(actual))==0);
    }
    puts("495 direct party restore cases passed");
    return 0;
}
#endif
