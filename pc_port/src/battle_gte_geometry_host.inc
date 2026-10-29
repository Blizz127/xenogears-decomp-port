/* Experimental native GTE geometry control bodies. */
static void battle_geom_control_run(BattleEntryCall *entry,uint32_t target)
{
    uint32_t *r=entry->cpu->gpr;
    if(target==0x8004a0bcu) {
        uint32_t x=CFC2(24),y=CFC2(25);r[8]=x;
        r[8]=(uint32_t)((int32_t)r[8]>>16);r[9]=y;
        r[9]=(uint32_t)((int32_t)r[9]>>16);
        battle_entry_write(entry,r[4],4,r[8]);battle_entry_write(entry,r[5],4,r[9]);
    } else if(target==0x8004a0dcu) {
        r[2]=CFC2(26);
    } else if(target==0x8004a12cu) {
        r[4]<<=16;r[5]<<=16;CTC2(r[4],24);CTC2(r[5],25);
    } else if(target==0x8004a14cu) {
        CTC2(r[4],26);
    } else battle_entry_fail(entry,"unsupported GTE control",target);
}

/* Only verified atlas geometry-control call sites. */
static int battle_geom_control_try_host(BattleMipsRuntime *runtime,
                                        PcPortMipsCpu *cpu, uint32_t target)
{
    if(runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
       (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_GTE_GEOMETRY_EXPERIMENT");
    if(!control || strcmp(control,"1")!=0)return 0;
    uint32_t ra=cpu->gpr[31];unsigned length;const char *expected;
    if(target==0x8004a0bcu && ra==0x80026090u) {
        length=32u;expected="2f9e759091975bad3585277ae26c3eb50addd764857351a5213c14fb2c857a89";
        if(((cpu->gpr[4]|cpu->gpr[5])&3u) ||
           cpu->gpr[4]<BATTLE_BUS_KUSEG_RAM_END || cpu->gpr[5]<BATTLE_BUS_KUSEG_RAM_END ||
           !resolve_memory(runtime,cpu->gpr[4],4,1) || !resolve_memory(runtime,cpu->gpr[5],4,1))return 0;
    } else if(target==0x8004a0dcu && ra==0x80026098u) {
        length=12u;expected="4ecf2f5bbdb450de400e5eeafac260aaf27c3f5d36b3f67ef8ceb47855a2e800";
    } else if(target==0x8004a12cu && (ra==0x800260acu || ra==0x800262ecu)) {
        length=24u;expected="9aa95b09a0c5f31d49c3f954d1bc4cbe5101b46feb9a7a9a92673d0716fdfcf1";
    } else if(target==0x8004a14cu && (ra==0x800260b4u || ra==0x800262f8u)) {
        length=12u;expected="d8c79a8c2661cba5b0c79fdf925407b5a4945c383bb0c8c293287878a693cac2";
    } else return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+length)return 0;
    uint8_t digest[32];file1_digest(image->bytes+offset,length,digest);
    for(unsigned i=0;i<sizeof(digest);++i)
        if(file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
           file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_geom_control_run(&entry,target);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
