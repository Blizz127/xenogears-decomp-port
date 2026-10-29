/* Staged complete startup slot loop and sprite binding body. Deeper CPU
 * routines retain their own native/refusal boundary; no legacy allowlist. */
#define SA_R(a) battle_entry_read(entry,(a),4)
#define SA_H(a) battle_entry_read(entry,(a),2)
#define SA_S(a) ((uint32_t)(int32_t)(int16_t)SA_H(a))
#define SA_B(a) battle_entry_read(entry,(a),1)
#define SA_W(a,v) battle_entry_write(entry,(a),4,(v))
#define SA_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define SA_V0 (entry->cpu->gpr[2])
#define SA_V1 (entry->cpu->gpr[3])
#define SA_A0 (entry->cpu->gpr[4])
#define SA_A1 (entry->cpu->gpr[5])
#define SA_A2 (entry->cpu->gpr[6])
#define SA_A3 (entry->cpu->gpr[7])
#define SA_S0 (entry->cpu->gpr[16])
#define SA_S1 (entry->cpu->gpr[17])
#define SA_S2 (entry->cpu->gpr[18])
#define SA_S3 (entry->cpu->gpr[19])
#define SA_S4 (entry->cpu->gpr[20])
#define SA_SP (entry->cpu->gpr[29])
#define SA_RA (entry->cpu->gpr[31])

static void battle_startup_actors_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    SA_RA=site+8u;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-startup-actors] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"startup actors callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending startup actors callee return",target);
}

static void battle_startup_actors_slots(BattleEntryCall *entry)
{
    uint32_t p,q;
    SA_SP-=0x20u;SA_W(SA_SP+0x10u,SA_S0);SA_S0=3;SA_V0=0x800c0000u;
    SA_W(SA_SP+0x18u,SA_S2);SA_S2=SA_V0+0x3eb0u;
    SA_W(SA_SP+0x14u,SA_S1);SA_S1=SA_S2+0x54u;SA_W(SA_SP+0x1cu,SA_RA);
    do {
        SA_A1=SA_B(SA_S1+6u);SA_V0=(int32_t)SA_A1<8;
        int valid=SA_V0!=0;SA_A1+=3u;
        if(valid){
            SA_V0=SA_A1<<1;SA_V0+=SA_A1;SA_V0<<=2;SA_V0+=SA_S2;
            SA_V1=0x8000u;SA_V0+=SA_V1;SA_V0=SA_R(SA_V0+0xd24u);
            valid=SA_V0!=0;SA_A0=SA_S0;
            if(valid){SA_A2=1;battle_startup_actors_call(entry,0x801e67a4u,0x801e6770u);}
        }
        SA_S0++;SA_V0=11;int again=SA_S0!=SA_V0;SA_S1+=0x1cu;
        if(!again)break;
    }while(1);
    p=SA_R(SA_SP+0x1cu);q=SA_R(SA_SP+0x18u);SA_RA=p;
    p=SA_R(SA_SP+0x14u);SA_S2=q;q=SA_R(SA_SP+0x10u);SA_S1=p;
    SA_SP+=0x20u;SA_S0=q;
}

static void battle_startup_actors_store_left(BattleEntryCall *entry,uint32_t address,uint32_t value)
{
    unsigned shift=(3u-(address&3u))*8u;
    uint32_t old=SA_R(address&~3u),mask=~(UINT32_MAX>>shift);
    SA_W(address&~3u,(old&mask)|(value>>shift));
}
static void battle_startup_actors_store_right(BattleEntryCall *entry,uint32_t address,uint32_t value)
{
    unsigned shift=(address&3u)*8u;
    uint32_t old=SA_R(address&~3u),mask=~(UINT32_MAX<<shift);
    SA_W(address&~3u,(old&mask)|(value<<shift));
}

