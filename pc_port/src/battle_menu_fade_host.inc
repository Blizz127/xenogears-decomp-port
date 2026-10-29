/* Scratch fade-render prefix: child is separately refused. */
static void battle_menu_fade_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[29]-=112u;r[6]&=0xffu;const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};
    for(unsigned i=0;i<10;i++)battle_entry_write(entry,r[29]+108u-i*4u,4,r[regs[i]]);
    if(!r[6])battle_entry_fail(entry,"non-fade menu render unsupported",0x8008d598u);
    r[31]=0x8008d5d4u;battle_phase_call(entry,0x8008d328u,0x8008d5ccu);
    battle_entry_fail(entry,"fade render child returned without continuation",0x8008d328u);
}

/* Fade branch only; child is guarded before any rendering. */
static int battle_menu_fade_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || !(cpu->gpr[6]&0xffu) || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MENU_FADE_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    uint8_t *code=resolve_memory(runtime,target,60u,0);if(!code)return 0;static const char expected[]="86bcabb2df9b13eaaa0e42ec8255a3979db61613686c0b942e5899246a63d684";
    uint8_t digest[32];file1_digest(code,60u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t sp=cpu->gpr[29];if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+112u)return 0;
    for(unsigned i=0;i<10;i++){uint8_t *slot=resolve_memory(runtime,sp-4u-i*4u,4,1);if(!slot)return 0;uintptr_t a=(uintptr_t)slot,b=(uintptr_t)code;if(a<b+60u && b<a+4u)return 0;}
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_menu_fade_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
