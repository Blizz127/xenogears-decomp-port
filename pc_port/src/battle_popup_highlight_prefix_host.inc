/* Experimental 800BCD98 prefix: latch the mask and reach the guarded child.
 * The task traversal and popup services are not converted here. */
/* Continue the owner loop only across empty slots and lookup misses. Calls
 * that create a task or invoke its cleanup callback remain guarded. */
static void battle_popup_owner_miss_loop(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    if(r[19]!=0) battle_entry_fail(entry,"unsupported popup loop position",r[19]);
    r[4]=r[2];
    if(r[4]) battle_entry_fail(entry,"popup cleanup callback not converted",r[4]);
    for (;;) {
        r[17]+=4u;r[19]++;r[2]=r[18]&0xffffu;r[18]=r[2]>>1;r[2]=11u;
        int more=(r[19]!=r[2]);r[2]=r[18]&1u;
        if(!more) {
            r[31]=battle_entry_read(entry,r[29]+40u,4);
            r[21]=battle_entry_read(entry,r[29]+36u,4);
            r[20]=battle_entry_read(entry,r[29]+32u,4);
            r[19]=battle_entry_read(entry,r[29]+28u,4);
            r[18]=battle_entry_read(entry,r[29]+24u,4);
            r[17]=battle_entry_read(entry,r[29]+20u,4);
            r[16]=battle_entry_read(entry,r[29]+16u,4);
            r[29]+=48u;return;
        }
        int set=(r[2]!=0);r[2]=r[17]+r[21];
        r[16]=battle_entry_read(entry,r[2],4);r[4]=r[16];
        if(!r[16])continue;
        r[5]=r[20]+0xffffcfacu;
        battle_phase_call(entry,0x8001d0a4u,set?0x800bce14u:0x800bce44u);
        if(set) {
            if(!r[2]) {
                const char *create=getenv("XENO_BATTLE_NATIVE_POPUP_CREATE_PREFIX_EXPERIMENT");
                if(create && strcmp(create,"1")==0) {
                    r[4]=r[16];battle_phase_call(entry,0x800bd098u,0x800bce24u);
                }
                battle_entry_fail(entry,"popup task creation not converted",r[16]);
            }
        } else {
            r[4]=r[2];
            if(r[4]) battle_entry_fail(entry,"popup cleanup callback not converted",r[4]);
        }
    }
}

/* After nonzero setup: only the first clear-bit, nonempty-owner lookup.
 * The lookup and the remaining owner loop are intentionally refused. */
static void battle_popup_owner_prefix_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr,owner;
    if ((r[18]&1u) || runtime_read(entry->runtime,0x800ccb68u,4,&owner)!=0 || !owner)
        battle_entry_fail(entry,"unsupported first popup owner",r[18]);
    r[21]=0x8cb8u;r[20]=0x800c0000u;r[2]=0x800c0000u;
    r[17]=r[2]+0x3eb0u;r[2]=r[18]&1u;r[2]=r[17]+r[21];
    r[16]=battle_entry_read(entry,r[2],4);r[4]=r[16];
    if (r[16]!=owner)battle_entry_fail(entry,"popup owner changed",r[16]);
    r[5]=r[20]+0xffffcfacu;
    battle_phase_call(entry,0x8001d0a4u,0x800bce44u);
    const char *loop=getenv("XENO_BATTLE_NATIVE_POPUP_OWNER_LOOP_EXPERIMENT");
    if(loop && strcmp(loop,"1")==0) { battle_popup_owner_miss_loop(entry);return; }
    battle_entry_fail(entry,"popup owner lookup returned without continuation",0x8001d0a4u);
}

static void battle_popup_highlight_prefix_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    r[29]-=48u;
    battle_entry_write(entry,r[29]+24u,4,r[18]);
    r[18]=r[4];r[2]=0x800c0000u;
    battle_entry_write(entry,r[2]+0x3d14u,2,r[4]);
    r[2]=r[18]&0xffffu;
    battle_entry_write(entry,r[29]+40u,4,r[31]);
    battle_entry_write(entry,r[29]+36u,4,r[21]);
    battle_entry_write(entry,r[29]+32u,4,r[20]);
    battle_entry_write(entry,r[29]+28u,4,r[19]);
    battle_entry_write(entry,r[29]+20u,4,r[17]);
    battle_entry_write(entry,r[29]+16u,4,r[16]);
    if (r[2]) {
        r[19]=0;
        battle_phase_call(entry,0x800bcc60u,0x800bcdccu);
        const char *loop=getenv("XENO_BATTLE_NATIVE_POPUP_OWNER_PREFIX_EXPERIMENT");
        if (loop && strcmp(loop,"1")==0) { battle_popup_owner_prefix_run(entry);return; }
        battle_entry_fail(entry,"highlight setup returned without continuation",0x800bcc60u);
    }
    r[4]=0;
    battle_phase_call(entry,0x800bcb54u,0x800bcddcu);
    battle_entry_fail(entry,"highlight clear returned without continuation",0x800bcb54u);
}

static int battle_popup_highlight_prefix_try_host(BattleMipsRuntime *runtime,
                                                   PcPortMipsCpu *cpu,
                                                   uint32_t target)
{
    if (target!=0x800bcd98u || cpu->gpr[31]!=0x8008067cu ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
        (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_HIGHLIGHT_PREFIX_EXPERIMENT");
    if (!enabled || strcmp(enabled,"1")!=0) return 0;
    const uint8_t *code=resolve_memory(runtime,target,0x114u,0);
    if (!code) return 0;
    static const char expected[]="0b52283ff5977bcaefc90fec4303330c4bfc349efb1dd4b017becca9c895bfc0";
    uint8_t digest[32];file1_digest(code,0x114u,digest);
    for (unsigned i=0;i<sizeof(digest);i++)
        if (file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
            file1_hex_digit(digest[i]&15u)!=expected[2*i+1]) return 0;
    uint32_t sp=cpu->gpr[29];
    if ((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+48u) return 0;
    uint8_t *mask=resolve_memory(runtime,0x800c3d14u,2u,1);
    if (!mask) return 0;
    const unsigned offsets[]={16,20,24,28,32,36,40};
    for (unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++) {
        uint8_t *saved=resolve_memory(runtime,sp-48u+offsets[i],4u,1);
        if (!saved) return 0;
        uintptr_t a=(uintptr_t)saved,b=(uintptr_t)code,c=(uintptr_t)mask;
        if ((a<b+0x114u && b<a+4u) || (a<c+2u && c<a+4u)) return 0;
    }
    uintptr_t a=(uintptr_t)mask,b=(uintptr_t)code;
    if (a<b+0x114u && b<a+2u) return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if (setjmp(entry.failure)!=0) return -1;
    battle_popup_highlight_prefix_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);
    return 1;
}
