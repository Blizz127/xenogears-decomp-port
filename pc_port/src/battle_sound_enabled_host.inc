/* Experimental guest ABI for enabled SFX wrapper; audio service separately guarded. */
/* A missing exact binding must not silently read an old guest-RAM count. */
static int battle_sound_count_bound(BattleMipsRuntime *runtime)
{
    ResolvedData *entry = find_data(runtime, 0x80059478u, 4u);
    uint8_t *host = dlsym(RTLD_DEFAULT, "D_80059478");
    return host && entry && entry->address == 0x80059478u &&
           entry->size == 4u && entry->host == host && entry->name &&
           strcmp(entry->name, "D_80059478") == 0;
}
static void battle_sound_enabled_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[29]-=24u;r[2]=0x80060000u;uint32_t flags=battle_entry_read(entry,r[2]+0xffff957cu,2);r[5]=r[4];r[2]=flags&0x800u;
    battle_entry_write(entry,r[29]+16u,4,r[31]);if(!r[2])battle_entry_fail(entry,"sound disabled on enabled path",flags);
    r[2]=2;r[1]=0x80060000u;battle_entry_write(entry,r[1]+0xffff9404u,4,r[2]);r[2]=0xffff8000u;r[6]=0x6000u;r[4]=0x80060000u;
    uint32_t slot=battle_entry_read(entry,r[4]+0xffff9478u,4);
    if (slot < 2u || slot > 24u || (slot & 1u))
        battle_entry_fail(entry,"invalid sound element count",slot);
    r[7]=0x4000u;r[4]=slot-2u;r[4]|=r[2];r[4]=(uint32_t)(int32_t)(int16_t)r[4];
    battle_phase_call(entry,0x8003b644u,0x80039e00u);uint32_t ra=battle_entry_read(entry,r[29]+16u,4);r[29]+=24u;r[31]=ra;
}

/* Enabled wrapper only; actual sound service remains independently refused. */
static int battle_sound_enabled_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x80039db8u || cpu->gpr[31]!=0x8008aa64u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_SOUND_ENABLED_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+96u)return 0;
    static const char expected[]="491128d2006c81928253d1cd460dbad281ce6215187649f37dcca0bcf49fd53b";
    uint8_t digest[32];file1_digest(image->bytes+offset,96u,digest);
    for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t flags,count,sp=cpu->gpr[29];if(!battle_sound_count_bound(runtime) || (sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+24u || runtime_read(runtime,0x8005957cu,2,&flags)!=0 || !(flags&0x800u) || !resolve_memory(runtime,sp-8u,4,1) || !resolve_memory(runtime,sp-8u,4,0) || !resolve_memory(runtime,0x80059404u,4,1) || runtime_read(runtime,0x80059478u,4,&count)!=0 || count<2u || count>24u || (count&1u))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_sound_enabled_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
