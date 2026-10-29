/* Battle-context bodies for the lit (variant 1) packet callbacks of the
 * 0x8004FE50 table: FT3 8002F2E0, FT4 8002FAE8, GT3 8002F4B4, F3 8002ED20,
 * G3 8002F6B4 and F4 8002F8D0 (handwritten; all end in the 8002E1F4 tail
 * the triangle walkers share), as the packet-render host calls them from
 * 0x8002C864 (return address 0x8002C86C).
 *
 * The legacy temp2.c bodies read the native LP64 pointer globals, a separate
 * interface the battle path never sets (it writes the canonical guest
 * words).  These read what retail reads -- vertices 0x8005953C, face
 * normals or lighting records 0x80059498, vertex normals 0x8005952C, packet
 * cursor 0x80059424, OT 0x80059568, emitted count
 * 0x80059578, screen bounds 0x800500F8 / 0x800500FC and OT shift 0x80050100
 * -- through the production bus, keep every pointer a guest u32, and drive
 * the GTE backend in the original order: the lookahead RTPT after the last
 * face, the XY stores made before a face is rejected, and the delay-slot
 * work of each branch.  The retail "flag" tests read GTE data register 31
 * (mfc2, LZCR), not FLAG (control register 31), so they never reject; the
 * MFC2(31) reads below keep that.  Retail masks the packet cursor to its 24-bit
 * physical form; shard_sdk_address maps such words back for the bus.
 * Field users keep the legacy bodies; nothing here touches them. */
#define LF_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define LF_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define LF_B(a) battle_entry_read(entry,shard_sdk_address(a),1)
#define LF_W(a,v) battle_entry_write(entry,shard_sdk_address(a),4,(v))
#define LF_AT (entry->cpu->gpr[1])
#define LF_V0 (entry->cpu->gpr[2])
#define LF_V1 (entry->cpu->gpr[3])
#define LF_A0 (entry->cpu->gpr[4])
#define LF_A1 (entry->cpu->gpr[5])
#define LF_A2 (entry->cpu->gpr[6])
#define LF_A3 (entry->cpu->gpr[7])
#define LF_T0 (entry->cpu->gpr[8])
#define LF_T1 (entry->cpu->gpr[9])
#define LF_T2 (entry->cpu->gpr[10])
#define LF_T3 (entry->cpu->gpr[11])
#define LF_T4 (entry->cpu->gpr[12])
#define LF_T5 (entry->cpu->gpr[13])
#define LF_T6 (entry->cpu->gpr[14])
#define LF_T7 (entry->cpu->gpr[15])
#define LF_S0 (entry->cpu->gpr[16])
#define LF_S1 (entry->cpu->gpr[17])
#define LF_S2 (entry->cpu->gpr[18])
#define LF_S3 (entry->cpu->gpr[19])
#define LF_S4 (entry->cpu->gpr[20])
#define LF_S5 (entry->cpu->gpr[21])
#define LF_S6 (entry->cpu->gpr[22])
#define LF_S7 (entry->cpu->gpr[23])
#define LF_T8 (entry->cpu->gpr[24])
#define LF_T9 (entry->cpu->gpr[25])
#define LF_SP (entry->cpu->gpr[29])
#define LF_RTPT 0x4a280030
#define LF_RTPS 0x4a180001
#define LF_NCLIP 0x4b400006
#define LF_AVSZ3 0x4b58002d
#define LF_AVSZ4 0x4b68002e
#define LF_NCS 0x4ac8041e
#define LF_NCT 0x4ad80420
#define LF_NCCS 0x4b08041b
#define LF_NCCT 0x4b18043f