static void battle_startup_actors_bind(BattleEntryCall *entry)
{
    uint32_t p,q,r;
    SA_SP-=0x50u;SA_W(SA_SP+0x40u,SA_S2);SA_S2=SA_A0;
    SA_W(SA_SP+0x38u,SA_S0);SA_S0=SA_A1<<1;SA_S0+=SA_A1;
    SA_W(SA_SP+0x44u,SA_S3);SA_S3=0x800c3eb0u;SA_S0<<=2;SA_S0+=SA_S3;
    SA_V0=0x8000u;SA_S0+=SA_V0;SA_W(SA_SP+0x4cu,SA_RA);
    SA_W(SA_SP+0x48u,SA_S4);SA_W(SA_SP+0x3cu,SA_S1);
    p=SA_S(SA_S0+0xd28u);q=SA_S(SA_S0+0xd2au);SA_A3=p;SA_V0=0x20u;SA_V1=q;
    SA_W(SA_SP+0x14u,SA_V0);SA_W(SA_SP+0x18u,0);SA_W(SA_SP+0x1cu,0);
    SA_W(SA_SP+0x20u,0);SA_W(SA_SP+0x24u,SA_A2);SA_W(SA_SP+0x28u,0);
    SA_W(SA_SP+0x2cu,0);SA_W(SA_SP+0x30u,0);SA_W(SA_SP+0x10u,SA_V1);
    p=SA_R(SA_S0+0xd2cu);SA_A1=0;SA_V0=p;SA_W(SA_SP+0x34u,SA_V0);
    SA_A0=SA_R(SA_S0+0xd24u);SA_A2=SA_S2+0x1c0u;
    battle_startup_actors_call(entry,0x800ba984u,0x801e6824u);
    SA_A2=0x3fffffffu;SA_S1=SA_S2<<3;
    p=SA_R(SA_V0+4u);q=SA_H(SA_S0+0xd2au);SA_S4=p;
    r=SA_R(SA_S4+0x24u);SA_V1=q;SA_S1-=SA_S2;SA_A0=r;SA_SH(SA_A0+6u,SA_V1);
    p=SA_R(SA_S4+0x24u);q=SA_H(SA_S0+0xd28u);SA_A0=p;
    SA_S1<<=2;SA_V1=q;SA_SH(SA_A0+4u,SA_V1);
    p=SA_R(SA_S4+0x7cu);q=SA_R(SA_S4+0x24u);SA_A0=p;
    SA_S1+=SA_S3;SA_V1=q;
    /* LWL/LWR merge forwarding: a fault on the second read does not
     * retire the first partial load. Both reads occur even when aligned. */
    uint32_t address=SA_V1+7u;unsigned shift=(3u-(address&3u))*8u;
    p=SA_R(address&~3u);address=SA_V1+4u;
    q=SA_R(address&~3u);SA_A1=(SA_A1&~(UINT32_MAX<<shift))|(p<<shift);
    shift=(address&3u)*8u;SA_A1=(SA_A1&~(UINT32_MAX>>shift))|(q>>shift);
    battle_startup_actors_store_left(entry,SA_A0+0x11u,SA_A1);
    battle_startup_actors_store_right(entry,SA_A0+0xeu,SA_A1);
    SA_A0=SA_S2<<2;SA_A0+=SA_S3;SA_V1=0x8c8cu;
    p=SA_R(SA_V0+4u);SA_V1=SA_A0+SA_V1;SA_A1=p;SA_W(SA_V1,SA_A1);
    SA_V1=0x8cb8u;SA_A0+=SA_V1;SA_W(SA_A0,SA_V0);SA_V0=SA_S2<<30;
    p=SA_R(SA_S4+0xa8u);q=SA_R(SA_S4+0xacu);SA_A0=p;
    SA_A0&=SA_A2;SA_V1=q;SA_A0|=SA_V0;SA_V0=0xfffffffcu;SA_V1&=SA_V0;
    SA_V0=SA_S2>>2;SA_V0&=3u;SA_V1|=SA_V0;
    SA_W(SA_S4+0xa8u,SA_A0);SA_W(SA_S4+0xacu,SA_V1);
    p=SA_S(SA_S1+0xeu);q=SA_S(SA_S1+0x10u);SA_A1=p;SA_A2=q;SA_A0=SA_S4;
    battle_startup_actors_call(entry,0x80021d3cu,0x801e68d8u);
    p=SA_B(SA_S1+0xau);SA_A0=SA_S4;SA_S0=p;SA_S0=SA_S0!=0;SA_S0<<=11;
    SA_A1=SA_S0;battle_startup_actors_call(entry,0x800223b0u,0x801e68f0u);
    SA_A0=SA_S4;SA_A1=SA_S0;battle_startup_actors_call(entry,0x80021fe0u,0x801e68fcu);
    SA_V0=SA_B(SA_S1+7u);if(SA_V0)SA_SH(SA_S4+0x9eu,0);
    p=SA_R(SA_SP+0x4cu);q=SA_R(SA_SP+0x48u);SA_RA=p;
    p=SA_R(SA_SP+0x44u);SA_S4=q;q=SA_R(SA_SP+0x40u);SA_S3=p;
    p=SA_R(SA_SP+0x3cu);SA_S2=q;q=SA_R(SA_SP+0x38u);SA_S1=p;
    SA_SP+=0x50u;SA_S0=q;
}

