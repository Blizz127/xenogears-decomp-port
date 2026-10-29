/* Experimental 3B644: matching SED prefix through the next guarded service. */
/* Refuse a pair outside the sound manager before the prefix changes state.
 * This checks the element span and stack alias only; it does not replace the
 * still-required event, callback, bank and channel preflight. */
static int battle_sound_elements_preflight(BattleMipsRuntime *runtime,
                                           uint32_t manager, uint32_t spec,
                                           uint32_t sp)
{
    uint32_t elements, count, index = spec & 0xffu;
    if ((manager & 3u) || manager < BATTLE_BUS_KUSEG_RAM_END ||
        manager > UINT32_MAX - 0x20d4u || (sp & 3u) ||
        sp < BATTLE_BUS_KUSEG_RAM_END + 80u ||
        runtime_read(runtime, manager + 0x14u, 1, &elements) != 0 ||
        runtime_read(runtime, 0x80059404u, 4, &count) != 0 ||
        count != 2u || elements > 24u || index + count > elements)
        return 0;
    uint32_t offset = 0x94u + index * 0x158u;
    uint32_t length = count * 0x158u;
    uint32_t segment = manager & 0xff800000u;
    if ((segment == 0x80000000u || segment == 0xa0000000u) &&
        (uint64_t)(manager & 0x1fffffu) + offset + length > BATTLE_BUS_KUSEG_RAM_END)
        return 0;
    uint8_t *header = resolve_memory(runtime, manager, 0x94u, 0);
    uint8_t *read_span = resolve_memory(runtime, manager + offset, length, 0);
    uint8_t *write_span = resolve_memory(runtime, manager + offset, length, 1);
    uint8_t *frame = resolve_memory(runtime, sp - 80u, 80u, 1);
    if (!header || !read_span || !write_span || !frame ||
        read_span != write_span || (uintptr_t)read_span != (uintptr_t)header + offset)
        return 0;
    uintptr_t a = (uintptr_t)write_span, b = (uintptr_t)frame;
    return !(a < b + 80u && b < a + length);
}

static void battle_sound_prefix_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[29]-=80u;battle_entry_write(entry,r[29]+48u,4,r[18]);r[18]=0x80060000u;
    uint32_t list=battle_entry_read(entry,r[18]+0xffff9440u,4);battle_entry_write(entry,r[29]+72u,4,r[30]);r[18]=list;r[30]=r[6];
    battle_entry_write(entry,r[29]+40u,4,r[16]);battle_entry_write(entry,r[29]+16u,4,r[5]);r[3]=(uint32_t)((int32_t)r[5]>>16);
    battle_entry_write(entry,r[29]+76u,4,r[31]);battle_entry_write(entry,r[29]+68u,4,r[23]);battle_entry_write(entry,r[29]+64u,4,r[22]);battle_entry_write(entry,r[29]+60u,4,r[21]);battle_entry_write(entry,r[29]+56u,4,r[20]);battle_entry_write(entry,r[29]+52u,4,r[19]);battle_entry_write(entry,r[29]+44u,4,r[17]);battle_entry_write(entry,r[29]+24u,2,r[7]);
    uint32_t id=battle_entry_read(entry,r[18]+20u,2);r[20]=0x80060000u;uint32_t manager=battle_entry_read(entry,r[20]+0xffff95d8u,4);r[2]=id;r[16]=r[4];r[20]=manager;
    if(r[2]==r[3])goto matched;
    for(unsigned i=0;i<8;i++){uint32_t next=battle_entry_read(entry,r[18]+28u,4);r[18]=next;if(!r[18])battle_entry_fail(entry,"no matching SED",0);r[2]=battle_entry_read(entry,r[18]+20u,2);if(r[2]==r[3])goto matched;}
    battle_entry_fail(entry,"SED traversal limit",r[18]);
matched:
    r[4]=battle_entry_read(entry,r[18]+22u,2);battle_phase_call(entry,0x800383ecu,0x8003b6c0u);
    r[23]=r[2];uint32_t bank=r[23];r[3]=r[30]<<16;
    uint32_t fallback=0;if(!bank){r[23]=0x80060000u;fallback=battle_entry_read(entry,r[23]+0xffff9558u,4);}
    uint32_t off=battle_entry_read(entry,r[18]+24u,2);if(!bank)r[23]=fallback;uint32_t packed=battle_entry_read(entry,r[29]+16u,4);r[2]=off;
    r[2]=r[18]+r[2];r[8]=packed;r[4]=r[8]&0xffffu;r[2]+=r[4];uint32_t level=battle_entry_read(entry,r[2],1);r[3]=(uint32_t)((int32_t)r[3]>>16);r[2]=level;
    int64_t product=(int64_t)(int32_t)r[3]*(int64_t)(int32_t)r[2];entry->cpu->lo=(uint32_t)product;entry->cpu->hi=(uint32_t)((uint64_t)product>>32);
    r[2]=entry->cpu->lo;r[30]=r[2]>>7;r[2]=(r[30]>>15)&1u;r[8]=r[16]>>8;if(r[2])r[30]=0x7fffu;
    r[2]=(r[4]<<2)+32u;r[21]=r[18]+r[2];r[3]=r[16]&0xffu;r[2]=r[3]<<1;r[2]+=r[3];r[2]<<=2;r[2]-=r[3];r[2]<<=2;r[2]-=r[3];r[2]<<=3;r[2]+=148u;r[17]=r[20]+r[2];r[16]=r[17]+39u;
    r[4]=0x80060000u;uint32_t event=battle_entry_read(entry,r[4]+0xffff95bcu,4);r[22]=0x80060000u;r[4]=event;uint32_t count=battle_entry_read(entry,r[22]+0xffff9404u,4);r[19]=r[17]+48u;r[22]=count;
    if (r[29] > UINT32_MAX - 80u ||
        !battle_sound_elements_preflight(entry->runtime,r[20],r[3],r[29]+80u))
        battle_entry_fail(entry,"sound element span changed",r[17]);
    r[31]=0x8003b76cu;battle_entry_write(entry,r[29]+32u,1,r[8]);battle_phase_call(entry,0x800404b4u,0x8003b764u);
    battle_entry_fail(entry,"audio event service returned without native continuation",0x800404b4u);
}

