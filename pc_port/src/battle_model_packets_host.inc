/* Model packet construction uses the original 40-byte descriptor data and
 * canonical 32-bit working pointers. No original instructions are executed.
 * Unconverted callbacks/dependencies are refused before the generic ABI. */
#define MP_R(a) battle_entry_read(entry,(a),4)
#define MP_H(a) battle_entry_read(entry,(a),2)
#define MP_B(a) battle_entry_read(entry,(a),1)
#define MP_S(a) ((uint32_t)(int32_t)(int16_t)MP_H(a))
#define MP_W(a,v) battle_entry_write(entry,(a),4,(v))
#define MP_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define MP_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define MP_AT (entry->cpu->gpr[1])
#define MP_V0 (entry->cpu->gpr[2])
#define MP_V1 (entry->cpu->gpr[3])
#define MP_A0 (entry->cpu->gpr[4])
#define MP_A1 (entry->cpu->gpr[5])
#define MP_A2 (entry->cpu->gpr[6])
#define MP_A3 (entry->cpu->gpr[7])
#define MP_T0 (entry->cpu->gpr[8])
#define MP_S0 (entry->cpu->gpr[16])
#define MP_S1 (entry->cpu->gpr[17])
#define MP_S2 (entry->cpu->gpr[18])
#define MP_S3 (entry->cpu->gpr[19])
#define MP_S4 (entry->cpu->gpr[20])
#define MP_S5 (entry->cpu->gpr[21])
#define MP_SP (entry->cpu->gpr[29])
#define MP_RA (entry->cpu->gpr[31])

static void battle_model_packets_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    MP_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-model-packets] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"model-packet callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending model-packet return load",target);
}

