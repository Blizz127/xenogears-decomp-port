/* Complete sprite dependencies of BB350. This does not adopt BB350 or its
 * heap/work-list boundary. All packed pointers remain guest-domain words. */
#define ES_R(a) battle_entry_read(entry,(a),4)
#define ES_H(a) battle_entry_read(entry,(a),2)
#define ES_S(a) ((uint32_t)(int32_t)(int16_t)ES_H(a))
#define ES_W(a,v) battle_entry_write(entry,(a),4,(v))
#define ES_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define ES_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define ES_AT (entry->cpu->gpr[1])
#define ES_V0 (entry->cpu->gpr[2])
#define ES_V1 (entry->cpu->gpr[3])
#define ES_A0 (entry->cpu->gpr[4])
#define ES_A1 (entry->cpu->gpr[5])
#define ES_A2 (entry->cpu->gpr[6])
#define ES_A3 (entry->cpu->gpr[7])
#define ES_T0 (entry->cpu->gpr[8])
#define ES_S0 (entry->cpu->gpr[16])
#define ES_SP (entry->cpu->gpr[29])
#define ES_RA (entry->cpu->gpr[31])

static uint32_t battle_effect_sprite_sra12(uint32_t value)
{
    return (value>>12)|((value&0x80000000u)?0xfff00000u:0u);
}

static void battle_effect_sprite_mult(BattleEntryCall *entry,uint32_t a,uint32_t b)
{
    uint64_t product=(uint64_t)((int64_t)(int32_t)a*(int64_t)(int32_t)b);
    entry->cpu->lo=(uint32_t)product;
    entry->cpu->hi=(uint32_t)(product>>32);
}

static void battle_effect_sprite_initialize(BattleEntryCall *entry)
{
    ES_T0=ES_A0;ES_A2=0xff0fffffu;ES_A1=0xfeffffffu;
    ES_A0=0xfff0ffffu;ES_A3=0xfffe1fffu;ES_V0=0x2du;
    ES_W(ES_T0+0x3cu,0);ES_SB(ES_T0+0x2bu,ES_V0);
    ES_V0=ES_R(ES_T0+0x3cu);ES_V1=0xfffffffbu;
    ES_W(ES_T0+0x40u,0);ES_SH(ES_T0+0x3au,0);
    ES_SH(ES_T0+0x30u,0);ES_SH(ES_T0+0x32u,0);
    ES_SH(ES_T0+0x34u,0);ES_W(ES_T0+0xa8u,0);
    ES_V0&=ES_V1;ES_V1=0xfffffff7u;ES_V0&=ES_V1;
    ES_V1=0xffffffefu;ES_V0&=ES_V1;
    ES_V0&=ES_A2;ES_V0&=ES_A1;ES_V0&=ES_A0;
    ES_W(ES_T0+0x3cu,ES_V0);
    ES_V0=ES_R(ES_T0+0x40u);ES_V1=0xffffff03u;ES_V0&=ES_V1;
    ES_V1=0xffffe0ffu;ES_V0&=ES_V1;ES_V0&=ES_A3;
    ES_W(ES_T0+0x40u,ES_V0);ES_V0=ES_T0+0xa8u;
    ES_W(ES_V0+4u,0);ES_W(ES_V0+8u,0);
    ES_SB(ES_T0+0xb0u,0);ES_SB(ES_T0+0xafu,0);
    ES_V0=0x80060000u;ES_V0=ES_R(0x80059198u);ES_V0++;
    ES_V1=ES_V0<<14;
    battle_effect_sprite_mult(entry,ES_V0,ES_V1);ES_V1=entry->cpu->lo;
    ES_V0=ES_S(ES_T0+0x82u);
    battle_effect_sprite_mult(entry,ES_V1,ES_V0);
    ES_A0=0xfff1ffffu;ES_V0=0xfff80000u;
    ES_V1=ES_R(ES_T0+0xacu);ES_V0|=0x7fu;ES_V1&=ES_V0;
    ES_V0=ES_R(ES_T0+0xa8u);ES_V1|=0x8000u;
    ES_W(ES_T0+0xacu,ES_V1);ES_V0&=ES_A0;ES_A1=entry->cpu->lo;
    /* The branch-delay store sees the unadjusted MFLO value in A1. */
    ES_W(ES_T0+0xa8u,ES_V0);
    if(ES_A1&0x80000000u)ES_A1+=0xfffu;
    ES_V0=battle_effect_sprite_sra12(ES_A1);
    ES_W(ES_T0+0x1cu,ES_V0);ES_V0=0x10u;
    ES_W(ES_T0+0x64u,0);ES_W(ES_T0+0x70u,0);
    ES_W(ES_T0+0x44u,0);ES_W(ES_T0+0x68u,0);
    ES_SH(ES_T0+0x80u,0);ES_SB(ES_T0+0x8cu,ES_V0);
    ES_SH(ES_T0+0x84u,0);ES_W(ES_T0+0x6cu,0);
    ES_W(ES_T0+0x50u,0);
}

static void battle_effect_sprite_clear_transform(BattleEntryCall *entry)
{
    ES_SH(ES_A0,0);ES_SH(ES_A0+2u,0);ES_SH(ES_A0+4u,0);
    ES_W(ES_A0+0x2cu,0);
}

