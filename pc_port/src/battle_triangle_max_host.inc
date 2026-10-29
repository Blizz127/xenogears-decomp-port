/* Four native maximum-SZ triangle entries share E490..E64C and the
 * noncontiguous E1F4..E22C return tail.
 * Guest packet links and pointers stay words; only the GTE backend is called. */
/* These routines intentionally form physical RAM addresses by masking the
 * packet pointer to 24 bits. Use the existing explicit-pointer conversion;
 * the generic mixed guest/host bus deliberately does not infer this domain. */
#define TM_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define TM_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define TM_W(a,v) battle_entry_write(entry,shard_sdk_address(a),4,(v))
#define TM_AT (entry->cpu->gpr[1])
#define TM_V0 (entry->cpu->gpr[2])
#define TM_V1 (entry->cpu->gpr[3])
#define TM_A0 (entry->cpu->gpr[4])
#define TM_A1 (entry->cpu->gpr[5])
#define TM_A3 (entry->cpu->gpr[7])
#define TM_T0 (entry->cpu->gpr[8])
#define TM_T1 (entry->cpu->gpr[9])
#define TM_T2 (entry->cpu->gpr[10])
#define TM_T3 (entry->cpu->gpr[11])
#define TM_T4 (entry->cpu->gpr[12])
#define TM_T5 (entry->cpu->gpr[13])
#define TM_T7 (entry->cpu->gpr[15])
#define TM_S0 (entry->cpu->gpr[16])
#define TM_S1 (entry->cpu->gpr[17])
#define TM_S2 (entry->cpu->gpr[18])
#define TM_S3 (entry->cpu->gpr[19])
#define TM_S4 (entry->cpu->gpr[20])
#define TM_S5 (entry->cpu->gpr[21])
#define TM_S6 (entry->cpu->gpr[22])
#define TM_S7 (entry->cpu->gpr[23])
#define TM_T8 (entry->cpu->gpr[24])
#define TM_T9 (entry->cpu->gpr[25])
#define TM_SP (entry->cpu->gpr[29])

