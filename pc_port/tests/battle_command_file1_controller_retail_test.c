#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "battle_mips_adapter.h"
#include "xeno_battle_command_file1_controller.h"

enum {
    RAM_SIZE = 0x200000, MODULE_ADDR = 0x801e5000,
    MODULE_SIZE = 0x4c3c, ENTRY = 0x801e6ce8,
    HALT = 0xfffffffcu, STACK = 0x801ff000u,
    CONTROL_ADDR = 0x80050000u, UI_ADDR = 0x80051000u,
    WINDOW_ADDR = 0x80052000u, TABLE_ADDR = 0x80053000u,
    STRING_ADDR = 0x80054000u, ALT_WINDOW_ADDR = 0x80052100u,
};

enum EventId {
    E_SETUP, E_YIELD, E_LOCAL6750, E_CTOR, E_RESET,
    E_GET_STRING, E_QUEUE, E_LOCAL5B00, E_CLEAR_WAIT, E_DESTROY, E_CLEANUP
};

typedef struct Event { uint32_t id, a[8]; } Event;
typedef struct Trace { Event e[32]; unsigned n, overflow; } Trace;
typedef struct Case {
    uint16_t arg0, arg2, x, y, units, rows, fallback;
    uint8_t arg1, phase, ui_bf, ui_mode;
    uint16_t window_flags;
    int16_t cursor_x, cursor_y;
    int32_t saved_x, saved_y;
    uint8_t rebind_on_lookup;
    const char *name;
} Case;

uint8_t g_PsxRam[RAM_SIZE];
static uint8_t host_control[0x828], host_ui[0x200];
static uint8_t host_window[0x98], host_alt_window[0x98];
xbcf1_u8 *D_800D3278 = host_control;
xbcf1_u8 *D_800D2D28 = host_ui;
xbcf1_u8 *D_800D2DAC = host_window;
void *D_800D3340 = (void *)(uintptr_t)TABLE_ADDR;
xbcf1_u8 D_800D3014;
const xbcf1_u16 xeno_battle_command_file1_defaults_9C10[5] =
    {0x7fff, 0x7fff, 0x10, 8, 0x1f0};
xbcf1_s32 xeno_battle_command_file1_cycle_9C1C;
xbcf1_s32 xeno_battle_command_file1_x_9C30;
xbcf1_s32 xeno_battle_command_file1_y_9C34;

static Trace *trace;
static int native_mode;
static int lookup_rebind;
static uint8_t module_bytes[MODULE_SIZE];
#ifndef AUDIT_YIELDS
#define AUDIT_YIELDS 1
#endif
static int audit_yields_left;
static PcPortMipsCpu *audit_cpu;
static uint32_t audit_seen[315], audit_branches[315];
static unsigned audit_max_events;
static void audit_fetch(uint32_t address, uint32_t instruction)
{
 if(!audit_cpu || address!=audit_cpu->pc || address<ENTRY || address>=ENTRY+0x4ec) return;
 unsigned i=(address-ENTRY)/4, op=instruction>>26;
 unsigned rs=(instruction>>21)&31, rt=(instruction>>16)&31;
 uint32_t a=audit_cpu->gpr[rs], b=audit_cpu->gpr[rt];
 int taken=-1;
 audit_seen[i]++;
 if(op==4) taken=a==b;
 if(op==5) taken=a!=b;
 if(op==6) taken=(int32_t)a<=0;
 if(op==7) taken=(int32_t)a>0;
 if(op==1 && rt==0) taken=(int32_t)a<0;
 if(op==1 && rt==1) taken=(int32_t)a>=0;
 if(taken>=0) audit_branches[i]|=1u<<taken;
}

