/* Experimental ScaleMatrixL guest register/HI/LO body. */
static void battle_scale_matrix_run(BattleEntryCall *entry) {
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[4], 4); /* 8004974C */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[5], 4); /* 80049750 */
        entry->cpu->gpr[8]=delayed_0;
        entry->cpu->gpr[11]=delayed_1;
    }
    entry->cpu->gpr[9] = entry->cpu->gpr[8] & 0xFFFFu; /* 80049754 andi $t1, $t0, 0xFFFF */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] << 16; /* 80049758 sll $t1, $t1, 16 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 16); /* 8004975C sra $t1, $t1, 16 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[11]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 80049760 multu $t1, $t3 */
    entry->cpu->gpr[10] = (uint32_t)((int32_t)entry->cpu->gpr[8] >> 16); /* 80049764 sra $t2, $t0, 16 */
    {
        uint32_t delayed_0=battle_entry_read(entry, entry->cpu->gpr[5] + 0x4u, 4); /* 80049768 */
        uint32_t delayed_1=battle_entry_read(entry, entry->cpu->gpr[5] + 0x8u, 4); /* 8004976C */
        entry->cpu->gpr[12]=delayed_0;
        uint32_t delayed_2=battle_entry_read(entry, entry->cpu->gpr[4] + 0x4u, 4); /* 80049770 */
        entry->cpu->gpr[13]=delayed_1;
        entry->cpu->gpr[8]=delayed_2;
    }
    entry->cpu->gpr[2] = entry->cpu->gpr[4] + 0u; /* 80049774 addu $v0, $a0, $zero */
    entry->cpu->gpr[9] = entry->cpu->lo; /* 80049778 mflo $t1 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 12); /* 8004977C sra $t1, $t1, 12 */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] & 0xFFFFu; /* 80049780 andi $t1, $t1, 0xFFFF */
    { uint64_t product=(uint64_t)entry->cpu->gpr[10] * entry->cpu->gpr[11]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 80049784 multu $t2, $t3 */
    entry->cpu->gpr[10] = entry->cpu->lo; /* 80049788 mflo $t2 */
    entry->cpu->gpr[10] = (uint32_t)((int32_t)entry->cpu->gpr[10] >> 12); /* 8004978C sra $t2, $t2, 12 */
    entry->cpu->gpr[10] = entry->cpu->gpr[10] << 16; /* 80049790 sll $t2, $t2, 16 */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] | entry->cpu->gpr[10]; /* 80049794 or $t1, $t1, $t2 */
    battle_entry_write(entry, entry->cpu->gpr[4], 4, entry->cpu->gpr[9]); /* 80049798 sw $t1, 0x0($a0) */
    entry->cpu->gpr[9] = entry->cpu->gpr[8] & 0xFFFFu; /* 8004979C andi $t1, $t0, 0xFFFF */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] << 16; /* 800497A0 sll $t1, $t1, 16 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 16); /* 800497A4 sra $t1, $t1, 16 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[11]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 800497A8 multu $t1, $t3 */
    entry->cpu->gpr[10] = (uint32_t)((int32_t)entry->cpu->gpr[8] >> 16); /* 800497AC sra $t2, $t0, 16 */
    entry->cpu->gpr[8] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x8u, 4); /* 800497B0 lw $t0, 0x8($a0) */
    entry->cpu->gpr[9] = entry->cpu->lo; /* 800497B4 mflo $t1 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 12); /* 800497B8 sra $t1, $t1, 12 */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] & 0xFFFFu; /* 800497BC andi $t1, $t1, 0xFFFF */
    { uint64_t product=(uint64_t)entry->cpu->gpr[10] * entry->cpu->gpr[12]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 800497C0 multu $t2, $t4 */
    entry->cpu->gpr[10] = entry->cpu->lo; /* 800497C4 mflo $t2 */
    entry->cpu->gpr[10] = (uint32_t)((int32_t)entry->cpu->gpr[10] >> 12); /* 800497C8 sra $t2, $t2, 12 */
    entry->cpu->gpr[10] = entry->cpu->gpr[10] << 16; /* 800497CC sll $t2, $t2, 16 */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] | entry->cpu->gpr[10]; /* 800497D0 or $t1, $t1, $t2 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x4u, 4, entry->cpu->gpr[9]); /* 800497D4 sw $t1, 0x4($a0) */
    entry->cpu->gpr[9] = entry->cpu->gpr[8] & 0xFFFFu; /* 800497D8 andi $t1, $t0, 0xFFFF */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] << 16; /* 800497DC sll $t1, $t1, 16 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 16); /* 800497E0 sra $t1, $t1, 16 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[12]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 800497E4 multu $t1, $t4 */
    entry->cpu->gpr[10] = (uint32_t)((int32_t)entry->cpu->gpr[8] >> 16); /* 800497E8 sra $t2, $t0, 16 */
    entry->cpu->gpr[8] = battle_entry_read(entry, entry->cpu->gpr[4] + 0xCu, 4); /* 800497EC lw $t0, 0xC($a0) */
    entry->cpu->gpr[9] = entry->cpu->lo; /* 800497F0 mflo $t1 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 12); /* 800497F4 sra $t1, $t1, 12 */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] & 0xFFFFu; /* 800497F8 andi $t1, $t1, 0xFFFF */
    { uint64_t product=(uint64_t)entry->cpu->gpr[10] * entry->cpu->gpr[12]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 800497FC multu $t2, $t4 */
    entry->cpu->gpr[10] = entry->cpu->lo; /* 80049800 mflo $t2 */
    entry->cpu->gpr[10] = (uint32_t)((int32_t)entry->cpu->gpr[10] >> 12); /* 80049804 sra $t2, $t2, 12 */
    entry->cpu->gpr[10] = entry->cpu->gpr[10] << 16; /* 80049808 sll $t2, $t2, 16 */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] | entry->cpu->gpr[10]; /* 8004980C or $t1, $t1, $t2 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x8u, 4, entry->cpu->gpr[9]); /* 80049810 sw $t1, 0x8($a0) */
    entry->cpu->gpr[9] = entry->cpu->gpr[8] & 0xFFFFu; /* 80049814 andi $t1, $t0, 0xFFFF */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] << 16; /* 80049818 sll $t1, $t1, 16 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 16); /* 8004981C sra $t1, $t1, 16 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[13]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 80049820 multu $t1, $t5 */
    entry->cpu->gpr[10] = (uint32_t)((int32_t)entry->cpu->gpr[8] >> 16); /* 80049824 sra $t2, $t0, 16 */
    entry->cpu->gpr[8] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x10u, 4); /* 80049828 lw $t0, 0x10($a0) */
    entry->cpu->gpr[9] = entry->cpu->lo; /* 8004982C mflo $t1 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 12); /* 80049830 sra $t1, $t1, 12 */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] & 0xFFFFu; /* 80049834 andi $t1, $t1, 0xFFFF */
    { uint64_t product=(uint64_t)entry->cpu->gpr[10] * entry->cpu->gpr[13]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 80049838 multu $t2, $t5 */
    entry->cpu->gpr[10] = entry->cpu->lo; /* 8004983C mflo $t2 */
    entry->cpu->gpr[10] = (uint32_t)((int32_t)entry->cpu->gpr[10] >> 12); /* 80049840 sra $t2, $t2, 12 */
    entry->cpu->gpr[10] = entry->cpu->gpr[10] << 16; /* 80049844 sll $t2, $t2, 16 */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] | entry->cpu->gpr[10]; /* 80049848 or $t1, $t1, $t2 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0xCu, 4, entry->cpu->gpr[9]); /* 8004984C sw $t1, 0xC($a0) */
    entry->cpu->gpr[9] = entry->cpu->gpr[8] & 0xFFFFu; /* 80049850 andi $t1, $t0, 0xFFFF */
    entry->cpu->gpr[9] = entry->cpu->gpr[9] << 16; /* 80049854 sll $t1, $t1, 16 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 16); /* 80049858 sra $t1, $t1, 16 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[13]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004985C multu $t1, $t5 */
    entry->cpu->gpr[9] = entry->cpu->lo; /* 80049860 mflo $t1 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[9] >> 12); /* 80049864 sra $t1, $t1, 12 */
    /* 80049868 jr $ra */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x10u, 4, entry->cpu->gpr[9]); goto bl_exit;
    ; /* 80049870 nop  */
    ; /* 80049874 nop  */
    ; /* 80049878 nop  */
