/* Test-only decoder for the eight known untextured popup primitives. */
static void popup_decoder_full_test_run(BattleEntryCall *e)
{
    uint32_t *r=e->cpu->gpr;
    r[29]-=56u;battle_entry_write(e,r[29]+16u,4,r[16]);r[16]=r[5];
    battle_entry_write(e,r[29]+32u,4,r[20]);r[20]=r[6];
    battle_entry_write(e,r[29]+44u,4,r[23]);r[23]=r[7];
    battle_entry_write(e,r[29]+48u,4,r[31]);battle_entry_write(e,r[29]+40u,4,r[22]);
    battle_entry_write(e,r[29]+36u,4,r[21]);battle_entry_write(e,r[29]+28u,4,r[19]);
    battle_entry_write(e,r[29]+24u,4,r[18]);battle_entry_write(e,r[29]+20u,4,r[17]);
    r[2]=battle_entry_read(e,r[4]+0x10u,4);r[22]=battle_entry_read(e,r[4]+0x14u,4);
    if(r[20]!=0 || r[22]!=8u)battle_entry_fail(e,"unsupported synthetic decoder inputs",r[22]);
    r[17]=r[2]+r[4];r[21]=0;r[18]=0x80u;
    for(unsigned n=0;n<8u;n++) {
        r[2]=battle_entry_read(e,r[17],1);r[4]=r[16];battle_entry_write(e,r[16]+3u,1,r[2]);
        r[2]=battle_entry_read(e,r[17]+3u,1);r[5]=r[23]&0xffu;
        battle_entry_write(e,r[16]+7u,1,r[2]);
        battle_phase_call(e,0x80043c24u,0x800b1790u);
        r[4]=r[16];r[2]=battle_entry_read(e,r[17]+3u,1);r[3]=battle_entry_read(e,r[17]+2u,1);
        r[4]=r[2]&0x1cu;r[3]=((r[3]^1u)&1u)<<8;r[4]|=r[3];
        if(r[4]!=0u && r[4]!=0x10u && r[4]!=0x100u && r[4]!=0x110u)battle_entry_fail(e,"unsupported synthetic decoder kind",r[4]);
        const unsigned src[]={4,5,6,8,9,10,12,13,14};
        const unsigned dst[]={4,5,6,12,13,14,20,21,22};
        unsigned bytes=(r[4]&0x10u)?9u:3u;
        for(unsigned i=0;i<bytes;i++) {r[2]=battle_entry_read(e,r[17]+src[i],1);battle_entry_write(e,r[16]+dst[i],1,r[2]);}
        r[21]++;r[3]=battle_entry_read(e,r[17],1);r[2]=battle_entry_read(e,r[17]+1u,1);
        r[3]=(r[3]+1u)<<2;r[16]+=r[3];r[2]=(r[2]+1u)<<2;r[17]+=r[2];
    }
    r[31]=battle_entry_read(e,r[29]+48u,4);r[23]=battle_entry_read(e,r[29]+44u,4);
    r[22]=battle_entry_read(e,r[29]+40u,4);r[21]=battle_entry_read(e,r[29]+36u,4);
    r[20]=battle_entry_read(e,r[29]+32u,4);r[19]=battle_entry_read(e,r[29]+28u,4);
    r[18]=battle_entry_read(e,r[29]+24u,4);r[17]=battle_entry_read(e,r[29]+20u,4);
    r[16]=battle_entry_read(e,r[29]+16u,4);r[29]+=56u;
}