static uint8_t *guest(uint32_t a) { return g_PsxRam + (a & 0x1fffffu); }
static uint16_t r16(const uint8_t *p) { uint16_t v; memcpy(&v,p,2); return v; }
static uint32_t r32(const uint8_t *p) { uint32_t v; memcpy(&v,p,4); return v; }
static void w16(uint8_t *p,uint16_t v) { memcpy(p,&v,2); }
static void w32(uint8_t *p,uint32_t v) { memcpy(p,&v,4); }
static uint8_t *ui(void) { return native_mode ? D_800D2D28 : guest(r32(guest(0x800d2d28))); }
static uint8_t *win(void) { return native_mode ? D_800D2DAC : guest(r32(guest(0x800d2dac))); }
static uint32_t ptr_id(const void *p)
{
    if(p==host_control) return CONTROL_ADDR;
    if(p==host_ui) return UI_ADDR;
    if(p==host_window) return WINDOW_ADDR;
    if(p==host_alt_window) return ALT_WINDOW_ADDR;
    return (uint32_t)(uintptr_t)p;
}

static void ev(uint32_t id, const uint32_t *a, unsigned n)
{
    Event *e;
    if(!trace || trace->n >= sizeof(trace->e)/sizeof(trace->e[0])) {
        if(trace) trace->overflow=1;
        return;
    }
    e = &trace->e[trace->n++];
    memset(e,0,sizeof(*e)); e->id=id;
    if (n) memcpy(e->a,a,n*4);
}

void func_8008F8F4(xbcf1_s32 a0,xbcf1_u16 a1,xbcf1_u16 a2,
                    xbcf1_u16 a3,xbcf1_u16 a4,xbcf1_u32 a5,xbcf1_u32 a6)
{ uint32_t a[]={a0,a1,a2,a3,a4,a5,a6}; ev(E_SETUP,a,7); }
void func_800716D8(void) { ev(E_YIELD,0,0); if(--audit_yields_left<=0) ui()[0xbf]=1; }
void func_8008FA60(xbcf1_s32 a0) { uint32_t a[]={a0}; ev(E_CLEANUP,a,1); }
void xeno_battle_command_file1_helper_6750(xbcf1_u8 a0,xbcf1_u16 a1,
 xbcf1_u16 a2,xbcf1_u16 a3,xbcf1_u16 a4)
{ uint32_t a[]={a0,a1,a2,a3,a4}; ev(E_LOCAL6750,a,5); }
void xeno_battle_command_file1_helper_5B00(xbcf1_s32 a0,xbcf1_s32 a1)
{ uint32_t a[]={(uint32_t)a0,(uint32_t)a1}; ev(E_LOCAL5B00,a,2); }
void func_80032F54(void *p,xbcf1_s32 a1,xbcf1_s32 a2,xbcf1_s32 a3,
 xbcf1_s32 a4,xbcf1_s32 a5,xbcf1_s32 a6)
{ uint32_t a[]={ptr_id(p),a1,a2,a3,a4,a5,a6}; ev(E_CTOR,a,7); w16(win()+0x10,4); }
void func_80034614(void *p) { uint32_t a[]={ptr_id(p)}; ev(E_RESET,a,1); }
void *GetStringEntry(void *p,xbcf1_s32 i)
{
 uint32_t a[]={ptr_id(p),(uint32_t)i}; ev(E_GET_STRING,a,2);
 if(lookup_rebind) D_800D2DAC=host_alt_window;
 return (void *)(uintptr_t)STRING_ADDR;
}
void func_80034714(void *p,void *s)
{ uint32_t a[]={ptr_id(p),ptr_id(s)}; ev(E_QUEUE,a,2); }
void func_800345E0(void *p)
{ uint32_t a[]={ptr_id(p)}; ev(E_CLEAR_WAIT,a,1); w16(win()+0x10,r16(win()+0x10)&(uint16_t)~8); }
void func_800346D4(void *p) { uint32_t a[]={ptr_id(p)}; ev(E_DESTROY,a,1); }

