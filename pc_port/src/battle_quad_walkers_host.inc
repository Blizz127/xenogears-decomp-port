/* Four native quad entries share E274..E448 and the E1F4..E22C return tail.
 * Guest packet links and pointers stay words; only the GTE backend is called. */
/* These routines intentionally form physical RAM addresses by masking the
 * packet pointer to 24 bits. Use the existing explicit-pointer conversion;
 * the generic mixed guest/host bus deliberately does not infer this domain. */
#define QW_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define QW_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define QW_W(a,v) battle_entry_write(entry,shard_sdk_address(a),4,(v))
#define QW_AT (entry->cpu->gpr[1])
#define QW_V0 (entry->cpu->gpr[2])
#define QW_V1 (entry->cpu->gpr[3])
#define QW_A0 (entry->cpu->gpr[4])
#define QW_A1 (entry->cpu->gpr[5])
#define QW_A3 (entry->cpu->gpr[7])
#define QW_T0 (entry->cpu->gpr[8])
#define QW_T1 (entry->cpu->gpr[9])
#define QW_T2 (entry->cpu->gpr[10])
#define QW_T3 (entry->cpu->gpr[11])
#define QW_T4 (entry->cpu->gpr[12])
#define QW_T5 (entry->cpu->gpr[13])
#define QW_T6 (entry->cpu->gpr[14])
#define QW_T7 (entry->cpu->gpr[15])
#define QW_S0 (entry->cpu->gpr[16])
#define QW_S1 (entry->cpu->gpr[17])
#define QW_S2 (entry->cpu->gpr[18])
#define QW_S3 (entry->cpu->gpr[19])
#define QW_S4 (entry->cpu->gpr[20])
#define QW_S5 (entry->cpu->gpr[21])
#define QW_S6 (entry->cpu->gpr[22])
#define QW_S7 (entry->cpu->gpr[23])
#define QW_T8 (entry->cpu->gpr[24])
#define QW_T9 (entry->cpu->gpr[25])
#define QW_SP (entry->cpu->gpr[29])

