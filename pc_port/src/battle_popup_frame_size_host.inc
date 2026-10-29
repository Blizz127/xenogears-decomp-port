/* Read-only popup frame-size leaf, scoped to the highlight constructor. */
static void battle_popup_frame_size_run(BattleEntryCall *entry,const uint8_t *data,
                                        const uint8_t *snapshot)
{
    uint32_t *r=entry->cpu->gpr,base=r[4],pos=40u;
    if(memcmp(data,snapshot,0x170u)!=0)battle_entry_fail(entry,"popup frame changed",base);
    r[5]=0;r[6]=r[5];
    r[2]=battle_entry_read(entry,r[4]+0x10u,4);
    r[7]=battle_entry_read(entry,r[4]+0x14u,4);
    if(r[2]!=28u || r[7]!=8u)battle_entry_fail(entry,"popup frame header changed",base);
    r[4]+=r[2];
    for(unsigned n=0;n<8u;n++) {
        if(pos>0x170u-2u || r[4]!=base+pos-12u ||
           resolve_memory(entry->runtime,r[4],2u,0)!=data+pos)
            battle_entry_fail(entry,"popup frame span changed",r[4]);
        r[5]++;
        r[3]=battle_entry_read(entry,r[4],1);
        r[2]=battle_entry_read(entry,r[4]+1u,1);
        if(r[3]!=snapshot[pos] || r[2]!=snapshot[pos+1u])
            battle_entry_fail(entry,"popup frame record changed",r[4]);
        r[3]=(r[3]+1u)<<2;r[6]+=r[3];
        r[2]=(r[2]+1u)<<2;
        if(r[2]>0x170u-pos)battle_entry_fail(entry,"popup frame stride changed",r[2]);
        pos+=r[2];r[4]+=r[2];
    }
    r[2]=r[6];
}

static int battle_popup_frame_size_try_host(BattleMipsRuntime *runtime,
                                             PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800b16a4u || cpu->gpr[31]!=0x800bd158u || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_FRAME_SIZE_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,0x4cu,0);
    uint8_t *data=dlsym(RTLD_DEFAULT,"D_8001C76C");
    ResolvedData *binding=find_data(runtime,0x8001c76cu,0x170u);
    if(!code || !data || (uintptr_t)data>UINT32_MAX-0x170u || !binding ||
       binding->address!=0x8001c76cu || binding->host!=data || binding->size!=0x170u ||
       resolve_memory(runtime,cpu->gpr[4],0x1cu,0)!=data+12u)return 0;
    static const char code_hash[]="407016beda83459888ae2a063ef1e73348d74f61f2b811a5917a74b70ace64c2";
    static const char data_hash[]="831bfa067137aed3fed17327ed7738e768e499dca076162f99c67c02dac5d7b3";
    uint8_t digest[32];file1_digest(code,0x4cu,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=code_hash[2*i] || file1_hex_digit(digest[i]&15u)!=code_hash[2*i+1])return 0;
    uint8_t snapshot[0x170u];memcpy(snapshot,data,sizeof snapshot);
    file1_digest(snapshot,sizeof snapshot,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=data_hash[2*i] || file1_hex_digit(digest[i]&15u)!=data_hash[2*i+1])return 0;
    uint32_t offset=load_le(snapshot+28u,4),count=load_le(snapshot+32u,4),pos;
    if(offset!=28u || count!=8u)return 0;
    pos=12u+offset;
    for(unsigned i=0;i<count;i++) {
        if(pos>0x170u-2u)return 0;
        uint32_t stride=((uint32_t)snapshot[pos+1u]+1u)*4u;
        if(stride>0x170u-pos)return 0;
        pos+=stride;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_popup_frame_size_run(&entry,data,snapshot);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
