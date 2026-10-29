/* Startup resource bounds and canonical archive-pointer relocation. The
 * loaded startup module has its own identity; command FILE1 shares addresses.
 * Direct host-side archive APIs keep their separate representation contract. */
#define RB_R(a) battle_entry_read(entry,(a),4)
#define RB_H(a) battle_entry_read(entry,(a),2)
#define RB_W(a,v) battle_entry_write(entry,(a),4,(v))
#define RB_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define RB_AT (entry->cpu->gpr[1])
#define RB_V0 (entry->cpu->gpr[2])
#define RB_V1 (entry->cpu->gpr[3])
#define RB_A0 (entry->cpu->gpr[4])
#define RB_A1 (entry->cpu->gpr[5])
#define RB_A2 (entry->cpu->gpr[6])
#define RB_A3 (entry->cpu->gpr[7])
#define RB_T0 (entry->cpu->gpr[8])
#define RB_T1 (entry->cpu->gpr[9])
#define RB_T2 (entry->cpu->gpr[10])
#define RB_T3 (entry->cpu->gpr[11])
#define RB_T4 (entry->cpu->gpr[12])
#define RB_T5 (entry->cpu->gpr[13])
#define RB_T6 (entry->cpu->gpr[14])
#define RB_S0 (entry->cpu->gpr[16])
#define RB_SP (entry->cpu->gpr[29])
#define RB_RA (entry->cpu->gpr[31])

static void battle_archive_relocate_run(BattleEntryCall *entry)
{
    RB_A1=1;RB_V0=RB_R(RB_A0);RB_A2=RB_A0;
    if(RB_V0) {
        RB_V1=RB_A0+4u;
        do {
            uint32_t offset=RB_R(RB_V1);RB_A1++;RB_V0=offset+RB_A2;
            RB_W(RB_V1,RB_V0);RB_V0=RB_R(RB_A0);RB_V0=RB_V0<RB_A1;RB_V1+=4u;
        } while(!RB_V0);
        RB_V0=RB_R(RB_A0);
    }
}

static void battle_resource_bounds_relocate(BattleEntryCall *entry)
{
    RB_RA=0x801e70fcu;RB_S0=RB_A0;
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,0x8003342cu);
    int rc=PcPortMipsRun(&callee,0x8003342cu,0x801e70fcu,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-resource-bounds] relocation from 0x801e70f4: %s\n",callee.error);
        battle_entry_fail(entry,"resource relocation stopped",0x8003342cu);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending resource relocation return load",0x8003342cu);
}

static void battle_resource_bounds_run(BattleEntryCall *entry)
{
    RB_SP-=0x20u;uint32_t sp=RB_SP;
    RB_W(sp+0x18u,RB_S0);RB_W(sp+0x1cu,RB_RA);battle_resource_bounds_relocate(entry);
    RB_T3=0x800u;RB_T4=0x800u;RB_T2=0xfffff800u;RB_A1=0xfffff800u;
    RB_T5=RB_R(RB_S0);RB_T1=0;
    if((int32_t)RB_T5>0) {
        RB_T6=0x1101u;RB_A0=RB_S0;
        do {
            RB_A2=RB_R(RB_A0+4u);RB_V0=RB_H(RB_A2);
            int match=RB_V0==RB_T6;RB_A2+=4u;
            if(match) {
                RB_A3=RB_H(RB_A2);RB_A2+=2u;
                RB_T0=RB_H(RB_A2);RB_A2+=2u;
                RB_V1=RB_H(RB_A2);RB_A2+=2u;
                uint32_t y=RB_H(RB_A2);RB_A3+=RB_V1;RB_V0=y;RB_T0+=RB_V0;
                RB_V0=(int32_t)RB_A3<(int32_t)RB_T3;RB_A2+=2u;
                if(RB_V0)RB_T3=RB_A3;
                RB_V0=(int32_t)RB_T0<(int32_t)RB_T4;if(RB_V0)RB_T4=RB_T0;
                uint32_t width=RB_H(RB_A2),height=RB_H(RB_A2+2u);RB_V0=width;
                RB_A3+=RB_V0;RB_V1=height;RB_V0=(int32_t)RB_T2<(int32_t)RB_A3;
                RB_T0+=RB_V1;if(RB_V0)RB_T2=RB_A3;
                RB_V0=(int32_t)RB_A1<(int32_t)RB_T0;if(RB_V0)RB_A1=RB_T0;
            }
            RB_T1++;RB_V0=(int32_t)RB_T1<(int32_t)RB_T5;RB_A0+=4u;
        } while(RB_V0);
    }
    RB_A0=RB_T2-RB_T3;RB_A1-=RB_T4;
    RB_V1=(uint32_t)(int32_t)(int16_t)RB_A0;RB_V0=(uint32_t)(int32_t)(int16_t)RB_A1;
    uint64_t product=(uint64_t)((int64_t)(int32_t)RB_V1*(int64_t)(int32_t)RB_V0);
    entry->cpu->hi=(uint32_t)(product>>32);entry->cpu->lo=(uint32_t)product;
    RB_AT=0x800d0000u;RB_SH(0x800d2d30u,RB_T3);
    RB_AT=0x800d0000u;RB_SH(0x800d2d34u,RB_T4);
    RB_AT=0x800d0000u;RB_SH(0x800d2d2cu,RB_A0);
    RB_AT=0x800c0000u;RB_SH(0x800c3ea8u,RB_A1);RB_V0=entry->cpu->lo;
    uint32_t ra=RB_R(sp+0x1cu),s0=RB_R(sp+0x18u);RB_RA=ra;RB_SP=sp+0x20u;RB_S0=s0;
}

static int battle_resource_bounds_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if((target!=0x801e70e8u&&target!=0x8003342cu)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_RESOURCE_BOUNDS");if(control&&strcmp(control,"0")==0)return 0;
    static const struct {uint32_t address,bytes;const char *sha;} regions[]={
        {0x801e70e8u,296u,"435abb1d6779bd86e0c96676cdac5d11caea54b2724c256c9f4735e5e07b9a16"},
        {0x801e4000u,32u,"6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140"},
        {0x801e5840u,228u,"e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2"},
        {0x8003342cu,72u,"5032ef7b832871473c3654f6ddb73d0031aacf51119de54d9d81f3c9a071fac6"}
    };
    unsigned begin=target==0x8003342cu?3:0,end=target==0x8003342cu?4:3;
    for(unsigned r=begin;r<end;r++) {
        const uint8_t *code=PSX_ADDR(regions[r].address);
        if(r==3) {
            const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
            if(!image||!image->bytes||image->size<offset+regions[r].bytes)return 0;
            code=image->bytes+offset;
        }
        uint8_t digest[32];file1_digest(code,regions[r].bytes,digest);
        for(unsigned i=0;i<sizeof(digest);i++)
            if(file1_hex_digit(digest[i]>>4)!=regions[r].sha[i*2]||file1_hex_digit(digest[i]&15u)!=regions[r].sha[i*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x8003342cu)battle_archive_relocate_run(&entry);else battle_resource_bounds_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef RB_R
#undef RB_H
#undef RB_W
#undef RB_SH
#undef RB_AT
#undef RB_V0
#undef RB_V1
#undef RB_A0
#undef RB_A1
#undef RB_A2
#undef RB_A3
#undef RB_T0
#undef RB_T1
#undef RB_T2
#undef RB_T3
#undef RB_T4
#undef RB_T5
#undef RB_T6
#undef RB_S0
#undef RB_SP
#undef RB_RA