static void battle_model_packet_dispatch_run(BattleEntryCall *entry)
{
    MP_SP-=0x30u;uint32_t sp=MP_SP;
    MP_W(sp+0x10u,MP_S0);MP_S0=MP_A0;MP_W(sp+0x28u,MP_RA);MP_W(sp+0x24u,MP_S5);
    MP_W(sp+0x20u,MP_S4);MP_W(sp+0x1cu,MP_S3);MP_W(sp+0x18u,MP_S2);MP_W(sp+0x14u,MP_S1);
    uint32_t flags=MP_H(MP_S0);MP_AT=0x80060000u;MP_V0=flags;
    MP_W(0x80059424u,MP_A1);MP_V0&=1u;MP_S1=MP_A2;
    if(!MP_V0){
        MP_V0=MP_R(MP_S0+0x30u);
        if(MP_V0&&MP_S1){
            MP_A0=0x26u;battle_model_packets_call(entry,0x800324b8u,0x8002c920u);
            MP_A0=MP_R(MP_S0+0x30u);MP_A1=0;battle_model_packets_call(entry,0x80031bdcu,0x8002c92cu);
            flags=MP_H(MP_S0);MP_W(MP_S0+0x18u,MP_V0);MP_V1=flags|1u;MP_SH(MP_S0,MP_V1);
        }
    }
    uint32_t source=MP_R(MP_S0+0x14u),commands=MP_R(MP_S0+0x10u);MP_V1=source;
    uint32_t normals=MP_R(MP_S0+0xcu);MP_V0=commands;
    uint32_t vertices=MP_R(MP_S0+8u);MP_A0=normals;
    uint32_t light=MP_R(MP_S0+0x18u);MP_A1=vertices;MP_AT=0x80060000u;MP_A2=light;
    MP_W(0x80059538u,MP_V1);flags=MP_H(MP_S0);MP_AT=0x80060000u;MP_V1=flags;
    MP_W(0x80059528u,MP_V0);MP_V0=1;MP_AT=0x80060000u;MP_W(0x8005952cu,MP_A0);
    MP_AT=0x80060000u;MP_W(0x8005953cu,MP_A1);MP_AT=0x80060000u;MP_W(0x80059498u,MP_A2);
    int one=MP_S1==MP_V0;MP_V0=(int32_t)MP_S1<2;
    if(one)MP_S5=1;
    else if(MP_V0){if(!MP_S1)MP_S5=0;}
    else{
        MP_V0=2;int two=MP_S1==MP_V0;MP_V0=3;
        if(two){
            MP_V0=MP_V1&2u;int cached=MP_V0!=0;MP_V0=MP_V1&1u;
            if(cached){MP_S5=1;if(MP_V0)MP_S5=4;}
            else{MP_S5=3;if(MP_V0){MP_V0=MP_V1|2u;MP_SH(MP_S0,MP_V0);}else MP_S5=1;}
        }else{
            int three=MP_S1==MP_V0;MP_V0=MP_V1&1u;
            if(three&&MP_V0){MP_S5=3;MP_V0=MP_V1|2u;MP_SH(MP_S0,MP_V0);}
        }
    }
    uint32_t groups=MP_H(MP_S0+6u),vertices_count=MP_H(MP_S0+4u);MP_S2=groups;
    MP_V1=0x80060000u;MP_V0=vertices_count;uint32_t total=MP_R(0x800595c0u);
    MP_S2--;MP_V1=total+MP_V0;MP_V0=UINT32_MAX;MP_AT=0x80060000u;MP_W(0x800595c0u,MP_V1);
    if(MP_S2!=MP_V0){
        MP_S4=UINT32_MAX;
        do{
            MP_V1=0x80060000u;MP_V1=MP_R(0x80059528u);
            uint32_t count=MP_S(MP_V1+2u);MP_V0=MP_V1+4u;MP_S0=count;MP_AT=0x80060000u;MP_W(0x80059528u,MP_V0);
            uint32_t type=MP_B(MP_V1);MP_S0--;MP_V1=type;
            MP_V0=((MP_V1<<2)+MP_V1)<<3;MP_V1=0x8004fe50u;MP_S1=MP_V0+MP_V1;
            uint32_t callback=MP_R(MP_S1+0x18u);int empty=MP_S0==MP_S4;MP_S3=callback;MP_A2=MP_S5<<16;
            if(!empty)do{
                MP_A0=0x80060000u;source=MP_R(0x80059538u);MP_A1=0x80060000u;MP_A0=source;
                MP_A1=MP_R(0x80059528u);MP_A2=(uint32_t)((int32_t)MP_A2>>16);
                battle_model_packets_call(entry,MP_S3,0x8002ca94u);
                if(MP_V0){
                    MP_V0=0x80060000u;commands=MP_R(0x80059528u);
                    uint32_t command_stride=MP_R(MP_S1+0x1cu);MP_V0=commands;
                    uint32_t output_stride=MP_R(MP_S1+0x24u);MP_V1=command_stride;
                    uint32_t source_stride=MP_R(MP_S1+0x20u);MP_A0=output_stride;
                    MP_V0+=MP_V1;MP_A1=source_stride;MP_AT=0x80060000u;MP_W(0x80059528u,MP_V0);
                    MP_V0=0x80060000u;uint32_t output=MP_R(0x80059424u);MP_V1=0x80060000u;MP_V0=output;
                    source=MP_R(0x80059538u);MP_V0+=MP_A0;MP_V1=source+MP_A1;
                    MP_AT=0x80060000u;MP_W(0x80059424u,MP_V0);MP_AT=0x80060000u;MP_W(0x80059538u,MP_V1);MP_S0--;
                }else{
                    MP_V0=0x80060000u;source=MP_R(0x80059538u);MP_S0++;MP_V0=source+4u;
                    MP_AT=0x80060000u;MP_W(0x80059538u,MP_V0);MP_S0--;
                }
                MP_A2=MP_S5<<16;
            }while(MP_S0!=MP_S4);
            MP_S2--;
        }while(MP_S2!=MP_S4);
    }
    battle_model_packets_call(entry,0x8002ccacu,0x8002cb24u);
    uint32_t ra=MP_R(sp+0x28u),saved=MP_R(sp+0x24u);MP_RA=ra;
    uint32_t next=MP_R(sp+0x20u);MP_S5=saved;saved=MP_R(sp+0x1cu);MP_S4=next;
    next=MP_R(sp+0x18u);MP_S3=saved;saved=MP_R(sp+0x14u);MP_S2=next;
    next=MP_R(sp+0x10u);MP_S1=saved;MP_SP=sp+0x30u;MP_S0=next;
}

static void battle_model_packet_reset_run(BattleEntryCall *entry)
{MP_V0=1;MP_AT=0x80050000u;MP_W(0x80050108u,0);MP_AT=0x80050000u;MP_W(0x8005010cu,MP_V0);}

