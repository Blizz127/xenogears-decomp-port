/* Retail paired-task constructor prefix, stopping before HeapAlloc. */
static void battle_popup_pair_prefix_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    r[29]-=40u;battle_entry_write(entry,r[29]+16u,4,r[16]);r[16]=r[5];
    r[5]=0x80060000u;r[5]=battle_entry_read(entry,r[5]+0xffff91afu,1);
    battle_entry_write(entry,r[29]+32u,4,r[20]);r[20]=battle_entry_read(entry,r[29]+56u,4);
    battle_entry_write(entry,r[29]+20u,4,r[17]);r[17]=r[6];
    battle_entry_write(entry,r[29]+24u,4,r[18]);r[18]=r[7];
    battle_entry_write(entry,r[29]+36u,4,r[31]);
    battle_entry_write(entry,r[29]+28u,4,r[19]);
    battle_phase_call(entry,0x80031bdcu,0x8001d208u);
    battle_entry_fail(entry,"paired task heap allocation returned without continuation",0x80031bdcu);
}

static int battle_popup_pair_prefix_try_host(BattleMipsRuntime *runtime,
                                              PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8001d1d8u || cpu->gpr[31]!=0x800bd0e4u || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0) || cpu->gpr[4]!=0x68u)return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_PAIR_PREFIX_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t off=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<off+0xc0u)return 0;
    static const char expected[]="e3cdb57ad6c31bfc1a9540248e0b7fda1172fe6142340d3dd17e41d6374fde4e";
    uint8_t digest[32];file1_digest(image->bytes+off,0xc0u,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
           file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    ResolvedData *binding=find_data(runtime,0x800591afu,1u);
    uint8_t *flags=dlsym(RTLD_DEFAULT,"D_800591AF");
    uint32_t sp=cpu->gpr[29];
    if(!binding || binding->address!=0x800591afu || binding->host!=flags ||
       binding->size<1u || !flags || (sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+40u ||
       sp>UINT32_MAX-19u)return 0;
    uint8_t *code=resolve_memory(runtime,target,0xc0u,0);
    uint8_t *arg=resolve_memory(runtime,sp+16u,4u,0);
    if(!code || !arg)return 0;
    const unsigned offsets[]={16,20,24,28,32,36};
    for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++) {
        uint8_t *saved=resolve_memory(runtime,sp-40u+offsets[i],4u,1);
        if(!saved)return 0;
        uintptr_t a=(uintptr_t)saved,b=(uintptr_t)code,c=(uintptr_t)arg,d=(uintptr_t)flags;
        if((a<b+0xc0u && b<a+4u) || (a<c+4u && c<a+4u) || (a<d+1u && d<a+4u))return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_popup_pair_prefix_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
