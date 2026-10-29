/* Experimental sound wrapper and bank-ID child; sound service remains guarded. */
static void battle_sound_bank_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    r[2]=0x80060000u;
    uint32_t bank=battle_entry_read(entry,r[2]+0xffff919cu,4);
    r[29]-=0x18u; r[2]=bank;
    battle_entry_write(entry,r[29]+0x10u,4,r[31]);
    uint32_t id=battle_entry_read(entry,r[2]+0x14u,2);
    r[4]&=0xffu; r[2]=id;
    r[2]<<=16; r[4]|=r[2];
    battle_phase_call(entry,0x80039db8u,0x8008aa5cu);
    uint32_t ra=battle_entry_read(entry,r[29]+0x10u,4);
    r[29]+=0x18u; r[31]=ra;
}
static void battle_sound_wrapper_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    r[2]=0x800d0000u;
    uint32_t enabled=battle_entry_read(entry,r[2]+0x366cu,1);
    r[29]-=0x18u; r[2]=enabled;
    battle_entry_write(entry,r[29]+0x10u,4,r[31]);
    if (enabled) { r[4]&=0xffu; battle_phase_call(entry,0x8008aa40u,0x8008aa88u); }
    uint32_t ra=battle_entry_read(entry,r[29]+0x10u,4);
    r[29]+=0x18u; r[31]=ra;
}

static int battle_menu_sound_try_host(BattleMipsRuntime *runtime,
                                      PcPortMipsCpu *cpu, uint32_t target)
{
    if (runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime != runtime || runtime->bridge_cpu != cpu ||
        (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *control = getenv("XENO_BATTLE_NATIVE_MENU_SOUND_EXPERIMENT");
    if (!control || strcmp(control, "1") != 0) return 0;
    unsigned length; const char *expected;
    if (target == 0x8008aa74u && cpu->gpr[31] == 0x800802bcu) {
        length=0x2cu; expected="5c775a9f503a956c6cadb245f130f3db2ec6275c77888068f84bfdce8e4422a4";
    } else if (target == 0x8008aa40u && cpu->gpr[31] == 0x8008aa90u) {
        length=0x34u; expected="7d06bab042e53b54bb618495ee74f452a61796de82810b15c1cbdb3652900388";
    } else return 0;
    uint8_t digest[32]; file1_digest(PSX_ADDR(target),length,digest);
    for (unsigned i=0;i<sizeof(digest);++i)
        if (file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
            file1_hex_digit(digest[i]&15u)!=expected[2*i+1]) return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if (setjmp(entry.failure)!=0) return -1;
    if (target==0x8008aa74u) battle_sound_wrapper_run(&entry);
    else battle_sound_bank_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
}
