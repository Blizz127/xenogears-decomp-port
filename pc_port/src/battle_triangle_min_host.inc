/* Four native minimum-SZ triangle entries (packet table variant 3, rows 0..7)
 * share E8FC..EAB8 and the noncontiguous E1F4..E22C return tail.  The retail
 * bytes are battle_triangle_max_host.inc's E448..E64C with the two SZ
 * selection branches inverted (BNEZ -> BEQZ: the OT depth is the smallest SZ).
 * Guest packet links and pointers stay words; only the GTE backend is called. */
/* These routines intentionally form physical RAM addresses by masking the
 * packet pointer to 24 bits. Use the existing explicit-pointer conversion;
 * the generic mixed guest/host bus deliberately does not infer this domain. */
#define TN_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define TN_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define TN_W(a,v) battle_entry_write(entry,shard_sdk_address(a),4,(v))
#define TN_AT (entry->cpu->gpr[1])
#define TN_V0 (entry->cpu->gpr[2])
#define TN_V1 (entry->cpu->gpr[3])
#define TN_A0 (entry->cpu->gpr[4])
#define TN_A1 (entry->cpu->gpr[5])
#define TN_A3 (entry->cpu->gpr[7])
#define TN_T0 (entry->cpu->gpr[8])
#define TN_T1 (entry->cpu->gpr[9])
#define TN_T2 (entry->cpu->gpr[10])
#define TN_T3 (entry->cpu->gpr[11])
#define TN_T4 (entry->cpu->gpr[12])
#define TN_T5 (entry->cpu->gpr[13])
#define TN_T7 (entry->cpu->gpr[15])
#define TN_S0 (entry->cpu->gpr[16])
#define TN_S1 (entry->cpu->gpr[17])
#define TN_S2 (entry->cpu->gpr[18])
#define TN_S3 (entry->cpu->gpr[19])
#define TN_S4 (entry->cpu->gpr[20])
#define TN_S5 (entry->cpu->gpr[21])
#define TN_S6 (entry->cpu->gpr[22])
#define TN_S7 (entry->cpu->gpr[23])
#define TN_T8 (entry->cpu->gpr[24])
#define TN_T9 (entry->cpu->gpr[25])
#define TN_SP (entry->cpu->gpr[29])

