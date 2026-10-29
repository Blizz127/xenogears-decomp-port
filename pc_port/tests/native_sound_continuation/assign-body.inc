/* Scratch-only transcription; the surrounding sound event bracket is held. */
static void scratch_sound_assign(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[2]=r[5]<<2;r[3]=0x8006252cu;r[6]=r[2]+r[3];r[2]=r[5]<24u;
 if(r[2]){r[2]=battle_entry_read(e,r[6],4);
  if(r[2]==r[4]){r[2]=1;r[3]=battle_entry_read(e,0x80059554u,4);r[2]<<=(r[5]&31u);r[2]|=r[3];r[1]=0x80060000u;battle_entry_write(e,0x80059554u,4,r[2]);return;}
  if(r[2]){r[3]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[2]+4u,2);r[2]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[4]+4u,2);r[2]=(int32_t)r[2]<(int32_t)r[3];if(r[2])return;}
  r[2]=0xffffu;battle_entry_write(e,r[4]+6u,2,r[2]);r[2]=1;r[3]=battle_entry_read(e,0x80059554u,4);r[2]<<=(r[5]&31u);battle_entry_write(e,r[4],2,r[5]);battle_entry_write(e,r[6],4,r[4]);r[3]|=r[2];r[1]=0x80060000u;battle_entry_write(e,0x80059554u,4,r[3]);r[3]=battle_entry_read(e,0x800594fcu,4);r[2]=~r[2];r[2]&=r[3];r[1]=0x80060000u;battle_entry_write(e,0x800594fcu,4,r[2]);
 }
}
