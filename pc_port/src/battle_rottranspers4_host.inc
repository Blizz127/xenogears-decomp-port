/* Experimental guest ABI projection using PsyCross GTE instructions. */
static void battle_rottranspers4_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    for(unsigned i=0;i<3;i++) {MTC2(battle_entry_read(entry,r[4+i],4),i*2);MTC2(battle_entry_read(entry,r[4+i]+4u,4),i*2+1);}
    doCOP2(0x4a280030);
    uint32_t a=battle_entry_read(entry,r[29]+16u,4),b=battle_entry_read(entry,r[29]+20u,4);r[8]=a;
    uint32_t c=battle_entry_read(entry,r[29]+24u,4);r[9]=b;
    battle_entry_write(entry,r[8],4,MFC2(12));r[10]=c;battle_entry_write(entry,r[9],4,MFC2(13));battle_entry_write(entry,r[10],4,MFC2(14));
    uint32_t flags=CFC2(31);MTC2(battle_entry_read(entry,r[7],4),0);r[3]=flags;MTC2(battle_entry_read(entry,r[7]+4u,4),1);doCOP2(0x4a180001);
    a=battle_entry_read(entry,r[29]+28u,4);b=battle_entry_read(entry,r[29]+32u,4);r[8]=a;c=battle_entry_read(entry,r[29]+36u,4);r[9]=b;
    battle_entry_write(entry,r[8],4,MFC2(14));r[10]=c;battle_entry_write(entry,r[9],4,MFC2(8));
    uint32_t fourth=CFC2(31),depth=MFC2(19);r[8]=fourth;r[8]|=r[3];r[2]=depth;battle_entry_write(entry,r[10],4,r[8]);r[2]=(uint32_t)((int32_t)r[2]>>2);
}

/* Atlas projection only; reject output/input or output/argument-stack alias. */
static int battle_rottranspers4_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8004a73cu || cpu->gpr[31]!=0x80026210u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ROTTRANSPERS4_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+120u)return 0;
    static const char expected[]="7aabdde0f4e769f401038cd12dfc6daed66e77372ee145f066b22b7b3edc97eb";
    uint8_t digest[32];file1_digest(image->bytes+offset,120u,digest);
    for(unsigned i=0;i<sizeof(digest);++i)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint8_t *inputs[8],*args[6],*outputs[6];uint32_t sp=cpu->gpr[29];
    if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END || sp>UINT32_MAX-36u)return 0;
    for(unsigned i=0;i<4;i++) {uint32_t p=cpu->gpr[4+i];if((p&3u) || p<BATTLE_BUS_KUSEG_RAM_END || p>UINT32_MAX-4u)return 0;
        for(unsigned j=0;j<2;j++){inputs[i*2+j]=resolve_memory(runtime,p+j*4u,4,0);if(!inputs[i*2+j])return 0;}}
    for(unsigned i=0;i<6;i++) {uint32_t out;args[i]=resolve_memory(runtime,sp+16u+i*4u,4,0);
        if(!args[i] || runtime_read(runtime,sp+16u+i*4u,4,&out)!=0 || (out&3u) || out<BATTLE_BUS_KUSEG_RAM_END)return 0;
        outputs[i]=resolve_memory(runtime,out,4,1);if(!outputs[i])return 0;}
    for(unsigned i=0;i<6;i++) {uintptr_t out=(uintptr_t)outputs[i];
        for(unsigned j=0;j<8;j++){uintptr_t in=(uintptr_t)inputs[j];if(out<in+4 && in<out+4)return 0;}
        for(unsigned j=0;j<6;j++){uintptr_t in=(uintptr_t)args[j];if(out<in+4 && in<out+4)return 0;}}
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_rottranspers4_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