static int bus_read(void *o,uint32_t a,unsigned width,uint32_t *v)
{
    (void)o;
    if (a<0x80000000u || (uint64_t)a+width>0x80200000u) return -1;
    *v=0; for(unsigned i=0;i<width;i++) *v|=(uint32_t)guest(a)[i]<<(i*8);
    if(width==4) audit_fetch(a,*v);
    return 0;
}
static int bus_write(void *o,uint32_t a,unsigned width,uint32_t v)
{
    (void)o;
    if (a<0x80000000u || (uint64_t)a+width>0x80200000u) return -1;
    for(unsigned i=0;i<width;i++) guest(a)[i]=(uint8_t)(v>>(i*8));
    return 0;
}
static uint32_t stack_arg(PcPortMipsCpu *c,unsigned i)
{ return r32(guest(c->gpr[29]+0x10+4*(i-4))); }
static int bridge(void *o,PcPortMipsCpu *c,uint32_t t)
{
    uint32_t a[8]={0}; unsigned n=0; uint32_t id;
    (void)o;
    switch(t) {
    case 0x8008f8f4: id=E_SETUP;n=7;break;
    case 0x800716d8: id=E_YIELD;break;
    case 0x801e6750: id=E_LOCAL6750;n=5;break;
    case 0x80032f54: id=E_CTOR;n=7;break;
    case 0x80034614: id=E_RESET;n=1;break;
    case 0x80033728: id=E_GET_STRING;n=2;break;
    case 0x80034714: id=E_QUEUE;n=2;break;
    case 0x801e5b00: id=E_LOCAL5B00;n=2;break;
    case 0x800345e0: id=E_CLEAR_WAIT;n=1;break;
    case 0x800346d4: id=E_DESTROY;n=1;break;
    case 0x8008fa60: id=E_CLEANUP;n=1;break;
    default:return 0;
    }
    if(t==0x8008f8f4) {
        for(unsigned i=0;i<4;i++) a[i]=c->gpr[4+i];
        for(unsigned i=4;i<7;i++) a[i]=stack_arg(c,i);
    } else if(t==0x801e6750) {
        for(unsigned i=0;i<4;i++) a[i]=c->gpr[4+i];
        a[4]=stack_arg(c,4);
    } else if(t==0x80032f54) {
        a[0]=c->gpr[4];
        for(unsigned i=1;i<4;i++) a[i]=c->gpr[4+i];
        for(unsigned i=4;i<7;i++) a[i]=stack_arg(c,i);
        w16(win()+0x10,4);
    } else if(t==0x80033728) {
        a[0]=c->gpr[4]; a[1]=c->gpr[5]; c->gpr[2]=STRING_ADDR;
        if(lookup_rebind) w32(guest(0x800d2dac),ALT_WINDOW_ADDR);
    } else if(t==0x80034714) { a[0]=c->gpr[4]; a[1]=c->gpr[5]; }
    else if(t==0x801e5b00) { a[0]=c->gpr[4];a[1]=c->gpr[5]; }
    else if(t==0x8008fa60) a[0]=c->gpr[4];
    else if(t==0x80034614 || t==0x800345e0 || t==0x800346d4) a[0]=c->gpr[4];
    if(t==0x800716d8) if(--audit_yields_left<=0) ui()[0xbf]=1;
    if(t==0x800345e0) w16(win()+0x10,r16(win()+0x10)&(uint16_t)~8);
    ev(id,a,n); return 1;
}

static void init_common(const Case *c,int native)
{
    uint8_t *ctl,*u,*w;
    native_mode=native; audit_yields_left=AUDIT_YIELDS;
    lookup_rebind=c->rebind_on_lookup;
    if(native) {
        memset(host_control,0xa5,sizeof(host_control));memset(host_ui,0xa5,sizeof(host_ui));
        memset(host_window,0xa5,sizeof(host_window));memset(host_alt_window,0xa5,sizeof(host_alt_window));
        D_800D3278=host_control;D_800D2D28=host_ui;D_800D2DAC=host_window;
        ctl=host_control;u=host_ui;w=host_window;
        D_800D3014=c->ui_mode; xeno_battle_command_file1_cycle_9C1C=0x12345678;
        xeno_battle_command_file1_x_9C30=c->saved_x;
        xeno_battle_command_file1_y_9C34=c->saved_y;
    } else {
        memset(g_PsxRam,0xa5,sizeof(g_PsxRam));memcpy(guest(MODULE_ADDR),module_bytes,MODULE_SIZE);
        w32(guest(0x800d3278),CONTROL_ADDR);w32(guest(0x800d2d28),UI_ADDR);
        w32(guest(0x800d2dac),WINDOW_ADDR);w32(guest(0x800d3340),TABLE_ADDR);
        guest(0x800d3014)[0]=c->ui_mode;ctl=guest(CONTROL_ADDR);u=guest(UI_ADDR);w=guest(WINDOW_ADDR);
        w32(guest(0x801e9c1c),0x12345678);
        w32(guest(0x801e9c30),(uint32_t)c->saved_x);
        w32(guest(0x801e9c34),(uint32_t)c->saved_y);
    }
    w16(ctl+0x7f6,c->x);w16(ctl+0x7f8,c->y);w16(ctl+0x7fa,c->units);
    w16(ctl+0x7fc,c->rows);w16(ctl+0x7fe,c->fallback);ctl[0x802]=c->phase;
    u[0xbf]=c->ui_bf;u[0xc8]=0x31;u[0xc9]=0x32;u[0xcf]=0x33;u[0x9e]=0x34;
    w16(w+0x10,c->window_flags);w16(w+0,(uint16_t)c->cursor_x);w16(w+2,(uint16_t)c->cursor_y);
    w=native?host_alt_window:guest(ALT_WINDOW_ADDR);
    w16(w+0x10,c->window_flags);w16(w+0,(uint16_t)c->cursor_x);w16(w+2,(uint16_t)c->cursor_y);
}

