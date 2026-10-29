/* Complete AA820 selector. Effect records contain guest code words, even
 * when their consumers execute compiled host callbacks. No host pointer
 * conversion or narrowing of the full 32-bit selector is permitted here. */
static int battle_callback_selector_try_host(BattleMipsRuntime *runtime,
                                             PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800aa820u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_CALLBACK_SELECTOR");
    if(control&&strcmp(control,"0")==0)return 0;
    const char *expected="935a173e33ad56fce9dc1291e0a4552620c892ce42b17a030e7ea29187d02e35";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),120u,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    switch(cpu->gpr[4]){
    case 1:cpu->gpr[2]=0x800a3514u;break;
    case 2:cpu->gpr[2]=0x800a3578u;break;
    case 3:cpu->gpr[2]=0x800a35c8u;break;
    default:cpu->gpr[2]=0x800a3490u;break;
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
