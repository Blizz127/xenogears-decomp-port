/* Test-only translation after a synthetic paired-task return. */
static void popup_create_suffix_test_run(BattleEntryCall *e)
{
    uint32_t *r=e->cpu->gpr;
    r[20]=r[2];r[2]=0x80060000u;r[2]=battle_entry_read(e,r[2]+0xffff91acu,1);
    battle_entry_write(e,r[20]+0x38u,4,r[21]);r[3]=battle_entry_read(e,r[21]+0x36u,2);
    battle_entry_write(e,r[20]+0x54u,4,r[3]);
    if(r[2]) {r[3]=0x80060000u;r[2]=battle_entry_read(e,r[3]+0xffff9464u,4);r[2]--;battle_entry_write(e,r[3]+0xffff9464u,4,r[2]);}
    r[3]=0x7fffffffu;r[4]=r[20]+0x4cu;r[5]=0;r[6]=r[5];
    r[2]=battle_entry_read(e,r[20]+0x14u,4);r[7]=r[5];r[2]&=r[3];
    battle_entry_write(e,r[20]+0x14u,4,r[2]);
    battle_phase_call(e,0x80021b04u,0x800bd134u);
    r[18]=0x8001c76cu;r[4]=r[18];r[5]=0;
    battle_phase_call(e,0x800b168cu,0x800bd148u);
    r[4]=r[2];battle_phase_call(e,0x800b16a4u,0x800bd150u);
    r[17]=r[2];r[4]=r[17]<<1;r[5]=0;
    battle_phase_call(e,0x80031bdcu,0x800bd160u);
    battle_entry_fail(e,"synthetic frame buffer allocator returned",0x80031bdcu);
}
