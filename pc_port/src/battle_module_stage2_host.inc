/* Archive 0x0c/file 4: inventory, turn order and command-menu setup.
 * Packed addresses and o32 caller frames remain on the battle memory bus.
 * Callees retain their independent native/refusal boundaries. */
#define BATTLE_STAGE2_ITEMS_ADDRESS 0x801e4cd0u
#define BATTLE_STAGE2_ORDER_ADDRESS 0x801e4e7cu
#define BATTLE_STAGE2_COMMANDS_ADDRESS 0x801e5014u

#define M2_R8(a) battle_entry_read(entry,(a),1)
#define M2_R16(a) battle_entry_read(entry,(a),2)
#define M2_R32(a) battle_entry_read(entry,(a),4)
#define M2_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define M2_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define M2_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define M2_CALL(target,site,...) battle_entry_call(entry,(target),(site), \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t),(uint32_t[]){__VA_ARGS__})
#define M2_S0 (entry->cpu->gpr[16])
#define M2_S1 (entry->cpu->gpr[17])
#define M2_S2 (entry->cpu->gpr[18])
#define M2_S3 (entry->cpu->gpr[19])
#define M2_S4 (entry->cpu->gpr[20])
#define M2_S5 (entry->cpu->gpr[21])
#define M2_S6 (entry->cpu->gpr[22])
#define M2_S7 (entry->cpu->gpr[23])
#define M2_FP (entry->cpu->gpr[30])
#define M2_V0 (entry->cpu->gpr[2])
#define M2_V1 (entry->cpu->gpr[3])

static void battle_stage2_items_run(BattleEntryCall *entry)
{
    for (uint32_t i=0;i<48;i++) {
        M2_W8(0x800d2ce0u+i,0); M2_W8(0x800d2cb0u+i,0); M2_W8(0x800d2fe4u+i,0);
    }
    for (uint32_t i=0;i<150;i++) {
        if (M2_R8(0x8006f5c4u+i)>=100) M2_W8(0x8006f5c4u+i,99);
        if (M2_R8(0x8006f5c4u+i)==0) M2_W8(0x8006f65au+i,0);
    }
    uint32_t count=0;
    for (uint32_t i=0;i<150 && count<48;i++) {
        uint32_t id=M2_R8(0x8006f65au+i);
        if (id==0 || id>=49) continue;
        M2_W8(0x800d2ce0u+count,id);
        M2_W8(0x800d2cb0u+count,M2_R8(0x8006f5c4u+i));
        M2_W8(0x800d2fe4u+count,M2_R8(0x8006f65au+i));
        count++;
    }
    M2_W8(M2_R32(0x800c3eacu)+0x2d9u,47);
    count=0;
    /* The original secondary scan has no 48-entry cap. Preserve its stores
     * for all 100 source entries, including the final return index. */
    for (uint32_t i=0;i<100;i++) {
        uint32_t id=M2_R8(0x8006f3d0u+i);
        if (id-50u>=23u) continue;
        M2_W8(0x800c3d70u+count,id);
        M2_W8(0x800d3688u+count,M2_R8(0x8006f36cu+i));
        count++;
    }
    while (count<48) {
        M2_W8(0x800c3d70u+count,0); M2_W8(0x800d3688u+count,0); count++;
    }
    M2_V0=0; M2_V1=count;
}

