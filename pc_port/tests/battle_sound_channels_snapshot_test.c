/* Popup task lookup versus retail CPU. Both sides use PsyCross GTE. */
#include "battle_mips_runtime.c"
#include "psx/gtereg.h"
/* Unused runtime globals required by the linked fixture. */
void *g_Heap;
unsigned int g_HeapIsErrorHandlerOff;
uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
/* compat.o's real pump is the sole definition; with a frozen vblank count it
 * early-returns before touching pad state (a conflicting no-op stub here
 * breaks the link with multiple-definition). */
int PsyX_Sys_GetVBlankCount(void) { return 0; }
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


SoundVoiceData *g_SoundChannels[24];
static int lockstep,can_reserve,held,reserve_calls,release_calls;
int PcPort_SoundLockstepActive(void) { return lockstep; }
int PsyX_Sys_ReserveSoundEvent(unsigned event) {
 reserve_calls++;if(!can_reserve || event!=7u || held)return 0;held=1;return 1;
}
int PsyX_Sys_ReleaseSoundEventReservation(void) {
 release_calls++;if(!held)return 0;held=0;return 1;
}
int main(void) {
 BattleMipsRuntime rt={0};rt.data_count=1;
 rt.data[0]=(ResolvedData){0x8006252cu,sizeof g_SoundChannels,(uint8_t*)g_SoundChannels,"g_SoundChannels"};
 rt.host_range_count=1;rt.host_ranges[0]=(HostRange){(uintptr_t)g_PsxRam,(uintptr_t)g_PsxRam+PSX_RAM_SIZE,1};
 assert(host_address_valid(&rt,(uintptr_t)g_PsxRam,8u,1));
 assert(!host_address_valid(&rt,UINTPTR_MAX-3u,8u,0));
 assert(!host_address_valid(&rt,rt.host_ranges[0].end-3u,8u,0));
 BattleSoundChannelSnapshot before;
 assert(!setenv("SDL_AUDIODRIVER","dummy",1));assert(!setenv("ALSOFT_DRIVERS","null",1));
 lockstep=0;assert(!battle_sound_channel_diag_safe());
 lockstep=1;assert(!setenv("ALSOFT_DRIVERS","alsa",1));assert(!battle_sound_channel_diag_safe());
 assert(!setenv("ALSOFT_DRIVERS","null",1));assert(battle_sound_channel_diag_safe());
 unsigned reason=0;
 assert(!battle_sound_channel_snapshot_reserved(&rt,7,&before,&reason,NULL,NULL) && reason==102 && !held);
 can_reserve=1;assert(battle_sound_channel_snapshot_reserved(&rt,7,&before,&reason,NULL,NULL) && !held);
 assert(reserve_calls==2 && release_calls==1);
 PcPortMipsCpu cpu={0};BattleSoundBoundaryProbe shape;
 cpu.gpr[17]=0x80150000u;cpu.gpr[16]=cpu.gpr[17]+0x27u;cpu.gpr[19]=cpu.gpr[17]+0x30u;
 cpu.gpr[18]=0x80120000u;cpu.gpr[21]=0x80120100u;cpu.gpr[20]=0x80140000u;
 cpu.gpr[23]=0x80160000u;cpu.gpr[29]=0x801ff000u;cpu.gpr[22]=2u;
 store_le(PSX_ADDR(cpu.gpr[21]),2,0x180u);store_le(PSX_ADDR(cpu.gpr[21]+2u),2,0x184u);
 store_le(PSX_ADDR(cpu.gpr[16]),1,3u);store_le(PSX_ADDR(cpu.gpr[16]+0x158u),1,4u);
 assert(battle_sound_channel_snapshot_reserved(&rt,7,&before,&reason,&cpu,&shape) && !held);
 assert(shape.valid && !shape.problem && shape.offsets[0]==0x180u && shape.offsets[1]==0x184u && shape.voices[0]==3u && shape.voices[1]==4u);
 assert(shape.slot_relation[0]==0 && shape.slot_relation[1]==0 && shape.alias_mask==0);
 BattleSoundBoundaryPrepared prepared;BattleSoundBoundaryProbe aliases={0};
 cpu.gpr[20]=cpu.gpr[17]+0x40u;
 battle_sound_boundary_prepare(&rt,&cpu,&prepared,&aliases);
 assert(prepared.valid && prepared.alias_mask==1u); /* elements / manager */
 assert(battle_sound_channel_snapshot_reserved(&rt,7,&before,&reason,&cpu,&shape) && !held);
 assert(shape.valid && shape.alias_mask==1u);
 cpu.gpr[20]=0x80140000u;cpu.gpr[29]=cpu.gpr[17]+0x100u;
 battle_sound_boundary_prepare(&rt,&cpu,&prepared,&aliases);
 assert(prepared.valid && prepared.alias_mask==2u); /* elements / frame */
 cpu.gpr[29]=0x801ff000u;
 g_SoundChannels[3]=PSX_ADDR(cpu.gpr[19]);g_SoundChannels[4]=PSX_ADDR(0x80141000u);
 store_le(PSX_ADDR(cpu.gpr[19]+4u),2,0x1234u);store_le(PSX_ADDR(0x80141004u),2,0x8123u);
 assert(battle_sound_channel_snapshot_reserved(&rt,7,&before,&reason,&cpu,&shape) && !held);
 assert(shape.valid && shape.slot_relation[0]==1u && shape.slot_relation[1]==2u && shape.slot_priority[0]==0x1234 && shape.slot_priority[1]==(int16_t)0x8123);
 assert(shape.manager_arena_valid && shape.manager_slots==0u && shape.other_slots==2u);
 g_SoundChannels[3]=PSX_ADDR(cpu.gpr[20]+0x94u+0x30u);
 assert(battle_sound_channel_snapshot_reserved(&rt,7,&before,&reason,&cpu,&shape) && !held);
 assert(shape.manager_arena_valid && shape.manager_slots==1u && shape.other_slots==1u);
 g_SoundChannels[3]=NULL;g_SoundChannels[4]=NULL;
 cpu.gpr[19]++;assert(battle_sound_channel_snapshot_reserved(&rt,7,&before,&reason,&cpu,&shape) && !held);
 assert(!shape.valid && shape.problem==1u);
 cpu.gpr[19]--;cpu.gpr[20]=0x12345678u;
 assert(battle_sound_channel_snapshot_reserved(&rt,7,&before,&reason,&cpu,&shape) && !held);
 assert(!shape.valid && shape.problem==2u);
 assert(battle_sound_channel_snapshot(&rt,&before) && before.populated==0);
 g_SoundChannels[2]=PSX_ADDR(0x80140000u);g_SoundChannels[23]=PSX_ADDR(0x80141000u);
 assert(battle_sound_channel_snapshot(&rt,&before) && before.populated==2);
 assert(battle_sound_channels_unchanged(&rt,&before));
 g_SoundChannels[2]=PSX_ADDR(0x80140020u);assert(!battle_sound_channels_unchanged(&rt,&before));
 unsigned problem=0;g_SoundChannels[2]=(void*)(uintptr_t)0x100000000ull;
 assert(!battle_sound_channel_snapshot_reserved(&rt,7,&before,&problem,NULL,NULL) && problem==4u && !held);
 assert(release_calls==8);
 rt.host_ranges[1]=(HostRange){0x80000000u,0x80001000u,1};rt.host_range_count=2;
 g_SoundChannels[2]=(void*)(uintptr_t)0x80000000u;assert(!battle_sound_channel_snapshot(&rt,&before));
 g_SoundChannels[2]=(SoundVoiceData*)PSX_ADDR(0x80140001u);assert(!battle_sound_channel_snapshot(&rt,&before));
 g_SoundChannels[2]=PSX_ADDR(0x80140000u);rt.host_ranges[0].writable=0;
 assert(!battle_sound_channel_snapshot(&rt,&before));rt.host_ranges[0].writable=1;
 rt.data[0].size-=sizeof(void*);assert(!battle_sound_channel_snapshot_detail(&rt,&before,&problem) && problem==1u);
 puts("sound channel snapshot and boundary spans PASS synthetic valid and refusal cases");
}
