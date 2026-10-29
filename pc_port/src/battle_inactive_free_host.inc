/* Experimental inactive resource cleanup: active heap paths refuse before mutation. */
static void battle_inactive_free_run(BattleEntryCall *entry,uint32_t target)
{
    uint32_t *r=entry->cpu->gpr;r[2]=0x800d0000u;uint32_t frame=battle_entry_read(entry,r[2]+0x2d28u,4);r[29]-=24u;r[2]=frame;
    battle_entry_write(entry,r[29]+16u,4,r[31]);r[2]=battle_entry_read(entry,r[2]+(target==0x8007fce8u?0xaeu:0x96u),1);
    if(r[2])battle_entry_fail(entry,"active heap cleanup unsupported",target);
    uint32_t ra=battle_entry_read(entry,r[29]+16u,4);r[29]+=24u;r[31]=ra;
}

/* Only inactive paths; active resource owners remain separately guarded. */
static int battle_inactive_free_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    uint32_t ra=cpu->gpr[31],flag_offset;const char *expected;
    if(target==0x8007fce8u && ra==0x80080a70u){flag_offset=0xaeu;expected="22bd6f08aa26bfe4b7f2c83f48aa088f4f4baae3f36b24875199ee50eb6e20ad";}
    else if(target==0x8007fdecu && ra==0x80080a78u){flag_offset=0x96u;expected="d0762ea8344857c97be6c81789d46b194e6b8736e129c7bc82bc6133970ae568";}
    else return 0;
    if(runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_INACTIVE_FREE_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,80u,0);if(!code)return 0;uint8_t digest[32];file1_digest(code,80u,digest);
    for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t sp=cpu->gpr[29],frame,flag;if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+24u || runtime_read(runtime,0x800d2d28u,4,&frame)!=0 || frame<BATTLE_BUS_KUSEG_RAM_END || frame>UINT32_MAX-flag_offset)return 0;
    uint8_t *slot=resolve_memory(runtime,sp-8u,4,1),*flagptr=resolve_memory(runtime,frame+flag_offset,1,0);if(!slot || !resolve_memory(runtime,sp-8u,4,0) || !flagptr || runtime_read(runtime,frame+flag_offset,1,&flag)!=0 || flag)return 0;
    uintptr_t a=(uintptr_t)slot,b=(uintptr_t)code,c=(uintptr_t)flagptr;if((a<b+80u && b<a+4u)||(a<c+1u && c<a+4u))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_inactive_free_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