static void battle_quad_vertices(BattleEntryCall *entry)
{
    QW_T6=QW_T4&0xffffu;QW_T6<<=3;QW_T6+=QW_S0;
    QW_T0=(QW_T4>>13)&0xfff8u;QW_T0+=QW_S0;
    MTC2(QW_R(QW_T0),2);MTC2(QW_R(QW_T0+4u),3);
    QW_T0=QW_T5<<3;QW_T0+=QW_S0;
    MTC2(QW_R(QW_T0),4);MTC2(QW_R(QW_T0+4u),5);
}
static void battle_quad_walkers_run(BattleEntryCall *entry,uint32_t target)
{
    switch(target){
    case 0x8002e22cu:QW_T9=12;QW_T8=0x0c000000u;QW_A3=0x34u;break;
    case 0x8002e240u:QW_T9=8;QW_T8=0x08000000u;QW_A3=0x24u;break;
    case 0x8002e254u:QW_T9=4;QW_T8=0x05000000u;QW_A3=0x18u;break;
    default:QW_T9=8;QW_T8=0x09000000u;QW_A3=0x28u;break;
    }
    QW_W(QW_SP-4u,QW_S0);QW_W(QW_SP-8u,QW_S1);QW_W(QW_SP-0x10u,QW_S2);
    QW_W(QW_SP-0x14u,QW_S3);QW_W(QW_SP-0x18u,QW_S4);QW_W(QW_SP-0x1cu,QW_S5);
    QW_W(QW_SP-0x20u,QW_S6);QW_W(QW_SP-0x24u,QW_S7);
    QW_S0=0x80060000u;uint32_t p=QW_R(QW_S0-0x6ac4u);QW_S2=0x80060000u;QW_S0=p;
    p=QW_R(QW_S2-0x6a88u);QW_S3=0x80060000u;QW_S2=p;
    p=QW_R(QW_S3-0x6bdcu);QW_S4=0x80060000u;QW_S3=p;
    p=QW_R(QW_S4-0x6a98u);QW_V0=0x80050000u;QW_S4=p;
    p=QW_R(QW_V0+0xfcu);QW_V1=0x80050000u;QW_V0=p;
    p=QW_R(QW_V1+0xf8u);QW_T7=0x80050000u;QW_V1=p;
    p=QW_R(QW_T7+0x100u);uint32_t q=QW_R(QW_A0);QW_T7=p;
    p=QW_H(QW_A0+4u);QW_T4=q;QW_T6=QW_T4&0xffffu;QW_T5=p;
    battle_quad_vertices(entry);QW_S3-=QW_A3;QW_S6=0x00ffffffu;
    int pending_t0=0;uint32_t next_t0=0;
    for(;;){
        MTC2(QW_R(QW_T6),0);
        if(pending_t0){QW_T0=next_t0;pending_t0=0;}
        MTC2(QW_R(QW_T6+4u),1);
        int empty=QW_A1==0;
        doCOP2(0x4a280030); /* RTPT in count-branch delay, including lookahead. */
        if(empty)break;
        QW_A1--;QW_A0+=8u;p=QW_R(QW_A0);q=QW_H(QW_A0+4u);QW_T4=p;QW_S3+=QW_A3;QW_T5=q;
        battle_quad_vertices(entry);
        p=MFC2(31);doCOP2(0x4b400006);QW_T0=p; /* NCLIP before LZCR rejection. */
        int negative=(int32_t)QW_T0<0;QW_S3&=QW_S6;if(negative)continue;
        p=QW_H(QW_A0-2u);q=MFC2(12);QW_T0=p;
        p=MFC2(24);QW_T1=q;QW_T0<<=3;QW_T4=p;
        int back=(int32_t)QW_T4<=0;QW_T0+=QW_S0;if(back)continue;
        p=MFC2(13);MTC2(QW_R(QW_T0),0);QW_T2=p;
        MTC2(QW_R(QW_T0+4u),1);
        p=MFC2(14);doCOP2(0x4a180001);QW_T3=p; /* RTPS for the fourth vertex. */
        p=MFC2(31);q=MFC2(14);QW_T0=p;negative=(int32_t)QW_T0<0;QW_T4=q;
        doCOP2(0x4b68002e); /* AVSZ4 is the second LZCR branch delay. */
        if(negative)continue;
        QW_T0=QW_T1<QW_V0;int inside=QW_T0!=0;QW_T0=QW_T2<QW_V0;
        if(!inside){inside=QW_T0!=0;QW_T0=QW_T3<QW_V0;
            if(!inside){inside=QW_T0!=0;QW_T0=QW_T4<QW_V0;if(!inside)inside=QW_T0!=0;}}
        QW_T0=QW_T1&0xffffu;if(!inside)continue;
        QW_T0=QW_T0<QW_V1;inside=QW_T0!=0;QW_T0=QW_T2&0xffffu;
        if(!inside){QW_T0=QW_T0<QW_V1;inside=QW_T0!=0;QW_T0=QW_T3&0xffffu;
            if(!inside){QW_T0=QW_T0<QW_V1;inside=QW_T0!=0;QW_T0=QW_T4&0xffffu;
                if(!inside){QW_T0=QW_T0<QW_V1;inside=QW_T0!=0;}}}
        p=MFC2(7);
        if(!inside){pending_t0=1;next_t0=p;continue;}
        QW_S2++;QW_T0=p;int zero=QW_T0==0;
        QW_T0=(uint32_t)((int32_t)QW_T0>>(QW_T7&31u));if(zero)continue;
        QW_T0<<=2;QW_T0+=QW_S4;p=QW_R(QW_T0);QW_W(QW_T0,QW_S3);QW_T5=p;
        QW_T0=QW_T5|QW_T8;QW_W(QW_S3,QW_T0);
        QW_W(QW_S3+8u,QW_T1);QW_T0=QW_S3+QW_T9;QW_W(QW_T0+8u,QW_T2);
        QW_T0+=QW_T9;QW_W(QW_T0+8u,QW_T3);QW_T0+=QW_T9;QW_W(QW_T0+8u,QW_T4);
    }
    QW_S3+=QW_A3;QW_AT=0x80060000u;QW_W(QW_AT-0x6a88u,QW_S2);
    QW_AT=0x80060000u;QW_W(QW_AT-0x6bdcu,QW_S3);
    p=QW_R(QW_SP-4u);q=QW_R(QW_SP-8u);QW_S0=p;
    p=QW_R(QW_SP-0x10u);QW_S1=q;q=QW_R(QW_SP-0x14u);QW_S2=p;
    p=QW_R(QW_SP-0x18u);QW_S3=q;q=QW_R(QW_SP-0x1cu);QW_S4=p;
    p=QW_R(QW_SP-0x20u);QW_S5=q;QW_S6=p;
    q=QW_R(QW_SP-0x24u);
    /* The original return delay slot loads S7. Keep that pending at the
     * caller boundary; the native packet caller commits it after its next
     * successful load, preserving the first post-return fault state. */
    entry->cpu->load_valid=1;entry->cpu->load_reg=23;entry->cpu->load_value=q;entry->cpu->load_mask=0;
}
static int battle_quad_walkers_target(uint32_t target)
{return target==0x8002e22cu||target==0x8002e240u||target==0x8002e254u||target==0x8002e268u;}
static int battle_quad_walkers_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_quad_walkers_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_QUAD_WALKERS");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(0x8002e1f4u);
    if(!image||image->size<offset+596u)return 0;
    uint8_t digest[32];file1_digest(image->bytes+offset,596u,digest);
    const char *expected="94738cccc1d66bd41df77b55857150939d92bdc5bf7c2aecd22ddb57db5b9105";
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_quad_walkers_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef QW_R
#undef QW_H
#undef QW_W
#undef QW_AT
#undef QW_V0
#undef QW_V1
#undef QW_A0
#undef QW_A1
#undef QW_A3
#undef QW_T0
#undef QW_T1
#undef QW_T2
#undef QW_T3
#undef QW_T4
#undef QW_T5
#undef QW_T6
#undef QW_T7
#undef QW_S0
#undef QW_S1
#undef QW_S2
#undef QW_S3
#undef QW_S4
#undef QW_S5
#undef QW_S6
#undef QW_S7
#undef QW_T8
#undef QW_T9
#undef QW_SP
