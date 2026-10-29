/* Scratch-only LP64 sound-channel projection; not connected to live audio. */
static int scratch_sound_slot(BattleMipsRuntime *rt,uint32_t address,unsigned width,uint8_t **slot,unsigned *offset)
{
 uint32_t a=address;if(a<0x00200000u)a|=0x80000000u;else if((a&0xff800000u)==0xa0000000u)a-=0x20000000u;
 if(a<0x8006252cu||a>=0x8006258cu)return 0;
 unsigned rel=a-0x8006252cu;*offset=rel&3u;if((width!=1&&width!=2&&width!=4)||*offset+width>4u)return -1;
 ResolvedData *d=find_data(rt,0x8006252cu,192u);if(!d||d->address!=0x8006252cu||d->size<192u||!d->name||strcmp(d->name,"g_SoundChannels"))return -1;
 *slot=d->host+(rel/4u)*sizeof(void*);return 1;
}
static int scratch_sound_pointer_word(uint8_t *slot,uint32_t *word)
{
 uintptr_t host=0;memcpy(&host,slot,sizeof(void*));if(host>UINT32_MAX)return -1;*word=(uint32_t)host;return 0;
}
static int scratch_sound_project_read(BattleMipsRuntime *rt,uint32_t address,unsigned width,uint32_t *value)
{
 uint8_t *slot;unsigned offset;int t=scratch_sound_slot(rt,address,width,&slot,&offset);if(t<=0)return t;uint32_t word;if(scratch_sound_pointer_word(slot,&word))return -1;
 const char *guest=getenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW");
 if(guest&&(!strcmp(guest,"1")||!strcmp(guest,"2"))&&word) {
  uintptr_t base=(uintptr_t)g_PsxRam,host=word;
  if(host<base || host>=base+BATTLE_BUS_KUSEG_RAM_END)return -1;
  word=(!strcmp(guest,"2")?0xa0000000u:0x80000000u)+(uint32_t)(host-base);
 }
 *value=width==4?word:(word>>(offset*8u))&((1u<<(width*8u))-1u);return 1;
}
static int scratch_sound_project_write(BattleMipsRuntime *rt,uint32_t address,unsigned width,uint32_t value)
{
 uint8_t *slot;unsigned offset;int t=scratch_sound_slot(rt,address,width,&slot,&offset);if(t<=0)return t;uint32_t old;if(scratch_sound_pointer_word(slot,&old))return -1;
 const char *guest=getenv("XENO_SCRATCH_SOUND_SLOT_GUEST_VIEW");int guest_view=guest&&(!strcmp(guest,"1")||!strcmp(guest,"2"));
 if(guest_view&&old){uintptr_t base=(uintptr_t)g_PsxRam;if(old<base || old>=base+BATTLE_BUS_KUSEG_RAM_END)return -1;old=(!strcmp(guest,"2")?0xa0000000u:0x80000000u)+(uint32_t)(old-base);}
 uint32_t mask=width==4?UINT32_MAX:((1u<<(width*8u))-1u)<<(offset*8u),word=(old&~mask)|((value<<(offset*8u))&mask);uint8_t *host=NULL;
 if(word){if(guest_view){if((word&0xffe00000u)!=0x80000000u && (word&0xffe00000u)!=0xa0000000u)return -1;host=g_PsxRam+(word&0x1fffffu);}
 else {if((word&0xff800000u)==0x80000000u||(word&0xff800000u)==0xa0000000u||!host_address_valid(rt,(uintptr_t)word,1,0))return -1;host=(uint8_t *)(uintptr_t)word;}}
 memcpy(slot,&host,sizeof host);return 1;
}