static void battle_stage2_order_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x30u;
    entry->cpu->gpr[29]=sp;
    M2_W32(sp+0x2cu,entry->cpu->gpr[31]);
    for (unsigned r=0;r<3;r++) M2_W32(sp+0x20u+r*4u,entry->cpu->gpr[16+r]);
    M2_W8(0x800d2caau,0);
    M2_CALL(0x80078508u,0x801e4e98u,sp+0x10u);
    M2_S0=0; M2_S1=sp+0x10u; M2_S2=1;
    do {
        M2_CALL(0x8001bd40u,0x801e4eb0u,0,10);
        uint32_t value=M2_V0, flag=M2_S1+(value&0xffu);
        if (M2_R8(flag)==0) {
            M2_W8(flag,M2_S2); M2_W8(0x800d2dd8u+M2_S0,value); M2_S0++;
        }
    } while ((int32_t)M2_S0<11);
    M2_W8(0x800d2dd7u,0);
    for (M2_S0=3;M2_S0<11;M2_S0++) {
        if (M2_R8(0x800d2dccu+M2_S0)!=0 &&
            (M2_R16(0x800ccd1cu+M2_S0*0x170u)&0x200u)!=0) {
            M2_W16(0x800d2e06u+M2_S0*2u,1);
            M2_W16(0x800d2df0u+M2_S0*2u,1);
        }
    }
    int32_t minimum=65535;
    for (M2_S0=0;M2_S0<11;M2_S0++) {
        if (M2_R8(0x800d2dccu+M2_S0)==0) continue;
        int32_t value=(int16_t)M2_R16(0x800d2e06u+M2_S0*2u);
        if (value<minimum) minimum=value;
    }
    uint32_t offset=(uint32_t)minimum-1u;
    for (M2_S0=0;M2_S0<11;M2_S0++) {
        if (M2_R8(0x800d2dccu+M2_S0)!=0)
            M2_W16(0x800d2e06u+M2_S0*2u,M2_R16(0x800d2e06u+M2_S0*2u)-offset);
    }
    M2_V0=0; M2_V1=0x800d2e1cu;
    entry->cpu->gpr[31]=M2_R32(sp+0x2cu);
    for (unsigned r=0;r<3;r++) entry->cpu->gpr[16+r]=M2_R32(sp+0x20u+r*4u);
    entry->cpu->gpr[29]=sp+0x30u;
}

static void battle_stage2_commands_run(BattleEntryCall *entry)
{
    uint32_t flag=M2_R8(0x8006f9ddu),sp=entry->cpu->gpr[29]-0x38u;
    entry->cpu->gpr[29]=sp;
    M2_W32(sp+0x34u,entry->cpu->gpr[31]); M2_W32(sp+0x30u,M2_FP);
    for (unsigned r=0;r<8;r++) M2_W32(sp+0x10u+r*4u,entry->cpu->gpr[16+r]);
    if (flag&0x20u) M2_W8(0x800c3d48u,1);
    M2_S2=0; M2_FP=0x800c20f0u; M2_S7=7; M2_S4=0x800ccd62u;
    M2_S3=M2_S1=M2_S6=M2_S5=0;
    do {
        M2_W8(0x800d32a4u+M2_S5,M2_R8(0x8006ed0bu+M2_R8(0x800d2d24u+M2_S2)*32u));
        for (uint32_t i=0;i<8;i++) {
            uint32_t menu=M2_R32(0x800c3eacu)+M2_S1;
            uint32_t source=M2_R32(M2_FP+M2_R8(0x800ccd3eu+M2_S3)*4u);
            M2_W8(menu+i,M2_R8(source+i));
        }
        for (uint32_t i=0;i<4;i++) {
            uint32_t id=M2_R8(0x800d2d24u+M2_S2),menu=M2_R32(0x800c3eacu)+M2_S1;
            uint32_t source=M2_R32(id==M2_S7 ? 0x800c2138u : 0x800c2130u);
            M2_W8(menu+i+8u,M2_R8(source+i));
            menu=M2_R32(0x800c3eacu)+M2_S1; source=M2_R32(0x800c2134u);
            M2_W8(menu+i+12u,M2_R8(source+i));
        }
        M2_S0=M2_S2&0xffu;
        M2_CALL(0x800841e0u,0x801e5184u,M2_S0);
        M2_W8(M2_R32(0x800c3eacu)+M2_S1+0x3cu,M2_V0);
        uint32_t selected=M2_R8(M2_R32(0x800c3eacu)+M2_S1+0x3cu);
        M2_CALL(0x80085310u,0x801e51b4u,M2_S0,selected);
        M2_W8(0x800c3ebau+M2_S6,M2_V0);
        uint32_t menu=M2_R32(0x800c3eacu)+M2_S1;
        for (uint32_t i=0;i<16;i++) {
            uint32_t bits=M2_R16(0x800ccd62u+M2_S3),mask=M2_R16(0x800c3234u+i*2u);
            M2_W16(menu+0x1cu+i*2u,bits&mask);
        }
        if (M2_R8(0x800ccd88u+M2_S3)==0xffu || (M2_R8(0x8006f9ddu)&0x40u)) {
            menu=M2_R32(0x800c3eacu)+M2_S1;
            M2_W16(menu+0x2au,M2_R16(0x800c3242u));
            M2_W16(0x800ccd62u+M2_S3,M2_R16(0x800ccd62u+M2_S3)|M2_R16(0x800c3242u));
        }
        flag=M2_R8(0x8006f9ddu); M2_S3+=0x170u;
        if (flag&0x80u) {
            menu=M2_R32(0x800c3eacu)+M2_S1;
            M2_W16(menu+0x2cu,M2_R16(0x800c3244u));
            M2_W16(M2_S4,M2_R16(M2_S4)|M2_R16(0x800c3244u));
        }
        M2_S4+=0x170u; M2_S1+=0x40u; M2_S6+=0x1cu; M2_S2++; M2_S5+=8;
    } while ((int32_t)M2_S2<3);
    M2_S2=3; M2_S1=0x54;
    do {
        M2_S0=M2_S2&0xffu;
        M2_CALL(0x800841e0u,0x801e5300u,M2_S0);
        uint32_t offset=M2_S2*0x40u;
        M2_W8(M2_R32(0x800c3eacu)+offset+0x3cu,M2_V0);
        uint32_t selected=M2_R8(M2_R32(0x800c3eacu)+offset+0x3cu);
        M2_S2++;
        M2_CALL(0x80085310u,0x801e5330u,M2_S0,selected);
        M2_W8(0x800c3ebau+M2_S1,M2_V0);
        M2_S1+=0x1cu;
    } while ((int32_t)M2_S2<11);
    M2_V0=0;
    entry->cpu->gpr[31]=M2_R32(sp+0x34u); M2_FP=M2_R32(sp+0x30u);
    for (unsigned r=0;r<8;r++) entry->cpu->gpr[16+r]=M2_R32(sp+0x10u+r*4u);
    entry->cpu->gpr[29]=sp+0x38u;
}

