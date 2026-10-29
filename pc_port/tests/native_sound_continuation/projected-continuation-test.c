/* Retail sound continuation compared with test-only translated bodies.
 * Event calls are synthetic and PsyCross GTE is shared, not a hardware oracle. */
#include "battle-runtime-projection.c"
#include "psx/gtereg.h"
/* Unused runtime globals required by the linked fixture. */
void *g_Heap;
unsigned int g_HeapIsErrorHandlerOff;
uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
int32_t D_80059478;
unsigned g_PcPortPresentedFrames;
static unsigned pump_calls,mutate_at,hook_mode,wrapper_calls;static uint32_t mutate_address;
void PcPort_BattleStateHashBeforeGuest(PcPortMipsCpu *cpu,uint32_t target){if(hook_mode&&target==0x80076a10u&&cpu->gpr[31]==0x8008d7b0u&&++wrapper_calls==2)cpu->gpr[16]=0x7fff0000u;}
extern void PcPort_ServiceVblank(void);
extern void PcPort_ResetVblankService(void);
extern void func_8004B7D0(void (*)(void));
static unsigned fake_vblank;

/* compat.o's real pump is the sole definition; with a frozen vblank count it
 * early-returns before touching pad state (a conflicting no-op stub here
 * breaks the link with multiple-definition). */
int PsyX_Sys_GetVBlankCount(void) { return (int)fake_vblank; }
int PcPort_QuickCheckpointFieldIsActive(void) { return 0; }
void PsyX_UpdateInput(void) {}
void ControllerPoll(void) {}
void ControllerPushState(void) {}
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }
extern int func_80025FA8(uint8_t *, int, uint8_t *, int, int, int, int, int, int)
    __attribute__((weak));
extern int ReadGeomScreen(void);
extern void ReadGeomOffset(long *, long *);
extern MATRIX *ScaleMatrixL(MATRIX *, VECTOR *);

static int sdk_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (target >= 0x80025fa8u && target < 0x80026338u) return 0;
    if (target >= 0x80043a1cu && target < 0x80043d00u) return 0;
    return runtime_bridge(opaque, cpu, target);
}

static void initial_gte(void)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    CTC2(4096, 0); CTC2(4096, 2); CTC2(4096, 4);
    CTC2(123, 5); CTC2(456, 6); CTC2(789, 7);
    SetGeomOffset(160, 112); SetGeomScreen(512);
}


