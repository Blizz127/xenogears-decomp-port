/* Color-fade cleanup work-list leaf. This deliberately handles only bounded,
 * host-addressed heap-owned lists. It relies on the existing game-thread
 * ownership of work lists; it does not dispatch callbacks or take a lock. */
static int battle_color_work_refuse(const PcPortMipsCpu *cpu, unsigned line)
{
    const char *diagnostic=getenv("XENO_BATTLE_COLOR_WORK_REMOVE_DIAG");
    if(diagnostic && strcmp(diagnostic,"1")==0) {
        fprintf(stderr,"[xeno-port][color-work-guard] line=%u a0=%08x gp=%08x ra=%08x\n",
                line,cpu->gpr[4],cpu->gpr[28],cpu->gpr[31]);
        void **head=dlsym(RTLD_DEFAULT,"g_WorkList"),**heap=dlsym(RTLD_DEFAULT,"g_Heap");
        uint32_t *count=dlsym(RTLD_DEFAULT,"g_NumWorkListEntries");
        uintptr_t base=(uintptr_t)g_PsxRam,node=head?(uintptr_t)*head:0;
        fprintf(stderr,"[xeno-port][color-work-guard] ram=%p head=%p heap=%p count=%u slots=%08x,%08x\n",
                (void*)base,(void*)node,heap?*heap:NULL,count?*count:0u,
                base?load_le(g_PsxRam+0xc3554u,4u):0u,base?load_le(g_PsxRam+0xc3558u,4u):0u);
        if(base && node>=base && node-base<=PSX_RAM_SIZE-28u)
            fprintf(stderr,"[xeno-port][color-work-guard] head-owner=%08x head-next=%08x\n",
                    load_le((uint8_t*)node,4u),load_le((uint8_t*)node+24u,4u));
    }
    return 0;
}