static void battle_triangle_min_vertices(BattleEntryCall *entry)
{
    TN_T0=TN_T4&0xffffu;TN_T0<<=3;TN_T0+=TN_S0;
    MTC2(TN_R(TN_T0),0);MTC2(TN_R(TN_T0+4u),1);
    TN_T0=(TN_T4>>13)&0xfff8u;TN_T0+=TN_S0;
    MTC2(TN_R(TN_T0),2);MTC2(TN_R(TN_T0+4u),3);
    TN_T0=TN_T5<<3;TN_T0+=TN_S0;
    MTC2(TN_R(TN_T0),4);MTC2(TN_R(TN_T0+4u),5);
}
static void battle_triangle_min_run(BattleEntryCall *entry,uint32_t target)
{
    switch(target){
    case 0x8002e8b4u:TN_T9=12;TN_T8=0x09000000u;TN_A3=0x28u;break;
    case 0x8002e8c8u:TN_T9=8;TN_T8=0x06000000u;TN_A3=0x1cu;break;
    case 0x8002e8dcu:TN_T9=4;TN_T8=0x04000000u;TN_A3=0x14u;break;
    default:TN_T9=8;TN_T8=0x07000000u;TN_A3=0x20u;break;
    }
    TN_W(TN_SP-4u,TN_S0);TN_W(TN_SP-8u,TN_S1);TN_W(TN_SP-0x10u,TN_S2);
    TN_W(TN_SP-0x14u,TN_S3);TN_W(TN_SP-0x18u,TN_S4);TN_W(TN_SP-0x1cu,TN_S5);
    TN_W(TN_SP-0x20u,TN_S6);TN_W(TN_SP-0x24u,TN_S7);
    TN_S0=0x80060000u;uint32_t p=TN_R(TN_S0-0x6ac4u);TN_S2=0x80060000u;TN_S0=p;
    p=TN_R(TN_S2-0x6a88u);TN_S3=0x80060000u;TN_S2=p;
    p=TN_R(TN_S3-0x6bdcu);TN_S4=0x80060000u;TN_S3=p;
    p=TN_R(TN_S4-0x6a98u);TN_V0=0x80050000u;TN_S4=p;
    p=TN_R(TN_V0+0xfcu);TN_V1=0x80050000u;TN_V0=p;
    p=TN_R(TN_V1+0xf8u);TN_T7=0x80050000u;TN_V1=p;
    p=TN_R(TN_T7+0x100u);uint32_t q=TN_R(TN_A0);TN_T7=p;
    p=TN_H(TN_A0+4u);TN_T4=q;
    /* ADDI faults before committing the pending third-index load. */
    if(TN_T7==0x7ffffffeu||TN_T7==0x7fffffffu)
        battle_entry_fail(entry,"triangle minimum depth shift overflow",0x8002e95cu);
    TN_T7+=2u;TN_T5=p;
    battle_triangle_min_vertices(entry);TN_S3-=TN_A3;TN_S6=0x00ffffffu;
    int pending_t4=0;uint32_t next_t4=0;
    for(;;){
        doCOP2(0x4a280030); /* RTPT, including final lookahead. */
        if(pending_t4){TN_T4=next_t4;pending_t4=0;}
        int empty=TN_A1==0;TN_A1--;if(empty)break;
        TN_A0+=8u;p=TN_R(TN_A0);q=TN_H(TN_A0+4u);TN_T4=p;TN_S3+=TN_A3;TN_T5=q;
        battle_triangle_min_vertices(entry);
        p=MFC2(31);q=MFC2(12);TN_T0=p;int negative=(int32_t)TN_T0<0;TN_T1=q;TN_T0=TN_T1<TN_V0;
        if(negative)continue;
        p=MFC2(13);q=MFC2(14);TN_T2=p;doCOP2(0x4b400006);TN_T3=q; /* NCLIP */
        int inside=TN_T0!=0;TN_T0=TN_T2<TN_V0;
        if(!inside){inside=TN_T0!=0;TN_T0=TN_T3<TN_V0;if(!inside)inside=TN_T0!=0;}
        TN_T0=TN_T1&0xffffu;if(!inside)continue;
        TN_T0=TN_T0<TN_V1;inside=TN_T0!=0;TN_T0=TN_T2&0xffffu;
        if(!inside){TN_T0=TN_T0<TN_V1;inside=TN_T0!=0;TN_T0=TN_T3&0xffffu;
            if(!inside){TN_T0=TN_T0<TN_V1;inside=TN_T0!=0;}}
        p=MFC2(24);
        if(!inside){pending_t4=1;next_t4=p;continue;}
        q=MFC2(18);TN_T4=p;int back=(int32_t)TN_T4<=0;TN_T5=q;
        TN_W(TN_S3+8u,TN_T1); /* Backface branch delay: XY0 is always written. */
        if(back)continue;
        TN_T0=TN_S3+TN_T9;TN_W(TN_T0+8u,TN_T2);TN_T0+=TN_T9;TN_W(TN_T0+8u,TN_T3);
        p=MFC2(17);q=MFC2(19);TN_T0=p;
        TN_T3=(int32_t)TN_T5<(int32_t)TN_T0;TN_T2=q;
        int less=TN_T3!=0;TN_S3&=TN_S6;if(less)TN_T0=TN_T5; /* BEQZ: keep the smaller SZ */
        TN_T3=(int32_t)TN_T2<(int32_t)TN_T0;less=TN_T3!=0;TN_S2++;
        if(less)TN_T0=TN_T2;
        int zero=TN_T0==0;TN_T0=(uint32_t)((int32_t)TN_T0>>(TN_T7&31u));if(zero)continue;
        TN_T0<<=2;TN_T0+=TN_S4;p=TN_R(TN_T0);TN_W(TN_T0,TN_S3);TN_T1=p;TN_T1|=TN_T8;
        TN_W(TN_S3,TN_T1);
    }
    TN_S3+=TN_A3;TN_AT=0x80060000u;TN_W(TN_AT-0x6a88u,TN_S2);
    TN_AT=0x80060000u;TN_W(TN_AT-0x6bdcu,TN_S3);
    p=TN_R(TN_SP-4u);q=TN_R(TN_SP-8u);TN_S0=p;
    p=TN_R(TN_SP-0x10u);TN_S1=q;q=TN_R(TN_SP-0x14u);TN_S2=p;
    p=TN_R(TN_SP-0x18u);TN_S3=q;q=TN_R(TN_SP-0x1cu);TN_S4=p;
    p=TN_R(TN_SP-0x20u);TN_S5=q;TN_S6=p;
    q=TN_R(TN_SP-0x24u);
    /* The original return delay slot loads S7. Keep that pending at the
     * caller boundary; the native packet caller commits it after its next
     * successful load, preserving the first post-return fault state. */
    entry->cpu->load_valid=1;entry->cpu->load_reg=23;entry->cpu->load_value=q;entry->cpu->load_mask=0;
}
static int battle_triangle_min_target(uint32_t target)
{return target==0x8002e8b4u||target==0x8002e8c8u||target==0x8002e8dcu||target==0x8002e8f0u;}
static int battle_triangle_min_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_triangle_min_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TRIANGLE_MIN");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    const uint32_t starts[]={0x8002e8b4u,0x8002e1f4u};const size_t lengths[]={516u,56u};
    const char *expected[]={"506637b53b02a8a0aff20d28f34632ca4dba75b4a4ad987c32dc89fc5d69e6fc","b7d98d849c2248e0da88f393f8e57a1f9b42df3c4d568ab3c194b01b91f5e2de"};
    if(!image)return 0;
    for(unsigned span=0;span<2;span++){
        size_t offset=XENO_RD_SLUS_OFF(starts[span]);if(image->size<offset+lengths[span])return 0;
        uint8_t hash[32];file1_digest(image->bytes+offset,lengths[span],hash);
        for(unsigned i=0;i<32;i++)if(file1_hex_digit(hash[i]>>4)!=expected[span][i*2]||file1_hex_digit(hash[i]&15u)!=expected[span][i*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_triangle_min_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef TN_R
#undef TN_H
#undef TN_W
#undef TN_AT
#undef TN_V0
#undef TN_V1
#undef TN_A0
#undef TN_A1
#undef TN_A3
#undef TN_T0
#undef TN_T1
#undef TN_T2
#undef TN_T3
#undef TN_T4
#undef TN_T5
#undef TN_T7
#undef TN_S0
#undef TN_S1
#undef TN_S2
#undef TN_S3
#undef TN_S4
#undef TN_S5
#undef TN_S6
#undef TN_S7
#undef TN_T8
#undef TN_T9
#undef TN_SP
