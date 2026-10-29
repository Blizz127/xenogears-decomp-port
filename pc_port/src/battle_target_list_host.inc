/* Native target-list construction and its eligibility/priority leaves.
 * Candidate/matching bytes remain in the original bus-visible frame. */
#define BATTLE_TARGET_LIST_ADDRESS 0x800841e0u
#define BATTLE_TARGET_ELIGIBILITY_ADDRESS 0x80083ff4u
#define BATTLE_TARGET_PRIORITY_ADDRESS 0x80085310u

#define TL_R8(a) battle_entry_read(entry,(a),1)
#define TL_R16(a) battle_entry_read(entry,(a),2)
#define TL_R32(a) battle_entry_read(entry,(a),4)
#define TL_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define TL_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define TL_CALL(target,site,...) battle_entry_call(entry,(target),(site), \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t),(uint32_t[]){__VA_ARGS__})
#define TL_S0 (entry->cpu->gpr[16])
#define TL_S1 (entry->cpu->gpr[17])
#define TL_S2 (entry->cpu->gpr[18])
#define TL_S3 (entry->cpu->gpr[19])
#define TL_S4 (entry->cpu->gpr[20])
#define TL_V0 (entry->cpu->gpr[2])
#define TL_V1 (entry->cpu->gpr[3])

static void battle_target_eligibility_run(BattleEntryCall *entry)
{
    uint32_t actor=entry->cpu->gpr[4]&0xffu,target=entry->cpu->gpr[5]&0xffu;
    TL_V0=0;
    if (TL_R8(0x800d2dccu+target)==0 || TL_R8(0x800c3eb7u+target*28u)!=0) return;
    if (TL_R8(0x800d32a1u+target*8u)!=0) {
        TL_V0=(TL_R16(0x800cce08u+target*0x170u)&0xc001u)==0;
        return;
    }
    uint32_t target_group=TL_R8(0x800c3eb4u+target*28u);
    uint32_t actor_group=TL_R8(0x800c3eb4u+actor*28u);
    uint32_t base=TL_R32(0x800d3364u);
    TL_V1=target_group*8u+actor_group*64u;
    if (TL_R8(base+TL_V1+0x140u)!=0) return;
    TL_V0=(TL_R16(0x800ccd64u+target*0x170u)&0xc001u)==0;
}

static void battle_target_priority_run(BattleEntryCall *entry)
{
    uint32_t actor=entry->cpu->gpr[4]&0xffu,target=entry->cpu->gpr[5]&0xffu;
    TL_V1=TL_R16(0x800c3ebeu+actor*28u);
    TL_V0=TL_R16(0x800c3ebeu+target*28u)<TL_V1;
}

