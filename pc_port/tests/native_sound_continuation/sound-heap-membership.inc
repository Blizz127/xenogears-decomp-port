/* Synthetic-only sound heap membership capture. The preflight resolves the
 * whole known arena before lock; capture walks only bounded in-arena headers
 * while the raw SDL reservation is held. A match is not a lifetime guarantee. */
typedef struct {BattleMipsRuntime *runtime;uint32_t base,size,manager,count;unsigned event;} SyntheticSoundHeap;
static int prepare_synthetic_sound_heap(BattleMipsRuntime *rt,uint32_t base,uint32_t size,
                                        uint32_t manager,uint32_t count,unsigned event,SyntheticSoundHeap *p)
{
 memset(p,0,sizeof *p);
 if(!event || !size || count>24u || base<BATTLE_BUS_KUSEG_RAM_END || (base&15u) ||
    base>UINT32_MAX-size || manager<base+16u || manager>base+size ||
    !host_address_valid(rt,base,size,0))return 0;
 p->runtime=rt;p->base=base;p->size=size;p->manager=manager;p->count=count;p->event=event;return 1;
}
static int capture_synthetic_sound_heap_state(const SyntheticSoundHeap *p,int enabled)
{
 if(!p->runtime || !PsyX_Sys_ValidateSoundEventReservation(p->event,enabled))return 0;
 uint32_t current=p->base,limit=p->base+p->size;int found=0;
 for(unsigned i=0;i<64u;i++) {
  if((current&15u) || current<p->base || current>limit-16u)return 0;
  const uint8_t *header=(const uint8_t*)(uintptr_t)current;
  uint32_t flags=load_le(header,2),end=load_le(header+8u,4),next=load_le(header+12u,4);
  if((i==0 && flags!=0x8000u) || (i!=0 && flags!=2u) || end<current+16u || end>limit ||
     (next && ((next&15u) || next<end || next>limit-16u)))return 0;
  if(i && current+16u==p->manager) {
   uint32_t needed=0x94u+p->count*0x158u;
   if(p->manager>limit-needed || end<p->manager+needed || found ||
      load_le((const uint8_t*)(uintptr_t)p->manager+0x14u,1)!=p->count)return 0;
   found=1;
  }
  if(!next)return found;current=next;
 }
 return 0;
}
static int capture_synthetic_sound_heap(const SyntheticSoundHeap *p)
{
 return capture_synthetic_sound_heap_state(p,1);
}
/* Validate a small synthetic linked-manager list while the same raw
 * reservation is held. This is a snapshot, not a callback lifetime proof. */
static int capture_synthetic_manager_list_state(const SyntheticSoundHeap *p,uint32_t head,int enabled)
{
 if(!p->runtime || !PsyX_Sys_ValidateSoundEventReservation(p->event,enabled))return 0;
 uint32_t seen[8];unsigned count=0;int selected=0;
 while(head && count<8u) {
  if(head<p->base || head>p->base+p->size-0x94u || (head&3u))return 0;
  for(unsigned i=0;i<count;i++)if(seen[i]==head)return 0;
  seen[count++]=head;unsigned elements=load_le((const uint8_t*)(uintptr_t)head+0x14u,1);
  SyntheticSoundHeap member=*p;member.manager=head;member.count=elements;
  if(elements>24u || !capture_synthetic_sound_heap_state(&member,enabled))return 0;
  if(head==p->manager)selected=1;
  head=load_le((const uint8_t*)(uintptr_t)head,4);
 }
 return !head && selected;
}
static int capture_synthetic_manager_list(const SyntheticSoundHeap *p,uint32_t head)
{
 return capture_synthetic_manager_list_state(p,head,1);
}
