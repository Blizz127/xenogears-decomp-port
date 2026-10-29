/* Complete actor target selector and its first-set-bit service. Selectors
 * return an index byte as well as writing a truncated halfword mask. Guest
 * addresses, stack aliases and ordered data faults retain their original
 * behavior; these bodies do not fetch or decode CPU instructions. */
#define TS_R(a) battle_entry_read(entry,(a),4)
#define TS_H(a) battle_entry_read(entry,(a),2)
#define TS_B(a) battle_entry_read(entry,(a),1)
#define TS_W(a,v) battle_entry_write(entry,(a),4,(v))
#define TS_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define TS_V0 (entry->cpu->gpr[2])
#define TS_V1 (entry->cpu->gpr[3])
#define TS_A0 (entry->cpu->gpr[4])
#define TS_A1 (entry->cpu->gpr[5])
#define TS_A2 (entry->cpu->gpr[6])
#define TS_S0 (entry->cpu->gpr[16])
#define TS_SP (entry->cpu->gpr[29])
#define TS_RA (entry->cpu->gpr[31])

static void battle_target_selector_first_bit_run(BattleEntryCall *entry)
{
    TS_V1=0;TS_A0=0x800c0000u;
    TS_A0=TS_H(TS_A0+0x3e30u);
    do {
        /* A0 is the zero-extended halfword, so arithmetic and logical
         * shifts agree throughout the original thirteen-bit scan. */
        TS_V0=(TS_A0>>(TS_V1&31u))&1u;
        if(TS_V0)break;
        TS_V1++;TS_V0=(uint32_t)((int32_t)TS_V1<13);
    } while(TS_V0);
    TS_V0=TS_V1;
}

static void battle_target_selector_call(BattleEntryCall *entry)
{
    TS_RA=0x800af45cu;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,0x800af400u);
    int rc=PcPortMipsRun(&callee,0x800af400u,0x800af45cu,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-target-selector] callee 0x800af400 from 0x800af454: %s\n",callee.error);
        battle_entry_fail(entry,"target-selector first-bit callee stopped",0x800af400u);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending target-selector first-bit return",0x800af400u);
}

static void battle_target_selector_run(BattleEntryCall *entry)
{
    TS_SP-=0x18u;
    TS_W(TS_SP+0x10u,TS_S0);TS_S0=TS_A2;
    TS_V1=TS_A1&0xffu;TS_V0=0xffu;
    TS_W(TS_SP+0x14u,TS_RA);
    switch(TS_V1) {
    case 0xffu:
        battle_target_selector_call(entry);
        TS_A1=TS_V0;
        break;
    case 0xfeu:
        TS_V0=0xfdu;TS_A1=0x800c0000u;
        TS_A1=TS_B(TS_A1+0x3d40u);
        break;
    case 0xfdu:
        TS_V0=0xf9u;TS_A1=TS_B(TS_A0+0x20u);
        break;
    case 0xf9u:
        TS_V0=0xfcu;TS_A1=TS_B(TS_A0+0x20u);
        break;
    case 0xfcu:
        TS_V0=0xfau;TS_A1=TS_B(TS_A0+0x21u);
        break;
    case 0xfau:
        TS_A1=0x1fu;
        break;
    case 0xf8u:
        TS_V0=0xf7u;TS_V0=TS_B(TS_A0+0x20u);
        TS_V0<<=1;TS_A1=TS_V0+0xdu;
        break;
    case 0xf7u:
        TS_V0=TS_A1&0xffu;TS_V0=TS_B(TS_A0+0x20u);
        TS_V0<<=1;TS_A1=TS_V0+0xeu;
        break;
    default:
        break;
    }
    TS_V0=TS_A1&0xffu;
    TS_V1=1u<<(TS_V0&31u);
    TS_SH(TS_S0,TS_V1);
    /* The output may overlap either saved word. RA's load is still
     * pending when the S0 access starts, including when that access faults. */
    uint32_t saved_ra=TS_R(TS_SP+0x14u);
    uint32_t saved_s0=TS_R(TS_SP+0x10u);
    TS_RA=saved_ra;TS_SP+=0x18u;TS_S0=saved_s0;
}

static int battle_target_selector_try_host(BattleMipsRuntime *runtime,
                                             PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct {uint32_t target,bytes;const char *sha;} bodies[]={
        {0x800af438u,224u,"d886f9d31b12dc1934b50bcaeb611d2358a2a3550b205432bb9a9a48771601c8"},
        {0x800af400u,56u,"d8f1f9a0bfa426fc0272bb8d71bde1fa7c5313bf132d67c5e19118e10095d065"}
    };
    unsigned index=0;while(index<2u&&bodies[index].target!=target)index++;
    if(index==2u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_TARGET_SELECTOR");
    if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),bodies[index].bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[index].sha[i*2]||
           file1_hex_digit(digest[i]&15u)!=bodies[index].sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    if(target==0x800af438u)battle_target_selector_run(&entry);
    else battle_target_selector_first_bit_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef TS_R
#undef TS_H
#undef TS_B
#undef TS_W
#undef TS_SH
#undef TS_V0
#undef TS_V1
#undef TS_A0
#undef TS_A1
#undef TS_A2
#undef TS_S0
#undef TS_SP
#undef TS_RA
