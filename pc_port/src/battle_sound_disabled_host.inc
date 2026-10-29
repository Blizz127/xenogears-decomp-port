/* Only the retail branch where SFX are already disabled. */
static int battle_sound_disabled_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x80039db8u || cpu->gpr[31]!=0x8008aa64u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_SOUND_DISABLED_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+96u)return 0;
    static const char expected[]="491128d2006c81928253d1cd460dbad281ce6215187649f37dcca0bcf49fd53b";
    uint8_t digest[32];file1_digest(image->bytes+offset,96u,digest);
    for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t flags,sp=cpu->gpr[29];if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+24u || runtime_read(runtime,0x8005957cu,2,&flags)!=0 || (flags&0x800u) || !resolve_memory(runtime,sp-8u,4,1) || !resolve_memory(runtime,sp-8u,4,0))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    uint32_t *r=cpu->gpr;r[29]-=24u;r[2]=0x80060000u;uint32_t f=battle_entry_read(&entry,r[2]+0xffff957cu,2);r[5]=r[4];r[2]=f&0x800u;
    battle_entry_write(&entry,r[29]+16u,4,r[31]);uint32_t saved=battle_entry_read(&entry,r[29]+16u,4);r[29]+=24u;r[31]=saved;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

