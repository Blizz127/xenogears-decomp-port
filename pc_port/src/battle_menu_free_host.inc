/* Experimental menu teardown through the guarded heap-free edge. */
static void battle_menu_free_run(BattleEntryCall *entry,uint32_t expected_frame)
{
    uint32_t *r=entry->cpu->gpr;r[2]=0x800d0000u;uint32_t frame=battle_entry_read(entry,r[2]+0x2d28u,4);r[29]-=24u;r[2]=frame;if(frame!=expected_frame)battle_entry_fail(entry,"menu frame changed",frame);battle_entry_write(entry,r[29]+16u,4,r[31]);
    battle_entry_write(entry,r[2]+0xadu,1,0);r[2]=0x800d0000u;frame=battle_entry_read(entry,r[2]+0x2d28u,4);r[4]&=0xffu;r[2]=frame;if(frame!=expected_frame)battle_entry_fail(entry,"menu frame changed",frame);battle_entry_write(entry,r[2]+0xc7u,1,0);
    r[2]=0x800d0000u;frame=battle_entry_read(entry,r[2]+0x2d28u,4);r[4]<<=3;r[2]=frame;if(frame!=expected_frame)battle_entry_fail(entry,"menu frame changed",frame);battle_entry_write(entry,r[2]+0xa8u,1,0);
    r[1]=0x800d0000u+r[4];uint32_t state=battle_entry_read(entry,r[1]+0x32a1u,1);r[2]=2;r[3]=state;int different=r[3]!=r[2];r[2]=1;
    if(!different){r[1]=0x800d0000u+r[4];battle_entry_write(entry,r[1]+0x32a1u,1,r[2]);}
    r[4]=0x800d0000u;uint32_t heap=battle_entry_read(entry,r[4]+0x2db4u,4);r[31]=0x80080150u;r[4]=heap;battle_phase_call(entry,0x800320e8u,0x80080148u);
    battle_entry_fail(entry,"menu teardown child returned without native continuation",0x800320e8u);
}

/* Teardown only through a guarded, unconverted heap-free edge. */
static int battle_menu_free_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800800e8u || cpu->gpr[31]!=0x80080a80u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MENU_FREE_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    uint8_t *code=resolve_memory(runtime,target,120u,0);if(!code)return 0;static const char expected[]="8c8b120d9839d2e071b65a86f7e3d011bf9a2ba1c02ae7ee6dd47e1a1bc2913b";
    uint8_t digest[32];file1_digest(code,120u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t sp=cpu->gpr[29],frame,state_addr=0x800d32a1u+((cpu->gpr[4]&0xffu)<<3);if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+24u)return 0;
    uint8_t *ptr=resolve_memory(runtime,0x800d2d28u,4,0),*heap=resolve_memory(runtime,0x800d2db4u,4,0),*state=resolve_memory(runtime,state_addr,1,0),*slot=resolve_memory(runtime,sp-8u,4,1);
    uint8_t *state_write=resolve_memory(runtime,state_addr,1,1);
    if(!ptr || !heap || !state || !state_write || !slot || runtime_read(runtime,0x800d2d28u,4,&frame)!=0 || frame<BATTLE_BUS_KUSEG_RAM_END || frame>UINT32_MAX-0xc7u)return 0;
    const uint32_t offsets[]={0xadu,0xc7u,0xa8u};uint8_t *out[3];for(unsigned i=0;i<3;i++){out[i]=resolve_memory(runtime,frame+offsets[i],1,1);if(!out[i])return 0;}
    uint8_t *writes[]={slot,out[0],out[1],out[2],state_write};unsigned widths[]={4,1,1,1,1};uint8_t *protected[]={code,ptr,heap};unsigned sizes[]={120,4,4};
    for(unsigned i=0;i<5;i++){uintptr_t a=(uintptr_t)writes[i];for(unsigned j=0;j<3;j++){uintptr_t b=(uintptr_t)protected[j];if(a<b+sizes[j] && b<a+widths[i])return 0;}}
    for(unsigned i=0;i<4;i++){uintptr_t a=(uintptr_t)writes[i],b=(uintptr_t)state;if(a<b+1u && b<a+widths[i])return 0;}
    for(unsigned i=1;i<4;i++){uintptr_t a=(uintptr_t)slot,b=(uintptr_t)out[i-1];if(a<b+1u && b<a+4u)return 0;}
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_menu_free_run(&entry,frame);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
