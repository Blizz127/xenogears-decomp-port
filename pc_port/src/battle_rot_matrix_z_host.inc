/* Experimental RotMatrixZ guest-register/HI/LO body. */
static uint32_t battle_rot_read_s16(BattleEntryCall *e,uint32_t a){return (uint32_t)(int32_t)(int16_t)battle_entry_read(e,a,2);}
static void battle_rot_matrix_z_run(BattleEntryCall *entry) {
    entry->cpu->gpr[15] = entry->cpu->gpr[4] + 0u; /* 8004B18C addu $t7, $a0, $zero */
    entry->cpu->gpr[2] = entry->cpu->gpr[5] + 0u; /* 8004B190 addu $v0, $a1, $zero */
    /* 8004B194 bgez $t7, .L8004B1D0 */
    { int c_ = ((int32_t)entry->cpu->gpr[15] >= 0); entry->cpu->gpr[25] = entry->cpu->gpr[15] & 0xFFFu; if (c_) goto L_8004B1D0; }
    entry->cpu->gpr[15] = 0u - entry->cpu->gpr[15]; /* 8004B19C negu $t7, $t7 */
    /* 8004B1A0 bgez $t7, .L8004B1A8 */
    { int c_ = ((int32_t)entry->cpu->gpr[15] >= 0); entry->cpu->gpr[15] = entry->cpu->gpr[15] & 0xFFFu; if (c_) goto L_8004B1A8; }
L_8004B1A8:
    entry->cpu->gpr[24] = entry->cpu->gpr[15] << 2; /* 8004B1A8 sll $t8, $t7, 2 */
    entry->cpu->gpr[25] = 0x80050000u; /* 8004B1AC lui $t9, %hi(D_800523F0) */
    entry->cpu->gpr[25] = entry->cpu->gpr[25] + entry->cpu->gpr[24]; /* 8004B1B0 addu $t9, $t9, $t8 */
    entry->cpu->gpr[25] = battle_entry_read(entry, entry->cpu->gpr[25] + 0x23F0u, 4); /* 8004B1B4 lw $t9, %lo(D_800523F0)($t9) */
    ; /* 8004B1B8 nop  */
    entry->cpu->gpr[14] = entry->cpu->gpr[25] << 16; /* 8004B1BC sll $t6, $t9, 16 */
    entry->cpu->gpr[14] = (uint32_t)((int32_t)entry->cpu->gpr[14] >> 16); /* 8004B1C0 sra $t6, $t6, 16 */
    entry->cpu->gpr[9] = 0u - entry->cpu->gpr[14]; /* 8004B1C4 negu $t1, $t6 */
    /* 8004B1C8 j .L8004B1F0 */
    entry->cpu->gpr[8] = (uint32_t)((int32_t)entry->cpu->gpr[25] >> 16); goto L_8004B1F0;
L_8004B1D0:
    entry->cpu->gpr[24] = entry->cpu->gpr[25] << 2; /* 8004B1D0 sll $t8, $t9, 2 */
    entry->cpu->gpr[25] = 0x80050000u; /* 8004B1D4 lui $t9, %hi(D_800523F0) */
    entry->cpu->gpr[25] = entry->cpu->gpr[25] + entry->cpu->gpr[24]; /* 8004B1D8 addu $t9, $t9, $t8 */
    entry->cpu->gpr[25] = battle_entry_read(entry, entry->cpu->gpr[25] + 0x23F0u, 4); /* 8004B1DC lw $t9, %lo(D_800523F0)($t9) */
    ; /* 8004B1E0 nop  */
    entry->cpu->gpr[24] = entry->cpu->gpr[25] << 16; /* 8004B1E4 sll $t8, $t9, 16 */
    entry->cpu->gpr[9] = (uint32_t)((int32_t)entry->cpu->gpr[24] >> 16); /* 8004B1E8 sra $t1, $t8, 16 */
    entry->cpu->gpr[8] = (uint32_t)((int32_t)entry->cpu->gpr[25] >> 16); /* 8004B1EC sra $t0, $t9, 16 */
L_8004B1F0:
    {
        uint32_t delayed_0=battle_rot_read_s16(entry, entry->cpu->gpr[5]); /* 8004B1F0 */
        uint32_t delayed_1=battle_rot_read_s16(entry, entry->cpu->gpr[5] + 0x6u); /* 8004B1F4 */
        entry->cpu->gpr[10]=delayed_0;
        entry->cpu->gpr[13]=delayed_1;
    }
    { uint64_t product=(uint64_t)entry->cpu->gpr[8] * entry->cpu->gpr[10]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B1F8 multu $t0, $t2 */
    {
        uint32_t delayed_0=battle_rot_read_s16(entry, entry->cpu->gpr[5] + 0x2u); /* 8004B1FC */
        uint32_t delayed_1=battle_rot_read_s16(entry, entry->cpu->gpr[5] + 0x8u); /* 8004B200 */
        entry->cpu->gpr[11]=delayed_0;
        entry->cpu->gpr[14]=delayed_1;
    }
    entry->cpu->gpr[24] = entry->cpu->lo; /* 8004B204 mflo $t8 */
    {
        uint32_t delayed_0=battle_rot_read_s16(entry, entry->cpu->gpr[5] + 0x4u); /* 8004B208 */
        uint32_t delayed_1=battle_rot_read_s16(entry, entry->cpu->gpr[5] + 0xAu); /* 8004B20C */
        entry->cpu->gpr[12]=delayed_0;
        entry->cpu->gpr[15]=delayed_1;
    }
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[13]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B210 multu $t1, $t5 */
    ; /* 8004B214 nop  */
    ; /* 8004B218 nop  */
    entry->cpu->gpr[25] = entry->cpu->lo; /* 8004B21C mflo $t9 */
    entry->cpu->gpr[25] = entry->cpu->gpr[24] - entry->cpu->gpr[25]; /* 8004B220 subu $t9, $t8, $t9 */
    entry->cpu->gpr[24] = (uint32_t)((int32_t)entry->cpu->gpr[25] >> 12); /* 8004B224 sra $t8, $t9, 12 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[8] * entry->cpu->gpr[11]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B228 multu $t0, $t3 */
    battle_entry_write(entry, entry->cpu->gpr[5], 2, entry->cpu->gpr[24]); /* 8004B22C sh $t8, 0x0($a1) */
    ; /* 8004B230 nop  */
    entry->cpu->gpr[24] = entry->cpu->lo; /* 8004B234 mflo $t8 */
    ; /* 8004B238 nop  */
    ; /* 8004B23C nop  */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[14]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B240 multu $t1, $t6 */
    ; /* 8004B244 nop  */
    ; /* 8004B248 nop  */
    entry->cpu->gpr[25] = entry->cpu->lo; /* 8004B24C mflo $t9 */
    entry->cpu->gpr[25] = entry->cpu->gpr[24] - entry->cpu->gpr[25]; /* 8004B250 subu $t9, $t8, $t9 */
    entry->cpu->gpr[24] = (uint32_t)((int32_t)entry->cpu->gpr[25] >> 12); /* 8004B254 sra $t8, $t9, 12 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[8] * entry->cpu->gpr[12]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B258 multu $t0, $t4 */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x2u, 2, entry->cpu->gpr[24]); /* 8004B25C sh $t8, 0x2($a1) */
    ; /* 8004B260 nop  */
    entry->cpu->gpr[24] = entry->cpu->lo; /* 8004B264 mflo $t8 */
    ; /* 8004B268 nop  */
    ; /* 8004B26C nop  */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[15]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B270 multu $t1, $t7 */
    ; /* 8004B274 nop  */
    ; /* 8004B278 nop  */
    entry->cpu->gpr[25] = entry->cpu->lo; /* 8004B27C mflo $t9 */
    entry->cpu->gpr[25] = entry->cpu->gpr[24] - entry->cpu->gpr[25]; /* 8004B280 subu $t9, $t8, $t9 */
    entry->cpu->gpr[24] = (uint32_t)((int32_t)entry->cpu->gpr[25] >> 12); /* 8004B284 sra $t8, $t9, 12 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[10]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B288 multu $t1, $t2 */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x4u, 2, entry->cpu->gpr[24]); /* 8004B28C sh $t8, 0x4($a1) */
    ; /* 8004B290 nop  */
    entry->cpu->gpr[24] = entry->cpu->lo; /* 8004B294 mflo $t8 */
    ; /* 8004B298 nop  */
    ; /* 8004B29C nop  */
    { uint64_t product=(uint64_t)entry->cpu->gpr[8] * entry->cpu->gpr[13]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B2A0 multu $t0, $t5 */
    ; /* 8004B2A4 nop  */
    ; /* 8004B2A8 nop  */
    entry->cpu->gpr[25] = entry->cpu->lo; /* 8004B2AC mflo $t9 */
    entry->cpu->gpr[25] = entry->cpu->gpr[24] + entry->cpu->gpr[25]; /* 8004B2B0 addu $t9, $t8, $t9 */
    entry->cpu->gpr[24] = (uint32_t)((int32_t)entry->cpu->gpr[25] >> 12); /* 8004B2B4 sra $t8, $t9, 12 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[11]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B2B8 multu $t1, $t3 */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x6u, 2, entry->cpu->gpr[24]); /* 8004B2BC sh $t8, 0x6($a1) */
    ; /* 8004B2C0 nop  */
    entry->cpu->gpr[24] = entry->cpu->lo; /* 8004B2C4 mflo $t8 */
    ; /* 8004B2C8 nop  */
    ; /* 8004B2CC nop  */
    { uint64_t product=(uint64_t)entry->cpu->gpr[8] * entry->cpu->gpr[14]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B2D0 multu $t0, $t6 */
    ; /* 8004B2D4 nop  */
    ; /* 8004B2D8 nop  */
    entry->cpu->gpr[25] = entry->cpu->lo; /* 8004B2DC mflo $t9 */
    entry->cpu->gpr[25] = entry->cpu->gpr[24] + entry->cpu->gpr[25]; /* 8004B2E0 addu $t9, $t8, $t9 */
    entry->cpu->gpr[24] = (uint32_t)((int32_t)entry->cpu->gpr[25] >> 12); /* 8004B2E4 sra $t8, $t9, 12 */
    { uint64_t product=(uint64_t)entry->cpu->gpr[9] * entry->cpu->gpr[12]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B2E8 multu $t1, $t4 */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0x8u, 2, entry->cpu->gpr[24]); /* 8004B2EC sh $t8, 0x8($a1) */
    ; /* 8004B2F0 nop  */
    entry->cpu->gpr[24] = entry->cpu->lo; /* 8004B2F4 mflo $t8 */
    ; /* 8004B2F8 nop  */
    ; /* 8004B2FC nop  */
    { uint64_t product=(uint64_t)entry->cpu->gpr[8] * entry->cpu->gpr[15]; entry->cpu->lo=(uint32_t)product; entry->cpu->hi=(uint32_t)(product>>32); } /* 8004B300 multu $t0, $t7 */
    ; /* 8004B304 nop  */
    ; /* 8004B308 nop  */
    entry->cpu->gpr[25] = entry->cpu->lo; /* 8004B30C mflo $t9 */
    entry->cpu->gpr[25] = entry->cpu->gpr[24] + entry->cpu->gpr[25]; /* 8004B310 addu $t9, $t8, $t9 */
    entry->cpu->gpr[24] = (uint32_t)((int32_t)entry->cpu->gpr[25] >> 12); /* 8004B314 sra $t8, $t9, 12 */
    battle_entry_write(entry, entry->cpu->gpr[5] + 0xAu, 2, entry->cpu->gpr[24]); /* 8004B318 sh $t8, 0xA($a1) */
    /* 8004B31C jr $ra */
    ; goto bl_exit;
    ; /* 8004B324 nop  */
    ; /* 8004B328 nop  */
bl_exit:
}

