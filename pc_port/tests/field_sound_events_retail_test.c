/* Differential boundary test. Only the reference executes retail CPU code.
 * Native side is the verbatim extracted production helper; this is not a
 * production-TU link, audio-rendering test or playable-slice acceptance. */
#include "field_sound_events_retail_test.h"
#include "battle_mips_adapter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_SIZE 0x200000u
#define BASE 0x8006faf0u
#define HEADER 0x80150000u
#define HALT 0x80001000u
#define TABLE_WORDS 32
static u8 ram[RAM_SIZE];
_Alignas(u32) static u8 native_header[32];
s16 D_800C3A38;
u16 D_800AE060[TABLE_WORDS];
s32 D_800C3A64;
u16 D_800C3A2C;
s32 D_800B06A0;
void *D_800B235C;
typedef struct { u32 event, channel, index; } Call;
static Call native_calls[16], retail_calls[16];
static unsigned native_count, retail_count, cases;
static void require(int ok, const char *what) {
    if (!ok) { fprintf(stderr, "FIELD SOUND EVENTS FAIL case=%u %s\n", cases, what); exit(1); }
}
static unsigned offset(u32 addr, unsigned width) {
    unsigned off = addr & 0x1fffffffu;
    require(off <= RAM_SIZE - width, "bus range");
    return off;
}
static u32 get(u32 addr, unsigned width) {
    unsigned off = offset(addr, width); u32 v = 0;
    for (unsigned i=0;i<width;i++) v |= (u32)ram[off+i] << (8*i);
    return v;
}
static void put(u32 addr, unsigned width, u32 v) {
    unsigned off = offset(addr, width);
    for (unsigned i=0;i<width;i++) ram[off+i] = (u8)(v >> (8*i));
}
static int read_bus(void *p,u32 a,unsigned w,u32 *v) { (void)p; *v=get(a,w); return 0; }
static int write_bus(void *p,u32 a,unsigned w,u32 v) {
    (void)p;
    require(a==0x800c3a64u || (a>=0x801fe000u && a<0x80200000u), "unexpected retail write");
    put(a,w,v);return 0;
}
static int bridge(void *p,PcPortMipsCpu *cpu,u32 target) {
    (void)p;
    if (target != 0x80039ec4u) return 0;
    require(retail_count<16,"retail call limit");
    retail_calls[retail_count++] = (Call){cpu->gpr[4],cpu->gpr[5],get(0x800c3a64u,4)};
    return 1;
}
void func_80039EC4(s32 event,s32 channel) {
    require(native_count<16,"native call limit");
    native_calls[native_count++] = (Call){(u32)event,(u32)channel,(u32)D_800C3A64};
}
static void run_case(s16 enabled,u16 bank,u16 start,s32 now,int initial,unsigned channel,unsigned sound) {
    PcPortMipsCpu cpu;
    PcPortMipsBus bus={0};
    u16 table_before[TABLE_WORDS]; u8 header_before[32];
    cases++; native_count=retail_count=0;
    memset(native_calls,0,sizeof(native_calls));memset(retail_calls,0,sizeof(retail_calls));
    memset(native_header,0,sizeof(native_header));
    native_header[0x12]=0x5a; native_header[0x13]=0xa5;
    native_header[0x14]=(u8)bank;native_header[0x15]=(u8)(bank>>8);
    for (unsigned i=0;i<TABLE_WORDS/2;i++) {
        D_800AE060[i*2]=(i<4)?(u16)(i/2):0xffffu;
        D_800AE060[i*2+1]=(u16)((channel<<8)|sound|0xa800u);
    }
    D_800C3A38=enabled;D_800C3A64=initial;D_800C3A2C=start;D_800B06A0=now;
    D_800B235C=native_header;
    memcpy(table_before,D_800AE060,sizeof(table_before));memcpy(header_before,native_header,sizeof(header_before));
    put(0x800c3a38u,2,(u16)enabled);put(0x800c3a64u,4,(u32)initial);
    put(0x800c3a2cu,2,start);put(0x800b06a0u,4,(u32)now);put(0x800b235cu,4,HEADER);
    for (unsigned i=0;i<TABLE_WORDS;i++) put(0x800ae060u+i*2,2,D_800AE060[i]);
    memcpy(ram+offset(HEADER,sizeof(native_header)),native_header,sizeof(native_header));
    bus.read=read_bus;bus.write=write_bus;bus.bridge=bridge;
    PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=0x801ff000u;cpu.gpr[31]=HALT;
    int rc=PcPortMipsRun(&cpu,0x80085678u,HALT,10000);
    if(rc!=PC_PORT_MIPS_HALTED) fprintf(stderr,"oracle rc=%d %s\n",rc,cpu.error);
    require(rc==PC_PORT_MIPS_HALTED,"oracle completed");
    func_80085678();
    require(native_count==retail_count,"call count");
    require(memcmp(native_calls,retail_calls,native_count*sizeof(Call))==0,"event/channel/index trace");
    require((u32)D_800C3A64==get(0x800c3a64u,4),"final event index");
    require(memcmp(table_before,D_800AE060,sizeof(table_before))==0,"native table unchanged");
    require(memcmp(header_before,native_header,sizeof(header_before))==0,"native header unchanged");
    require(D_800C3A38==enabled && D_800C3A2C==start && D_800B06A0==now && D_800B235C==native_header,"other globals unchanged");
}
int main(int argc,char **argv) {
    const u16 banks[]={0,1,0x1234,0x8000,0xffff};
    const s16 states[]={0xff,0,-1,0x1ff};
    const s32 clocks[]={-1,0,1,2,65534,65535,65536};
    require(argc==2,"field image argument");
    FILE *f=fopen(argv[1],"rb");require(f!=NULL,"open pinned field");
    size_t n=fread(ram+(BASE&0x1fffffffu),1,260862,f);require(n==260862 && fgetc(f)==EOF,"field size");fclose(f);
    for(unsigned b=0;b<5;b++) for(unsigned c=0;c<8;c++) for(unsigned state=0;state<4;state++)
      for(unsigned t=0;t<7;t++) for(unsigned initial=0;initial<2;initial++) {
        /* A future sentinel ends every active route; clocks never reach it
         * for start=65535. For start=0 keep the clock below 65535. */
        run_case(states[state],banks[b],65535,clocks[t],initial,c,(b*53+c*17)&0xff);
        if(clocks[t]<65535) run_case(states[state],banks[b],0,clocks[t],initial,c,(b*53+c*17)&0xff);
      }
    printf("FIELD SOUND EVENTS PASS cases=%u (extracted native helper vs pinned retail oracle)\n",cases);
    return 0;
}
