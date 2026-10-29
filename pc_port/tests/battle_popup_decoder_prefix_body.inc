/* Test-only first popup decoder step, ending at the ShadeTex boundary. */
static void popup_decoder_prefix_test_run(BattleEntryCall *e)
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
    r[2]=battle_entry_read(e,r[17],1);r[4]=r[16];battle_entry_write(e,r[16]+3u,1,r[2]);
    r[2]=battle_entry_read(e,r[17]+3u,1);r[5]=r[23]&0xffu;
    battle_entry_write(e,r[16]+7u,1,r[2]);
    battle_phase_call(e,0x80043c24u,0x800b1790u);
    battle_entry_fail(e,"synthetic ShadeTex returned",0x80043c24u);
}
