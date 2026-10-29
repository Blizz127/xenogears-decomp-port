/* Model-node construction and its packed-buffer/texture/cleanup helpers.
 * These are structured C bodies, with canonical 32-bit data accesses. Main
 * executable identity comes from the verified asset cache, never game .text.
 * Packet generation remains an explicit dependency boundary in the bridge. */
#define MN_R(a) battle_entry_read(entry,(a),4)
#define MN_H(a) battle_entry_read(entry,(a),2)
#define MN_W(a,v) battle_entry_write(entry,(a),4,(v))
#define MN_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define MN_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define MN_AT (entry->cpu->gpr[1])
#define MN_V0 (entry->cpu->gpr[2])
#define MN_V1 (entry->cpu->gpr[3])
#define MN_A0 (entry->cpu->gpr[4])
#define MN_A1 (entry->cpu->gpr[5])
#define MN_A2 (entry->cpu->gpr[6])
#define MN_A3 (entry->cpu->gpr[7])
#define MN_T0 (entry->cpu->gpr[8])
#define MN_S0 (entry->cpu->gpr[16])
#define MN_S1 (entry->cpu->gpr[17])
#define MN_S2 (entry->cpu->gpr[18])
#define MN_S3 (entry->cpu->gpr[19])
#define MN_S4 (entry->cpu->gpr[20])
#define MN_S5 (entry->cpu->gpr[21])
#define MN_S6 (entry->cpu->gpr[22])
#define MN_S7 (entry->cpu->gpr[23])
#define MN_SP (entry->cpu->gpr[29])
#define MN_FP (entry->cpu->gpr[30])
#define MN_RA (entry->cpu->gpr[31])

static void battle_model_nodes_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    MN_RA=site+8u;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-model-nodes] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"model-node callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending model-node callee return load",target);
}

static void battle_model_buffers_run(BattleEntryCall *entry)
{
    MN_SP-=0x20u;uint32_t sp=MN_SP;
    MN_W(sp+0x10u,MN_S0);MN_S0=MN_A0;MN_W(sp+0x14u,MN_S1);MN_S1=MN_A1;
    MN_W(sp+0x18u,MN_S2);MN_S2=MN_A2;MN_W(sp+0x1cu,MN_RA);MN_A0=0x25u;
    battle_model_nodes_call(entry,0x800324b8u,0x8002cb74u);
    uint32_t size=MN_R(MN_S0+0x34u);MN_A1=0;MN_A0=size<<1;
    battle_model_nodes_call(entry,0x80031bdcu,0x8002cb84u);
    MN_W(MN_S1,MN_V0);MN_V1=MN_R(MN_S0+0x34u);MN_V0+=MN_V1;MN_W(MN_S2,MN_V0);
    uint32_t ra=MN_R(sp+0x1cu),s2=MN_R(sp+0x18u);MN_RA=ra;
    uint32_t s1=MN_R(sp+0x14u);MN_S2=s2;
    uint32_t s0=MN_R(sp+0x10u);MN_S1=s1;MN_SP=sp+0x20u;MN_S0=s0;
}

static void battle_model_texture_run(BattleEntryCall *entry,int clut)
{
    MN_SP-=0x18u;uint32_t sp=MN_SP;
    if(clut) {
        MN_A0&=0xffffu;MN_W(sp+0x10u,MN_RA);MN_A1&=0xffffu;
        battle_model_nodes_call(entry,0x80043a58u,0x8002cc80u);
        MN_V0&=0xfff0u;MN_AT=0x80060000u;MN_W(0x80059314u,MN_V0);
        MN_AT=0x80050000u;MN_W(0x8005010cu,0);
    } else {
        MN_A2=MN_A0&0xffffu;MN_A3=MN_A1&0xffffu;MN_A0=0;MN_W(sp+0x10u,MN_RA);MN_A1=0;
        battle_model_nodes_call(entry,0x80043a1cu,0x8002cc24u);
        MN_V0&=0x1fu;MN_AT=0x80060000u;MN_W(0x80059310u,MN_V0);
        MN_V0=1;MN_AT=0x80050000u;MN_W(0x80050108u,MN_V0);
    }
    uint32_t ra=MN_R(sp+0x10u);MN_SP=sp+0x18u;MN_RA=ra;
}