static void battle_model_packet_latch_run(BattleEntryCall *entry,int clut)
{
    uint32_t color=MP_H(MP_A0);
    if(clut){
        MP_V0=0x80050000u;MP_A0=color;uint32_t mode=MP_R(0x8005010cu);
        MP_AT=0x80060000u;MP_V0=mode;MP_SH(0x8005930cu,MP_A0);int raw=MP_V0!=0;MP_V0=MP_A0&15u;
        if(!raw){
            MP_V1=0x80060000u;uint32_t base=MP_R(0x80059314u);MP_AT=0x80060000u;MP_V1=base;
            MP_SH(0x8005930cu,MP_V0);MP_V0|=MP_V1;MP_AT=0x80060000u;MP_SH(0x8005930cu,MP_V0);
        }
    }else{
        MP_V1=0x80050000u;MP_A0=color;uint32_t mode=MP_R(0x80050108u);
        MP_V0=1;MP_V1=mode;MP_AT=0x80060000u;MP_SH(0x80059308u,MP_A0);
        int partial=MP_V1==MP_V0;MP_V0=MP_A0&0xffe0u;
        if(partial){
            MP_V1=0x80060000u;uint32_t base=MP_R(0x80059310u);MP_AT=0x80060000u;MP_V1=base;
            MP_SH(0x80059308u,MP_V0);MP_V0|=MP_V1;MP_AT=0x80060000u;MP_SH(0x80059308u,MP_V0);
        }else{MP_V0=2;if(MP_V1==MP_V0){MP_V0=0x80060000u;uint32_t base=MP_R(0x80059310u);MP_AT=0x80060000u;MP_V0=base;MP_SH(0x80059308u,MP_V0);}}
    }
}

static void battle_model_packet_control_run(BattleEntryCall *entry)
{
    MP_SP-=0x18u;uint32_t sp=MP_SP;MP_W(sp+0x10u,MP_RA);
    uint32_t code=MP_B(MP_A0+3u);MP_V0=0xc0u;MP_A1=code;MP_V1=MP_A1&0xf0u;
    int control=MP_V1==MP_V0;MP_V0=1;
    if(control){
        MP_V1=MP_A1&0xffu;MP_V0=0xc4u;int page=MP_V1==MP_V0;MP_V0=0xc8u;
        if(page){battle_model_packets_call(entry,0x8002ccc8u,0x8002cda0u);MP_V0=0;}
        else{int clut=MP_V1==MP_V0;MP_V0=1;if(clut){battle_model_packets_call(entry,0x8002cd24u,0x8002cdb0u);MP_V0=0;}}
    }
    uint32_t ra=MP_R(sp+0x10u);MP_SP=sp+0x18u;MP_RA=ra;
}

static void battle_model_packet_flat_run(BattleEntryCall *entry,int quad)
{
    MP_V0=0x80060000u;uint32_t out=MP_R(0x80059424u);MP_V1=quad?5:4;MP_V0=out;
    MP_SB(MP_V0+3u,MP_V1);MP_V1=MP_R(MP_A0);MP_W(MP_V0+4u,MP_V1);MP_V0=1;
}

static void battle_model_packet_texture_run(BattleEntryCall *entry,int quad)
{
    MP_SP-=0x18u;uint32_t sp=MP_SP;MP_W(sp+0x10u,MP_S0);MP_W(sp+0x14u,MP_RA);MP_S0=MP_A0;
    battle_model_packets_call(entry,0x8002cd64u,quad?0x8002d0f0u:0x8002d990u);
    int emit=MP_V0!=0;MP_V0=quad?9:7;
    if(emit){
        MP_A0=0x80060000u;MP_A0=MP_R(0x80059424u);MP_SB(MP_A0+3u,MP_V0);
        if(quad){MP_V0=MP_R(MP_S0);MP_W(MP_A0+4u,MP_V0);}
        MP_V0=0x80060000u;uint32_t color=MP_H(0x8005930cu),uv=MP_H(MP_S0+4u);MP_V0=color<<16;MP_V1=uv|MP_V0;MP_W(MP_A0+0xcu,MP_V1);
        MP_V0=0x80060000u;color=MP_H(0x80059308u);uv=MP_H(MP_S0+6u);MP_V0=color<<16;MP_V1=uv|MP_V0;MP_W(MP_A0+0x14u,MP_V1);
        MP_V0=MP_H(MP_S0+(quad?8u:0u));MP_SH(MP_A0+0x1cu,MP_V0);
        uint32_t last=quad?MP_H(MP_S0+0xau):MP_B(MP_S0+3u);MP_V0=1;MP_V1=last;
        if(quad)MP_SH(MP_A0+0x24u,MP_V1);else MP_SB(MP_A0+7u,MP_V1);
    }else MP_V0=0;
    uint32_t ra=MP_R(sp+0x14u),s0=MP_R(sp+0x10u);MP_RA=ra;MP_SP=sp+0x18u;MP_S0=s0;
}

/* Every configured build callback is implemented below. External lighting
 * boundaries are explicit refusals until their concrete contracts are proved. */

