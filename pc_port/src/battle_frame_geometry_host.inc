/* Experimental frame geometry update with explicit guest register effects. */
static void battle_geometry_run(BattleEntryCall *entry) {
    entry->cpu->gpr[6] = entry->cpu->gpr[4] + 0u; /* 800765C4 addu $a2, $a0, $zero */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800765C8 lui $v1, %hi(D_800D3280) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x3280u, 1); /* 800765CC lbu $v1, %lo(D_800D3280)($v1) */
    entry->cpu->gpr[4] = 0x800D0000u; /* 800765D0 lui $a0, %hi(D_800D2D28) */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[4] + 0x2D28u, 4); /* 800765D4 lw $a0, %lo(D_800D2D28)($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[3] << 1; /* 800765D8 sll $v0, $v1, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800765DC addu $v0, $v0, $v1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[6]; /* 800765E0 addu $v0, $v0, $a2 */
    entry->cpu->gpr[3] = 0x800C0000u; /* 800765E4 lui $v1, %hi(D_800C3254) */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x3254u; /* 800765E8 addiu $v1, $v1, %lo(D_800C3254) */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] << 1; /* 800765EC sll $v0, $v0, 1 */
    entry->cpu->gpr[7] = entry->cpu->gpr[2] + entry->cpu->gpr[3]; /* 800765F0 addu $a3, $v0, $v1 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[7], 2); /* 800765F4 lhu $v1, 0x0($a3) */
    entry->cpu->gpr[2] = 0u | 0x1Cu; /* 800765F8 ori $v0, $zero, 0x1C */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x44u, 4, entry->cpu->gpr[2]); /* 800765FC sw $v0, 0x44($a0) */
    entry->cpu->gpr[2] = entry->cpu->gpr[6] << 1; /* 80076600 sll $v0, $a2, 1 */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + entry->cpu->gpr[6]; /* 80076604 addu $v0, $v0, $a2 */
    entry->cpu->gpr[5] = entry->cpu->gpr[2] << 5; /* 80076608 sll $a1, $v0, 5 */
    entry->cpu->gpr[2] = entry->cpu->gpr[6] << 3; /* 8007660C sll $v0, $a2, 3 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x48u; /* 80076610 addiu $v1, $v1, 0x48 */
    entry->cpu->gpr[3] = entry->cpu->gpr[5] + entry->cpu->gpr[3]; /* 80076614 addu $v1, $a1, $v1 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x34u, 4, entry->cpu->gpr[3]); /* 80076618 sw $v1, 0x34($a0) */
    entry->cpu->gpr[1] = 0x800D0000u; /* 8007661C lui $at, %hi(D_800D32A1) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[2]; /* 80076620 addu $at, $at, $v0 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x32A1u, 1); /* 80076624 lbu $v0, %lo(D_800D32A1)($at) */
    ; /* 80076628 nop  */
    /* 8007662C beqz $v0, .L80076660 */
    { int c_ = (entry->cpu->gpr[2] == 0); entry->cpu->gpr[2] = 0u | 0x7u; if (c_) goto L_80076660; }
    entry->cpu->gpr[1] = 0x800D0000u; /* 80076634 lui $at, %hi(D_800D2D24) */
    entry->cpu->gpr[1] = entry->cpu->gpr[1] + entry->cpu->gpr[6]; /* 80076638 addu $at, $at, $a2 */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[1] + 0x2D24u, 1); /* 8007663C lbu $v1, %lo(D_800D2D24)($at) */
    ; /* 80076640 nop  */
    /* 80076644 beq $v1, $v0, .L80076660 */
    { int c_ = (entry->cpu->gpr[3] == entry->cpu->gpr[2]); entry->cpu->gpr[3] = 0u | 0x24u; if (c_) goto L_80076660; }
    { uint32_t value=battle_entry_read(entry,entry->cpu->gpr[7],2);
      battle_entry_write(entry,entry->cpu->gpr[4]+0x44u,4,entry->cpu->gpr[3]); entry->cpu->gpr[2]=value; }
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0x44u; /* 80076654 addiu $v0, $v0, 0x44 */
    entry->cpu->gpr[2] = entry->cpu->gpr[5] + entry->cpu->gpr[2]; /* 80076658 addu $v0, $a1, $v0 */
    battle_entry_write(entry, entry->cpu->gpr[4] + 0x34u, 4, entry->cpu->gpr[2]); /* 8007665C sw $v0, 0x34($a0) */