static void battle_model_nodes_free_run(BattleEntryCall *entry)
{
    MN_SP-=0x28u;uint32_t sp=MN_SP;
    MN_W(sp+0x20u,MN_S2);MN_S2=MN_A0;MN_W(sp+0x24u,MN_RA);
    MN_W(sp+0x1cu,MN_S1);MN_W(sp+0x18u,MN_S0);
    if(MN_S2) {
        MN_V0=MN_H(MN_S2+0xau);MN_S1=0;
        if((int32_t)MN_V0>0) {
            MN_S0=MN_S2+0x6cu;
            do {
                MN_A0=MN_R(MN_S0-4u);MN_S1++;
                if(MN_A0) {
                    battle_model_nodes_call(entry,0x800320e8u,0x8009f748u);
                    MN_W(MN_S0-4u,0);MN_W(MN_S0,0);
                }
                MN_V0=MN_H(MN_S2+0xau);MN_V0=(int32_t)MN_S1<(int32_t)MN_V0;MN_S0+=124u;
            } while(MN_V0);
        }
        MN_SH(MN_S2+0xau,0);MN_A0=MN_S2;
        battle_model_nodes_call(entry,0x800320e8u,0x8009f770u);
    }
    uint32_t ra=MN_R(sp+0x24u),s2=MN_R(sp+0x20u);MN_RA=ra;
    uint32_t s1=MN_R(sp+0x1cu);MN_S2=s2;
    uint32_t s0=MN_R(sp+0x18u);MN_S1=s1;MN_SP=sp+0x28u;MN_S0=s0;
}

