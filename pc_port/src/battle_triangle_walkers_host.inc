/* Four native triangle entries share the complete original E058..E22C body.
 * Guest packet links and pointers stay words; only the GTE backend is called. */
/* These routines intentionally form physical RAM addresses by masking the
 * packet pointer to 24 bits. Use the existing explicit-pointer conversion;
 * the generic mixed guest/host bus deliberately does not infer this domain. */
#define TW_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define TW_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define TW_W(a,v) battle_entry_write(entry,shard_sdk_address(a),4,(v))
#define TW_AT (entry->cpu->gpr[1])
#define TW_V0 (entry->cpu->gpr[2])
#define TW_V1 (entry->cpu->gpr[3])
#define TW_A0 (entry->cpu->gpr[4])
#define TW_A1 (entry->cpu->gpr[5])
#define TW_A3 (entry->cpu->gpr[7])
#define TW_T0 (entry->cpu->gpr[8])
#define TW_T1 (entry->cpu->gpr[9])
#define TW_T2 (entry->cpu->gpr[10])
#define TW_T3 (entry->cpu->gpr[11])
#define TW_T4 (entry->cpu->gpr[12])
#define TW_T5 (entry->cpu->gpr[13])
#define TW_T7 (entry->cpu->gpr[15])
#define TW_S0 (entry->cpu->gpr[16])
#define TW_S1 (entry->cpu->gpr[17])
#define TW_S2 (entry->cpu->gpr[18])
#define TW_S3 (entry->cpu->gpr[19])
#define TW_S4 (entry->cpu->gpr[20])
#define TW_S5 (entry->cpu->gpr[21])
#define TW_S6 (entry->cpu->gpr[22])
#define TW_S7 (entry->cpu->gpr[23])
#define TW_T8 (entry->cpu->gpr[24])
#define TW_T9 (entry->cpu->gpr[25])
#define TW_SP (entry->cpu->gpr[29])

