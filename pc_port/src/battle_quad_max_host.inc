/* Four native maximum-SZ quad entries share E694..E8B4 and the
 * noncontiguous E1F4..E22C return tail.
 * Guest packet links and pointers stay words; only the GTE backend is called. */
/* These routines intentionally form physical RAM addresses by masking the
 * packet pointer to 24 bits. Use the existing explicit-pointer conversion;
 * the generic mixed guest/host bus deliberately does not infer this domain. */
#define QM_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define QM_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define QM_W(a,v) battle_entry_write(entry,shard_sdk_address(a),4,(v))
#define QM_AT (entry->cpu->gpr[1])
#define QM_V0 (entry->cpu->gpr[2])
#define QM_V1 (entry->cpu->gpr[3])
#define QM_A0 (entry->cpu->gpr[4])
#define QM_A1 (entry->cpu->gpr[5])
#define QM_A3 (entry->cpu->gpr[7])
#define QM_T0 (entry->cpu->gpr[8])
#define QM_T1 (entry->cpu->gpr[9])
#define QM_T2 (entry->cpu->gpr[10])
#define QM_T3 (entry->cpu->gpr[11])
#define QM_T4 (entry->cpu->gpr[12])
#define QM_T5 (entry->cpu->gpr[13])
#define QM_T6 (entry->cpu->gpr[14])
#define QM_T7 (entry->cpu->gpr[15])
#define QM_S0 (entry->cpu->gpr[16])
#define QM_S1 (entry->cpu->gpr[17])
#define QM_S2 (entry->cpu->gpr[18])
#define QM_S3 (entry->cpu->gpr[19])
#define QM_S4 (entry->cpu->gpr[20])
#define QM_S5 (entry->cpu->gpr[21])
#define QM_S6 (entry->cpu->gpr[22])
#define QM_S7 (entry->cpu->gpr[23])
#define QM_T8 (entry->cpu->gpr[24])
#define QM_T9 (entry->cpu->gpr[25])
#define QM_SP (entry->cpu->gpr[29])

