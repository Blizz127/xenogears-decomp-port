/* Test-only pre-resolved sound continuation memory. No resolver, loader or
 * logging is entered by these accessors while the reservation is held. */
typedef struct { uint32_t address,size;uint8_t *host;int write; } PreparedSoundSpan;
typedef struct { BattleMipsRuntime *runtime;PreparedSoundSpan spans[7]; unsigned event,captured;uint8_t expected[7][0x2b0],bank_expected[0x40];uint8_t *bank; } PreparedSoundMemory;
static PreparedSoundMemory prepared_memory;
static int prepare_sound_memory(BattleMipsRuntime *rt,const PcPortMipsCpu *cpu,PreparedSoundMemory *p)
{
 memset(p,0,sizeof *p);const uint32_t *r=cpu->gpr;
 const uint32_t addresses[]={r[17],r[20],r[29],r[18],r[21],0x800595bcu,0x80059504u};
 const uint32_t sizes[]={0x2b0u,0x4cu,80u,0x18u,4u,4u,4u};
 for(unsigned i=0;i<7;i++) {
  uint32_t a=addresses[i],n=sizes[i];int write=i<3;
  if(a<BATTLE_BUS_KUSEG_RAM_END || a>UINT32_MAX-n)return 0;
  uint8_t *base=resolve_memory(rt,a,n,0);
  if(!base || (write && resolve_memory(rt,a,n,1)!=base))return 0;
  /* Refuse any subaccess whose shared binding differs from the full span. */
  for(unsigned off=0;off<n;off++)for(unsigned w=1;w<=4;w*=2)if(off+w<=n)
   if(resolve_memory(rt,a+off,w,0)!=base+off || (write && resolve_memory(rt,a+off,w,1)!=base+off))return 0;
  p->spans[i]=(PreparedSoundSpan){a,n,base,write};
 }
 for(unsigned i=0;i<7;i++)for(unsigned j=i+1;j<7;j++) {
  uintptr_t a=(uintptr_t)p->spans[i].host,b=(uintptr_t)p->spans[j].host;
  if(a<b+p->spans[j].size && b<a+p->spans[i].size)return 0;
 }
 if(r[23]) {
  if(r[23]>UINT32_MAX-0x40u || !(p->bank=resolve_memory(rt,r[23],0x40u,0)))return 0;
  for(unsigned off=0;off<0x40u;off++)for(unsigned w=1;w<=4;w*=2)if(off+w<=0x40u)
   if(resolve_memory(rt,r[23]+off,w,0)!=p->bank+off)return 0;
  uintptr_t b=(uintptr_t)p->bank;
  for(unsigned i=0;i<7;i++) {uintptr_t a=(uintptr_t)p->spans[i].host;if(b<a+p->spans[i].size && a<b+0x40u)return 0;}
 }
 p->runtime=rt;return 1;
}
/* Capture while the raw reservation is held, before disabling the event. */
static int capture_sound_spans(PreparedSoundMemory *p,unsigned event)
{
 if(!p->runtime || !event || p->captured || !PsyX_Sys_ValidateSoundEventReservation(event,1))return 0;
 for(unsigned i=0;i<7;i++)memcpy(p->expected[i],p->spans[i].host,p->spans[i].size);
 if(p->bank)memcpy(p->bank_expected,p->bank,sizeof p->bank_expected);
 p->event=event;p->captured=1;return 1;
}
static int prepared_sound_data_unchanged(BattleEntryCall *e)
{
 PreparedSoundMemory *p=&prepared_memory;
 if(p->runtime!=e->runtime || !p->captured)return 0;
 for(unsigned i=0;i<7;i++)if(memcmp(p->expected[i],p->spans[i].host,p->spans[i].size))return 0;
 return !p->bank || !memcmp(p->bank_expected,p->bank,sizeof p->bank_expected);
}
/* Refresh after a helper's validated writes, before the next callback. */
static void prepared_sound_refresh(BattleEntryCall *e)
{
 PreparedSoundMemory *p=&prepared_memory;if(p->runtime!=e->runtime || !p->captured)longjmp(e->failure,1);
 for(unsigned i=0;i<7;i++)memcpy(p->expected[i],p->spans[i].host,p->spans[i].size);
 if(p->bank)memcpy(p->bank_expected,p->bank,sizeof p->bank_expected);
}
static uint8_t *prepared_sound_address(BattleEntryCall *e,uint32_t address,unsigned width,int write)
{
 if(prepared_memory.runtime!=e->runtime || (width!=1u && width!=2u && width!=4u))longjmp(e->failure,1);
 for(unsigned i=0;i<7;i++) {
  PreparedSoundSpan *s=&prepared_memory.spans[i];
  if(address>=s->address && address-s->address<=s->size && width<=s->size-(address-s->address) && (!write || s->write))
   return s->host+(address-s->address);
 }
 longjmp(e->failure,1);
}
static uint32_t prepared_sound_read(BattleEntryCall *e,uint32_t address,unsigned width)
{return load_le(prepared_sound_address(e,address,width,0),width);}
static void prepared_sound_write(BattleEntryCall *e,uint32_t address,unsigned width,uint32_t value)
{
 uint8_t *p=prepared_sound_address(e,address,width,1);
 runtime_note_store(e->runtime,p,width);store_le(p,width,value);
 uintptr_t a=(uintptr_t)p;
 if(prepared_memory.captured)for(unsigned i=0;i<3;i++) {
  PreparedSoundSpan *span=&prepared_memory.spans[i];uintptr_t b=(uintptr_t)span->host;
  if(a>=b && a-b<=span->size && width<=span->size-(a-b))memcpy(prepared_memory.expected[i]+(a-b),p,width);
 }
}
