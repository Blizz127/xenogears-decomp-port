/* Resident startup-module resource initialization (801E7210). Resource offsets,
 * ownership, actor descriptors and guest frames retain their original word
 * widths. External callees remain independent native/refusal boundaries. */
#define RI_R8(a) battle_entry_read(entry,(a),1)
#define RI_R16(a) battle_entry_read(entry,(a),2)
#define RI_RS16(a) ((uint32_t)(int32_t)(int16_t)RI_R16(a))
#define RI_R32(a) battle_entry_read(entry,(a),4)
#define RI_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define RI_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define RI_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define RI_CALL(target,site,...) battle_entry_call(entry,(target),(site), \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t),(uint32_t[]){__VA_ARGS__})
#define RI_V0 (entry->cpu->gpr[2])
#define RI_V1 (entry->cpu->gpr[3])
#define RI_A0 (entry->cpu->gpr[4])
#define RI_A1 (entry->cpu->gpr[5])
#define RI_A2 (entry->cpu->gpr[6])
#define RI_A3 (entry->cpu->gpr[7])
#define RI_T0 (entry->cpu->gpr[8])
#define RI_T1 (entry->cpu->gpr[9])
#define RI_S0 (entry->cpu->gpr[16])
#define RI_S1 (entry->cpu->gpr[17])
#define RI_S2 (entry->cpu->gpr[18])
#define RI_S3 (entry->cpu->gpr[19])
#define RI_S4 (entry->cpu->gpr[20])
#define RI_S5 (entry->cpu->gpr[21])
#define RI_S6 (entry->cpu->gpr[22])
#define RI_S7 (entry->cpu->gpr[23])
#define RI_FP (entry->cpu->gpr[30])

