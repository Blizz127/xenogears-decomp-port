/* Real retail controller; explicitly simulated external callback contracts. */
#include "battle_mips_adapter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#if defined(XBT_UI_REAL_DISPLAY) && !defined(XBT_UI_NATIVE)
#error XBT_UI_REAL_DISPLAY requires XBT_UI_NATIVE
#endif

enum { LOOKUP=0x80089C08, DISPLAY=0x800BC404, HIGHLIGHT=0x800BCD98,
       FRAME=0x800716D8, PICK=0x80084854, OWNER=0xC3EAC, EVENT=0xD3014 };
typedef struct { uint32_t fn,a,b,result; unsigned owner, entry_owner; uint8_t event, before_targets[2], entry_event; } Call;
static Call expected[128];
static unsigned count, cursor, actor, swap_owner, model_owner;
static unsigned terminal_event, target_writes;
static unsigned entry_gate;
static uint8_t ram[0x200000], contexts[2][0x4000], final_contexts[2][0x4000];
static uint8_t initial_contexts[2][0x4000];
#ifdef XBT_UI_NATIVE
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t u8;
struct BattleCommandContext;
struct BattleCommandContext *D_800C3EAC;
u8 D_800D3014;
static int native_active;
extern u32 func_80084B40(u32);
#endif
#ifdef XBT_UI_REAL_DISPLAY
static unsigned display_entries, display_callbacks, display_pending, display_writes;
static uint16_t display_value;
u8 D_800C37C8[2];
u16 D_800C3CDC[2], D_80059454[2];
extern void PcPortUiNativeDisplay(u32);
#endif
static const unsigned bases[2]={0x100000,0x110000};
static unsigned model_event;
static unsigned model_visible_event;
static int bridge(void *, PcPortMipsCpu *, uint32_t);
#define SKIPPED_DISPLAY 0xFFFFFFF0u
static uint32_t get(unsigned at, unsigned width) {
    uint32_t value=0;
    for (unsigned i=0; i<width; i++) value|=(uint32_t)ram[at+i]<<(8*i);
    return value;
}
static void put(unsigned at, unsigned width, uint32_t value) {
    for (unsigned i=0; i<width; i++) ram[at+i]=(uint8_t)(value>>(8*i));
}
static unsigned mask(unsigned target) { return (target*0x123+0x8123)&0xFFFF; }
static unsigned add_call(unsigned fn, unsigned a, unsigned b, unsigned result) {
    assert(count<128);
    uint8_t before0=contexts[0][actor*64+0x3C], before1=contexts[1][actor*64+0x3C];
    unsigned entry_owner=model_owner;
    if (fn==DISPLAY && swap_owner && !entry_gate) {
        model_owner^=1;
        contexts[model_owner][actor*64+0x3C]+=7;
    }
    expected[count++]=(Call){fn,a,b,result,model_owner,entry_owner,(uint8_t)model_event,{before0,before1},(uint8_t)model_visible_event};
    if (fn==FRAME) model_visible_event=model_event;
    return result;
}
static unsigned lookup(unsigned target) { return add_call(LOOKUP,target,0,mask(target)); }
static int read_bus(void *opaque, uint32_t address, unsigned width, uint32_t *value) {
    (void)opaque;
    unsigned at=address&0x1FFFFFFF;
    assert((uint64_t)at+width<=sizeof(ram));
#ifdef XBT_UI_REAL_DISPLAY
    if (address==DISPLAY) {
        assert(width==4 && get(0xC37C8,1)==entry_gate);
        display_entries++;
        if (entry_gate) {
            /* Observation only: discard clobbers and execute every guest instruction. */
            PcPortMipsCpu observation=*(PcPortMipsCpu *)opaque;
            assert(bridge(NULL,&observation,SKIPPED_DISPLAY)==1);
        }
    }
#endif
    *value=get(at,width); return 0;
}
static int write_bus(void *opaque, uint32_t address, unsigned width, uint32_t value) {
    (void)opaque;
    unsigned at=address&0x1FFFFFFF;
    int stack=width==4 && !(at&3) && at>=0x1FEFD8 && at<=0x1FEFFC;
#ifdef XBT_UI_REAL_DISPLAY
    stack |= width==4 && (at==0x1FEFC0 || at==0x1FEFC4);
    if (at==0x59454) {
        assert(width==2 && value==display_value && !display_pending);
        display_writes++; put(at,width,value); return 0;
    }
#endif
    int event=width==1 && at==EVENT;
    unsigned owner=get(OWNER,4)&0x1FFFFFFF;
    int target=width==1 && at==owner+actor*64+0x3C;
    assert(stack || event || target);
    if (target) {
        assert(terminal_event!=5 && cursor==count && target_writes==0);
        assert(value==final_contexts[model_owner][actor*64+0x3C]);
        target_writes++;
    }
    put(at,width,value); return 0;
}
static int bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target) {
    (void)opaque;
#ifdef XBT_UI_REAL_DISPLAY
    if (target==DISPLAY) return 0; /* Always execute the actual guest wrapper. */
    if (target==SKIPPED_DISPLAY) { assert(entry_gate); target=DISPLAY; }
    if (target==0x800BC2F0) {
        assert(entry_gate==0);
        assert(cpu->gpr[4]==1 && !display_pending && display_entries==display_callbacks+1);
        assert(cursor<count && expected[cursor].fn==DISPLAY);
        display_pending=1;
        if (native_active) { D_800C37C8[0]=255; D_800C3CDC[0]^=0x1111; }
        else { put(0xC37C8,1,255); put(0xC3CDC,2,get(0xC3CDC,2)^0x1111); }
        for (unsigned r=2; r<=15; r++) cpu->gpr[r]=0xBAD10000+r;
        return 1;
    }
    if (target==0x800BC460) {
        assert(display_pending==1);
        display_pending=0; display_callbacks++; target=DISPLAY;
    }
#endif
    if (target!=LOOKUP && target!=DISPLAY && target!=HIGHLIGHT && target!=FRAME && target!=PICK)
        return 0;
    assert(cursor<count);
#ifdef XBT_UI_REAL_DISPLAY
    if (native_active) assert(D_80059454[0]==display_value && D_80059454[1]==0xBEEF);
    else assert(get(0x59454,2)==display_value && get(0x59456,2)==0xBEEF);
#endif
#ifdef XBT_UI_EARLY_WRITE
    if (!cursor) write_bus(NULL,0x80000000|(bases[0]+actor*64+0x3C),1,0);
#endif
    const Call *call=&expected[cursor++];
#ifdef XBT_UI_NATIVE
    if (native_active) {
        assert(D_800D3014==call->entry_event);
        assert((u8 *)D_800C3EAC==ram+bases[call->entry_owner]);
    } else
#endif
    {
        assert(ram[EVENT]==call->entry_event);
        assert(get(OWNER,4)==(0x80000000|bases[call->entry_owner]));
    }
    for (unsigned i=0; i<2; i++) {
        contexts[i][actor*64+0x3C]=call->before_targets[i];
        assert(!memcmp(ram+bases[i],contexts[i],sizeof(contexts[i])));
    }
    assert(target==call->fn);
    if (target!=FRAME) assert(cpu->gpr[4]==call->a);
    if (target==PICK) assert(cpu->gpr[5]==call->b);
#ifdef XBT_UI_REAL_DISPLAY
    if (target==DISPLAY) {
        if (entry_gate) display_value=native_active ? D_800C3CDC[0] : (uint16_t)get(0xC3CDC,2);
        else {
            display_value=(uint16_t)(call->a^0x5A5A);
            if (native_active) { D_800C37C8[0]=0; D_800C3CDC[0]=display_value; }
            else { put(0xC37C8,1,0); put(0xC3CDC,2,display_value); }
        }
    }
#endif
    if (target==DISPLAY && swap_owner && !entry_gate) {
        put(OWNER,4,0x80000000|bases[call->owner]);
        ram[bases[call->owner]+actor*64+0x3C]+=7;
#ifdef XBT_UI_NATIVE
        if (native_active) D_800C3EAC=(struct BattleCommandContext *)(ram+bases[call->owner]);
#endif
    }
    if (target==FRAME) ram[EVENT]=call->event;
#ifdef XBT_UI_NATIVE
    if (target==FRAME && native_active) D_800D3014=call->event;
#endif
    /* Legal caller-saved clobbers ensure the loop preserves its own state. */
    for (unsigned r=2; r<=15; r++) cpu->gpr[r]=0xBAD00000+r;
    cpu->gpr[24]=0xBAD00018; cpu->gpr[25]=0xBAD00019;
    cpu->gpr[2]=call->result;
    return 1;
}
#ifdef XBT_UI_NATIVE
static u32 native_call(u32 target, u32 a, u32 b) {
    PcPortMipsCpu cpu={0}; cpu.gpr[4]=a; cpu.gpr[5]=b;
    assert(bridge(NULL,&cpu,target)==1); return cpu.gpr[2];
}
u16 func_80089C08(u8 target) { return (u16)native_call(LOOKUP,target,0); }
#ifdef XBT_UI_REAL_DISPLAY
void func_800BC404(u32 value) {
    assert(D_800C37C8[0]==entry_gate);
    display_entries++;
    if (entry_gate) (void)native_call(SKIPPED_DISPLAY,value,0);
    PcPortUiNativeDisplay(value);
    assert(D_800C3CDC[0]==display_value && D_800C3CDC[1]==0xCDEF);
}
void func_800BC2F0(u32 value) { (void)native_call(0x800BC2F0,value,0); }
void func_800BC460(u32 value) { (void)native_call(0x800BC460,value,0); }
#else
void func_800BC404(u32 value) { (void)native_call(DISPLAY,value,0); }
#endif
void func_800BCD98(u32 value) { (void)native_call(HIGHLIGHT,value,0); }
void func_800716D8(void) { (void)native_call(FRAME,0,0); }
u32 func_80084854(u32 actor_arg, u32 direction) { return native_call(PICK,actor_arg,direction); }
#include "../../src/battle/target_ui_impl.inc"
#endif
static void run(unsigned actor_word, unsigned initial, unsigned terminal, unsigned swapping, unsigned odd, unsigned gate) {
    entry_gate=gate;
    actor=actor_word&255; swap_owner=swapping; model_owner=0; model_event=8;
    model_visible_event=8;
    terminal_event=terminal; target_writes=0;
    memset(contexts,0xA5,sizeof(contexts));
    contexts[0][0x2E8]=(uint8_t)initial;
    contexts[1][0x2E8]=(uint8_t)(initial^0xFF);
    contexts[0][actor*64+0x3C]=17;
    contexts[1][actor*64+0x3C]=29;
    memcpy(initial_contexts,contexts,sizeof(contexts));
    for (unsigned i=0; i<2; i++) memcpy(ram+bases[i],contexts[i],sizeof(contexts[i]));
    put(OWNER,4,0x80000000|bases[0]); ram[EVENT]=0xDE;
    count=cursor=0;
#ifdef XBT_UI_REAL_DISPLAY
    display_entries=display_callbacks=display_pending=display_writes=0;
    display_value=0x2468;
    put(0xC37C8,2,0xAB00|entry_gate); put(0xC3CDC,4,0xCDEF1357); put(0x59454,4,0xBEEF2468);
#endif
    unsigned selected=initial;
    const unsigned events[]={8,9,255,0,1,2,3,terminal};
    for (unsigned i=odd; i<sizeof(events)/sizeof(events[0]); i++) {
        unsigned own_mask=lookup(actor), selected_mask=lookup(selected);
        add_call(DISPLAY,(own_mask|selected_mask)&0xFFFF,0,0);
        selected_mask=lookup(selected);
        add_call(HIGHLIGHT,selected_mask&0xFFFF,0,0);
        model_event=events[i]; add_call(FRAME,0,0,0);
        if (model_event<4) {
            unsigned next=(selected+model_event+1)&255;
            add_call(PICK,selected,model_event,next); selected=next;
        }
        if (model_event!=terminal) model_visible_event=8;
        /* Default events do nothing. Event8 keeps the inner wait loop alive. */
    }
    unsigned result;
    if (terminal==5) {
        unsigned own_mask=lookup(actor);
        unsigned target_mask=lookup(contexts[model_owner][actor*64+0x3C]);
        add_call(DISPLAY,(own_mask|target_mask)&0xFFFF,0,0);
        target_mask=lookup(contexts[model_owner][actor*64+0x3C]);
        add_call(HIGHLIGHT,target_mask&0xFFFF,0,0);
        result=0;
    } else {
        contexts[model_owner][actor*64+0x3C]=(uint8_t)selected;
        result=1;
    }
    memcpy(final_contexts,contexts,sizeof(contexts));
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={.opaque=&cpu,.read=read_bus,.write=write_bus,.bridge=bridge};
    PcPortMipsCpuInit(&cpu,&bus);
    for (unsigned r=16; r<=30; r++) cpu.gpr[r]=0xABCD0000+r;
    cpu.gpr[4]=actor_word; cpu.gpr[29]=0x801FF000; cpu.gpr[31]=0xFFFFFFFC;
    assert(PcPortMipsRun(&cpu,0x80084B40,0xFFFFFFFC,10000)==0);
    assert(cursor==count && cpu.gpr[2]==result && ram[EVENT]==8);
    assert(target_writes==(terminal!=5));
    if (swapping && terminal!=5) assert(model_owner==(entry_gate ? 0 : odd));
    assert(get(OWNER,4)==(0x80000000|bases[model_owner]));
    for (unsigned i=0; i<2; i++) assert(!memcmp(ram+bases[i],final_contexts[i],sizeof(contexts[i])));
    for (unsigned r=16; r<=23; r++) assert(cpu.gpr[r]==0xABCD0000+r);
    assert(cpu.gpr[30]==0xABCD001E);
    assert(cpu.gpr[29]==0x801FF000 && cpu.gpr[31]==0xFFFFFFFC);
#ifdef XBT_UI_REAL_DISPLAY
    unsigned expected_displays=8-odd+(terminal==5);
    assert(display_entries==expected_displays && display_callbacks==(entry_gate ? 0 : expected_displays));
    assert(display_writes==expected_displays && !display_pending);
    assert(get(0x59454,4)==(0xBEEF0000u|display_value));
    assert(get(0xC37C8,2)==(0xAB00|entry_gate) && get(0xC3CDE,2)==0xCDEF);
#endif
#ifdef XBT_UI_NATIVE
    for (unsigned i=0; i<2; i++) memcpy(ram+bases[i],initial_contexts[i],sizeof(contexts[i]));
    put(OWNER,4,0x80000000|bases[0]);
    D_800C3EAC=(struct BattleCommandContext *)(ram+bases[0]); D_800D3014=0xDE;
    cursor=0; native_active=1;
#ifdef XBT_UI_REAL_DISPLAY
    display_entries=display_callbacks=display_pending=0; display_value=0x2468;
    D_800C37C8[0]=(u8)entry_gate; D_800C37C8[1]=0xAB;
    D_800C3CDC[0]=0x1357; D_800C3CDC[1]=0xCDEF;
    D_80059454[0]=0x2468; D_80059454[1]=0xBEEF;
#endif
    assert(func_80084B40(actor_word)==result);
    native_active=0;
    assert(cursor==count && D_800D3014==8);
    assert((u8 *)D_800C3EAC==ram+bases[model_owner]);
    for (unsigned i=0; i<2; i++) assert(!memcmp(ram+bases[i],final_contexts[i],sizeof(contexts[i])));
#ifdef XBT_UI_REAL_DISPLAY
    assert(display_entries==expected_displays && display_callbacks==(entry_gate ? 0 : expected_displays) && !display_pending);
    assert(D_80059454[0]==display_value && D_80059454[1]==0xBEEF);
    assert(D_800C37C8[0]==entry_gate && D_800C37C8[1]==0xAB && D_800C3CDC[1]==0xCDEF);
#endif
#endif
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x15050,SEEK_SET));
    assert(fread(ram+0x84B40,1,0x1E8,f)==0x1E8);
    assert(!fseek(f,0x720,SEEK_SET));
    assert(fread(ram+0x70210,1,32,f)==32); fclose(f);
