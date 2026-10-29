/* Experimental 3B644: only a finite, nonmatching SED list ending in NULL. */
static void battle_sound_missing_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[29]-=80u;battle_entry_write(entry,r[29]+48u,4,r[18]);r[18]=0x80060000u;
    uint32_t list=battle_entry_read(entry,r[18]+0xffff9440u,4);battle_entry_write(entry,r[29]+72u,4,r[30]);r[18]=list;r[30]=r[6];
    battle_entry_write(entry,r[29]+40u,4,r[16]);battle_entry_write(entry,r[29]+16u,4,r[5]);r[3]=(uint32_t)((int32_t)r[5]>>16);
    battle_entry_write(entry,r[29]+76u,4,r[31]);battle_entry_write(entry,r[29]+68u,4,r[23]);battle_entry_write(entry,r[29]+64u,4,r[22]);battle_entry_write(entry,r[29]+60u,4,r[21]);battle_entry_write(entry,r[29]+56u,4,r[20]);battle_entry_write(entry,r[29]+52u,4,r[19]);battle_entry_write(entry,r[29]+44u,4,r[17]);battle_entry_write(entry,r[29]+24u,2,r[7]);
    uint32_t id=battle_entry_read(entry,r[18]+20u,2);r[20]=0x80060000u;uint32_t manager=battle_entry_read(entry,r[20]+0xffff95d8u,4);r[2]=id;r[16]=r[4];r[20]=manager;
    if(r[2]==r[3])battle_entry_fail(entry,"matching SED unsupported",r[18]);
    for(unsigned i=0;i<8;i++){uint32_t next=battle_entry_read(entry,r[18]+28u,4);r[18]=next;if(!r[18])goto cleanup;r[2]=battle_entry_read(entry,r[18]+20u,2);if(r[2]==r[3])battle_entry_fail(entry,"matching SED unsupported",r[18]);}
    battle_entry_fail(entry,"SED traversal limit",r[18]);
cleanup:;
    const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};const unsigned offsets[]={76,72,68,64,60,56,52,48,44,40};
    uint32_t prior=battle_entry_read(entry,r[29]+offsets[0],4);for(unsigned i=1;i<10;i++){uint32_t value=battle_entry_read(entry,r[29]+offsets[i],4);r[regs[i-1]]=prior;prior=value;}r[29]+=80u;r[16]=prior;
}

/* Bounded, nonmatching SED list only: a matched or cyclic list always declines. */
static int battle_sound_missing_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8003b644u || cpu->gpr[31]!=0x80039e08u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_SOUND_MISSING_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+0x2ecu)return 0;
    static const char expected[]="7434935d43c77794781ea582813b8d7a83f92780d84657484e27abb17a02f901";
    uint8_t digest[32];file1_digest(image->bytes+offset,0x2ecu,digest);
    for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t sp=cpu->gpr[29];if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+80u)return 0;
    const unsigned offsets[]={16,24,40,44,48,52,56,60,64,68,72,76};uint8_t *saved[12];unsigned widths[12];
    for(unsigned i=0;i<12;i++){widths[i]=i==1?2:4;saved[i]=resolve_memory(runtime,sp-80u+offsets[i],widths[i],1);if(!saved[i])return 0;if(i>=2 && !resolve_memory(runtime,sp-80u+offsets[i],4,0))return 0;}
    uint8_t *code=resolve_memory(runtime,target,0x2ecu,0);if(!code)return 0;
    for(unsigned i=0;i<12;i++){uintptr_t a=(uintptr_t)saved[i],b=(uintptr_t)code;if(a<b+0x2ecu && b<a+widths[i])return 0;}
    uint8_t *globals[]={resolve_memory(runtime,0x80059440u,4,0),resolve_memory(runtime,0x800595d8u,4,0)};
    for(unsigned j=0;j<2;j++){if(!globals[j])return 0;for(unsigned i=0;i<12;i++){uintptr_t a=(uintptr_t)saved[i],b=(uintptr_t)globals[j];if(a<b+4u && b<a+widths[i])return 0;}}
    uint32_t node,manager;if(runtime_read(runtime,0x80059440u,4,&node)!=0 || runtime_read(runtime,0x800595d8u,4,&manager)!=0)return 0;(void)manager;
    uint32_t wanted=(uint32_t)((int32_t)cpu->gpr[5]>>16);int ended=0;
    for(unsigned n=0;n<8;n++) {if(node<BATTLE_BUS_KUSEG_RAM_END || (node&3u) || node>UINT32_MAX-28u)return 0;
        uint8_t *id=resolve_memory(runtime,node+20u,2,0),*next=resolve_memory(runtime,node+28u,4,0);if(!id || !next)return 0;
        for(unsigned i=0;i<12;i++){uintptr_t a=(uintptr_t)saved[i],b=(uintptr_t)id,c=(uintptr_t)next;if((a<b+2u && b<a+widths[i]) || (a<c+4u && c<a+widths[i]))return 0;}
        uint32_t value;if(runtime_read(runtime,node+20u,2,&value)!=0 || value==wanted || runtime_read(runtime,node+28u,4,&node)!=0)return 0;
        if(node==0){ended=1;break;}}
    if(!ended)return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_sound_missing_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
