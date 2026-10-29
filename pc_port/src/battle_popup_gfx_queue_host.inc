/* Opt-in popup cleanup queue leaf. Only the first deferred node in empty
 * context lists is handled; uncertain allocation or aliasing refuses. */
static int battle_popup_gfx_queue_try_host(BattleMipsRuntime *rt,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target!=0x80025180u || cpu->gpr[31]!=0x800bd068u || cpu->gpr[28]!=0x80059170u ||
    rt->force_interpret || !rt->initialized || g_ActiveBattleRuntime!=rt || rt->bridge_cpu!=cpu ||
    (cpu->load_valid && cpu->load_reg!=0))return 0;
 const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_GFX_QUEUE_EXPERIMENT");if(!enabled || strcmp(enabled,"1"))return 0;
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t off=XENO_RD_SLUS_OFF(target);
 uint8_t *code=resolve_memory(rt,target,0x48u,0);if(!image || !image->bytes || image->size<off+0x48u || !code)return 0;
 if(memcmp(code,image->bytes+off,0x48u)) {
  if(!PcPortMipsInterpreterDisabled())return 0;
  for(unsigned i=0;i<0x48u;i++)if(code[i])return 0;
 }
 static const char sha[]="eff26e554ff18f383cf430aaa5e062001b0c1cc27e6955086a202b3c5509c818";
 uint8_t digest[32];file1_digest(image->bytes+off,0x48u,digest);
 for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=sha[2*i] || file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return 0;
 const uint32_t addrs[]={0x800592f8u,0x800592fcu,0x80059300u,0x80059304u,0x800594b4u,0x800594b8u,0x80059534u,0x80059580u};
 const char *names[]={"g_GfxCurContext","g_GfxWorkBufferSize","D_80059300","D_80059304","g_GfxWorkBuffers","g_GfxWorkBuffer2","g_GfxCurWorkBufferEnd","g_GfxCurWorkBuffer"};
 uint8_t *globals[8];uint32_t vals[8];
 for(unsigned i=0;i<8;i++) {
  ResolvedData *d=find_data(rt,addrs[i],4u);
  if(!d || d->address!=addrs[i] || d->size<4u || !d->host || !d->name || strcmp(d->name,names[i]))return 0;
  globals[i]=resolve_memory(rt,addrs[i],4u,0);
  if(globals[i]!=d->host || resolve_memory(rt,addrs[i],4u,1)!=globals[i])return 0;
  /* These host objects are pointers even though the guest slots are 32-bit.
   * A low-word update is safe only when the upper host word is already zero. */
  if(i>=4u && sizeof(uintptr_t)>4u &&
     (!host_address_valid(rt,(uintptr_t)globals[i],sizeof(uintptr_t),0) || load_le(globals[i]+4u,4u)))return 0;
  vals[i]=load_le(globals[i],4);
 }
 uint32_t context=vals[0],size=vals[1],base=context<2u?vals[4u+context]:0,cursor=vals[7],end=vals[6],payload=cpu->gpr[4],sp=cpu->gpr[29];
 if(context>1u || vals[2] || vals[3] || size<8u || size>0x100000u || base<BATTLE_BUS_KUSEG_RAM_END ||
    base>UINT32_MAX-size || end!=base+size || cursor<base || cursor>UINT32_MAX-8u || cursor+8u>end ||
    (cursor&3u) || payload<BATTLE_BUS_KUSEG_RAM_END || (payload&3u) || payload>UINT32_MAX-416u ||
    sp<BATTLE_BUS_KUSEG_RAM_END || (sp&3u) || sp>UINT32_MAX-32u)return 0;
 uint8_t *buffer=resolve_memory(rt,base,size,1),*node=resolve_memory(rt,cursor,8u,1),*data=resolve_memory(rt,payload,416u,0),*stack=resolve_memory(rt,sp,32u,0);
 if(!buffer || !node || !data || !stack || resolve_memory(rt,base,size,0)!=buffer ||
    resolve_memory(rt,cursor,8u,0)!=node || node!=buffer+(cursor-base) ||
    resolve_memory(rt,cursor,4u,1)!=node || resolve_memory(rt,cursor+4u,4u,1)!=node+4u)return 0;
 const uintptr_t ranges[]={(uintptr_t)node,(uintptr_t)data,(uintptr_t)stack,(uintptr_t)code};
 const size_t lengths[]={8u,416u,32u,0x48u};
 for(unsigned i=0;i<4;i++)for(unsigned j=i+1;j<4;j++)if(ranges[i]<ranges[j]+lengths[j] && ranges[j]<ranges[i]+lengths[i])return 0;
 for(unsigned i=0;i<8;i++)for(unsigned j=0;j<4;j++) {
  uintptr_t a=(uintptr_t)globals[i],b=ranges[j];size_t width=i>=4u?sizeof(uintptr_t):4u;
  if(a<b+lengths[j] && b<a+width)return 0;
 }
 for(unsigned i=0;i<8;i++)for(unsigned j=i+1;j<8;j++)
  if((uintptr_t)globals[i]<(uintptr_t)globals[j]+(j>=4u?sizeof(uintptr_t):4u) &&
     (uintptr_t)globals[j]<(uintptr_t)globals[i]+(i>=4u?sizeof(uintptr_t):4u))return 0;
 if((uintptr_t)buffer<(uintptr_t)data+416u && (uintptr_t)data<(uintptr_t)buffer+size)return 0;
 void **heap=dlsym(RTLD_DEFAULT,"g_Heap");uint32_t *needs=dlsym(RTLD_DEFAULT,"g_HeapNeedsConsolidation");MenuHeapSnapshot snap;
 if(!heap || !needs || !menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&snap))return 0;
 int buffer_block=-1,data_block=-1;
 for(unsigned i=0;i+1u<snap.count;i++)if(((snap.blocks[i].meta>>21)&15u)>1u) {
  uintptr_t begin=(uintptr_t)snap.blocks[i].address+8u,limit=(uintptr_t)snap.blocks[i].next-8u;
  if((uintptr_t)buffer>=begin && (uintptr_t)buffer+size<=limit)buffer_block=(int)i;
  if((uintptr_t)data==begin && (uintptr_t)data+416u<=limit)data_block=(int)i;
 }
 if(buffer_block<0 || data_block<0 || buffer_block==data_block)return 0;
 runtime_note_store(rt,globals[7],4);store_le(globals[7],4,cursor+8u);
 runtime_note_store(rt,node,4);store_le(node,4,payload);
 runtime_note_store(rt,node+4u,4);store_le(node+4u,4,0u);
 runtime_note_store(rt,globals[2u+context],4);store_le(globals[2u+context],4,cursor);
 cpu->gpr[5]=cursor;cpu->gpr[2]=0x80059300u+4u*context;cpu->gpr[3]=0;
 cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