static void battle_quad_max_vertices(BattleEntryCall *entry)
{
    QM_T6=QM_T4&0xffffu;QM_T6<<=3;QM_T6+=QM_S0;
    QM_T0=(QM_T4>>13)&0xfff8u;QM_T0+=QM_S0;
    MTC2(QM_R(QM_T0),2);MTC2(QM_R(QM_T0+4u),3);
    QM_T0=QM_T5<<3;QM_T0+=QM_S0;
    MTC2(QM_R(QM_T0),4);MTC2(QM_R(QM_T0+4u),5);
}
static void battle_quad_max_run(BattleEntryCall *entry,uint32_t target)
{
    switch(target){
    case 0x8002e64cu:QM_T9=12;QM_T8=0x0c000000u;QM_A3=0x34u;break;
    case 0x8002e660u:QM_T9=8;QM_T8=0x08000000u;QM_A3=0x24u;break;
    case 0x8002e674u:QM_T9=4;QM_T8=0x05000000u;QM_A3=0x18u;break;
    default:QM_T9=8;QM_T8=0x09000000u;QM_A3=0x28u;break;
    }
    QM_W(QM_SP-4u,QM_S0);QM_W(QM_SP-8u,QM_S1);QM_W(QM_SP-0x10u,QM_S2);
    QM_W(QM_SP-0x14u,QM_S3);QM_W(QM_SP-0x18u,QM_S4);QM_W(QM_SP-0x1cu,QM_S5);
    QM_W(QM_SP-0x20u,QM_S6);QM_W(QM_SP-0x24u,QM_S7);
    QM_S0=0x80060000u;uint32_t p=QM_R(QM_S0-0x6ac4u);QM_S2=0x80060000u;QM_S0=p;
    p=QM_R(QM_S2-0x6a88u);QM_S3=0x80060000u;QM_S2=p;
    p=QM_R(QM_S3-0x6bdcu);QM_S4=0x80060000u;QM_S3=p;
    p=QM_R(QM_S4-0x6a98u);QM_V0=0x80050000u;QM_S4=p;
    p=QM_R(QM_V0+0xfcu);QM_V1=0x80050000u;QM_V0=p;
    p=QM_R(QM_V1+0xf8u);QM_T7=0x80050000u;QM_V1=p;
    p=QM_R(QM_T7+0x100u);uint32_t q=QM_R(QM_A0);QM_T7=p;
    p=QM_H(QM_A0+4u);QM_T4=q;
    /* Signed ADDI faults before the pending third-index load commits. */
    if(QM_T7==0x7ffffffeu||QM_T7==0x7fffffffu)
        battle_entry_fail(entry,"quad maximum depth shift overflow",0x8002e6f4u);
    QM_T7+=2u;QM_T5=p;
    battle_quad_max_vertices(entry);QM_S3-=QM_A3;QM_S6=0x00ffffffu;
    unsigned pending_reg=0;uint32_t pending_value=0;
    for(;;){
        MTC2(QM_R(QM_T6),0);
        if(pending_reg){entry->cpu->gpr[pending_reg]=pending_value;pending_reg=0;}
        MTC2(QM_R(QM_T6+4u),1);
        int empty=QM_A1==0;
        doCOP2(0x4a280030); /* RTPT in count-branch delay, including lookahead. */
        if(empty)break;
        QM_A1--;QM_A0+=8u;p=QM_R(QM_A0);q=QM_H(QM_A0+4u);QM_T4=p;QM_S3+=QM_A3;QM_T5=q;
        battle_quad_max_vertices(entry);
        p=MFC2(31);q=MFC2(12);QM_T0=p;
        int negative=(int32_t)QM_T0<0;QM_T1=q;p=MFC2(13);
        if(negative){pending_reg=10;pending_value=p;continue;}
        q=MFC2(14);QM_T2=p;doCOP2(0x4b400006);QM_T3=q; /* NCLIP before SXY2 commit. */
        p=QM_H(QM_A0-2u);q=MFC2(24);QM_T5=p;QM_T0=QM_T5<<3;QM_T4=q;
        int back=(int32_t)QM_T4<=0;QM_T0+=QM_S0;if(back)continue;
        MTC2(QM_R(QM_T0),0);MTC2(QM_R(QM_T0+4u),1);doCOP2(0x4a180001); /* RTPS */
        p=MFC2(31);q=MFC2(14);QM_T0=p;negative=(int32_t)QM_T0<0;QM_T4=q;
        QM_T0=QM_T1<QM_V0;if(negative)continue;
        int inside=QM_T0!=0;QM_T0=QM_T2<QM_V0;
        if(!inside){inside=QM_T0!=0;QM_T0=QM_T3<QM_V0;
            if(!inside){inside=QM_T0!=0;QM_T0=QM_T4<QM_V0;if(!inside)inside=QM_T0!=0;}}
        QM_T0=QM_T1&0xffffu;if(!inside)continue;
        QM_T0=QM_T0<QM_V1;inside=QM_T0!=0;QM_T0=QM_T2&0xffffu;
        if(!inside){QM_T0=QM_T0<QM_V1;inside=QM_T0!=0;QM_T0=QM_T3&0xffffu;
            if(!inside){QM_T0=QM_T0<QM_V1;inside=QM_T0!=0;QM_T0=QM_T4&0xffffu;
                if(!inside){QM_T0=QM_T0<QM_V1;inside=QM_T0!=0;}}}
        QM_W(QM_S3+8u,QM_T1); /* Final x-reject branch delay writes XY0. */
        if(!inside)continue;
        QM_T0=QM_S3+QM_T9;QM_W(QM_T0+8u,QM_T2);QM_T0+=QM_T9;
        p=MFC2(16);QM_W(QM_T0+8u,QM_T3);QM_T1=p;
        int zero=QM_T1==0;QM_T0+=QM_T9;if(zero)continue;
        p=MFC2(17);QM_W(QM_T0+8u,QM_T4);QM_T2=p;
        zero=QM_T2==0;QM_T0=(int32_t)QM_T2<(int32_t)QM_T1;if(zero)continue;
        int less=QM_T0!=0;p=MFC2(18);if(!less)QM_T1=QM_T2;QM_T3=p;
        zero=QM_T3==0;p=MFC2(19);
        if(zero){pending_reg=12;pending_value=p;continue;}
        QM_T0=(int32_t)QM_T3<(int32_t)QM_T1;QM_T4=p;
        less=QM_T0!=0;QM_S3&=QM_S6;if(!less)QM_T1=QM_T3;
        zero=QM_T4==0;QM_T2=(int32_t)QM_T4<(int32_t)QM_T1;if(zero)continue;
        less=QM_T2!=0;QM_S2++;if(!less)QM_T1=QM_T4;
        zero=QM_T1==0;QM_T0=(uint32_t)((int32_t)QM_T1>>(QM_T7&31u));if(zero)continue;
        QM_T0<<=2;QM_T0+=QM_S4;p=QM_R(QM_T0);QM_W(QM_T0,QM_S3);QM_T1=p;QM_T1|=QM_T8;
        QM_W(QM_S3,QM_T1);
    }
    QM_S3+=QM_A3;QM_AT=0x80060000u;QM_W(QM_AT-0x6a88u,QM_S2);
    QM_AT=0x80060000u;QM_W(QM_AT-0x6bdcu,QM_S3);
    p=QM_R(QM_SP-4u);q=QM_R(QM_SP-8u);QM_S0=p;
    p=QM_R(QM_SP-0x10u);QM_S1=q;q=QM_R(QM_SP-0x14u);QM_S2=p;
    p=QM_R(QM_SP-0x18u);QM_S3=q;q=QM_R(QM_SP-0x1cu);QM_S4=p;
    p=QM_R(QM_SP-0x20u);QM_S5=q;QM_S6=p;
    q=QM_R(QM_SP-0x24u);
    /* The original return delay slot loads S7. Keep that pending at the
     * caller boundary; the native packet caller commits it after its next
     * successful load, preserving the first post-return fault state. */
    entry->cpu->load_valid=1;entry->cpu->load_reg=23;entry->cpu->load_value=q;entry->cpu->load_mask=0;
}
static int battle_quad_max_target(uint32_t target)
{return target==0x8002e64cu||target==0x8002e660u||target==0x8002e674u||target==0x8002e688u;}
static int battle_quad_max_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_quad_max_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_QUAD_MAX");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    const uint32_t starts[]={0x8002e64cu,0x8002e1f4u};const size_t lengths[]={616u,56u};
    const char *expected[]={"29af7a34ab11e88385f5b541aaf500ca27d4a73d75a8be98009b6d5189aebf7d","b7d98d849c2248e0da88f393f8e57a1f9b42df3c4d568ab3c194b01b91f5e2de"};
    if(!image)return 0;
    for(unsigned span=0;span<2;span++){
        size_t offset=XENO_RD_SLUS_OFF(starts[span]);if(image->size<offset+lengths[span])return 0;
        uint8_t hash[32];file1_digest(image->bytes+offset,lengths[span],hash);
        for(unsigned i=0;i<32;i++)if(file1_hex_digit(hash[i]>>4)!=expected[span][i*2]||file1_hex_digit(hash[i]&15u)!=expected[span][i*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_quad_max_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef QM_R
#undef QM_H
#undef QM_W
#undef QM_AT
#undef QM_V0
#undef QM_V1
#undef QM_A0
#undef QM_A1
#undef QM_A3
#undef QM_T0
#undef QM_T1
#undef QM_T2
#undef QM_T3
#undef QM_T4
#undef QM_T5
#undef QM_T6
#undef QM_T7
#undef QM_S0
#undef QM_S1
#undef QM_S2
#undef QM_S3
#undef QM_S4
#undef QM_S5
#undef QM_S6
#undef QM_S7
#undef QM_T8
#undef QM_T9
#undef QM_SP