static int battle_color_work_remove_try_host(BattleMipsRuntime *rt,
                                               PcPortMipsCpu *cpu,
                                               uint32_t target)
{
    static const uint32_t addresses[] = {0x80059594u,0x80059590u,0x8005918cu};
    static const char *names[] = {"g_WorkList","D_80059590","g_NumWorkListEntries"};
    uint8_t *globals[3];
    uint32_t values[3];
    if (target!=0x8001cb48u || cpu->gpr[31]!=0x800b3854u || cpu->gpr[28]!=0x80059170u ||
        rt->force_interpret || !rt->initialized || g_ActiveBattleRuntime!=rt || rt->bridge_cpu!=cpu ||
        (cpu->load_valid && cpu->load_reg!=0)) return battle_color_work_refuse(cpu,__LINE__);
#if defined(__linux__)
    if(!rt->owner_thread_recorded || !pthread_equal(rt->owner_thread,pthread_self()))return battle_color_work_refuse(cpu,__LINE__);
#else
    return battle_color_work_refuse(cpu,__LINE__);
#endif
    const char *enabled=getenv("XENO_BATTLE_NATIVE_COLOR_WORK_REMOVE_EXPERIMENT");
    if (!enabled || strcmp(enabled,"1")) return battle_color_work_refuse(cpu,__LINE__);
    uint32_t task=cpu->gpr[4];
    if (task<BATTLE_BUS_KUSEG_RAM_END || (task&3u) || task>UINT32_MAX-28u ||
        (task&0xff800000u)==0x80000000u || (task&0xff800000u)==0xa0000000u) return battle_color_work_refuse(cpu,__LINE__);

    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t off=XENO_RD_SLUS_OFF(target);
    uint8_t *code=resolve_memory(rt,target,0xa0u,0);
    if (!image || !image->bytes || image->size<off+0xa0u || !code || (uintptr_t)code>UINTPTR_MAX-0xa0u) return battle_color_work_refuse(cpu,__LINE__);
    if (memcmp(code,image->bytes+off,0xa0u)) {
        if (!PcPortMipsInterpreterDisabled()) return battle_color_work_refuse(cpu,__LINE__);
        for (unsigned i=0;i<0xa0u;i++) if(code[i]) return battle_color_work_refuse(cpu,__LINE__);
    }
    static const char sha[]="442d1deeeae343e5b0d5a373b425430fd9817b83727f5939e11554e96d226be9";
    uint8_t digest[32];file1_digest(image->bytes+off,0xa0u,digest);
    for (unsigned i=0;i<32;i++)
        if(file1_hex_digit(digest[i]>>4)!=sha[2*i] || file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return battle_color_work_refuse(cpu,__LINE__);

    for(unsigned i=0;i<3;i++) {
        ResolvedData *d=find_data(rt,addresses[i],4u);
        if(!d || d->address!=addresses[i] || d->size<4u || !d->host || !d->name || strcmp(d->name,names[i]))return battle_color_work_refuse(cpu,__LINE__);
        globals[i]=resolve_memory(rt,addresses[i],4u,0);
        if(globals[i]!=d->host || resolve_memory(rt,addresses[i],4u,1)!=globals[i])return battle_color_work_refuse(cpu,__LINE__);
        size_t width=i<2u?sizeof(uintptr_t):4u;
        if((uintptr_t)globals[i]>UINTPTR_MAX-width || !host_address_valid(rt,(uintptr_t)globals[i],width,1))return battle_color_work_refuse(cpu,__LINE__);
        if(i<2u && sizeof(uintptr_t)>4u &&
           (!host_address_valid(rt,(uintptr_t)globals[i],sizeof(uintptr_t),0) || load_le(globals[i]+4u,4u)))return battle_color_work_refuse(cpu,__LINE__);
        values[i]=load_le(globals[i],4);
    }
    for(unsigned i=0;i<3;i++)for(unsigned j=i+1;j<3;j++) {
        uintptr_t a=(uintptr_t)globals[i],b=(uintptr_t)globals[j];
        size_t aw=i<2u?sizeof(uintptr_t):4u,bw=j<2u?sizeof(uintptr_t):4u;
        if(a<b+bw && b<a+aw)return battle_color_work_refuse(cpu,__LINE__);
    }
    for(unsigned i=0;i<3;i++) {
        uintptr_t g=(uintptr_t)globals[i],c=(uintptr_t)code;size_t width=i<2u?sizeof(uintptr_t):4u;
        if(g<c+0xa0u && c<g+width)return battle_color_work_refuse(cpu,__LINE__);
    }
    if(values[2]==0 || values[2]>16u)return battle_color_work_refuse(cpu,__LINE__);
    void **heap=dlsym(RTLD_DEFAULT,"g_Heap");uint32_t *needs=dlsym(RTLD_DEFAULT,"g_HeapNeedsConsolidation");
    MenuHeapSnapshot before,after;
    if(!heap || !needs || (uintptr_t)heap>UINTPTR_MAX-sizeof(*heap) || (uintptr_t)needs>UINTPTR_MAX-sizeof(*needs) ||
       !host_address_valid(rt,(uintptr_t)heap,sizeof(*heap),0) || !host_address_valid(rt,(uintptr_t)needs,sizeof(*needs),0))return battle_color_work_refuse(cpu,__LINE__);
    if((uintptr_t)heap<(uintptr_t)needs+sizeof(*needs) && (uintptr_t)needs<(uintptr_t)heap+sizeof(*heap))return battle_color_work_refuse(cpu,__LINE__);
    for(unsigned i=0;i<3;i++) {
        uintptr_t g=(uintptr_t)globals[i];size_t width=i<2u?sizeof(uintptr_t):4u;
        if((g<(uintptr_t)heap+sizeof(*heap) && (uintptr_t)heap<g+width) ||
           (g<(uintptr_t)needs+sizeof(*needs) && (uintptr_t)needs<g+width))return battle_color_work_refuse(cpu,__LINE__);
    }
    if(*needs || !menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&before))return battle_color_work_refuse(cpu,__LINE__);

    if(before.blocks[0].address<(uintptr_t)g_PsxRam+0x6faf0u ||
       before.blocks[before.count-1u].address!=(uintptr_t)g_PsxRam+0x1fbff8u)return battle_color_work_refuse(cpu,__LINE__);

    uint8_t *nodes[16];uint8_t saved[16][28];uint32_t words[16];unsigned count=0,found=16u;
    uint32_t current=values[0];
    while(current) {
        if(count>=16u || current<BATTLE_BUS_KUSEG_RAM_END || (current&3u) || current>UINT32_MAX-28u ||
           (current&0xff800000u)==0x80000000u || (current&0xff800000u)==0xa0000000u)return battle_color_work_refuse(cpu,__LINE__);
        for(unsigned j=0;j<count;j++)if(words[j]==current)return battle_color_work_refuse(cpu,__LINE__);
        uint8_t *node=resolve_memory(rt,current,28u,1);
        if(!node || resolve_memory(rt,current,28u,0)!=node || (uintptr_t)node!=current)return battle_color_work_refuse(cpu,__LINE__);
        int owned=0;
        for(unsigned j=0;j+1u<before.count;j++)if(((before.blocks[j].meta>>21)&15u)>1u) {
            uintptr_t begin=(uintptr_t)before.blocks[j].address+8u,limit=(uintptr_t)before.blocks[j].next-8u;
            if((uintptr_t)node>=begin && (uintptr_t)node+28u<=limit)owned=1;
        }
        if(!owned)return battle_color_work_refuse(cpu,__LINE__);
        uintptr_t n=(uintptr_t)node,c=(uintptr_t)code;
        if(n<c+0xa0u && c<n+28u)return battle_color_work_refuse(cpu,__LINE__);
        if((n<(uintptr_t)heap+sizeof(*heap) && (uintptr_t)heap<n+28u) ||
           (n<(uintptr_t)needs+sizeof(*needs) && (uintptr_t)needs<n+28u))return battle_color_work_refuse(cpu,__LINE__);
        for(unsigned j=0;j<3;j++) {
            uintptr_t g=(uintptr_t)globals[j];size_t width=j<2u?sizeof(uintptr_t):4u;
            if(n<g+width && g<n+28u)return battle_color_work_refuse(cpu,__LINE__);
        }
        for(unsigned j=0;j<count;j++)if(n<(uintptr_t)nodes[j]+28u && (uintptr_t)nodes[j]<n+28u)return battle_color_work_refuse(cpu,__LINE__);
        nodes[count]=node;words[count]=current;memcpy(saved[count],node,28u);
        if(current==task)found=count;
        current=load_le(node+24u,4);count++;
    }
    if(found==16u || count!=values[2])return battle_color_work_refuse(cpu,__LINE__);
    if(!menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&after) ||
       memcmp(&before,&after,sizeof before))return battle_color_work_refuse(cpu,__LINE__);
    for(unsigned i=0;i<3;i++) {
        if(load_le(globals[i],4)!=values[i])return battle_color_work_refuse(cpu,__LINE__);
        if(i<2u && sizeof(uintptr_t)>4u && load_le(globals[i]+4u,4u))return battle_color_work_refuse(cpu,__LINE__);
    }
    for(unsigned i=0;i<count;i++)if(memcmp(nodes[i],saved[i],28u))return battle_color_work_refuse(cpu,__LINE__);

    uint32_t next=load_le(nodes[found]+24u,4),previous=found?words[found-1u]:0;
    uint8_t *link=found?nodes[found-1u]+24u:globals[0];
    runtime_note_store(rt,link,4);store_le(link,4,next);
    if(values[1]==task) {runtime_note_store(rt,globals[1],4);store_le(globals[1],4,next);}
    runtime_note_store(rt,globals[2],4);store_le(globals[2],4,values[2]-1u);
    cpu->gpr[2]=values[2]-1u;cpu->gpr[3]=task;cpu->gpr[5]=previous;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
