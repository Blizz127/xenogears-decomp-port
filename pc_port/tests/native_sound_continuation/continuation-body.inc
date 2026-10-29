/* Scratch-only retail continuation: synthetic events, no real mutex or live state. */
static void scratch_sound_continuation(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;unsigned iterations=0;
 do {
  if(iterations++>=2)battle_entry_fail(e,"scratch count limit",0);
  r[8]=battle_entry_read(e,r[29]+16u,4);r[2]=battle_entry_read(e,0x80059504u,4);battle_entry_write(e,r[16]-0x1fu,4,r[8]);r[8]=battle_entry_read(e,r[29]+32u,1);battle_entry_write(e,r[16]-0x1bu,4,r[2]);battle_entry_write(e,r[16]-0x20u,1,r[8]);r[2]=battle_entry_read(e,r[21],2);r[8]=1;
  if(r[2]){
   r[2]=battle_entry_read(e,r[16]-0x21u,1);r[3]=battle_entry_read(e,r[20]+0x48u,4);r[2]=r[8]<<(r[2]&31u);r[2]|=r[3];battle_entry_write(e,r[20]+0x48u,4,r[2]);r[2]=0x409u;battle_entry_write(e,r[17],2,r[2]);r[2]=battle_entry_read(e,r[18]+0x10u,2);r[2]&=1u;r[5]=3;if(r[2]){r[2]=0x40bu;battle_entry_write(e,r[17],2,r[2]);}
   r[2]=0x170u;battle_entry_write(e,r[16]-0x25u,2,r[2]);battle_entry_write(e,r[16]-0x23u,2,0);r[3]=battle_entry_read(e,r[21],2);r[2]=0x3cu;battle_entry_write(e,r[16]+0x3fu,2,r[2]);r[2]=15u;battle_entry_write(e,r[16]+0x3bu,2,r[2]);r[2]=0xffffu;battle_entry_write(e,r[16]+0x4bu,2,r[2]);r[2]=0x7f000000u;
   battle_entry_write(e,r[16]-0xfu,4,0);battle_entry_write(e,r[16]-0xbu,4,0);battle_entry_write(e,r[16]-7u,2,0);battle_entry_write(e,r[16]-5u,1,0);battle_entry_write(e,r[16]+0x35u,2,0);battle_entry_write(e,r[16]+0x39u,1,0);battle_entry_write(e,r[16]+0x47u,2,0);battle_entry_write(e,r[16]+0x3du,1,0);battle_entry_write(e,r[16]+0x4fu,2,r[30]);battle_entry_write(e,r[16]+0x51u,4,r[2]);r[8]=battle_entry_read(e,r[29]+24u,2);r[4]=r[17]+0x60u;
   battle_entry_write(e,r[16]+0x49u,2,0);battle_entry_write(e,r[16]+0xa9u,2,0);battle_entry_write(e,r[16]+0xabu,2,0);battle_entry_write(e,r[16]+0xadu,2,0);battle_entry_write(e,r[16]+0x15u,2,0);battle_entry_write(e,r[16]+0x17u,2,0);battle_entry_write(e,r[16]+0xa7u,2,0);r[3]+=r[18];battle_entry_write(e,r[16]+0x4du,2,r[8]);battle_entry_write(e,r[16]-0x17u,4,r[3]);battle_entry_write(e,r[16]-0x13u,4,r[3]);
   do{battle_entry_write(e,r[4]+0xf6u,2,0);r[5]--;r[4]-=0x20u;}while((int32_t)r[5]>=0);
   r[2]=battle_entry_read(e,r[18]+0x16u,1);battle_entry_write(e,r[16]+5u,4,r[23]);battle_entry_write(e,r[16]-2u,1,r[2]);
   if(r[23]){r[4]=0;r[5]=r[17];r[31]=0x8003b884u;scratch_sound_call(e,0x8003e5bcu);}
   r[4]=r[19];r[5]=battle_entry_read(e,r[16],1);r[2]=0x200u;battle_entry_write(e,r[16]+0xbu,2,0);r[31]=0x8003b89cu;battle_entry_write(e,r[16]+0xdu,2,r[2]);scratch_sound_call(e,0x8003e724u);r[21]+=2u;
  } else {
   r[2]=battle_entry_read(e,r[16]-0x21u,1);r[3]=battle_entry_read(e,r[20]+0x48u,4);r[2]=r[8]<<(r[2]&31u);r[2]=~r[2];r[2]&=r[3];battle_entry_write(e,r[20]+0x48u,4,r[2]);battle_entry_write(e,r[17],2,0);r[5]=battle_entry_read(e,r[16],1);r[4]=r[19];r[31]=0x8003b8ccu;scratch_sound_call(e,0x8003e83cu);r[21]+=2u;
  }
  r[16]+=0x158u;r[19]+=0x158u;r[22]--;r[17]+=0x158u;
 }while(r[22]);
 r[2]=battle_entry_read(e,r[20]+0x10u,2);r[4]=battle_entry_read(e,0x800595bcu,4);r[2]|=0x8000u;r[31]=0x8003b8fcu;battle_entry_write(e,r[20]+0x10u,2,r[2]);
 /* Test-only event stub: does not acquire or release a real sound event. */
 scratch_sound_call(e,0x800404a4u);
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)r[regs[i]]=battle_entry_read(e,r[29]+76u-4u*i,4);r[29]+=80u;
}
