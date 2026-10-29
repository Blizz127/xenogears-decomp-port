/* Test-only sound continuation with a real SDL event bracket and synthetic
 * sound data, vblank clock and event dispatch. Not production execution. */
#include "battle-runtime-projection.c"
#include "psx/gtereg.h"
#include <SDL.h>
extern void PsyX_Sys_InitSoundGate(void);
extern void PsyX_Sys_DispatchCounter2(void);
extern int PsyX_Sys_ReserveSoundEvent(unsigned int);
extern int PsyX_Sys_BeginReservedSoundBracket(unsigned int);
extern int PsyX_Sys_FinishReservedSoundBracket(void);
extern int PsyX_Sys_ValidateSoundEventReservation(unsigned int,int);
extern int PsyX_Sys_AbortReservedSoundBracket(void);
extern int OpenEvent(unsigned int, int, int, long (*)(void));
extern int EnableEvent(unsigned int);
extern int DisableEvent(unsigned int);
extern int CloseEvent(unsigned int);
static int event_id, sound_ticks, reenter_event;
static long event_tick(void) { ++sound_ticks; return 0; }
static int event_dispatch(void *unused) { (void)unused; PsyX_Sys_DispatchCounter2(); return 0; }
static void try_sound_tick(void) { SDL_Thread *t=SDL_CreateThread(event_dispatch,"combined-tick",NULL); assert(t); SDL_WaitThread(t,NULL); }
static void bracket_begin(void) { assert(PsyX_Sys_ReserveSoundEvent(event_id)); assert(PsyX_Sys_BeginReservedSoundBracket(event_id)); sound_ticks=0; }
static void bracket_finish(void) { assert(sound_ticks==0); assert(PsyX_Sys_FinishReservedSoundBracket()); try_sound_tick(); assert(sound_ticks==1); }

/* Unused runtime globals required by the linked fixture. */
void *g_Heap;
unsigned int g_HeapIsErrorHandlerOff;
uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
int32_t D_80059478;
SoundVoiceData *g_SoundChannels[24];
uint32_t g_SoundKeyOnFlags;
int32_t g_unk_VoicesNeedingProcessing;
uint32_t g_unk_SoundEvent;
SoundVolumeController g_SoundVolumeController;
AudioManager *g_SoundAudioManagerListHead;
void SoundHandleError(s32 error) { (void)error; abort(); }
void SoundReleaseAllVoices(AudioManager *manager) { (void)manager; abort(); }
extern s32 SoundRemoveAudioManagerFromList(AudioManager *);
extern s32 __real_SoundRemoveAudioManagerFromList(AudioManager *);
static uint64_t synthetic_manager_epoch;
s32 __wrap_SoundRemoveAudioManagerFromList(AudioManager *manager)
{
 s32 result=__real_SoundRemoveAudioManagerFromList(manager);
 if(!result)synthetic_manager_epoch++;
 return result;
}
void SpuSetCommonAttr(SpuCommonAttr *attr) { (void)attr; abort(); }
extern void PcPort_CommitPendingBootSoundCommonAttr(void);
extern void SoundAssignVoiceToChannelAndStop(SoundVoiceData *,uint32_t);
extern void SoundReleaseVoiceFromChannel(SoundVoiceData *,uint32_t);
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
int32_t D_80059488,D_80010000=-1,D_80059390;
extern void (*D_800501FC)(void);
extern void func_8003634C(void);
void PcPort_PrepareControllerPoll(void) {}
void PcPort_BeforeControllerPush(void) {}
void func_80035E44(void) {}
void func_80036220(void) {}
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
#include "release-prepared-body.inc"
#include "assign-prepared-body.inc"
/* Test-only guest alias adapter. It resolves before reservation and reuses
 * the existing host-pointer preflight; live ownership/rollback remain open. */
static uint32_t guest_assign_address,guest_assign_host;
static int prepare_guest_sound_assign(BattleMipsRuntime *rt,unsigned event,uint32_t guest,uint32_t voice,uint32_t proven_slot_segment)
{
 guest_assign_address=0;guest_assign_host=0;
 if(proven_slot_segment!=(guest&0xffe00000u))return 0;
 if((guest&0xffe00000u)!=0x80000000u || (guest&1u) || (guest&0x1fffffu)>BATTLE_BUS_KUSEG_RAM_END-8u)return 0;
 uint8_t *host=resolve_memory(rt,guest,8u,0);
 if(!host || host!=g_PsxRam+(guest&0x1fffffu) || resolve_memory(rt,guest,8u,1)!=host || (uintptr_t)host>UINT32_MAX)return 0;
 uint32_t candidates[2]={(uint32_t)(uintptr_t)host,0},voices[2]={voice,0};
 if(!prepare_sound_assign(rt,event,1u,candidates,voices,&prepared_assign))return 0;
 guest_assign_address=guest;guest_assign_host=candidates[0];return 1;
}
static void scratch_sound_assign_guest(BattleEntryCall *e)
{
 if(!guest_assign_address || e->cpu->gpr[4]!=guest_assign_address)battle_entry_fail(e,"synthetic guest assignment changed",e->cpu->gpr[4]);
 BattleEntryCall child={.runtime=e->runtime,.cpu=e->cpu};e->cpu->gpr[4]=guest_assign_host;
 if(setjmp(child.failure)==0) {scratch_sound_assign(&child);e->cpu->gpr[4]=guest_assign_address;return;}
 e->cpu->gpr[4]=guest_assign_address;longjmp(e->failure,1);
}
#include "continuation-memory.inc"
#include "sound-heap-membership.inc"
static const SyntheticSoundHeap *active_list_guard;
static int active_epoch_guard;
static uint64_t expected_manager_epoch;
static void bracket_begin_prepared_assignment(void) {
 assert(PsyX_Sys_ReserveSoundEvent(event_id));
 if(!capture_sound_assign(&prepared_assign) || !capture_sound_spans(&prepared_memory,event_id)) {PsyX_Sys_ReleaseSoundEventReservation();assert(!"synthetic capture refused");}
 assert(PsyX_Sys_BeginReservedSoundBracket(event_id));sound_ticks=0;
}

