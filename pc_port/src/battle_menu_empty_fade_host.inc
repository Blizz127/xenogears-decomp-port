/* Fully execute the fade branch when both primitive groups are empty. */
static int battle_menu_empty_fade_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu, uint32_t target)
{
    if (target != 0x8008d598u || cpu->gpr[31] != 0x80080758u || !(cpu->gpr[6] & 0xffu) ||
        runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime != runtime ||
        runtime->bridge_cpu != cpu || (cpu->load_valid && cpu->load_reg != 0)) return 0;
    const char *enabled = getenv("XENO_BATTLE_NATIVE_EMPTY_FADE_EXPERIMENT");
    if (!enabled || strcmp(enabled, "1") != 0) return 0;
    const uint32_t addr[] = {0x8008d598u, 0x8008dc00u, 0x8008d328u};
    const size_t size[] = {84u, 52u, 0x270u};
    const char *hash[] = {"721cf358c208576a33672068455c5f4df433d0c7759724b5adde6a7e5ad5d103",
                          "61514086c2d8b8fdadba183d3d3f0461d4fc1292e1fcd364633d87fa21597b7d",
                          "e4d026d93cfab31685205b6b89e7fd235653b6891f76fa2dbc3b15e51749ef48"};
    const uint8_t *protected_ptr[7]; size_t protected_size[7];
    for (unsigned i=0; i<3; ++i) {
        protected_ptr[i]=resolve_memory(runtime,addr[i],size[i],0); protected_size[i]=size[i];
        if (!protected_ptr[i]) return 0;
        uint8_t digest[32]; file1_digest(protected_ptr[i],size[i],digest);
        for (unsigned j=0; j<32; ++j)
            if (file1_hex_digit(digest[j]>>4)!=hash[i][j*2] || file1_hex_digit(digest[j]&15u)!=hash[i][j*2+1]) return 0;
    }
    uint32_t sp=cpu->gpr[29]; if ((sp&3u) || sp<BATTLE_BUS_KUSEG_RAM_END+176u) return 0;
    protected_ptr[3]=resolve_memory(runtime,0x800d2d28u,4u,0); protected_size[3]=4u;
    if (!protected_ptr[3]) return 0;
    uint32_t state=load_le(protected_ptr[3],4);
    if ((state&3u) || state>UINT32_MAX-0xd7u) return 0;
    protected_ptr[4]=resolve_memory(runtime,state+0xd0u,8u,0); protected_size[4]=8u;
    protected_ptr[5]=resolve_memory(runtime,state+0xa3u,1u,0); protected_size[5]=1u;
    uint8_t *out=resolve_memory(runtime,state+0xcbu,1u,1);
    if (!protected_ptr[4] || !protected_ptr[5] || !out || load_le(protected_ptr[4],4) || load_le(protected_ptr[4]+4,4)) return 0;
    protected_ptr[6]=out; protected_size[6]=1u;
    for (unsigned i=0; i<6; ++i)
        if ((uintptr_t)out < (uintptr_t)protected_ptr[i]+protected_size[i] && (uintptr_t)protected_ptr[i] < (uintptr_t)out+1u) return 0;
    uint8_t *slots[19];
    for (unsigned i=0; i<19; ++i) {
        uint32_t offset=i<10 ? 4u+i*4u : 112u+(uint32_t[]){8,12,16,20,24,28,32,36,40}[i-10];
        slots[i]=resolve_memory(runtime,sp-offset,4u,1); if (!slots[i]) return 0;
        for (unsigned j=0; j<7; ++j)
            if ((uintptr_t)slots[i] < (uintptr_t)protected_ptr[j]+protected_size[j] && (uintptr_t)protected_ptr[j] < (uintptr_t)slots[i]+4u) return 0;
        for (unsigned j=0; j<i; ++j)
            if ((uintptr_t)slots[i] < (uintptr_t)slots[j]+4u && (uintptr_t)slots[j] < (uintptr_t)slots[i]+4u) return 0;
    }
    const unsigned parent_regs[]={31,30,23,22,21,20,19,18,17,16};
    const unsigned child_regs[]={31,23,22,21,20,19,18,17,16};
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu}; if (setjmp(entry.failure)!=0) return -1;
    for (unsigned i=0; i<10; ++i) battle_entry_write(&entry,sp-4u-i*4u,4,cpu->gpr[parent_regs[i]]);
    for (unsigned i=0; i<9; ++i)
        battle_entry_write(&entry,sp-112u-(uint32_t[]){8,12,16,20,24,28,32,36,40}[i],4,
                           i==0 ? 0x8008d5d4u : cpu->gpr[child_regs[i]]);
    battle_entry_write(&entry,state+0xcbu,1,0); cpu->gpr[6]&=0xffu; cpu->gpr[4]=state; cpu->gpr[2]=*protected_ptr[5];
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
}
