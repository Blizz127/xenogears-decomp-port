/* Experimental allocation prefix: allocator must refuse until separately converted. */
static void battle_menu_alloc_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[29]-=24u;r[4]=0x5da4u;battle_entry_write(entry,r[29]+16u,4,r[31]);
    r[31]=0x8007fed8u;r[5]=0;battle_phase_call(entry,0x8008abb8u,0x8007fed0u);
    battle_entry_fail(entry,"menu allocator returned without native continuation",0x8008abb8u);
}

/* The first allocation edge only; an actual allocator remains unavailable. */
static int battle_menu_alloc_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8007fec4u || cpu->gpr[31]!=0x800805acu || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MENU_ALLOC_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,80u,0);if(!code)return 0;static const char expected[]="3b63a7f610a4150a620c040135e1dcb78ccf5beb4874bdf89299ad26d038ff8c";
    uint8_t digest[32];file1_digest(code,80u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t sp=cpu->gpr[29];if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+24u)return 0;
    uint8_t *slot=resolve_memory(runtime,sp-8u,4,1);if(!slot)return 0;uintptr_t a=(uintptr_t)slot,b=(uintptr_t)code;if(a<b+80u && b<a+4u)return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_menu_alloc_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
