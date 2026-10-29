/* Experimental cursor layout for counts zero or one; no cell loop. */
static void battle_small_cursor_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[8]=r[5];r[4]&=0xffu;r[2]=r[8]&0xffu;int unequal=r[4]!=r[2];r[29]-=8u;if(!unequal)r[8]=r[5]-1u;
    r[2]=r[4]-1u;r[6]=0;if((int32_t)r[2]>0)battle_entry_fail(entry,"cursor cell loop unsupported",r[4]);
    r[2]=0x800d0000u;r[2]=battle_entry_read(entry,r[2]+0x2d28u,4);battle_entry_write(entry,r[2]+0x97u,1,r[8]);
    r[3]=0x800d0000u;uint32_t frame=battle_entry_read(entry,r[3]+0x2d28u,4);r[2]=0x800d0000u;r[3]=frame;r[2]=battle_entry_read(entry,r[2]+0xffffcb34u,1);
    battle_entry_write(entry,r[3]+0x98u,1,r[2]);r[29]+=8u;
}

/* Only zero/one-option cursor layout at the command-menu edge. */
static int battle_small_cursor_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8007fbe0u || cpu->gpr[31]!=0x800806f0u || (cpu->gpr[4]&0xffu)>1u || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_SMALL_CURSOR_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,264u,0);if(!code)return 0;static const char expected[]="617d0424dd52399ef39d24a3934b662acb61c2b440844fc1f3542fa60fff8c5e";
    uint8_t digest[32];file1_digest(code,264u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t frame;uint8_t *ptr=resolve_memory(runtime,0x800d2d28u,4,0),*slot=resolve_memory(runtime,0x800ccb34u,1,0);
    if(!ptr || !slot || runtime_read(runtime,0x800d2d28u,4,&frame)!=0 || frame<BATTLE_BUS_KUSEG_RAM_END || frame>UINT32_MAX-0x98u)return 0;
    uint8_t *a=resolve_memory(runtime,frame+0x97u,1,1),*b=resolve_memory(runtime,frame+0x98u,1,1);if(!a || !b)return 0;
    uintptr_t x=(uintptr_t)a,y=(uintptr_t)b,c=(uintptr_t)code,p=(uintptr_t)ptr,z=(uintptr_t)slot;
    if((x<c+264u && c<x+1u)||(y<c+264u && c<y+1u)||(x<p+4u && p<x+1u)||(y<p+4u && p<y+1u)||(x<z+1u && z<x+1u))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_small_cursor_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
