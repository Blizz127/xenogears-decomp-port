/* Complete 801E693C body. Archive and heap calls use explicit native service
 * contracts; their original CPU frames, faults and CD timing are not adopted. */
#include "system/archive.h"

static int battle_startup_queue_enabled(void)
{
    const char *control=getenv("XENO_BATTLE_NATIVE_STARTUP_QUEUE");
    return control&&strcmp(control,"1")==0;
}
static int battle_startup_queue_identity(uint32_t target,unsigned size,const char *sha)
{
    const uint8_t *code;
    if(target<0x801e4000u){
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
        size_t offset=XENO_RD_SLUS_OFF(target);
        if(!image||!image->bytes||image->size<offset+size)return 0;
        code=image->bytes+offset;
    }else code=PSX_ADDR(target);
    uint8_t digest[32];file1_digest(code,size,digest);
    for(unsigned i=0;i<32;i++)
        if(file1_hex_digit(digest[i]>>4)!=sha[2*i]||
           file1_hex_digit(digest[i]&15u)!=sha[2*i+1])return 0;
    return 1;
}
static int battle_startup_queue_module(void)
{
    return battle_startup_queue_identity(0x801e693cu,272u,
        "1a8716ac9a30f2db585dd103a97be839f1ed1148e781a1006a1aeeb2bbdab61d")&&
        battle_startup_queue_identity(0x801e4000u,32u,
        "6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140")&&
        battle_startup_queue_identity(0x801e5840u,228u,
        "e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2");
}
static int battle_startup_queue_context(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu)
{
    return !runtime->force_interpret&&runtime->initialized&&
        g_ActiveBattleRuntime==runtime&&runtime->bridge_cpu==cpu&&
        !(cpu->load_valid&&cpu->load_reg!=0)&&battle_startup_queue_enabled();
}

#define SQ_R(a) battle_entry_read(entry,(a),4)
#define SQ_B(a) battle_entry_read(entry,(a),1)
#define SQ_W(a,v) battle_entry_write(entry,(a),4,(v))
#define SQ_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define SQ_V0 (entry->cpu->gpr[2])
#define SQ_V1 (entry->cpu->gpr[3])
#define SQ_A0 (entry->cpu->gpr[4])
#define SQ_A1 (entry->cpu->gpr[5])
#define SQ_A2 (entry->cpu->gpr[6])
#define SQ_S0 (entry->cpu->gpr[16])
#define SQ_S1 (entry->cpu->gpr[17])
#define SQ_S2 (entry->cpu->gpr[18])
#define SQ_S3 (entry->cpu->gpr[19])
#define SQ_S4 (entry->cpu->gpr[20])
#define SQ_S5 (entry->cpu->gpr[21])
#define SQ_S6 (entry->cpu->gpr[22])
#define SQ_SP (entry->cpu->gpr[29])
#define SQ_RA (entry->cpu->gpr[31])

static void battle_startup_queue_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-startup-queue] callee 0x%08x from 0x%08x: %s\n",
                target,site,callee.error);
        battle_entry_fail(entry,"startup queue child stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending startup queue child return",target);
}
static void battle_startup_queue_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    SQ_SP-=0x30u;SQ_W(SQ_SP+0x28u,SQ_S6);SQ_S6=SQ_A0;SQ_A0=0x2cu;SQ_A1=1;
    SQ_W(SQ_SP+0x2cu,SQ_RA);SQ_W(SQ_SP+0x24u,SQ_S5);SQ_W(SQ_SP+0x20u,SQ_S4);
    SQ_W(SQ_SP+0x1cu,SQ_S3);SQ_W(SQ_SP+0x18u,SQ_S2);SQ_W(SQ_SP+0x14u,SQ_S1);
    SQ_RA=0x801e6970u;SQ_W(SQ_SP+0x10u,SQ_S0);
    battle_startup_queue_call(entry,0x80028470u,0x801e6968u);
    SQ_S3=0;SQ_S4=SQ_S3;SQ_V0=0x801f0000u;SQ_S5=SQ_V0-0x6a44u;
    SQ_V0=0x800c0000u;SQ_S2=SQ_V0+0x3eb0u;SQ_S1=SQ_S2;SQ_S0=SQ_S6;
    do{
        SQ_V1=SQ_B(SQ_S1+6u);SQ_V0=(int32_t)SQ_V1<17;
        if(SQ_V0){
            SQ_V0=SQ_B(SQ_S1+8u);int skip=SQ_V0!=0;SQ_V0=SQ_V1<<3;
            if(!skip){
                SQ_V0+=SQ_S5;p=SQ_R(SQ_V0);SQ_S4++;SQ_A0=p;
                SQ_RA=0x801e69c8u;SQ_SH(SQ_S0,SQ_A0);
                battle_startup_queue_call(entry,0x800288ecu,0x801e69c0u);
                SQ_A0=SQ_V0;SQ_RA=0x801e69d4u;SQ_A1=0;
                battle_startup_queue_call(entry,0x80031bdcu,0x801e69ccu);
                SQ_W(SQ_S0+4u,SQ_V0);SQ_S0+=8u;SQ_V1=0x8000u;SQ_V1+=SQ_S2;
                SQ_W(SQ_V1+0xd24u,SQ_V0);SQ_W(SQ_V1+0xd2cu,0);
            }
        }
        SQ_S2+=12u;SQ_S3++;SQ_V0=3;int again=SQ_S3!=SQ_V0;SQ_S1+=28u;
        if(!again)break;
    }while(1);
    SQ_A0=SQ_S6;SQ_A1=0;SQ_V0=SQ_S4<<3;SQ_V0+=SQ_A0;SQ_A2=SQ_A1;
    SQ_SH(SQ_V0,0);SQ_RA=0x801e6a20u;SQ_W(SQ_V0+4u,0);
    battle_startup_queue_call(entry,0x80029afcu,0x801e6a18u);
    p=SQ_R(SQ_SP+0x2cu);q=SQ_R(SQ_SP+0x28u);SQ_RA=p;
    p=SQ_R(SQ_SP+0x24u);SQ_S6=q;q=SQ_R(SQ_SP+0x20u);SQ_S5=p;
    p=SQ_R(SQ_SP+0x1cu);SQ_S4=q;q=SQ_R(SQ_SP+0x18u);SQ_S3=p;
    p=SQ_R(SQ_SP+0x14u);SQ_S2=q;q=SQ_R(SQ_SP+0x10u);SQ_S1=p;
    SQ_SP+=0x30u;SQ_S0=q;
}
static int battle_startup_queue_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x801e693cu||!battle_startup_queue_context(runtime,cpu)||
       !battle_startup_queue_module())return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_startup_queue_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