bl_exit:
}

/* Restricted to the atlas ScaleMatrixL edge with aligned accessible spans. */
static int battle_scale_matrix_try_host(BattleMipsRuntime *runtime,
                                        PcPortMipsCpu *cpu, uint32_t target)
{
    if(target!=0x8004974cu || cpu->gpr[31]!=0x80026074u ||
       runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_SCALE_MATRIX_EXPERIMENT");
    if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+0x124u)return 0;
    static const char expected[]="1011aad3b6c547d9aae4abe7c7bbe570a07f69455a5c20b6a23b1ca3598ebaae";
    uint8_t digest[32];file1_digest(image->bytes+offset,0x124u,digest);
    for(unsigned i=0;i<sizeof(digest);++i)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
           file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t matrix=cpu->gpr[4],vector=cpu->gpr[5];
    if(((matrix|vector)&3u) || matrix>UINT32_MAX-16u || vector>UINT32_MAX-8u ||
       matrix<BATTLE_BUS_KUSEG_RAM_END || vector<BATTLE_BUS_KUSEG_RAM_END)return 0;
    for(unsigned i=0;i<20u;i+=4u)
        if(!resolve_memory(runtime,matrix+i,4,0) || !resolve_memory(runtime,matrix+i,4,1))return 0;
    for(unsigned i=0;i<12u;i+=4u)
        if(!resolve_memory(runtime,vector+i,4,0))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_scale_matrix_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