static int glyph_bridge(void*p,PcPortMipsCpu*c,uint32_t t){(void)p;(void)c;(void)t;return 0;}
static uint8_t initial[PSX_RAM_SIZE],expected[PSX_RAM_SIZE];
#include "release-body.inc"
#include "assign-body.inc"
#include "instrument-body.inc"
static uint32_t cpu_calls[8],host_calls[8];
static unsigned cpu_call_count,host_call_count;
static int synthetic_mutation;
static BattleMipsRuntime *callback_runtime;
static uint32_t callback_target;
static unsigned callback_count;
static void fixture_vblank(void) {
    callback_count++;
    if(synthetic_mutation && callback_target==0x8003e724u)
        assert(runtime_write(callback_runtime,0x80059554u,4,0x12000000u)==0);
    if(synthetic_mutation && callback_target==0x800404a4u)
        assert(runtime_write(callback_runtime,0x800594fcu,4,0x34000000u)==0);
}
static void synthetic_callback(BattleMipsRuntime *runtime,uint32_t target) {
    callback_runtime=runtime; callback_target=target; fake_vblank++;
    PcPort_ServiceVblank();
}
static void reset_vblank(void) {
    fake_vblank=0; callback_count=0; PcPort_ResetVblankService();
    func_8004B7D0(fixture_vblank);
}
static void scratch_sound_call(BattleEntryCall *e,uint32_t target) {
    assert(host_call_count < 8); host_calls[host_call_count++]=target;
    synthetic_callback(e->runtime,target);
    switch(target) {
    case 0x8003e5bcu: scratch_sound_instrument(e); break;
    case 0x8003e724u: scratch_sound_assign(e); break;
    case 0x8003e83cu: scratch_sound_release(e); break;
    case 0x800404a4u: e->cpu->gpr[2]=1; e->cpu->gpr[9]=12; e->cpu->gpr[10]=0xb0u; break;
    default: abort();
    }
}
#include "continuation-body.inc"
static int sound_event_oracle(void *opaque,PcPortMipsCpu *cpu,uint32_t target){
 if((target==0x8003e5bcu && cpu->gpr[31]==0x8003b884u) ||
    (target==0x8003e724u && cpu->gpr[31]==0x8003b89cu) ||
    (target==0x8003e83cu && cpu->gpr[31]==0x8003b8ccu) ||
    (target==0x800404a4u && cpu->gpr[31]==0x8003b8fcu)) {
     assert(cpu_call_count<8); cpu_calls[cpu_call_count++]=target;
     synthetic_callback(opaque,target);
 }
 if(target!=0x800404a4u||cpu->gpr[31]!=0x8003b8fcu)return 0;
 cpu->gpr[2]=1;cpu->gpr[9]=12;cpu->gpr[10]=0xb0u;return 1;
}
static void reject_invalid_sound_count(uint32_t bad) {
 BattleMipsRuntime rt={0}; PcPortMipsCpu cpu={0}; uint16_t flags=0x800u;
 D_80059478=(int32_t)bad; initialize_sound_count_data(&rt);
 rt.data[rt.data_count++]=(ResolvedData){0x8005957cu,2u,(uint8_t*)&flags,"sound_flags"};
 qsort(rt.data,rt.data_count,sizeof(rt.data[0]),data_compare);
 cpu.gpr[29]=0x801ff000u; cpu.gpr[31]=0x8008aa64u;
 BattleEntryCall entry={.runtime=&rt,.cpu=&cpu};
 if(setjmp(entry.failure)==0) {
  battle_sound_enabled_run(&entry);
  assert(!"invalid sound element count was accepted");
 }
}
static void test_sound_element_span(void) {
 BattleMipsRuntime rt={0};
 const uint32_t manager=0x80140000u, stack=0x801ff000u;
 store_le(PSX_ADDR(manager+0x14u),1,16);
 store_le(PSX_ADDR(0x80059404u),4,2);
 assert(battle_sound_elements_preflight(&rt,manager,14,stack));
 assert(!battle_sound_elements_preflight(&rt,manager,15,stack));
 assert(!battle_sound_elements_preflight(&rt,manager,254,stack));
 assert(!battle_sound_elements_preflight(&rt,manager+1u,14,stack));
 uint32_t pair=manager+0x94u+14u*0x158u;
 assert(!battle_sound_elements_preflight(&rt,manager,14,pair+80u));
 store_le(PSX_ADDR(0x80059404u),4,0);
 assert(!battle_sound_elements_preflight(&rt,manager,14,stack));
 store_le(PSX_ADDR(0x80059404u),4,2);
 store_le(PSX_ADDR(0x801fff14u),1,16);
 assert(!battle_sound_elements_preflight(&rt,0x801fff00u,0,stack));
}
int main(void){
 BattleMipsRuntime binding={0}; uint32_t count=0;
 D_80059478=16; store_le(PSX_ADDR(0x80059478u),4,0);
 initialize_sound_count_data(&binding);
 qsort(binding.data,binding.data_count,sizeof(binding.data[0]),data_compare);
 assert(binding.data_count==1 && binding.data[0].address==0x80059478u && binding.data[0].size==4);
 assert(battle_sound_count_bound(&binding));
 uint8_t *real_count=binding.data[0].host;
 binding.data[0].host=PSX_ADDR(0x80059478u);
 assert(!battle_sound_count_bound(&binding));
 binding.data[0].host=real_count;
 assert(!runtime_read(&binding,0x80059478u,4,&count) && count==16);
 D_80059478=10;
 assert(!runtime_read(&binding,0xa0059478u,4,&count) && count==10);
 assert(!runtime_write(&binding,0x80059478u,4,14) && D_80059478==14);
 assert(load_le(PSX_ADDR(0x80059478u),4)==0);
 const uint32_t bad_counts[]={0u,1u,3u,25u,26u,0xffffffffu};
 for(unsigned i=0;i<sizeof(bad_counts)/sizeof(bad_counts[0]);i++)
  reject_invalid_sound_count(bad_counts[i]);
 test_sound_element_span();
 FILE*f=fopen("disc/SLUS_006.64","rb");assert(f);assert(!fseek(f,0x800+0x8003b644-0x80010000,SEEK_SET));assert(fread(PSX_ADDR(0x8003b644u),1,0x2ecu,f)==0x2ecu);assert(!fseek(f,0x800+0x8003e5bc-0x80010000,SEEK_SET));assert(fread(PSX_ADDR(0x8003e5bcu),1,0x300u,f)==0x300u);fclose(f);void *slots[24],*expected_slots[24];uint32_t dirty,key;BattleMipsRuntime runtime={0};runtime.initialized=1;runtime.data_count=3;runtime.data[0]=(ResolvedData){0x800594fcu,4,(uint8_t*)&key,"key"};runtime.data[1]=(ResolvedData){0x80059554u,4,(uint8_t*)&dirty,"dirty"};runtime.data[2]=(ResolvedData){0x8006252cu,192u,(uint8_t*)slots,"g_SoundChannels"};runtime.host_range_count=1;runtime.host_ranges[0]=(HostRange){(uintptr_t)g_PsxRam,(uintptr_t)g_PsxRam+PSX_RAM_SIZE,1};assert(!setenv("XENO_SCRATCH_SOUND_POINTER_PROJECTION","1",1));
for(unsigned k=0;k<6;k++){cpu_call_count=host_call_count=0;synthetic_mutation=(k==5);uint32_t el=0x80150000u,sed=0x80120000u,manager=0x80140000u,bank=0x80160000u,sp=0x801ff000u-80u,count=(k==0||k==2)?1:2;memset(PSX_ADDR(el),0x55,0x300);memset(PSX_ADDR(sed),0,0x200);memset(PSX_ADDR(manager),0,0x80);memset(PSX_ADDR(bank),0,0x80);store_le(PSX_ADDR(0x80059504u),4,0x12345678u);dirty=0x01020304u;key=0xf0f0f0f0u;memset(slots,0,sizeof slots);store_le(PSX_ADDR(0x800595bcu),4,0x42u);store_le(PSX_ADDR(manager+0x48u),4,0xabcdu);store_le(PSX_ADDR(manager+0x10u),2,0x1234u);store_le(PSX_ADDR(sed+0x10u),2,k&1);store_le(PSX_ADDR(sed+0x16u),1,7);store_le(PSX_ADDR(sed+0x100u),2,k>=2?0x180u:0);store_le(PSX_ADDR(sed+0x102u),2,k>=3&&k!=4?0x184u:0);store_le(PSX_ADDR(bank+0x28u),4,0x11110000u);store_le(PSX_ADDR(bank+0x30u),4,0x1234u);store_le(PSX_ADDR(bank+0x34u),2,0x40u);store_le(PSX_ADDR(bank+0x36u),2,0x5678u);store_le(PSX_ADDR(bank+0x38u),4,0xa1b2c3d4u);store_le(PSX_ADDR(bank+0x3cu),2,0xe7f8u);store_le(PSX_ADDR(sp+16u),4,0x01040001u);store_le(PSX_ADDR(sp+24u),2,0x4000u);store_le(PSX_ADDR(sp+32u),1,0x80u);const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)store_le(PSX_ADDR(sp+76u-4u*i),4,i==0?0x80039e08u:0x50200000u+i);for(unsigned j=0;j<count;j++){uint32_t e=el+j*0x158u;store_le(PSX_ADDR(e+6u),1,j+1u);store_le(PSX_ADDR(e+0x27u),1,j+3u);store_le(PSX_ADDR(e+0x34u),2,0x100u);slots[j+3u]=(k==1&&j==0)?NULL:PSX_ADDR(e+0x30u);}memcpy(initial,g_PsxRam,sizeof initial);initial_gte();reset_vblank();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=sound_event_oracle;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[16]=(uint32_t)(uintptr_t)PSX_ADDR(el+0x27u);cpu.gpr[17]=(uint32_t)(uintptr_t)PSX_ADDR(el);cpu.gpr[18]=(uint32_t)(uintptr_t)PSX_ADDR(sed);cpu.gpr[19]=(uint32_t)(uintptr_t)PSX_ADDR(el+0x30u);cpu.gpr[20]=(uint32_t)(uintptr_t)PSX_ADDR(manager);cpu.gpr[21]=(uint32_t)(uintptr_t)PSX_ADDR(sed+0x100u);cpu.gpr[22]=count;cpu.gpr[23]=k>=3?(uint32_t)(uintptr_t)PSX_ADDR(bank):0;cpu.gpr[29]=sp;cpu.gpr[30]=0x4321u;cpu.gpr[31]=0x8003b76cu;PcPortMipsCpu host=cpu;assert(PcPortMipsRun(&cpu,0x8003b76cu,0x80039e08u,10000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);memcpy(expected_slots,slots,sizeof slots);uint32_t want_dirty=dirty,want_key=key;unsigned want_callback_count=callback_count;GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);dirty=0x01020304u;key=0xf0f0f0f0u;memset(slots,0,sizeof slots);for(unsigned j=0;j<count;j++)slots[j+3u]=(k==1&&j==0)?NULL:PSX_ADDR(el+j*0x158u+0x30u);initial_gte();reset_vblank();BattleEntryCall e={.runtime=&runtime,.cpu=&host};scratch_sound_continuation(&e);if(callback_count!=want_callback_count || callback_count!=cpu_call_count){fprintf(stderr,"case %u CPU calls=%u callbacks=%u host callbacks=%u fake=%u\n",k,cpu_call_count,want_callback_count,callback_count,fake_vblank);return 1;}assert(cpu_call_count==host_call_count&&!memcmp(cpu_calls,host_calls,cpu_call_count*sizeof(cpu_calls[0])));for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected)&&!memcmp(expected_slots,slots,sizeof slots)&&dirty==want_dirty&&key==want_key);assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));}puts("scratch projected sound continuation PASS six cases, helper order and real vblank service with synthetic event and clock");return 0;}