#ifdef XBT_UI_REAL_DISPLAY
    f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4C914,SEEK_SET));
    assert(fread(ram+0xBC404,1,0x50,f)==0x50); fclose(f);
#endif
#ifdef XBT_UI_BAD_CANCEL
    /* Isolated table-copy mutant: event5 incorrectly confirms. */
    put(0x70210+5*4,4,0x80084C30);
#endif
#ifdef XBT_UI_STALE_OWNER
    /* Isolated instruction-copy mutant: confirm writes to initial owner. */
    put(0x84C34,4,0x3C028010);
#endif
    unsigned cases=0;
    const unsigned actors[]={0xABCD0000,0x12340003,0x567800FF};
    for (unsigned a=0; a<3; a++) for (unsigned selected=0; selected<256; selected++)
    for (unsigned terminal=4; terminal<=7; terminal++) for (unsigned swapping=0; swapping<2; swapping++)
    for (unsigned odd=0; odd<2; odd++) {
#ifdef XBT_UI_REAL_DISPLAY
        const unsigned gates[]={0,1,255};
        for (unsigned g=0; g<3; g++) { run(actors[a],selected,terminal,swapping,odd,gates[g]); cases++; }
#else
        run(actors[a],selected,terminal,swapping,odd,0); cases++;
#endif
    }
#ifdef XBT_UI_REAL_DISPLAY
    assert(cases==36864);
#else
    assert(cases==12288);
#endif
    printf("TARGET UI retail/controller contract PASS %u cases, simulated callbacks\n",cases);
#ifdef XBT_UI_NATIVE
    printf("TARGET UI native/retail differential PASS %u cases, simulated callbacks\n",cases);
#endif
#ifdef XBT_UI_REAL_DISPLAY
    printf("TARGET UI real BC404 wrapper integration PASS %u cases, downstream callbacks simulated\n",cases);
#endif
}