static int battle_startup_actors_identity(uint32_t address,unsigned bytes,const char *sha)
{
    const uint8_t *code;
    if(address==0x80021d3cu){
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
        size_t offset=XENO_RD_SLUS_OFF(address);
        if(!image||!image->bytes||offset>image->size||bytes>image->size-offset)return 0;
        code=image->bytes+offset;
    }else code=PSX_ADDR(address);
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<32;i++)
        if(file1_hex_digit(digest[i]>>4)!=sha[i*2]||file1_hex_digit(digest[i]&15u)!=sha[i*2+1])return 0;
    return 1;
}
/* No startup-actors guest edge is left unconverted: 800BA984 is an adopted
 * leaf (h-frontier step 7).  The two main-executable edges 800223B0 (ra
 * 801E68F8) and 80021FE0 (ra 801E6904) are decided by
 * battle_startup_actors_boundary in battle_mips_runtime.c from the boundary
 * audit: they bind the legacy body, disclosed, and 800223B0 refuses when the
 * sprite holds a guest address.  Kept as the hook for a future refusing edge. */
static int battle_startup_actors_unconverted(uint32_t target,uint32_t return_pc)
{
    (void)target;(void)return_pc;return 0;
}
static int battle_startup_actors_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if((target!=0x801e6710u&&target!=0x801e67a4u&&target!=0x80021d3cu)||
       runtime->force_interpret||!runtime->initialized||g_ActiveBattleRuntime!=runtime||
       runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_STARTUP_ACTORS");
    if(!control||strcmp(control,"1"))return 0;
    if(target==0x80021d3cu){
        if(!battle_startup_actors_identity(target,20u,"94216a3f7083803d96bb45294f2f3ab0cb976ef0edd0ff3e3d4e0c0c620037ec"))return 0;
    }else{
        if(!battle_startup_actors_identity(target,target==0x801e6710u?148u:408u,
           target==0x801e6710u?"f9f7de88b5ffce865a52c234b7c3a827167b3c68ff62e4fe2fe13d7914f687ef":
           "4d051888aa898a8e1dec094923d5ead68d3b69976b968dfed02ceeaff31300d5")||
           !battle_startup_actors_identity(0x801e4000u,32u,"6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140")||
           !battle_startup_actors_identity(0x801e5840u,228u,"e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2"))return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure))return -1;
    if(target==0x801e6710u)battle_startup_actors_slots(&entry);
    else if(target==0x801e67a4u)battle_startup_actors_bind(&entry);
    else{
        cpu->gpr[6]<<=16;cpu->gpr[5]<<=16;
        battle_entry_write(&entry,cpu->gpr[4]+8u,4,cpu->gpr[6]);
        battle_entry_write(&entry,cpu->gpr[4],4,cpu->gpr[5]);
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef SA_R
#undef SA_H
#undef SA_S
#undef SA_B
#undef SA_W
#undef SA_SH
#undef SA_V0
#undef SA_V1
#undef SA_A0
#undef SA_A1
#undef SA_A2
#undef SA_A3
#undef SA_S0
#undef SA_S1
#undef SA_S2
#undef SA_S3
#undef SA_S4
#undef SA_SP
#undef SA_RA
