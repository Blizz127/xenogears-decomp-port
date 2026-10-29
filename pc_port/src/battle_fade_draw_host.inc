/* Complete fixed native color-fade draw callback,800B3878..800B397C.
 * Packed task/owner and graphics words, original frame and live SDK effects. */
#define FD_R(a) battle_entry_read(entry,(a),4)
#define FD_B(a) battle_entry_read(entry,(a),1)
#define FD_W(a,v) battle_entry_write(entry,(a),4,(v))
#define FD_WH(a,v) battle_entry_write(entry,(a),2,(v))
#define FD_WB(a,v) battle_entry_write(entry,(a),1,(v))
#define FD_V0 (entry->cpu->gpr[2])
#define FD_V1 (entry->cpu->gpr[3])
#define FD_A0 (entry->cpu->gpr[4])
#define FD_A1 (entry->cpu->gpr[5])
#define FD_A2 (entry->cpu->gpr[6])
#define FD_A3 (entry->cpu->gpr[7])
#define FD_S0 (entry->cpu->gpr[16])
#define FD_S1 (entry->cpu->gpr[17])
#define FD_S2 (entry->cpu->gpr[18])
#define FD_SP (entry->cpu->gpr[29])
#define FD_RA (entry->cpu->gpr[31])
static void battle_fade_draw_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    FD_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-fade-draw] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"fade draw callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending fade draw callee return",target);
}
static void battle_fade_draw_run(BattleEntryCall *entry)
{
    FD_SP-=0x28u;FD_A1=0x80060000u;FD_V0=0x80060000u;FD_W(FD_SP+0x20u,FD_S2);
    uint32_t p=FD_R(FD_A1-0x6a80u),q=FD_R(FD_V0-0x6accu);FD_S2=p;
    FD_W(FD_SP+0x24u,FD_RA);FD_V1=q;FD_W(FD_SP+0x1cu,FD_S1);FD_W(FD_SP+0x18u,FD_S0);
    p=FD_R(FD_A0+4u);FD_V0=FD_S2+0x50u;FD_S0=p;FD_V0=FD_V0<FD_V1;
    int space=FD_V0!=0;FD_A0=FD_S2;
    if(space){
        FD_V0=FD_S2+0x24u;FD_RA=0x800b38bcu;FD_W(FD_A1-0x6a80u,FD_V0);battle_fade_draw_call(entry,0x80043c9cu,0x800b38b4u);
        FD_A0=FD_S2;FD_RA=0x800b38c8u;FD_A1=1;battle_fade_draw_call(entry,0x80043bfcu,0x800b38c0u);
        FD_A0=0;FD_A2=FD_A0;p=FD_B(FD_S0+0x48u);FD_A3=FD_A0;FD_V0=p;FD_WB(FD_S2+4u,FD_V0);
        p=FD_B(FD_S0+0x49u);FD_V1=0x140u;FD_V0=p;FD_WB(FD_S2+5u,FD_V0);
        p=FD_B(FD_S0+0x4au);FD_V0=0xffffffe0u;FD_A1=p;
        FD_WH(FD_S2+8u,FD_V0);FD_WH(FD_S2+0xau,FD_V0);FD_WH(FD_S2+0xeu,FD_V0);FD_WH(FD_S2+0x10u,FD_V0);
        FD_V0=0xf0u;FD_WH(FD_S2+0xcu,FD_V1);FD_WH(FD_S2+0x12u,FD_V0);FD_WH(FD_S2+0x14u,FD_V1);FD_WH(FD_S2+0x16u,FD_V0);
        FD_WB(FD_S2+6u,FD_A1);p=FD_B(FD_S0+0x41u);FD_RA=0x800b3924u;FD_A1=p;FD_S1=FD_S2+0x18u;
        battle_fade_draw_call(entry,0x80043a1cu,0x800b391cu);
        FD_A0=FD_S1;FD_A1=0;FD_A2=FD_A1;FD_A3=FD_V0&0xffffu;FD_RA=0x800b393cu;FD_W(FD_SP+0x10u,0);
        battle_fade_draw_call(entry,0x800454dcu,0x800b3934u);
        FD_S0=0x80060000u;p=FD_R(FD_S0-0x6a94u);FD_A1=FD_S2;FD_A0=p;FD_RA=0x800b3950u;FD_A0+=8u;
        battle_fade_draw_call(entry,0x80043b48u,0x800b3948u);
        p=FD_R(FD_S0-0x6a94u);FD_A1=FD_S1;FD_A0=p;FD_RA=0x800b3960u;FD_A0+=8u;
        battle_fade_draw_call(entry,0x80043b48u,0x800b3958u);
    }
    p=FD_R(FD_SP+0x24u);q=FD_R(FD_SP+0x20u);FD_RA=p;
    p=FD_R(FD_SP+0x1cu);FD_S2=q;q=FD_R(FD_SP+0x18u);FD_S1=p;FD_SP+=0x28u;FD_S0=q;
}
static int battle_fade_draw_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800b3878u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_FADE_DRAW");if(control&&strcmp(control,"0")==0)return 0;
    const char *expected="0b87d853cdc6d76d6101a044ee6cfec3adb5d536aa61a65c3dfb1626d640ac8a";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),260u,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_fade_draw_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* These packet services are scoped to the six calls in this callback. Keep
 * the original byte writes, delayed loads and SetDrawMode's guest frame:
 * the generic typed SDK binding cannot preserve stack/packet aliases. */
