/* Synthetic-only preparation occurs before reservation; slot values are
 * captured separately under the real raw reservation. Not production dispatch. */
typedef struct {
 BattleMipsRuntime *runtime;unsigned event,count,captured;
 uint32_t candidates[2],voices[2];uintptr_t slots_before[2];
} PreparedSoundAssign;
static PreparedSoundAssign prepared_assign;
static int prepare_sound_assign(BattleMipsRuntime *rt,unsigned event,unsigned count,
                                const uint32_t candidates[2],const uint32_t voices[2],PreparedSoundAssign *p)
{
 memset(p,0,sizeof *p);
 if(!count || count>2u || prepared_release.runtime!=rt || prepared_release.event!=event)return 0;
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
 const size_t off=XENO_RD_SLUS_OFF(0x8003e724u),length=0x118u;
 if(!image || !image->bytes || image->size<off+length)return 0;
 static const char sha[]="35833c13d582877b82bf00c5e233e9aafb7d0ed0fbf6d4b44fdd9060caf5589c";
 uint8_t digest[32];file1_digest(image->bytes+off,length,digest);
 for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=sha[2*i] || file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return 0;
 for(unsigned i=0;i<count;i++) {
  uint32_t candidate=candidates[i],voice=voices[i];
  if(voice>=24u || (i && voice==voices[0]) || candidate<BATTLE_BUS_KUSEG_RAM_END ||
     (candidate&1u) || candidate>UINT32_MAX-8u ||
     (candidate&0xff800000u)==0x80000000u || (candidate&0xff800000u)==0xa0000000u ||
     !host_address_valid(rt,candidate,8u,0) || !host_address_valid(rt,candidate,8u,1))return 0;
  uintptr_t a=candidate,b=(uintptr_t)prepared_release.slots,c=(uintptr_t)prepared_release.dirty,d=(uintptr_t)prepared_release.key;
  if((a<b+24u*sizeof(void*) && b<a+8u) || (a<c+4u && c<a+8u) || (a<d+4u && d<a+8u) ||
     (i && a<(uintptr_t)candidates[0]+8u && (uintptr_t)candidates[0]<a+8u))return 0;
  p->candidates[i]=candidate;p->voices[i]=voice;
 }
 p->runtime=rt;p->event=event;p->count=count;return 1;
}
/* No resolver, file access, allocation, or logging while reserved. The caller
 * must release the raw reservation if this capture refuses. */
static int capture_sound_assign(PreparedSoundAssign *p)
{
 if(!p->runtime || !p->event || p->captured || !prepared_release.slots ||
    !PsyX_Sys_ValidateSoundEventReservation(p->event,1))return 0;
 for(unsigned i=0;i<p->count;i++) {
  uintptr_t old=0;memcpy(&old,prepared_release.slots+p->voices[i]*sizeof(void*),sizeof old);
  if(old>UINT32_MAX || (old && ((old&1u) || !host_address_valid(p->runtime,old,8u,0))))return 0;
  p->slots_before[i]=old;
 }
 p->captured=1;return 1;
}
static void scratch_sound_assign(BattleEntryCall *e)
{
 PreparedSoundAssign *p=&prepared_assign;uint32_t *r=e->cpu->gpr,index=r[5];
 if(!p->captured || p->runtime!=e->runtime || prepared_release.runtime!=e->runtime || prepared_release.event!=p->event ||
    !PsyX_Sys_ReservedSoundBracketPristine(p->event))battle_entry_fail(e,"synthetic assign reservation changed",p->event);
 unsigned match=p->count;
 for(unsigned i=0;i<p->count;i++)if(p->voices[i]==index && p->candidates[i]==r[4]){match=i;break;}
 if(match==p->count)battle_entry_fail(e,"synthetic assign input not prepared",index);
 uintptr_t old=0;memcpy(&old,prepared_release.slots+index*sizeof(void*),sizeof old);
 if(old!=p->slots_before[match])battle_entry_fail(e,"synthetic assign slot changed",index);
 uint8_t *candidate=(uint8_t*)(uintptr_t)r[4];uint32_t bit=1u<<(index&31u);
 r[3]=0x8006252cu;r[6]=r[3]+(index<<2);r[2]=(uint32_t)old;
 if((uint32_t)old==r[4]) {
  r[3]=load_le(prepared_release.dirty,4);r[2]=r[3]|bit;r[1]=0x80060000u;
  runtime_note_store(e->runtime,prepared_release.dirty,4);store_le(prepared_release.dirty,4,r[2]);return;
 }
 if(old) {
  r[3]=(uint32_t)(int32_t)(int16_t)load_le((const uint8_t*)old+4u,2);
  r[2]=(int32_t)(int16_t)load_le(candidate+4u,2)<(int32_t)r[3];
  if(r[2])return;
 }
 runtime_note_store(e->runtime,candidate+6u,2);store_le(candidate+6u,2,0xffffu);
 r[3]=load_le(prepared_release.dirty,4);
 runtime_note_store(e->runtime,candidate,2);store_le(candidate,2,index);
 void *slot=candidate;memcpy(prepared_release.slots+index*sizeof(void*),&slot,sizeof slot);
 r[3]|=bit;r[1]=0x80060000u;runtime_note_store(e->runtime,prepared_release.dirty,4);store_le(prepared_release.dirty,4,r[3]);
 r[3]=load_le(prepared_release.key,4);r[2]=r[3]&~bit;r[1]=0x80060000u;
 runtime_note_store(e->runtime,prepared_release.key,4);store_le(prepared_release.key,4,r[2]);
}