/* Both lit flat bodies have identical operations and shifted call sites. */
static void battle_model_packet_lit_flat_run(BattleEntryCall *entry,int quad)
{
    uint32_t delta=quad?0x18cu:0;
    MP_SP-=0x28u;uint32_t sp=MP_SP;
    MP_W(sp+0x18u,MP_S0);MP_S0=MP_A0;MP_W(sp+0x1cu,MP_S1);MP_S1=0x80060000u;
    uint32_t out=MP_R(0x80059424u);MP_V0=4;MP_S1=out;
    MP_W(sp+0x20u,MP_RA);MP_SB(MP_S1+3u,MP_V0);MP_V0=MP_A2&1u;MP_T0=MP_A1;
    int lighting=MP_V0!=0,advance=0;
    if(lighting){
        MP_V0=MP_A2&2u;MP_A3=sp+0x10u;int stream=MP_V0!=0;
        if(stream){
            MP_V1=0x80060000u;uint32_t p=MP_R(0x80059498u),color=MP_R(MP_S0);MP_V1=p;MP_V0=color;MP_W(MP_V1,MP_V0);
        }
        uint32_t a0=MP_S(MP_T0);MP_V0=0x80060000u;MP_A0=a0;
        uint32_t base=MP_R(0x8005953cu),a1=MP_S(MP_T0+2u);MP_V0=base;
        uint32_t a2=MP_S(MP_T0+4u);MP_A1=a1;
        if(stream){
            MP_A3=0x80060000u;MP_A2=a2;uint32_t p=MP_R(0x80059498u);MP_A0<<=3;MP_A3=p;
            MP_A0+=MP_V0;MP_A1=(MP_A1<<3)+MP_V0;MP_A2<<=3;MP_A3+=4u;
            MP_AT=0x80060000u;MP_W(0x80059498u,MP_A3);MP_A2+=MP_V0;
            battle_model_packets_call(entry,0x8002db84u,0x8002ce58u+delta);
            MP_A1=MP_S0;MP_A0=0x80060000u;MP_A0=MP_R(0x80059498u);MP_A2=MP_S1+4u;advance=1;
        }else{
            MP_A0<<=3;MP_A2=a2;MP_A0+=MP_V0;MP_A1=(MP_A1<<3)+MP_V0;MP_A2=(MP_A2<<3)+MP_V0;
            battle_model_packets_call(entry,0x8002db84u,0x8002ce9cu+delta);
            MP_A0=sp+0x10u;MP_A1=MP_S0;MP_A2=MP_S1+4u;
            battle_model_packets_call(entry,0x8004a260u,0x8002ceacu+delta);
        }
    }else{
        MP_V0=MP_A2&4u;MP_A1=MP_S0;lighting=MP_V0!=0;
        if(lighting){
            MP_A0=0x80060000u;uint32_t p=MP_R(0x80059498u);MP_A2=MP_S1+4u;MP_A0=p+4u;
            MP_AT=0x80060000u;MP_W(0x80059498u,MP_A0);advance=1;
        }
    }
    if(advance){
        battle_model_packets_call(entry,0x8004a260u,0x8002cee0u+delta);
        MP_V0=0x80060000u;MP_V0=MP_R(0x80059498u)+8u;MP_AT=0x80060000u;MP_W(0x80059498u,MP_V0);
    }
    if(lighting){MP_V0=MP_B(MP_S0+3u);MP_SB(MP_S1+7u,MP_V0);}
    else{MP_V0=MP_R(MP_S0);MP_W(MP_S1+4u,MP_V0);}
    MP_V0=1;uint32_t ra=MP_R(sp+0x20u),s1=MP_R(sp+0x1cu);MP_RA=ra;
    uint32_t s0=MP_R(sp+0x18u);MP_S1=s1;MP_SP=sp+0x28u;MP_S0=s0;
}

