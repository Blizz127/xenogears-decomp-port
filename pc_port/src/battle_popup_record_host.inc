/* Scoped, arithmetic-only popup frame record lookup. */
static int battle_popup_record_try_host(BattleMipsRuntime *runtime,
                                         PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800b168cu || (cpu->gpr[31]!=0x800bd150u && cpu->gpr[31]!=0x800bd178u && cpu->gpr[31]!=0x800bd1b4u) || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0) || cpu->gpr[5]!=0u ||
       cpu->gpr[4]>UINT32_MAX-12u)return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_RECORD_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1")!=0)return 0;
    uint8_t *data=dlsym(RTLD_DEFAULT,"D_8001C76C");
    ResolvedData *binding=find_data(runtime,0x8001c76cu,0x170u);
    const uint8_t *code=resolve_memory(runtime,target,0x18u,0);
    if(!data || !binding || binding->address!=0x8001c76cu || binding->host!=data ||
       binding->size!=0x170u || !code || resolve_memory(runtime,cpu->gpr[4],0x170u,0)!=data)return 0;
    static const char expected[]="e0dedf5e7431d44f04bd206c92e077bba44d06d0fc1d4d759b0317db5d0f53ef";
    uint8_t digest[32];file1_digest(code,0x18u,digest);
    for(unsigned i=0;i<sizeof digest;i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    cpu->gpr[2]=cpu->gpr[4]+12u;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}
