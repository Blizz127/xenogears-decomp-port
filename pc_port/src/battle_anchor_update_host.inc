/* Complete fixed native anchor updater, 800A44C0..800A4654.
 * Two guest records, live model words, original frame and GTE side effects. */
#define AN_R(a) battle_entry_read(entry,(a),4)
#define AN_H(a) battle_entry_read(entry,(a),2)
#define AN_SH(a) ((uint32_t)(int32_t)(int16_t)AN_H(a))
#define AN_W(a,v) battle_entry_write(entry,(a),4,(v))
#define AN_WH(a,v) battle_entry_write(entry,(a),2,(v))
#define AN_AT (entry->cpu->gpr[1])
#define AN_V0 (entry->cpu->gpr[2])
#define AN_V1 (entry->cpu->gpr[3])
#define AN_A0 (entry->cpu->gpr[4])
#define AN_A1 (entry->cpu->gpr[5])
#define AN_A2 (entry->cpu->gpr[6])
#define AN_S0 (entry->cpu->gpr[16])
#define AN_S1 (entry->cpu->gpr[17])
#define AN_S2 (entry->cpu->gpr[18])
#define AN_S3 (entry->cpu->gpr[19])
#define AN_S4 (entry->cpu->gpr[20])
#define AN_S5 (entry->cpu->gpr[21])
#define AN_SP (entry->cpu->gpr[29])
#define AN_RA (entry->cpu->gpr[31])
static void battle_anchor_update_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    AN_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-anchor-update] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"anchor update callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending anchor update callee return",target);
}
static void battle_anchor_update_run(BattleEntryCall *entry)
{
    AN_SP-=0x40u;AN_W(AN_SP+0x34u,AN_S5);AN_S5=AN_A0;
    AN_W(AN_SP+0x30u,AN_S4);AN_S4=0x1f800000u;
    AN_W(AN_SP+0x24u,AN_S1);AN_S1=0x800d3308u;
    AN_W(AN_SP+0x2cu,AN_S3);AN_S3=AN_S1-2u;
    AN_W(AN_SP+0x28u,AN_S2);AN_S2=AN_S1-4u;
    AN_W(AN_SP+0x20u,AN_S0);AN_S0=0;AN_W(AN_SP+0x38u,AN_RA);
    int again;
    do{
        AN_AT=0x800d0000u;AN_AT+=AN_S0;AN_V0=AN_SH(AN_AT+0x330au);
        if(AN_V0){
            AN_AT=0x800d0000u;AN_AT+=AN_S0;AN_V0=AN_SH(AN_AT+0x3314u);
            int copy=(int32_t)AN_V0<0;AN_V0<<=2;
            if(!copy){
                AN_V0+=AN_S5;AN_V0=AN_R(AN_V0);copy=AN_V0==0;AN_A2=AN_S4;
            }
            if(!copy){
                AN_V1=AN_R(AN_V0+4u);AN_AT=0x800d0000u;AN_AT+=AN_S0;
                uint32_t p=AN_SH(AN_AT+0x3316u);AN_A0=AN_V1+12u;AN_V0=p;
                AN_A1=AN_V0<<5;AN_A1-=AN_V0;AN_A1<<=2;AN_A1+=AN_V1;
                AN_RA=0x800a456cu;AN_A1+=0xa8u;battle_anchor_update_call(entry,0x8004931cu,0x800a4564u);
                AN_RA=0x800a4574u;AN_A0=AN_S4;battle_anchor_update_call(entry,0x80049efcu,0x800a456cu);
                AN_RA=0x800a457cu;AN_A0=AN_S4;battle_anchor_update_call(entry,0x80049f8cu,0x800a4574u);
                AN_V0=0x800d330cu;AN_V0+=AN_S0;
                MTC2(AN_R(AN_V0),0);MTC2(AN_R(AN_V0+4u),1);doCOP2(0x4a480012);
                AN_V0=AN_SP+0x10u;AN_W(AN_V0,MFC2(25));AN_W(AN_V0+4u,MFC2(26));AN_W(AN_V0+8u,MFC2(27));
                AN_V0=AN_H(AN_SP+0x10u);AN_WH(AN_S2,AN_V0);
                AN_V0=AN_H(AN_SP+0x14u);AN_WH(AN_S3,AN_V0);
                AN_V0=AN_H(AN_SP+0x18u);AN_WH(AN_S1,AN_V0);
            }else{
                AN_AT=0x800d0000u;AN_AT+=AN_S0;AN_V0=AN_H(AN_AT+0x330cu);AN_WH(AN_S2,AN_V0);
                AN_AT=0x800d0000u;AN_AT+=AN_S0;AN_V0=AN_H(AN_AT+0x330eu);AN_WH(AN_S3,AN_V0);
                AN_AT=0x800d0000u;AN_AT+=AN_S0;AN_V0=AN_H(AN_AT+0x3310u);AN_WH(AN_S1,AN_V0);
            }
        }
        AN_S1+=0x14u;AN_S3+=0x14u;AN_S2+=0x14u;AN_V0=0x800d3330u;
        AN_V0=(int32_t)AN_S1<(int32_t)AN_V0;again=AN_V0!=0;AN_S0+=0x14u;
    }while(again);
    uint32_t p=AN_R(AN_SP+0x38u),q=AN_R(AN_SP+0x34u);AN_RA=p;
    p=AN_R(AN_SP+0x30u);AN_S5=q;q=AN_R(AN_SP+0x2cu);AN_S4=p;
    p=AN_R(AN_SP+0x28u);AN_S3=q;q=AN_R(AN_SP+0x24u);AN_S2=p;
    p=AN_R(AN_SP+0x20u);AN_S1=q;AN_SP+=0x40u;AN_S0=p;
}
static int battle_anchor_update_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800a44c0u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ANCHOR_UPDATE");if(control&&strcmp(control,"0")==0)return 0;
    const char *expected="fdecae56f92c1249ed1d16967fb2af6e52e160794b451ae36547d80f1443c8cf";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),404u,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_anchor_update_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef AN_R
#undef AN_H
#undef AN_SH
#undef AN_W
#undef AN_WH
#undef AN_AT
#undef AN_V0
#undef AN_V1
#undef AN_A0
#undef AN_A1
#undef AN_A2
#undef AN_S0
#undef AN_S1
#undef AN_S2
#undef AN_S3
#undef AN_S4
#undef AN_S5
#undef AN_SP
#undef AN_RA