static void battle_lit_ft_save(BattleEntryCall *entry)
{
    LF_W(LF_SP-4u,LF_S0);LF_W(LF_SP-8u,LF_S1);LF_W(LF_SP-0x10u,LF_S2);
    LF_W(LF_SP-0x14u,LF_S3);LF_W(LF_SP-0x18u,LF_S4);LF_W(LF_SP-0x1cu,LF_S5);
    LF_W(LF_SP-0x20u,LF_S6);LF_W(LF_SP-0x24u,LF_S7);
}
/* Retail 8002E1F4..8002E228 with a3 = packet size. */
static void battle_lit_ft_tail(BattleEntryCall *entry)
{
    uint32_t p,q;
    LF_S3+=LF_A3;LF_AT=0x80060000u;LF_W(LF_AT-0x6a88u,LF_S2);
    LF_AT=0x80060000u;LF_W(LF_AT-0x6bdcu,LF_S3);
    p=LF_R(LF_SP-4u);q=LF_R(LF_SP-8u);LF_S0=p;
    p=LF_R(LF_SP-0x10u);LF_S1=q;q=LF_R(LF_SP-0x14u);LF_S2=p;
    p=LF_R(LF_SP-0x18u);LF_S3=q;q=LF_R(LF_SP-0x1cu);LF_S4=p;
    p=LF_R(LF_SP-0x20u);LF_S5=q;LF_S6=p;
    q=LF_R(LF_SP-0x24u);
    /* The return delay slot loads S7: leave it pending at the caller
     * boundary, as the triangle walkers do (battle_packet_render_call). */
    entry->cpu->load_valid=1;entry->cpu->load_reg=23;entry->cpu->load_value=q;entry->cpu->load_mask=0;
}
/* The prologue both share: model words, then the first command's V1 / V2. */
static void battle_lit_ft_begin(BattleEntryCall *entry)
{
    LF_S0=LF_R(0x8005953cu);LF_S2=LF_R(0x80059578u);LF_S3=LF_R(0x80059424u);
    LF_S4=LF_R(0x80059568u);LF_V0=LF_R(0x800500fcu);LF_V1=LF_R(0x800500f8u);
    LF_T7=LF_R(0x80050100u);
}
static void battle_lit_ft_first(BattleEntryCall *entry)
{
    uint32_t p,q;
    p=LF_R(LF_A0);q=LF_H(LF_A0+4u);LF_T4=p;LF_T5=q;
    LF_T6=((LF_T4&0xffffu)<<3)+LF_S0;
    LF_T0=((LF_T4>>13)&0xfff8u)+LF_S0;
    MTC2(LF_R(LF_T0),2);MTC2(LF_R(LF_T0+4u),3);
    LF_T0=(LF_T5<<3)+LF_S0;
    MTC2(LF_R(LF_T0),4);MTC2(LF_R(LF_T0+4u),5);
}
/* 8002F2E0: lit POLY_FT3 (0x20 bytes, tag length 7), one normal per face. */
static void battle_lit_ft3_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    battle_lit_ft_save(entry);battle_lit_ft_begin(entry);
    LF_S6=0x00ffffffu;LF_T8=0x07000000u;LF_T9=LF_R(0x80059498u);
    battle_lit_ft_first(entry);LF_S3-=0x20u;
    for(;;){
        MTC2(LF_R(LF_T6),0);MTC2(LF_R(LF_T6+4u),1);
        doCOP2(LF_RTPT); /* delay slot of the exit test: the lookahead runs */
        if(LF_A1==0)break;
        LF_A1--;LF_A0+=8u;p=LF_R(LF_A0);q=LF_H(LF_A0+4u);LF_T4=p;LF_T5=q;
        LF_S3+=0x20u;LF_T6=((LF_T4&0xffffu)<<3)+LF_S0;LF_T9+=8u;
        LF_T0=((LF_T4>>13)&0xfff8u)+LF_S0;
        MTC2(LF_R(LF_T0),2);MTC2(LF_R(LF_T0+4u),3);
        LF_T1=(LF_T5<<3)+LF_S0;
        MTC2(LF_R(LF_T1),4);MTC2(LF_R(LF_T1+4u),5);
        p=MFC2(31);q=MFC2(12);LF_T0=p;LF_T1=q;
        int negative=(int32_t)LF_T0<0;LF_T0=LF_T1<LF_V0;
        if(negative)continue;
        p=MFC2(13);q=MFC2(14);LF_T2=p;LF_T3=q;doCOP2(LF_NCLIP);
        int inside=LF_T0!=0;LF_T0=LF_T2<LF_V0;
        if(!inside){inside=LF_T0!=0;LF_T0=LF_T3<LF_V0;if(!inside)inside=LF_T0!=0;}
        LF_T0=LF_T1&0xffffu;if(!inside)continue;
        LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T2&0xffffu;
        if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T3&0xffffu;
            if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;}}
        LF_T0=MFC2(24); /* NCLIP result; the x reject's delay slot */
        if(!inside)continue;
        LF_W(LF_S3+8u,LF_T1);
        int back=(int32_t)LF_T0<=0;LF_W(LF_S3+0x10u,LF_T2);
        if(back)continue;
        doCOP2(LF_AVSZ3);LF_W(LF_S3+0x18u,LF_T3);LF_S3&=LF_S6;
        LF_T0=MFC2(7);LF_S2++;
        int zero=LF_T0==0;MTC2(LF_R(LF_T9-8u),0);
        if(zero)continue;
        MTC2(LF_R(LF_T9-4u),1);doCOP2(LF_NCS);
        LF_T0=(uint32_t)((int32_t)LF_T0>>(LF_T7&31u));LF_T0<<=2;LF_T0+=LF_S4;
        p=LF_B(LF_S3+7u);q=MFC2(22);LF_T1=p;LF_T2=q;
        LF_T1<<=24;LF_T2&=LF_S6;LF_T1|=LF_T2;LF_W(LF_S3+4u,LF_T1);
        LF_T1=LF_R(LF_T0);LF_W(LF_T0,LF_S3);LF_T1|=LF_T8;
        LF_W(LF_S3,LF_T1);
    }
    LF_AT=0x80060000u;LF_W(LF_AT-0x6b68u,LF_T9);LF_A3=0x20u;
    battle_lit_ft_tail(entry);
}
/* 8002FAE8: lit POLY_FT4 (0x28 bytes, tag length 9).  The fourth vertex
 * index is the command's halfword +6 and goes through RTPS; later commands
 * keep 13 bits of index 0, the first one 16 (retail masks differ). */
