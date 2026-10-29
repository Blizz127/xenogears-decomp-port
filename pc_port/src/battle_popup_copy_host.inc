/* Scoped popup frame copy; retail advances a0/a1 and consumes a2. */
static int battle_popup_copy_try_host(BattleMipsRuntime *runtime,
                                      PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8003f968u || cpu->gpr[31]!=0x800bd1a0u || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0) || cpu->gpr[6]!=208u ||
       cpu->gpr[5]<BATTLE_BUS_KUSEG_RAM_END || (cpu->gpr[5]&3u) ||
       cpu->gpr[5]>UINT32_MAX-416u || cpu->gpr[4]!=cpu->gpr[5]+208u)return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_COPY_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1")!=0)return 0;
    uint8_t *in=resolve_memory(runtime,cpu->gpr[5],208u,0);
    uint8_t *out=resolve_memory(runtime,cpu->gpr[4],208u,1);
    const uint8_t *resident=resolve_memory(runtime,target,52u,0);
    const uint8_t *parent=resolve_memory(runtime,0x800bd098u,0x164u,0);
    const uint8_t *stack=resolve_memory(runtime,cpu->gpr[29],56u,1);
    const uint8_t *data=dlsym(RTLD_DEFAULT,"D_8001C76C");
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if(!in || !out || !resident || !parent || !stack || !data || !image || !image->bytes ||
       image->size<offset+52u || (uintptr_t)out!=(uintptr_t)in+208u)return 0;
    void **heap=dlsym(RTLD_DEFAULT,"g_Heap");
    uint32_t *needs=dlsym(RTLD_DEFAULT,"g_HeapNeedsConsolidation");
    MenuHeapSnapshot snapshot;
    if(!heap || !needs || !menu_heap_snapshot(g_PsxRam,BATTLE_BUS_KUSEG_RAM_END,
                                              (uintptr_t)*heap,*needs,1u,&snapshot))return 0;
    int owned=0;
    for(unsigned i=0;i<snapshot.count;i++) {
        uint32_t tag=(snapshot.blocks[i].meta>>21)&15u;
        if((uintptr_t)snapshot.blocks[i].address+8u==(uintptr_t)in && tag>1u &&
           (uintptr_t)snapshot.blocks[i].next >= (uintptr_t)in+424u) {owned=1;break;}
    }
    if(!owned)return 0;
    const uint8_t *code=image->bytes+offset;
    uintptr_t a=(uintptr_t)in;
    const uintptr_t protected_ranges[]={(uintptr_t)resident,(uintptr_t)parent,(uintptr_t)stack,(uintptr_t)data};
    const size_t protected_sizes[]={52u,0x164u,56u,0x170u};
    for(unsigned i=0;i<4;i++)
        if(a<protected_ranges[i]+protected_sizes[i] && protected_ranges[i]<a+416u)return 0;
    static const char expected[]="0ce68d87981de9301e11a186d5b7b97a838ff7b244145d7261b6ffab7cb185df";
    uint8_t digest[32];file1_digest(code,52u,digest);
    for(unsigned i=0;i<sizeof digest;i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t destination=cpu->gpr[4];
    memcpy(out,in,208u);
    cpu->gpr[2]=destination;cpu->gpr[3]=destination;
    cpu->gpr[4]+=208u;cpu->gpr[5]+=208u;cpu->gpr[6]=0;
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}