/* The two textured flat forms differ in UV count and original call sites. */
static void battle_model_packet_lit_texture_run(BattleEntryCall *entry,int quad)
{
    MP_SP-=0x30u;uint32_t sp=MP_SP;
    MP_W(sp+0x24u,MP_S3);MP_S3=MP_A0;MP_W(sp+0x1cu,MP_S1);MP_S1=MP_A1;
    MP_W(sp+0x20u,MP_S2);MP_S2=MP_A2;MP_W(sp+0x28u,MP_RA);
    uint32_t control=quad?0x8002d550u:0x8002d834u;
    MP_RA=control+8u;MP_W(sp+0x18u,MP_S0);battle_model_packets_call(entry,0x8002cd64u,control);
    int emit=MP_V0!=0;MP_V0=quad?9:7;
    if(emit){
        MP_S0=0x80060000u;MP_S0=MP_R(0x80059424u);MP_SB(MP_S0+3u,MP_V0);
        MP_V0=0x80060000u;uint32_t color=MP_H(0x8005930cu),uv=MP_H(MP_S3+4u);MP_V0=color<<16;MP_V1=uv|MP_V0;MP_W(MP_S0+0xcu,MP_V1);
        MP_V0=0x80060000u;color=MP_H(0x80059308u);uv=MP_H(MP_S3+6u);MP_V0=color<<16;MP_V1=uv|MP_V0;MP_W(MP_S0+0x14u,MP_V1);
        MP_V0=MP_H(MP_S3+(quad?8u:0u));MP_SH(MP_S0+0x1cu,MP_V0);
        if(quad){MP_V0=MP_H(MP_S3+0xau);MP_SH(MP_S0+0x24u,MP_V0);}
        MP_V0=MP_S2&1u;int lit=MP_V0!=0;MP_V0=MP_S2&2u;int stream=MP_V0!=0,normal=0;
        if(lit){
            MP_A3=sp+0x10u;uint32_t a0=MP_S(MP_S1);
            if(stream){
                MP_A3=0x80060000u;MP_A0=a0;uint32_t p=MP_R(0x80059498u);MP_V0=0x80060000u;MP_A3=p;
            }else{MP_V0=0x80060000u;MP_A0=a0;}
            uint32_t base=MP_R(0x8005953cu),a1=MP_S(MP_S1+2u);MP_V0=base;
            uint32_t a2=MP_S(MP_S1+4u);MP_A1=a1;MP_A0<<=3;MP_A2=a2;
            MP_A0+=MP_V0;MP_A1=(MP_A1<<3)+MP_V0;MP_A2=(MP_A2<<3)+MP_V0;
            battle_model_packets_call(entry,0x8002db84u,quad?(stream?0x8002d604u:0x8002d63cu):(stream?0x8002d8dcu:0x8002d914u));
            if(stream){MP_A0=0x80060000u;MP_A0=MP_R(0x80059498u);}else MP_A0=sp+0x10u;
            normal=1;
        }else{
            MP_V0=MP_S2&4u;if(MP_V0){MP_A0=0x80060000u;MP_A0=MP_R(0x80059498u);normal=1;}
        }
        if(normal){MP_A1=MP_S0+4u;battle_model_packets_call(entry,0x8004a19cu,quad?0x8002d660u:0x8002d938u);}
        MP_V0=0x80060000u;MP_V0=MP_R(0x80059498u)+8u;MP_AT=0x80060000u;MP_W(0x80059498u,MP_V0);
        uint32_t code=MP_B(MP_S3+3u);MP_V0=1;MP_V1=code;MP_SB(MP_S0+7u,MP_V1);
    }else MP_V0=0;
    uint32_t ra=MP_R(sp+0x28u),s3=MP_R(sp+0x24u);MP_RA=ra;
    uint32_t s2=MP_R(sp+0x20u);MP_S3=s3;uint32_t s1=MP_R(sp+0x1cu);MP_S2=s2;
    uint32_t s0=MP_R(sp+0x18u);MP_S1=s1;MP_SP=sp+0x30u;MP_S0=s0;
}

static void battle_model_packet_gouraud_run(BattleEntryCall *entry,int cache)
{
    MP_SP-=0x30u;uint32_t sp=MP_SP;
    MP_W(sp+0x24u,MP_S1);MP_S1=MP_A0;if(cache)MP_A3=MP_A1;MP_V0=6;
    MP_W(sp+0x20u,MP_S0);MP_S0=0x80060000u;uint32_t out=MP_R(0x80059424u);
    if(cache)MP_A2&=2u;else MP_A3=MP_S1;MP_S0=out;
    MP_W(sp+0x28u,MP_RA);MP_SB(MP_S0+3u,MP_V0);
    if(cache&&MP_A2){
        MP_V1=0x80060000u;uint32_t p=MP_R(0x80059498u),color=MP_R(MP_S1);MP_V1=p;MP_V0=color;MP_W(MP_V1,MP_V0);
        MP_V0=0x80060000u;MP_V0=MP_R(0x80059498u)+4u;MP_AT=0x80060000u;MP_W(0x80059498u,MP_V0);
    }
    uint32_t indices=cache?MP_A3:MP_A1;
    uint32_t a0=MP_S(indices),a1=MP_S(indices+2u);MP_A0=a0;
    uint32_t a2=MP_S(indices+4u);if(cache)MP_A1=a1;else MP_V1=a1;
    MP_V0=MP_S0+4u;MP_A2=a2;MP_W(sp+0x10u,MP_V0);MP_V0=MP_S0+0xcu;MP_W(sp+0x14u,MP_V0);
    MP_V0=MP_S0+0x14u;if(cache)MP_A3=MP_S1;MP_W(sp+0x18u,MP_V0);
    MP_V0=0x80060000u;uint32_t base=MP_R(0x8005952cu);MP_A0<<=3;MP_V0=base;
    if(cache)MP_A1<<=3;else MP_V1<<=3;MP_A2<<=3;MP_A0+=MP_V0;
    MP_A1=MP_V0+(cache?MP_A1:MP_V1);MP_A2+=MP_V0;
    battle_model_packets_call(entry,0x8004a280u,cache?0x8002d74cu:0x8002d7e4u);
    MP_V0=MP_B(MP_S1+3u);MP_SB(MP_S0+7u,MP_V0);MP_V0=1;
    uint32_t ra=MP_R(sp+0x28u),s1=MP_R(sp+0x24u);MP_RA=ra;
    uint32_t s0=MP_R(sp+0x20u);MP_S1=s1;MP_SP=sp+0x30u;MP_S0=s0;
}

