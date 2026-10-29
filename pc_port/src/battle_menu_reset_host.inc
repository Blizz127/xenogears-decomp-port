/* Experimental menu selection reset in retail guest register and store order. */
static void battle_menu_reset_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[3]=0;r[4]=0xffu;
    for(unsigned i=0;i<7;i++){r[2]=0x800c0000u;r[2]=battle_entry_read(entry,r[2]+0x3eacu,4);r[2]+=r[3];r[3]++;battle_entry_write(entry,r[2]+0x2ccu,1,r[4]);r[2]=(int32_t)r[3]<7;}
    r[2]=0x800c0000u;r[2]=battle_entry_read(entry,r[2]+0x3eacu,4);battle_entry_write(entry,r[2]+0x2d6u,1,0);
}

/* Only the command-menu selection reset edge. */
static int battle_menu_reset_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x80085e78u || cpu->gpr[31]!=0x8008060cu || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MENU_RESET_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,60u,0);if(!code)return 0;
    static const char expected[]="1f7888a06170acb1c9fb27b3713e49eb4c740baf22b64ca4a49533f5e2cb684c";
    uint8_t digest[32];file1_digest(code,60u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t base;uint8_t *ptr=resolve_memory(runtime,0x800c3eacu,4,0);if(!ptr || runtime_read(runtime,0x800c3eacu,4,&base)!=0 || base<BATTLE_BUS_KUSEG_RAM_END || base>UINT32_MAX-0x2d6u)return 0;
    for(unsigned i=0;i<8;i++){uint32_t at=base+(i<7?0x2ccu+i:0x2d6u);uint8_t *out=resolve_memory(runtime,at,1,1);if(!out)return 0;uintptr_t a=(uintptr_t)out,b=(uintptr_t)code,c=(uintptr_t)ptr;if((a<b+60u && b<a+1u)||(a<c+4u && c<a+1u))return 0;}
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_menu_reset_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
