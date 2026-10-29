/* Scratch-only transcription; no real event bracket or live audio invocation. */
static void scratch_sound_release(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;
 r[2]=r[5]<<2;r[3]=0x8006252cu+r[2];r[2]=r[5]<24u;
 if(r[2]){r[2]=battle_entry_read(e,r[3],4);int equal=(r[2]==r[4]);r[2]=1;
  if(equal){battle_entry_write(e,r[3],4,0);r[3]=battle_entry_read(e,0x80059554u,4);r[2]<<=(r[5]&31u);r[3]|=r[2];r[1]=0x80060000u;battle_entry_write(e,0x80059554u,4,r[3]);r[3]=battle_entry_read(e,0x800594fcu,4);r[2]=~r[2];r[2]&=r[3];r[1]=0x80060000u;battle_entry_write(e,0x800594fcu,4,r[2]);}
 }
}