static int compare_state(const Case *c,Trace *r,Trace *n,int rr,int nr)
{
    int tr=memcmp(r,n,sizeof(*r));
    int ctl=memcmp(guest(CONTROL_ADDR),host_control,sizeof(host_control));
    int us=memcmp(guest(UI_ADDR),host_ui,sizeof(host_ui));
    int ws=memcmp(guest(WINDOW_ADDR),host_window,sizeof(host_window));
    int aws=memcmp(guest(ALT_WINDOW_ADDR),host_alt_window,sizeof(host_alt_window));
    int gp=r32(guest(0x800d2dac))!=(uint32_t)ptr_id(D_800D2DAC);
    int bad=rr!=nr || tr || ctl || us || ws || aws || gp || r->overflow || n->overflow;
    if(r->n>audit_max_events) audit_max_events=r->n;
    if(n->n>audit_max_events) audit_max_events=n->n;
    bad |= (int32_t)r32(guest(0x801e9c1c))!=xeno_battle_command_file1_cycle_9C1C;
    bad |= (int32_t)r32(guest(0x801e9c30))!=xeno_battle_command_file1_x_9C30;
    bad |= (int32_t)r32(guest(0x801e9c34))!=xeno_battle_command_file1_y_9C34;
    if(bad) fprintf(stderr,"DIFF FAIL %s ret=%d/%d events=%u/%u flags=%04x/%04x\n",
      c->name,rr,nr,r->n,n->n,r16(guest(WINDOW_ADDR)+0x10),r16(host_window+0x10));
    if(bad) {
      fprintf(stderr," parts trace=%d control=%d ui=%d window=%d alt=%d globalptr=%d globals=%08x/%08x %08x/%08x %08x/%08x\n",
       tr,ctl,us,ws,aws,gp,r32(guest(0x801e9c1c)),(uint32_t)xeno_battle_command_file1_cycle_9C1C,
       r32(guest(0x801e9c30)),(uint32_t)xeno_battle_command_file1_x_9C30,
       r32(guest(0x801e9c34)),(uint32_t)xeno_battle_command_file1_y_9C34);
      for(unsigned i=0;i<r->n && i<n->n;i++) if(memcmp(&r->e[i],&n->e[i],sizeof(Event))) {
        fprintf(stderr," event%u id=%u/%u\n",i,r->e[i].id,n->e[i].id);
        for(unsigned j=0;j<8;j++) fprintf(stderr,"  a%u=%08x/%08x\n",j,r->e[i].a[j],n->e[i].a[j]);
      }
    }
    return !bad;
}