#undef M2_R8
#undef M2_R16
#undef M2_R32
#undef M2_W8
#undef M2_W16
#undef M2_W32
#undef M2_CALL
#undef M2_S0
#undef M2_S1
#undef M2_S2
#undef M2_S3
#undef M2_S4
#undef M2_S5
#undef M2_S6
#undef M2_S7
#undef M2_FP
#undef M2_V0
#undef M2_V1

static int battle_module_stage2_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct { uint32_t address,bytes; const char *identity; void (*run)(BattleEntryCall *); } bodies[]={
        {BATTLE_STAGE2_ITEMS_ADDRESS,0x1acu,"b7310fec87836ab820b6e79f280ebe5e7fea13691347446d28a25cf8e65181e5",battle_stage2_items_run},
        {BATTLE_STAGE2_ORDER_ADDRESS,0x198u,"34625a5dc1da5b5cf44a8b783d977a5f9896ff1f4f40060d8b091e09d359fe7b",battle_stage2_order_run},
        {BATTLE_STAGE2_COMMANDS_ADDRESS,0x370u,"c7da46f5df0b302d6a108f859cacb4cc693bad98c3a1c278aecdc0a2798b3c64",battle_stage2_commands_run}
    };
    unsigned selected;
    for (selected=0;selected<3;selected++) if (bodies[selected].address==target) break;
    if (selected==3 || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime ||
        runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MODULE_STAGE2");
    if (control && strcmp(control,"0")==0) return 0;
    uint8_t digest[32];
    for (unsigned region=0;region<2;region++) {
        uint32_t address=region ? bodies[selected].address : BATTLE_MODULE_DISPATCH_ADDRESS;
        unsigned bytes=region ? bodies[selected].bytes : BATTLE_MODULE_DISPATCH_BYTES;
        const char *expected=region ? bodies[selected].identity : BATTLE_MODULE_DISPATCH_SHA256;
        file1_digest(PSX_ADDR(address),bytes,digest);
        for (unsigned i=0;i<sizeof(digest);i++)
            if (file1_hex_digit(digest[i]>>4)!=expected[i*2] ||
                file1_hex_digit(digest[i]&15u)!=expected[i*2+1]) return 0;
    }
    void (*volatile run)(BattleEntryCall *)=bodies[selected].run;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if (setjmp(entry.failure)!=0) return -1;
    run(&entry); cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
}
