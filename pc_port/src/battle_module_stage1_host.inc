/* Archive 0x0c/file 4: native battle-state, slot, enemy-resource and actor-mode
 * setup. Data and o32 frames stay in game memory; callees keep their own
 * native/refusal boundaries. No original CPU instruction is fetched here. */
#define BATTLE_STAGE1_STATE_ADDRESS 0x801e4048u
#define BATTLE_STAGE1_SLOTS_ADDRESS 0x801e4160u
#define BATTLE_STAGE1_ENEMIES_ADDRESS 0x801e4870u
#define BATTLE_STAGE1_MODES_ADDRESS 0x801e4ac0u

#define M1_R8(a) battle_entry_read(entry, (a), 1)
#define M1_R16(a) battle_entry_read(entry, (a), 2)
#define M1_R32(a) battle_entry_read(entry, (a), 4)
#define M1_W8(a,v) battle_entry_write(entry, (a), 1, (v))
#define M1_W16(a,v) battle_entry_write(entry, (a), 2, (v))
#define M1_W32(a,v) battle_entry_write(entry, (a), 4, (v))
#define M1_CALL0(target,site) battle_entry_call(entry, (target), (site), 0, NULL)
#define M1_CALL(target,site,...) battle_entry_call(entry, (target), (site), \
    sizeof((uint32_t[]){__VA_ARGS__}) / sizeof(uint32_t), (uint32_t[]){__VA_ARGS__})
#define M1_S0 (entry->cpu->gpr[16])
#define M1_S1 (entry->cpu->gpr[17])
#define M1_S2 (entry->cpu->gpr[18])
#define M1_S3 (entry->cpu->gpr[19])
#define M1_S4 (entry->cpu->gpr[20])
#define M1_S5 (entry->cpu->gpr[21])
#define M1_S6 (entry->cpu->gpr[22])
#define M1_S7 (entry->cpu->gpr[23])
#define M1_FP (entry->cpu->gpr[30])
#define M1_V0 (entry->cpu->gpr[2])
#define M1_V1 (entry->cpu->gpr[3])

static void battle_stage1_state_run(BattleEntryCall *entry)
{
    M1_W8(0x800d3338u,0); M1_W8(0x800c3d44u,0);
    M1_W8(0x800d2d50u,0); M1_W8(0x800d2fc4u,0);
    M1_W8(0x800c3d5cu,0); M1_W8(0x800d2d44u,0);
    M1_W8(0x800c492au,0); M1_W16(0x8005941cu,0);
    M1_W8(0x8005942cu,0);
    for (uint32_t i=0; i<3; i++) {
        M1_W8(0x80059468u+i, M1_R8(0x800d2d24u+i));
        M1_W8(0x800c3eb6u+i*0x1cu, M1_R8(0x800d2d24u+i));
    }
    for (uint32_t i=0; i<11; i++) {
        M1_W8(0x800c3eb7u+i*0x1cu,0);
        M1_W8(0x800c3eb8u+i*0x1cu,
               M1_R8(0x800d3294u) == 0 ? M1_R8(0x8006f8e5u+i) : 1);
        M1_W8(0x800c3eb9u+i*0x1cu,0);
    }
    M1_W8(0x800c48eau,0); M1_W8(0x800d2dc0u,0);
    M1_V0=0; M1_V1=0x134;
}

