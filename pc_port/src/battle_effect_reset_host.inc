/* Effect pools and actor reset. Guest records keep their original word width;
 * frames, live counts and ordered writes use the production memory bus. */
#define ER_R16(a) battle_entry_read(entry,(a),2)
#define ER_R32(a) battle_entry_read(entry,(a),4)
#define ER_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define ER_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define ER_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define ER_CALL(target,site,...) battle_entry_call(entry,(target),(site), \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t),(uint32_t[]){__VA_ARGS__})
#define ER_V0 (entry->cpu->gpr[2])
#define ER_V1 (entry->cpu->gpr[3])
#define ER_A0 (entry->cpu->gpr[4])
#define ER_A1 (entry->cpu->gpr[5])
#define ER_S0 (entry->cpu->gpr[16])
#define ER_S1 (entry->cpu->gpr[17])
#define ER_S2 (entry->cpu->gpr[18])
#define ER_S3 (entry->cpu->gpr[19])
#define ER_S4 (entry->cpu->gpr[20])
#define ER_S5 (entry->cpu->gpr[21])
#define ER_S6 (entry->cpu->gpr[22])
#define ER_S7 (entry->cpu->gpr[23])
#define ER_FP (entry->cpu->gpr[30])

static void battle_effect_pool20_initialize(BattleEntryCall *entry)
{
    ER_V0=ER_R32(ER_A0);
    entry->cpu->gpr[29]-=8u;
    if(ER_V0) {
        ER_A1=ER_V0; ER_V0=ER_R16(ER_A0+6u); ER_V1=0;
        ER_W16(ER_A0+4u,0);
        if(ER_V0) do {
            ER_W8(ER_A1,0);
            uint32_t count=ER_R16(ER_A0+6u); ER_V1++;
            ER_V0=(int32_t)ER_V1<(int32_t)count; ER_A1+=20u;
        } while(ER_V0);
    }
    entry->cpu->gpr[29]+=8u;
}

static void battle_effect_pool_allocate(BattleEntryCall *entry,int wide)
{
    uint32_t sp=entry->cpu->gpr[29]-0x20u; entry->cpu->gpr[29]=sp;
    ER_W32(sp+0x14u,ER_S1); ER_S1=ER_A0;
    ER_W32(sp+0x10u,ER_S0); ER_S0=ER_A1;
    ER_W32(sp+0x18u,entry->cpu->gpr[31]);
    if(wide || (int32_t)ER_S0>0) {
        if(!wide) ER_W16(ER_S1+6u,ER_S0);
        ER_CALL(0x80032498u,wide ? 0x800a2cc0u:0x800a2258u,4,0);
        uint32_t size;
        if(wide) {
            ER_V0=ER_S0+1u; size=ER_V0*124u;
            ER_W16(ER_S1+4u,ER_S0); ER_W16(ER_S1+6u,0);
        } else size=ER_S0*20u;
        ER_CALL(0x80031bdcu,wide ? 0x800a2ce0u:0x800a226cu,size,0);
        ER_W32(ER_S1,ER_V0);
        if(ER_V0) {
            ER_CALL(wide ? 0x800a2d5cu:0x800a22e8u,
                    wide ? 0x800a2cf8u:0x800a2284u,ER_S1);
            ER_V0=ER_S1;
        }
    } else ER_V0=0;
    entry->cpu->gpr[31]=ER_R32(sp+0x18u);
    ER_S1=ER_R32(sp+0x14u); ER_S0=ER_R32(sp+0x10u);
    entry->cpu->gpr[29]=sp+0x20u;
}

static void battle_effect_pool124_initialize(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x40u; entry->cpu->gpr[29]=sp;
    ER_W32(sp+0x38u,ER_FP); ER_FP=ER_A0;
    ER_W32(sp+0x30u,ER_S6); ER_W32(sp+0x3cu,entry->cpu->gpr[31]);
    ER_W32(sp+0x34u,ER_S7);
    for(unsigned r=0;r<6;r++) ER_W32(sp+0x2cu-r*4u,entry->cpu->gpr[21-r]);
    ER_V0=(uint32_t)(int32_t)(int16_t)ER_R16(ER_FP+4u);
    ER_S4=ER_R32(ER_FP); ER_V0++; ER_S6=0;
    if((int32_t)ER_V0>0) {
        ER_S7=0xc1; ER_S5=ER_S4+0x1eu;
        do {
            ER_S3=0; ER_S1=ER_S4; ER_S2=0x2c; ER_V0=0xffffffffu;
            ER_W16(ER_S5-8u,ER_V0); ER_W16(ER_S5,0);
            do {
                ER_S0=ER_S4+ER_S2;
                ER_CALL(0x80043cb0u,0x800a2dc4u,ER_S0);
                ER_CALL(0x80043bfcu,0x800a2dd0u,ER_S0,1);
                ER_CALL(0x80043a58u,0x800a2ddcu,0,0x1cd);
                ER_W16(ER_S1+0x3au,ER_V0);
                ER_CALL(0x80043a1cu,0x800a2df4u,0,1,0x380,0);
                ER_V1=0xf; ER_W8(ER_S1+0x48u,ER_V1); ER_W16(ER_S1+0x42u,ER_V0);
                ER_W8(ER_S1+0x38u,0); ER_W8(ER_S1+0x39u,ER_S7);
                ER_W8(ER_S1+0x40u,0); ER_W8(ER_S1+0x41u,ER_S7);
                ER_W8(ER_S1+0x49u,ER_S7); ER_W8(ER_S1+0x50u,ER_V1);
                ER_W8(ER_S1+0x51u,ER_S7);
                ER_S1+=0x28u; ER_S3++; ER_V0=(int32_t)ER_S3<2; ER_S2+=0x28u;
            } while(ER_V0);
            ER_S5+=0x7cu;
            uint32_t count=(uint32_t)(int32_t)(int16_t)ER_R16(ER_FP+4u);
            ER_S6++; ER_V0=(int32_t)ER_S6<(int32_t)(count+1u); ER_S4+=0x7cu;
        } while(ER_V0);
    }
    entry->cpu->gpr[31]=ER_R32(sp+0x3cu); ER_FP=ER_R32(sp+0x38u);
    for(unsigned r=0;r<8;r++) entry->cpu->gpr[23-r]=ER_R32(sp+0x34u-r*4u);
    entry->cpu->gpr[29]=sp+0x40u;
}