L_80076660:
    entry->cpu->gpr[2] = 0x800D0000u; /* 80076660 lui $v0, %hi(D_800D2D28) */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x2D28u, 4); /* 80076664 lw $v0, %lo(D_800D2D28)($v0) */
    ; /* 80076668 nop  */
    entry->cpu->gpr[4] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x34u, 4); /* 8007666C lw $a0, 0x34($v0) */
    entry->cpu->gpr[3] = 0u | 0x10u; /* 80076670 ori $v1, $zero, 0x10 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x3Cu, 4, entry->cpu->gpr[3]); /* 80076674 sw $v1, 0x3C($v0) */
    entry->cpu->gpr[5] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x3Cu, 4); /* 80076678 lw $a1, 0x3C($v0) */
    entry->cpu->gpr[3] = 0u | 0x98u; /* 8007667C ori $v1, $zero, 0x98 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x4Cu, 4, entry->cpu->gpr[3]); /* 80076680 sw $v1, 0x4C($v0) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[2] + 0x44u, 4); /* 80076684 lw $v1, 0x44($v0) */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] + 0xFFFFFFFBu; /* 80076688 addiu $a0, $a0, -0x5 */
    entry->cpu->gpr[4] = entry->cpu->gpr[4] - entry->cpu->gpr[5]; /* 8007668C subu $a0, $a0, $a1 */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + 0x5u; /* 80076690 addiu $v1, $v1, 0x5 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x54u, 4, entry->cpu->gpr[4]); /* 80076694 sw $a0, 0x54($v0) */
    { uint32_t first=battle_entry_read(entry,entry->cpu->gpr[2]+0x4cu,4);
      uint32_t second=battle_entry_read(entry,entry->cpu->gpr[2]+0x54u,4);
      entry->cpu->gpr[4]=first; entry->cpu->gpr[5]=second; }
    entry->cpu->gpr[4] = entry->cpu->gpr[4] - entry->cpu->gpr[3]; /* 800766A0 subu $a0, $a0, $v1 */
    entry->cpu->gpr[5] = entry->cpu->gpr[5] << 8; /* 800766A4 sll $a1, $a1, 8 */
    if (entry->cpu->gpr[4]) { entry->cpu->lo=entry->cpu->gpr[5]/entry->cpu->gpr[4]; entry->cpu->hi=entry->cpu->gpr[5]%entry->cpu->gpr[4]; } else { entry->cpu->lo=0xffffffffu; entry->cpu->hi=entry->cpu->gpr[5]; } /* 800766A8 divu $zero, $a1, $a0 */
    entry->cpu->gpr[5] = entry->cpu->lo; /* 800766AC mflo $a1 */
    entry->cpu->gpr[3] = 0u | 0x800u; /* 800766B0 ori $v1, $zero, 0x800 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x104u, 2, entry->cpu->gpr[3]); /* 800766B4 sh $v1, 0x104($v0) */
    entry->cpu->gpr[3] = 0u | 0x6u; /* 800766B8 ori $v1, $zero, 0x6 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0xA9u, 1, entry->cpu->gpr[3]); /* 800766BC sb $v1, 0xA9($v0) */
    entry->cpu->gpr[3] = 0u | 0x100u; /* 800766C0 ori $v1, $zero, 0x100 */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5Cu, 4, entry->cpu->gpr[4]); /* 800766C4 sw $a0, 0x5C($v0) */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x5Cu, 4, entry->cpu->gpr[3]); /* 800766C8 sw $v1, 0x5C($v0) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800766CC lui $v1, %hi(D_800D2D28) */
    { uint32_t owner=battle_entry_read(entry,entry->cpu->gpr[3]+0x2d28u,4);
      battle_entry_write(entry,entry->cpu->gpr[2]+0x64u,4,0u); entry->cpu->gpr[3]=owner; }
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x6Cu, 4, 0u); /* 800766D8 sw $zero, 0x6C($v0) */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x106u, 2, 0u); /* 800766DC sh $zero, 0x106($v0) */
    battle_entry_write(entry, entry->cpu->gpr[2] + 0x54u, 4, entry->cpu->gpr[5]); /* 800766E0 sw $a1, 0x54($v0) */
    entry->cpu->gpr[2] = 0u | 0x1u; /* 800766E4 ori $v0, $zero, 0x1 */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0xABu, 1, entry->cpu->gpr[2]); /* 800766E8 sb $v0, 0xAB($v1) */
    entry->cpu->gpr[3] = 0x800D0000u; /* 800766EC lui $v1, %hi(D_800D2D28) */
    entry->cpu->gpr[3] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x2D28u, 4); /* 800766F0 lw $v1, %lo(D_800D2D28)($v1) */
    ; /* 800766F4 nop  */
    entry->cpu->gpr[3] = entry->cpu->gpr[3] + entry->cpu->gpr[6]; /* 800766F8 addu $v1, $v1, $a2 */
    entry->cpu->gpr[2] = battle_entry_read(entry, entry->cpu->gpr[3] + 0x90u, 1); /* 800766FC lbu $v0, 0x90($v1) */
    ; /* 80076700 nop  */
    entry->cpu->gpr[2] = entry->cpu->gpr[2] + 0xFFFFFFFFu; /* 80076704 addiu $v0, $v0, -0x1 */
    /* 80076708 jr $ra */
    battle_entry_write(entry, entry->cpu->gpr[3] + 0x90u, 1, entry->cpu->gpr[2]); goto bl_exit;
bl_exit:
}

/* Restricted to the observed HUD frame call. */
static int battle_frame_geometry_try_host(BattleMipsRuntime *runtime,
                                          PcPortMipsCpu *cpu, uint32_t target)
{
    if (target!=0x800765c4u || cpu->gpr[31]!=0x800739c4u || cpu->gpr[4]>=11u ||
        runtime->force_interpret || !runtime->initialized ||
        g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
        (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_FRAME_GEOMETRY_EXPERIMENT");
    if (!control || strcmp(control,"1")!=0) return 0;
    static const char expected[]="d366601762d83d3478d1dbf47d42c4b90e588ad5a117b7d7d858e20f65699490";
    uint8_t digest[32];file1_digest(PSX_ADDR(target),0x14cu,digest);
    for (unsigned i=0;i<sizeof(digest);++i)
        if (file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
            file1_hex_digit(digest[i]&15u)!=expected[2*i+1]) return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_geometry_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