static void battle_stage1_slots_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x28u;
    entry->cpu->gpr[29]=sp;
    for (unsigned r=0; r<5; r++) M1_W32(sp+0x10u+r*4u,entry->cpu->gpr[16+r]);
    M1_W32(sp+0x24u,entry->cpu->gpr[31]);
    M1_S1=0;
    uint32_t table=M1_R32(0x8005949cu);
    M1_W8(0x800d3280u,0);
    M1_W32(0x800d3364u,table); M1_W32(0x800c3eb0u,table);
    for (uint32_t i=0; i<3; i++) {
        if ((M1_R8(0x800c3eb6u+i*0x1cu)&0x7fu) != 0x7fu) {
            M1_W8(0x800d2dccu+i,1);
            M1_W8(0x800d3280u,M1_R8(0x800d3280u)+1u);
        } else M1_W8(0x800d2dccu+i,0);
        M1_W8(0x800c3eb4u+i*0x1cu,M1_R8(0x800c3eb8u+i*0x1cu)==0 ?
               M1_R8(0x8006f9e0u+i)&0x7fu : i);
    }
    M1_W8(0x800d3280u,M1_R8(0x800d3280u)+0xffu);
    for (uint32_t i=3; i<11; i++) {
        uint32_t record=0x800c3eb4u+i*0x1cu;
        uint32_t actor=M1_R8(0x8006f9e1u+i)&0x7fu;
        if (actor != 0x7fu) {
            M1_W8(record+2u,actor);
            M1_W8(record+3u,M1_R8(0x8006f9e9u+i)&0x80u);
            M1_W8(record+4u,M1_R8(0x8006f9e1u+i)&0x80u);
            M1_W8(record+5u,M1_R8(0x8006f9e9u+i)&1u);
            M1_W8(0x800d2dccu+i,1);
            M1_W8(record,M1_R8(0x8006f9f1u+i)&0x7fu);
        } else {
            M1_W16(0x800ccd36u+i*0x170u,0);
            M1_W16(0x800ccd34u+i*0x170u,0);
            M1_W8(record+2u,0x7f); M1_W8(record+3u,0); M1_W8(record+4u,0);
            M1_W8(0x800d2dccu+i,0);
        }
        M1_W8(0x800c3e3du+i,M1_R8(record+2u)+1u);
    }
    for (uint32_t offset=0; offset<0x80; offset+=4) {
        M1_W8(0x800d301cu+offset,0); M1_W8(0x800d301du+offset,0);
    }
    /* First three slots and eight enemy slots use separate model counters. */
    for (unsigned side=0; side<2; side++) {
        M1_S1=side ? 3 : 0;
        M1_S4=1;
        M1_S3=M1_S1*0x1cu;
        M1_S0=0x800c3eb4u+M1_S3; M1_S2=M1_S0+1u;
        do {
            M1_V1=M1_R8(0x800c3eb6u+M1_S3);
            if (M1_V1 != 0x7fu) {
                if (M1_R8(0x800c3eb8u+M1_S3)==0) {
                    uint32_t index=(M1_R8(M1_S0)+(side ? 8u : 0u))*4u;
                    M1_W8(M1_S2,M1_R8(0x800d301cu+index));
                    M1_CALL(0x80089c08u,side ? 0x801e456cu : 0x801e443cu,M1_R8(M1_S2));
                    index=(M1_R8(M1_S0)+(side ? 8u : 0u))*4u;
                    M1_V1=M1_R8(0x800d301du+index)|M1_V0;
                    M1_W8(0x800d301du+index,M1_V1);
                    M1_V1=(M1_R8(M1_S0)+(side ? 8u : 0u))*4u;
                    M1_W8(0x800d301cu+M1_V1,M1_R8(0x800d301cu+M1_V1)+1u);
                } else {
                    M1_W8(M1_S2,0);
                    M1_W8(0x800d301du+(M1_R8(M1_S0)+(side ? 24u : 16u))*4u,M1_S4);
                    M1_W8(0x800d301cu+(M1_R8(M1_S0)+(side ? 24u : 16u))*4u,M1_S4);
                }
            }
            M1_S0+=0x1cu; M1_S2+=0x1cu; M1_S1++; M1_S3+=0x1cu;
        } while ((int32_t)M1_S1 < (side ? 11 : 3));
    }
    table=M1_R32(0x800d3364u);
    for (M1_S1=0; M1_S1<3; M1_S1++) {
        uint32_t record=0x800c3eb4u+M1_S1*0x1cu;
        if (M1_R8(record+2u)==0x7fu) continue;
        if (M1_R8(record+4u)==0) {
            M1_V1=M1_R8(record)<<5;
            M1_W16(record+10u,M1_R16(table+M1_V1+(M1_R8(record+1u)<<2)+4u));
            M1_V1=M1_R8(record)<<5;
            M1_W16(record+12u,M1_R16(table+M1_V1+(M1_R8(record+1u)<<2)+6u));
        } else {
            M1_W16(record+10u,M1_R16(table+(M1_R8(record)<<3)+0x100u));
            M1_W16(record+12u,M1_R16(table+(M1_R8(record)<<3)+0x102u));
        }
    }
    for (M1_S1=3; M1_S1<11; M1_S1++) {
        uint32_t record=0x800c3eb4u+M1_S1*0x1cu;
        if (M1_R8(record+2u)==0x7fu) continue;
        if (M1_R8(record+4u)==0) {
            table=M1_R32(0x800d3364u);
            M1_V1=M1_R8(record)<<5;
            M1_W16(record+10u,M1_R16(table+M1_V1+(M1_R8(record+1u)<<2)+0x10u));
            M1_V1=(M1_R8(record+1u)<<2)+(M1_R8(record)<<5);
            M1_W16(record+12u,M1_R16(table+M1_V1+0x12u));
        } else {
            table=M1_R32(0x800d3364u);
            M1_W16(record+10u,M1_R16(table+(M1_R8(record)<<3)+0x104u));
            M1_V1=table+(M1_R8(record)<<3);
            M1_W16(record+12u,M1_R16(M1_V1+0x106u));
        }
        M1_W8(record+6u,M1_R8(0x8006f9f7u+M1_S1-3u)&0x80u);
    }
    M1_V0=0;
    entry->cpu->gpr[31]=M1_R32(sp+0x24u);
    for (unsigned r=0; r<5; r++) entry->cpu->gpr[16+r]=M1_R32(sp+0x10u+r*4u);
    entry->cpu->gpr[29]=sp+0x28u;
}

