/* Fixed native RotMatrixYXZ, 8004A92C..8004ABB8. Keep staged Q12 rounding,
 * unsigned MULT HI/LO, halfword stores and the third trig lookup after the
 * first three stores. Table/output aliases therefore retain their ordering. */
#define RY_H(a) battle_entry_read(entry,shard_sdk_address(a),2)
#define RY_R(a) battle_entry_read(entry,shard_sdk_address(a),4)
#define RY_W(a,v) battle_entry_write(entry,shard_sdk_address(a),2,(v))
#define RY_V0 (entry->cpu->gpr[2])
#define RY_A0 (entry->cpu->gpr[4])
#define RY_A1 (entry->cpu->gpr[5])
#define RY_T0 (entry->cpu->gpr[8])
#define RY_T1 (entry->cpu->gpr[9])
#define RY_T2 (entry->cpu->gpr[10])
#define RY_T3 (entry->cpu->gpr[11])
#define RY_T4 (entry->cpu->gpr[12])
#define RY_T5 (entry->cpu->gpr[13])
#define RY_T6 (entry->cpu->gpr[14])
#define RY_T7 (entry->cpu->gpr[15])
#define RY_T8 (entry->cpu->gpr[24])
#define RY_T9 (entry->cpu->gpr[25])
#define RY_LO (entry->cpu->lo)
static void battle_rot_matrix_yxz_mult(BattleEntryCall *entry,uint32_t a,uint32_t b)
{
    uint64_t product=(uint64_t)a*b;
    entry->cpu->lo=(uint32_t)product;entry->cpu->hi=(uint32_t)(product>>32);
}
#define RY_MULT(a,b) battle_rot_matrix_yxz_mult(entry,(a),(b))
#define RY_ASR(v) ((uint32_t)((int32_t)(v)>>12))
static void battle_rot_matrix_yxz_run(BattleEntryCall *entry)
{
    uint32_t p=RY_H(RY_A0);RY_V0=RY_A1;RY_T7=(uint32_t)(int32_t)(int16_t)p;
    int negative=(int32_t)RY_T7<0;RY_T9=RY_T7&0xfffu;
    if(negative){
        RY_T7=0u-RY_T7;RY_T7&=0xfffu;RY_T8=RY_T7<<2;
        RY_T9=0x80050000u;RY_T9+=RY_T8;RY_T9=RY_R(RY_T9+0x23f0u);
        RY_T6=RY_T9<<16;RY_T6=(uint32_t)((int32_t)RY_T6>>16);RY_T3=0u-RY_T6;
        RY_T0=(uint32_t)((int32_t)RY_T9>>16);
    }else{
        RY_T8=RY_T9<<2;RY_T9=0x80050000u;RY_T9+=RY_T8;RY_T9=RY_R(RY_T9+0x23f0u);
        RY_T8=RY_T9<<16;RY_T3=(uint32_t)((int32_t)RY_T8>>16);RY_T6=0u-RY_T3;
        RY_T0=(uint32_t)((int32_t)RY_T9>>16);
    }
    p=RY_H(RY_A0+2u);RY_T7=(uint32_t)(int32_t)(int16_t)p;
    negative=(int32_t)RY_T7<0;RY_T9=RY_T7&0xfffu;
    if(negative){
        RY_T7=0u-RY_T7;RY_T7&=0xfffu;RY_T8=RY_T7<<2;
        RY_T9=0x80050000u;RY_T9+=RY_T8;RY_T9=RY_R(RY_T9+0x23f0u);
        RY_T8=RY_T9<<16;RY_T8=(uint32_t)((int32_t)RY_T8>>16);RY_T4=0u-RY_T8;
        RY_T1=(uint32_t)((int32_t)RY_T9>>16);
    }else{
        RY_T8=RY_T9<<2;RY_T9=0x80050000u;RY_T9+=RY_T8;RY_T9=RY_R(RY_T9+0x23f0u);
        RY_T8=RY_T9<<16;RY_T4=(uint32_t)((int32_t)RY_T8>>16);RY_T1=(uint32_t)((int32_t)RY_T9>>16);
    }
    RY_MULT(RY_T4,RY_T0);
    p=RY_H(RY_A0+4u);RY_W(RY_A1+0xau,RY_T6);RY_T7=(uint32_t)(int32_t)(int16_t)p;
    RY_T8=RY_LO;RY_T6=RY_ASR(RY_T8);RY_MULT(RY_T1,RY_T0);RY_W(RY_A1+4u,RY_T6);
    negative=(int32_t)RY_T7<0;RY_T9=RY_T7&0xfffu;
    if(negative){
        RY_T8=RY_LO;RY_T6=RY_ASR(RY_T8);RY_W(RY_A1+0x10u,RY_T6);
        RY_T7=0u-RY_T7;RY_T7&=0xfffu;RY_T8=RY_T7<<2;
        RY_T9=0x80050000u;RY_T9+=RY_T8;RY_T9=RY_R(RY_T9+0x23f0u);
        RY_T8=RY_T9<<16;RY_T8=(uint32_t)((int32_t)RY_T8>>16);RY_T5=0u-RY_T8;
        RY_T2=(uint32_t)((int32_t)RY_T9>>16);
    }else{
        RY_T7=RY_LO;RY_T6=RY_ASR(RY_T7);RY_W(RY_A1+0x10u,RY_T6);
        RY_T8=RY_T9<<2;RY_T9=0x80050000u;RY_T9+=RY_T8;RY_T9=RY_R(RY_T9+0x23f0u);
        RY_T8=RY_T9<<16;RY_T5=(uint32_t)((int32_t)RY_T8>>16);RY_T2=(uint32_t)((int32_t)RY_T9>>16);
    }
    RY_MULT(RY_T5,RY_T0);RY_T7=RY_LO;RY_T6=RY_ASR(RY_T7);RY_W(RY_A1+6u,RY_T6);
    RY_MULT(RY_T2,RY_T0);RY_T7=RY_LO;RY_T6=RY_ASR(RY_T7);
    RY_MULT(RY_T4,RY_T3);RY_W(RY_A1+8u,RY_T6);RY_T7=RY_LO;RY_T8=RY_ASR(RY_T7);
    RY_MULT(RY_T8,RY_T5);RY_T7=RY_LO;RY_T6=RY_ASR(RY_T7);
    RY_MULT(RY_T1,RY_T2);RY_T7=RY_LO;RY_T9=RY_ASR(RY_T7);RY_T7=RY_T9+RY_T6;
    RY_MULT(RY_T1,RY_T5);RY_W(RY_A1,RY_T7);RY_T6=RY_LO;RY_T9=RY_ASR(RY_T6);RY_T7=0u-RY_T9;
    RY_MULT(RY_T8,RY_T2);RY_T6=RY_LO;RY_T9=RY_ASR(RY_T6);RY_T6=RY_T7+RY_T9;
    RY_MULT(RY_T1,RY_T3);RY_W(RY_A1+2u,RY_T6);RY_T7=RY_LO;RY_T8=RY_ASR(RY_T7);
    RY_MULT(RY_T8,RY_T2);RY_T7=RY_LO;RY_T6=RY_ASR(RY_T7);
    RY_MULT(RY_T4,RY_T5);RY_T7=RY_LO;RY_T9=RY_ASR(RY_T7);RY_T7=RY_T9+RY_T6;
    RY_MULT(RY_T4,RY_T2);RY_W(RY_A1+0xeu,RY_T7);RY_T6=RY_LO;RY_T9=RY_ASR(RY_T6);RY_T7=0u-RY_T9;
    RY_MULT(RY_T8,RY_T5);RY_T6=RY_LO;RY_T9=RY_ASR(RY_T6);RY_T6=RY_T7+RY_T9;RY_W(RY_A1+0xcu,RY_T6);
}
static int battle_rot_matrix_yxz_try_host(BattleMipsRuntime *runtime,
                                          PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x8004a92cu||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ROT_MATRIX_YXZ");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    const size_t offset=XENO_RD_SLUS_OFF(0x8004a92cu);
    if(!image||image->size<offset+652u)return 0;
    const char *expected="a4afe25431565c18c6a9af2cb26adb1685a61e53b68ccc68a8a73407a4e37ad0";
    uint8_t hash[32];file1_digest(image->bytes+offset,652u,hash);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(hash[i]>>4)!=expected[i*2]||file1_hex_digit(hash[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_rot_matrix_yxz_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef RY_H
#undef RY_R
#undef RY_W
#undef RY_V0
#undef RY_A0
#undef RY_A1
#undef RY_T0
#undef RY_T1
#undef RY_T2
#undef RY_T3
#undef RY_T4
#undef RY_T5
#undef RY_T6
#undef RY_T7
#undef RY_T8
#undef RY_T9
#undef RY_LO
#undef RY_MULT
#undef RY_ASR