static void battle_lit_ft4_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    LF_T8=0x09000000u;
    battle_lit_ft_save(entry);battle_lit_ft_begin(entry);
    LF_T9=LF_R(0x80059498u);
    battle_lit_ft_first(entry);LF_S3-=0x28u;LF_S6=0x00ffffffu;
    for(;;){
        MTC2(LF_R(LF_T6),0);MTC2(LF_R(LF_T6+4u),1);
        doCOP2(LF_RTPT); /* delay slot of the exit test: the lookahead runs */
        if(LF_A1==0)break;
        LF_A1--;LF_A0+=8u;p=LF_R(LF_A0);q=LF_H(LF_A0+4u);LF_T4=p;LF_T5=q;
        LF_S3+=0x28u;LF_T6=((LF_T4<<3)&0xfff8u)+LF_S0;
        LF_T0=((LF_T4>>13)&0xfff8u)+LF_S0;
        MTC2(LF_R(LF_T0),2);MTC2(LF_R(LF_T0+4u),3);
        LF_T0=(LF_T5<<3)+LF_S0;
        MTC2(LF_R(LF_T0),4);MTC2(LF_R(LF_T0+4u),5);
        LF_T9+=8u;
        p=MFC2(31);doCOP2(LF_NCLIP);LF_T0=p; /* FLAG read before NCLIP */
        int negative=(int32_t)LF_T0<0;LF_S3&=LF_S6;
        if(negative)continue;
        p=LF_H(LF_A0-2u);q=MFC2(12);LF_T0=p;
        uint32_t mac0=MFC2(24);LF_T1=q;
        LF_T0<<=3;LF_T4=mac0;
        int back=(int32_t)LF_T4<=0;LF_T0+=LF_S0;
        if(back)continue;
        p=MFC2(14);q=MFC2(13);LF_T3=p;
        MTC2(LF_R(LF_T0),0);LF_T2=q;MTC2(LF_R(LF_T0+4u),1);
        p=MFC2(14);doCOP2(LF_RTPS);LF_T3=p; /* SXY2 read before RTPS */
        LF_T5=LF_T1<LF_V0;
        p=MFC2(31);q=MFC2(14);LF_T0=p;
        negative=(int32_t)LF_T0<0;doCOP2(LF_AVSZ4);LF_T4=q;
        if(negative)continue;
        int inside=LF_T5!=0;LF_T0=LF_T2<LF_V0;
        if(!inside){inside=LF_T0!=0;LF_T0=LF_T3<LF_V0;
            if(!inside){inside=LF_T0!=0;LF_T0=LF_T4<LF_V0;if(!inside)inside=LF_T0!=0;}}
        LF_T0=LF_T1&0xffffu;if(!inside)continue;
        LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T2&0xffffu;
        if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T3&0xffffu;
            if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T4&0xffffu;
                if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;}}}
        LF_T0=MFC2(7); /* OTZ; the x reject's delay slot */
        if(!inside)continue;
        LF_S2++;
        int zero=LF_T0==0;LF_T0=(uint32_t)((int32_t)LF_T0>>(LF_T7&31u));
        if(zero)continue;
        MTC2(LF_R(LF_T9-8u),0);MTC2(LF_R(LF_T9-4u),1);
        LF_T0<<=2;LF_T0+=LF_S4;doCOP2(LF_NCS);
        p=LF_R(LF_T0);q=LF_B(LF_S3+7u);LF_S1=p;LF_T5=q;
        LF_W(LF_S3+8u,LF_T1);LF_W(LF_S3+0x10u,LF_T2);
        LF_W(LF_S3+0x18u,LF_T3);LF_W(LF_S3+0x20u,LF_T4);
        LF_T2=MFC2(22);LF_T1=LF_T5<<24;LF_T2&=LF_S6;LF_T1|=LF_T2;LF_T2=LF_S1|LF_T8;
        LF_W(LF_S3+4u,LF_T1);LF_W(LF_T0,LF_S3);
        LF_W(LF_S3,LF_T2);
    }
    LF_AT=0x80060000u;LF_W(LF_AT-0x6b68u,LF_T9);LF_A3=0x28u;
    battle_lit_ft_tail(entry);
}
/* 8002F4B4: lit POLY_GT3 (0x28 bytes, tag length 9), row 3 variant 1 of
 * the 0x8004FE50 table.  One normal per vertex: the vertex word plus the
 * normals-minus-vertices delta (0x8005952C - 0x8005953C), staged in the
 * scratchpad at 0x1F800000/4/8 and reloaded for the NCT after the OTZ test.
 * Bounds, OT shift and the 24-bit cursor as the FT3 body; no 0x80059498
 * store (retail keeps the OT shift in t9 here).  Retail's handwritten
 * add/sub (trapping forms) take address deltas that cannot overflow. */
