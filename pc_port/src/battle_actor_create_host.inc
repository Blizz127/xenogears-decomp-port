/* Actor construction (800A8BF0). Packed guest records and the original frame
 * remain word-addressed; nested services are independent native/refusal paths. */
#define AC_R8(a) battle_entry_read(entry,(a),1)
#define AC_R16(a) battle_entry_read(entry,(a),2)
#define AC_RS16(a) ((uint32_t)(int32_t)(int16_t)AC_R16(a))
#define AC_R32(a) battle_entry_read(entry,(a),4)
#define AC_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define AC_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define AC_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define AC_CALL(target,site,...) battle_actor_create_call(entry,(target),(site), \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t),(uint32_t[]){__VA_ARGS__})
#define AC_V0 (entry->cpu->gpr[2])
#define AC_V1 (entry->cpu->gpr[3])
#define AC_A0 (entry->cpu->gpr[4])
#define AC_A1 (entry->cpu->gpr[5])
#define AC_A2 (entry->cpu->gpr[6])
#define AC_A3 (entry->cpu->gpr[7])
#define AC_T0 (entry->cpu->gpr[8])
#define AC_S0 (entry->cpu->gpr[16])
#define AC_S1 (entry->cpu->gpr[17])
#define AC_S2 (entry->cpu->gpr[18])
#define AC_S3 (entry->cpu->gpr[19])
#define AC_S4 (entry->cpu->gpr[20])
#define AC_S5 (entry->cpu->gpr[21])
#define AC_S6 (entry->cpu->gpr[22])
#define AC_S7 (entry->cpu->gpr[23])
#define AC_FP (entry->cpu->gpr[30])

static uint32_t battle_actor_create_call(BattleEntryCall *entry,uint32_t target,
                                         uint32_t site,unsigned argc,const uint32_t *args)
{
    /* JAL has already committed its link and argument registers when a nested
     * service refuses. Preserve that caller state even though the shared call
     * helper intentionally commits callee registers only after success. */
    entry->cpu->gpr[31]=site+8u;
    for(unsigned i=0;i<argc&&i<4;i++)entry->cpu->gpr[4+i]=args[i];
    return battle_entry_call(entry,target,site,argc,args);
}