static void battle_resource_init_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0xa0u;entry->cpu->gpr[29]=sp;
    RI_W32(sp+0x94u,RI_S7);RI_W32(sp+0x9cu,entry->cpu->gpr[31]);RI_W32(sp+0x98u,RI_FP);
    for(unsigned i=0;i<7;i++) RI_W32(sp+0x90u-i*4u,entry->cpu->gpr[22-i]);
    RI_W32(sp+0x50u,RI_A0);RI_W32(sp+0x58u,RI_A2);
    /* The original dereferences a0 before checking either early-out input. */
    RI_S6=RI_R32(RI_A0);RI_S7=RI_A3;
    if(!RI_A2 || !RI_A0 || !RI_S6) {RI_V0=0;goto done;}
    RI_CALL(0x80032498u,0x801e7268u,4,0);
    RI_S1=1;RI_V0=0x800c3d54u;
    RI_W32(0x800c3e38u,0);RI_W32(0x800c3ea0u,0);RI_W32(0x800d3344u,0);
    RI_W32(0x800d39ccu,0);RI_W32(0x800d3348u,0);
    do {RI_W32(RI_V0,0);RI_S1--;RI_V0-=4u;} while((int32_t)RI_S1>=0);
    RI_V0=24;
    do {RI_W32(0x800c3db4u+RI_V0,0);RI_V0-=24u;} while((int32_t)RI_V0>=0);
    RI_T1=RI_R32(sp+0x58u);RI_W16(0x800d361au,0);RI_S3=RI_S6;
    if(RI_T1) {
        RI_A0=RI_R32(sp+0x58u);RI_S1=1;
        RI_CALL(0x800320b8u,0x801e72e8u,RI_A0);
        RI_A2=RI_R32(sp+0x58u);RI_A3=RI_R32(sp+0x58u);
        RI_CALL(0x800a8bf0u,0x801e7310u,31,0xc4,RI_A2,RI_A3,0,0,0,0,0);
        RI_T1=RI_R32(sp+0x58u);
        RI_CALL(0x801e70e8u,0x801e7324u,RI_R32(RI_T1+4u));
        RI_T1=RI_R32(sp+0x58u);RI_V0=RI_R32(0x800d33e4u);
        RI_A1=RI_R32(RI_T1+0x14u);RI_A2=RI_R32(RI_V0+4u);RI_V1=RI_R32(RI_V0);
        RI_V0=RI_R16(RI_A2+10u);RI_W32(0x800c3e38u,RI_A2);RI_W32(0x800c3e48u,RI_V1);
        RI_V0=(int32_t)RI_S1<(int32_t)RI_V0;RI_A0=RI_A1+6u;
        if(RI_V0) {
            RI_V1=RI_A2+124u;
            do {
                RI_V0=RI_RS16(RI_A1);RI_W32(RI_V1+0x5cu,RI_V0);
                RI_V0=RI_RS16(RI_A0-4u);RI_S1++;RI_W32(RI_V1+0x60u,RI_V0);
                RI_V0=RI_RS16(RI_A0-2u);RI_A1+=8u;RI_W32(RI_V1+0x64u,RI_V0);
                RI_V0=RI_R16(RI_A0);RI_A0+=8u;RI_W16(RI_V1+0x52u,RI_V0);
                RI_V0=RI_R16(RI_A2+10u);RI_V0=(int32_t)RI_S1<(int32_t)RI_V0;RI_V1+=124u;
            } while(RI_V0);
        }
        RI_S3=RI_S6;
    }
    RI_S0=RI_R32(RI_S3-4u);
    RI_CALL(0x80031bdcu,0x801e73b8u,RI_S0,0);
    RI_S6=RI_V0;RI_W32(0x800658c8u,RI_S6);RI_A0=RI_S6;
    if(!RI_S6) goto done;
    RI_CALL(0x8003f968u,0x801e73d8u,RI_S6,RI_S3,RI_S0);
    RI_S0=RI_S3-4u;
    RI_CALL(0x800320b8u,0x801e73e4u,RI_S0);
    RI_CALL(0x800320e8u,0x801e73ecu,RI_S0);
    RI_S5=0;RI_S0=RI_R32(RI_S6+0x50cu);RI_FP=0;
    RI_S2=RI_S6+RI_S0;RI_V0=0u-(RI_S0!=0);RI_S0=RI_R32(RI_S6+0x510u);
    RI_S2&=RI_V0;RI_W32(sp+0x68u,RI_S2);
    RI_V1=RI_S6+RI_S0;RI_S2=RI_V1+4u;RI_V0=0u-(RI_S0!=0);
    RI_S0=RI_R32(RI_S6+0x514u);RI_S2&=RI_V0;RI_W32(sp+0x70u,RI_S2);RI_W32(sp+0x60u,RI_V1);
    RI_S2=RI_S6+RI_S0;
    RI_CALL(0x8003342cu,0x801e743cu,RI_S2);
    RI_T1=RI_R32(sp+0x58u);RI_S3=RI_R32(RI_S2+4u);RI_S4=RI_S6+0x340u;
    if(RI_T1) {
        RI_S1=0x800c3d0cu;RI_S0=0x800d33e4u;
        RI_CALL(0x800aa898u,0x801e7470u,RI_R32(RI_S0),RI_S1,RI_S2+8u,0);
        RI_A0=RI_R32(RI_S0);
        RI_CALL(0x800aa934u,0x801e7484u,RI_A0,RI_A0,RI_S1,0);
        RI_V0=RI_R32(0x800d33e4u);RI_A0=RI_R32(0x800c3e38u);
        RI_CALL(0x8009ef3cu,0x801e74a0u,RI_A0,RI_RS16(RI_V0+0x1cu));
    }
    RI_S1=0;RI_V1=sp+0x40u;
    do {
        RI_S1++;RI_S0=RI_R32(RI_S3);RI_V0=(int32_t)RI_S1<4;
        RI_S2=RI_S3+RI_S0;RI_W32(RI_V1,RI_S2);RI_V1+=4u;
    } while(RI_V0);
    RI_V1=RI_R32(sp+0x40u);RI_V0=RI_R16(RI_S6+0x464u);RI_A1=RI_R16(RI_V1);
    RI_W16(RI_S7,RI_V0);RI_V0=RI_R16(RI_S6+0x466u);RI_W16(RI_S7+2u,RI_V0);
    RI_V0=RI_R16(RI_S6+0x468u);
    for(unsigned i=6;i<=16;i+=2) RI_W16(RI_S7+i,0);
    RI_W16(RI_S7+4u,RI_V0);
    RI_T1=RI_R32(sp+0xb0u);RI_V0=RI_R16(RI_S6+0x46cu);
    RI_W16(RI_T1+2u,0);RI_W16(RI_T1+4u,0);RI_W16(RI_T1,RI_V0);
    RI_A0=RI_R32(sp+0xb0u);RI_V0=RI_R16(RI_S6+0x46eu);
    RI_W16(RI_A0+8u,0);RI_W16(RI_A0+10u,0);RI_W16(RI_A0+6u,RI_V0);
    RI_V0=RI_R16(RI_S6+0x470u);RI_V1+=2u;
    RI_W32(0x800d2fd0u,RI_V1);RI_W16(0x800d2fc8u,RI_A1);RI_W32(0x800d2fc0u,RI_A0);
    RI_W16(RI_A0+14u,0);RI_W16(RI_A0+16u,0);RI_W16(RI_A0+12u,RI_V0);
    RI_CALL(0x80049f5cu,0x801e7558u,RI_A0);
    RI_A0=RI_R8(RI_S6+0x474u);RI_A1=RI_R8(RI_S6+0x475u);RI_A2=RI_R8(RI_S6+0x476u);RI_S1=0;
    RI_CALL(0x8004a0ecu,0x801e756cu,RI_A0,RI_A1,RI_A2);
    RI_S2=RI_FP*24u;
    do {
        RI_T0=RI_S4+RI_S1*40u+32u;RI_V1=RI_R16(RI_T0+24u);
        RI_V0=RI_V1<<2;
        /* The complete resident dispatch-table identity is checked before
         * entering; no native address is loaded from game memory. */
        switch(RI_V1) {
        case 1:case 2: {
            uint32_t kind=RI_V1;RI_V1=RI_S5*4u;RI_S0=0x800c3d50u+RI_V1;
            RI_V0=RI_R32(RI_S0);uint32_t occupied=RI_V0;RI_V0=(int32_t)RI_S5<2;
            if(!occupied && RI_V0) {
                RI_V0=RI_R16(RI_T0+26u);
                RI_A0=RI_R16(RI_T0+16u);RI_A1=RI_R16(RI_T0+18u);RI_A2=RI_R16(RI_T0+20u);RI_A3=RI_R16(RI_T0+22u);
                RI_W32(sp+0x10u,RI_V0);RI_V0=RI_R16(RI_T0+28u);RI_W32(sp+0x14u,RI_V0);
                RI_V0=RI_R16(RI_T0+30u);RI_W32(sp+0x18u,RI_V0);
                if(kind==1) {
                    RI_V0=RI_R16(RI_T0+32u);RI_W32(sp+0x20u,RI_T0);RI_W32(sp+0x24u,0);RI_W32(sp+0x28u,0);RI_W32(sp+0x1cu,RI_V0);
                } else {
                    RI_V1=RI_R16(RI_T0+32u);RI_V0=RI_S4+0x118u;
                    RI_W32(sp+0x20u,RI_T0);RI_W32(sp+0x24u,RI_V0);RI_W32(sp+0x1cu,RI_V1);
                    RI_V0=RI_RS16(RI_T0+34u);RI_W32(sp+0x28u,RI_V0);
                }
                RI_V0=RI_RS16(RI_T0+36u);RI_W32(sp+0x2cu,RI_V0);
                RI_V0=RI_RS16(RI_T0+38u);RI_W32(sp+0x30u,RI_V0);
                battle_entry_call(entry,0x8002709cu,0x801e76b8u,0,NULL);
                RI_W32(RI_S0,RI_V0);
            }
            RI_S5++;break;
        }
        case 3:
            RI_V0=RI_R32(0x800c3ea0u);
            if(!RI_V0) {
                RI_V0=RI_RS16(RI_T0+20u);RI_A0=RI_RS16(RI_T0+16u);RI_A1=RI_RS16(RI_T0+18u);
                RI_A2=RI_RS16(RI_S4+0x114u);RI_A3=RI_RS16(RI_S4+0x116u);RI_W32(sp+0x10u,RI_V0);
                const unsigned offsets[]={30,22,26,28};
                for(unsigned i=0;i<4;i++){RI_V0=RI_RS16(RI_T0+offsets[i]);RI_W32(sp+0x14u+i*4u,RI_V0);}
                RI_V0=RI_RS16(RI_S4+0x110u);RI_W32(sp+0x24u,RI_V0);
                RI_V1=RI_RS16(RI_S4+0x112u);RI_V0=RI_S4+0x118u;
                RI_W32(sp+0x2cu,RI_T0);RI_W32(sp+0x30u,RI_V0);RI_W32(sp+0x28u,RI_V1);
                RI_V0=RI_RS16(RI_T0+32u);RI_W32(sp+0x34u,RI_V0);RI_V0=RI_RS16(RI_T0+34u);RI_W32(sp+0x38u,RI_V0);
                battle_entry_call(entry,0x801e7914u,0x801e7758u,0,NULL);RI_W32(0x800c3ea0u,RI_V0);
            }
            break;
        case 5:RI_V0=1;RI_W8(RI_S4+30u,RI_V0);break;
        case 7: {
            RI_V0=RI_R32(0x800c3db4u+RI_S2);uint32_t occupied=RI_V0;RI_V0=(int32_t)RI_FP<2;
            if(!occupied && RI_V0) {
                RI_V0=(int32_t)(RI_S1+1u)<4;
                if(RI_V0) {
                    RI_V0=RI_RS16(RI_T0+22u);RI_A1=RI_RS16(RI_T0+16u);RI_A2=RI_RS16(RI_T0+18u);RI_A3=RI_RS16(RI_T0+20u);
                    RI_W32(sp+0x10u,RI_V0);RI_V0=RI_RS16(RI_T0+32u);RI_W32(sp+0x14u,RI_V0);
                    RI_V0=RI_RS16(RI_T0+26u);RI_W32(sp+0x18u,RI_V0);RI_V0=RI_RS16(RI_T0+28u);RI_W32(sp+0x1cu,RI_V0);
                    RI_V0=RI_R32(sp+RI_S1*4u+0x44u);RI_A0=0x800c3da0u+RI_S2;RI_W32(sp+0x20u,RI_V0);
                    battle_entry_call(entry,0x80027d64u,0x801e77f4u,0,NULL);
                }
            }
            RI_S2+=24u;RI_FP++;break;
        }
        default:break;
        }
        RI_S1++;RI_V0=(int32_t)RI_S1<6;
    } while(RI_V0);
    RI_S0=RI_R8(RI_S4+30u);
    if(RI_S0) {
        RI_T1=RI_R32(sp+0xb4u);
        if(RI_T1) for(unsigned i=0;i<3;i++){RI_V0=RI_R8(RI_S4+0x118u+i);RI_W8(RI_T1+i,RI_V0);}
    }
    RI_T1=RI_R32(sp+0x68u);RI_S1=0;
    if(RI_T1 && RI_R32(sp+0x70u)) {
        RI_A0=RI_R32(sp+0x68u);RI_T1=RI_R32(sp+0x60u);RI_A1=RI_R32(sp+0x70u);RI_A2=RI_R32(RI_T1);
        RI_CALL(0x801e7ec4u,0x801e7888u,RI_A0,RI_A1,RI_A2);
    }
    do {
        RI_V0=RI_R32(0x800658c8u);RI_V0=RI_R8(RI_V0+RI_S1+0x340u);RI_W8(0x800d2d10u+RI_S1,RI_V0);
        RI_S1++;RI_V0=(int32_t)RI_S1<4;
    } while(RI_V0);
    RI_CALL(0x800445d0u,0x801e78c0u,0);
    RI_CALL(0x800320e8u,0x801e78ccu,RI_R32(sp+0x58u));
    RI_T1=RI_R32(sp+0x50u);RI_V0=RI_S0;RI_W32(RI_T1,RI_S6);