static void battle_lit_gt3_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    battle_lit_ft_save(entry);
    LF_S0=LF_R(0x8005953cu);LF_A3=LF_R(0x8005952cu);LF_S2=LF_R(0x80059578u);
    LF_S3=LF_R(0x80059424u);LF_S4=LF_R(0x80059568u);LF_V0=LF_R(0x800500fcu);
    LF_V1=LF_R(0x800500f8u);LF_T9=LF_R(0x80050100u);
    LF_S6=0x00ffffffu;LF_A2=0x1f800000u;
    p=LF_R(LF_A0);q=LF_H(LF_A0+4u);LF_T4=p;LF_T5=q;
    LF_T6=((LF_T4&0xffffu)<<3)+LF_S0;
    LF_T7=((LF_T4>>13)&0xfff8u)+LF_S0;
    LF_T8=(LF_T5<<3)+LF_S0;
    LF_S3-=0x28u;LF_A3-=LF_S0;
    for(;;){
        MTC2(LF_R(LF_T6),0);MTC2(LF_R(LF_T6+4u),1);
        MTC2(LF_R(LF_T7),2);MTC2(LF_R(LF_T7+4u),3);
        MTC2(LF_R(LF_T8),4);MTC2(LF_R(LF_T8+4u),5);
        doCOP2(LF_RTPT); /* delay slot of the exit test: the lookahead runs */
        if(LF_A1==0)break;
        LF_T6+=LF_A3;LF_T7+=LF_A3;LF_W(LF_A2,LF_T6);LF_T8+=LF_A3;
        LF_W(LF_A2+4u,LF_T7);LF_W(LF_A2+8u,LF_T8);
        LF_A1--;LF_A0+=8u;p=LF_R(LF_A0);q=LF_H(LF_A0+4u);LF_T4=p;LF_T5=q;
        LF_S3+=0x28u;LF_T6=((LF_T4&0xffffu)<<3)+LF_S0;
        LF_T7=((LF_T4>>13)&0xfff8u)+LF_S0;
        LF_T8=(LF_T5<<3)+LF_S0;
        p=MFC2(31);q=MFC2(12);LF_T0=p;LF_T1=q;
        int negative=(int32_t)LF_T0<0;LF_T0=LF_T1<LF_V0;
        if(negative)continue;
        p=MFC2(13);q=MFC2(14);LF_T2=p;LF_T3=q;doCOP2(LF_NCLIP);
        int inside=LF_T0!=0;LF_T0=LF_T2<LF_V0;
        if(!inside){inside=LF_T0!=0;LF_T0=LF_T3<LF_V0;if(!inside)inside=LF_T0!=0;}
        LF_T0=LF_T1&0xffffu;if(!inside)continue;
        LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T2&0xffffu;
        if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T3&0xffffu;
            if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;}}
        LF_T0=MFC2(24); /* NCLIP result; the x reject's delay slot */
        if(!inside)continue;
        LF_W(LF_S3+8u,LF_T1);
        int back=(int32_t)LF_T0<=0;LF_W(LF_S3+0x14u,LF_T2);
        if(back)continue;
        doCOP2(LF_AVSZ3);LF_W(LF_S3+0x20u,LF_T3);LF_S3&=LF_S6;
        LF_T0=MFC2(7);
        int zero=LF_T0==0;LF_T1=LF_R(LF_A2); /* the OTZ test's delay slot */
        if(zero)continue;
        LF_S2++;MTC2(LF_R(LF_T1),0);LF_T2=LF_R(LF_A2+4u);MTC2(LF_R(LF_T1+4u),1);
        LF_T1=LF_R(LF_A2+8u);MTC2(LF_R(LF_T2),2);MTC2(LF_R(LF_T2+4u),3);
        MTC2(LF_R(LF_T1),4);MTC2(LF_R(LF_T1+4u),5);doCOP2(LF_NCT);
        LF_T0=(uint32_t)((int32_t)LF_T0>>(LF_T9&31u));LF_T0<<=2;LF_T0+=LF_S4;
        p=LF_B(LF_S3+7u);q=MFC2(20);LF_T1=p;LF_T2=q;
        LF_T1<<=24;LF_T2&=LF_S6;LF_T1|=LF_T2;LF_W(LF_S3+4u,LF_T1);
        LF_W(LF_S3+0x10u,MFC2(21));LF_W(LF_S3+0x1cu,MFC2(22));
        LF_T1=LF_R(LF_T0);LF_W(LF_T0,LF_S3);LF_AT=0x09000000u;LF_T1|=LF_AT;
        LF_W(LF_S3,LF_T1);
    }
    LF_A3=0x28u;
    battle_lit_ft_tail(entry);
}
/* 8002ED20: lit POLY_F3 (0x14 bytes, tag length 4), row 0 variant 1.  One
 * 12-byte lighting record per face at 0x80059498 (a colour word, then the
 * face normal), lit with NCCS; the colour word's top byte becomes the code
 * byte, and the advanced cursor is stored back.  Unlike FT3: RTPT ahead of
 * the exit test, whose delay slot decrements the count (a1 ends at -1);
 * AVSZ3 and all three XY stores before the back-face test; and no OTZ-zero
 * rejection (an OTZ-0 face is counted and linked at OT[0]). */