static void battle_stage1_enemies_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x38u;
    entry->cpu->gpr[29]=sp;
    for (unsigned r=0; r<8; r++) M1_W32(sp+0x10u+r*4u,entry->cpu->gpr[16+r]);
    M1_W32(sp+0x30u,M1_FP); M1_W32(sp+0x34u,entry->cpu->gpr[31]);
    M1_S6=3; M1_FP=0x800d3410u; M1_S2=0x800c3d19u; M1_S1=0x800c3d18u;
    M1_S4=0x450; M1_S0=M1_S5=0; M1_S3=0x800c3f0au;
    uint32_t scenario=M1_R32(0x800c3dd0u);
    M1_W16(0x800c48e8u,0); M1_W16(0x800d39e0u,0);
    M1_S7=scenario+0x32u;
    M1_W32(0x800c3ddcu,scenario+M1_R16(scenario+0x30u));
    do {
        M1_W8(0x800c3d1bu+M1_S5,0);
        M1_V1=M1_R8(M1_S3);
        if (M1_V1 != 0x7fu) {
            M1_CALL(0x8003f99cu,0x801e4934u,0x800ccce8u+M1_S4,M1_S7+M1_V1*0x170u,0x170);
            uint32_t actor=M1_R8(M1_S3);
            scenario=M1_R32(0x800c3dd0u);
            uint32_t header=scenario+M1_R16(scenario+actor*2u);
            M1_W32(0x800d3400u+M1_S0,header+M1_R16(header));
            M1_W32(0x800d3404u+M1_S0,header+M1_R16(header+2u));
            uint32_t offset=M1_R16(header+4u);
            if (offset != 0xffffu) {
                M1_W32(0x800d3408u+M1_S0,header+offset); M1_W8(M1_S1,1);
            } else M1_W8(M1_S1,0);
            offset=M1_R16(header+6u);
            if (offset != 0xffffu) {
                M1_W32(0x800d340cu+M1_S0,header+offset); M1_W8(M1_S2,1);
            } else M1_W8(M1_S2,0);
            for (unsigned i=0; i<4; i++) M1_W32(M1_S0+M1_FP+12u-i*4u,0);
            for (unsigned i=0; i<8; i++) M1_W16(M1_S0+0x800d342eu-i*2u,0);
            for (unsigned i=0; i<16; i++) M1_W8(M1_S0+0x800d343fu-i,0);
            M1_V1=UINT32_MAX;
        } else {
            M1_CALL(0x8003f8e8u,0x801e4a58u,0x800ccce8u+M1_S4,0x170);
            M1_W8(M1_S1,0); M1_W8(M1_S2,0);
        }
        M1_S2+=4; M1_S1+=4; M1_S4+=0x170; M1_S0+=0x40; M1_S5+=4; M1_S6++; M1_S3+=0x1c;
    } while ((int32_t)M1_S6<11);
    M1_V0=0;
    entry->cpu->gpr[31]=M1_R32(sp+0x34u); M1_FP=M1_R32(sp+0x30u);
    for (unsigned r=0; r<8; r++) entry->cpu->gpr[16+r]=M1_R32(sp+0x10u+r*4u);
    entry->cpu->gpr[29]=sp+0x38u;
}