static void battle_model_packet_gouraud_quad_run(BattleEntryCall *entry)
{
    MP_SP-=0x30u;uint32_t sp=MP_SP;
    MP_W(sp+0x28u,MP_S2);MP_S2=MP_A0;MP_W(sp+0x24u,MP_S1);MP_S1=MP_A1;MP_V0=8;
    MP_W(sp+0x20u,MP_S0);MP_S0=0x80060000u;uint32_t out=MP_R(0x80059424u);MP_A3=MP_S2;MP_S0=out;
    MP_W(sp+0x2cu,MP_RA);MP_SB(MP_S0+3u,MP_V0);
    uint32_t a0=MP_S(MP_S1),a1=MP_S(MP_S1+2u);MP_A0=a0;uint32_t a2=MP_S(MP_S1+4u);MP_A1=a1;
    MP_V0=MP_S0+4u;MP_A2=a2;MP_W(sp+0x10u,MP_V0);MP_V0=MP_S0+0xcu;MP_W(sp+0x14u,MP_V0);
    MP_V0=MP_S0+0x14u;MP_W(sp+0x18u,MP_V0);MP_V0=0x80060000u;
    uint32_t base=MP_R(0x8005952cu);MP_A0<<=3;MP_V0=base;MP_A1<<=3;MP_A2<<=3;
    MP_A0+=MP_V0;MP_A1+=MP_V0;MP_A2+=MP_V0;battle_model_packets_call(entry,0x8004a280u,0x8002d1f0u);
    MP_A1=MP_S2;MP_A2=MP_S0+0x1cu;a0=MP_S(MP_S1+6u);MP_V0=0x80060000u;MP_A0=a0;
    base=MP_R(0x8005952cu);MP_A0<<=3;MP_V0=base;MP_A0+=MP_V0;battle_model_packets_call(entry,0x8004a260u,0x8002d210u);
    MP_V0=MP_B(MP_S2+3u);MP_SB(MP_S0+7u,MP_V0);MP_V0=1;
    uint32_t ra=MP_R(sp+0x2cu),s2=MP_R(sp+0x28u);MP_RA=ra;uint32_t s1=MP_R(sp+0x24u);MP_S2=s2;
    uint32_t s0=MP_R(sp+0x20u);MP_S1=s1;MP_SP=sp+0x30u;MP_S0=s0;
}

static void battle_model_packet_gouraud_texture_run(BattleEntryCall *entry,int quad)
{
    MP_SP-=0x28u;uint32_t sp=MP_SP;
    MP_W(sp+0x20u,MP_S2);MP_S2=MP_A0;MP_W(sp+0x1cu,MP_S1);MP_S1=MP_A1;MP_W(sp+0x24u,MP_RA);
    uint32_t control=quad?0x8002d25cu:0x8002da2cu;
    MP_RA=control+8u;MP_W(sp+0x18u,MP_S0);battle_model_packets_call(entry,0x8002cd64u,control);
    int emit=MP_V0!=0;MP_V0=quad?12:9;
    if(emit){
        MP_S0=0x80060000u;MP_S0=MP_R(0x80059424u);MP_SB(MP_S0+3u,MP_V0);
        uint32_t a0=MP_S(MP_S1),a1=MP_S(MP_S1+2u);MP_A0=a0;uint32_t a2=MP_S(MP_S1+4u);MP_A1=a1;
        MP_V0=MP_S0+0x10u;MP_A2=a2;MP_W(sp+0x10u,MP_V0);MP_V0=MP_S0+0x1cu;MP_A3=MP_S0+4u;MP_W(sp+0x14u,MP_V0);
        MP_V0=0x80060000u;uint32_t base=MP_R(0x8005952cu);MP_A0<<=3;MP_V0=base;
        MP_A0+=MP_V0;MP_A1=(MP_A1<<3)+MP_V0;MP_A2=(MP_A2<<3)+MP_V0;
        battle_model_packets_call(entry,0x8004a1b8u,quad?0x8002d2b8u:0x8002da88u);
        if(quad){
            MP_A1=MP_S0+0x28u;a0=MP_S(MP_S1+6u);MP_V0=0x80060000u;MP_A0=a0;
            base=MP_R(0x8005952cu);MP_A0<<=3;MP_V0=base;MP_A0+=MP_V0;battle_model_packets_call(entry,0x8004a19cu,0x8002d2d4u);
        }
        MP_V0=0x80060000u;uint32_t color=MP_H(0x8005930cu),uv=MP_H(MP_S2+4u);MP_V0=color<<16;MP_V1=uv|MP_V0;MP_W(MP_S0+0xcu,MP_V1);
        MP_V0=0x80060000u;color=MP_H(0x80059308u);uv=MP_H(MP_S2+6u);MP_V0=color<<16;MP_V1=uv|MP_V0;MP_W(MP_S0+0x18u,MP_V1);
        MP_V0=MP_H(MP_S2+(quad?8u:0u));MP_SH(MP_S0+0x24u,MP_V0);
        if(quad){MP_V0=MP_H(MP_S2+0xau);MP_SH(MP_S0+0x30u,MP_V0);}
        uint32_t code=MP_B(MP_S2+3u);MP_V0=1;MP_V1=code;MP_SB(MP_S0+7u,MP_V1);
    }else MP_V0=0;
    uint32_t ra=MP_R(sp+0x24u),s2=MP_R(sp+0x20u);MP_RA=ra;uint32_t s1=MP_R(sp+0x1cu);MP_S2=s2;
    uint32_t s0=MP_R(sp+0x18u);MP_S1=s1;MP_SP=sp+0x28u;MP_S0=s0;
}

