/* Experimental SoundFindWdsEntry: finite native list search. */
static void battle_wds_lookup_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;r[3]=0x80060000u;r[3]=battle_entry_read(entry,r[3]+0xffff9558u,4);
    for(unsigned i=0;i<8;i++){if(!r[3])goto done;r[2]=battle_entry_read(entry,r[3]+32u,2);if(r[2]==r[4])goto done;r[3]=battle_entry_read(entry,r[3]+44u,4);}
    if(r[3])battle_entry_fail(entry,"WDS traversal limit",r[3]);
done:r[2]=r[3];
}

/* Restrict to a bounded, accessible list and the sound-service call site. */
static int battle_wds_lookup_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800383ecu || cpu->gpr[31]!=0x8003b6c8u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_WDS_LOOKUP_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+60u)return 0;
    static const char expected[]="d874c612b44d9874b11181b668f762a5b76f376a85529ee220cdf787e444ef93";
    uint8_t digest[32];file1_digest(image->bytes+offset,60u,digest);
    for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t node;if(runtime_read(runtime,0x80059558u,4,&node)!=0)return 0;int done=node==0;
    for(unsigned n=0;!done && n<8;n++){if(node<BATTLE_BUS_KUSEG_RAM_END || (node&3u) || node>UINT32_MAX-44u)return 0;
        uint32_t id;if(runtime_read(runtime,node+32u,2,&id)!=0)return 0;if(id==cpu->gpr[4]){done=1;break;}
        if(runtime_read(runtime,node+44u,4,&node)!=0)return 0;if(node==0)done=1;}
    if(!done)return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_wds_lookup_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
