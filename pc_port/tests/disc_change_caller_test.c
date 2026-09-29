#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"

SystemMenu *g_Menu;
static SystemMenu menus[2];
static u8 ram[0x200000];
typedef struct { u32 kind, arg, owner, state; } Event;
static Event trace[2][256];
static unsigned count[2],side,owner,frames,queries,validations;
static unsigned initial_frames,failures,finish_by_query,replace;
static s32 validation_error;
static u32 target_disc;
static unsigned transition_left;
static u32 guest_menu(unsigned i) {return 0x80100000+i*0x2000;}
static u8 *ptr(u32 a,unsigned w) {
    a&=0x1fffffff;assert((uint64_t)a+w<=sizeof ram);return ram+a;
}
static void put(u32 a,u32 v) {u8*p=ptr(a,4);for(unsigned i=0;i<4;++i)p[i]=v>>(i*8);}
static u8 state(void) {
    return side?g_Menu->transitionEffectState:*ptr(guest_menu(owner)+0x329,1);
}
static void set_state(u8 v) {
    if(side)g_Menu->transitionEffectState=v;else *ptr(guest_menu(owner)+0x329,1)=v;
}
static void event(unsigned kind,u32 arg) {
    assert(count[side]<256);
    trace[side][count[side]++]=(Event){kind,arg,owner,state()};
    if(replace==1 || (replace==2 && count[side]%3==0)) {
        owner^=1;
        if(side)g_Menu=&menus[owner];else put(0x800625a0,guest_menu(owner));
    }
}
static void func_801D1E80(void) {
    event(0,0);transition_left=initial_frames;set_state(transition_left?0xa7:0);
}
static void func_801C7BF4(void) {
    event(1,0);++frames;
    if(transition_left)--transition_left;
    set_state(transition_left?0x81:0);
}
static void func_801D22F4(s32 arg) {event(2,arg);}
static s32 ArchiveGetDiscNumber(void) {
    event(3,0);++queries;
    return finish_by_query && validations>=failures ? target_disc : -19;
}
static void func_801E92CC(void) {event(4,0);}
static void func_801D2F4C(s32 arg) {event(5,arg);}
static s32 func_801E93A0(s32 arg) {
    event(6,arg);assert((u32)arg==target_disc);
    return validations++<failures ? validation_error : 0;
}
/* This retail callee takes no argument; incidental a0 contents are ignored. */
static void func_801D32B4() {event(7,0);}
static void func_801D2484(void) {event(8,0);}
#include "disc_caller.inc"

static int read_bus(void*o,u32 a,unsigned w,u32*v) {
    (void)o;u8*p=ptr(a,w);*v=0;for(unsigned i=0;i<w;++i)*v|=(u32)p[i]<<(i*8);return 0;
}
static int write_bus(void*o,u32 a,unsigned w,u32 v) {
    (void)o;u8*p=ptr(a,w);for(unsigned i=0;i<w;++i)p[i]=v>>(i*8);return 0;
}
static int bridge(void*o,PcPortMipsCpu*c,u32 target) {
    (void)o;
    switch(target) {
    case 0x801d1e80:func_801D1E80();break;
    case 0x801c7bf4:func_801C7BF4();break;
    case 0x801d22f4:func_801D22F4(c->gpr[4]);break;
    case 0x80028530:c->gpr[2]=ArchiveGetDiscNumber();break;
    case 0x801e92cc:func_801E92CC();break;
    case 0x801d2f4c:func_801D2F4C(c->gpr[4]);break;
    case 0x801e93a0:c->gpr[2]=func_801E93A0(c->gpr[4]);break;
    case 0x801d32b4:func_801D32B4();break;
    case 0x801d2484:func_801D2484();break;
    default:return 0;
    }
    return 1;
}
static void reset(unsigned native) {
    side=native;owner=0;frames=queries=validations=transition_left=0;
    if(native) {memset(menus,0,sizeof menus);g_Menu=&menus[0];}
    else {memset(ptr(guest_menu(0),0x4000),0,0x4000);put(0x800625a0,guest_menu(0));}
}
int main(void) {
    FILE*f=fopen("disc/menu.bin","rb");assert(f);
    assert(fread(ram+0x1c5000,1,0x3b000,f)>0x25000);fclose(f);
    assert(offsetof(SystemMenu,transitionEffectState)!=0x329);
    const u32 upper[]={0,0x12340000,0xffffff00};
    unsigned fixtures=0;
    for(unsigned byte=0;byte<256;++byte)for(unsigned high=0;high<3;++high)
    for(initial_frames=0;initial_frames<3;++initial_frames)
    for(failures=0;failures<3;++failures)
    for(finish_by_query=0;finish_by_query<2;++finish_by_query)
    for(replace=0;replace<3;++replace) {
        target_disc=byte+1;validation_error=high==0?1:high==1?2:-1;
        memset(trace,0,sizeof trace);count[0]=count[1]=0;reset(0);
        PcPortMipsBus bus={.read=read_bus,.write=write_bus,.bridge=bridge};PcPortMipsCpu cpu;
        PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=upper[high]|byte;
        cpu.gpr[29]=0x801b0000;cpu.gpr[31]=0xfffffffc;
        int status=PcPortMipsRun(&cpu,0x801c8694,0xfffffffc,20000);
        if(status)fprintf(stderr,"MIPS %d %s pc=%08x\n",status,cpu.error,cpu.pc);
        assert(status==PC_PORT_MIPS_HALTED);
        unsigned gframes=frames,gqueries=queries,gvalidations=validations,gowner=owner;
        assert(gframes==initial_frames+30*failures);
        assert(gqueries==failures+1);
        assert(gvalidations==failures+!finish_by_query);
        reset(1);func_801C8694((s32)(upper[high]|byte));
        assert(count[0]==count[1]);assert(!memcmp(trace[0],trace[1],sizeof trace[0]));
        assert(frames==gframes && queries==gqueries && validations==gvalidations && owner==gowner);
        for(unsigned i=0;i<2;++i) {
            assert(menus[i].transitionEffectState==*ptr(guest_menu(i)+0x329,1));
            /* Callees may update only the named state byte in this fixture. */
            u8 saved=menus[i].transitionEffectState;menus[i].transitionEffectState=0;
            const u8*p=(const u8*)&menus[i];for(size_t j=0;j<sizeof menus[i];++j)assert(p[j]==0);
            menus[i].transitionEffectState=saved;
        }
        ++fixtures;
    }
    printf("PASS %u retail/native disc-change caller fixtures; UI and CD boundaries intercepted\n",fixtures);
}
