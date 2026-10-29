/* Experimental 8009AB00 actor flag clear in the retail guest ABI. */
static void battle_actor_clear_flag_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[4]&=0xffu;r[2]=r[4]<<1;r[2]+=r[4];r[2]<<=3;r[2]-=r[4];r[3]=0x800c0000u;
    uint32_t base=battle_entry_read(entry,r[3]+0x34b0u,4);r[2]<<=4;r[3]=base+r[2];r[2]=battle_entry_read(entry,r[3]+0x15au,1);r[2]&=0xfeu;battle_entry_write(entry,r[3]+0x15au,1,r[2]);
}

/* Only the command-menu actor flag edge. */
static int battle_actor_clear_flag_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8009ab00u || cpu->gpr[31]!=0x80080614u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ACTOR_CLEAR_FLAG_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,56u,0);if(!code)return 0;
    static const char expected[]="ebdf10aa44492923434bc45f4ac36e4afbcc915fc139661817e1ff519273a14a";
    uint8_t digest[32];file1_digest(code,56u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t base,offset=(cpu->gpr[4]&0xffu)*0x170u+0x15au;if(runtime_read(runtime,0x800c34b0u,4,&base)!=0 || base<BATTLE_BUS_KUSEG_RAM_END || base>UINT32_MAX-offset)return 0;
    uint8_t *out=resolve_memory(runtime,base+offset,1,1);if(!out || !resolve_memory(runtime,base+offset,1,0))return 0;
    uintptr_t a=(uintptr_t)out,b=(uintptr_t)code;if(a<b+56u && b<a+1u)return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_actor_clear_flag_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