static void battle_lit_f3_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    LF_T8=0x04000000u;
    battle_lit_ft_save(entry);battle_lit_ft_begin(entry);
    LF_S6=0x00ffffffu;LF_A2=LF_R(0x80059498u); /* side-effect-free reads */
    battle_lit_ft_first(entry);LF_S3-=0x14u;
    for(;;){
        MTC2(LF_R(LF_T6),0);MTC2(LF_R(LF_T6+4u),1);doCOP2(LF_RTPT);
        int last=LF_A1==0;LF_A1--; /* the exit test's delay slot */
        if(last)break;
        LF_A0+=8u;p=LF_R(LF_A0);q=LF_H(LF_A0+4u);LF_T4=p;LF_T5=q;
        LF_S3+=0x14u;LF_T6=((LF_T4&0xffffu)<<3)+LF_S0;LF_A2+=0xcu;
        LF_T0=((LF_T4>>13)&0xfff8u)+LF_S0;
        MTC2(LF_R(LF_T0),2);MTC2(LF_R(LF_T0+4u),3);
        LF_T1=(LF_T5<<3)+LF_S0;
        MTC2(LF_R(LF_T1),4);MTC2(LF_R(LF_T1+4u),5);
        p=MFC2(31);q=MFC2(12);LF_T0=p;LF_T1=q;
        int negative=(int32_t)LF_T0<0;LF_T0=LF_T1<LF_V0;
        if(negative)continue;
        p=MFC2(13);q=MFC2(14);LF_T2=p;LF_T3=q;doCOP2(LF_NCLIP);
        int inside=LF_T0!=0;LF_T0=LF_T2<LF_V0;
        if(!inside){inside=LF_T0!=0;LF_T0=LF_T3<LF_V0;if(!inside)inside=LF_T0!=0;}
        LF_T0=LF_T1&0xffffu;if(!inside)continue;
        LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T2&0xffffu;
        if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T3&0xffffu;
            if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;}}
        LF_T0=MFC2(24); /* NCLIP result; the y reject's delay slot */
        if(!inside)continue;
        LF_W(LF_S3+8u,LF_T1);doCOP2(LF_AVSZ3);LF_W(LF_S3+0xcu,LF_T2);
        int back=(int32_t)LF_T0<=0;LF_W(LF_S3+0x10u,LF_T3);
        if(back)continue;
        LF_S3&=LF_S6;LF_T0=MFC2(7);LF_S2++;
        LF_T1=LF_R(LF_A2-0xcu);MTC2(LF_R(LF_A2-8u),0);MTC2(LF_R(LF_A2-4u),1);
        MTC2(LF_T1,6);doCOP2(LF_NCCS);
        LF_T0=(uint32_t)((int32_t)LF_T0>>(LF_T7&31u));LF_T0<<=2;LF_T0+=LF_S4;
        LF_T2=MFC2(22);LF_AT=0xff000000u;LF_T1&=LF_AT;LF_T2&=LF_S6;LF_T1|=LF_T2;
        LF_W(LF_S3+4u,LF_T1);
        LF_T1=LF_R(LF_T0);LF_W(LF_T0,LF_S3);LF_T1|=LF_T8;
        LF_W(LF_S3,LF_T1);
    }
    LF_AT=0x80060000u;LF_W(LF_AT-0x6b68u,LF_A2);LF_A3=0x14u;
    battle_lit_ft_tail(entry);
}
/* 8002F6B4: lit POLY_G3 (0x1C bytes, tag length 6), row 2 variant 1.  One
 * normal per vertex, staged in the scratchpad as GT3 does and reloaded after
 * the OTZ test (here into t2 / t1 / t2), lit with NCCT; one colour word per
 * face at 0x80059498 (stride 4, stored back), whose top byte becomes the code
 * byte.  RGB0 / RGB1 / RGB2 land at +4 / +0xC / +0x14.  Bounds, OT shift
 * (t9) and the 24-bit cursor as GT3; the count after the OTZ test.  Retail's
 * handwritten add/sub (trapping forms) take address deltas that cannot
 * overflow, as in GT3. */
