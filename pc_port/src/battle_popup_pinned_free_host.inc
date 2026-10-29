/* Scoped pinned-block HeapFree return. No block or heap metadata is written;
 * work-list/heap ownership is assumed to remain on the game thread. */
static int battle_popup_pinned_free_try_host(BattleMipsRuntime *rt, PcPortMipsCpu *cpu, uint32_t target)
{
    if(target!=0x800320e8u || cpu->gpr[31]!=0x800bd080u || cpu->gpr[28]!=0x80059170u ||
       rt->force_interpret || !rt->initialized || g_ActiveBattleRuntime!=rt || rt->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
#if defined(__linux__)
    if(!rt->owner_thread_recorded || !pthread_equal(rt->owner_thread,pthread_self()))return 0;
#else
    return 0;
#endif
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_PINNED_FREE_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1"))return 0;
    uint32_t block=cpu->gpr[4],sp=cpu->gpr[29];
    if(block<BATTLE_BUS_KUSEG_RAM_END || (block&3u) || block>UINT32_MAX-4u ||
       (block&0xff800000u)==0x80000000u || (block&0xff800000u)==0xa0000000u ||
       (sp&3u) || sp<0x801fc000u+32u || sp>BATTLE_STACK_TOP)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t off=XENO_RD_SLUS_OFF(target);uint8_t *code=resolve_memory(rt,target,0xa4u,0);
    if(!image || !image->bytes || image->size<off+0xa4u || !code || (uintptr_t)code>UINTPTR_MAX-0xa4u)return 0;
    if(memcmp(code,image->bytes+off,0xa4u)) {
        if(!PcPortMipsInterpreterDisabled())return 0;
        for(unsigned i=0;i<0xa4u;i++)if(code[i])return 0;
    }
    static const char sha[]="172bdbe744af67620d606319e150a97318f12d59d115f81d13748c22a7bcf7ac";
    uint8_t digest[32];file1_digest(image->bytes+off,0xa4u,digest);
    for(unsigned i=0;i<32;i++)
        if(file1_hex_digit(digest[i]>>4)!=sha[2*i] || file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return 0;
    uint8_t *metadata=resolve_memory(rt,block-4u,4u,0),*stack=resolve_memory(rt,sp-8u,4u,1);
    if(!metadata || (uintptr_t)metadata!=block-4u || !stack || resolve_memory(rt,sp-8u,4u,0)!=stack ||
       (uintptr_t)stack>UINTPTR_MAX-4u)return 0;
    void **heap=dlsym(RTLD_DEFAULT,"g_Heap");uint32_t *needs=dlsym(RTLD_DEFAULT,"g_HeapNeedsConsolidation");
    MenuHeapSnapshot before,after;
    if(!heap || !needs || (uintptr_t)heap>UINTPTR_MAX-sizeof(*heap) || (uintptr_t)needs>UINTPTR_MAX-sizeof(*needs) ||
       !host_address_valid(rt,(uintptr_t)heap,sizeof(*heap),0) || !host_address_valid(rt,(uintptr_t)needs,sizeof(*needs),0))return 0;
    const uintptr_t ranges[]={(uintptr_t)stack,(uintptr_t)metadata,(uintptr_t)code,(uintptr_t)heap,(uintptr_t)needs};
    const size_t sizes[]={4u,4u,0xa4u,sizeof(*heap),sizeof(*needs)};
    for(unsigned i=0;i<5;i++)for(unsigned j=i+1;j<5;j++)
        if(ranges[i]<ranges[j]+sizes[j] && ranges[j]<ranges[i]+sizes[i])return 0;
    if(*needs || !menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&before) ||
       before.blocks[0].address<(uintptr_t)g_PsxRam+0x6faf0u ||
       before.blocks[before.count-1u].address!=(uintptr_t)g_PsxRam+0x1fbff8u)return 0;
    uint32_t meta=load_le(metadata,4u);int owned=0;
    for(unsigned i=0;i+1u<before.count;i++)
        if((uintptr_t)before.blocks[i].address+8u==block && before.blocks[i].meta==meta &&
           ((meta>>21)&15u)>1u)owned=1;
    if(!owned || !(meta&0x02000000u))return 0;
    if(!menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&after) ||
       memcmp(&before,&after,sizeof before) || load_le(metadata,4u)!=meta)return 0;
    runtime_note_store(rt,stack,4u);store_le(stack,4u,cpu->gpr[31]);
    cpu->gpr[5]=block;cpu->gpr[4]=meta;cpu->gpr[3]=0x03ff0000u;cpu->gpr[2]=UINT32_MAX;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
