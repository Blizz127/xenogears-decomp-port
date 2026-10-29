/* Opt-in menu-timer sprite update for disjoint, heap-owned synthetic shapes.
 * Refuse before writing if any address or the verified retail text is uncertain. */
static int battle_menu_sprite_active_try_host(BattleMipsRuntime *rt, PcPortMipsCpu *cpu, uint32_t target)
{
 if(target!=0x8001f6b0u || cpu->gpr[31]!=0x800bcc34u || rt->force_interpret ||
    !rt->initialized || g_ActiveBattleRuntime!=rt || rt->bridge_cpu!=cpu ||
    (cpu->load_valid && cpu->load_reg!=0))return 0;
 const char *enabled=getenv("XENO_BATTLE_NATIVE_MENU_SPRITE_ACTIVE_EXPERIMENT");
 if(!enabled || strcmp(enabled,"1"))return 0;
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
 size_t off=XENO_RD_SLUS_OFF(target);uint8_t *code=resolve_memory(rt,target,0xa0u,0);
 if(!image || !image->bytes || image->size<off+0xa0u || !code)return 0;
 if(memcmp(code,image->bytes+off,0xa0u)) {
  if(!PcPortMipsInterpreterDisabled())return 0;
  for(unsigned i=0;i<0xa0u;i++)if(code[i])return 0;
 }
 static const char hash[]="cb7dff828c141c1959ce71eba1492665e5bdf561cc0f7a9859ed08ac6efe2884";
 uint8_t digest[32];file1_digest(image->bytes+off,0xa0u,digest);
 for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[2*i] || file1_hex_digit(digest[i]&15u)!=hash[2*i+1])return 0;
 uint32_t sprite=cpu->gpr[4],sp=cpu->gpr[29];
 if(sprite<BATTLE_BUS_KUSEG_RAM_END || (sprite&3u) || sprite>UINT32_MAX-0x41u ||
    sp<BATTLE_BUS_KUSEG_RAM_END+8u || (sp&3u))return 0;
 uint8_t *s=resolve_memory(rt,sprite,0x41u,0),*stack=resolve_memory(rt,sp-8u,4u,1);
 if(!s || !stack || resolve_memory(rt,sp-8u,4u,0)!=stack ||
    resolve_memory(rt,sprite+0x20u,4u,0)!=s+0x20u ||
    resolve_memory(rt,sprite+0x24u,4u,0)!=s+0x24u ||
    resolve_memory(rt,sprite+0x28u,4u,0)!=s+0x28u ||
    resolve_memory(rt,sprite+0x3cu,4u,0)!=s+0x3cu ||
    resolve_memory(rt,sprite+0x40u,1u,0)!=s+0x40u)return 0;
 uint32_t flags=load_le(s+0x3cu,4),base=load_le(s+0x20u,4),other=load_le(s+0x24u,4);
 unsigned count=s[0x40u]>>2;
 if(flags!=5u || count!=10u || (base|other)&3u ||
    base<BATTLE_BUS_KUSEG_RAM_END || other<BATTLE_BUS_KUSEG_RAM_END ||
    base>UINT32_MAX-0x34u || other>UINT32_MAX-8u)return 0;
 uint8_t *b=resolve_memory(rt,base,0x34u,0),*o=resolve_memory(rt,other,8u,0);
 if(!b || !o || resolve_memory(rt,base+0x30u,4u,0)!=b+0x30u ||
    resolve_memory(rt,other+4u,4u,0)!=o+4u)return 0;
 uint32_t prim=load_le(b+0x30u,4);if(prim<BATTLE_BUS_KUSEG_RAM_END || (prim&3u) || prim>UINT32_MAX-240u)return 0;
 uint8_t *p=resolve_memory(rt,prim,240u,1);
 if(!p || resolve_memory(rt,prim,240u,0)!=p)return 0;
 for(unsigned i=0;i<count;i++)
  if(resolve_memory(rt,prim+24u*i+10u,2u,0)!=p+24u*i+10u ||
     resolve_memory(rt,prim+24u*i+10u,2u,1)!=p+24u*i+10u ||
     resolve_memory(rt,prim+24u*i+16u,4u,1)!=p+24u*i+16u)return 0;
 const uint8_t *ranges[]={s,b,o,p,stack,code};
 const size_t sizes[]={0x41u,0x34u,8u,240u,4u,0xa0u};
 for(unsigned i=0;i<6;i++)for(unsigned j=i+1;j<6;j++) {
  uintptr_t x=(uintptr_t)ranges[i],y=(uintptr_t)ranges[j];
  if(x<y+sizes[j] && y<x+sizes[i])return 0;
 }
 void **heap=dlsym(RTLD_DEFAULT,"g_Heap");uint32_t *needs=dlsym(RTLD_DEFAULT,"g_HeapNeedsConsolidation");
 MenuHeapSnapshot snapshot;
 if(!heap || !needs || !menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&snapshot))return 0;
 /* The caller stack must not overwrite heap metadata or payload. */
 uintptr_t stack_start=(uintptr_t)stack;
 uintptr_t heap_start=(uintptr_t)snapshot.blocks[0].address;
 uintptr_t heap_end=(uintptr_t)snapshot.blocks[snapshot.count-1u].address+8u;
 if(stack_start<heap_end && heap_start<stack_start+4u)return 0;
 for(unsigned k=0;k<4;k++) {
  int owned=0;uintptr_t x=(uintptr_t)ranges[k];
  for(unsigned i=0;i+1u<snapshot.count;i++) {
   unsigned tag=(snapshot.blocks[i].meta>>21)&15u;
   if(tag>1u && x>=(uintptr_t)snapshot.blocks[i].address+8u &&
      x+sizes[k]<=(uintptr_t)snapshot.blocks[i].next-8u) {owned=1;break;}
  }
  if(!owned)return 0;
 }
 uint32_t color=load_le(s+0x28u,4),abr=(flags>>5)&7u;if(abr)--abr;
 runtime_note_store(rt,stack,4);store_le(stack,4,load_le(o+4u,4));
 for(unsigned i=0;i<count;i++) {
  uint8_t *q=p+24u*i;uint32_t t=load_le(q+10u,2);
  runtime_note_store(rt,q+16u,4);store_le(q+16u,4,color);
  runtime_note_store(rt,q+10u,2);store_le(q+10u,2,(t&0xff9fu)|(abr<<5));
 }
 cpu->gpr[2]=count;cpu->gpr[3]=abr<<5;cpu->gpr[4]=prim+24u*count;
 cpu->gpr[5]=color;cpu->gpr[7]=abr;cpu->gpr[8]=count;cpu->gpr[9]=sprite;
 cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
