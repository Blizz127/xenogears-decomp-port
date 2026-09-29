#include "common.h"
#include "main/game.h"
#include "field/actor.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void func_8009FDD4(void);
void func_8009FE4C(void);
s32 D_800AFD1C;
s32 D_8006F990[3];
int g_GamePartyMembers[MAX_PARTY_MEMBERS];
static GameState state;
GameState* g_pGameState=&state;
static ActorData actor;
ActorData* g_FieldScriptVMCurActor=&actor;
static u8 script[0x10000];
void* g_FieldScriptVMCurScriptData=script;
static int calls,slot_seen;
void func_800AD4D4(s32 slot) { ++calls; slot_seen=slot; }
void func_800ACFD0(s32 slot) { ++calls; slot_seen=slot; }

static int check(int want,int slot,unsigned ip) {
    if(calls!=want || (want && slot_seen!=slot) || actor.scriptInstructionPointer!=ip) {
        fprintf(stderr,"FIELD GEAR DISPATCH FAIL calls=%d/%d slot=%d/%d ip=%u/%u\n",
                calls,want,slot_seen,slot,actor.scriptInstructionPointer,ip);
        return 0;
    }
    return 1;
}
int main(void) {
    for(int slot=0;slot<3;++slot) for(int riding=0;riding<2;++riding) {
        memset(&state,0,sizeof(state));memset(&actor,0,sizeof(actor));
        for(int i=0;i<3;++i) D_8006F990[i]=100+i;
        D_800AFD1C=100+slot;
        ((u8*)&state)[0x22b1+slot]=riding;
        actor.scriptInstructionPointer=0xffff;
        calls=0;slot_seen=-1;
        func_8009FDD4();
        if(!check(!riding,slot,0)) return 1;
        for(int present=0;present<2;++present) {
            actor.scriptInstructionPointer=10;script[11]=slot;
            g_GamePartyMembers[slot]=present?slot:0xff;
            calls=0;slot_seen=-1;
            func_8009FE4C();
            if(!check(present && riding,slot,12)) return 1;
        }
    }
    D_800AFD1C=999;actor.scriptInstructionPointer=42;calls=0;
    func_8009FDD4();
    if(!check(0,0,43)) return 1;
    puts("FIELD GEAR DISPATCH PASS slots/ride-state/party-presence/IP-wrap/no-owner");
    return 0;
}
