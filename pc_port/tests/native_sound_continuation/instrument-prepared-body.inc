/* Test-only index-zero instrument adapter. Resolve all spans before taking
 * the reservation; the fixture does not prove real manager/bank lifetimes. */
typedef struct {
 BattleMipsRuntime *runtime;unsigned event,count;
 uint32_t bank_address,elements[2];uint8_t *bank,*element_ptrs[2];
} PreparedSoundInstrument;
static PreparedSoundInstrument prepared_instrument;
static int prepare_sound_instrument(BattleMipsRuntime *rt,unsigned event,uint32_t bank,
                                    unsigned count,const uint32_t elements[2],PreparedSoundInstrument *p)
{
 memset(p,0,sizeof *p);
 if(!bank || bank>UINT32_MAX-0x40u || !count || count>2u || prepared_release.runtime!=rt || prepared_release.event!=event)return 0;
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
 const size_t off=XENO_RD_SLUS_OFF(0x8003e5bcu),length=0xc4u;
 if(!image || !image->bytes || image->size<off+length)return 0;
 static const char sha[]="4809ff2fe6e7021905042ca9d3c7129bbd80fc8fdd1819376850128794708190";
 uint8_t digest[32];file1_digest(image->bytes+off,length,digest);
 for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=sha[2*i] || file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return 0;
 uint8_t *bp=resolve_memory(rt,bank,0x40u,0);if(!bp)return 0;
 for(unsigned i=0;i<count;i++) {
  if(elements[i]>UINT32_MAX-0x70u || (elements[i]&3u))return 0;
  uint8_t *q=resolve_memory(rt,elements[i],0x70u,0);
  if(!q || resolve_memory(rt,elements[i],0x70u,1)!=q)return 0;
  uintptr_t a=(uintptr_t)q,b=(uintptr_t)bp;
  if(a<b+0x40u && b<a+0x70u)return 0;
  if(i && a<(uintptr_t)p->element_ptrs[0]+0x70u && (uintptr_t)p->element_ptrs[0]<a+0x70u)return 0;
  p->elements[i]=elements[i];p->element_ptrs[i]=q;
 }
 p->runtime=rt;p->event=event;p->count=count;p->bank_address=bank;p->bank=bp;return 1;
}
static void instrument_store(BattleMipsRuntime *rt,uint8_t *p,unsigned width,uint32_t value)
{runtime_note_store(rt,p,width);store_le(p,width,value);}
static void scratch_sound_instrument(BattleEntryCall *e)
{
 PreparedSoundInstrument *p=&prepared_instrument;uint32_t *r=e->cpu->gpr;
 if(p->runtime!=e->runtime || !PsyX_Sys_ReservedSoundBracketPristine(p->event) || r[4]!=0u)
  battle_entry_fail(e,"synthetic instrument reservation or index changed",r[4]);
 unsigned i=p->count;for(unsigned j=0;j<p->count;j++)if(p->elements[j]==r[5]){i=j;break;}
 if(i==p->count)battle_entry_fail(e,"synthetic instrument element not prepared",r[5]);
 uint8_t *q=p->element_ptrs[i],*b=p->bank;
 if(load_le(q+0x2cu,4)!=p->bank_address)battle_entry_fail(e,"synthetic instrument bank changed",r[5]);
 instrument_store(e->runtime,q+0x26u,1,r[4]);r[4]=p->bank_address+0x30u;
 r[3]=load_le(b+0x30u,4);r[2]=load_le(b+0x28u,4);r[3]<<=3;r[2]+=r[3];instrument_store(e->runtime,q+0x4cu,4,r[2]);
 r[2]=load_le(b+0x34u,2);r[2]<<=3;r[3]+=r[2];instrument_store(e->runtime,q+0x50u,4,r[3]);
 r[6]=load_le(b+0x3cu,2);r[2]=r[6]&7u;instrument_store(e->runtime,q+0x54u,1,r[2]);r[2]=(r[6]>>4)&7u;instrument_store(e->runtime,q+0x55u,1,r[2]);r[2]=(r[6]>>8)&7u;instrument_store(e->runtime,q+0x56u,1,r[2]);
 r[6]=load_le(b+0x38u,4);r[3]=r[6]>>24;r[2]=r[6]&0x7fu;instrument_store(e->runtime,q+0x57u,1,r[2]);r[2]=(r[6]>>8)&15u;instrument_store(e->runtime,q+0x58u,1,r[2]);r[2]=(r[6]>>16)&0x7fu;instrument_store(e->runtime,q+0x59u,1,r[2]);r[2]=(r[6]>>12)&15u;instrument_store(e->runtime,q+0x5bu,1,r[2]);r[2]=load_le(q,2);r[3]&=31u;instrument_store(e->runtime,q+0x28u,1,r[3]);instrument_store(e->runtime,q+0x5au,1,r[3]);r[3]=load_le(b+0x36u,2);r[2]|=0x8000u;instrument_store(e->runtime,q,2,r[2]);instrument_store(e->runtime,q+0x6cu,2,r[3]);
}
