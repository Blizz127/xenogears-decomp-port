/* One matching work or timer node with a null callback; all others are unrelated.
 * The game thread owns the lists. No callback is dispatched. */
static int battle_popup_one_null_unlink_try_host(BattleMipsRuntime *rt,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8001ce74u || cpu->gpr[31]!=0x800bd070u || cpu->gpr[28]!=0x80059170u ||
       rt->force_interpret || !rt->initialized || g_ActiveBattleRuntime!=rt || rt->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
#if defined(__linux__)
    if(!rt->owner_thread_recorded || !pthread_equal(rt->owner_thread,pthread_self()))return 0;
#else
    return 0;
#endif
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_ONE_NULL_UNLINK_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1"))return 0;
    uint32_t task=cpu->gpr[4],sp=cpu->gpr[29];
    if(task<BATTLE_BUS_KUSEG_RAM_END || (task&3u) || task>UINT32_MAX-28u ||
       (task&0xff800000u)==0x80000000u || (task&0xff800000u)==0xa0000000u ||
       (sp&3u) || sp<0x801fc000u+40u || sp>BATTLE_STACK_TOP)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t off=XENO_RD_SLUS_OFF(target);uint8_t *code=resolve_memory(rt,target,0x1c0u,0);
    if(!image || !image->bytes || image->size<off+0x1c0u || !code || (uintptr_t)code>UINTPTR_MAX-0x1c0u)return 0;
    if(memcmp(code,image->bytes+off,0x1c0u)) {
        if(!PcPortMipsInterpreterDisabled())return 0;
        for(unsigned i=0;i<0x1c0u;i++)if(code[i])return 0;
    }
    static const char sha[]="332c3e524bac30361defb253500baec9c7edb045233db48e6f26f1783ce37e7b";
    uint8_t digest[32];file1_digest(image->bytes+off,0x1c0u,digest);
    for(unsigned i=0;i<32;i++)
        if(file1_hex_digit(digest[i]>>4)!=sha[2*i] || file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return 0;
    const uint32_t addresses[]={0x80059594u,0x8005958cu,0x80059590u};
    const char *names[]={"g_WorkList","g_TimerWorkList","D_80059590"};
    uint8_t *globals[3];uint32_t values[3];
    for(unsigned i=0;i<3;i++) {
        ResolvedData *d=find_data(rt,addresses[i],4u);
        if(!d || d->address!=addresses[i] || d->size<4u || !d->host || !d->name || strcmp(d->name,names[i]))return 0;
        globals[i]=resolve_memory(rt,addresses[i],4u,0);
        if(globals[i]!=d->host || resolve_memory(rt,addresses[i],4u,1)!=globals[i] ||
           (uintptr_t)globals[i]>UINTPTR_MAX-sizeof(uintptr_t) ||
           !host_address_valid(rt,(uintptr_t)globals[i],sizeof(uintptr_t),1) ||
           !host_address_valid(rt,(uintptr_t)globals[i],sizeof(uintptr_t),0))return 0;
        if(sizeof(uintptr_t)>4u && load_le(globals[i]+4u,4u))return 0;
        values[i]=load_le(globals[i],4u);
    }
    uint8_t *stack=resolve_memory(rt,sp-40u,40u,1),*owner=resolve_memory(rt,task,20u,0);
    if(!stack || resolve_memory(rt,sp-40u,40u,0)!=stack || (uintptr_t)stack>UINTPTR_MAX-40u ||
       !owner || (uintptr_t)owner!=task || resolve_memory(rt,task,20u,1)!=owner)return 0;
    for(unsigned i=16u;i<=32u;i+=4u)
        if(resolve_memory(rt,sp-40u+i,4u,1)!=stack+i || resolve_memory(rt,sp-40u+i,4u,0)!=stack+i)return 0;
    void **heap=dlsym(RTLD_DEFAULT,"g_Heap");uint32_t *needs=dlsym(RTLD_DEFAULT,"g_HeapNeedsConsolidation");
    MenuHeapSnapshot before,after;
    if(!heap || !needs || (uintptr_t)heap>UINTPTR_MAX-sizeof(*heap) || (uintptr_t)needs>UINTPTR_MAX-sizeof(*needs) ||
       !host_address_valid(rt,(uintptr_t)heap,sizeof(*heap),0) || !host_address_valid(rt,(uintptr_t)needs,sizeof(*needs),0))return 0;
    const uintptr_t ranges[]={(uintptr_t)stack,(uintptr_t)code,(uintptr_t)globals[0],(uintptr_t)globals[1],(uintptr_t)globals[2],(uintptr_t)heap,(uintptr_t)needs};
    const size_t sizes[]={40u,0x1c0u,sizeof(uintptr_t),sizeof(uintptr_t),sizeof(uintptr_t),sizeof(*heap),sizeof(*needs)};
    for(unsigned i=0;i<7;i++)for(unsigned j=i+1;j<7;j++)
        if(ranges[i]<ranges[j]+sizes[j] && ranges[j]<ranges[i]+sizes[i])return 0;
    for(unsigned i=0;i<7;i++)if((uintptr_t)owner<ranges[i]+sizes[i] && ranges[i]<(uintptr_t)owner+20u)return 0;
    if(*needs || !menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&before) ||
       before.blocks[0].address<(uintptr_t)g_PsxRam+0x6faf0u ||
       before.blocks[before.count-1u].address!=(uintptr_t)g_PsxRam+0x1fbff8u)return 0;
    int owner_owned=0;
    for(unsigned j=0;j+1u<before.count;j++)if(((before.blocks[j].meta>>21)&15u)>1u) {
        uintptr_t begin=(uintptr_t)before.blocks[j].address+8u,limit=(uintptr_t)before.blocks[j].next-8u;
        if((uintptr_t)owner>=begin && (uintptr_t)owner+20u<=limit)owner_owned=1;
    }
    if(!owner_owned)return 0;
    uint8_t owner_saved[20];memcpy(owner_saved,owner,20u);
    uint32_t owner_id=load_le(owner+16u,4u)&0x1fffffffu,result2=cpu->gpr[2];
    uint8_t *nodes[2][16],saved[2][16][28];uint32_t words[2][16];unsigned counts[2]={0,0},found=16u,found_list=0;
    for(unsigned list=0;list<2;list++) {
        uint32_t current=values[list];
        while(current) {
            unsigned n=counts[list];
            if(n==16u || current<BATTLE_BUS_KUSEG_RAM_END || (current&3u) || current>UINT32_MAX-28u ||
               (current&0xff800000u)==0x80000000u || (current&0xff800000u)==0xa0000000u)return 0;
            uint8_t *node=resolve_memory(rt,current,28u,1);
            if(!node || (uintptr_t)node!=current || resolve_memory(rt,current,28u,0)!=node)return 0;
            int owned=0;
            for(unsigned j=0;j+1u<before.count;j++)if(((before.blocks[j].meta>>21)&15u)>1u) {
                uintptr_t begin=(uintptr_t)before.blocks[j].address+8u,limit=(uintptr_t)before.blocks[j].next-8u;
                if((uintptr_t)node>=begin && (uintptr_t)node+28u<=limit)owned=1;
            }
            if(!owned)return 0;
            uintptr_t p=(uintptr_t)node,o=(uintptr_t)owner;
            if(p!=o && p<o+20u && o<p+28u)return 0;
            for(unsigned j=0;j<7;j++)if(p<ranges[j]+sizes[j] && ranges[j]<p+28u)return 0;
            for(unsigned l=0;l<=list;l++)for(unsigned j=0;j<counts[l];j++)
                if(p<(uintptr_t)nodes[l][j]+28u && (uintptr_t)nodes[l][j]<p+28u)return 0;
            nodes[list][n]=node;words[list][n]=current;memcpy(saved[list][n],node,28u);counts[list]++;
            uint32_t owner_word=load_le(node,4u);result2=owner_word;
            if(owner_word==task) {
                uint32_t flags=load_le(node+20u,4u);
                if(found!=16u || (flags&0x40000000u) || (flags&0x1fffffffu)!=owner_id || load_le(node+12u,4u))return 0;
                found=n;found_list=list;result2=0;
            }
            current=load_le(node+24u,4u);
        }
    }
    if(found==16u)return 0;
    if(!menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&after) || memcmp(&before,&after,sizeof before))return 0;
    for(unsigned i=0;i<3;i++)if(load_le(globals[i],4u)!=values[i] ||
       (sizeof(uintptr_t)>4u && load_le(globals[i]+4u,4u)))return 0;
    if(memcmp(owner,owner_saved,20u))return 0;
    for(unsigned l=0;l<2;l++)for(unsigned i=0;i<counts[l];i++)if(memcmp(nodes[l][i],saved[l][i],28u))return 0;
    const unsigned regs[]={16u,19u,17u,31u,18u},offsets[]={16u,28u,20u,32u,24u};
    for(unsigned i=0;i<5;i++) {runtime_note_store(rt,stack+offsets[i],4u);store_le(stack+offsets[i],4u,cpu->gpr[regs[i]]);}
    uint32_t next=load_le(nodes[found_list][found]+24u,4u);
    uint8_t *link=found?nodes[found_list][found-1u]+24u:globals[found_list];
    runtime_note_store(rt,link,4u);store_le(link,4u,next);
    if(values[2]==words[found_list][found]) {runtime_note_store(rt,globals[2],4u);store_le(globals[2],4u,next);}
    cpu->gpr[2]=result2;cpu->gpr[3]=owner_id;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
