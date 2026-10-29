/* Test-only direct LP64 slots; caller supplies the synthetic event bracket. */
static void scratch_sound_assign(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;
 if(!PsyX_Sys_ValidateSoundEventReservation(event_id,0))
  battle_entry_fail(e,"synthetic sound event identity changed",event_id);
 ResolvedData *binding=find_data(e->runtime,0x8006252cu,24u*sizeof(void*));
 if(!binding || binding->address!=0x8006252cu || binding->size<24u*sizeof(void*))battle_entry_fail(e,"synthetic slot binding invalid",0);
 void **slots=(void**)binding->host;
 r[2]=r[5]<<2;r[3]=0x8006252cu;r[6]=r[2]+r[3];r[2]=r[5]<24u;
 if(r[2]){uintptr_t old=(uintptr_t)slots[r[5]];if(old>UINT32_MAX)battle_entry_fail(e,"synthetic slot too wide",r[5]);r[2]=(uint32_t)old;
  if(r[2]==r[4]){r[2]=1;r[3]=battle_entry_read(e,0x80059554u,4);r[2]<<=(r[5]&31u);r[2]|=r[3];r[1]=0x80060000u;battle_entry_write(e,0x80059554u,4,r[2]);return;}
  if(r[2]){r[3]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[2]+4u,2);r[2]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[4]+4u,2);r[2]=(int32_t)r[2]<(int32_t)r[3];if(r[2])return;}
  r[2]=0xffffu;battle_entry_write(e,r[4]+6u,2,r[2]);r[2]=1;r[3]=battle_entry_read(e,0x80059554u,4);r[2]<<=(r[5]&31u);battle_entry_write(e,r[4],2,r[5]);if(!host_address_valid(e->runtime,(uintptr_t)r[4],8u,1))battle_entry_fail(e,"synthetic candidate invalid",r[4]);slots[r[5]]=(void*)(uintptr_t)r[4];r[3]|=r[2];r[1]=0x80060000u;battle_entry_write(e,0x80059554u,4,r[3]);r[3]=battle_entry_read(e,0x800594fcu,4);r[2]=~r[2];r[2]&=r[3];r[1]=0x80060000u;battle_entry_write(e,0x800594fcu,4,r[2]);
 }
}