static void battle_model_nodes_run(BattleEntryCall *entry)
{
    MN_SP-=0x50u;uint32_t sp=MN_SP;
    uint32_t t0=MN_H(sp+0x60u);MN_W(sp+0x48u,MN_FP);MN_T0=t0;
    uint32_t fp=MN_H(sp+0x64u);MN_W(sp+0x38u,MN_S4);MN_FP=fp;
    MN_S4=MN_A0;MN_W(sp+0x28u,MN_S0);MN_S0=MN_A1;MN_W(sp+0x44u,MN_S7);
    uint32_t s7=MN_H(sp+0x68u);MN_A0=4;MN_S7=s7;MN_W(sp+0x40u,MN_S6);
    uint32_t s6=MN_H(sp+0x6cu);MN_A1=0;MN_S6=s6;MN_W(sp+0x4cu,MN_RA);
    MN_W(sp+0x3cu,MN_S5);MN_W(sp+0x34u,MN_S3);MN_W(sp+0x30u,MN_S2);MN_W(sp+0x2cu,MN_S1);
    MN_W(sp+0x10u,MN_A2);MN_W(sp+0x18u,MN_A3);MN_RA=0x8009eca8u;MN_SH(sp+0x20u,MN_T0);
    battle_model_nodes_call(entry,0x80032498u,0x8009eca0u);
    MN_S3=MN_S0;
    uint32_t part=MN_H(MN_S3),count=MN_R(MN_S4+4u);MN_V1=part;MN_V0=count;MN_S2=0;
    for(;;) {
        MN_V0=MN_V1<MN_V0;MN_S3+=4u;
        if(!MN_V0) {
            MN_S3-=4u;MN_V0=0xffffu;MN_S3+=4u;
            if(MN_V1!=MN_V0) {MN_S3-=4u;break;}
        }
        part=MN_H(MN_S3);count=MN_R(MN_S4+4u);MN_V1=part;MN_S2++;MN_V0=count;
    }
    uint32_t nonempty=MN_S2;MN_S2++;
    if(!nonempty) {MN_V0=0;goto finish;}
    MN_A0=((MN_S2<<5)-MN_S2)<<2;MN_A1=0;
    battle_model_nodes_call(entry,0x80031bdcu,0x8009ecfcu);
    MN_S1=MN_V0;MN_S3=MN_S0;
    if(!MN_S1) {MN_V0=0;goto finish;}
    MN_S0=MN_S1+124u;MN_S5=1;
    part=MN_H(MN_S3);uint32_t parent=MN_H(MN_S3+2u);MN_A0=part;MN_V0=1;MN_A1=parent;
    MN_SB(MN_S1+4u,MN_V0);MN_SB(MN_S1+5u,MN_V0);MN_SB(MN_S1+6u,MN_V0);
    MN_V0=0x1000u;MN_SH(MN_S1+0x4cu,MN_V0);MN_SH(MN_S1+0x4eu,MN_V0);MN_SH(MN_S1+0x50u,MN_V0);
    MN_V0=0xffffu;MN_W(MN_S1,0);MN_SB(MN_S1+7u,0);MN_SH(MN_S1+8u,MN_V0);MN_SH(MN_S1+0xau,MN_S2);
    MN_W(MN_S1+0x68u,0);MN_W(MN_S1+0x6cu,0);
    MN_SH(MN_S1+0x54u,0);MN_SH(MN_S1+0x56u,0);MN_SH(MN_S1+0x58u,0);
    MN_W(MN_S1+0x5cu,0);MN_W(MN_S1+0x60u,0);MN_W(MN_S1+0x64u,0);
    MN_W(MN_S1+0x70u,0);MN_W(MN_S1+0x74u,0);MN_W(MN_S1+0x78u,0);
    int pending_parent=0;
    for(;;) {
        count=MN_R(MN_S4+4u);if(pending_parent)MN_A1=parent;
        MN_V1=MN_A0&0xffffu;MN_V0=count;
        int valid=MN_V1<MN_V0;MN_V0=0xffffu;
        if(!valid) {valid=MN_V1==MN_V0;MN_V1=MN_A1&0xffffu;if(!valid)break;}
        else MN_V1=MN_A1&0xffffu;
        int root=MN_V1==MN_V0;MN_V0=MN_V1<<5;
        if(root)MN_W(MN_S0,0);
        else {MN_V0=((MN_V0-MN_V1)<<2)+MN_S1+124u;MN_W(MN_S0,MN_V0);}
        MN_SH(MN_S0+0xau,MN_S5);MN_S5++;MN_V0=1;
        MN_SB(MN_S0+4u,MN_V0);MN_SB(MN_S0+5u,MN_V0);MN_SB(MN_S0+7u,MN_V0);
        MN_V0=0x1000u;MN_V1=MN_A0&0xffffu;
        MN_SH(MN_S0+0x4cu,MN_V0);MN_SH(MN_S0+0x4eu,MN_V0);MN_SH(MN_S0+0x50u,MN_V0);
        MN_V0=0xffffu;MN_SB(MN_S0+6u,0);MN_SH(MN_S0+0x52u,0);MN_SH(MN_S0+8u,MN_A0);
        if(MN_V1==MN_V0) {MN_W(MN_S0+0x68u,0);MN_W(MN_S0+0x6cu,0);}
        else {
            MN_A1=MN_S0+0x68u;uint32_t items=MN_R(MN_S4);MN_S2=MN_V1<<2;MN_V0=items+MN_S2;
            MN_A0=MN_R(MN_V0);MN_A2=MN_S0+0x6cu;
            battle_model_nodes_call(entry,0x8002cb54u,0x8009ee18u);
            MN_V0=MN_R(MN_S0+0x68u);
            if(!MN_V0) {
                MN_A0=MN_S1;battle_model_nodes_call(entry,0x8009f708u,0x8009ed18u);MN_V0=0;goto finish;
            }
            MN_T0=MN_R(sp+0x18u);MN_A1=MN_FP<<16;
            if(MN_T0) {
                t0=MN_H(sp+0x20u);MN_A1=(uint32_t)((int32_t)MN_A1>>16);MN_T0=t0;
                MN_A0=(uint32_t)(int32_t)(int16_t)MN_T0;
                battle_model_nodes_call(entry,0x8002cc10u,0x8009ee4cu);
                MN_A0=(uint32_t)(int32_t)(int16_t)MN_S7;MN_A1=(uint32_t)(int32_t)(int16_t)MN_S6;
                battle_model_nodes_call(entry,0x8002cc74u,0x8009ee60u);
            }
            items=MN_R(MN_S4);uint32_t buffer=MN_R(MN_S0+0x68u);MN_V0=items;
            uint32_t mode=MN_R(sp+0x10u);MN_A1=buffer;MN_V0+=MN_S2;MN_A2=mode;
            MN_A0=MN_R(MN_V0);
            battle_model_nodes_call(entry,0x8002c8ccu,0x8009ee7cu);
            items=MN_R(MN_S4);uint32_t dest=MN_R(MN_S0+0x6cu);MN_V0=items+MN_S2;MN_A0=dest;
            uint32_t model=MN_R(MN_V0);buffer=MN_R(MN_S0+0x68u);MN_V0=model;
            uint32_t bytes=MN_R(MN_V0+0x34u);MN_A1=buffer;MN_A2=bytes;
            battle_model_nodes_call(entry,0x8003f968u,0x8009ee9cu);
        }
        MN_SH(MN_S0+0x54u,0);MN_SH(MN_S0+0x56u,0);MN_SH(MN_S0+0x58u,0);
        MN_W(MN_S0+0x5cu,0);MN_W(MN_S0+0x60u,0);MN_W(MN_S0+0x64u,0);
        MN_W(MN_S0+0x70u,0);MN_W(MN_S0+0x74u,0);MN_W(MN_S0+0x78u,0);
        MN_S0+=124u;MN_S3+=4u;
        part=MN_H(MN_S3);parent=MN_H(MN_S3+2u);MN_A0=part;
        /* The parent load commits only after the next count read. */
        pending_parent=1;
    }
    MN_V0=MN_S1;
finish:
    {
        uint32_t ra=MN_R(sp+0x4cu),fp_saved=MN_R(sp+0x48u);MN_RA=ra;
        uint32_t saved=MN_R(sp+0x44u);MN_FP=fp_saved;
        uint32_t next=MN_R(sp+0x40u);MN_S7=saved;
        saved=MN_R(sp+0x3cu);MN_S6=next;
        next=MN_R(sp+0x38u);MN_S5=saved;
        saved=MN_R(sp+0x34u);MN_S4=next;
        next=MN_R(sp+0x30u);MN_S3=saved;
        saved=MN_R(sp+0x2cu);MN_S2=next;
        next=MN_R(sp+0x28u);MN_S1=saved;MN_SP=sp+0x50u;MN_S0=next;
    }
}