done:
    entry->cpu->gpr[31]=RI_R32(sp+0x9cu);RI_FP=RI_R32(sp+0x98u);
    for(unsigned i=0;i<8;i++) entry->cpu->gpr[23-i]=RI_R32(sp+0x94u-i*4u);
    entry->cpu->gpr[29]=sp+0xa0u;
}

static int battle_resource_init_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x801e7210u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_RESOURCE_INIT");if(control && strcmp(control,"0")==0)return 0;
    static const struct {uint32_t address,size;const char *sha;} regions[]={
        {0x801e7210u,1796,"4a57dcf9aa943e38add1572724c70962982e714d422ca56346cb3ba5715ed87e"},
        {0x801e4000u,32,"6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140"},
        {0x801e5840u,228,"e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2"}};
    for(unsigned r=0;r<sizeof(regions)/sizeof(regions[0]);r++) {
        uint8_t digest[32];file1_digest(PSX_ADDR(regions[r].address),regions[r].size,digest);
        for(unsigned i=0;i<sizeof(digest);i++)
            if(file1_hex_digit(digest[i]>>4)!=regions[r].sha[i*2] || file1_hex_digit(digest[i]&15u)!=regions[r].sha[i*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_resource_init_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef RI_R8
#undef RI_R16
#undef RI_RS16
#undef RI_R32
#undef RI_W8
#undef RI_W16
#undef RI_W32
#undef RI_CALL
#undef RI_V0
#undef RI_V1
#undef RI_A0
#undef RI_A1
#undef RI_A2
#undef RI_A3
#undef RI_T0
#undef RI_T1
#undef RI_S0
#undef RI_S1
#undef RI_S2
#undef RI_S3
#undef RI_S4
#undef RI_S5
#undef RI_S6
#undef RI_S7
#undef RI_FP
