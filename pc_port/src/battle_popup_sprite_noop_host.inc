/* Scoped no-op branch of the popup sprite update. */
static int battle_popup_sprite_noop_try_host(BattleMipsRuntime *runtime,
                                             PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8001f6b0u || (cpu->gpr[31]!=0x800bd1ccu && cpu->gpr[31]!=0x800bcb40u && cpu->gpr[31]!=0x800bcc34u && cpu->gpr[31]!=0x800bd05cu) || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0) || cpu->gpr[4]<BATTLE_BUS_KUSEG_RAM_END ||
       (cpu->gpr[4]&3u) || cpu->gpr[4]>UINT32_MAX-0x40u)return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_SPRITE_NOOP_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1")!=0)return 0;
    uint32_t flags;
    if(runtime_read(runtime,cpu->gpr[4]+0x3cu,4,&flags)!=0 || (flags&3u)==1u)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+0xa0u)return 0;
    static const char expected[]="cb7dff828c141c1959ce71eba1492665e5bdf561cc0f7a9859ed08ac6efe2884";
    uint8_t digest[32];file1_digest(image->bytes+offset,0xa0u,digest);
    for(unsigned i=0;i<sizeof digest;i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    cpu->gpr[9]=cpu->gpr[4];cpu->gpr[4]=flags;cpu->gpr[2]=1u;cpu->gpr[3]=flags&3u;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
