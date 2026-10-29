/* Retail 800BAF40 is a return with an empty delay slot. */
static int battle_empty_menu_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800baf40u || cpu->gpr[31]!=0x80080a38u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_EMPTY_MENU_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,8u,0);if(!code)return 0;static const char expected[]="6d64edf91449c1b17746c1ef18afa2eb25c70bdf1322ab3df5a2630993b7e2f1";
    uint8_t digest[32];file1_digest(code,8u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
