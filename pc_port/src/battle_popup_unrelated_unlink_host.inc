/* Popup cleanup traversal for empty lists or unrelated heap-owned nodes.
 * No callback or list mutation occurs. Work lists rely on game-thread ownership. */
static int battle_popup_unrelated_unlink_try_host(BattleMipsRuntime *rt, PcPortMipsCpu *cpu, uint32_t target)
{
    if(target!=0x8001ce74u || cpu->gpr[31]!=0x800bd070u || cpu->gpr[28]!=0x80059170u ||
       rt->force_interpret || !rt->initialized || g_ActiveBattleRuntime!=rt || rt->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
#if defined(__linux__)
    if(!rt->owner_thread_recorded || !pthread_equal(rt->owner_thread,pthread_self()))return 0;
#else
    return 0;
#endif
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_UNRELATED_UNLINK_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1"))return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t off=XENO_RD_SLUS_OFF(target);
    uint8_t *code=resolve_memory(rt,target,0x1c0u,0);
    if(!image || !image->bytes || image->size<off+0x1c0u || !code || (uintptr_t)code>UINTPTR_MAX-0x1c0u)return 0;
    if(memcmp(code,image->bytes+off,0x1c0u)) {
        if(!PcPortMipsInterpreterDisabled())return 0;
        for(unsigned i=0;i<0x1c0u;i++)if(code[i])return 0;
    }
    static const char sha[]="332c3e524bac30361defb253500baec9c7edb045233db48e6f26f1783ce37e7b";
    uint8_t digest[32];file1_digest(image->bytes+off,0x1c0u,digest);
    for(unsigned i=0;i<32;i++)
        if(file1_hex_digit(digest[i]>>4)!=sha[2*i] || file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return 0;
    const uint32_t addresses[]={0x80059594u,0x8005958cu};
    const char *names[]={"g_WorkList","g_TimerWorkList"};
    uint8_t *heads[2];uint32_t values[2];
    for(unsigned i=0;i<2;i++) {
        ResolvedData *d=find_data(rt,addresses[i],4u);
        if(!d || d->address!=addresses[i] || d->size<4u || !d->host || !d->name || strcmp(d->name,names[i]))return 0;
        heads[i]=resolve_memory(rt,addresses[i],4u,0);
        if(heads[i]!=d->host || resolve_memory(rt,addresses[i],4u,1)!=heads[i] ||
           (uintptr_t)heads[i]>UINTPTR_MAX-sizeof(uintptr_t) ||
           !host_address_valid(rt,(uintptr_t)heads[i],sizeof(uintptr_t),0))return 0;
        if(sizeof(uintptr_t)>4u && load_le(heads[i]+4u,4u))return 0;
        values[i]=load_le(heads[i],4u);
    }
    uint32_t sp=cpu->gpr[29];
    if((sp&3u) || sp<0x801fc000u+40u || sp>BATTLE_STACK_TOP)return 0;
    uint8_t *stack=resolve_memory(rt,sp-40u,40u,1);
    if(!stack || resolve_memory(rt,sp-40u,40u,0)!=stack ||
       (uintptr_t)stack>UINTPTR_MAX-40u)return 0;
    for(unsigned i=16u;i<=32u;i+=4u)
        if(resolve_memory(rt,sp-40u+i,4u,1)!=stack+i || resolve_memory(rt,sp-40u+i,4u,0)!=stack+i)return 0;
    const uintptr_t ranges[]={(uintptr_t)stack,(uintptr_t)heads[0],(uintptr_t)heads[1],(uintptr_t)code};
    const size_t sizes[]={40u,sizeof(uintptr_t),sizeof(uintptr_t),0x1c0u};
    for(unsigned i=0;i<4;i++)for(unsigned j=i+1;j<4;j++)
        if(ranges[i]<ranges[j]+sizes[j] && ranges[j]<ranges[i]+sizes[i])return 0;
    uint32_t result=cpu->gpr[2];
    if(values[0] || values[1]) {
    void **heap=dlsym(RTLD_DEFAULT,"g_Heap");
    uint32_t *needs=dlsym(RTLD_DEFAULT,"g_HeapNeedsConsolidation");
    MenuHeapSnapshot before,after;
    if(!heap || !needs || (uintptr_t)heap>UINTPTR_MAX-sizeof(*heap) || (uintptr_t)needs>UINTPTR_MAX-sizeof(*needs) ||
       !host_address_valid(rt,(uintptr_t)heap,sizeof(*heap),0) || !host_address_valid(rt,(uintptr_t)needs,sizeof(*needs),0))return 0;
    uintptr_t hv=(uintptr_t)heap,nv=(uintptr_t)needs;
    if(hv<nv+sizeof(*needs) && nv<hv+sizeof(*heap))return 0;
    for(unsigned i=0;i<4;i++)
        if((hv<ranges[i]+sizes[i] && ranges[i]<hv+sizeof(*heap)) ||
           (nv<ranges[i]+sizes[i] && ranges[i]<nv+sizeof(*needs)))return 0;
    if(*needs || !menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&before))return 0;
    if(before.blocks[0].address<(uintptr_t)g_PsxRam+0x6faf0u ||
       before.blocks[before.count-1u].address!=(uintptr_t)g_PsxRam+0x1fbff8u)return 0;
    uint8_t *nodes[16],saved[16][28];unsigned count=0;
    for(unsigned list=0;list<2;list++) {
        uint32_t current=values[list];
        while(current) {
            if(count==16u || current<BATTLE_BUS_KUSEG_RAM_END || (current&3u) || current>UINT32_MAX-28u ||
               (current&0xff800000u)==0x80000000u || (current&0xff800000u)==0xa0000000u)return 0;
            uint8_t *node=resolve_memory(rt,current,28u,0);
            if(!node || (uintptr_t)node!=current || resolve_memory(rt,current,28u,1)!=node)return 0;
            int owned=0;
            for(unsigned j=0;j+1u<before.count;j++)if(((before.blocks[j].meta>>21)&15u)>1u) {
                uintptr_t begin=(uintptr_t)before.blocks[j].address+8u,limit=(uintptr_t)before.blocks[j].next-8u;
                if((uintptr_t)node>=begin && (uintptr_t)node+28u<=limit)owned=1;
            }
            if(!owned)return 0;
            uintptr_t n=(uintptr_t)node;
            if((n<hv+sizeof(*heap) && hv<n+28u) || (n<nv+sizeof(*needs) && nv<n+28u))return 0;
            for(unsigned j=0;j<4;j++)if(n<ranges[j]+sizes[j] && ranges[j]<n+28u)return 0;
            for(unsigned j=0;j<count;j++)if(n<(uintptr_t)nodes[j]+28u && (uintptr_t)nodes[j]<n+28u)return 0;
            nodes[count]=node;memcpy(saved[count],node,28u);count++;
            result=load_le(node,4u);
            if(result==cpu->gpr[4])return 0; /* A match may invoke a callback. */
            current=load_le(node+24u,4u);
        }
    }
    if(!menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&after) ||
       memcmp(&before,&after,sizeof before))return 0;
    for(unsigned i=0;i<2;i++)
        if(load_le(heads[i],4u)!=values[i] || (sizeof(uintptr_t)>4u && load_le(heads[i]+4u,4u)))return 0;
    for(unsigned i=0;i<count;i++)if(memcmp(nodes[i],saved[i],28u))return 0;
    } else if(load_le(heads[0],4u) || load_le(heads[1],4u) ||
       (sizeof(uintptr_t)>4u && (load_le(heads[0]+4u,4u) || load_le(heads[1]+4u,4u))))return 0;
    const unsigned regs[]={16u,19u,17u,31u,18u},offsets[]={16u,28u,20u,32u,24u};
    for(unsigned i=0;i<5;i++) {runtime_note_store(rt,stack+offsets[i],4u);store_le(stack+offsets[i],4u,cpu->gpr[regs[i]]);}
    cpu->gpr[2]=result;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