static int battle_model_nodes_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct {uint32_t target,bytes;const char *sha;} bodies[]={
        {0x8009ec4cu,752u,"18b036f8e54ca72ef107ace2ae2e9a7deb088750a4133da8cd710d3cfb4f4bdf"},
        {0x8009f708u,140u,"4b9ad7ffc3d4742466f4ef70efbc8e210d78f0b8ce6dcec9ece683933a4bfcba"},
        {0x8002cb54u,104u,"50ff6379e4592a12839dd1da3ee05d582aa76ad0915582d5187d33d03db48f6c"},
        {0x8002cc10u,68u,"bc80862eb2b352493a01215dce1f0d1ecff8620f75db325d418d2f19c45c452d"},
        {0x8002cc74u,56u,"8e0e7c13d4fa41fcfb1de78afc089f9cb224ac070b68e1fc7e87d47a5a35dbfc"}
    };
    unsigned index=0;while(index<sizeof(bodies)/sizeof(bodies[0])&&bodies[index].target!=target)index++;
    if(index==sizeof(bodies)/sizeof(bodies[0])||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MODEL_NODES");if(control&&strcmp(control,"0")==0)return 0;
    const uint8_t *code=PSX_ADDR(target);uint32_t bytes=bodies[index].bytes;
    if(target<BATTLE_BASE) {
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
        if(!image||image->size<offset+bytes)return 0;
        code=image->bytes+offset;
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[index].sha[i*2]||file1_hex_digit(digest[i]&15u)!=bodies[index].sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x8009ec4cu)battle_model_nodes_run(&entry);
    else if(target==0x8009f708u)battle_model_nodes_free_run(&entry);
    else if(target==0x8002cb54u)battle_model_buffers_run(&entry);
    else battle_model_texture_run(&entry,target==0x8002cc74u);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef MN_R
#undef MN_H
#undef MN_W
#undef MN_SH
#undef MN_SB
#undef MN_AT
#undef MN_V0
#undef MN_V1
#undef MN_A0
#undef MN_A1
#undef MN_A2
#undef MN_A3
#undef MN_T0
#undef MN_S0
#undef MN_S1
#undef MN_S2
#undef MN_S3
#undef MN_S4
#undef MN_S5
#undef MN_S6
#undef MN_S7
#undef MN_SP
#undef MN_FP
#undef MN_RA
