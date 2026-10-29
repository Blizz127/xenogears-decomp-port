/* Test-only release adapter: preparation is outside the event reservation;
 * application requires the same thread's unchanged reserved bracket. It is
 * not wired into production and does not prove callback/data rollback. */
typedef struct {
 BattleMipsRuntime *runtime;
 uint8_t *slots,*dirty,*key;
 unsigned event;
} PreparedSoundRelease;
static PreparedSoundRelease prepared_release;
extern int PsyX_Sys_ReservedSoundBracketPristine(unsigned int);
static int prepare_sound_release(BattleMipsRuntime *rt,unsigned event,PreparedSoundRelease *p)
{
 memset(p,0,sizeof *p);
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
 const size_t off=XENO_RD_SLUS_OFF(0x8003e83cu),length=0x68u;
 if(!event || !image || !image->bytes || image->size<off+length)return 0;
 static const char sha[]="414550c5e775d02c8caa4262fe8f96f65d99ecb870fc79c66fceb3ebcb24e4ce";
 uint8_t digest[32];file1_digest(image->bytes+off,length,digest);
 for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=sha[2*i] || file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return 0;
 ResolvedData *slots=find_data(rt,0x8006252cu,24u*sizeof(void*));
 ResolvedData *dirty=find_data(rt,0x80059554u,4u),*key=find_data(rt,0x800594fcu,4u);
 if(!slots || slots->address!=0x8006252cu || !slots->host || slots->size<24u*sizeof(void*) ||
    !slots->name || strcmp(slots->name,"g_SoundChannels") || !dirty || dirty->address!=0x80059554u ||
    dirty->size<4u || !dirty->host || !key || key->address!=0x800594fcu || key->size<4u || !key->host)return 0;
 uintptr_t a=(uintptr_t)slots->host,b=(uintptr_t)dirty->host,c=(uintptr_t)key->host;
 if((a<b+4u && b<a+24u*sizeof(void*)) || (a<c+4u && c<a+24u*sizeof(void*)) ||
    (b<c+4u && c<b+4u))return 0;
 p->runtime=rt;p->slots=slots->host;p->dirty=dirty->host;p->key=key->host;p->event=event;return 1;
}
static void scratch_sound_release(BattleEntryCall *e)
{
 PreparedSoundRelease *p=&prepared_release;uint32_t *r=e->cpu->gpr;
 if(p->runtime!=e->runtime || !p->slots || !PsyX_Sys_ReservedSoundBracketPristine(p->event))
  battle_entry_fail(e,"synthetic release reservation changed",p->event);
 uint32_t index=r[5],slot_address=0x8006252cu+(index<<2),result=0,third=slot_address;
 uintptr_t old=0;
 if(index<24u) {
  memcpy(&old,p->slots+index*sizeof(void*),sizeof old);
  if(old>UINT32_MAX)battle_entry_fail(e,"synthetic release slot too wide",index);
  result=1u;
  if((uint32_t)old==r[4]) {
   void *empty=NULL;memcpy(p->slots+index*sizeof(void*),&empty,sizeof empty);
   uint32_t bit=1u<<(index&31u),dirty=load_le(p->dirty,4),key=load_le(p->key,4);
   runtime_note_store(e->runtime,p->dirty,4);store_le(p->dirty,4,dirty|bit);
   runtime_note_store(e->runtime,p->key,4);store_le(p->key,4,key&~bit);
   r[1]=0x80060000u;third=key;result=key&~bit;
  }
 }
 r[2]=result;r[3]=third;
}
