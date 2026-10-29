/* Experimental menu state cleanup: active callback path remains refused. */
static void battle_inactive_state_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[29]-=24u;r[4]&=0xffu;battle_entry_write(entry,r[29]+16u,4,r[31]);r[1]=0x800d0000u+r[4];
    uint32_t state=battle_entry_read(entry,r[1]+0x2d24u,1);r[2]=4;r[3]=state;
    if(r[3]==r[2]){r[2]=0x800d0000u;r[2]=battle_entry_read(entry,r[2]+0x2d28u,4);r[2]=battle_entry_read(entry,r[2]+0x8eu,1);if(r[2])battle_entry_fail(entry,"active state callback unsupported",0x8007fb70u);}
    uint32_t ra=battle_entry_read(entry,r[29]+16u,4);r[29]+=24u;r[31]=ra;
}

/* Only branches which skip the active state callbacks. */
static int battle_inactive_state_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8007fb70u || cpu->gpr[31]!=0x80080a88u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_INACTIVE_STATE_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,112u,0);if(!code)return 0;static const char expected[]="f0bd7910a1125962b02d20908ea74131a17fdc75e4921d8325e6889d794a8e52";
    uint8_t digest[32];file1_digest(code,112u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t sp=cpu->gpr[29],state,address=0x800d2d24u+(cpu->gpr[4]&0xffu);if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+24u)return 0;
    uint8_t *slot=resolve_memory(runtime,sp-8u,4,1),*stateptr=resolve_memory(runtime,address,1,0);if(!slot || !resolve_memory(runtime,sp-8u,4,0) || !stateptr || runtime_read(runtime,address,1,&state)!=0)return 0;
    uintptr_t a=(uintptr_t)slot,b=(uintptr_t)code,c=(uintptr_t)stateptr;if((a<b+112u && b<a+4u)||(a<c+1u && c<a+4u))return 0;
    if(state==4u){uint8_t *fp=resolve_memory(runtime,0x800d2d28u,4,0);uint32_t frame,flag;if(!fp)return 0;c=(uintptr_t)fp;if(a<c+4u && c<a+4u)return 0;
        if(runtime_read(runtime,0x800d2d28u,4,&frame)!=0 || frame<BATTLE_BUS_KUSEG_RAM_END || frame>UINT32_MAX-0x8eu)return 0;
        uint8_t *f=resolve_memory(runtime,frame+0x8eu,1,0);if(!f || runtime_read(runtime,frame+0x8eu,1,&flag)!=0 || flag)return 0;c=(uintptr_t)f;if(a<c+1u && c<a+4u)return 0;}
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_inactive_state_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
