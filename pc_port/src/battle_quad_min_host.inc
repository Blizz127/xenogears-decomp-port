/* Four native minimum-SZ quad entries (packet table variant 3, rows 8..15)
 * share EB00..ED20 and the noncontiguous E1F4..E22C return tail.  The retail
 * bytes are battle_quad_max_host.inc's E64C..E8B4 with the three SZ
 * selection branches inverted (BNEZ -> BEQZ: the OT depth is the smallest SZ).
 * Guest packet links and pointers stay words; only the GTE backend is called. */
/* These routines intentionally form physical RAM addresses by masking the
 * packet pointer to 24 bits. Use the existing explicit-pointer conversion;
 * the generic mixed guest/host bus deliberately does not infer this domain. */
#define QN_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define QN_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define QN_W(a,v) battle_entry_write(entry,shard_sdk_address(a),4,(v))
#define QN_AT (entry->cpu->gpr[1])
#define QN_V0 (entry->cpu->gpr[2])
#define QN_V1 (entry->cpu->gpr[3])
#define QN_A0 (entry->cpu->gpr[4])
#define QN_A1 (entry->cpu->gpr[5])
#define QN_A3 (entry->cpu->gpr[7])
#define QN_T0 (entry->cpu->gpr[8])
#define QN_T1 (entry->cpu->gpr[9])
#define QN_T2 (entry->cpu->gpr[10])
#define QN_T3 (entry->cpu->gpr[11])
#define QN_T4 (entry->cpu->gpr[12])
#define QN_T5 (entry->cpu->gpr[13])
#define QN_T6 (entry->cpu->gpr[14])
#define QN_T7 (entry->cpu->gpr[15])
#define QN_S0 (entry->cpu->gpr[16])
#define QN_S1 (entry->cpu->gpr[17])
#define QN_S2 (entry->cpu->gpr[18])
#define QN_S3 (entry->cpu->gpr[19])
#define QN_S4 (entry->cpu->gpr[20])
#define QN_S5 (entry->cpu->gpr[21])
#define QN_S6 (entry->cpu->gpr[22])
#define QN_S7 (entry->cpu->gpr[23])
#define QN_T8 (entry->cpu->gpr[24])
#define QN_T9 (entry->cpu->gpr[25])
#define QN_SP (entry->cpu->gpr[29])

