/* Experimental faithful guest-register bodies for five atlas GPU helpers. */
static void battle_atlas_gpu_run(BattleEntryCall *entry,uint32_t target)
{
    uint32_t *r=entry->cpu->gpr;
    if(target==0x80043cb0u) {
        r[2]=9; battle_entry_write(entry,r[4]+3u,1,r[2]);
        r[2]=44; battle_entry_write(entry,r[4]+7u,1,r[2]);
    } else if(target==0x80043bfcu || target==0x80043c24u) {
        uint32_t bit=target==0x80043bfcu?2u:1u;
        r[2]=battle_entry_read(entry,r[4]+7u,1);
        r[2]=r[5] ? r[2]|bit : r[2]&(~bit&0xffu);
        battle_entry_write(entry,r[4]+7u,1,r[2]);
    } else if(target==0x80043a1cu) {
        r[2]=(r[4]&3u)<<7;r[5]=(r[5]&3u)<<5;r[2]|=r[5];
        r[3]=(r[7]&0x100u)>>4;r[2]|=r[3];r[6]=(r[6]&0x3ffu)>>6;r[2]|=r[6];
        r[7]=(r[7]&0x200u)<<2;r[2]|=r[7];
    } else if(target==0x80043a58u) {
        r[2]=r[5]<<6;r[4]=(uint32_t)((int32_t)r[4]>>4)&0x3fu;r[2]=(r[2]|r[4])&0xffffu;
    } else battle_entry_fail(entry,"unsupported atlas GPU helper",target);
}

/* Exact atlas GPU-helper call sites and the sprite transparency edge. */
static int battle_atlas_gpu_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime ||
       runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    uint32_t ra=cpu->gpr[31];
    int popup_shade=target==0x80043c24u && ra==0x800b1798u;
    const char *control=getenv(popup_shade ? "XENO_BATTLE_NATIVE_POPUP_SHADE_EXPERIMENT" : "XENO_BATTLE_NATIVE_ATLAS_GPU_EXPERIMENT");
    if(!control || strcmp(control,"1")!=0)return 0;
    unsigned length;const char *expected;
    if(target==0x80043cb0u && ra==0x80026110u) {length=20;expected="b9c80a1f0571ac7390790b7eaa055a21fb9fe4c1c94374385bc021661bf9e535";}
    else if(target==0x80043bfcu && (ra==0x8002611cu || ra==0x800768f0u)) {length=40;expected="f7bd3db94e23b108242338e61ab13d8b1f6877d7a227b90ed3f11c550612804b";}
    else if(target==0x80043c24u && (ra==0x80026128u || popup_shade)) {length=40;expected="0c8699692a9c170bc55cb4e3171f8001f6b2f19d33b1e8c6cbb722317bf88377";}
    else if(target==0x80043a1cu && ra==0x80026140u) {length=60;expected="ca97bb54edac0e68e757a06d6a25eb10c8fea745f5234450b15d0997d759bf42";}
    else if(target==0x80043a58u && ra==0x80026154u) {length=24;expected="879cc65128f4db04b58c281f0867c888d51875da3e7ff5bbb309a1595a4f2457";}
    else return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image || !image->bytes || image->size<offset+length)return 0;
    uint8_t digest[32];file1_digest(image->bytes+offset,length,digest);
    for(unsigned i=0;i<sizeof(digest);++i)if(file1_hex_digit(digest[i]>>4)!=expected[i*2] || file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    if(target==0x80043cb0u || target==0x80043bfcu || target==0x80043c24u) {
        uint32_t ptr=cpu->gpr[4];if(ptr<BATTLE_BUS_KUSEG_RAM_END || ptr>UINT32_MAX-7u)return 0;
        if(!resolve_memory(runtime,ptr+7u,1,1))return 0;
        if(target==0x80043cb0u) {if(!resolve_memory(runtime,ptr+3u,1,1))return 0;}
        else if(!resolve_memory(runtime,ptr+7u,1,0))return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_atlas_gpu_run(&entry,target);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
