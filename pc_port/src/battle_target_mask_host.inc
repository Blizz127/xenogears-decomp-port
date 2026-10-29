/* Experimental guest ABI for 80089C08 target-mask lookup. */
static void battle_target_mask_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[4]=(r[4]&0xffu)<<1;r[1]=0x800c0000u+r[4];r[2]=battle_entry_read(entry,r[1]+0x3448u,2);
}

/* Three command-menu target-mask edges only. */
static int battle_target_mask_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    uint32_t ra=cpu->gpr[31];if(target!=0x80089c08u || (ra!=0x8008061cu && ra!=0x80080638u && ra!=0x80080674u) ||
       runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TARGET_MASK_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,28u,0);if(!code)return 0;static const char expected[]="74c04258a3cf89081e9e82cef4ed802aff2c03b830a9fc7bb204291532cb45c0";
    uint8_t digest[32];file1_digest(code,28u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    if(!resolve_memory(runtime,0x800c3448u+((cpu->gpr[4]&0xffu)<<1),2,0))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_target_mask_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
