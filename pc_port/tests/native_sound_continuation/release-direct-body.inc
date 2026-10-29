/* Test-only direct LP64 slot release; the fixture owns the event bracket. */
static void scratch_sound_release(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;
 if(!PsyX_Sys_ValidateSoundEventReservation(event_id,0))
  battle_entry_fail(e,"synthetic sound event identity changed",event_id);
 ResolvedData *binding=find_data(e->runtime,0x8006252cu,24u*sizeof(void*));
 if(!binding || binding->address!=0x8006252cu || binding->size<24u*sizeof(void*))battle_entry_fail(e,"synthetic slot binding invalid",0);
 void **slots=(void**)binding->host;
 r[2]=r[5]<<2;r[3]=0x8006252cu+r[2];r[2]=r[5]<24u;
 if(r[2]){uintptr_t old=(uintptr_t)slots[r[5]];if(old>UINT32_MAX)battle_entry_fail(e,"synthetic slot too wide",r[5]);r[2]=(uint32_t)old;int equal=(r[2]==r[4]);r[2]=1;
  if(equal){slots[r[5]]=NULL;r[3]=battle_entry_read(e,0x80059554u,4);r[2]<<=(r[5]&31u);r[3]|=r[2];r[1]=0x80060000u;battle_entry_write(e,0x80059554u,4,r[3]);r[3]=battle_entry_read(e,0x800594fcu,4);r[2]=~r[2];r[2]&=r[3];r[1]=0x80060000u;battle_entry_write(e,0x800594fcu,4,r[2]);}
 }
}