static void battle_triangle_max_vertices(BattleEntryCall *entry)
{
    TM_T0=TM_T4&0xffffu;TM_T0<<=3;TM_T0+=TM_S0;
    MTC2(TM_R(TM_T0),0);MTC2(TM_R(TM_T0+4u),1);
    TM_T0=(TM_T4>>13)&0xfff8u;TM_T0+=TM_S0;
    MTC2(TM_R(TM_T0),2);MTC2(TM_R(TM_T0+4u),3);
    TM_T0=TM_T5<<3;TM_T0+=TM_S0;
    MTC2(TM_R(TM_T0),4);MTC2(TM_R(TM_T0+4u),5);
}
static void battle_triangle_max_run(BattleEntryCall *entry,uint32_t target)
{
    switch(target){
    case 0x8002e448u:TM_T9=12;TM_T8=0x09000000u;TM_A3=0x28u;break;
    case 0x8002e45cu:TM_T9=8;TM_T8=0x06000000u;TM_A3=0x1cu;break;
    case 0x8002e470u:TM_T9=4;TM_T8=0x04000000u;TM_A3=0x14u;break;
    default:TM_T9=8;TM_T8=0x07000000u;TM_A3=0x20u;break;
    }
    TM_W(TM_SP-4u,TM_S0);TM_W(TM_SP-8u,TM_S1);TM_W(TM_SP-0x10u,TM_S2);
    TM_W(TM_SP-0x14u,TM_S3);TM_W(TM_SP-0x18u,TM_S4);TM_W(TM_SP-0x1cu,TM_S5);
    TM_W(TM_SP-0x20u,TM_S6);TM_W(TM_SP-0x24u,TM_S7);
    TM_S0=0x80060000u;uint32_t p=TM_R(TM_S0-0x6ac4u);TM_S2=0x80060000u;TM_S0=p;
    p=TM_R(TM_S2-0x6a88u);TM_S3=0x80060000u;TM_S2=p;
    p=TM_R(TM_S3-0x6bdcu);TM_S4=0x80060000u;TM_S3=p;
    p=TM_R(TM_S4-0x6a98u);TM_V0=0x80050000u;TM_S4=p;
    p=TM_R(TM_V0+0xfcu);TM_V1=0x80050000u;TM_V0=p;
    p=TM_R(TM_V1+0xf8u);TM_T7=0x80050000u;TM_V1=p;
    p=TM_R(TM_T7+0x100u);uint32_t q=TM_R(TM_A0);TM_T7=p;
    p=TM_H(TM_A0+4u);TM_T4=q;
    /* ADDI faults before committing the pending third-index load. */
    if(TM_T7==0x7ffffffeu||TM_T7==0x7fffffffu)
        battle_entry_fail(entry,"triangle maximum depth shift overflow",0x8002e4f0u);
    TM_T7+=2u;TM_T5=p;
    battle_triangle_max_vertices(entry);TM_S3-=TM_A3;TM_S6=0x00ffffffu;
    int pending_t4=0;uint32_t next_t4=0;
    for(;;){
        doCOP2(0x4a280030); /* RTPT, including final lookahead. */
        if(pending_t4){TM_T4=next_t4;pending_t4=0;}
        int empty=TM_A1==0;TM_A1--;if(empty)break;
        TM_A0+=8u;p=TM_R(TM_A0);q=TM_H(TM_A0+4u);TM_T4=p;TM_S3+=TM_A3;TM_T5=q;
        battle_triangle_max_vertices(entry);
        p=MFC2(31);q=MFC2(12);TM_T0=p;int negative=(int32_t)TM_T0<0;TM_T1=q;TM_T0=TM_T1<TM_V0;
        if(negative)continue;
        p=MFC2(13);q=MFC2(14);TM_T2=p;doCOP2(0x4b400006);TM_T3=q; /* NCLIP */
        int inside=TM_T0!=0;TM_T0=TM_T2<TM_V0;
        if(!inside){inside=TM_T0!=0;TM_T0=TM_T3<TM_V0;if(!inside)inside=TM_T0!=0;}
        TM_T0=TM_T1&0xffffu;if(!inside)continue;
        TM_T0=TM_T0<TM_V1;inside=TM_T0!=0;TM_T0=TM_T2&0xffffu;
        if(!inside){TM_T0=TM_T0<TM_V1;inside=TM_T0!=0;TM_T0=TM_T3&0xffffu;
            if(!inside){TM_T0=TM_T0<TM_V1;inside=TM_T0!=0;}}
        p=MFC2(24);
        if(!inside){pending_t4=1;next_t4=p;continue;}
        q=MFC2(18);TM_T4=p;int back=(int32_t)TM_T4<=0;TM_T5=q;
        TM_W(TM_S3+8u,TM_T1); /* Backface branch delay: XY0 is always written. */
        if(back)continue;
        TM_T0=TM_S3+TM_T9;TM_W(TM_T0+8u,TM_T2);TM_T0+=TM_T9;TM_W(TM_T0+8u,TM_T3);
        p=MFC2(17);q=MFC2(19);TM_T0=p;
        TM_T3=(int32_t)TM_T5<(int32_t)TM_T0;TM_T2=q;
        int less=TM_T3!=0;TM_S3&=TM_S6;if(!less)TM_T0=TM_T5;
        TM_T3=(int32_t)TM_T2<(int32_t)TM_T0;less=TM_T3!=0;TM_S2++;
        if(!less)TM_T0=TM_T2;
        int zero=TM_T0==0;TM_T0=(uint32_t)((int32_t)TM_T0>>(TM_T7&31u));if(zero)continue;
        TM_T0<<=2;TM_T0+=TM_S4;p=TM_R(TM_T0);TM_W(TM_T0,TM_S3);TM_T1=p;TM_T1|=TM_T8;
        TM_W(TM_S3,TM_T1);
    }
    TM_S3+=TM_A3;TM_AT=0x80060000u;TM_W(TM_AT-0x6a88u,TM_S2);
    TM_AT=0x80060000u;TM_W(TM_AT-0x6bdcu,TM_S3);
    p=TM_R(TM_SP-4u);q=TM_R(TM_SP-8u);TM_S0=p;
    p=TM_R(TM_SP-0x10u);TM_S1=q;q=TM_R(TM_SP-0x14u);TM_S2=p;
    p=TM_R(TM_SP-0x18u);TM_S3=q;q=TM_R(TM_SP-0x1cu);TM_S4=p;
    p=TM_R(TM_SP-0x20u);TM_S5=q;TM_S6=p;
    q=TM_R(TM_SP-0x24u);
    /* The original return delay slot loads S7. Keep that pending at the
     * caller boundary; the native packet caller commits it after its next
     * successful load, preserving the first post-return fault state. */
    entry->cpu->load_valid=1;entry->cpu->load_reg=23;entry->cpu->load_value=q;entry->cpu->load_mask=0;
}
static int battle_triangle_max_target(uint32_t target)
{return target==0x8002e448u||target==0x8002e45cu||target==0x8002e470u||target==0x8002e484u;}
static int battle_triangle_max_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_triangle_max_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TRIANGLE_MAX");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    const uint32_t starts[]={0x8002e448u,0x8002e1f4u};const size_t lengths[]={516u,56u};
    const char *expected[]={"a4abde1913bc632d5554d059c3d9bdabe1860ea71f671dfeec4490e7b72d10fd","b7d98d849c2248e0da88f393f8e57a1f9b42df3c4d568ab3c194b01b91f5e2de"};
    if(!image)return 0;
    for(unsigned span=0;span<2;span++){
        size_t offset=XENO_RD_SLUS_OFF(starts[span]);if(image->size<offset+lengths[span])return 0;
        uint8_t hash[32];file1_digest(image->bytes+offset,lengths[span],hash);
        for(unsigned i=0;i<32;i++)if(file1_hex_digit(hash[i]>>4)!=expected[span][i*2]||file1_hex_digit(hash[i]&15u)!=expected[span][i*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_triangle_max_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef TM_R
#undef TM_H
#undef TM_W
#undef TM_AT
#undef TM_V0
#undef TM_V1
#undef TM_A0
#undef TM_A1
#undef TM_A3
#undef TM_T0
#undef TM_T1
#undef TM_T2
#undef TM_T3
#undef TM_T4
#undef TM_T5
#undef TM_T7
#undef TM_S0
#undef TM_S1
#undef TM_S2
#undef TM_S3
#undef TM_S4
#undef TM_S5
#undef TM_S6
#undef TM_S7
#undef TM_T8
#undef TM_T9
#undef TM_SP