static void battle_triangle_vertices(BattleEntryCall *entry)
{
    TW_T0=TW_T4&0xffffu;TW_T0<<=3;TW_T0+=TW_S0;
    MTC2(TW_R(TW_T0),0);MTC2(TW_R(TW_T0+4u),1);
    TW_T0=(TW_T4>>13)&0xfff8u;TW_T0+=TW_S0;
    MTC2(TW_R(TW_T0),2);MTC2(TW_R(TW_T0+4u),3);
    TW_T0=TW_T5<<3;TW_T0+=TW_S0;
    MTC2(TW_R(TW_T0),4);MTC2(TW_R(TW_T0+4u),5);
}
static void battle_triangle_walkers_run(BattleEntryCall *entry,uint32_t target)
{
    switch(target){
    case 0x8002e010u:TW_T9=12;TW_T8=0x09000000u;TW_A3=0x28u;break;
    case 0x8002e024u:TW_T9=8;TW_T8=0x06000000u;TW_A3=0x1cu;break;
    case 0x8002e038u:TW_T9=4;TW_T8=0x04000000u;TW_A3=0x14u;break;
    default:TW_T9=8;TW_T8=0x07000000u;TW_A3=0x20u;break;
    }
    TW_W(TW_SP-4u,TW_S0);TW_W(TW_SP-8u,TW_S1);TW_W(TW_SP-0x10u,TW_S2);
    TW_W(TW_SP-0x14u,TW_S3);TW_W(TW_SP-0x18u,TW_S4);TW_W(TW_SP-0x1cu,TW_S5);
    TW_W(TW_SP-0x20u,TW_S6);TW_W(TW_SP-0x24u,TW_S7);
    TW_S0=0x80060000u;uint32_t p=TW_R(TW_S0-0x6ac4u);TW_S2=0x80060000u;TW_S0=p;
    p=TW_R(TW_S2-0x6a88u);TW_S3=0x80060000u;TW_S2=p;
    p=TW_R(TW_S3-0x6bdcu);TW_S4=0x80060000u;TW_S3=p;
    p=TW_R(TW_S4-0x6a98u);TW_V0=0x80050000u;TW_S4=p;
    p=TW_R(TW_V0+0xfcu);TW_V1=0x80050000u;TW_V0=p;
    p=TW_R(TW_V1+0xf8u);TW_T7=0x80050000u;TW_V1=p;
    p=TW_R(TW_T7+0x100u);uint32_t q=TW_R(TW_A0);TW_T7=p;
    p=TW_H(TW_A0+4u);TW_T4=q;TW_T0=TW_T4&0xffffu;TW_T5=p;
    battle_triangle_vertices(entry);TW_S3-=TW_A3;TW_S6=0x00ffffffu;
    int pending_t4=0;uint32_t next_t4=0;
    for(;;){
        doCOP2(0x4a280030); /* RTPT, including final lookahead. */
        if(pending_t4){TW_T4=next_t4;pending_t4=0;}
        int empty=TW_A1==0;TW_A1--;if(empty)break;
        TW_A0+=8u;p=TW_R(TW_A0);q=TW_H(TW_A0+4u);TW_T4=p;TW_S3+=TW_A3;TW_T5=q;
        battle_triangle_vertices(entry);
        p=MFC2(31);q=MFC2(12);TW_T0=p;int negative=(int32_t)TW_T0<0;TW_T1=q;TW_T0=TW_T1<TW_V0;
        if(negative)continue;
        p=MFC2(13);q=MFC2(14);TW_T2=p;doCOP2(0x4b400006);TW_T3=q; /* NCLIP */
        int inside=TW_T0!=0;TW_T0=TW_T2<TW_V0;
        if(!inside){inside=TW_T0!=0;TW_T0=TW_T3<TW_V0;if(!inside)inside=TW_T0!=0;}
        TW_T0=TW_T1&0xffffu;if(!inside)continue;
        TW_T0=TW_T0<TW_V1;inside=TW_T0!=0;TW_T0=TW_T2&0xffffu;
        if(!inside){TW_T0=TW_T0<TW_V1;inside=TW_T0!=0;TW_T0=TW_T3&0xffffu;
            if(!inside){TW_T0=TW_T0<TW_V1;inside=TW_T0!=0;}}
        p=MFC2(24);
        if(!inside){pending_t4=1;next_t4=p;continue;}
        doCOP2(0x4b58002d);TW_T4=p; /* AVSZ3 commits preceding MFC2. */
        int back=(int32_t)TW_T4<=0;TW_S3&=TW_S6;if(back)continue;
        TW_W(TW_S3+8u,TW_T1);TW_T0=TW_S3+TW_T9;TW_W(TW_T0+8u,TW_T2);
        TW_T0+=TW_T9;TW_W(TW_T0+8u,TW_T3);
        p=MFC2(7);TW_S2++;TW_T5=p;int zero=TW_T5==0;
        TW_T5=(uint32_t)((int32_t)TW_T5>>(TW_T7&31u));if(zero)continue;
        TW_T5<<=2;TW_T5+=TW_S4;p=TW_R(TW_T5);TW_W(TW_T5,TW_S3);TW_T1=p;TW_T1|=TW_T8;
        TW_W(TW_S3,TW_T1);
    }
    TW_S3+=TW_A3;TW_AT=0x80060000u;TW_W(TW_AT-0x6a88u,TW_S2);
    TW_AT=0x80060000u;TW_W(TW_AT-0x6bdcu,TW_S3);
    p=TW_R(TW_SP-4u);q=TW_R(TW_SP-8u);TW_S0=p;
    p=TW_R(TW_SP-0x10u);TW_S1=q;q=TW_R(TW_SP-0x14u);TW_S2=p;
    p=TW_R(TW_SP-0x18u);TW_S3=q;q=TW_R(TW_SP-0x1cu);TW_S4=p;
    p=TW_R(TW_SP-0x20u);TW_S5=q;TW_S6=p;
    q=TW_R(TW_SP-0x24u);
    /* The original return delay slot loads S7. Keep that pending at the
     * caller boundary; the native packet caller commits it after its next
     * successful load, preserving the first post-return fault state. */
    entry->cpu->load_valid=1;entry->cpu->load_reg=23;entry->cpu->load_value=q;entry->cpu->load_mask=0;
}
static int battle_triangle_walkers_target(uint32_t target)
{return target==0x8002e010u||target==0x8002e024u||target==0x8002e038u||target==0x8002e04cu;}
static int battle_triangle_walkers_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_triangle_walkers_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TRIANGLE_WALKERS");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(0x8002e010u);
    if(!image||image->size<offset+540u)return 0;
    uint8_t digest[32];file1_digest(image->bytes+offset,540u,digest);
    const char *expected="829378b4d3db1385404ec611ddf5f1f08e731ea10a83c0ebcdb9f2a8c4cace40";
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_triangle_walkers_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef TW_R
#undef TW_H
#undef TW_W
#undef TW_AT
#undef TW_V0
#undef TW_V1
#undef TW_A0
#undef TW_A1
#undef TW_A3
#undef TW_T0
#undef TW_T1
#undef TW_T2
#undef TW_T3
#undef TW_T4
#undef TW_T5
#undef TW_T7
#undef TW_S0
#undef TW_S1
#undef TW_S2
#undef TW_S3
#undef TW_S4
#undef TW_S5
#undef TW_S6
#undef TW_S7
#undef TW_T8
#undef TW_T9
#undef TW_SP
