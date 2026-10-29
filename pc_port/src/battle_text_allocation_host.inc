/* Text-buffer allocation wrapper (retail 8008AC00). Native heap services own
 * allocation policy; this body preserves the original frame and word arithmetic. */
#define BATTLE_TEXT_ALLOCATION_ADDRESS 0x8008ac00u

static void battle_text_allocation_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x18u;
    entry->cpu->gpr[29]=sp;
    battle_entry_write(entry,sp+0x10u,4,entry->cpu->gpr[16]);
    entry->cpu->gpr[16]=entry->cpu->gpr[4];
    battle_entry_write(entry,sp+0x14u,4,entry->cpu->gpr[31]);
    const uint32_t user_args[]={2,0};
    battle_entry_call(entry,0x80032498u,0x8008ac14u,2,user_args);
    entry->cpu->gpr[16]+=3u;
    const uint32_t alloc_args[]={entry->cpu->gpr[16]*26u,0};
    battle_entry_call(entry,0x80031bdcu,0x8008ac34u,2,alloc_args);
    entry->cpu->gpr[31]=battle_entry_read(entry,sp+0x14u,4);
    entry->cpu->gpr[16]=battle_entry_read(entry,sp+0x10u,4);
    entry->cpu->gpr[29]=sp+0x18u;
}

static int battle_text_allocation_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=BATTLE_TEXT_ALLOCATION_ADDRESS || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TEXT_ALLOCATION");
    if(control && strcmp(control,"0")==0) return 0;
    static const char identity[]="d7e00854f0396dc7292ac671de24946698c57da9d5a2c44d69c5dffabbd8784d";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),80,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=identity[i*2] ||
           file1_hex_digit(digest[i]&15u)!=identity[i*2+1]) return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0) return -1;
    battle_text_allocation_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
