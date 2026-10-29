/* Popup highlight task creation, only through the allocation boundary. */
static void battle_popup_create_prefix_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    r[29]-=56u;r[5]=r[4];r[4]=0x68u;
    r[6]=0x800bcfacu;r[7]=0x800bceacu;r[2]=0x800c0000u;
    battle_entry_write(entry,r[29]+48u,4,r[31]);
    battle_entry_write(entry,r[29]+44u,4,r[21]);
    battle_entry_write(entry,r[29]+40u,4,r[20]);
    battle_entry_write(entry,r[29]+36u,4,r[19]);
    battle_entry_write(entry,r[29]+32u,4,r[18]);
    battle_entry_write(entry,r[29]+28u,4,r[17]);
    battle_entry_write(entry,r[29]+24u,4,r[16]);
    r[21]=battle_entry_read(entry,r[5]+4u,4);
    r[2]+=0xffffd024u;
    battle_entry_write(entry,r[29]+16u,4,r[2]);
    battle_phase_call(entry,0x8001d1d8u,0x800bd0dcu);
    battle_entry_fail(entry,"popup paired allocation returned without continuation",0x8001d1d8u);
}

static int battle_popup_create_prefix_try_host(BattleMipsRuntime *runtime,
                                                PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800bd098u || cpu->gpr[31]!=0x800bce2cu || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_CREATE_PREFIX_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,0x164u,0);
    if(!code)return 0;
    static const char expected[]="7be44418476d349fbc96a6af577a7adc98db37c6894f01809013ac23d5b22616";
    uint8_t digest[32];file1_digest(code,0x164u,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
           file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t sp=cpu->gpr[29],owner=cpu->gpr[4];
    if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+56u || (owner&3u) ||
       owner<BATTLE_BUS_KUSEG_RAM_END || owner>UINT32_MAX-7u)return 0;
    uint8_t *source=resolve_memory(runtime,owner+4u,4u,0);
    if(!source)return 0;
    const unsigned offsets[]={16,24,28,32,36,40,44,48};
    for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++) {
        uint8_t *saved=resolve_memory(runtime,sp-56u+offsets[i],4u,1);
        if(!saved)return 0;
        uintptr_t a=(uintptr_t)saved,b=(uintptr_t)code,c=(uintptr_t)source;
        if((a<b+0x164u && b<a+4u) || (a<c+4u && c<a+4u))return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_popup_create_prefix_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