static void battle_model_packet_debug_run(BattleEntryCall *entry)
{
    MP_SP-=0x18u;uint32_t sp=MP_SP;MP_W(sp+0x10u,MP_S0);MP_S0=0x80060000u;
    uint32_t out=MP_R(0x80059424u);MP_W(sp+0x14u,MP_RA);MP_S0=out;MP_A0=MP_S0;
    battle_model_packets_call(entry,0x80043c60u,0x8002db10u);
    MP_A0=MP_S0;MP_A1=1;battle_model_packets_call(entry,0x80043c24u,0x8002db1cu);
    MP_A0=1;MP_A1=0;MP_A2=0x280u;MP_A3=0;battle_model_packets_call(entry,0x80043a1cu,0x8002db30u);
    MP_A0=0;MP_A1=0x1e0u;MP_V1=0x80060000u;uint32_t base=MP_R(0x80059310u);MP_V0&=0xffe0u;MP_V1=base;MP_V0|=MP_V1;
    MP_RA=0x8002db58u;MP_SH(MP_S0+0x16u,MP_V0);battle_model_packets_call(entry,0x80043a58u,0x8002db50u);
    MP_V1=MP_V0&15u;MP_A0=0x80060000u;base=MP_R(0x80059314u);MP_V0=1;MP_A0=base;MP_V1|=MP_A0;MP_SH(MP_S0+0xeu,MP_V1);
    uint32_t ra=MP_R(sp+0x14u),s0=MP_R(sp+0x10u);MP_RA=ra;MP_SP=sp+0x18u;MP_S0=s0;
}

static int battle_model_packets_target(uint32_t target)
{
    switch(target){
    case 0x8002c8ccu:
    case 0x8002ccacu:
    case 0x8002ccc8u:
    case 0x8002cd24u:
    case 0x8002cd64u:
    case 0x8002cdccu:
    case 0x8002cf34u:
    case 0x8002cf58u:
    case 0x8002d0c0u:
    case 0x8002d0e4u:
    case 0x8002d180u:
    case 0x8002d244u:
    case 0x8002d530u:
    case 0x8002d6acu:
    case 0x8002d77cu:
    case 0x8002d814u:
    case 0x8002d984u:
    case 0x8002da14u:
    case 0x8002dafcu:
        return 1;
    default:return 0;
    }
}

static int battle_model_packets_callback(uint32_t target)
{
    return battle_model_packets_target(target)&&target>=0x8002cdccu;
}