static void battle_actor_create_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0xc8u;entry->cpu->gpr[29]=sp;
    AC_W32(sp+0xbcu,AC_S7);AC_S7=AC_R32(sp+0xe8u);AC_T0=AC_R16(sp+0xd8u);
    AC_W32(sp+0xa0u,AC_S0);AC_S0=AC_A1;AC_W32(sp+0xa8u,AC_S2);
    AC_W16(sp+0x60u,AC_T0);AC_T0=AC_R16(sp+0xdcu);AC_S2=AC_A3;
    AC_W32(sp+0x50u,AC_A0);AC_W16(sp+0x68u,AC_T0);AC_T0=AC_R16(sp+0xe0u);
    AC_A0=4;AC_W16(sp+0x70u,AC_T0);AC_T0=AC_R16(sp+0xe4u);AC_A1=0;
    AC_W32(sp+0xc4u,entry->cpu->gpr[31]);AC_W32(sp+0xc0u,AC_FP);
    for(unsigned i=0;i<4;i++)AC_W32(sp+0xb8u-i*4u,entry->cpu->gpr[22-i]);
    AC_W32(sp+0xa4u,AC_S1);AC_W32(sp+0x58u,AC_A2);AC_W16(sp+0x78u,AC_T0);
    AC_CALL(0x80032498u,0x800a8c54u,4,0);
    AC_T0=AC_R32(sp+0x50u);AC_V0=(int32_t)AC_T0<32;AC_FP=AC_S0;
    if(!AC_V0)goto done;
    AC_V0=AC_T0<<2;AC_V0=AC_R32(0x800d3368u+AC_V0);AC_A0=0x11c;
    if(AC_V0)goto done;
    AC_CALL(0x80031bdcu,0x800a8c8cu,0x11c,0);
    AC_S3=AC_V0;AC_V0=AC_S0&1u;
    uint32_t existing_model=AC_V0;AC_V0=AC_S0&4u;
    if(!existing_model){
        AC_CALL(0x8003342cu,0x800a8ca4u,AC_S2);
        AC_A0=AC_R32(AC_S2+16u);AC_CALL(0x8003342cu,0x800a8cb0u,AC_A0);
        AC_V0=AC_S0&4u;
    }
    AC_W8(AC_S3+0x62u,0);AC_W8(AC_S3+0x63u,0);
    if(!AC_V0){
        AC_A0=AC_R32(sp+0x58u);AC_CALL(0x8003342cu,0x800a8cccu,AC_A0);
        AC_T0=AC_R32(sp+0x58u);AC_A0=AC_R32(AC_T0+8u);AC_CALL(0x8003342cu,0x800a8ce0u,AC_A0);
        AC_T0=AC_R32(sp+0x58u);AC_S1=AC_R32(AC_T0+4u);AC_CALL(0x8003342cu,0x800a8cf4u,AC_S1);
        AC_A0=AC_R32(AC_S1+4u);AC_CALL(0x8003342cu,0x800a8d00u,AC_A0);
        AC_T0=AC_R32(sp+0x58u);AC_S0=AC_R32(AC_T0+8u);
        AC_V0=AC_R32(AC_S0+12u);AC_A0=AC_R32(AC_S0+8u);
        if(AC_V0!=AC_A0){
            AC_CALL(0x8003864cu,0x800a8d2cu,AC_A0,0);
            if(!AC_V0){AC_A0=AC_R32(AC_S0+8u);AC_CALL(0x80038428u,0x800a8d40u,AC_A0);AC_V0=1;AC_W8(AC_S3+0x62u,AC_V0);}
        }
    }
    AC_T0=AC_R32(AC_S2+16u);AC_W32(sp+0x80u,AC_T0);AC_S1=AC_R32(AC_T0+4u);
    AC_S5=AC_R32(AC_S2+4u);AC_S6=AC_R32(AC_S2+8u);AC_T0=AC_R32(sp+0x50u);AC_S0=AC_R32(AC_S2+12u);
    AC_V0=AC_T0<<2;AC_W32(0x800d3368u+AC_V0,AC_S3);
    for(unsigned i=0;i<3;i++){AC_V0=AC_R16(AC_S1+2u+i*2u);AC_W16(AC_S3+0x24u+i*2u,AC_V0);}
    AC_V0=AC_R8(AC_S1+10u);AC_W8(AC_S3+0x2au,AC_V0);
    AC_V0=AC_R16(AC_S1+12u);AC_W16(AC_S3+0x4au,AC_V0);AC_V0&=0x200u;AC_S2=AC_S0-AC_S6;
    if(AC_V0)AC_CALL(0x80030988u,0x800a8dd4u,2,2,0x40,0x40);
    AC_V0=AC_FP&1u;existing_model=AC_V0;AC_V0=AC_FP&0x40u;
    if(!existing_model){
        AC_S4=0;
        if(!AC_V0){AC_V0=AC_R16(AC_S3+0x4au)&4u;AC_S4=AC_V0<1;}
        AC_A0=AC_S5;AC_A1=(uint32_t)(int32_t)(int16_t)AC_S4;
        AC_T0=AC_R16(sp+0x60u);AC_W32(sp+0x10u,AC_A1);AC_A2=(uint32_t)(int32_t)(int16_t)AC_T0;
        AC_T0=AC_R16(sp+0x68u);AC_A3=(uint32_t)(int32_t)(int16_t)AC_T0;
        AC_T0=AC_R16(sp+0x70u);AC_V0=(uint32_t)(int32_t)(int16_t)AC_T0;
        AC_T0=AC_R16(sp+0x78u);AC_W32(sp+0x14u,AC_V0);AC_V0=(uint32_t)(int32_t)(int16_t)AC_T0;
        AC_W32(sp+0x18u,AC_V0);battle_actor_create_call(entry,0x8002dde4u,0x800a8e44u,0,NULL);
        AC_CALL(0x80031bdcu,0x800a8e50u,AC_S2,1);
        AC_A0=AC_V0;AC_A1=AC_S6;AC_W32(0x800c3b70u,AC_A0);AC_CALL(0x8003f968u,0x800a8e68u,AC_A0,AC_A1,AC_S2);
        AC_W32(0x800c3b6cu,0);
        for(;;){
            AC_V1=AC_R32(0x800c3b6cu);AC_V0=AC_V1<<3;AC_V0=AC_R32(0x800c3accu+AC_V0);
            uint32_t occupied=AC_V0;AC_V0=AC_V1+1u;if(!occupied)break;
            AC_W32(0x800c3b6cu,AC_V0);AC_V0=(int32_t)AC_V0<20;if(!AC_V0)break;
        }
        AC_V0=0x800c3accu;AC_A1=AC_R32(0x800c3b6cu);AC_A0=AC_R32(0x800c3b70u);AC_A1=(AC_A1<<3)+AC_V0;
        AC_CALL(0x8009eba8u,0x800a8ed0u,AC_A0,AC_A1);
    }
    AC_V0=AC_R32(0x800c3b6cu);AC_V1=0x800c3accu;AC_V0=(AC_V0<<3)+AC_V1;AC_W32(AC_S3,AC_V0);
    AC_V0=AC_FP&0x40u;AC_A2=0;AC_A1=AC_S0;
    uint32_t positioned=0;
    if(!AC_V0){AC_V0=AC_R16(AC_S3+0x4au)&4u;AC_A2=2;positioned=!AC_V0;}
    if(positioned){
        for(unsigned i=0;i<4;i++){AC_T0=AC_R16(sp+0x60u+i*8u);AC_V0=(uint32_t)(int32_t)(int16_t)AC_T0;AC_W32(sp+0x10u+i*4u,AC_V0);}
        AC_A0=AC_R32(AC_S3);AC_A3=1;
    }else{for(unsigned i=0;i<4;i++)AC_W32(sp+0x10u+i*4u,0);AC_A0=AC_R32(AC_S3);AC_A3=0;}
    battle_actor_create_call(entry,0x8009ec4cu,0x800a8f88u,0,NULL);AC_W32(AC_S3+4u,AC_V0);
    if(AC_S7){for(unsigned i=0;i<3;i++){
        uint32_t destination=AC_R32(AC_S3+4u);
        /* A fault in the immediately following position read precedes the
         * original delayed destination load's register commit. */
        AC_V0=AC_RS16(AC_S7+i*2u);AC_V1=destination;AC_W32(AC_V1+0x5cu+i*4u,AC_V0);
    }}
    AC_V0=AC_FP&1u;existing_model=AC_V0;AC_V0=UINT32_MAX;
    if(!existing_model){for(unsigned i=0;i<4;i++){AC_T0=AC_R16(sp+0x60u+i*8u);AC_W16(AC_S3+0x90u+i*2u,AC_T0);}}
    else AC_W16(AC_S3+0x90u,AC_V0);
    AC_V0=AC_FP&4u;uint32_t no_sound=AC_V0;AC_V0=AC_FP&0x80u;
    if(!no_sound&&!AC_V0){AC_T0=AC_R32(sp+0x58u);AC_W32(AC_S3+0xacu,AC_T0);}else AC_W32(AC_S3+0xacu,0);
    AC_S4=0;AC_S7=0xc0;AC_S6=0xfe;AC_S2=AC_S3;AC_S5=0xb8;
    do{
        AC_S0=AC_S3+AC_S5;AC_CALL(0x80043cb0u,0x800a9044u,AC_S0);AC_CALL(0x80043bfcu,0x800a9050u,AC_S0,1);
        AC_V0=AC_R32(0x800658c8u);AC_V0=AC_R8(AC_V0+0x478u);AC_W8(AC_S2+0xbcu,AC_V0);
        AC_V0=AC_R32(0x800658c8u);AC_A0=0x30;AC_V0=AC_R8(AC_V0+0x479u);AC_A1=0x1cc;AC_W8(AC_S2+0xbdu,AC_V0);
        AC_V0=AC_R32(0x800658c8u);AC_S5+=0x28u;AC_V0=AC_R8(AC_V0+0x47au);AC_S4++;AC_W8(AC_S2+0xbeu,AC_V0);
        AC_CALL(0x80043a58u,0x800a909cu,AC_A0,AC_A1);
        AC_A0=0;AC_A1=2;AC_A2=0x380;AC_A3=0;AC_W16(AC_S2+0xc6u,AC_V0);AC_CALL(0x80043a1cu,0x800a90b4u,0,2,0x380,0);
        AC_W16(AC_S2+0xceu,AC_V0);AC_W8(AC_S2+0xc4u,AC_S7);AC_W8(AC_S2+0xc5u,AC_S7);
        AC_W8(AC_S2+0xccu,AC_S6);AC_W8(AC_S2+0xcdu,AC_S7);AC_W8(AC_S2+0xd4u,AC_S7);
        AC_W8(AC_S2+0xd5u,AC_S6);AC_W8(AC_S2+0xdcu,AC_S6);AC_W8(AC_S2+0xddu,AC_S6);
        AC_V0=(int32_t)AC_S4<2;AC_S2+=0x28u;
    }while(AC_V0);
    AC_V0=AC_FP&0x40u;
    if(!AC_V0){
        AC_V0=AC_R32(0x800658c8u);AC_V1=AC_RS16(AC_S1+8u);AC_V0=AC_RS16(AC_V0+0x344u);
        int64_t product=(int64_t)(int32_t)AC_V1*(int32_t)AC_V0;
        entry->cpu->lo=(uint32_t)product;entry->cpu->hi=(uint32_t)((uint64_t)product>>32);
        AC_V0=(uint32_t)((int32_t)entry->cpu->lo>>12);
    }else AC_V0=AC_R16(AC_S1+8u);
    AC_W16(AC_S3+0x1cu,AC_V0);AC_V0=AC_R8(AC_S1+14u);AC_A0=AC_S3;AC_W8(AC_S3+0x10cu,AC_V0);
    AC_CALL(0x800aa6e0u,0x800a9130u,AC_A0);
    AC_V0=AC_R8(AC_S1+16u);AC_W8(AC_S3+0x10eu,AC_V0);AC_V0=AC_R8(AC_S3+0x10eu);AC_A0=AC_V0<<1;
    if(AC_V0){
        AC_A0=(AC_A0+AC_V0)<<4;AC_CALL(0x80031bdcu,0x800a915cu,AC_A0,0);
        AC_V1=AC_R8(AC_S3+0x10eu);AC_S4=0;AC_W32(AC_S3+0x118u,AC_V0);
        if((int32_t)AC_V1>0){
            AC_V1=0;
            do{
                AC_V0=AC_R32(AC_S3+0x118u)+AC_V1;AC_W16(AC_V0+0x1au,0);
                AC_V0=AC_R32(AC_S3+0x118u)+AC_V1;AC_W32(AC_V0+4u,0);
                AC_V0=AC_R32(AC_S3+0x118u)+AC_V1;AC_W32(AC_V0+8u,0);
                AC_V0=AC_R32(AC_S3+0x118u);AC_S4++;AC_V0+=AC_V1;AC_W32(AC_V0+12u,0);
                AC_V0=AC_R8(AC_S3+0x10eu);AC_V0=(int32_t)AC_S4<(int32_t)AC_V0;AC_V1+=0x30u;
            }while(AC_V0);
        }
    }
    AC_V0=AC_R8(AC_S1+18u);AC_W8(AC_S3+0x10du,AC_V0);AC_V0=AC_R8(AC_S3+0x10du);
    if(AC_V0){
        AC_S1+=20u;AC_A0=((AC_V0<<3)+AC_V0)<<2;AC_CALL(0x80031bdcu,0x800a91f8u,AC_A0,0);
        AC_S2=AC_V0;AC_V0=AC_R8(AC_S3+0x10du);AC_S4=0;AC_W32(AC_S3+0x114u,AC_S2);
        if((int32_t)AC_V0>0){
            AC_S0=AC_S2+24u;AC_S6=AC_R32(sp+0x80u);
            do{
                AC_S5=AC_RS16(AC_S1+0x22u);AC_V0=AC_R16(AC_S1);AC_S1+=2u;AC_W16(AC_S2,AC_V0);
                AC_A2=AC_RS16(AC_S1);AC_S1+=2u;AC_A3=AC_RS16(AC_S1);AC_S1+=2u;
                AC_V0=AC_RS16(AC_S1);AC_T0=AC_R16(sp+0x60u);AC_S1+=2u;AC_W32(sp+0x10u,AC_V0);
                AC_V0=AC_RS16(AC_S1);AC_S1+=2u;AC_W32(sp+0x14u,AC_V0);
                AC_V0=AC_RS16(AC_S1);AC_S1+=2u;AC_W32(sp+0x1cu,AC_S5);AC_W32(sp+0x18u,AC_V0);
                AC_V0=AC_R16(AC_S1);AC_S1+=2u;AC_V0=(uint32_t)(int32_t)(int16_t)(AC_T0+AC_V0);AC_W32(sp+0x20u,AC_V0);
                AC_V0=AC_R16(AC_S1);AC_T0=AC_R16(sp+0x68u);AC_S1+=2u;AC_V0=(uint32_t)(int32_t)(int16_t)(AC_T0+AC_V0);AC_W32(sp+0x24u,AC_V0);
                AC_V0=AC_RS16(AC_S1);AC_T0=AC_R16(sp+0x78u);AC_S1+=2u;AC_W32(sp+0x28u,AC_V0);
                AC_V0=AC_RS16(AC_S1);AC_S1+=2u;AC_W32(sp+0x2cu,AC_V0);
                AC_V1=AC_R16(AC_S1);AC_S1+=2u;AC_V0=(uint32_t)(int32_t)(int16_t)AC_T0;AC_T0=AC_R16(sp+0x70u);
                AC_W32(sp+0x34u,AC_V0);AC_V1=(uint32_t)(int32_t)(int16_t)(AC_T0+AC_V1);AC_W32(sp+0x30u,AC_V1);
                for(unsigned i=0;i<5;i++){AC_V0=AC_R8(AC_S1);AC_S1+=2u;AC_W32(sp+0x38u+i*4u,AC_V0);}
                AC_V0=AC_R8(AC_S1);AC_A0=AC_S2;AC_W32(sp+0x4cu,AC_V0);AC_A1=AC_R32(AC_S6+8u);AC_S1+=4u;
                battle_actor_create_call(entry,0x800a7064u,0x800a932cu,0,NULL);AC_A1=0;
                if((int32_t)AC_S5>0){
                    do{
                        AC_A0=AC_R16(AC_S1);AC_S1+=2u;AC_V1=AC_A1<<4;AC_V0=AC_R32(AC_S0);AC_A1++;AC_V0+=AC_V1;AC_W16(AC_V0+6u,AC_A0);
                        const unsigned offsets[]={14,0,2};
                        for(unsigned i=0;i<3;i++){AC_V0=AC_R32(AC_S0);AC_A0=AC_R16(AC_S1);AC_S1+=2u;AC_V0+=AC_V1;AC_W16(AC_V0+offsets[i],AC_A0);}
                        AC_V0=AC_R32(AC_S0);AC_A0=AC_R16(AC_S1);AC_S1+=2u;AC_V1+=AC_V0;AC_V0=(int32_t)AC_A1<(int32_t)AC_S5;AC_W16(AC_V1+4u,AC_A0);
                    }while(AC_V0);
                }
                AC_S6+=4u;AC_S4++;AC_S0+=0x24u;AC_V0=AC_R8(AC_S3+0x10du);AC_V0=(int32_t)AC_S4<(int32_t)AC_V0;AC_S2+=0x24u;
            }while(AC_V0);
        }
    }
    AC_T0=AC_R8(sp+0x50u);AC_W8(AC_S3+0x22u,0);AC_W8(AC_S3+0x20u,AC_T0);AC_T0=AC_R32(sp+0x50u);
    AC_V0=((AC_T0<<3)-AC_T0)<<2;AC_V0=AC_R8(0x800c3eb7u+AC_V0);uint32_t special=AC_V0;
    AC_V0=(int32_t)AC_T0<11;uint32_t in_party=AC_V0;AC_V0=1;
    AC_W8(AC_S3+0x34u,special&&in_party?0:AC_V0);
    AC_V0=AC_FP&0x40u;uint32_t no_links=AC_V0;AC_V0=AC_FP&2u;
    if(!no_links){
        AC_A0=AC_S3;AC_T0=AC_R32(sp+0x58u);AC_S0=0x800c3d0cu;AC_S1=AC_R32(AC_T0+4u);
        AC_V0=AC_R32(AC_T0+8u);AC_A1=AC_S0;AC_W32(AC_S3+0xb0u,AC_V0);AC_A3=AC_R32(AC_S1+4u);AC_A2=AC_S1+8u;
        AC_CALL(0x800aa898u,0x800a9450u,AC_A0,AC_A1,AC_A2,AC_A3);
        AC_CALL(0x800aa934u,0x800a9464u,AC_S3,AC_S3,AC_S0,0);AC_CALL(0x800aff9cu,0x800a946cu,AC_S3);AC_V0=AC_FP&2u;
    }
    if(!AC_V0){
        AC_A0=AC_R32(0x800c3b70u);AC_CALL(0x8002c644u,0x800a9488u,AC_A0);
        AC_A0=AC_R32(0x800c3b70u);AC_CALL(0x8002c4bcu,0x800a9498u,AC_A0);
        AC_A0=AC_R32(0x800c3b70u);AC_CALL(0x80031894u,0x800a94a8u,AC_A0);
        AC_S1=AC_V0;AC_CALL(0x80031bdcu,0x800a94b8u,AC_V0,0);
        AC_S0=AC_V0;AC_A0=AC_S0;AC_A1=AC_R32(0x800c3b70u);AC_CALL(0x8003f968u,0x800a94d0u,AC_A0,AC_A1,AC_S1);
        AC_A0=AC_R32(0x800c3b70u);AC_CALL(0x800320e8u,0x800a94e0u,AC_A0);
        AC_A0=AC_R32(AC_S3);AC_CALL(0x8009f794u,0x800a94ecu,AC_A0,0);
        AC_A1=AC_R32(AC_S3);AC_CALL(0x8009eba8u,0x800a94f8u,AC_S0,AC_A1);AC_W32(AC_S3+0xa8u,AC_S0);
    }else AC_W32(AC_S3+0xa8u,0);
