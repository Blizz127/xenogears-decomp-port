/* Experimental guest ABI for event 2, event 5, and out-of-range events only. */
static void battle_attack_ring_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[3]=0x800d0000u;uint32_t event=battle_entry_read(entry,r[3]+0x3014u,1);
    r[29]-=24u;r[3]=event;battle_entry_write(entry,r[29]+20u,4,r[31]);r[2]=event<8u;
    battle_entry_write(entry,r[29]+16u,4,r[16]);
    if(event<8u) {
        r[2]=event<<2;r[1]=0x80070000u+r[2];r[2]=battle_entry_read(entry,r[1]+0x10u,4);
        if(event==2u){r[3]=0x800c0000u;uint32_t context=battle_entry_read(entry,r[3]+0x3eacu,4);r[2]=3;r[3]=context;battle_entry_write(entry,r[3]+0x2ddu,1,r[2]);}
        else if(event!=5u)battle_entry_fail(entry,"unsupported attack-ring event",event);
    }
    uint32_t ra=battle_entry_read(entry,r[29]+20u,4),s0=battle_entry_read(entry,r[29]+16u,4);r[31]=ra;r[29]+=24u;r[16]=s0;
}

/* Attack menu state-1; unconverted events refuse before guest mutation. */
static int battle_attack_ring_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8008115cu || cpu->gpr[31]!=0x80080800u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ATTACK_RING_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const uint32_t addresses[]={0x8008115cu,0x80070010u};const unsigned lengths[]={0x1bcu,32u};
    const char *hashes[]={"c3639f06d1cb7097d5551490f63c8eea7431404779ce59cbcd108ae38238678f","c9be78ab2802b5bc30ad1feede1534b2b4f85e9d4181dc1db277a4adfad9238a"};
    uint8_t *code[2];for(unsigned k=0;k<2;k++){code[k]=resolve_memory(runtime,addresses[k],lengths[k],0);if(!code[k])return 0;uint8_t digest[32];file1_digest(code[k],lengths[k],digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=hashes[k][2*i] || file1_hex_digit(digest[i]&15u)!=hashes[k][2*i+1])return 0;}
    uint32_t sp=cpu->gpr[29],event;if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+24u || runtime_read(runtime,0x800d3014u,1,&event)!=0 || (event!=2u && event!=5u && event<8u))return 0;
    uint8_t *slots[2];for(unsigned i=0;i<2;i++){slots[i]=resolve_memory(runtime,sp-8u+i*4u,4,1);if(!slots[i] || !resolve_memory(runtime,sp-8u+i*4u,4,0))return 0;
        for(unsigned j=0;j<2;j++){uintptr_t x=(uintptr_t)slots[i],y=(uintptr_t)code[j];if(x<y+lengths[j] && y<x+4u)return 0;}}
    if(event==2u){uint32_t context;uint8_t *ptr=resolve_memory(runtime,0x800c3eacu,4,0);if(!ptr || runtime_read(runtime,0x800c3eacu,4,&context)!=0 || context<BATTLE_BUS_KUSEG_RAM_END || context>UINT32_MAX-0x2ddu)return 0;
        for(unsigned i=0;i<2;i++){uintptr_t x=(uintptr_t)slots[i],y=(uintptr_t)ptr;if(x<y+4u && y<x+4u)return 0;}
        uint8_t *out=resolve_memory(runtime,context+0x2ddu,1,1);if(!out)return 0;uintptr_t x=(uintptr_t)out,y=(uintptr_t)code[0];if(x<y+lengths[0] && y<x+1u)return 0;}
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_attack_ring_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