static int battle_fade_draw_sdk_target(uint32_t target,uint32_t caller)
{
    return (target==0x80043c9cu&&caller==0x800b38bcu)||
           (target==0x80043bfcu&&caller==0x800b38c8u)||
           (target==0x80043a1cu&&caller==0x800b3924u)||
           (target==0x800454dcu&&caller==0x800b393cu)||
           (target==0x80043b48u&&(caller==0x800b3950u||caller==0x800b3960u))||
           (target==0x80043b48u &&
            (caller==0x80073b24u||caller==0x80073b50u||
             caller==0x80073bc8u||caller==0x80073e50u) &&
            getenv("XENO_BATTLE_NATIVE_PACKET_LINK_EXPERIMENT") &&
            strcmp(getenv("XENO_BATTLE_NATIVE_PACKET_LINK_EXPERIMENT"),"1")==0);
}
static void battle_fade_draw_mode_word(BattleEntryCall *entry)
{
    FD_V0=0x80050000u;FD_V0=FD_B(FD_V0+0x68d0u);
    FD_V0=(FD_V0-1u)<2u;FD_V1=0xe1000000u;
    if(FD_V0){
        if(FD_A1)FD_V1|=0x800u;
        FD_V0=FD_A2&0x27ffu;if(FD_A0)FD_V0|=0x1000u;
    }else{
        if(FD_A1)FD_V1|=0x200u;
        FD_V0=FD_A2&0x9ffu;if(FD_A0)FD_V0|=0x400u;
    }
    FD_V0|=FD_V1;
}
static void battle_fade_draw_window_word(BattleEntryCall *entry)
{
    FD_SP-=0x10u;
    if(!FD_A0)FD_V0=0;
    else{
        FD_A1=FD_B(FD_A0);FD_A1>>=3;FD_W(FD_SP,FD_A1);
        FD_A2=(uint32_t)(int32_t)(int16_t)battle_entry_read(entry,FD_A0+4u,2);
        FD_A2=((0u-FD_A2)&0xffu)>>3;FD_W(FD_SP+8u,FD_A2);
        uint32_t p=FD_B(FD_A0+2u);FD_A1<<=10;FD_V0=p;FD_V0>>=3;
        FD_W(FD_SP+4u,FD_V0);FD_V0<<=15;
        p=battle_entry_read(entry,FD_A0+6u,2);FD_A0=0xe2000000u;FD_V1=(uint32_t)(int32_t)(int16_t)p;
        FD_A1|=FD_A0;FD_V0|=FD_A1;FD_V1=((0u-FD_V1)&0xffu)>>3;
        FD_A0=FD_V1<<5;FD_V0|=FD_A0;FD_V0|=FD_A2;FD_W(FD_SP+0xcu,FD_V1);
    }
    FD_SP+=0x10u;
}
static void battle_fade_draw_sdk_run(BattleEntryCall *entry,uint32_t target)
{
    uint32_t p,q,r;
    if(target==0x80043c9cu){
        FD_V0=5;FD_WB(FD_A0+3u,FD_V0);FD_V0=0x28u;FD_WB(FD_A0+7u,FD_V0);
    }else if(target==0x80043bfcu){
        FD_V0=FD_B(FD_A0+7u);FD_V0=FD_A1?FD_V0|2u:FD_V0&0xfdu;FD_WB(FD_A0+7u,FD_V0);
    }else if(target==0x80043a1cu){
        FD_V0=(FD_A0&3u)<<7;FD_A1=(FD_A1&3u)<<5;FD_V0|=FD_A1;
        FD_V1=(FD_A3&0x100u)>>4;FD_V0|=FD_V1;FD_A2=(FD_A2&0x3ffu)>>6;
        FD_V0|=FD_A2;FD_A3=(FD_A3&0x200u)<<2;FD_V0|=FD_A3;
    }else if(target==0x80043b48u){
        FD_A2=0xffffffu;FD_A3=0xff000000u;
        p=FD_R(FD_A1);q=FD_R(FD_A0);FD_V1=p;FD_V1&=FD_A3;FD_V0=q;
        FD_V0&=FD_A2;FD_V1|=FD_V0;FD_W(FD_A1,FD_V1);
        p=FD_R(FD_A0);FD_A1&=FD_A2;FD_V0=p;FD_V0&=FD_A3;FD_V0|=FD_A1;FD_W(FD_A0,FD_V0);
    }else{
        FD_SP-=0x20u;FD_W(FD_SP+0x10u,FD_S0);FD_S0=FD_A0;FD_A0=FD_A1;
        FD_V0=2;FD_A1=FD_A2;FD_W(FD_SP+0x14u,FD_S1);
        p=FD_R(FD_SP+0x30u);FD_A2=FD_A3&0xffffu;FD_S1=p;FD_W(FD_SP+0x18u,FD_RA);
        FD_RA=0x8004550cu;FD_WB(FD_S0+3u,FD_V0);battle_fade_draw_mode_word(entry);
        FD_W(FD_S0+4u,FD_V0);FD_RA=0x80045518u;FD_A0=FD_S1;battle_fade_draw_window_word(entry);
        FD_W(FD_S0+8u,FD_V0);p=FD_R(FD_SP+0x18u);q=FD_R(FD_SP+0x14u);FD_RA=p;
        r=FD_R(FD_SP+0x10u);FD_S1=q;FD_SP+=0x20u;FD_S0=r;
    }
}
static int battle_fade_draw_sdk_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_fade_draw_sdk_target(target,cpu->gpr[31])||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_FADE_DRAW");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);if(!image)return 0;
    static const struct {uint32_t start;unsigned length;const char *sha;} spans[]={
        {0x80043c9cu,20,"f58352356211937f99b0d3d2c5a0ba15bb0ae7b0cceb07f2a3710805d21bccb8"},
        {0x80043bfcu,40,"f7bd3db94e23b108242338e61ab13d8b1f6877d7a227b90ed3f11c550612804b"},
        {0x80043a1cu,60,"ca97bb54edac0e68e757a06d6a25eb10c8fea745f5234450b15d0997d759bf42"},
        {0x80043b48u,60,"1c2e33c9e0ddfcd56385916112089fab3d7d343336e2514633a70260ef7d4064"},
        {0x800454dcu,88,"7b4adc8244e499de946aa604de368434cb2e7aef9ac7bffdd3c9d79c30edb0c5"},
        {0x800459dcu,88,"165c0f29e23a39119c72207a8697b899ee1174ecab5da0dac1ab671c922b7d99"},
        {0x80045c10u,132,"9762c42142ce9b5872675cd3f067fa1c6d07ef10e3880fcac24044f89497f9f6"}};
    for(unsigned j=0;j<sizeof(spans)/sizeof(spans[0]);j++){
        if(spans[j].start!=target&&!(target==0x800454dcu&&j>=5))continue;
        size_t off=XENO_RD_SLUS_OFF(spans[j].start);if(image->size<off+spans[j].length)return 0;
        uint8_t digest[32];file1_digest(image->bytes+off,spans[j].length,digest);
        for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=spans[j].sha[i*2]||file1_hex_digit(digest[i]&15u)!=spans[j].sha[i*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_fade_draw_sdk_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef FD_R
#undef FD_B
#undef FD_W
#undef FD_WH
#undef FD_WB
#undef FD_V0
#undef FD_V1
#undef FD_A0
#undef FD_A1
#undef FD_A2
#undef FD_A3
#undef FD_S0
#undef FD_S1
#undef FD_S2
#undef FD_SP
#undef FD_RA