static void battle_target_list_run(BattleEntryCall *entry)
{
    uint32_t actor=entry->cpu->gpr[4],sp=entry->cpu->gpr[29]-0x50u;
    entry->cpu->gpr[29]=sp;
    TL_W32(sp+0x48u,TL_S4); TL_S4=actor;
    TL_W32(sp+0x38u,TL_S0); TL_S0=0; TL_V1=sp+0x10u;
    TL_W32(sp+0x4cu,entry->cpu->gpr[31]);
    TL_W32(sp+0x44u,TL_S3); TL_W32(sp+0x40u,TL_S2); TL_W32(sp+0x3cu,TL_S1);
    TL_W8(0x800d3274u,0);
    do {
        TL_W8(TL_V1,0xff); TL_W8(0x800c3e90u+TL_S0,0xff);
        TL_W8(sp+0x20u+TL_S0,0); TL_S0++; TL_V1++;
    } while ((int32_t)TL_S0<12);
    unsigned party=(TL_S4&0xffu)<3;
    TL_S0=party ? 3u : 0u; TL_S3=0; TL_S2=sp+0x10u;
    do {
        TL_S1=TL_S0;
        TL_CALL(BATTLE_TARGET_ELIGIBILITY_ADDRESS,party ? 0x80084260u : 0x800842bcu,
                TL_S4&0xffu,TL_S1&0xffu);
        TL_V0&=0xffu; TL_S0++;
        if (TL_V0!=0) {
            TL_W8(TL_S2,TL_S1); TL_S2++;
            TL_V0=TL_R8(0x800d3274u); TL_S3++; TL_V0++;
            TL_W8(0x800d3274u,TL_V0);
        }
    } while ((int32_t)TL_S0<(party ? 11 : 3));

    uint32_t output=0;
    if ((int32_t)TL_S3>0) {
        uint32_t actor_offset=(TL_S4&0xffu)*28u;
        uint32_t candidate=sp+0x10u,end=candidate+TL_S3,matching=sp+0x20u;
        do {
            uint32_t target_offset=TL_R8(candidate)*28u;
            TL_V1=TL_R8(0x800c3eb4u+actor_offset);
            if (TL_V1==TL_R8(0x800c3eb4u+target_offset)) {
                TL_W8(0x800c3e90u+output,TL_R8(candidate));
                TL_W8(candidate,0xff); TL_W8(matching,1); matching++; output++;
            }
            candidate++;
        } while ((int32_t)candidate<(int32_t)end);
    }
    TL_S0=0;
    if ((int32_t)TL_S3>0) {
        TL_V1=sp+0x10u;
        do {
            uint32_t value=TL_R8(TL_V1); TL_S0++;
            if (value!=0xffu) { TL_W8(0x800c3e90u+output,TL_R8(TL_V1)); output++; }
            TL_V1++;
        } while ((int32_t)TL_S0<(int32_t)TL_S3);
    }
    uint32_t restrict_group=TL_R8(sp+0x20u);
    TL_S0=1;
    for (uint32_t i=1;i<TL_S3;i++) {
        if (restrict_group && TL_R8(sp+0x20u+i)==0) continue;
        uint32_t first=TL_R8(0x800c3e90u),current=TL_R8(0x800c3e90u+i);
        TL_V1=TL_R16(0x800ccd34u+first*0x170u);
        uint32_t rank=TL_R16(0x800ccd34u+current*0x170u);
        if (rank<TL_V1) {
            first=TL_R8(0x800c3e90u);
            TL_W8(0x800c3e90u,current); TL_W8(0x800c3e90u+i,first);
        }
    }
    TL_V0=TL_R8(0x800c3e90u);
    entry->cpu->gpr[31]=TL_R32(sp+0x4cu);
    TL_S4=TL_R32(sp+0x48u); TL_S3=TL_R32(sp+0x44u); TL_S2=TL_R32(sp+0x40u);
    TL_S1=TL_R32(sp+0x3cu); TL_S0=TL_R32(sp+0x38u); entry->cpu->gpr[29]=sp+0x50u;
}

#undef TL_R8
#undef TL_R16
#undef TL_R32
#undef TL_W8
#undef TL_W32
#undef TL_CALL
#undef TL_S0
#undef TL_S1
#undef TL_S2
#undef TL_S3
#undef TL_S4
#undef TL_V0
#undef TL_V1

static int battle_target_list_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct { uint32_t address,bytes; const char *identity; void (*run)(BattleEntryCall *); } bodies[]={
        {BATTLE_TARGET_LIST_ADDRESS,872,"8591fbf353f29325387d719127537f0d8b7c2befaadb8c004841791279fdb73d",battle_target_list_run},
        {BATTLE_TARGET_ELIGIBILITY_ADDRESS,276,"1cd53256b57c208c551a4d2b42855760ab72723916768831f980649e455313f8",battle_target_eligibility_run},
        {BATTLE_TARGET_PRIORITY_ADDRESS,64,"61966a108f74d3f207d071dc246ed9198069c6bf42b2e709d0dd1c89c98862f3",battle_target_priority_run}
    };
    unsigned selected;
    for (selected=0;selected<3;selected++) if (bodies[selected].address==target) break;
    if (selected==3 || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime ||
        runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TARGET_LIST");
    if (control && strcmp(control,"0")==0) return 0;
    uint8_t digest[32];
    file1_digest(PSX_ADDR(target),bodies[selected].bytes,digest);
    const char *expected=bodies[selected].identity;
    for (unsigned i=0;i<sizeof(digest);i++)
        if (file1_hex_digit(digest[i]>>4)!=expected[i*2] ||
            file1_hex_digit(digest[i]&15u)!=expected[i*2+1]) return 0;
    void (*volatile run)(BattleEntryCall *)=bodies[selected].run;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if (setjmp(entry.failure)!=0) return -1;
    run(&entry); cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
}
