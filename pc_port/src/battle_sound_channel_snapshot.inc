/* Read-only LP64 channel-table snapshot. The diagnostic holds a raw sound
 * reservation without changing event state; it does not project writes,
 * validate callbacks or authorize the sound continuation. It cannot detect
 * changed-then-restored slots or changes to pointed-to data. */
extern int PcPort_SoundLockstepActive(void) __attribute__((weak));
extern int PsyX_Sys_ReserveSoundEvent(unsigned int) __attribute__((weak));
extern int PsyX_Sys_ReleaseSoundEventReservation(void) __attribute__((weak));
static int battle_sound_channel_diag_safe(void)
{
    const char *al=getenv("ALSOFT_DRIVERS"),*sdl=getenv("SDL_AUDIODRIVER");
    return PcPort_SoundLockstepActive && PcPort_SoundLockstepActive() &&
           al && strcmp(al,"null")==0 && sdl && strcmp(sdl,"dummy")==0;
}

typedef struct {
    uintptr_t slots[24];
    unsigned populated;
} BattleSoundChannelSnapshot;

/* Read-only span and value observations under the sound reservation. The
 * pointed-to offsets, aliases and lifetimes are not validated. Slot relation
 * is 0 for empty/out-of-range, 1 for the candidate, 2 for another pointer.
 * These checks do not authorize writes, callbacks, or the sound continuation. */
typedef struct {
    int valid;
    unsigned problem;
    uint32_t offsets[2], voices[2], slot_relation[2], alias_mask;
    unsigned manager_arena_valid, manager_slots, other_slots;
    int32_t slot_priority[2];
} BattleSoundBoundaryProbe;

typedef struct {
    uint8_t *offsets, *voice0, *voice1, *candidate0, *candidate1, *manager_arena;
    int valid;
    uint32_t alias_mask;
} BattleSoundBoundaryPrepared;

/* Resolve spans before taking the sound mutex: the shared resolver can emit
 * diagnostics and must not acquire stdio or dynamic-loader locks under it. */
static void battle_sound_boundary_prepare(BattleMipsRuntime *runtime,const PcPortMipsCpu *cpu,
                                          BattleSoundBoundaryPrepared *prepared,
                                          BattleSoundBoundaryProbe *probe)
{
    memset(prepared,0,sizeof *prepared);
    const uint32_t *r=cpu->gpr;
    if(r[22]!=2u || r[17]>UINT32_MAX-0x2b0u || r[16]!=r[17]+0x27u ||
       r[19]!=r[17]+0x30u || r[20]>UINT32_MAX-0x4cu ||
       r[18]>UINT32_MAX-0x18u || r[21]>UINT32_MAX-4u ||
       r[29]>UINT32_MAX-80u || (r[23] && r[23]>UINT32_MAX-0x40u)) {probe->problem=1;return;}
    uint8_t *elements=resolve_memory(runtime,r[17],0x2b0u,0);
    uint8_t *elements_write=resolve_memory(runtime,r[17],0x2b0u,1);
    uint8_t *manager=resolve_memory(runtime,r[20],0x4cu,0);
    uint8_t *manager_write=resolve_memory(runtime,r[20],0x4cu,1);
    uint8_t *frame=resolve_memory(runtime,r[29],80u,0);
    uint8_t *frame_write=resolve_memory(runtime,r[29],80u,1);
    prepared->offsets=resolve_memory(runtime,r[21],4u,0);
    prepared->voice0=resolve_memory(runtime,r[16],1u,0);
    prepared->voice1=resolve_memory(runtime,r[16]+0x158u,1u,0);
    prepared->candidate0=resolve_memory(runtime,r[19],8u,0);
    prepared->candidate1=resolve_memory(runtime,r[19]+0x158u,8u,0);
    uint8_t *sed=resolve_memory(runtime,r[18],0x18u,0);
    uint8_t *bank=r[23]?resolve_memory(runtime,r[23],0x40u,0):NULL;
    if(!elements || elements!=elements_write || !manager || manager!=manager_write ||
       !frame || frame!=frame_write || !sed ||
       !prepared->offsets || !prepared->voice0 || !prepared->voice1 ||
       !prepared->candidate0 || !prepared->candidate1 || (r[23] && !bank)) {probe->problem=2;return;}
    /* Bits enumerate pairwise overlaps between the six resolved spans: elements,
     * manager header, frame, SED header, offset pair and optional bank.
     * This records address overlap only, not lifetime or write safety. */
    const uint8_t *spans[]={elements,manager,frame,sed,prepared->offsets,bank};
    const size_t lengths[]={0x2b0u,0x4cu,80u,0x18u,4u,0x40u};
    unsigned bit=0;
    for(unsigned i=0;i<6;i++)for(unsigned j=i+1;j<6;j++,bit++) {
        uintptr_t a=(uintptr_t)spans[i],b=(uintptr_t)spans[j];
        if(a && b && a<b+lengths[j] && b<a+lengths[i])prepared->alias_mask|=1u<<bit;
    }
    /* Optional read-only classification; full arena resolution happens before
     * reservation. It does not prove ownership, allocation lifetime or that
     * pointed-to contents stay unchanged across callbacks. */
    if(r[20]<=UINT32_MAX-0x20d4u) {
        uint8_t *arena=resolve_memory(runtime,r[20],0x20d4u,0);
        if(arena==manager) {
            int contiguous=1;
            for(unsigned i=0;i<24u;i++) {
                unsigned off=0x94u+i*0x158u+0x30u;
                if(resolve_memory(runtime,r[20]+off,8u,0)!=arena+off) {contiguous=0;break;}
            }
            if(contiguous)prepared->manager_arena=arena;
        }
    }
    prepared->valid=1;
}