static int battle_model_packets_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct {uint32_t target,bytes;const char *sha;} bodies[]={
        {0x8002c8ccu,648u,"3c191926c53124d3c605b0a08a8e568188989296a7958d27342631e5242e5055"},
        {0x8002ccacu,28u,"1aba0faf2f62e976201f7b9bc7fe358d3aabca63131813c61481dd909c0c21ff"},
        {0x8002ccc8u,92u,"c9684c121ec89f9ab4053bb0b8fa7c7cf3c0d2b90301f976f3f6fcc014160cbf"},
        {0x8002cd24u,64u,"c2dceca829d379c43feb4a93d27c29cd69dacdda24c4af86759d20bf70ac49f1"},
        {0x8002cd64u,104u,"3af8618d51ac04d72c872da0d763c625c7ff9320bf6b38cea6d2201e56813ada"},
        {0x8002cdccu,360u,"051744d300dc807d721f7bee226cc5ed9227e085f3c8e2a954225db0b998e662"},
        {0x8002cf34u,36u,"4b3cbcd521576f4f05abb626d535e79de56263bdac1d968cbb28b03ce54224b1"},
        {0x8002cf58u,360u,"db845b6935cbbb67872f83ede80d690351fd003382efba69b8e00b540caefac4"},
        {0x8002d0c0u,36u,"e6fcd952c0c6db6c3893dbd81a1defb64ed8884a08591fe4eb6c7e77aae61a99"},
        {0x8002d0e4u,156u,"40ec16bdb9f17b58f4677b28fe2d040a9ceabd767bc5eb53f5f3d5226f8def71"},
        {0x8002d180u,196u,"81dbec3433e2d3e7c16965e0ff8440ab7cc8689a662af341bc31dcee7cd2ace6"},
        {0x8002d244u,272u,"e2385d8887ad967f44feba4790a5853e756480a186e35e4ae31c46e4987fd5f0"},
        {0x8002d530u,380u,"45b0ef2da507da9fad748b495453f455d5d827a75f992079f4460fd0c68103ed"},
        {0x8002d6acu,208u,"eee5529eb0a8441925bd7aa398a0457e66d023e88c5722ad1a82ddbbf25667a5"},
        {0x8002d77cu,152u,"845d52b22ab1a80cad126db96f69354af5471a0d22931bee17a45e659e06d3fc"},
        {0x8002d814u,368u,"440906f633795c00a4b1381399c556f28de5d61499a3f716ae5bbc8d91016076"},
        {0x8002d984u,144u,"d4d2a7d7679a03748c027de9d0d69cbbacfe0acd39d70aeb5a039660a456e5ff"},
        {0x8002da14u,232u,"e6e2c846190642dd02a79c4e358be5b6bf8134399a1eea1ca648568370c73fe0"},
        {0x8002dafcu,136u,"7fd1f8330c672f1beaed8e213cce57b524714072f984029b0ec7632662938a87"}
    };
    unsigned index=0;while(index<sizeof(bodies)/sizeof(bodies[0])&&bodies[index].target!=target)index++;
    if(index==sizeof(bodies)/sizeof(bodies[0])||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MODEL_PACKETS");if(control&&strcmp(control,"0")==0)return 0;
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
    uint32_t bytes=bodies[index].bytes;if(!image||image->size<offset+bytes)return 0;
    uint8_t digest[32];file1_digest(image->bytes+offset,bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[index].sha[i*2]||file1_hex_digit(digest[i]&15u)!=bodies[index].sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    switch(target){
    case 0x8002c8ccu:battle_model_packet_dispatch_run(&entry);break;
    case 0x8002ccacu:battle_model_packet_reset_run(&entry);break;
    case 0x8002ccc8u:case 0x8002cd24u:battle_model_packet_latch_run(&entry,target==0x8002cd24u);break;
    case 0x8002cd64u:battle_model_packet_control_run(&entry);break;
    case 0x8002cdccu:case 0x8002cf58u:battle_model_packet_lit_flat_run(&entry,target==0x8002cf58u);break;
    case 0x8002cf34u:case 0x8002d0c0u:battle_model_packet_flat_run(&entry,target==0x8002d0c0u);break;
    case 0x8002d0e4u:case 0x8002d984u:battle_model_packet_texture_run(&entry,target==0x8002d0e4u);break;
    case 0x8002d180u:battle_model_packet_gouraud_quad_run(&entry);break;
    case 0x8002d244u:case 0x8002da14u:battle_model_packet_gouraud_texture_run(&entry,target==0x8002d244u);break;
    case 0x8002d530u:case 0x8002d814u:battle_model_packet_lit_texture_run(&entry,target==0x8002d530u);break;
    case 0x8002d6acu:case 0x8002d77cu:battle_model_packet_gouraud_run(&entry,target==0x8002d6acu);break;
    case 0x8002dafcu:battle_model_packet_debug_run(&entry);break;
    default:abort();
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef MP_R
#undef MP_H
#undef MP_B
#undef MP_S
#undef MP_W
#undef MP_SH
#undef MP_SB
#undef MP_AT
#undef MP_V0
#undef MP_V1
#undef MP_A0
#undef MP_A1
#undef MP_A2
#undef MP_A3
#undef MP_T0
#undef MP_S0
#undef MP_S1
#undef MP_S2
#undef MP_S3
#undef MP_S4
#undef MP_S5
#undef MP_SP
#undef MP_RA
