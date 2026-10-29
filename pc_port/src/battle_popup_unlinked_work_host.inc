/* Scoped callback on an already-unlinked heap work node.  No callbacks or
 * list writes occur; the retail count increment/decrement is preserved. */
static int battle_popup_unlinked_work_try_host(BattleMipsRuntime *rt,
                                               PcPortMipsCpu *cpu,
                                               uint32_t target)
{
    static const uint32_t addresses[] = {0x80059594u,0x80059590u,0x8005918cu};
    static const char *names[] = {"g_WorkList","D_80059590","g_NumWorkListEntries"};
    uint8_t *globals[3];
    uint32_t values[3];
    if (target!=0x8001cb48u || cpu->gpr[31]!=0x8001cf34u || cpu->gpr[28]!=0x80059170u ||
        rt->force_interpret || !rt->initialized || g_ActiveBattleRuntime!=rt || rt->bridge_cpu!=cpu ||
        (cpu->load_valid && cpu->load_reg!=0)) return 0;
#if defined(__linux__)
    if(!rt->owner_thread_recorded || !pthread_equal(rt->owner_thread,pthread_self()))return 0;
#else
    return 0;
#endif
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_UNLINK_CALLBACK_EXPERIMENT");
    if (!enabled || strcmp(enabled,"1")) return 0;
    uint32_t task=cpu->gpr[4];
    if (task<BATTLE_BUS_KUSEG_RAM_END || (task&3u) || task>UINT32_MAX-28u ||
        (task&0xff800000u)==0x80000000u || (task&0xff800000u)==0xa0000000u) return 0;

    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t off=XENO_RD_SLUS_OFF(target);
    uint8_t *code=resolve_memory(rt,target,0xa0u,0);
    if (!image || !image->bytes || image->size<off+0xa0u || !code || (uintptr_t)code>UINTPTR_MAX-0xa0u) return 0;
    if (memcmp(code,image->bytes+off,0xa0u)) {
        if (!PcPortMipsInterpreterDisabled()) return 0;
        for (unsigned i=0;i<0xa0u;i++) if(code[i]) return 0;
    }
    static const char sha[]="442d1deeeae343e5b0d5a373b425430fd9817b83727f5939e11554e96d226be9";
    uint8_t digest[32];file1_digest(image->bytes+off,0xa0u,digest);
    for (unsigned i=0;i<32;i++)
        if(file1_hex_digit(digest[i]>>4)!=sha[2*i] || file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return 0;

    for(unsigned i=0;i<3;i++) {
        ResolvedData *d=find_data(rt,addresses[i],4u);
        if(!d || d->address!=addresses[i] || d->size<4u || !d->host || !d->name || strcmp(d->name,names[i]))return 0;
        globals[i]=resolve_memory(rt,addresses[i],4u,0);
        if(globals[i]!=d->host || resolve_memory(rt,addresses[i],4u,1)!=globals[i])return 0;
        size_t width=i<2u?sizeof(uintptr_t):4u;
        if((uintptr_t)globals[i]>UINTPTR_MAX-width || !host_address_valid(rt,(uintptr_t)globals[i],width,1))return 0;
        if(i<2u && sizeof(uintptr_t)>4u &&
           (!host_address_valid(rt,(uintptr_t)globals[i],sizeof(uintptr_t),0) || load_le(globals[i]+4u,4u)))return 0;
        values[i]=load_le(globals[i],4);
    }
    for(unsigned i=0;i<3;i++)for(unsigned j=i+1;j<3;j++) {
        uintptr_t a=(uintptr_t)globals[i],b=(uintptr_t)globals[j];
        size_t aw=i<2u?sizeof(uintptr_t):4u,bw=j<2u?sizeof(uintptr_t):4u;
        if(a<b+bw && b<a+aw)return 0;
    }
    for(unsigned i=0;i<3;i++) {
        uintptr_t g=(uintptr_t)globals[i],c=(uintptr_t)code;size_t width=i<2u?sizeof(uintptr_t):4u;
        if(g<c+0xa0u && c<g+width)return 0;
    }
    if(values[2]==0 || values[2]>17u || values[1]==task)return 0;
    void **heap=dlsym(RTLD_DEFAULT,"g_Heap");uint32_t *needs=dlsym(RTLD_DEFAULT,"g_HeapNeedsConsolidation");
    MenuHeapSnapshot before,after;
    if(!heap || !needs || (uintptr_t)heap>UINTPTR_MAX-sizeof(*heap) || (uintptr_t)needs>UINTPTR_MAX-sizeof(*needs) ||
       !host_address_valid(rt,(uintptr_t)heap,sizeof(*heap),0) || !host_address_valid(rt,(uintptr_t)needs,sizeof(*needs),0))return 0;
    if((uintptr_t)heap<(uintptr_t)needs+sizeof(*needs) && (uintptr_t)needs<(uintptr_t)heap+sizeof(*heap))return 0;
    for(unsigned i=0;i<3;i++) {
        uintptr_t g=(uintptr_t)globals[i];size_t width=i<2u?sizeof(uintptr_t):4u;
        if((g<(uintptr_t)heap+sizeof(*heap) && (uintptr_t)heap<g+width) ||
           (g<(uintptr_t)needs+sizeof(*needs) && (uintptr_t)needs<g+width))return 0;
    }
    if(*needs || !menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&before))return 0;

    if(before.blocks[0].address<(uintptr_t)g_PsxRam+0x6faf0u ||
       before.blocks[before.count-1u].address!=(uintptr_t)g_PsxRam+0x1fbff8u)return 0;

    uint8_t *entry=resolve_memory(rt,task,28u,0);
    if(!entry || (uintptr_t)entry!=task || resolve_memory(rt,task,28u,1)!=entry ||
       load_le(entry+12u,4u)!=0x8001cb48u)return 0;
    int target_owned=0;
    for(unsigned j=0;j+1u<before.count;j++)if(((before.blocks[j].meta>>21)&15u)>1u) {
        uintptr_t begin=(uintptr_t)before.blocks[j].address+8u,limit=(uintptr_t)before.blocks[j].next-8u;
        if((uintptr_t)entry>=begin && (uintptr_t)entry+28u<=limit)target_owned=1;
    }
    if(!target_owned)return 0;
    uintptr_t e=(uintptr_t)entry,c=(uintptr_t)code;
    if((e<c+0xa0u && c<e+28u) ||
       (e<(uintptr_t)heap+sizeof(*heap) && (uintptr_t)heap<e+28u) ||
       (e<(uintptr_t)needs+sizeof(*needs) && (uintptr_t)needs<e+28u))return 0;
    for(unsigned j=0;j<3;j++) {
        uintptr_t g=(uintptr_t)globals[j];size_t width=j<2u?sizeof(uintptr_t):4u;
        if(e<g+width && g<e+28u)return 0;
    }
    uint8_t entry_saved[28];memcpy(entry_saved,entry,28u);
    uint8_t *nodes[16];uint8_t saved[16][28];uint32_t words[16];unsigned count=0;
    uint32_t current=values[0];
    while(current) {
        if(count>=16u || current<BATTLE_BUS_KUSEG_RAM_END || (current&3u) || current>UINT32_MAX-28u ||
           (current&0xff800000u)==0x80000000u || (current&0xff800000u)==0xa0000000u)return 0;
        for(unsigned j=0;j<count;j++)if(words[j]==current)return 0;
        uint8_t *node=resolve_memory(rt,current,28u,1);
        if(!node || resolve_memory(rt,current,28u,0)!=node || (uintptr_t)node!=current)return 0;
        int owned=0;
        for(unsigned j=0;j+1u<before.count;j++)if(((before.blocks[j].meta>>21)&15u)>1u) {
            uintptr_t begin=(uintptr_t)before.blocks[j].address+8u,limit=(uintptr_t)before.blocks[j].next-8u;
            if((uintptr_t)node>=begin && (uintptr_t)node+28u<=limit)owned=1;
        }
        if(!owned)return 0;
        uintptr_t n=(uintptr_t)node,c=(uintptr_t)code;
        if(n<(uintptr_t)entry+28u && (uintptr_t)entry<n+28u)return 0;
        if(n<c+0xa0u && c<n+28u)return 0;
        if((n<(uintptr_t)heap+sizeof(*heap) && (uintptr_t)heap<n+28u) ||
           (n<(uintptr_t)needs+sizeof(*needs) && (uintptr_t)needs<n+28u))return 0;
        for(unsigned j=0;j<3;j++) {
            uintptr_t g=(uintptr_t)globals[j];size_t width=j<2u?sizeof(uintptr_t):4u;
            if(n<g+width && g<n+28u)return 0;
        }
        for(unsigned j=0;j<count;j++)if(n<(uintptr_t)nodes[j]+28u && (uintptr_t)nodes[j]<n+28u)return 0;
        nodes[count]=node;words[count]=current;memcpy(saved[count],node,28u);
        if(current==task)return 0;
        current=load_le(node+24u,4);count++;
    }
    if(count+1u!=values[2])return 0;
    if(!menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,(uintptr_t)*heap,*needs,1u,&after) ||
       memcmp(&before,&after,sizeof before))return 0;
    for(unsigned i=0;i<3;i++) {
        if(load_le(globals[i],4)!=values[i])return 0;
        if(i<2u && sizeof(uintptr_t)>4u && load_le(globals[i]+4u,4u))return 0;
    }
    for(unsigned i=0;i<count;i++)if(memcmp(nodes[i],saved[i],28u))return 0;
    if(memcmp(entry,entry_saved,28u))return 0;

    runtime_note_store(rt,globals[2],4);store_le(globals[2],4,values[2]+1u);
    runtime_note_store(rt,globals[2],4);store_le(globals[2],4,values[2]);
    cpu->gpr[2]=values[2];cpu->gpr[3]=0;cpu->gpr[5]=count?words[count-1u]:0;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
