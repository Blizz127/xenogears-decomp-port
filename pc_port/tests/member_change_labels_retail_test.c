/* Compare the installed label drawer with its retail MIPS instructions.
 * Render/GPU boundaries record arguments; pointer rebinding at those boundaries
 * is an adversarial fixture for the source's required fresh loads. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"

enum { RAM_SIZE=0x200000, BASE=0x801c5000u, ENTRY=0x801c59e0u,
       HALT=0xfffffffcu, ITEMS=0x80010000u, INDICES=0x80020000u,
       TABLE=0x80030000u, STRINGS=0x80040000u, WORK=0x80050000u,
       MENU0=0x80070000u, MENU1=0x80074000u, MENU_GLOBAL=0x800625a0u };
typedef struct { uint32_t id, args[4]; } Event;
typedef struct { Event e[128]; unsigned n; } Trace;
static uint8_t ram[RAM_SIZE], module[0x10000], indices[16];
static size_t module_size;
static SystemMenu menus[2];
SystemMenu *g_Menu;
static MenuString strings[16], before[16];
static MenuString *native_items;
static uint32_t work_buffers[4][4];
static uint8_t string_tokens[1024];
static Trace trace;
static unsigned mode, mutation, stage, checks;
static PcPortMipsCpu *oracle_cpu;
static unsigned seen[108];
u8 D_801CB400[4]={5,17,64,255};
extern void func_801C59E0(void *, void *, s32, s32);
extern void func_801C5B90(void);

static void require(int ok, const char *why) {
    checks++;
    if (!ok) { fprintf(stderr,"MEMBER LABELS FAIL %s mode=%u mutation=%u stage=%u\n",why,mode,mutation,stage); exit(1); }
}
static uint8_t *guest(uint32_t a) { return ram+(a&0x1fffffu); }
static uint32_t word(const void *p) { uint32_t v; memcpy(&v,p,4); return v; }
static void w32(uint32_t a,uint32_t v) { memcpy(guest(a),&v,4); }
static uint32_t pointer_id(const void *p) {
    uintptr_t a=(uintptr_t)p, b=(uintptr_t)native_items;
    if(a>=b && a<b+16*sizeof(MenuString)) {
        size_t off=a-b, index=off/sizeof(MenuString), field=off%sizeof(MenuString);
        if(field==offsetof(MenuString,vramDest)) field=0x70;
        require(field==0 || field==0x70,"unexpected item pointer");
        return ITEMS+(uint32_t)index*0x80+(uint32_t)field;
    }
    for(unsigned i=0;i<4;i++) if(p==work_buffers[i]) return WORK+i*0x100;
    if(a>=(uintptr_t)string_tokens && a<(uintptr_t)string_tokens+sizeof(string_tokens))
        return STRINGS+(uint32_t)(a-(uintptr_t)string_tokens);
    return (uint32_t)a;
}
static void event(unsigned id,uint32_t a,uint32_t b,uint32_t c,uint32_t d) {
    require(trace.n<128,"trace overflow");
    trace.e[trace.n++]=(Event){id,{a,b,c,d}};
}
static void change_work(void) {
    if(!mutation) return;
    stage++;
    unsigned m=(mutation==2)?(stage/2)%2:0, w=stage%4;
    if(mode) { g_Menu=&menus[m]; g_Menu->unk4E0[0].pVramBuffer=work_buffers[w]; }
    else { w32(MENU_GLOBAL,m?MENU1:MENU0); w32((m?MENU1:MENU0)+0x558,WORK+w*0x100); }
}
void *GetStringEntry(void *p,s32 index) {
    event(1,pointer_id(p),(uint32_t)index,0,0); change_work();
    return string_tokens+index*4;
}
s32 SystemRenderStringEntry(void *p,void *work,s32 height,s32 flag) {
    uint32_t id=pointer_id(p);
    event(2,id,pointer_id(work),(uint32_t)height,(uint32_t)flag); change_work();
    return ((id-STRINGS)/4*7+(uint32_t)flag+3)&0xff;
}
void func_801C57A0(MenuString *p,int index,s32 arg2,u32 attr) {
    event(3,pointer_id(p),(uint32_t)index,(uint32_t)arg2,attr); change_work();
}
int LoadImage(RECT *rect,u_long *work) {
    event(4,pointer_id(rect),pointer_id(work),word(rect),word((uint8_t*)rect+4)); return 0;
}
int DrawSync(int value) { event(5,(uint32_t)value,0,0,0); return 0; }
void SystemTransferPaletteToVRAM(int a,int b) { event(6,(uint32_t)a,(uint32_t)b,0,0); }
void *HeapAlloc(u_int size,u_int flags) { event(7,size,flags,0,0); return work_buffers[0]; }
void func_801C5724(void) { event(8,0,0,0,0); }

static int read_bus(void *o,uint32_t a,unsigned width,uint32_t *v) {
    (void)o;
    if(a<0x80000000u || (uint64_t)a+width>0x80200000u) return -1;
    *v=0; for(unsigned i=0;i<width;i++) *v|=(uint32_t)guest(a)[i]<<(i*8);
    if(oracle_cpu && width==4 && a==oracle_cpu->pc && a>=ENTRY && a<ENTRY+432)
        seen[(a-ENTRY)/4]++;
    return 0;
}
static int write_bus(void *o,uint32_t a,unsigned width,uint32_t v) {
    (void)o;
    if(a<0x80000000u || (uint64_t)a+width>0x80200000u) return -1;
    for(unsigned i=0;i<width;i++) guest(a)[i]=(uint8_t)(v>>(i*8));
    return 0;
}
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t) {
    (void)o;
    uint32_t *a=c->gpr+4;
    switch(t) {
    case 0x80033728u: event(1,a[0],a[1],0,0); change_work(); c->gpr[2]=STRINGS+a[1]*4; return 1;
    case 0x80034eacu: event(2,a[0],a[1],a[2],a[3]); change_work(); c->gpr[2]=((a[0]-STRINGS)/4*7+a[3]+3)&0xff; return 1;
    case 0x801c57a0u: event(3,a[0],a[1],a[2],a[3]); change_work(); return 1;
    case 0x80044894u: event(4,a[0],a[1],word(guest(a[0])),word(guest(a[0]+4))); c->gpr[2]=0; return 1;
    case 0x800445d0u: event(5,a[0],0,0,0); c->gpr[2]=0; return 1;
    default: return 0;
    }
}
static void initialize(unsigned mut) {
    mutation=mut; stage=0; memset(&trace,0,sizeof(trace));
    memset(menus,0,sizeof(menus)); memset(strings,0x5a,sizeof(strings));
    memcpy(before,strings,sizeof(strings)); native_items=strings; g_Menu=&menus[0];
    for(unsigned i=0;i<2;i++) { menus[i].unk2E0=(void*)(uintptr_t)(TABLE+i*0x1000); menus[i].unk4E0[0].pVramBuffer=work_buffers[0]; }
    memset(ram,0,sizeof(ram)); memcpy(guest(BASE),module,module_size);
    memset(guest(ITEMS),0x5a,16*0x80); memcpy(guest(INDICES),indices,sizeof(indices));
    w32(MENU_GLOBAL,MENU0);
    for(unsigned i=0;i<2;i++) { uint32_t m=i?MENU1:MENU0; w32(m+0x2e0,TABLE+i*0x1000); w32(m+0x558,WORK); }
}
static void run_case(int count,int offset,unsigned mut) {
    initialize(mut); mode=0;
    PcPortMipsBus bus={0}; bus.read=read_bus; bus.write=write_bus; bus.bridge=bridge;
    PcPortMipsCpu cpu; PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[4]=ITEMS;cpu.gpr[5]=INDICES;cpu.gpr[6]=(uint32_t)offset;cpu.gpr[7]=(uint32_t)count;
    cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=HALT;
    oracle_cpu=&cpu;
    require(PcPortMipsRun(&cpu,ENTRY,HALT,10000)==PC_PORT_MIPS_HALTED,"retail execution");
    oracle_cpu=NULL;
    Trace expected=trace; uint8_t expected_items[16*0x80]; memcpy(expected_items,guest(ITEMS),sizeof(expected_items));
    initialize(mut);mode=1;
    func_801C59E0(strings,indices,offset,count);
    require(memcmp(&trace,&expected,sizeof(trace))==0,"boundary trace and fresh pointers");
    for(unsigned i=0;i<16;i++) {
        require(strings[i].width==expected_items[i*0x80+0x7e],"width");
        require(memcmp(&strings[i].vramDest,expected_items+i*0x80+0x70,8)==0,"rectangle");
        before[i].width=strings[i].width;before[i].vramDest=strings[i].vramDest;
    }
    require(memcmp(before,strings,sizeof(strings))==0,"other native item bytes preserved");
}
int main(int argc,char **argv) {
    require(argc==2,"retail module path"); FILE *f=fopen(argv[1],"rb"); require(f!=NULL,"open retail");
    module_size=fread(module,1,sizeof(module),f); require(!ferror(f)&&feof(f),"read bounded retail"); fclose(f);
    require(module_size>0xb90,"retail size");
    for(unsigned i=0;i<16;i++) indices[i]=(uint8_t)(i*17);
    const int counts[]={-7,0,1,2,3,4,8,15};
    const int offsets[]={-19,-4,-3,-1,0,1,3,4,17,123};
    unsigned cases=0;
    for(unsigned m=0;m<3;m++) for(unsigned c=0;c<sizeof(counts)/sizeof(counts[0]);c++)
        for(unsigned o=0;o<sizeof(offsets)/sizeof(offsets[0]);o++) {run_case(counts[c],offsets[o],m);cases++;}
    for(unsigned i=0;i<108;i++) require(seen[i]>0,"retail instruction coverage");
    initialize(0);mode=1;native_items=menus[0].unk4E0;
    g_Menu->unk4E0[0].pVramBuffer=work_buffers[3];
    func_801C5B90();
    require(g_Menu->unk4E0[0].pVramBuffer==work_buffers[0],"initializer typed work storage");
    require(trace.n==19 && trace.e[0].id==6 && trace.e[0].args[0]==0 && trace.e[0].args[1]==0x1d1 &&
            trace.e[1].id==7 && trace.e[1].args[0]==0x38e && trace.e[1].args[1]==0 && trace.e[18].id==8,"initializer boundary order");
    for(unsigned i=0;i<4;i++) require(g_Menu->unk4E0[i].width==(uint8_t)(D_801CB400[i]*7+(i&1)+3),"initializer typed item array");
    printf("MEMBER LABELS PASS cases=%u checks=%u retail_slots=108 initializer=1\n",cases,checks);
    return 0;
}