/* Bounded matching SED and volume inputs; event service remains guarded. */
static int battle_sound_prefix_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8003b644u || cpu->gpr[31]!=0x80039e08u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_SOUND_PREFIX_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+0x2ecu)return 0;
    static const char expected[]="7434935d43c77794781ea582813b8d7a83f92780d84657484e27abb17a02f901";
    uint8_t digest[32];file1_digest(image->bytes+offset,0x2ecu,digest);
    for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t sp=cpu->gpr[29];if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+80u)return 0;
    const unsigned offsets[]={16,24,40,44,48,52,56,60,64,68,72,76,32};uint8_t *saved[13];unsigned widths[13];
    for(unsigned i=0;i<13;i++){widths[i]=i==1?2:i==12?1:4;saved[i]=resolve_memory(runtime,sp-80u+offsets[i],widths[i],1);if(!saved[i])return 0;if(i>=2 && i<12 && !resolve_memory(runtime,sp-80u+offsets[i],4,0))return 0;}
    uint8_t *code=resolve_memory(runtime,target,0x2ecu,0);if(!code)return 0;
    for(unsigned i=0;i<13;i++){uintptr_t a=(uintptr_t)saved[i],b=(uintptr_t)code;if(a<b+0x2ecu && b<a+widths[i])return 0;}
    uint8_t *globals[]={resolve_memory(runtime,0x80059440u,4,0),resolve_memory(runtime,0x800595d8u,4,0)};
    for(unsigned j=0;j<2;j++){if(!globals[j])return 0;for(unsigned i=0;i<13;i++){uintptr_t a=(uintptr_t)saved[i],b=(uintptr_t)globals[j];if(a<b+4u && b<a+widths[i])return 0;}}
    uint32_t node,manager;if(runtime_read(runtime,0x80059440u,4,&node)!=0 || runtime_read(runtime,0x800595d8u,4,&manager)!=0 || !battle_sound_elements_preflight(runtime,manager,cpu->gpr[4],sp))return 0;
    uint32_t wanted=(uint32_t)((int32_t)cpu->gpr[5]>>16);int found=0;uint32_t found_node=0;
    for(unsigned n=0;n<8;n++) {if(node<BATTLE_BUS_KUSEG_RAM_END || (node&3u) || node>UINT32_MAX-28u)return 0;
        uint8_t *id=resolve_memory(runtime,node+20u,2,0),*next=resolve_memory(runtime,node+28u,4,0);if(!id || !next)return 0;
        for(unsigned i=0;i<13;i++){uintptr_t a=(uintptr_t)saved[i],b=(uintptr_t)id,c=(uintptr_t)next;if((a<b+2u && b<a+widths[i]) || (a<c+4u && c<a+widths[i]))return 0;}
        uint32_t value;if(runtime_read(runtime,node+20u,2,&value)!=0)return 0;
        if(value==wanted){uint8_t *bank=resolve_memory(runtime,node+22u,2,0);if(!bank)return 0;for(unsigned i=0;i<13;i++){uintptr_t a=(uintptr_t)saved[i],b=(uintptr_t)bank;if(a<b+2u && b<a+widths[i])return 0;}found=1;found_node=node;break;}
        if(runtime_read(runtime,node+28u,4,&node)!=0 || node==0)return 0;}
    if(!found)return 0;
    uint32_t volume_offset;if(runtime_read(runtime,found_node+24u,2,&volume_offset)!=0 || found_node>UINT32_MAX-volume_offset || found_node+volume_offset>UINT32_MAX-(cpu->gpr[5]&0xffffu))return 0;
    uint8_t *offset_ptr=resolve_memory(runtime,found_node+24u,2,0),*volume=resolve_memory(runtime,found_node+volume_offset+(cpu->gpr[5]&0xffffu),1,0);
    if(!offset_ptr || !volume || !resolve_memory(runtime,0x80059558u,4,0) || !resolve_memory(runtime,0x800595bcu,4,0) || !resolve_memory(runtime,0x80059404u,4,0))return 0;
    for(unsigned i=0;i<12;i++){uintptr_t a=(uintptr_t)saved[i],b=(uintptr_t)offset_ptr,c=(uintptr_t)volume;if((a<b+2u && b<a+widths[i]) || (a<c+1u && c<a+widths[i]))return 0;}
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_sound_prefix_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