#include "instrument-prepared-body.inc"
static uint32_t cpu_calls[8],host_calls[8];
static unsigned cpu_call_count,host_call_count;
static int synthetic_mutation;
static unsigned transient_callback_writes;
static BattleMipsRuntime *callback_runtime;
static uint32_t callback_target;
static unsigned callback_count;
static void fixture_vblank(void) {
    assert((uint32_t)D_80059488==callback_count+1u);
    callback_count++;
    if(reenter_event==1 && callback_target==0x8003e724u) {
        CloseEvent(event_id);
        int replacement=OpenEvent(0xf2000002u,2,0x1000,event_tick);
        assert(replacement==event_id); EnableEvent(replacement);
        uint32_t prior; assert(runtime_read(callback_runtime,0x80150008u,4,&prior)==0 && prior==0x01040001u);
        assert(runtime_write(callback_runtime,0x80150008u,4,0x89abcdefu)==0);
        reenter_event=-1;
    }
    if(synthetic_mutation==1 && callback_target==0x8003e724u)
        assert(runtime_write(callback_runtime,0x80059554u,4,0x12000000u)==0);
    if(synthetic_mutation==2 && callback_target==0x8003e724u)
        assert(runtime_write(callback_runtime,0x80140048u,4,0x89abcdefu)==0);
    if(synthetic_mutation==3 && callback_target==0x8003e724u)
        assert(runtime_write(callback_runtime,0x80150008u,4,0x11223344u)==0);
    if(synthetic_mutation==4 && callback_target==0x8003e724u)
        assert(runtime_write(callback_runtime,0x80160030u,4,0x55667788u)==0);
    if(synthetic_mutation==5 && callback_target==0x8003e724u)
        assert(runtime_write(callback_runtime,0x80120010u,4,0x22334455u)==0);
    if(synthetic_mutation==6 && callback_target==0x8003e724u)
        assert(runtime_write(callback_runtime,0x80120100u,4,0x66778899u)==0);
    if(synthetic_mutation==7 && callback_target==0x8003e724u) {
        uint32_t prior;assert(!runtime_read(callback_runtime,0x80140048u,4,&prior));
        assert(!runtime_write(callback_runtime,0x80140048u,4,prior^0x01000000u));
        assert(!runtime_write(callback_runtime,0x80140048u,4,prior));transient_callback_writes++;
    }
    if(synthetic_mutation==8 && callback_target==0x8003e724u)
        PcPort_CommitPendingBootSoundCommonAttr();
    if(synthetic_mutation==9 && callback_target==0x8003e724u)
        assert(SoundRemoveAudioManagerFromList((AudioManager *)PSX_ADDR(0x80140000u))==0);
    if(synthetic_mutation==10 && callback_target==0x8003e724u) {
        AudioManager *manager=(AudioManager *)PSX_ADDR(0x80140000u);
        assert(SoundRemoveAudioManagerFromList(manager)==0);
        g_SoundAudioManagerListHead=manager; /* deliberately restored by fixture */
    }
    if(synthetic_mutation && callback_target==0x800404a4u)
        assert(runtime_write(callback_runtime,0x800594fcu,4,0x34000000u)==0);
}
static void synthetic_callback(BattleMipsRuntime *runtime,uint32_t target) {
    callback_runtime=runtime; callback_target=target; fake_vblank++;
    PcPort_ServiceVblank();
    if(synthetic_mutation!=8) {try_sound_tick(); assert(sound_ticks==0);}
}
static void reset_vblank(void) {
    fake_vblank=0; callback_count=0; D_80059488=0; PcPort_ResetVblankService();
    D_800501FC=fixture_vblank;
    func_8004B7D0(func_8003634C);
}
static void scratch_sound_call(BattleEntryCall *e,uint32_t target) {
    assert(host_call_count < 8); host_calls[host_call_count++]=target;
    synthetic_callback(e->runtime,target);
    if(active_list_guard && !capture_synthetic_manager_list_state(active_list_guard,
       (uint32_t)(uintptr_t)g_SoundAudioManagerListHead,0))longjmp(e->failure,1);
    if(active_epoch_guard && synthetic_manager_epoch!=expected_manager_epoch)longjmp(e->failure,1);
    if(!prepared_sound_data_unchanged(e))longjmp(e->failure,1);
    switch(target) {
    case 0x8003e5bcu: scratch_sound_instrument(e); break;
    case 0x8003e724u: scratch_sound_assign(e); break;
    case 0x8003e83cu: scratch_sound_release(e); break;
    case 0x800404a4u: if(reenter_event==-1) { if(PsyX_Sys_FinishReservedSoundBracket()) abort(); battle_entry_fail(e,"synthetic callback changed event",0); } bracket_finish(); e->cpu->gpr[2]=1; e->cpu->gpr[9]=12; e->cpu->gpr[10]=0xb0u; break;
    default: abort();
    }
    if(target!=0x800404a4u)prepared_sound_refresh(e);
}
#define battle_entry_read prepared_sound_read
#define battle_entry_write prepared_sound_write
#include "continuation-body.inc"
#undef battle_entry_read
#undef battle_entry_write
static int sound_event_oracle(void *opaque,PcPortMipsCpu *cpu,uint32_t target){
 if((target==0x8003e5bcu && cpu->gpr[31]==0x8003b884u) ||
    (target==0x8003e724u && cpu->gpr[31]==0x8003b89cu) ||
    (target==0x8003e83cu && cpu->gpr[31]==0x8003b8ccu) ||
    (target==0x800404a4u && cpu->gpr[31]==0x8003b8fcu)) {
     assert(cpu_call_count<8); cpu_calls[cpu_call_count++]=target;
     synthetic_callback(opaque,target);
 }
 if(target!=0x800404a4u||cpu->gpr[31]!=0x8003b8fcu)return 0;
 bracket_finish(); cpu->gpr[2]=1;cpu->gpr[9]=12;cpu->gpr[10]=0xb0u;return 1;
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
int main(void){ assert(SDL_Init(0)==0); PsyX_Sys_InitSoundGate(); event_id=OpenEvent(0xf2000002u,2,0x1000,event_tick); assert(event_id>0); EnableEvent(event_id);
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
const unsigned cases[]={0,1,2,5,6,7}; for(unsigned case_index=0;case_index<6;case_index++){unsigned k=cases[case_index];cpu_call_count=host_call_count=0;synthetic_mutation=(k==5);uint32_t el=0x80150000u,sed=0x80120000u,manager=0x80140000u,bank=0x80160000u,sp=0x801ff000u-80u,count=(k==0||k==2)?1:2;memset(PSX_ADDR(el),0x55,0x300);memset(PSX_ADDR(sed),0,0x200);memset(PSX_ADDR(manager),0,0x80);memset(PSX_ADDR(bank),0,0x80);store_le(PSX_ADDR(0x80059504u),4,0x12345678u);dirty=0x01020304u;key=0xf0f0f0f0u;memset(slots,0,sizeof slots);store_le(PSX_ADDR(0x800595bcu),4,0x42u);store_le(PSX_ADDR(manager+0x48u),4,0xabcdu);store_le(PSX_ADDR(manager+0x10u),2,0x1234u);store_le(PSX_ADDR(sed+0x10u),2,k&1);store_le(PSX_ADDR(sed+0x16u),1,7);store_le(PSX_ADDR(sed+0x100u),2,k>=2?0x180u:0);store_le(PSX_ADDR(sed+0x102u),2,k>=3&&k!=4?0x184u:0);store_le(PSX_ADDR(bank+0x28u),4,0x11110000u);store_le(PSX_ADDR(bank+0x30u),4,0x1234u);store_le(PSX_ADDR(bank+0x34u),2,0x40u);store_le(PSX_ADDR(bank+0x36u),2,0x5678u);store_le(PSX_ADDR(bank+0x38u),4,0xa1b2c3d4u);store_le(PSX_ADDR(bank+0x3cu),2,0xe7f8u);store_le(PSX_ADDR(sp+16u),4,0x01040001u);store_le(PSX_ADDR(sp+24u),2,0x4000u);store_le(PSX_ADDR(sp+32u),1,0x80u);const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)store_le(PSX_ADDR(sp+76u-4u*i),4,i==0?0x80039e08u:0x50200000u+i);for(unsigned j=0;j<count;j++){uint32_t e=el+j*0x158u;store_le(PSX_ADDR(e+6u),1,j+1u);store_le(PSX_ADDR(e+0x27u),1,j+3u);store_le(PSX_ADDR(e+0x34u),2,0x100u);slots[j+3u]=((k==1||k==2)&&j==0)?NULL:((k==6||k==7)&&j==0)?PSX_ADDR(el+0x280u):PSX_ADDR(e+0x30u);}if(k==6 || k==7)store_le(PSX_ADDR(el+0x284u),2,k==6?0x200u:0xff00u);memcpy(initial,g_PsxRam,sizeof initial);initial_gte();reset_vblank();PcPortMipsCpu cpu;initialize_cpu(&cpu,&runtime);cpu.bus.bridge=sound_event_oracle;for(unsigned i=1;i<32;i++)cpu.gpr[i]=0x31400000u+i;cpu.gpr[16]=(uint32_t)(uintptr_t)PSX_ADDR(el+0x27u);cpu.gpr[17]=(uint32_t)(uintptr_t)PSX_ADDR(el);cpu.gpr[18]=(uint32_t)(uintptr_t)PSX_ADDR(sed);cpu.gpr[19]=(uint32_t)(uintptr_t)PSX_ADDR(el+0x30u);cpu.gpr[20]=(uint32_t)(uintptr_t)PSX_ADDR(manager);cpu.gpr[21]=(uint32_t)(uintptr_t)PSX_ADDR(sed+0x100u);cpu.gpr[22]=count;cpu.gpr[23]=k>=3?(uint32_t)(uintptr_t)PSX_ADDR(bank):0;cpu.gpr[29]=sp;cpu.gpr[30]=0x4321u;cpu.gpr[31]=0x8003b76cu;PcPortMipsCpu host=cpu;bracket_begin();assert(PcPortMipsRun(&cpu,0x8003b76cu,0x80039e08u,10000)==PC_PORT_MIPS_HALTED);memcpy(expected,g_PsxRam,sizeof expected);memcpy(expected_slots,slots,sizeof slots);uint32_t want_dirty=dirty,want_key=key;unsigned want_callback_count=callback_count;GTERegisters want=gteRegs;memcpy(g_PsxRam,initial,sizeof initial);dirty=0x01020304u;key=0xf0f0f0f0u;memset(slots,0,sizeof slots);for(unsigned j=0;j<count;j++)slots[j+3u]=((k==1||k==2)&&j==0)?NULL:((k==6||k==7)&&j==0)?PSX_ADDR(el+0x280u):PSX_ADDR(el+j*0x158u+0x30u);initial_gte();reset_vblank();BattleEntryCall e={.runtime=&runtime,.cpu=&host};assert(prepare_sound_release(&runtime,event_id,&prepared_release));
uint32_t candidates[2]={(uint32_t)(uintptr_t)PSX_ADDR(el+0x30u),(uint32_t)(uintptr_t)PSX_ADDR(el+0x188u)},voices[2]={3u,4u};
uint32_t elements[2]={(uint32_t)(uintptr_t)PSX_ADDR(el),(uint32_t)(uintptr_t)PSX_ADDR(el+0x158u)};
if(host.gpr[23])assert(prepare_sound_instrument(&runtime,event_id,host.gpr[23],count,elements,&prepared_instrument));
assert(prepare_sound_assign(&runtime,event_id,count,candidates,voices,&prepared_assign));
assert(prepare_sound_memory(&runtime,&host,&prepared_memory));
if(setjmp(e.failure)) {PsyX_Sys_AbortReservedSoundBracket();assert(!"prepared continuation refused");}
bracket_begin_prepared_assignment();scratch_sound_continuation(&e);if(callback_count!=want_callback_count || callback_count!=cpu_call_count){fprintf(stderr,"case %u CPU calls=%u callbacks=%u host callbacks=%u fake=%u\n",k,cpu_call_count,want_callback_count,callback_count,fake_vblank);return 1;}assert(cpu_call_count==host_call_count&&!memcmp(cpu_calls,host_calls,cpu_call_count*sizeof(cpu_calls[0])));for(unsigned i=1;i<32;i++)if(cpu.gpr[i]!=host.gpr[i]){fprintf(stderr,"case %u r%u cpu=%08x host=%08x\n",k,i,cpu.gpr[i],host.gpr[i]);return 1;}assert(cpu.hi==host.hi&&cpu.lo==host.lo&&!memcmp(expected,g_PsxRam,sizeof expected)&&!memcmp(expected_slots,slots,sizeof slots)&&dirty==want_dirty&&key==want_key);if(k==0)assert(slots[3]==NULL);if(k==2)assert(slots[3]==PSX_ADDR(el+0x30u));assert(!memcmp(&want,&gteRegs,sizeof(gteRegs)));
if(k==5) {
 memcpy(g_PsxRam,initial,sizeof initial);dirty=0x01020304u;key=0xf0f0f0f0u;memset(slots,0,sizeof slots);
 for(unsigned j=0;j<count;j++)slots[j+3u]=PSX_ADDR(el+j*0x158u+0x30u);
 initial_gte();reset_vblank();/* Reconstruct entry registers, not the post-return CPU. */
 initialize_cpu(&host,&runtime); for(unsigned i=1;i<32;i++)host.gpr[i]=0x31400000u+i;
 host.gpr[16]=(uint32_t)(uintptr_t)PSX_ADDR(el+0x27u);host.gpr[17]=(uint32_t)(uintptr_t)PSX_ADDR(el);host.gpr[18]=(uint32_t)(uintptr_t)PSX_ADDR(sed);host.gpr[19]=(uint32_t)(uintptr_t)PSX_ADDR(el+0x30u);host.gpr[20]=(uint32_t)(uintptr_t)PSX_ADDR(manager);host.gpr[21]=(uint32_t)(uintptr_t)PSX_ADDR(sed+0x100u);host.gpr[22]=count;host.gpr[23]=(uint32_t)(uintptr_t)PSX_ADDR(bank);host.gpr[29]=sp;host.gpr[30]=0x4321u;host.gpr[31]=0x8003b76cu;
 BattleEntryCall failure_entry={.runtime=&runtime,.cpu=&host}; host_call_count=0;reenter_event=1;
 assert(prepare_sound_assign(&runtime,event_id,count,candidates,voices,&prepared_assign));
 assert(prepare_sound_memory(&runtime,&host,&prepared_memory));bracket_begin_prepared_assignment();
 if(setjmp(failure_entry.failure)==0) {scratch_sound_continuation(&failure_entry); assert(!"expected callback re-entry refusal");}
 assert(reenter_event==-1);assert(dirty==0x12000000u); /* assignment did not run after replacement */
 assert(load_le(PSX_ADDR(el+8u),4)==0x89abcdefu); /* callback mutation survives refusal */
 assert(PsyX_Sys_AbortReservedSoundBracket()==2);
 try_sound_tick();assert(sound_ticks==1); /* replacement survives and reservation is released */
 reenter_event=0;
}
}
 /* A changed-then-restored event must refuse release before any writes. */
 assert(prepare_sound_release(&runtime,event_id,&prepared_release));
 dirty=0x12345678u;key=0xabcdef01u;slots[3]=PSX_ADDR(0x80150030u);
 PcPortMipsCpu refused={0};refused.gpr[4]=(uint32_t)(uintptr_t)slots[3];refused.gpr[5]=3u;
 BattleEntryCall refusal={.runtime=&runtime,.cpu=&refused};bracket_begin();
 EnableEvent(event_id);DisableEvent(event_id);
 if(setjmp(refusal.failure)==0) {scratch_sound_release(&refusal);assert(!"expected changed event refusal");}
 assert(dirty==0x12345678u && key==0xabcdef01u && slots[3]==PSX_ADDR(0x80150030u));
 assert(PsyX_Sys_AbortReservedSoundBracket()==2);EnableEvent(event_id);
 /* Compare the out-of-range no-write branch with retail CPU execution. */
 PcPortMipsCpu retail_release;initialize_cpu(&retail_release,&runtime);retail_release.bus.bridge=sound_event_oracle;
 for(unsigned i=1;i<32;i++)retail_release.gpr[i]=0x31400000u+i;
 retail_release.gpr[5]=24u;retail_release.gpr[31]=0x8000deacu;
 PcPortMipsCpu native_release=retail_release;
 bracket_begin();assert(PcPortMipsRun(&retail_release,0x8003e83cu,retail_release.gpr[31],100)==PC_PORT_MIPS_HALTED);bracket_finish();
 BattleEntryCall leaf={.runtime=&runtime,.cpu=&native_release};bracket_begin();scratch_sound_release(&leaf);bracket_finish();
 assert(!memcmp(retail_release.gpr,native_release.gpr,sizeof retail_release.gpr) && retail_release.hi==native_release.hi && retail_release.lo==native_release.lo);
 assert(dirty==0x12345678u && key==0xabcdef01u && slots[3]==PSX_ADDR(0x80150030u));
 /* Simulate a callback changing a prepared slot; refuse before further writes. */
 uint32_t one_candidate[2]={(uint32_t)(uintptr_t)PSX_ADDR(0x80150030u),0},one_voice[2]={3u,0};
 assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
 PcPortMipsCpu memory_for_slots={0};memory_for_slots.gpr[17]=0x80150000u;memory_for_slots.gpr[20]=0x80140000u;
 memory_for_slots.gpr[29]=0x801fefb0u;memory_for_slots.gpr[18]=0x80120000u;memory_for_slots.gpr[21]=0x80120100u;
 assert(prepare_sound_memory(&runtime,&memory_for_slots,&prepared_memory));
 PcPortMipsCpu changed={0};changed.gpr[4]=one_candidate[0];changed.gpr[5]=3u;
 BattleEntryCall changed_entry={.runtime=&runtime,.cpu=&changed};bracket_begin_prepared_assignment();slots[3]=NULL;
 uint32_t candidate_before=load_le(PSX_ADDR(0x80150030u),4);
 if(setjmp(changed_entry.failure)==0) {scratch_sound_assign(&changed_entry);assert(!"expected changed slot refusal");}
 assert(!slots[3] && dirty==0x12345678u && key==0xabcdef01u && load_le(PSX_ADDR(0x80150030u),4)==candidate_before);
 assert(PsyX_Sys_AbortReservedSoundBracket()==1);
 /* Failed raw-reservation capture must release the lock without disabling. */
 slots[3]=(void*)(uintptr_t)0x100000000ull;
 assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
 assert(PsyX_Sys_ReserveSoundEvent(event_id));assert(!capture_sound_assign(&prepared_assign));
 assert(PsyX_Sys_ReleaseSoundEventReservation());sound_ticks=0;try_sound_tick();assert(sound_ticks==1);
 /* A changed bank binding refuses before touching the instrument element. */
 uint32_t instrument_elements[2]={(uint32_t)(uintptr_t)PSX_ADDR(0x80150000u),0};
 uint32_t instrument_bank=(uint32_t)(uintptr_t)PSX_ADDR(0x80160000u);
 assert(!prepare_sound_instrument(&runtime,event_id,instrument_elements[0],1u,instrument_elements,&prepared_instrument));
 assert(prepare_sound_instrument(&runtime,event_id,instrument_bank,1u,instrument_elements,&prepared_instrument));
 store_le(PSX_ADDR(0x8015002cu),4,1u);uint8_t saved_instrument[0x70];memcpy(saved_instrument,PSX_ADDR(0x80150000u),sizeof saved_instrument);
 PcPortMipsCpu invalid_instrument={0};invalid_instrument.gpr[5]=instrument_elements[0];
 BattleEntryCall invalid_entry={.runtime=&runtime,.cpu=&invalid_instrument};bracket_begin();
 if(setjmp(invalid_entry.failure)==0) {scratch_sound_instrument(&invalid_entry);assert(!"expected changed bank refusal");}
 assert(!memcmp(saved_instrument,PSX_ADDR(0x80150000u),sizeof saved_instrument));
 assert(PsyX_Sys_AbortReservedSoundBracket()==1);
 /* An aliased span fails preparation, and an uncaptured access releases the
  * synthetic bracket before reporting the refusal. */
 PcPortMipsCpu memory_cpu={0};memory_cpu.gpr[17]=0x80150000u;memory_cpu.gpr[20]=0x80150000u;
 memory_cpu.gpr[29]=0x801fefb0u;memory_cpu.gpr[18]=0x80120000u;memory_cpu.gpr[21]=0x80120100u;
 assert(!prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));memory_cpu.gpr[20]=0x80140000u;
 assert(prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));
 BattleEntryCall bad_access={.runtime=&runtime,.cpu=&memory_cpu};bracket_begin();
 if(setjmp(bad_access.failure)==0) {(void)prepared_sound_read(&bad_access,0x80180000u,4u);assert(!"expected uncaptured read refusal");}
 assert(PsyX_Sys_AbortReservedSoundBracket()==1);sound_ticks=0;try_sound_tick();assert(sound_ticks==1);
 /* A callback can mutate the manager without changing event history. */
 slots[3]=PSX_ADDR(0x80150030u);store_le(PSX_ADDR(0x80140048u),4,0x13579bdfu);
 assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
 assert(prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));
 PcPortMipsCpu manager_cpu={0};manager_cpu.gpr[4]=one_candidate[0];manager_cpu.gpr[5]=3u;
 BattleEntryCall manager_entry={.runtime=&runtime,.cpu=&manager_cpu};reset_vblank();synthetic_mutation=2;host_call_count=0;
 bracket_begin_prepared_assignment();
 if(setjmp(manager_entry.failure)==0) {scratch_sound_call(&manager_entry,0x8003e724u);assert(!"expected manager mutation refusal");}
 assert(load_le(PSX_ADDR(0x80140048u),4)==0x89abcdefu && slots[3]==PSX_ADDR(0x80150030u));
 assert(PsyX_Sys_ReservedSoundBracketPristine(event_id)); /* event history alone missed the data change */
 assert(PsyX_Sys_AbortReservedSoundBracket()==1);synthetic_mutation=0;
 /* Detect element, bank, SED and offset changes while event history remains intact. */
 for(unsigned mode=3;mode<=6;mode++) {
  slots[3]=PSX_ADDR(0x80150030u);memory_cpu.gpr[23]=instrument_bank;
  assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
  assert(prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));
  manager_cpu.gpr[4]=one_candidate[0];manager_cpu.gpr[5]=3u;reset_vblank();synthetic_mutation=mode;host_call_count=0;
  bracket_begin_prepared_assignment();
  if(setjmp(manager_entry.failure)==0) {scratch_sound_call(&manager_entry,0x8003e724u);assert(!"expected sound data mutation refusal");}
  assert(PsyX_Sys_ReservedSoundBracketPristine(event_id));
  const uint32_t changed_addresses[]={0x80150008u,0x80160030u,0x80120010u,0x80120100u};
  const uint32_t changed_values[]={0x11223344u,0x55667788u,0x22334455u,0x66778899u};
  assert(load_le(PSX_ADDR(changed_addresses[mode-3u]),4)==changed_values[mode-3u]);
  assert(slots[3]==PSX_ADDR(0x80150030u));assert(PsyX_Sys_AbortReservedSoundBracket()==1);synthetic_mutation=0;
 }
 /* A callback can change and restore captured data before the next snapshot.
  * This fixed synthetic case documents the snapshot's detection limit. */
 slots[3]=PSX_ADDR(0x80150030u);memory_cpu.gpr[23]=instrument_bank;
 assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
 assert(prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));
 manager_cpu.gpr[4]=one_candidate[0];manager_cpu.gpr[5]=3u;reset_vblank();
 synthetic_mutation=7;transient_callback_writes=0;host_call_count=0;bracket_begin_prepared_assignment();
 if(setjmp(manager_entry.failure)==0) scratch_sound_call(&manager_entry,0x8003e724u);
 else assert(!"restored synthetic change unexpectedly detected");
 assert(transient_callback_writes==1u && PsyX_Sys_ReservedSoundBracketPristine(event_id));
 assert(PsyX_Sys_AbortReservedSoundBracket()==1);synthetic_mutation=0;
 /* Invoke the real boot sound commit as a deliberately installed callback.
  * Its nested event pair invalidates the bracket even with an empty mask. */
 slots[3]=PSX_ADDR(0x80150030u);memory_cpu.gpr[23]=instrument_bank;
 assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
 assert(prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));
 manager_cpu.gpr[4]=one_candidate[0];manager_cpu.gpr[5]=3u;reset_vblank();
 memset(&g_SoundVolumeController,0,sizeof g_SoundVolumeController);g_unk_SoundEvent=(uint32_t)event_id;
 synthetic_mutation=8;host_call_count=0;bracket_begin_prepared_assignment();
 if(setjmp(manager_entry.failure)==0) {scratch_sound_call(&manager_entry,0x8003e724u);assert(!"expected nested boot commit refusal");}
 assert(!PsyX_Sys_ReservedSoundBracketPristine(event_id));
 assert(PsyX_Sys_AbortReservedSoundBracket()==2);synthetic_mutation=0;
 sound_ticks=0;try_sound_tick();assert(sound_ticks==1);
 /* The real list-removal leaf has no local event bracket. Removing a head
  * need not modify manager bytes, so the current data snapshot can miss it. */
 slots[3]=PSX_ADDR(0x80150030u);memory_cpu.gpr[23]=instrument_bank;
 store_le(PSX_ADDR(0x80140000u),4,0);store_le(PSX_ADDR(0x80140010u),2,0);
 g_SoundAudioManagerListHead=(AudioManager *)PSX_ADDR(0x80140000u);
 assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
 assert(prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));
 manager_cpu.gpr[4]=one_candidate[0];manager_cpu.gpr[5]=3u;reset_vblank();
 synthetic_mutation=9;host_call_count=0;bracket_begin_prepared_assignment();
 if(setjmp(manager_entry.failure)==0)scratch_sound_call(&manager_entry,0x8003e724u);
 else assert(!"head-only synthetic removal unexpectedly detected");
 assert(!g_SoundAudioManagerListHead && PsyX_Sys_ReservedSoundBracketPristine(event_id));
 assert(PsyX_Sys_AbortReservedSoundBracket()==1);synthetic_mutation=0;
 /* A bounded list/heap snapshot can detect this particular unlink while
  * the event is disabled; it still cannot prove lifetime or restored changes. */
 uint8_t *manager_arena=PSX_ADDR(0x8013fef0u),*manager_block=PSX_ADDR(0x80140000u);
 uint32_t arena_addr=(uint32_t)(uintptr_t)manager_arena,manager_addr=(uint32_t)(uintptr_t)manager_block;
 memset(manager_arena,0,0x110u);store_le(manager_arena,2,0x8000u);
 store_le(manager_arena+8u,4,arena_addr+16u);store_le(manager_arena+12u,4,arena_addr+0x100u);
 store_le(manager_arena+0x100u,2,2u);store_le(manager_arena+0x108u,4,manager_addr+0x94u+2u*0x158u);
 store_le(manager_block+0x14u,1,2u);store_le(manager_block+0x10u,2,0);
 g_SoundAudioManagerListHead=(AudioManager *)manager_block;
 SyntheticSoundHeap list_guard;assert(prepare_synthetic_sound_heap(&runtime,arena_addr,0x6300u,manager_addr,2u,event_id,&list_guard));
 slots[3]=PSX_ADDR(0x80150030u);memory_cpu.gpr[23]=instrument_bank;
 assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
 assert(prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));
 manager_cpu.gpr[4]=one_candidate[0];manager_cpu.gpr[5]=3u;reset_vblank();synthetic_mutation=9;host_call_count=0;
 assert(PsyX_Sys_ReserveSoundEvent(event_id));assert(capture_sound_assign(&prepared_assign));
 assert(capture_sound_spans(&prepared_memory,event_id));assert(capture_synthetic_manager_list(&list_guard,manager_addr));
 assert(PsyX_Sys_BeginReservedSoundBracket(event_id));sound_ticks=0;active_list_guard=&list_guard;
 if(setjmp(manager_entry.failure)==0){scratch_sound_call(&manager_entry,0x8003e724u);assert(!"expected manager membership refusal");}
 active_list_guard=NULL;assert(!g_SoundAudioManagerListHead && slots[3]==PSX_ADDR(0x80150030u));
 assert(PsyX_Sys_ReservedSoundBracketPristine(event_id));assert(PsyX_Sys_AbortReservedSoundBracket()==1);synthetic_mutation=0;
 /* Restoring the head before the check makes even the membership snapshot
  * look unchanged; this controlled callback is not a live-game claim. */
 g_SoundAudioManagerListHead=(AudioManager *)manager_block;slots[3]=PSX_ADDR(0x80150030u);
 assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
 assert(prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));
 manager_cpu.gpr[4]=one_candidate[0];manager_cpu.gpr[5]=3u;reset_vblank();synthetic_mutation=10;host_call_count=0;
 assert(PsyX_Sys_ReserveSoundEvent(event_id));assert(capture_sound_assign(&prepared_assign));
 assert(capture_sound_spans(&prepared_memory,event_id));assert(capture_synthetic_manager_list(&list_guard,manager_addr));
 assert(PsyX_Sys_BeginReservedSoundBracket(event_id));sound_ticks=0;active_list_guard=&list_guard;
 if(setjmp(manager_entry.failure)==0)scratch_sound_call(&manager_entry,0x8003e724u);
 else assert(!"restored synthetic membership unexpectedly detected");
 active_list_guard=NULL;assert(g_SoundAudioManagerListHead==(AudioManager *)manager_block);
 assert(PsyX_Sys_ReservedSoundBracketPristine(event_id));assert(PsyX_Sys_AbortReservedSoundBracket()==1);synthetic_mutation=0;
 /* A fixture-only counter around the real removal observes that same restored
  * unlink. It does not cover arbitrary writes or establish production safety. */
 slots[3]=PSX_ADDR(0x80150030u);
 assert(prepare_sound_assign(&runtime,event_id,1u,one_candidate,one_voice,&prepared_assign));
 assert(prepare_sound_memory(&runtime,&memory_cpu,&prepared_memory));
 manager_cpu.gpr[4]=one_candidate[0];manager_cpu.gpr[5]=3u;reset_vblank();synthetic_mutation=10;host_call_count=0;
 assert(PsyX_Sys_ReserveSoundEvent(event_id));assert(capture_sound_assign(&prepared_assign));
 assert(capture_sound_spans(&prepared_memory,event_id));assert(capture_synthetic_manager_list(&list_guard,manager_addr));
 expected_manager_epoch=synthetic_manager_epoch;active_epoch_guard=1;
 assert(PsyX_Sys_BeginReservedSoundBracket(event_id));sound_ticks=0;active_list_guard=&list_guard;
 if(setjmp(manager_entry.failure)==0){scratch_sound_call(&manager_entry,0x8003e724u);assert(!"expected synthetic epoch refusal");}
 active_list_guard=NULL;active_epoch_guard=0;
 assert(synthetic_manager_epoch==expected_manager_epoch+1u && g_SoundAudioManagerListHead==(AudioManager *)manager_block);
 assert(slots[3]==PSX_ADDR(0x80150030u) && PsyX_Sys_ReservedSoundBracketPristine(event_id));
 assert(PsyX_Sys_AbortReservedSoundBracket()==1);synthetic_mutation=0;
 /* Heap capture reads bounded headers only under the raw reservation. */
 uint8_t *heap=PSX_ADDR(0x80170000u);uint32_t h=(uint32_t)(uintptr_t)heap;
 memset(heap,0,0x6300u);store_le(heap,2,0x8000u);store_le(heap+8u,4,h+16u);store_le(heap+12u,4,h+0x100u);
 store_le(heap+0x100u,2,2u);store_le(heap+0x108u,4,h+0x110u+0x94u+2u*0x158u);store_le(heap+0x124u,1,2u);
 SyntheticSoundHeap view;assert(prepare_synthetic_sound_heap(&runtime,h,0x6300u,h+0x110u,2u,event_id,&view));
 assert(!capture_synthetic_sound_heap(&view));assert(PsyX_Sys_ReserveSoundEvent(event_id));
 assert(capture_synthetic_sound_heap(&view));
 assert(capture_synthetic_manager_list(&view,h+0x110u));
 store_le(heap+0x10cu,4,h+0x500u);store_le(heap+0x500u,2,2u);
 store_le(heap+0x508u,4,h+0x510u+0x94u+2u*0x158u);store_le(heap+0x524u,1,2u);
 store_le(heap+0x110u,4,h+0x510u);assert(capture_synthetic_manager_list(&view,h+0x110u));
 store_le(heap+0x510u,4,h+0x110u);assert(!capture_synthetic_manager_list(&view,h+0x110u));
 store_le(heap+0x510u,4,0);store_le(heap+0x110u,4,0);store_le(heap+0x10cu,4,0);
 assert(!capture_synthetic_manager_list(&view,0));
 store_le(heap+0x124u,1,3u);assert(!capture_synthetic_sound_heap(&view));store_le(heap+0x124u,1,2u);
 store_le(heap+0x108u,4,h+0x110u+0x94u+0x158u);assert(!capture_synthetic_sound_heap(&view));
 store_le(heap+0x108u,4,h+0x110u+0x94u+2u*0x158u);store_le(heap+0x10cu,4,h+0x100u);
 assert(!capture_synthetic_sound_heap(&view));assert(PsyX_Sys_ReleaseSoundEventReservation());
 /* Actual host leaves inside the real SDL bracket; data and callback are
  * synthetic and the callback is explicitly registered by this fixture. */
 uint8_t *actual_element=PSX_ADDR(0x801a0000u),*actual_bank=PSX_ADDR(0x801a1000u);
 assert((uintptr_t)actual_bank<=UINT32_MAX);memset(actual_element,0,0x158u);memset(actual_bank,0,0x80u);
 store_le(actual_element+0x2cu,4,(uint32_t)(uintptr_t)actual_bank);
 store_le(actual_bank+0x28u,4,0x12340u);store_le(actual_bank+0x30u,4,0x123u);
 store_le(actual_bank+0x34u,2,0x45u);store_le(actual_bank+0x36u,2,0x67u);
 store_le(actual_bank+0x38u,4,0x15345678u);store_le(actual_bank+0x3cu,2,0x765u);
 memset(g_SoundChannels,0,sizeof g_SoundChannels);g_SoundKeyOnFlags=0xffffffffu;g_unk_VoicesNeedingProcessing=0;
 synthetic_mutation=0;reenter_event=0;reset_vblank();bracket_begin();
 func_8003E5BC(0,(AudioElement *)actual_element);assert(load_le(actual_element+0x4cu,4)==0x12c58u);
 SoundAssignVoiceToChannelAndStop((SoundVoiceData *)(actual_element+0x30u),22u);
 assert(g_SoundChannels[22]==(SoundVoiceData *)(actual_element+0x30u));
 synthetic_callback(&runtime,0x8003e724u);assert(callback_count==1u && PsyX_Sys_ReservedSoundBracketPristine(event_id));
 SoundReleaseVoiceFromChannel((SoundVoiceData *)(actual_element+0x30u),22u);
 assert(g_SoundChannels[22]==NULL && g_unk_VoicesNeedingProcessing==(1u<<22));
 bracket_finish();
 puts("actual native sound leaves and controller callback PASS inside synthetic SDL bracket");
 /* A guest alias and a host pointer can denote the same fixture address but
  * compare unequal as words. The scratch projection refuses the guest pointer
  * store after earlier voice writes; no rollback is implied by that refusal. */
 uint32_t alias_voice=0x80150030u;uint8_t *alias_host=PSX_ADDR(alias_voice);
 memset(alias_host,0,8u);store_le(alias_host+4u,2,2u);
 memset(slots,0,sizeof slots);slots[3]=alias_host;dirty=0;key=0xffffffffu;
 PcPortMipsCpu alias_cpu;initialize_cpu(&alias_cpu,&runtime);alias_cpu.bus.bridge=sound_event_oracle;
 alias_cpu.gpr[4]=alias_voice;alias_cpu.gpr[5]=3u;alias_cpu.gpr[28]=0x80059170u;alias_cpu.gpr[31]=0x8000deacu;
 assert(PcPortMipsRun(&alias_cpu,0x8003e724u,alias_cpu.gpr[31],200)!=PC_PORT_MIPS_HALTED);
 assert(load_le(alias_host,2)==3u && load_le(alias_host+6u,2)==0xffffu);
 assert(slots[3]==alias_host && dirty==0 && key==0xffffffffu);
 puts("synthetic mixed-pointer alias: scratch projection refused after prior voice writes");
 memset(alias_host,0,8u);store_le(alias_host+4u,2,2u);uint8_t alias_before[8];memcpy(alias_before,alias_host,8u);
 uint32_t alias_candidates[2]={alias_voice,0},alias_voices[2]={3u,0};
 assert(prepare_sound_release(&runtime,event_id,&prepared_release));
 assert(!prepare_sound_assign(&runtime,event_id,1u,alias_candidates,alias_voices,&prepared_assign));
 assert(!memcmp(alias_before,alias_host,8u) && slots[3]==alias_host && dirty==0 && key==0xffffffffu);
 puts("synthetic prepared assignment: guest alias refused before voice writes");
 /* CPU oracle uses a synthetic guest view of slots; the adapter resolves the
  * candidate to host storage before reservation. Fixed slot/priority cases. */
 assert(!setenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW","1",1));
 assert(prepare_sound_release(&runtime,event_id,&prepared_release));
 assert(!prepare_guest_sound_assign(&runtime,event_id,alias_voice+1u,3u,0x80000000u));
 assert(!guest_assign_address && !guest_assign_host);
 assert(!prepare_guest_sound_assign(&runtime,event_id,0x801ffffeu,3u,0x80000000u));
 assert(!guest_assign_address && !guest_assign_host);
 for(unsigned mode=0;mode<4;mode++) {
  uint8_t *other_voice=PSX_ADDR(0x80151030u);
  memset(alias_host,0,8u);memset(other_voice,0,8u);store_le(alias_host+4u,2,2u);store_le(other_voice+4u,2,mode==3?3u:1u);
  slots[3]=mode==0?NULL:mode==1?alias_host:other_voice;dirty=0;key=0xffffffffu;
  assert(prepare_sound_release(&runtime,event_id,&prepared_release));
  assert(prepare_guest_sound_assign(&runtime,event_id,alias_voice,3u,0x80000000u));
  PcPortMipsCpu guest_cpu;initialize_cpu(&guest_cpu,&runtime);guest_cpu.bus.bridge=sound_event_oracle;
  for(unsigned i=1;i<32;i++)guest_cpu.gpr[i]=0x31400000u+i;
  guest_cpu.gpr[4]=alias_voice;guest_cpu.gpr[5]=3u;guest_cpu.gpr[31]=0x8000deacu;
  PcPortMipsCpu native_cpu=guest_cpu;memcpy(initial,g_PsxRam,sizeof initial);
  assert(PsyX_Sys_ReserveSoundEvent(event_id));assert(capture_sound_assign(&prepared_assign));
  assert(PsyX_Sys_BeginReservedSoundBracket(event_id));
  assert(PcPortMipsRun(&guest_cpu,0x8003e724u,guest_cpu.gpr[31],200)==PC_PORT_MIPS_HALTED);
  memcpy(expected,g_PsxRam,sizeof expected);uint32_t want_dirty=dirty,want_key=key;void *want_slot=slots[3];
  assert(PsyX_Sys_AbortReservedSoundBracket()==1);
  memcpy(g_PsxRam,initial,sizeof initial);slots[3]=mode==0?NULL:mode==1?alias_host:other_voice;dirty=0;key=0xffffffffu;
  assert(prepare_sound_release(&runtime,event_id,&prepared_release));
  assert(prepare_guest_sound_assign(&runtime,event_id,alias_voice,3u,0x80000000u));
  assert(PsyX_Sys_ReserveSoundEvent(event_id));assert(capture_sound_assign(&prepared_assign));
  assert(PsyX_Sys_BeginReservedSoundBracket(event_id));
  BattleEntryCall guest_entry={.runtime=&runtime,.cpu=&native_cpu};
  if(setjmp(guest_entry.failure)==0)scratch_sound_assign_guest(&guest_entry);else assert(!"synthetic guest assignment refused");
  assert(!memcmp(guest_cpu.gpr,native_cpu.gpr,sizeof guest_cpu.gpr) && guest_cpu.hi==native_cpu.hi && guest_cpu.lo==native_cpu.lo);
  assert(!memcmp(expected,g_PsxRam,sizeof expected) && dirty==want_dirty && key==want_key && slots[3]==want_slot);
  assert(slots[3]==(mode==3?other_voice:alias_host));
  assert(dirty==(mode==3?0u:(1u<<3)) && key==((mode==0||mode==2)?0xfffffff7u:0xffffffffu));
  assert(PsyX_Sys_AbortReservedSoundBracket()==1);
 }
 /* If a stored slot uses KSEG1 while the argument uses KSEG0, retail word
  * comparison differs even though both map to this host pointer. The fixture
  * adapter cannot recover that provenance from the host pointer alone. */
 assert(!setenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW","2",1));
 memset(alias_host,0,8u);store_le(alias_host+4u,2,2u);slots[3]=alias_host;dirty=0;key=0xffffffffu;
 assert(prepare_sound_release(&runtime,event_id,&prepared_release));assert(prepare_guest_sound_assign(&runtime,event_id,alias_voice,3u,0x80000000u));
 PcPortMipsCpu mixed_cpu;initialize_cpu(&mixed_cpu,&runtime);mixed_cpu.bus.bridge=sound_event_oracle;
 mixed_cpu.gpr[4]=alias_voice;mixed_cpu.gpr[5]=3u;mixed_cpu.gpr[31]=0x8000deacu;
 memcpy(initial,g_PsxRam,sizeof initial);assert(PsyX_Sys_ReserveSoundEvent(event_id));assert(capture_sound_assign(&prepared_assign));
 assert(PsyX_Sys_BeginReservedSoundBracket(event_id));
 assert(PcPortMipsRun(&mixed_cpu,0x8003e724u,mixed_cpu.gpr[31],200)==PC_PORT_MIPS_HALTED);
 assert(key==0xfffffff7u && dirty==(1u<<3));assert(PsyX_Sys_AbortReservedSoundBracket()==1);
 memcpy(g_PsxRam,initial,sizeof initial);slots[3]=alias_host;dirty=0;key=0xffffffffu;
 assert(prepare_sound_release(&runtime,event_id,&prepared_release));
 assert(!prepare_guest_sound_assign(&runtime,event_id,alias_voice,3u,0xa0000000u));
 assert(!guest_assign_address && !guest_assign_host && key==0xffffffffu && dirty==0);
 puts("synthetic KSEG0/KSEG1 mixed view: retail branch differs and provenance preflight refuses");
 assert(!setenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW","1",1));
 /* A changed slot refuses before writes and restores the guest argument. */
 memset(alias_host,0,8u);store_le(alias_host+4u,2,2u);slots[3]=alias_host;dirty=0;key=0xffffffffu;
 assert(prepare_sound_release(&runtime,event_id,&prepared_release));assert(prepare_guest_sound_assign(&runtime,event_id,alias_voice,3u,0x80000000u));
 assert(PsyX_Sys_ReserveSoundEvent(event_id));assert(capture_sound_assign(&prepared_assign));
 assert(PsyX_Sys_BeginReservedSoundBracket(event_id));
 slots[3]=PSX_ADDR(0x80151030u);uint8_t unchanged_voice[8];memcpy(unchanged_voice,alias_host,8u);
 PcPortMipsCpu refusal_cpu={0};refusal_cpu.gpr[4]=alias_voice;refusal_cpu.gpr[5]=3u;
 BattleEntryCall refusal_entry={.runtime=&runtime,.cpu=&refusal_cpu};
 if(setjmp(refusal_entry.failure)==0){scratch_sound_assign_guest(&refusal_entry);assert(!"expected changed-slot refusal");}
 assert(refusal_cpu.gpr[4]==alias_voice && !memcmp(unchanged_voice,alias_host,8u) && dirty==0 && key==0xffffffffu);
 assert(PsyX_Sys_AbortReservedSoundBracket()==1);
 unsetenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW");
 puts("synthetic guest alias assignment: four slot/priority CPU comparisons PASS");
 CloseEvent(event_id); SDL_Quit(); puts("combined continuation and real controller vblank path PASS synthetic callbacks with SDL bracket");return 0;}
