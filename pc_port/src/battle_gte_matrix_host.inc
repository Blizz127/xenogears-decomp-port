/* Native GTE matrix-register setters for verified atlas calls. */
static void battle_set_matrix_run(BattleEntryCall *entry,uint32_t target)
{
    uint32_t *r=entry->cpu->gpr;
    if(target==0x80049efcu) {
        uint32_t a=battle_entry_read(entry,r[4],4);
        uint32_t b=battle_entry_read(entry,r[4]+4u,4);r[8]=a;
        uint32_t c=battle_entry_read(entry,r[4]+8u,4);r[9]=b;
        uint32_t d=battle_entry_read(entry,r[4]+12u,4);r[10]=c;
        uint32_t e=battle_entry_read(entry,r[4]+16u,4);r[11]=d;
        CTC2(r[8],0);r[12]=e;CTC2(r[9],1);CTC2(r[10],2);CTC2(r[11],3);CTC2(r[12],4);
    } else if(target==0x80049f8cu) {
        uint32_t a=battle_entry_read(entry,r[4]+20u,4);
        uint32_t b=battle_entry_read(entry,r[4]+24u,4);r[8]=a;
        uint32_t c=battle_entry_read(entry,r[4]+28u,4);r[9]=b;
        CTC2(r[8],5);r[10]=c;CTC2(r[9],6);CTC2(r[10],7);
    } else battle_entry_fail(entry,"unsupported GTE matrix setter",target);
}

/* Only the two atlas matrix-setter call sites. */
static int battle_set_matrix_try_host(BattleMipsRuntime *runtime,
                                      PcPortMipsCpu *cpu, uint32_t target)
{
    if(runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_GTE_MATRIX_EXPERIMENT");
    if(!control || strcmp(control,"1")!=0)return 0;
    uint32_t ra=cpu->gpr[31];unsigned length,start,end;const char *expected;
    if(target==0x80049efcu && ra==0x800260bcu) {
        length=48u;start=0;end=20u;expected="095799477a23b46afa40d99dc027e7ef467cd14b8e135234d7e5b5c9ebc07883";
    } else if(target==0x80049f8cu && ra==0x800260c4u) {
        length=32u;start=20u;end=32u;expected="2f228789930df0a6d6db76e145ba3301694d7c01b9df2dbf45d2f48ad7aaee1e";
    } else return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+length)return 0;
    uint8_t digest[32];file1_digest(image->bytes+offset,length,digest);
    for(unsigned i=0;i<sizeof(digest);++i)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
           file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t matrix=cpu->gpr[4];
    if((matrix&3u) || matrix>UINT32_MAX-(end-4u) || matrix<BATTLE_BUS_KUSEG_RAM_END)return 0;
    for(unsigned i=start;i<end;i+=4u)
        if(!resolve_memory(runtime,matrix+i,4,0))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_set_matrix_run(&entry,target);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