static void battle_lit_g3_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    battle_lit_ft_save(entry);
    LF_S0=LF_R(0x8005953cu);LF_A3=LF_R(0x8005952cu);LF_S2=LF_R(0x80059578u);
    LF_S3=LF_R(0x80059424u);LF_S4=LF_R(0x80059568u);LF_V0=LF_R(0x800500fcu);
    LF_V1=LF_R(0x800500f8u);LF_T9=LF_R(0x80050100u);LF_S7=LF_R(0x80059498u);
    LF_S6=0x00ffffffu;LF_A2=0x1f800000u;
    p=LF_R(LF_A0);q=LF_H(LF_A0+4u);LF_T4=p;LF_T5=q;
    LF_T6=((LF_T4&0xffffu)<<3)+LF_S0;
    LF_T7=((LF_T4>>13)&0xfff8u)+LF_S0;
    LF_T8=(LF_T5<<3)+LF_S0;
    LF_S3-=0x1cu;LF_A3-=LF_S0;
    for(;;){
        MTC2(LF_R(LF_T6),0);MTC2(LF_R(LF_T6+4u),1);
        MTC2(LF_R(LF_T7),2);MTC2(LF_R(LF_T7+4u),3);
        MTC2(LF_R(LF_T8),4);MTC2(LF_R(LF_T8+4u),5);
        doCOP2(LF_RTPT); /* delay slot of the exit test: the lookahead runs */
        if(LF_A1==0)break;
        LF_T6+=LF_A3;LF_T7+=LF_A3;LF_T8+=LF_A3;
        LF_W(LF_A2,LF_T6);LF_W(LF_A2+4u,LF_T7);LF_W(LF_A2+8u,LF_T8);
        LF_A1--;LF_A0+=8u;LF_S7+=4u;p=LF_R(LF_A0);q=LF_H(LF_A0+4u);LF_T4=p;LF_T5=q;
        LF_S3+=0x1cu;LF_T6=((LF_T4&0xffffu)<<3)+LF_S0;
        LF_T7=((LF_T4>>13)&0xfff8u)+LF_S0;
        LF_T8=(LF_T5<<3)+LF_S0;
        p=MFC2(31);q=MFC2(12);LF_T0=p;LF_T1=q;
        int negative=(int32_t)LF_T0<0;LF_T0=LF_T1<LF_V0;
        if(negative)continue;
        p=MFC2(13);q=MFC2(14);LF_T2=p;LF_T3=q;doCOP2(LF_NCLIP);
        int inside=LF_T0!=0;LF_T0=LF_T2<LF_V0;
        if(!inside){inside=LF_T0!=0;LF_T0=LF_T3<LF_V0;if(!inside)inside=LF_T0!=0;}
        LF_T0=LF_T1&0xffffu;if(!inside)continue;
        LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T2&0xffffu;
        if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T3&0xffffu;
            if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;}}
        LF_T0=MFC2(24); /* NCLIP result; the y reject's delay slot */
        if(!inside)continue;
        LF_W(LF_S3+8u,LF_T1);
        int back=(int32_t)LF_T0<=0;LF_W(LF_S3+0x10u,LF_T2);
        if(back)continue;
        doCOP2(LF_AVSZ3);LF_W(LF_S3+0x18u,LF_T3);LF_S3&=LF_S6;
        LF_T0=MFC2(7);
        int zero=LF_T0==0;LF_T2=LF_R(LF_A2); /* the OTZ test's delay slot */
        if(zero)continue;
        LF_S2++;MTC2(LF_R(LF_T2),0);LF_T1=LF_R(LF_A2+4u);MTC2(LF_R(LF_T2+4u),1);
        LF_T2=LF_R(LF_A2+8u);MTC2(LF_R(LF_T1),2);MTC2(LF_R(LF_T1+4u),3);
        LF_T1=LF_R(LF_S7-4u);MTC2(LF_R(LF_T2),4);MTC2(LF_R(LF_T2+4u),5);
        MTC2(LF_T1,6);doCOP2(LF_NCCT);
        LF_T0=(uint32_t)((int32_t)LF_T0>>(LF_T9&31u));LF_T0<<=2;LF_T0+=LF_S4;
        LF_T2=MFC2(20);LF_AT=0xff000000u;LF_T1&=LF_AT;LF_T2&=LF_S6;LF_T1|=LF_T2;
        LF_W(LF_S3+4u,LF_T1);LF_W(LF_S3+0xcu,MFC2(21));LF_W(LF_S3+0x14u,MFC2(22));
        LF_T1=LF_R(LF_T0);LF_W(LF_T0,LF_S3);LF_AT=0x06000000u;LF_T1|=LF_AT;
        LF_W(LF_S3,LF_T1);
    }
    LF_AT=0x80060000u;LF_W(LF_AT-0x6b68u,LF_S7);LF_A3=0x1cu;
    battle_lit_ft_tail(entry);
}
/* 8002F8D0: lit POLY_F4 (0x18 bytes, tag length 5), row 8 variant 1.
 * Lighting records as F3 (12 bytes, NCCS, the code byte from the colour
 * word, the cursor stored back).  As FT4: later commands keep 13 bits of
 * index 0, the cursor mask sits in the first LZCR test's delay slot, the
 * fourth vertex (halfword +6) goes through RTPS after the back-face test,
 * and the count comes before the OTZ test; the XY stores come only after
 * every test. */
