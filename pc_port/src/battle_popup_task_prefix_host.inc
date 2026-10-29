/* Experimental 800BCC60 path with no existing popup task and a live owner.
 * Timer allocation and the parent continuation are still guarded. */
static void battle_popup_task_prefix_run(BattleEntryCall *entry,
                                         uint32_t expected_slot,
                                         uint32_t expected_owner)
{
    uint32_t *r=entry->cpu->gpr;
    r[29]-=32u;battle_entry_write(entry,r[29]+24u,4,r[18]);
    r[18]=0x800c0000u;r[4]=battle_entry_read(entry,r[18]+0x3748u,4);
    battle_entry_write(entry,r[29]+28u,4,r[31]);
    battle_entry_write(entry,r[29]+20u,4,r[17]);
    battle_entry_write(entry,r[29]+16u,4,r[16]);
    if(r[4])battle_entry_fail(entry,"popup task appeared",r[4]);
    r[2]=0x800c0000u;r[17]=r[2]+0x3eb0u;
    r[2]=battle_entry_read(entry,r[17]+0xa72u,1);r[3]=0x8cb8u;
    if(r[2]!=expected_slot)battle_entry_fail(entry,"popup slot changed",r[2]);
    r[2]<<=2;r[2]+=r[17];r[2]+=r[3];r[4]=battle_entry_read(entry,r[2],4);
    if(r[4]!=expected_owner || !r[4])battle_entry_fail(entry,"popup owner changed",r[4]);
    r[5]=12u;battle_phase_call(entry,0x8001cd08u,0x800bccccu);
    battle_entry_fail(entry,"popup task allocator returned without continuation",0x8001cd08u);
}

static int battle_popup_task_prefix_try_host(BattleMipsRuntime *runtime,
                                              PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800bcc60u || cpu->gpr[31]!=0x800bcdd4u ||
       runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_TASK_PREFIX_EXPERIMENT");
    if(!enabled || strcmp(enabled,"1")!=0)return 0;
    const uint8_t *code=resolve_memory(runtime,target,0x12cu,0);if(!code)return 0;
    static const char expected[]="3fde24aafcfd227a0e3f3a48ec4fcce59debd01b973204f117fa7c2fac2f8ecc";
    uint8_t digest[32];file1_digest(code,0x12cu,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
           file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t sp=cpu->gpr[29],task,slot,owner;
    if((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+32u ||
       runtime_read(runtime,0x800c3748u,4,&task)!=0 || task ||
       runtime_read(runtime,0x800c4922u,1,&slot)!=0 || slot>=11u ||
       runtime_read(runtime,0x800ccb68u+slot*4u,4,&owner)!=0 || !owner)return 0;
    uint8_t *inputs[]={resolve_memory(runtime,0x800c3748u,4u,0),
        resolve_memory(runtime,0x800c4922u,1u,0),
        resolve_memory(runtime,0x800ccb68u+slot*4u,4u,0)};
    const unsigned widths[]={4,1,4},offsets[]={16,20,24,28};
    for(unsigned j=0;j<3;j++)if(!inputs[j])return 0;
    for(unsigned i=0;i<4;i++) {
        uint8_t *saved=resolve_memory(runtime,sp-32u+offsets[i],4u,1);
        if(!saved)return 0;
        uintptr_t a=(uintptr_t)saved,b=(uintptr_t)code;
        if(a<b+0x12cu && b<a+4u)return 0;
        for(unsigned j=0;j<3;j++) {
            uintptr_t c=(uintptr_t)inputs[j];
            if(a<c+widths[j] && c<a+4u)return 0;
        }
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_popup_task_prefix_run(&entry,slot,owner);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
