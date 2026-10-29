/* Complete animation playback stream initializer. Pointer fields remain
 * guest words; input/output aliases and faults retain their ordered effects.
 * No child calls, stack accesses or CPU instruction execution are needed. */
static void battle_animation_stream_run(BattleEntryCall *entry)
{
    PcPortMipsCpu *cpu=entry->cpu;
    uint32_t actor=cpu->gpr[4],animation=cpu->gpr[5];

    /* Every load has a completed nonfaulting delay before the next access.
     * Keep V0 committed at each boundary, including failed later writes. */
    cpu->gpr[2]=battle_entry_read(entry,animation+0x12u,2);
    if(cpu->gpr[2]==0) {
        cpu->gpr[2]=UINT32_MAX;
        battle_entry_write(entry,actor+0x98u,2,cpu->gpr[2]);
        return;
    }

    battle_entry_write(entry,actor+0x98u,2,0);
    if(cpu->gpr[6]!=0)
        cpu->gpr[2]=battle_entry_read(entry,animation+2u,2);
    else
        cpu->gpr[2]=UINT32_MAX;
    battle_entry_write(entry,actor+0x9au,2,cpu->gpr[2]);
    battle_entry_write(entry,actor+0x9cu,2,0);

    /* Earlier actor writes may change both this header and the offset.
     * The second header value is copied even when an alias made it zero. */
    cpu->gpr[2]=battle_entry_read(entry,animation+0x12u,2);
    battle_entry_write(entry,actor+0x9eu,2,cpu->gpr[2]);
    cpu->gpr[2]=battle_entry_read(entry,animation+0x14u,4);
    cpu->gpr[2]+=animation;
    battle_entry_write(entry,actor+0xa0u,4,cpu->gpr[2]);
    battle_entry_write(entry,actor+0xa4u,4,cpu->gpr[2]);
}

static int battle_animation_stream_try_host(BattleMipsRuntime *runtime,
                                            PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800ae1bcu||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ANIMATION_STREAM");
    if(control&&strcmp(control,"0")==0)return 0;
    const char *expected=
        "1c4e52b31bceb27241444be3a73a03f2b3cd8da6391c72904f881383e32c689e";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),100u,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||
           file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_animation_stream_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
