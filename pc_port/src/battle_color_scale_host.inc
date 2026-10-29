/* Actor color adjustment. The original truncates a signed Q8 product toward
 * zero, then tests the signed low halfword for its upper clamp. Negative
 * results deliberately keep their low byte; there is no lower clamp. */
static int battle_color_scale_try_host(BattleMipsRuntime *runtime,
                                       PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800aa514u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_COLOR_SCALE");
    if(control&&strcmp(control,"0")==0)return 0;
    const char *expected="497998fe1563096b2587d4aee6ba4cccf06bcf2b223e15e9dc838e3bafe0242a";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),80u,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    cpu->gpr[5]=(uint32_t)(int32_t)(int16_t)cpu->gpr[5];
    cpu->gpr[4]=(uint32_t)(int32_t)(int16_t)cpu->gpr[4];
    int64_t product=(int64_t)(int32_t)cpu->gpr[5]*(int32_t)cpu->gpr[4];
    cpu->hi=(uint32_t)((uint64_t)product>>32);cpu->lo=(uint32_t)product;
    cpu->gpr[2]=cpu->lo;
    if((int32_t)cpu->gpr[2]<0)cpu->gpr[2]+=255u;
    cpu->gpr[2]=(uint32_t)((int32_t)cpu->gpr[2]>>8);
    cpu->gpr[2]+=cpu->gpr[6];cpu->gpr[3]=cpu->gpr[2];
    cpu->gpr[2]=(uint32_t)(int32_t)(int16_t)cpu->gpr[2];
    cpu->gpr[2]=(int32_t)cpu->gpr[2]<256;
    if(!cpu->gpr[2])cpu->gpr[3]=255u;
    cpu->gpr[2]=cpu->gpr[3]&255u;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