static void **battle_sound_channel_binding(BattleMipsRuntime *runtime)
{
    void **native=dlsym(RTLD_DEFAULT,"g_SoundChannels");
    ResolvedData *binding=find_data(runtime,0x8006252cu,24u*sizeof(void*));
    if(!native || !binding || binding->address!=0x8006252cu ||
       binding->host!=(uint8_t*)native || binding->size<24u*sizeof(void*) ||
       !binding->name || strcmp(binding->name,"g_SoundChannels")!=0)return NULL;
    return native;
}

/* problem 1 = binding; 2..25 = invalid slot index + 2. */
static int battle_sound_channel_snapshot_from(BattleMipsRuntime *runtime,
                                               void **native,
                                               BattleSoundChannelSnapshot *snapshot,
                                               unsigned *problem)
{
    if(problem)*problem=0;
    if(!native || !snapshot) {if(problem)*problem=1;return 0;}
    memset(snapshot,0,sizeof *snapshot);
    for(unsigned i=0;i<24u;i++) {
        uintptr_t p;memcpy(&p,&native[i],sizeof p);
        if(p && (p<BATTLE_BUS_KUSEG_RAM_END || p>UINT32_MAX-7u || (p&1u) ||
                 (p&0xff800000u)==0x80000000u || (p&0xff800000u)==0xa0000000u ||
                 !host_address_valid(runtime,p,8u,0) ||
                 !host_address_valid(runtime,p,8u,1))) {
            if(problem)*problem=i+2u;
            return 0;
        }
        snapshot->slots[i]=p;
        if(p)snapshot->populated++;
    }
    return 1;
}

static int battle_sound_channel_snapshot_detail(BattleMipsRuntime *runtime,
                                                 BattleSoundChannelSnapshot *snapshot,
                                                 unsigned *problem)
{
    return battle_sound_channel_snapshot_from(runtime,battle_sound_channel_binding(runtime),snapshot,problem);
}

static int battle_sound_channel_snapshot(BattleMipsRuntime *runtime,
                                          BattleSoundChannelSnapshot *snapshot)
{
    return battle_sound_channel_snapshot_detail(runtime,snapshot,NULL);
}

/* The raw reservation does not disable the event. Release even on refusal;
 * this diagnostic never authorizes writes or the retail sound continuation. */
static int battle_sound_channel_snapshot_reserved(BattleMipsRuntime *runtime,
                                                   uint32_t event,
                                                   BattleSoundChannelSnapshot *snapshot,
                                                   unsigned *problem,
                                                   const PcPortMipsCpu *cpu,
                                                   BattleSoundBoundaryProbe *probe)
{
    if(problem)*problem=0;
    if(probe)memset(probe,0,sizeof *probe);
    if(!battle_sound_channel_diag_safe()) {if(problem)*problem=100;return 0;}
    if(!PsyX_Sys_ReserveSoundEvent || !PsyX_Sys_ReleaseSoundEventReservation) {
        if(problem)*problem=101;
        return 0;
    }
    /* Resolve symbols and optional spans before locking: the loader and
     * shared resolver can take unrelated locks or print diagnostics. */
    void **native=battle_sound_channel_binding(runtime);
    if(!native) {if(problem)*problem=1;return 0;}
    BattleSoundBoundaryPrepared prepared={0};
    if(cpu && probe)battle_sound_boundary_prepare(runtime,cpu,&prepared,probe);
    if(!PsyX_Sys_ReserveSoundEvent(event)) {if(problem)*problem=102;return 0;}
    int ok=battle_sound_channel_snapshot_from(runtime,native,snapshot,problem);
    if(ok && cpu && probe && prepared.valid) {
        probe->offsets[0]=load_le(prepared.offsets,2);probe->offsets[1]=load_le(prepared.offsets+2u,2);
        probe->voices[0]=*prepared.voice0;probe->voices[1]=*prepared.voice1;
        uint8_t *candidates[]={prepared.candidate0,prepared.candidate1};
        for(unsigned i=0;i<2;i++)if(probe->voices[i]<24u) {
            uintptr_t slot=snapshot->slots[probe->voices[i]];
            if(slot) {
                probe->slot_relation[i]=slot==(uintptr_t)candidates[i]?1u:2u;
                probe->slot_priority[i]=(int32_t)(int16_t)load_le((const uint8_t *)slot+4u,2);
            }
        }
        if(prepared.manager_arena) {
            probe->manager_arena_valid=1;
            for(unsigned i=0;i<24u;i++)if(snapshot->slots[i]) {
                int found=0;
                for(unsigned j=0;j<24u;j++)
                    if(snapshot->slots[i]==(uintptr_t)prepared.manager_arena+0x94u+j*0x158u+0x30u) {found=1;break;}
                if(found)probe->manager_slots++;else probe->other_slots++;
            }
        }
        probe->alias_mask=prepared.alias_mask;probe->valid=1;
    }
    if(!PsyX_Sys_ReleaseSoundEventReservation()) {
        fprintf(stderr,"[xeno-port][sound-channels] reservation release failed\n");
        abort();
    }
    return ok;
}

static int battle_sound_channels_unchanged(BattleMipsRuntime *runtime,
                                             const BattleSoundChannelSnapshot *before)
{
    BattleSoundChannelSnapshot after;
    return before && battle_sound_channel_snapshot(runtime,&after) &&
           before->populated==after.populated &&
           memcmp(before->slots,after.slots,sizeof after.slots)==0;
}