static int one(const Case *c)
{
    Trace rt={0},nt={0}; PcPortMipsCpu cpu; PcPortMipsBus bus={0,bus_read,bus_write,bridge,0,0,0};
    int rr,nr;
    init_common(c,0);trace=&rt;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[4]=c->arg0;cpu.gpr[5]=c->arg1;
    cpu.gpr[6]=c->arg2;cpu.gpr[29]=STACK;cpu.gpr[31]=HALT;
    audit_cpu=&cpu; rr=PcPortMipsRun(&cpu,ENTRY,HALT,4096); audit_cpu=NULL;if(rr!=PC_PORT_MIPS_HALTED){fprintf(stderr,"MIPS %s %s\n",c->name,cpu.error);return 0;}
    rr=(int)cpu.gpr[2];
    init_common(c,1);trace=&nt;nr=xeno_battle_command_file1_controller(c->arg0,c->arg1,c->arg2);
    native_mode=0;return compare_state(c,&rt,&nt,rr,nr);
}

int main(int argc, char **argv)
{
    FILE *f;
    if (argc != 2) {
        fprintf(stderr, "usage: %s ARCHIVE20_FILE1\n", argv[0]);
        return 2;
    }
    f=fopen(argv[1],"rb");
    unsigned count=0;
    if(!f||fread(module_bytes,1,sizeof(module_bytes),f)!=sizeof(module_bytes)||fclose(f)) return 2;
    Case tests[]={
      {.arg0=0x1234,.arg1=0x22,.arg2=0,.x=30,.y=24,.units=8,.rows=4,
       .fallback=0x10,.phase=0,.ui_bf=1,.ui_mode=0,.window_flags=4,
       .cursor_x=2,.cursor_y=3,.rebind_on_lookup=1,.name="base-lookup-rebind"},
      {.arg0=0x4321,.arg1=0xff,.arg2=2,.x=0x7fff,.y=0x7fff,.units=5,.rows=7,
       .fallback=0x10,.phase=0,.ui_bf=0,.ui_mode=0,.window_flags=4,
       .cursor_x=-2,.cursor_y=3,.name="sentinel-alt"},
      {.arg0=1,.arg1=3,.arg2=4,.x=0x7fff,.y=0x7fff,.units=1,.rows=2,
       .fallback=0x10,.phase=0,.ui_bf=1,.ui_mode=0,.window_flags=4,
       .cursor_x=0,.cursor_y=0,.name="bit4-y"},
      {.arg0=2,.arg1=4,.arg2=8,.x=12,.y=13,.units=9,.rows=6,
       .fallback=0x10,.phase=0,.ui_bf=0,.ui_mode=0,.window_flags=4,
       .cursor_x=0,.cursor_y=0,.name="bit8"},
      {.arg0=3,.arg1=5,.arg2=0,.x=20,.y=21,.units=7,.rows=4,
       .fallback=0x13,.phase=0,.ui_bf=1,.ui_mode=0,.window_flags=4,
       .cursor_x=0,.cursor_y=0,.name="fallback"},
      {.arg0=4,.arg1=6,.arg2=0x13,.x=20,.y=21,.units=7,.rows=4,
       .fallback=0x10,.phase=0,.ui_bf=1,.ui_mode=0,.window_flags=4,
       .cursor_x=0,.cursor_y=0,.name="bits1-2-10"},
      {.arg0=5,.arg1=7,.arg2=0,.x=20,.y=21,.units=7,.rows=4,
       .fallback=0x10,.phase=1,.ui_bf=1,.ui_mode=0,.window_flags=4,
       .cursor_x=0,.cursor_y=0,.name="phase-active"},
      {.arg0=6,.arg1=8,.arg2=0,.x=20,.y=21,.units=7,.rows=4,
       .fallback=0x10,.phase=1,.ui_bf=1,.ui_mode=0,.window_flags=12,
       .cursor_x=-32768,.cursor_y=32767,
       .saved_x=0x7fffffff,.saved_y=(int32_t)0x80000000u,
       .name="phase-wait-active-wrap"},
      {.arg0=7,.arg1=9,.arg2=0,.x=20,.y=21,.units=7,.rows=4,
       .fallback=0x10,.phase=1,.ui_bf=1,.ui_mode=4,.window_flags=8,
       .cursor_x=-3,.cursor_y=4,.name="phase-wait-close"},
      {.arg0=8,.arg1=10,.arg2=8,.x=20,.y=21,.units=7,.rows=4,
       .fallback=0x10,.phase=1,.ui_bf=1,.ui_mode=0,.window_flags=0,
       .cursor_x=-3,.cursor_y=4,.name="phase-close-bit8"},
    };
    for(unsigned i=0;i<sizeof(tests)/sizeof(tests[0]);i++){count++;if(!one(&tests[i]))return 1;}
    Case audit_extra[]={
      {.arg0=0xffff,.arg1=0xff,.arg2=2,.x=0x7fff,.y=0xffff,.units=0x1553,.rows=3,.fallback=0,.phase=0,.ui_bf=0,.window_flags=4,.name="audit-centered-negative-ymax"},
      {.arg0=0xffff,.arg1=0,.arg2=1,.x=0x7fff,.y=0xffff,.units=0x1554,.rows=1,.fallback=0,.phase=0,.ui_bf=0,.window_flags=4,.name="audit-width-wrap-alternate-no-xshift"},
      {.arg0=0,.arg1=0,.arg2=0,.x=0x7fff,.y=0xffff,.units=0x1553,.rows=3,.fallback=0,.phase=0,.ui_bf=0,.window_flags=4,.name="audit-width-wrap-alternate-xshift"},
      {.arg0=0,.arg1=0,.arg2=0,.x=0xffff,.y=0xffff,.units=0xffff,.rows=0xffff,.fallback=0,.phase=0,.ui_bf=0,.window_flags=4,.name="audit-fallback-zero-highfields"},
      {.arg0=0,.arg1=0,.arg2=0x10,.x=0,.y=0,.units=0,.rows=0,.fallback=0,.phase=255,.ui_bf=0,.ui_mode=4,.window_flags=0xffff,.cursor_x=-32768,.cursor_y=32767,.saved_x=0x7fffffff,.saved_y=(int32_t)0x80000000u,.name="audit-phase255-all-window-flags"},
    };
    for(unsigned i=0;i<sizeof(audit_extra)/sizeof(audit_extra[0]);i++){count++;if(!one(&audit_extra[i]))return 1;}

    {
      static const uint16_t flagv[]={0,1,2,3,4,8,0x10,0x13,0xffff};
      static const uint16_t xv[]={30,0x7fff,0xffff};
      static const uint16_t yv[]={24,0x7fff};
      static const uint16_t rowv[]={0,4,5,0xffff};
      static const uint8_t a1v[]={0x22,0xff};
      for(unsigned a=0;a<sizeof(flagv)/sizeof(flagv[0]);a++)
       for(unsigned b=0;b<sizeof(xv)/sizeof(xv[0]);b++)
        for(unsigned d=0;d<sizeof(yv)/sizeof(yv[0]);d++)
         for(unsigned e=0;e<sizeof(rowv)/sizeof(rowv[0]);e++)
          for(unsigned g=0;g<sizeof(a1v)/sizeof(a1v[0]);g++) {
           Case t={.arg0=0xbeef,.arg1=a1v[g],.arg2=flagv[a],.x=xv[b],.y=yv[d],
            .units=0xffff,.rows=rowv[e],.fallback=0x13,.phase=0,.ui_bf=(uint8_t)(a&1),
            .ui_mode=0,.window_flags=4,.cursor_x=-32768,.cursor_y=32767,.name="create-cross"};
           count++;if(!one(&t))return 1;
          }
      for(unsigned a=0;a<2;a++) for(unsigned f=0;f<4;f++) for(unsigned m=0;m<2;m++) {
        Case t={.arg0=0x55aa,.arg1=0x44,.arg2=(uint16_t)(a?8:0),.x=10,.y=11,
         .units=3,.rows=4,.fallback=0x10,.phase=1,.ui_bf=1,.ui_mode=(uint8_t)(m?4:0),
         .window_flags=(uint16_t[]){0,4,8,12}[f],.cursor_x=-32768,.cursor_y=32767,
         .name="update-cross"};
        count++;if(!one(&t))return 1;
      }
    }
    printf("candidate differential GREEN cases=%u\n",count);
    printf("AUDIT max_events=%u yields=%d\n",audit_max_events,AUDIT_YIELDS);
    for(unsigned i=0;i<315;i++) printf("PC %08x %u %u\n",ENTRY+4*i,audit_seen[i],audit_branches[i]);
    return 0;
}
