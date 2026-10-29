/* VectorNormalS: the word-input entry, shared halfword-output tail and
 * fixed SQR/GPF kernel. This entry preserves the GTE and architectural
 * side effects that the arithmetic-only compatibility function omits.
 * The lookup data comes from the existing private-executable data loader. */
#define VN_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define VN_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define VN_W(a,v) battle_entry_write(entry,shard_sdk_address(a),2,(v))
#define VN_AT (entry->cpu->gpr[1])
#define VN_V0 (entry->cpu->gpr[2])
#define VN_V1 (entry->cpu->gpr[3])
#define VN_A0 (entry->cpu->gpr[4])
#define VN_A1 (entry->cpu->gpr[5])
#define VN_A3 (entry->cpu->gpr[7])
#define VN_T0 (entry->cpu->gpr[8])
#define VN_T1 (entry->cpu->gpr[9])
#define VN_T2 (entry->cpu->gpr[10])
#define VN_T3 (entry->cpu->gpr[11])
#define VN_T4 (entry->cpu->gpr[12])
#define VN_T5 (entry->cpu->gpr[13])
#define VN_T6 (entry->cpu->gpr[14])
#define VN_RA (entry->cpu->gpr[31])

static uint32_t battle_vector_normal_s_checked(BattleEntryCall *entry,
                                               int64_t value,uint32_t site)
{
    if(value<INT32_MIN||value>INT32_MAX)
        battle_entry_fail(entry,"vector normalization signed overflow",site);
    return (uint32_t)value;
}
static void battle_vector_normal_s_run(BattleEntryCall *entry)
{
    uint32_t p=VN_R(VN_A0),q=VN_R(VN_A0+4u);VN_T0=p;
    p=VN_R(VN_A0+8u);VN_T1=q;VN_T2=p;
    VN_A3=VN_RA;VN_RA=0x80048dc4u;
    MTC2(VN_T0,9);MTC2(VN_T1,10);MTC2(VN_T2,11);
    doCOP2(0x4aa00428); /* SQR(0), including its IR/MAC/FLAG effects. */
    p=MFC2(25);q=MFC2(26);VN_T3=p;
    p=MFC2(27);VN_T4=q;
    /* The first signed ADD can fail while the third MAC read is pending. */
    VN_T3=battle_vector_normal_s_checked(entry,(int64_t)(int32_t)VN_T3+(int32_t)VN_T4,0x80048df8u);
    VN_T5=p;
    VN_V0=battle_vector_normal_s_checked(entry,(int64_t)(int32_t)VN_T3+(int32_t)VN_T5,0x80048dfcu);
    MTC2(VN_V0,30);p=MFC2(31);VN_AT=0xfffffffeu;VN_V1=p;
    VN_V1&=VN_AT;VN_T6=31u;
    VN_T6=battle_vector_normal_s_checked(entry,(int64_t)(int32_t)VN_T6-(int32_t)VN_V1,0x80048e1cu);
    VN_T3=battle_vector_normal_s_checked(entry,(int64_t)(int32_t)VN_V1-24,0x80048e20u);
    int negative=(int32_t)VN_T3<0;VN_T6=(uint32_t)((int32_t)VN_T6>>1);
    if(!negative)VN_T4=VN_V0<<(VN_T3&31u);
    else{
        VN_T3=24u;VN_T3=battle_vector_normal_s_checked(entry,(int64_t)(int32_t)VN_T3-(int32_t)VN_V1,0x80048e38u);
        VN_T4=(uint32_t)((int32_t)VN_V0>>(VN_T3&31u));
    }
    VN_T4=battle_vector_normal_s_checked(entry,(int64_t)(int32_t)VN_T4-64,0x80048e40u);
    VN_T4<<=1;VN_T5=0x80050000u;VN_T5+=VN_T4;
    /* Zero length intentionally reads 80056B14, preceding the usual table.
     * No clamp or synthetic zero-vector shortcut replaces that access. */
    p=VN_H(VN_T5+0x6b94u);VN_T5=(uint32_t)(int32_t)(int16_t)p;
    MTC2(VN_T5,8);MTC2(VN_T0,9);MTC2(VN_T1,10);MTC2(VN_T2,11);
    doCOP2(0x4b90003d); /* GPF(0). */
    p=MFC2(25);q=MFC2(26);VN_T0=p;
    p=MFC2(27);VN_T1=q;VN_T0=(uint32_t)((int32_t)VN_T0>>(VN_T6&31u));VN_T2=p;
    VN_T1=(uint32_t)((int32_t)VN_T1>>(VN_T6&31u));
    VN_T2=(uint32_t)((int32_t)VN_T2>>(VN_T6&31u));
    VN_RA=VN_A3;
    VN_W(VN_A1,VN_T0);VN_W(VN_A1+2u,VN_T1);VN_W(VN_A1+4u,VN_T2);
}
static int battle_vector_normal_s_try_host(BattleMipsRuntime *runtime,
                                           PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x80048d68u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_VECTOR_NORMAL_S");
    if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    const uint32_t starts[]={0x80048d68u,0x80048dbcu,0x80048dd8u};
    const size_t lengths[]={20u,28u,188u};
    const char *expected[]={
        "dadf449ade2ebb2272634c57bbe865b5ebff661ae33bfd17af673782c309b618",
        "0b5167c2fcd95a128e0ca424e8c7afd696649b0ffe92860ce39c89f94024f535",
        "7e6f8fdab6570a20c556da4d6e50ae83ff8934ea0592ba60da26d5a76e4e4db6"};
    if(!image)return 0;
    for(unsigned span=0;span<3;span++){
        size_t offset=XENO_RD_SLUS_OFF(starts[span]);if(image->size<offset+lengths[span])return 0;
        uint8_t hash[32];file1_digest(image->bytes+offset,lengths[span],hash);
        for(unsigned i=0;i<32;i++)if(file1_hex_digit(hash[i]>>4)!=expected[span][i*2]||file1_hex_digit(hash[i]&15u)!=expected[span][i*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_vector_normal_s_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef VN_R
#undef VN_H
#undef VN_W
#undef VN_AT
#undef VN_V0
#undef VN_V1
#undef VN_A0
#undef VN_A1
#undef VN_A3
#undef VN_T0
#undef VN_T1
#undef VN_T2
#undef VN_T3
#undef VN_T4
#undef VN_T5
#undef VN_T6
#undef VN_RA