static void battle_lit_f4_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    LF_T8=0x05000000u;
    battle_lit_ft_save(entry);battle_lit_ft_begin(entry);
    LF_A2=LF_R(0x80059498u);
    battle_lit_ft_first(entry);LF_S3-=0x18u;LF_S6=0x00ffffffu;
    for(;;){
        MTC2(LF_R(LF_T6),0);MTC2(LF_R(LF_T6+4u),1);
        doCOP2(LF_RTPT); /* delay slot of the exit test: the lookahead runs */
        if(LF_A1==0)break;
        LF_A1--;LF_A0+=8u;p=LF_R(LF_A0);q=LF_H(LF_A0+4u);LF_T4=p;LF_T5=q;
        LF_S3+=0x18u;LF_T6=((LF_T4<<3)&0xfff8u)+LF_S0;
        LF_T0=((LF_T4>>13)&0xfff8u)+LF_S0;
        MTC2(LF_R(LF_T0),2);MTC2(LF_R(LF_T0+4u),3);
        LF_T0=(LF_T5<<3)+LF_S0;
        MTC2(LF_R(LF_T0),4);MTC2(LF_R(LF_T0+4u),5);
        LF_A2+=0xcu;
        p=MFC2(31);doCOP2(LF_NCLIP);LF_T0=p; /* LZCR read before NCLIP */
        int negative=(int32_t)LF_T0<0;LF_S3&=LF_S6;
        if(negative)continue;
        LF_T5=LF_H(LF_A0-2u);LF_T1=MFC2(12);LF_T4=MFC2(24);
        LF_T0=LF_T5<<3;
        int back=(int32_t)LF_T4<=0;LF_T0+=LF_S0;
        if(back)continue;
        LF_T2=MFC2(13);MTC2(LF_R(LF_T0),0);MTC2(LF_R(LF_T0+4u),1);
        LF_T3=MFC2(14);doCOP2(LF_RTPS); /* SXY2 read before RTPS */
        p=MFC2(31);q=MFC2(14);LF_T0=p;LF_T4=q;
        negative=(int32_t)LF_T0<0;doCOP2(LF_AVSZ4);
        if(negative)continue;
        LF_T0=LF_T1<LF_V0;
        int inside=LF_T0!=0;LF_T0=LF_T2<LF_V0;
        if(!inside){inside=LF_T0!=0;LF_T0=LF_T3<LF_V0;
            if(!inside){inside=LF_T0!=0;LF_T0=LF_T4<LF_V0;if(!inside)inside=LF_T0!=0;}}
        LF_T0=LF_T1&0xffffu;if(!inside)continue;
        LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T2&0xffffu;
        if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T3&0xffffu;
            if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;LF_T0=LF_T4&0xffffu;
                if(!inside){LF_T0=LF_T0<LF_V1;inside=LF_T0!=0;}}}
        LF_T0=MFC2(7); /* OTZ; the y reject's delay slot */
        if(!inside)continue;
        LF_S2++;
        int zero=LF_T0==0;LF_T0=(uint32_t)((int32_t)LF_T0>>(LF_T7&31u));
        if(zero)continue;
        LF_T5=LF_R(LF_A2-0xcu);MTC2(LF_R(LF_A2-8u),0);MTC2(LF_R(LF_A2-4u),1);
        MTC2(LF_T5,6);doCOP2(LF_NCCS);
        LF_T0<<=2;LF_T0+=LF_S4;LF_S1=LF_R(LF_T0);
        LF_W(LF_S3+8u,LF_T1);LF_W(LF_S3+0xcu,LF_T2);
        LF_W(LF_S3+0x10u,LF_T3);LF_W(LF_S3+0x14u,LF_T4);
        LF_T2=MFC2(22);LF_AT=0xff000000u;LF_T5&=LF_AT;LF_T2&=LF_S6;LF_T1=LF_T5|LF_T2;
        LF_W(LF_S3+4u,LF_T1);LF_W(LF_T0,LF_S3);LF_T1=LF_S1|LF_T8;
        LF_W(LF_S3,LF_T1);
    }
    LF_AT=0x80060000u;LF_W(LF_AT-0x6b68u,LF_A2);LF_A3=0x18u;
    battle_lit_ft_tail(entry);
}
static const struct {uint32_t address;unsigned bytes;const char *sha;} battle_lit_ft_bodies[]={
    {0x8002f2e0u,0x1d4u,"648b7bb0e8ca0474b0cd2fd2f44fa006d965ac3475a4a8239e3625ec4032a540"},
    {0x8002fae8u,0x214u,"0aad615182563b964aa23bbc2447cf969136f2083ed6b27fcaf3683c1d55bd5c"},
    {0x8002e1f4u,0x38u,"b7d98d849c2248e0da88f393f8e57a1f9b42df3c4d568ab3c194b01b91f5e2de"},
    {0x8002f4b4u,0x200u,"b5c1c8ba79f06d23330897a15f53351bcc505469014e82e82d45d5efd09d246a"},
    {0x8002ed20u,0x1d8u,"f0f3d3d37934a9f1a597feb0aa10e15daa54f271064b733020302cbd9fa43d26"},
    {0x8002f6b4u,0x21cu,"d711b272ff9fa2e86527c0f91e9f10348e6ff50fb5dde3c3409f787f9be871a0"},
    {0x8002f8d0u,0x218u,"38df8b7797eba1f5a19a517c5eb64fbc7094b10c4e7fe693fa9f0ecf7588a2db"},
};
#define LF_BODIES (sizeof(battle_lit_ft_bodies)/sizeof(battle_lit_ft_bodies[0]))
#define LF_TAIL 2u /* the shared 8002E1F4 tail's row */
/* The retail image is read-only once loaded, so each range is checked once. */
static int battle_lit_ft_identity(unsigned index)
{
    static int verified[LF_BODIES];
    if(verified[index]==0){
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
        size_t offset=XENO_RD_SLUS_OFF(battle_lit_ft_bodies[index].address);
        int ok=image&&image->bytes&&image->size>=offset+battle_lit_ft_bodies[index].bytes;
        if(ok){
            uint8_t digest[32];const char *expected=battle_lit_ft_bodies[index].sha;
            file1_digest(image->bytes+offset,battle_lit_ft_bodies[index].bytes,digest);
            for(unsigned i=0;i<32&&ok;i++)
                ok=file1_hex_digit(digest[i]>>4)==expected[i*2]&&file1_hex_digit(digest[i]&15u)==expected[i*2+1];
        }
        if(!image)return 0; /* not loaded yet: decide later */
        verified[index]=ok?1:-1;
    }
    return verified[index]>0;
}
static int battle_lit_ft_target(uint32_t target)
{
    switch(target){
    case 0x8002f2e0u:case 0x8002fae8u:case 0x8002f4b4u:
    case 0x8002ed20u:case 0x8002f6b4u:case 0x8002f8d0u:return 1;
    default:return 0;
    }
}
/* A body's identity row, found by its address (never the shared tail's). */
static int battle_lit_ft_body(uint32_t target)
{
    for(unsigned i=0;i<LF_BODIES;i++)
        if(i!=LF_TAIL&&battle_lit_ft_bodies[i].address==target)return (int)i;
    return -1;
}
static int battle_lit_ft_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_lit_ft_target(target)||cpu->gpr[31]!=0x8002c86cu||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_LIT_FT_PACKETS");if(control&&strcmp(control,"0")==0)return 0;
    int body=battle_lit_ft_body(target);
    if(body<0||!battle_lit_ft_identity((unsigned)body)||!battle_lit_ft_identity(LF_TAIL))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    switch(target){
    case 0x8002f2e0u:battle_lit_ft3_run(&entry);break;
    case 0x8002fae8u:battle_lit_ft4_run(&entry);break;
    case 0x8002f4b4u:battle_lit_gt3_run(&entry);break;
    case 0x8002ed20u:battle_lit_f3_run(&entry);break;
    case 0x8002f6b4u:battle_lit_g3_run(&entry);break;
    default:battle_lit_f4_run(&entry);break;
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef LF_R
#undef LF_H
#undef LF_B
#undef LF_W
#undef LF_AT
#undef LF_V0
#undef LF_V1
#undef LF_A0
#undef LF_A1
#undef LF_A2
#undef LF_A3
#undef LF_T0
#undef LF_T1
#undef LF_T2
#undef LF_T3
#undef LF_T4
#undef LF_T5
#undef LF_T6
#undef LF_T7
#undef LF_S0
#undef LF_S1
#undef LF_S2
#undef LF_S3
#undef LF_S4
#undef LF_S5
#undef LF_S6
#undef LF_S7
#undef LF_T8
#undef LF_T9
#undef LF_SP
#undef LF_RTPT
#undef LF_RTPS
#undef LF_NCLIP
#undef LF_AVSZ3
#undef LF_AVSZ4
#undef LF_NCS
#undef LF_NCT
#undef LF_NCCS
#undef LF_NCCT
#undef LF_BODIES
#undef LF_TAIL