/* Exact call edges precede the legacy cache, including opt-out and rejection.
 * These services preserve CPU state except V0; HeapAlloc additionally retains
 * its established V1=0 and RAM-pointer-to-guest representation. */
static int battle_startup_queue_service_target(uint32_t target,uint32_t caller)
{
    return (target==0x80028470u&&caller==0x801e6970u)||
        (target==0x800288ecu&&caller==0x801e69c8u)||
        (target==0x80031bdcu&&caller==0x801e69d4u)||
        (target==0x80029afcu&&caller==0x801e6a20u);
}
static int battle_startup_queue_service_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_startup_queue_service_target(target,cpu->gpr[31])||
       !battle_startup_queue_context(runtime,cpu)||!battle_startup_queue_module())return 0;
    const char *name;unsigned bytes;const char *sha;
    if(target==0x80028470u){name="ArchiveSetIndex";bytes=68u;sha="0051629ad10ac9949e9229ef5a64dd36ac682f668d7e2cf1ed917e57c6eff2de";}
    else if(target==0x800288ecu){name="ArchiveDecodeAlignedSize";bytes=60u;sha="177211cdf57fe0a64590260c1e3facab70223bc251b53e833e26153fd4dfef42";}
    else if(target==0x80031bdcu){name="HeapAlloc";bytes=916u;sha="769fc2a84de3a5c0fc3829121bd31f8821ce5dad2cfa1f5dcfe6fcbbab14f6f0";}
    else{name="func_80029AFC";bytes=948u;sha="5360b9425de4e457726eed9f36e26238c0237dd5a257cccd5f349608b270e4ad";}
    if(!battle_startup_queue_identity(target,bytes,sha))return 0;
    const ResolvedFunction *resolved=find_function(runtime,target);
    if(!resolved||!resolved->host||!resolved->name||strcmp(resolved->name,name))return -1;
    if(target==0x80028470u){
        if(cpu->gpr[4]!=0x2cu||cpu->gpr[5]!=1u)return -1;
        int (*select_index)(int,int)=(int (*)(int,int))resolved->host;
        cpu->gpr[2]=(uint32_t)select_index(0x2c,1);
    }else if(target==0x800288ecu){
        int (*aligned_size)(unsigned int)=(int (*)(unsigned int))resolved->host;
        cpu->gpr[2]=(uint32_t)aligned_size(cpu->gpr[4]);
    }else if(target==0x80031bdcu){
        if(cpu->gpr[5])return -1;
        return bridge_heap_services(runtime,cpu,resolved);
    }else{
        if(cpu->gpr[5]||cpu->gpr[6]||(cpu->gpr[4]&3u))return -1;
        uint8_t *queue=module_memory_pointer(runtime,cpu->gpr[4],32u,1);
        uintptr_t address=(uintptr_t)queue,ram=(uintptr_t)g_PsxRam;
        /* The production adapter selects the packed format by RAM membership.
         * Refuse host structs/scratchpad and unterminated wider queues. */
        if(!queue||address<ram||address>ram+PSX_RAM_SIZE-32u)return -1;
        unsigned count=0;
        while(count<4u&&load_le(queue+8u*count,2)!=0)count++;
        if(count==4u)return -1;
        for(unsigned i=0;i<count;i++){
            uint32_t data=load_le(queue+8u*i+4u,4);
            if(!data)continue;
            uintptr_t destination;
            if(data>=0x80000000u&&data<0x80200000u)
                destination=ram+(data-0x80000000u);
            else destination=(uintptr_t)data;
            /* Match ArchiveReadPsxStreamQueue exactly. KSEG1/physical aliases
             * are not translated by that adapter. Arena membership is not
             * evidence of allocation ownership or sufficient file capacity. */
            if((data&3u)||destination<ram||destination>=ram+PSX_RAM_SIZE)return -1;
        }
        int (*read_queue)(StreamDataQueueEntry *,int,int)=
            (int (*)(StreamDataQueueEntry *,int,int))resolved->host;
        cpu->gpr[2]=(uint32_t)read_queue((StreamDataQueueEntry *)queue,0,0);
    }
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef SQ_R
#undef SQ_B
#undef SQ_W
#undef SQ_SH
#undef SQ_V0
#undef SQ_V1
#undef SQ_A0
#undef SQ_A1
#undef SQ_A2
#undef SQ_S0
#undef SQ_S1
#undef SQ_S2
#undef SQ_S3
#undef SQ_S4
#undef SQ_S5
#undef SQ_S6
#undef SQ_SP
#undef SQ_RA