static void battle_effect_sprite_call_clear(BattleEntryCall *entry)
{
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,0x8002393cu);
    int rc=PcPortMipsRun(&callee,0x8002393cu,0x800239bcu,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED)
        battle_entry_fail(entry,"effect sprite transform child stopped",0x8002393cu);
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending effect sprite transform return",0x8002393cu);
}

static void battle_effect_sprite_bind_transform(BattleEntryCall *entry)
{
    ES_SP-=0x18u;ES_W(ES_SP+0x10u,ES_S0);ES_S0=ES_A0;
    ES_A0=ES_S0+0xb4u;ES_W(ES_SP+0x14u,ES_RA);
    ES_RA=0x800239bcu;ES_W(ES_S0+0x20u,ES_A0);
    battle_effect_sprite_call_clear(entry);
    ES_V1=ES_R(ES_S0+0x20u);ES_V0=ES_S0+0xf4u;
    ES_W(ES_S0+0x7cu,ES_V0);ES_V0=ES_S0+0x124u;
    ES_W(ES_V1+0x34u,ES_V0);
    ES_V1=ES_R(ES_S0+0x20u);ES_V0=ES_S0+0x110u;
    ES_W(ES_S0+0x24u,ES_V0);ES_W(ES_V1+0x38u,0);
    /* RA is pending until the following saved-S0 read succeeds. */
    uint32_t saved_ra=ES_R(ES_SP+0x14u),saved_s0=ES_R(ES_SP+0x10u);
    ES_RA=saved_ra;ES_S0=saved_s0;ES_SP+=0x18u;
}

static void battle_effect_sprite_actor_scale(BattleEntryCall *entry)
{
    ES_A0<<=2;ES_AT=0x800d0000u;ES_AT+=ES_A0;
    ES_A0=ES_R(ES_AT+0x3368u);ES_V0=0;
    if(!ES_A0)return;
    /* Both adjacent loads retain their original delayed commit prefixes. */
    uint32_t node=ES_R(ES_A0+4u),scale=ES_S(ES_A0+0x1cu);
    ES_V0=node;
    uint32_t component=ES_S(ES_V0+0x4eu);
    ES_V1=scale;ES_V0=component;
    battle_effect_sprite_mult(entry,ES_V1,ES_V0);ES_V0=entry->cpu->lo;
    ES_V1=ES_S(ES_A0+0x24u);ES_V0=battle_effect_sprite_sra12(ES_V0);
    battle_effect_sprite_mult(entry,ES_V1,ES_V0);ES_V0=entry->cpu->lo;
    ES_V0=battle_effect_sprite_sra12(ES_V0);
}

static void battle_effect_sprite_set_scale(BattleEntryCall *entry)
{
    ES_V0=ES_R(ES_A0+0x20u);ES_V1=0x10000000u;
    if(!ES_V0)return;
    ES_SH(ES_A0+0x2cu,ES_A1);ES_SH(ES_V0+0xau,ES_A1);
    ES_SH(ES_V0+8u,ES_A1);ES_SH(ES_V0+6u,ES_A1);
    ES_V0=ES_R(ES_A0+0x3cu);ES_V0|=ES_V1;
    ES_W(ES_A0+0x3cu,ES_V0);
}

static int battle_effect_sprite_try_host(BattleMipsRuntime *runtime,
                                         PcPortMipsCpu *cpu,uint32_t target)
{
    const char *expected;unsigned bytes;
    switch(target) {
    case 0x80023804u:bytes=312;expected="5d7d436c085a1c0ccc14fc5143013b7cb75079bbfd9672b1f3a8a2787278ff69";break;
    case 0x800239a0u:bytes=84;expected="d980bb631fe0eb3f9962e72931fa36427d64d7257873087e08792c586dd6f75c";break;
    case 0x8002393cu:bytes=20;expected="59e7fd69ad0c04193fa5da2b38106bae438f3050973a4fbe68f16127b92fb682";break;
    case 0x800aa600u:bytes=80;expected="d584122fe8c81c526d709b7c08cd12962f822fa30cb7bb6af89dafc9733641bd";break;
    case 0x80022000u:bytes=56;expected="daa73a955905fe34d31204c01207f55d4da7dadd4419a5437ba90afcd23d70db";break;
    default:return 0;
    }
    if(runtime->force_interpret||!runtime->initialized||g_ActiveBattleRuntime!=runtime||
       runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_EFFECT_SPRITE_DEPS");
    if(control&&strcmp(control,"0")==0)return 0;
    const uint8_t *code=PSX_ADDR(target);
    if(target<BATTLE_BASE) {
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
        size_t offset=XENO_RD_SLUS_OFF(target);
        if(!image||!image->bytes||image->size<offset+bytes)return 0;
        code=image->bytes+offset;
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||
           file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    switch(target) {
    case 0x80023804u:battle_effect_sprite_initialize(&entry);break;
    case 0x800239a0u:battle_effect_sprite_bind_transform(&entry);break;
    case 0x8002393cu:battle_effect_sprite_clear_transform(&entry);break;
    case 0x800aa600u:battle_effect_sprite_actor_scale(&entry);break;
    case 0x80022000u:battle_effect_sprite_set_scale(&entry);break;
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef ES_R
#undef ES_H
#undef ES_S
#undef ES_W
#undef ES_SH
#undef ES_SB
#undef ES_AT
#undef ES_V0
#undef ES_V1
#undef ES_A0
#undef ES_A1
#undef ES_A2
#undef ES_A3
#undef ES_T0
#undef ES_S0
#undef ES_SP
#undef ES_RA