done:
    entry->cpu->gpr[31]=AC_R32(sp+0xc4u);AC_FP=AC_R32(sp+0xc0u);
    for(unsigned i=0;i<8;i++)entry->cpu->gpr[23-i]=AC_R32(sp+0xbcu-i*4u);
    entry->cpu->gpr[29]=sp+0xc8u;
}

static int battle_actor_create_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800a8bf0u || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_ACTOR_CREATE");if(control&&strcmp(control,"0")==0)return 0;
    static const char digest_hex[]="919455b6ccbb8e67bfd286da31247188fb43eb157f9b93f87bcb1cf4262811a9";
    uint8_t digest[32];file1_digest(PSX_ADDR(0x800a8bf0u),2384,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=digest_hex[i*2]||file1_hex_digit(digest[i]&15u)!=digest_hex[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_actor_create_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef AC_R8
#undef AC_R16
#undef AC_RS16
#undef AC_R32
#undef AC_W8
#undef AC_W16
#undef AC_W32
#undef AC_CALL
#undef AC_V0
#undef AC_V1
#undef AC_A0
#undef AC_A1
#undef AC_A2
#undef AC_A3
#undef AC_T0
#undef AC_S0
#undef AC_S1
#undef AC_S2
#undef AC_S3
#undef AC_S4
#undef AC_S5
#undef AC_S6
#undef AC_S7
#undef AC_FP