static void battle_quad_min_vertices(BattleEntryCall *entry)
{
    QN_T6=QN_T4&0xffffu;QN_T6<<=3;QN_T6+=QN_S0;
    QN_T0=(QN_T4>>13)&0xfff8u;QN_T0+=QN_S0;
    MTC2(QN_R(QN_T0),2);MTC2(QN_R(QN_T0+4u),3);
    QN_T0=QN_T5<<3;QN_T0+=QN_S0;
    MTC2(QN_R(QN_T0),4);MTC2(QN_R(QN_T0+4u),5);
}
static void battle_quad_min_run(BattleEntryCall *entry,uint32_t target)
{
    switch(target){
    case 0x8002eab8u:QN_T9=12;QN_T8=0x0c000000u;QN_A3=0x34u;break;
    case 0x8002eaccu:QN_T9=8;QN_T8=0x08000000u;QN_A3=0x24u;break;
    case 0x8002eae0u:QN_T9=4;QN_T8=0x05000000u;QN_A3=0x18u;break;
    default:QN_T9=8;QN_T8=0x09000000u;QN_A3=0x28u;break;
    }
    QN_W(QN_SP-4u,QN_S0);QN_W(QN_SP-8u,QN_S1);QN_W(QN_SP-0x10u,QN_S2);
    QN_W(QN_SP-0x14u,QN_S3);QN_W(QN_SP-0x18u,QN_S4);QN_W(QN_SP-0x1cu,QN_S5);
    QN_W(QN_SP-0x20u,QN_S6);QN_W(QN_SP-0x24u,QN_S7);
    QN_S0=0x80060000u;uint32_t p=QN_R(QN_S0-0x6ac4u);QN_S2=0x80060000u;QN_S0=p;
    p=QN_R(QN_S2-0x6a88u);QN_S3=0x80060000u;QN_S2=p;
    p=QN_R(QN_S3-0x6bdcu);QN_S4=0x80060000u;QN_S3=p;
    p=QN_R(QN_S4-0x6a98u);QN_V0=0x80050000u;QN_S4=p;
    p=QN_R(QN_V0+0xfcu);QN_V1=0x80050000u;QN_V0=p;
    p=QN_R(QN_V1+0xf8u);QN_T7=0x80050000u;QN_V1=p;
    p=QN_R(QN_T7+0x100u);uint32_t q=QN_R(QN_A0);QN_T7=p;
    p=QN_H(QN_A0+4u);QN_T4=q;
    /* Signed ADDI faults before the pending third-index load commits. */
    if(QN_T7==0x7ffffffeu||QN_T7==0x7fffffffu)
        battle_entry_fail(entry,"quad minimum depth shift overflow",0x8002eb60u);
    QN_T7+=2u;QN_T5=p;
    battle_quad_min_vertices(entry);QN_S3-=QN_A3;QN_S6=0x00ffffffu;
    unsigned pending_reg=0;uint32_t pending_value=0;
    for(;;){
        MTC2(QN_R(QN_T6),0);
        if(pending_reg){entry->cpu->gpr[pending_reg]=pending_value;pending_reg=0;}
        MTC2(QN_R(QN_T6+4u),1);
        int empty=QN_A1==0;
        doCOP2(0x4a280030); /* RTPT in count-branch delay, including lookahead. */
        if(empty)break;
        QN_A1--;QN_A0+=8u;p=QN_R(QN_A0);q=QN_H(QN_A0+4u);QN_T4=p;QN_S3+=QN_A3;QN_T5=q;
        battle_quad_min_vertices(entry);
        p=MFC2(31);q=MFC2(12);QN_T0=p;
        int negative=(int32_t)QN_T0<0;QN_T1=q;p=MFC2(13);
        if(negative){pending_reg=10;pending_value=p;continue;}
        q=MFC2(14);QN_T2=p;doCOP2(0x4b400006);QN_T3=q; /* NCLIP before SXY2 commit. */
        p=QN_H(QN_A0-2u);q=MFC2(24);QN_T5=p;QN_T0=QN_T5<<3;QN_T4=q;
        int back=(int32_t)QN_T4<=0;QN_T0+=QN_S0;if(back)continue;
        MTC2(QN_R(QN_T0),0);MTC2(QN_R(QN_T0+4u),1);doCOP2(0x4a180001); /* RTPS */
        p=MFC2(31);q=MFC2(14);QN_T0=p;negative=(int32_t)QN_T0<0;QN_T4=q;
        QN_T0=QN_T1<QN_V0;if(negative)continue;
        int inside=QN_T0!=0;QN_T0=QN_T2<QN_V0;
        if(!inside){inside=QN_T0!=0;QN_T0=QN_T3<QN_V0;
            if(!inside){inside=QN_T0!=0;QN_T0=QN_T4<QN_V0;if(!inside)inside=QN_T0!=0;}}
        QN_T0=QN_T1&0xffffu;if(!inside)continue;
        QN_T0=QN_T0<QN_V1;inside=QN_T0!=0;QN_T0=QN_T2&0xffffu;
        if(!inside){QN_T0=QN_T0<QN_V1;inside=QN_T0!=0;QN_T0=QN_T3&0xffffu;
            if(!inside){QN_T0=QN_T0<QN_V1;inside=QN_T0!=0;QN_T0=QN_T4&0xffffu;
                if(!inside){QN_T0=QN_T0<QN_V1;inside=QN_T0!=0;}}}
        QN_W(QN_S3+8u,QN_T1); /* Final x-reject branch delay writes XY0. */
        if(!inside)continue;
        QN_T0=QN_S3+QN_T9;QN_W(QN_T0+8u,QN_T2);QN_T0+=QN_T9;
        p=MFC2(16);QN_W(QN_T0+8u,QN_T3);QN_T1=p;
        int zero=QN_T1==0;QN_T0+=QN_T9;if(zero)continue;
        p=MFC2(17);QN_W(QN_T0+8u,QN_T4);QN_T2=p;
        zero=QN_T2==0;QN_T0=(int32_t)QN_T2<(int32_t)QN_T1;if(zero)continue;
        int less=QN_T0!=0;p=MFC2(18);if(less)QN_T1=QN_T2;QN_T3=p; /* BEQZ: keep the smaller SZ */
        zero=QN_T3==0;p=MFC2(19);
        if(zero){pending_reg=12;pending_value=p;continue;}
        QN_T0=(int32_t)QN_T3<(int32_t)QN_T1;QN_T4=p;
        less=QN_T0!=0;QN_S3&=QN_S6;if(less)QN_T1=QN_T3;
        zero=QN_T4==0;QN_T2=(int32_t)QN_T4<(int32_t)QN_T1;if(zero)continue;
        less=QN_T2!=0;QN_S2++;if(less)QN_T1=QN_T4;
        zero=QN_T1==0;QN_T0=(uint32_t)((int32_t)QN_T1>>(QN_T7&31u));if(zero)continue;
        QN_T0<<=2;QN_T0+=QN_S4;p=QN_R(QN_T0);QN_W(QN_T0,QN_S3);QN_T1=p;QN_T1|=QN_T8;
        QN_W(QN_S3,QN_T1);
    }
    QN_S3+=QN_A3;QN_AT=0x80060000u;QN_W(QN_AT-0x6a88u,QN_S2);
    QN_AT=0x80060000u;QN_W(QN_AT-0x6bdcu,QN_S3);
    p=QN_R(QN_SP-4u);q=QN_R(QN_SP-8u);QN_S0=p;
    p=QN_R(QN_SP-0x10u);QN_S1=q;q=QN_R(QN_SP-0x14u);QN_S2=p;
    p=QN_R(QN_SP-0x18u);QN_S3=q;q=QN_R(QN_SP-0x1cu);QN_S4=p;
    p=QN_R(QN_SP-0x20u);QN_S5=q;QN_S6=p;
    q=QN_R(QN_SP-0x24u);
    /* The original return delay slot loads S7. Keep that pending at the
     * caller boundary; the native packet caller commits it after its next
     * successful load, preserving the first post-return fault state. */
    entry->cpu->load_valid=1;entry->cpu->load_reg=23;entry->cpu->load_value=q;entry->cpu->load_mask=0;
}
static int battle_quad_min_target(uint32_t target)
{return target==0x8002eab8u||target==0x8002eaccu||target==0x8002eae0u||target==0x8002eaf4u;}
static int battle_quad_min_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_quad_min_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_QUAD_MIN");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    const uint32_t starts[]={0x8002eab8u,0x8002e1f4u};const size_t lengths[]={616u,56u};
    const char *expected[]={"ce239c09488fb372186a176eccfb1c2dcffea56e5991be014d5a6f0f2b4ab230","b7d98d849c2248e0da88f393f8e57a1f9b42df3c4d568ab3c194b01b91f5e2de"};
    if(!image)return 0;
    for(unsigned span=0;span<2;span++){
        size_t offset=XENO_RD_SLUS_OFF(starts[span]);if(image->size<offset+lengths[span])return 0;
        uint8_t hash[32];file1_digest(image->bytes+offset,lengths[span],hash);
        for(unsigned i=0;i<32;i++)if(file1_hex_digit(hash[i]>>4)!=expected[span][i*2]||file1_hex_digit(hash[i]&15u)!=expected[span][i*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_quad_min_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef QN_R
#undef QN_H
#undef QN_W
#undef QN_AT
#undef QN_V0
#undef QN_V1
#undef QN_A0
#undef QN_A1
#undef QN_A3
#undef QN_T0
#undef QN_T1
#undef QN_T2
#undef QN_T3
#undef QN_T4
#undef QN_T5
#undef QN_T6
#undef QN_T7
#undef QN_S0
#undef QN_S1
#undef QN_S2
#undef QN_S3
#undef QN_S4
#undef QN_S5
#undef QN_S6
#undef QN_S7
#undef QN_T8
#undef QN_T9
#undef QN_SP