static void battle_effect_clear_handles(BattleEntryCall *entry)
{
    ER_V1=8; ER_V0=0x800c3bccu;
    do {
        ER_W32(ER_V0,0); ER_V1--; ER_V0-=4u;
    } while((int32_t)ER_V1>=0);
}

static void battle_effect_reset_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x18u; entry->cpu->gpr[29]=sp;
    ER_V0=ER_R32(0x800658c8u);
    ER_W32(sp+0x10u,entry->cpu->gpr[31]);
    ER_W32(0x800c3e88u,0); ER_W16(0x800c3cf0u,0);
    ER_W8(0x800c3d6cu,0); ER_W8(0x800c3d68u,0); ER_W16(0x800c3b7cu,0);
    uint32_t count=(uint32_t)(int32_t)(int16_t)ER_R16(ER_V0+0x348u);
    ER_V0=1; ER_W8(0x800c3b74u,ER_V0);
    ER_CALL(0x800a2234u,0x800a8b5cu,0x800c3d0c,count);
    ER_V0=ER_R32(0x800658c8u);
    count=(uint32_t)(int32_t)(int16_t)ER_R16(ER_V0+0x34au);
    ER_CALL(0x800a2ca4u,0x800a8b7cu,0x800c3d04,count);
    battle_entry_call(entry,0x800b00d0u,0x800a8b84u,0,NULL);
    ER_V1=31; ER_V0=0x800d33e4u;
    do {
        ER_W32(ER_V0,0); ER_V1--; ER_V0-=4u;
    } while((int32_t)ER_V1>=0);
    ER_V0=0x98;
    do { ER_W32(0x800c3accu+ER_V0,0); ER_V0-=8u; } while((int32_t)ER_V0>=0);
    ER_V0=0x14;
    do { ER_W16(0x800d330au+ER_V0,0); ER_V0-=20u; } while((int32_t)ER_V0>=0);
    entry->cpu->gpr[31]=ER_R32(sp+0x10u); entry->cpu->gpr[29]=sp+0x18u;
}

static int battle_effect_reset_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct {uint32_t address,size;const char *sha;} bodies[]={
        {0x800a8b0cu,228,"d8a9319200de7602a227fe1b8fd9349f364e7691fb1e2e28e7215cf0353d9583"},
        {0x800a2234u,116,"0c12249860322bed7d937d2276599ac428e79a606f32e5b678a98a94172503c5"},
        {0x800a22e8u,72,"fb45f407379a01a8b20e8714010e32f7d85cffa29354ea23dedc99acaebf124c"},
        {0x800a2ca4u,120,"a47661b2522a24103356dd7ebbff584cf08562181fd0665b48078560c3b4eb54"},
        {0x800a2d5cu,300,"dcea56cb6d39b05fd00eb3ad9e1ef8202b95dd538d3442f9226095edb01b142d"},
        {0x800b00d0u,36,"99fa80ecd6fe4df0bda74d80e07eeea5738cab2d91f0e280d20e7b9cfe5bd7a8"}};
    unsigned index;
    for(index=0;index<sizeof(bodies)/sizeof(bodies[0]);index++) if(target==bodies[index].address) break;
    if(index==sizeof(bodies)/sizeof(bodies[0]) || runtime->force_interpret || !runtime->initialized ||
       g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_EFFECT_RESET");
    if(control && strcmp(control,"0")==0) return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),bodies[index].size,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[index].sha[i*2] ||
           file1_hex_digit(digest[i]&15u)!=bodies[index].sha[i*2+1]) return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0) return -1;
    switch(target) {
    case 0x800a8b0cu:battle_effect_reset_run(&entry);break;
    case 0x800a2234u:battle_effect_pool_allocate(&entry,0);break;
    case 0x800a22e8u:battle_effect_pool20_initialize(&entry);break;
    case 0x800a2ca4u:battle_effect_pool_allocate(&entry,1);break;
    case 0x800a2d5cu:battle_effect_pool124_initialize(&entry);break;
    case 0x800b00d0u:battle_effect_clear_handles(&entry);break;
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef ER_R16
#undef ER_R32
#undef ER_W8
#undef ER_W16
#undef ER_W32
#undef ER_CALL
#undef ER_V0
#undef ER_V1
#undef ER_A0
#undef ER_A1
#undef ER_S0
#undef ER_S1
#undef ER_S2
#undef ER_S3
#undef ER_S4
#undef ER_S5
#undef ER_S6
#undef ER_S7
#undef ER_FP
