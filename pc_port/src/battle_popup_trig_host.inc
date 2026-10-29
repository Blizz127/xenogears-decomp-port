/* Scoped retail table lookup for the popup colour routine. */
static int battle_popup_trig_try_host(BattleMipsRuntime *runtime,
                                      PcPortMipsCpu *cpu,uint32_t target)
{
    int cosine=target==0x8003f8b0u && (cpu->gpr[31]==0x800bcb14u || cpu->gpr[31]==0x800bcbe4u);
    int sine=target==0x8003f8ccu && cpu->gpr[31]==0x800bcc0cu;
    if((!cosine && !sine) || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_TRIG_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t code_offset=XENO_RD_SLUS_OFF(target);
    uint32_t angle=cpu->gpr[4]&0xfffu, table_address=0x800523f0u+angle*4u+(sine?2u:0u),value;
    size_t table_offset=XENO_RD_SLUS_OFF(table_address);
    if(!image || !image->bytes || image->size<code_offset+28u || image->size<table_offset+2u ||
       runtime_read(runtime,table_address,2,&value)!=0 || value!=load_le(image->bytes+table_offset,2))return 0;
    const char *expected=cosine?"05824b33c9230ec581a78db8d6cc2ec8acccf1e2c2a3da094fe8305af4ede8c0":
                                "81e57d431b9edfe11421c08923e4ccf5f83e54359c27ac822e30381abbf6838e";
    uint8_t digest[32];file1_digest(image->bytes+code_offset,28u,digest);
    for(unsigned i=0;i<sizeof digest;i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    cpu->gpr[4]=angle;cpu->gpr[8]=angle*4u;cpu->gpr[1]=0x80050000u+cpu->gpr[8];
    cpu->gpr[2]=(uint32_t)(int32_t)(int16_t)value;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
