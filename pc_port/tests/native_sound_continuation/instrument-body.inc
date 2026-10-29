/* Scratch-only instruction transcription; no real sound-event bracket. */
static void scratch_sound_instrument(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;
 battle_entry_write(e,r[5]+0x26u,1,r[4]);r[4]=(uint32_t)((int32_t)(int16_t)r[4]*16);r[2]=battle_entry_read(e,r[5]+0x2cu,4);r[4]+=0x30u;r[4]+=r[2];r[3]=battle_entry_read(e,r[4],4);r[2]=battle_entry_read(e,r[2]+0x28u,4);r[3]<<=3;r[2]+=r[3];battle_entry_write(e,r[5]+0x4cu,4,r[2]);r[2]=battle_entry_read(e,r[4]+4u,2);r[2]<<=3;r[3]+=r[2];battle_entry_write(e,r[5]+0x50u,4,r[3]);
 r[6]=battle_entry_read(e,r[4]+0xcu,2);r[2]=r[6]&7u;battle_entry_write(e,r[5]+0x54u,1,r[2]);r[2]=(r[6]>>4)&7u;battle_entry_write(e,r[5]+0x55u,1,r[2]);r[2]=(r[6]>>8)&7u;battle_entry_write(e,r[5]+0x56u,1,r[2]);
 r[6]=battle_entry_read(e,r[4]+8u,4);r[3]=r[6]>>24;r[2]=r[6]&0x7fu;battle_entry_write(e,r[5]+0x57u,1,r[2]);r[2]=(r[6]>>8)&15u;battle_entry_write(e,r[5]+0x58u,1,r[2]);r[2]=(r[6]>>16)&0x7fu;battle_entry_write(e,r[5]+0x59u,1,r[2]);r[2]=(r[6]>>12)&15u;battle_entry_write(e,r[5]+0x5bu,1,r[2]);r[2]=battle_entry_read(e,r[5],2);r[3]&=31u;battle_entry_write(e,r[5]+0x28u,1,r[3]);battle_entry_write(e,r[5]+0x5au,1,r[3]);r[3]=battle_entry_read(e,r[4]+6u,2);r[2]|=0x8000u;battle_entry_write(e,r[5],2,r[2]);battle_entry_write(e,r[5]+0x6cu,2,r[3]);
}
