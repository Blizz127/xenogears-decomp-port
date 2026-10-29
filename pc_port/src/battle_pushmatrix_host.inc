/* Experimental non-overflow PushMatrix; gate synchronizes guest/native stacks. */
static void battle_pushmatrix_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    r[14]=0x80050000u;
    uint32_t offset=battle_entry_read(entry,r[14]+0x6d2cu,4);
    r[14]=offset;
    r[1]=(int32_t)r[14] < 0x280;
    r[1]=0x80050000u;
    if ((int32_t)offset>=0x280) battle_entry_fail(entry,"matrix overflow unsupported",offset);
    r[15]=0x80050000u; r[15]+=r[14]; r[15]+=0x6d30u;
    uint32_t c0=CFC2(0),c1=CFC2(1);r[8]=c0;
    battle_entry_write(entry,r[15],4,r[8]);r[9]=c1;battle_entry_write(entry,r[15]+4u,4,r[9]);
    uint32_t c2=CFC2(2),c3=CFC2(3);r[8]=c2;
    battle_entry_write(entry,r[15]+8u,4,r[8]);r[9]=c3;battle_entry_write(entry,r[15]+12u,4,r[9]);
    r[8]=CFC2(4);battle_entry_write(entry,r[15]+16u,4,r[8]);
    uint32_t c5=CFC2(5),c6=CFC2(6);r[8]=c5;uint32_t c7=CFC2(7);r[9]=c6;
    battle_entry_write(entry,r[15]+20u,4,r[8]);r[10]=c7;
    battle_entry_write(entry,r[15]+24u,4,r[9]);battle_entry_write(entry,r[15]+28u,4,r[10]);
    r[14]+=0x20u;r[1]=0x80050000u;
    battle_entry_write(entry,r[1]+0x6d2cu,4,r[14]);
    PushMatrix();
}

extern int matrixLevel;
extern MATRIX stack[], *currentMatrix;
/* Only the atlas PushMatrix edge; refuse unsynchronized or overflow stacks. */
static int battle_pushmatrix_try_host(BattleMipsRuntime *runtime,
                                      PcPortMipsCpu *cpu, uint32_t target)
{
    if(target!=0x8004960cu || cpu->gpr[31]!=0x80026064u ||
       runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_PUSHMATRIX_EXPERIMENT");
    if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+0xa0u)return 0;
    static const char expected[]="3db4f4059cf64a2c6ee0a33f702a85c767f099ed592532eed339022c2c03b984";
    uint8_t digest[32];file1_digest(image->bytes+offset,0xa0u,digest);
    for(unsigned i=0;i<sizeof(digest);++i)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
           file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t guest_offset;
    if(runtime_read(runtime,0x80056d2cu,4,&guest_offset)!=0 ||
       (guest_offset & 31u)!=0 || guest_offset>=640u || matrixLevel<0 || matrixLevel>=20 ||
       (uint32_t)matrixLevel!=guest_offset/32u || currentMatrix!=stack+matrixLevel)return 0;
    for(uint32_t i=0;i<guest_offset;i+=4u) {
        uint32_t guest_word;
        if(runtime_read(runtime,0x80056d30u+i,4,&guest_word)!=0 ||
           guest_word!=load_le((const uint8_t *)stack+i,4))return 0;
    }
    if(!resolve_memory(runtime,0x80056d2cu,4,1))return 0;
    for(unsigned i=0;i<32u;i+=4u)
        if(!resolve_memory(runtime,0x80056d30u+guest_offset+i,4,1))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_pushmatrix_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
