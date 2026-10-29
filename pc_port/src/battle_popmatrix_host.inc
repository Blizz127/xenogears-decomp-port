/* Nonempty guest PopMatrix; native stack synchronization guarded separately. */
static void battle_popmatrix_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[14]=0x80050000u;uint32_t offset=battle_entry_read(entry,r[14]+0x6d2cu,4);r[14]=offset;r[1]=0x80050000u;
    if((int32_t)offset<=0)battle_entry_fail(entry,"empty matrix stack",offset);
    r[14]-=32u;r[1]=0x80050000u;battle_entry_write(entry,r[1]+0x6d2cu,4,r[14]);r[15]=0x80050000u+r[14]+0x6d30u;
    uint32_t a=battle_entry_read(entry,r[15],4);uint32_t b=battle_entry_read(entry,r[15]+4u,4);r[8]=a;CTC2(r[8],0);r[9]=b;CTC2(r[9],1);
    a=battle_entry_read(entry,r[15]+8u,4);b=battle_entry_read(entry,r[15]+12u,4);r[8]=a;CTC2(r[8],2);r[9]=b;CTC2(r[9],3);
    r[8]=battle_entry_read(entry,r[15]+16u,4);CTC2(r[8],4);
    a=battle_entry_read(entry,r[15]+20u,4);b=battle_entry_read(entry,r[15]+24u,4);r[8]=a;uint32_t c=battle_entry_read(entry,r[15]+28u,4);r[9]=b;CTC2(r[8],5);r[10]=c;CTC2(r[9],6);CTC2(r[10],7);
    PopMatrix();
}

/* Decline empty or desynchronized stacks; only the atlas restore edge. */
static int battle_popmatrix_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800496acu || cpu->gpr[31]!=0x80026300u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_POPMATRIX_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+160u)return 0;
    static const char expected[]="5a827d4618f75e764db67c0a1c92fedf5b01ac82848cfb92393c857d15a8c14b";
    uint8_t digest[32];file1_digest(image->bytes+offset,160u,digest);
    for(unsigned i=0;i<sizeof(digest);++i)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t guest_offset;if(runtime_read(runtime,0x80056d2cu,4,&guest_offset)!=0 || (guest_offset&31u)!=0 ||
       guest_offset==0 || guest_offset>640u || matrixLevel<1 || matrixLevel>20 ||
       (uint32_t)matrixLevel!=guest_offset/32u || currentMatrix!=stack+matrixLevel)return 0;
    for(uint32_t i=0;i<guest_offset;i+=4u) {uint32_t word;if(runtime_read(runtime,0x80056d30u+i,4,&word)!=0 || word!=load_le((const uint8_t*)stack+i,4))return 0;}
    if(!resolve_memory(runtime,0x80056d2cu,4,1))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_popmatrix_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