/* Restricted to the atlas RotMatrixZ edge and an exact retail trig entry. */
static int battle_rot_matrix_z_try_host(BattleMipsRuntime *runtime,
                                        PcPortMipsCpu *cpu, uint32_t target)
{
    if(target!=0x8004b18cu || cpu->gpr[31]!=0x80026084u ||
       runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ROT_MATRIX_EXPERIMENT");
    if(!control || strcmp(control,"1")!=0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+0x198u)return 0;
    static const char expected[]="61d070c21903b9f4ad86f1c70a1e377f4482efb267bba6dfc46f110e73ba2454";
    uint8_t digest[32];file1_digest(image->bytes+offset,0x198u,digest);
    for(unsigned i=0;i<sizeof(digest);++i)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
           file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint32_t angle=cpu->gpr[4];
    uint32_t index=((int32_t)angle<0 ? 0u-angle : angle)&0xfffu;
    uint32_t table_addr=0x800523f0u+index*4u,table_word;
    size_t table_offset=XENO_RD_SLUS_OFF(table_addr);
    if(image->size<table_offset+4u || runtime_read(runtime,table_addr,4,&table_word)!=0 ||
       table_word!=load_le(image->bytes+table_offset,4))return 0;
    uint32_t matrix=cpu->gpr[5];
    if((matrix&1u) || matrix>UINT32_MAX-10u || matrix<BATTLE_BUS_KUSEG_RAM_END)return 0;
    for(unsigned i=0;i<12u;i+=2u)
        if(!resolve_memory(runtime,matrix+i,2,0) || !resolve_memory(runtime,matrix+i,2,1))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_rot_matrix_z_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
