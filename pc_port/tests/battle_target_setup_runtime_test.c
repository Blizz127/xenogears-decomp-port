/* Real runtime resolver/dispatcher; retail BC3F8 is a marker callback fixture,
 * not a substitute claim for the allocator's actual on-free callback. */
#include "../src/battle_mips_runtime.c"
#include "battle_target_setup.h"
#include <assert.h>

uint8_t g_PsxRam[PSX_RAM_SIZE], g_PsxScratchpad[4096];
uint32_t D_8006F99C[4], D_8006F9AC[4];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
/* Runtime bridge polls this hook even in a non-presenting fixture. */
void PcPort_PadVblankPump(void) {}
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p,int s) { (void)p; (void)s; abort(); }
unsigned MFC2(int r) { (void)r; abort(); }
unsigned CFC2(int r) { (void)r; abort(); }
void MTC2(unsigned v,int r) { (void)v; (void)r; abort(); }
void CTC2(unsigned v,int r) { (void)v; (void)r; abort(); }
int doCOP2(int op) { (void)op; abort(); }

static uint32_t host_tasks[2][7];
static uint8_t body[0x114], expected_ram[PSX_RAM_SIZE];
static uint32_t expected_vectors[8];
static unsigned callback_count;
static uint32_t callback_tasks[2];
static int invoke(void *opaque, uint32_t code, uint32_t task) {
    (void)opaque;
    assert(code==0x800BC3F8u);
    assert(callback_count<2 && task==callback_tasks[callback_count]);
    callback_count++;
#ifdef XBT_SETUP_MASK_ARGUMENT
    task &= 0x1FFFFFFFu;
#endif
    return PcPort_BattleMipsDispatchCallback(code,(void *)(uintptr_t)task)==1 ? 0 : -1;
}
#ifdef XBT_SETUP_RAW_RAM
static int raw_write(void *opaque,uint32_t a,unsigned w,uint32_t v) {
    (void)opaque; store_le(PSX_ADDR(a),w,v); return 0;
}
#endif
static void reset(uint32_t first,uint32_t second,unsigned seed) {
    memset(g_PsxRam,0xA5,sizeof(g_PsxRam));
    memcpy(PSX_ADDR(0x800BC2F0),body,sizeof(body));
    memset(D_8006F99C,0x5A,sizeof(D_8006F99C));
    memset(D_8006F9AC,0x5A,sizeof(D_8006F9AC));
    store_le(PSX_ADDR(0x800C3680),4,first);
    store_le(PSX_ADDR(0x800C3684),4,second);
    for (unsigned i=0;i<2;i++) {
        memset(host_tasks[i],0xC3,sizeof(host_tasks[i]));
        host_tasks[i][3]=0x800BC3F8;
        store_le(PSX_ADDR(0x8010000C+i*0x20),4,0x800BC3F8);
    }
    for (unsigned i=0;i<6;i++)
        store_le(PSX_ADDR(0x800D30A0+(i/3)*8+(i%3)*2),2,(seed+i*0x3333)&0xFFFF);
    callback_count=0;
}
static void run(uint32_t mode,unsigned domain,unsigned roots,unsigned seed) {
    uint32_t handles[2];
    handles[0]=domain==0 ? 0x80100000 : domain==1 ? 0xA0100000 : (uint32_t)(uintptr_t)host_tasks[0];
    handles[1]=domain==0 ? 0x80100020 : (domain==1 || domain==3) ? 0xA0100020 : (uint32_t)(uintptr_t)host_tasks[1];
    uint32_t first=(roots&1)?handles[0]:0, second=(roots&2)?handles[1]:0;
    callback_tasks[0]=first?first:second;
    callback_tasks[1]=second;
    reset(first,second,seed);
    PcPortMipsCpu cpu;
    initialize_cpu(&cpu,&g_BattleRuntime);
    cpu.gpr[29]=0x801FF000; cpu.gpr[4]=mode;
    assert(PcPortMipsRun(&cpu,0x800BC2F0,BATTLE_HALT_PC,200)==0);
    memcpy(expected_ram,g_PsxRam,sizeof(expected_ram));
    memcpy(expected_vectors,D_8006F99C,16);
    memcpy(expected_vectors+4,D_8006F9AC,16);
    reset(first,second,seed);
    PcPortMipsCpu parent;
    initialize_cpu(&parent,&g_BattleRuntime);
    parent.gpr[29]=0x801FF000;
    PcPortMipsCpu saved=parent;
    g_BattleRuntime.bridge_cpu=&parent;
    PcPortMipsBus memory=parent.bus;
#ifdef XBT_SETUP_RAW_RAM
    memory.write=raw_write;
#endif
    assert(PcPortBattleTargetSetup(&memory,mode,invoke,NULL)==0);
    assert(g_BattleRuntime.bridge_cpu==&parent && !memcmp(&parent,&saved,sizeof(parent)));
    g_BattleRuntime.bridge_cpu=NULL;
    assert(callback_count==((mode==2 || mode==4)?0u:(unsigned)(!!first+!!second)));
    uint32_t marker=(mode==2 || mode==4 || !roots)?0xA5A5A5A5u:second?second:first;
    assert(load_le(PSX_ADDR(0x800C367C),4)==marker);
    /* Only the two retail save words differ; native must leave the entire
     * frame untouched. The leaf marker does not verify nested SP inheritance. */
    for (unsigned i=0;i<0x18;i++) assert(g_PsxRam[0x1FEFE8+i]==0xA5);
    memcpy(expected_ram+0x1FEFF8,g_PsxRam+0x1FEFF8,8);
    assert(!memcmp(expected_ram,g_PsxRam,sizeof(expected_ram)));
    assert(!memcmp(expected_vectors,D_8006F99C,16));
    assert(!memcmp(expected_vectors+4,D_8006F9AC,16));
    for (unsigned i=0;i<6;i++) {
        uint32_t value=mode==2?((seed+i*0x3333)&0xFFFFu)<<16:0x5A5A5A5Au;
        assert((i<3?D_8006F99C[i]:D_8006F9AC[i-3])==value);
    }
    assert(D_8006F99C[3]==0x5A5A5A5Au && D_8006F9AC[3]==0x5A5A5A5Au);
    for (unsigned i=0;i<32;i++) assert(g_PsxRam[0x6F99C+i]==0xA5);
    for (unsigned i=0;i<2;i++) for (unsigned j=0;j<7;j++)
        assert(host_tasks[i][j]==(j==3?0x800BC3F8u:0xC3C3C3C3u));
}
int main(void) {
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x4C800,SEEK_SET));
    assert(fread(body,1,sizeof(body),f)==sizeof(body)); fclose(f);
    assert((uintptr_t)host_tasks+sizeof(host_tasks)<=UINT32_MAX);
    initialize_runtime(&g_BattleRuntime);
    g_ActiveBattleRuntime=&g_BattleRuntime;
    assert(resolve_memory(&g_BattleRuntime,0x8006F99C,16,1)==(uint8_t*)D_8006F99C);
    assert(resolve_memory(&g_BattleRuntime,0xA006F9AC,16,1)==(uint8_t*)D_8006F9AC);
    assert(resolve_memory(&g_BattleRuntime,(uint32_t)(uintptr_t)host_tasks,4,1)==(uint8_t*)host_tasks);
    const uint32_t modes[]={0,1,2,4,0xFFFFFFFF,0x80000002,0x10004};
    const unsigned seeds[]={0,0x7FFF,0x8000,0xFFFF};
    unsigned cases=0;
    for (unsigned m=0;m<7;m++) for (unsigned d=0;d<4;d++)
    for (unsigned r=0;r<4;r++) for (unsigned s=0;s<4;s++) {
        run(modes[m],d,r,seeds[s]); cases++;
    }
    assert(cases==448);
    puts("TARGET SETUP runtime composition PASS 448 cases; real resolver/dispatcher, retail marker callback");
}