static void battle_stage1_modes_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x18u;
    entry->cpu->gpr[29]=sp; M1_W32(sp+0x10u,entry->cpu->gpr[31]);
    M1_CALL0(0x80097d5cu,0x801e4ac8u);
    uint32_t special=M1_R8(0x800d3294u);
    entry->cpu->gpr[5]=0;
    if (special != 0) M1_CALL0(0x8009b098u,0x801e4ae4u);
    for (uint32_t i=0; i<3; i++) {
        uint32_t record=0x800c3eb4u+i*0x1cu, flags=0x800cce42u+i*0x170u;
        if (M1_R8(record+2u) != 0x7fu) {
            M1_W8(M1_R32(0x800c3ea4u)+i*0x1e4u+0x853du,1);
            if (M1_R8(record+4u) != 0) {
                M1_W8(0x800d32a1u+i*8u,1); M1_W8(flags,M1_R8(flags)|0x80u);
                if (M1_R8(record+2u) != 7)
                    M1_W8(M1_R32(0x800c3ea4u)+i*0x1e4u+0x853du,2);
            } else {
                M1_W8(0x800d32a1u+i*8u,0); M1_W8(flags,M1_R8(flags)&0x7fu);
            }
        } else {
            M1_W8(0x800d32a1u+i*8u,0); M1_W8(flags,M1_R8(flags)&0x7fu);
            M1_W8(M1_R32(0x800c3ea4u)+i*0x1e4u+0x853du,0);
        }
    }
    for (uint32_t i=3; i<11; i++) {
        uint32_t record=0x800c3eb4u+i*0x1cu;
        M1_W8(0x800d32a1u+i*8u,M1_R8(record+2u)!=0x7fu && M1_R8(record+4u)!=0);
    }
    for (uint32_t i=0; i<3; i++) {
        M1_W8(0x800d32a5u+i*8u,M1_R8(0x800ccd4au+i*0x170u));
        M1_W8(0x800d32a6u+i*8u,M1_R8(0x800ccd4bu+i*0x170u));
    }
    M1_V0=0; M1_V1=24;
    entry->cpu->gpr[31]=M1_R32(sp+0x10u); entry->cpu->gpr[29]=sp+0x18u;
}

#undef M1_R8
#undef M1_R16
#undef M1_R32
#undef M1_W8
#undef M1_W16
#undef M1_W32
#undef M1_CALL0
#undef M1_CALL
#undef M1_S0
#undef M1_S1
#undef M1_S2
#undef M1_S3
#undef M1_S4
#undef M1_S5
#undef M1_S6
#undef M1_S7
#undef M1_FP
#undef M1_V0
#undef M1_V1

static int battle_module_stage1_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct { uint32_t address,bytes; const char *identity; void (*run)(BattleEntryCall *); } bodies[]={
        {BATTLE_STAGE1_STATE_ADDRESS,0x118u,"b3c89ac25d933f8957d5705230b46bcea9c3b1e9a81f45c6d8bf16d351f60606",battle_stage1_state_run},
        {BATTLE_STAGE1_SLOTS_ADDRESS,0x710u,"2c59db4a6fc5eba53d28e223112f2408575a1dc03f1a24001fb3e86069f02a7e",battle_stage1_slots_run},
        {BATTLE_STAGE1_ENEMIES_ADDRESS,0x250u,"fe4c1c42d9fd38abaa646bda980024ce9390d0be52e3b4d13c2cdb8394e9c0e7",battle_stage1_enemies_run},
        {BATTLE_STAGE1_MODES_ADDRESS,0x210u,"29b6a4bc93b4bf807e5fb0e84ff97a49784163c40a16bac9bf27af353d00ea1e",battle_stage1_modes_run}
    };
    unsigned selected;
    for (selected=0; selected<4; selected++) if (bodies[selected].address==target) break;
    if (selected==4 || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime ||
        runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MODULE_STAGE1");
    if (control && strcmp(control,"0")==0) return 0;
    /* Dispatcher identity identifies the resident loader; each adopted body
     * also checks its own entire region. Mutable scenario data is not hashed. */
    uint8_t digest[32];
    for (unsigned region=0; region<2; region++) {
        uint32_t address=region ? bodies[selected].address : BATTLE_MODULE_DISPATCH_ADDRESS;
        unsigned bytes=region ? bodies[selected].bytes : BATTLE_MODULE_DISPATCH_BYTES;
        const char *expected=region ? bodies[selected].identity : BATTLE_MODULE_DISPATCH_SHA256;
        file1_digest(PSX_ADDR(address),bytes,digest);
        for (unsigned i=0; i<sizeof(digest); i++)
            if (file1_hex_digit(digest[i]>>4)!=expected[i*2] ||
                file1_hex_digit(digest[i]&15u)!=expected[i*2+1]) return 0;
    }
    void (*volatile run)(BattleEntryCall *)=bodies[selected].run;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if (setjmp(entry.failure)!=0) return -1;
    run(&entry); cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
}
