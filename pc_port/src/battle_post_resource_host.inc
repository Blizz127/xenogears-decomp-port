/* Post-resource battle setup and its complete local reset bodies. Main-exe
 * graphics/audio services retain their native pointer-storage contracts. */
#define PR_R(a) battle_entry_read(entry,(a),4)
#define PR_H(a) ((uint32_t)(int32_t)(int16_t)battle_entry_read(entry,(a),2))
#define PR_B(a) battle_entry_read(entry,(a),1)
#define PR_W(a,v) battle_entry_write(entry,(a),4,(v))
#define PR_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define PR_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define PR_V0 (entry->cpu->gpr[2])
#define PR_V1 (entry->cpu->gpr[3])
#define PR_A0 (entry->cpu->gpr[4])
#define PR_A1 (entry->cpu->gpr[5])
#define PR_A2 (entry->cpu->gpr[6])
#define PR_A3 (entry->cpu->gpr[7])
#define PR_S0 (entry->cpu->gpr[16])
#define PR_S1 (entry->cpu->gpr[17])
#define PR_SP (entry->cpu->gpr[29])
#define PR_RA (entry->cpu->gpr[31])
#define PR_CALL(t,s) battle_post_resource_call(entry,(t),(s))

static void battle_post_resource_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    PR_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-post-resource] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"post-resource callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending post-resource callee return",target);
}

static void battle_post_resource_vectors(BattleEntryCall *entry,unsigned offset)
{
    uint32_t a1=PR_H(PR_V0+offset),a2=PR_H(PR_V0+offset+2u);PR_A1=a1;
    uint32_t a3=PR_H(PR_V0+offset+4u);PR_A2=a2;PR_A3=a3;
}

static void battle_post_resource_run(BattleEntryCall *entry)
{
    PR_SP-=0x20u;PR_W(PR_SP+0x10u,PR_S0);PR_S0=PR_A0;PR_W(PR_SP+0x18u,PR_RA);
    PR_RA=0x800b81d4u;PR_W(PR_SP+0x14u,PR_S1);PR_CALL(0x800b88c4u,0x800b81ccu);
    PR_S1=0x80060000u;PR_CALL(0x800b8840u,0x800b81d4u);
    PR_A0=PR_S0;PR_CALL(0x801e62e0u,0x800b81dcu);
    PR_V0=0x80060000u;PR_A0=PR_R(0x800595acu);PR_S0=0x800d0000u;PR_CALL(0x80038310u,0x800b81ecu);
    uint32_t source=PR_R(PR_S1+0x58c8u);PR_S0+=0x30a0u;PR_V0=source;
    battle_post_resource_vectors(entry,0x482u);PR_A0=PR_S0;PR_CALL(0x80021b04u,0x800b8208u);
    source=PR_R(PR_S1+0x58c8u);PR_A0=0x800d0000u;PR_V0=source;
    battle_post_resource_vectors(entry,0x482u);PR_A0+=0x3354u;PR_CALL(0x80021b04u,0x800b8224u);
    PR_V0=PR_R(PR_S1+0x58c8u);battle_post_resource_vectors(entry,0x47cu);
    PR_A0=PR_S0+8u;PR_CALL(0x80021b04u,0x800b8240u);
    source=PR_R(PR_S1+0x58c8u);PR_A0=0x800d0000u;PR_V0=source;
    battle_post_resource_vectors(entry,0x47cu);PR_A0+=0x335cu;PR_CALL(0x80021b04u,0x800b825cu);
    PR_A0=1;PR_CALL(0x80044534u,0x800b8264u);
    uint32_t ra=PR_R(PR_SP+0x18u),s1=PR_R(PR_SP+0x14u);PR_RA=ra;
    uint32_t s0=PR_R(PR_SP+0x10u);PR_S1=s1;PR_SP+=0x20u;PR_S0=s0;
}

static void battle_post_resource_renderer(BattleEntryCall *entry)
{
    PR_SP-=0x20u;PR_A0=3;PR_V0=0x800c0000u;PR_A1=PR_V0;PR_A2=11;
    PR_W(0x800c3d58u,0);PR_V0=0x800c3eb0u;PR_V1=PR_V0+0x54u;
    PR_W(PR_SP+0x18u,PR_RA);PR_W(PR_SP+0x14u,PR_S1);PR_W(PR_SP+0x10u,PR_S0);
    do {
        PR_V0=PR_B(PR_V1+6u);PR_V0=PR_V0<17u;
        if(PR_V0) {
            PR_V0=PR_B(PR_V1+8u);
            if(PR_V0){PR_V0=PR_R(PR_A1+0x3d58u);PR_V0++;PR_W(PR_A1+0x3d58u,PR_V0);}
        }
        PR_A0++;PR_V1+=0x1cu;
    } while(PR_A0!=PR_A2);
    PR_V0=0x800c0000u;uint32_t count=PR_R(0x800c3d58u);PR_A0=0x80060000u;PR_V0=count;
    PR_V1=PR_V0>>31;PR_V0+=PR_V1;PR_V0=(uint32_t)((int32_t)PR_V0>>1);PR_V0--;
    int negative=(int32_t)PR_V0<0;PR_W(0x80059198u,PR_V0);if(negative)PR_W(0x80059198u,0);
    PR_V0=0x800c0000u;PR_S0=PR_V0+0x3eb0u;PR_A2=PR_S0+0xb70u;PR_V0=0x8000u;PR_S1=PR_S0+PR_V0;
    uint32_t n=PR_R(0x80059198u);PR_V1=0x80050000u;PR_A1=n;PR_W(0x80059198u,0);
    uint32_t old=PR_R(PR_S1+0xc50u);PR_V0=2;PR_A0=old;PR_W(0x80050100u,PR_V0);
    int alternate=PR_A0==PR_A2;PR_W(PR_S1+0xdacu,PR_A1);if(alternate)PR_A2=PR_S0+0x4be0u;
    PR_A0=PR_A2+0x70u;PR_A1=0x1000u;PR_W(PR_S1+0xc50u,PR_A2);
    PR_RA=0x800b89a8u;PR_W(PR_S1+0xc54u,PR_A0);PR_CALL(0x80044ad8u,0x800b89a0u);
    PR_V0=1;PR_W(PR_S1+0xc84u,PR_V0);
    uint32_t v1=PR_B(PR_S0+0xb88u),a0=PR_B(PR_S0+0xb89u);PR_V1=v1;
    uint32_t a1=PR_B(PR_S0+0xb8au);PR_A0=a0;
    uint32_t a2=PR_B(PR_S0+0xb8bu);PR_A1=a1;PR_V0=PR_S0+0x4be0u;PR_A2=a2;
    PR_W(PR_S1+0xc50u,PR_V0);PR_SB(PR_S1+0xda8u,0);
    PR_SB(PR_S0+0x4bf8u,PR_V1);PR_SB(PR_S0+0x4bf9u,PR_A0);PR_SB(PR_S0+0x4bfau,PR_A1);PR_SB(PR_S0+0x4bfbu,PR_A2);
    uint32_t ra=PR_R(PR_SP+0x18u),s1=PR_R(PR_SP+0x14u);PR_RA=ra;
    uint32_t s0=PR_R(PR_SP+0x10u);PR_S1=s1;PR_SP+=0x20u;PR_S0=s0;
}

static void battle_post_resource_reset(BattleEntryCall *entry)
{
    PR_SP-=0x18u;PR_V1=0x80060000u;PR_V0=1;PR_SB(0x800591adu,PR_V0);
    PR_V0=0x80060000u;PR_W(0x80059464u,0);PR_V0=0x80060000u;PR_V1=0x80060000u;
    PR_SB(0x800591acu,0);PR_V0=0x2000u;PR_W(PR_SP+0x10u,PR_RA);
    PR_RA=0x800b8874u;PR_W(0x800591a8u,PR_V0);PR_CALL(0x800bed30u,0x800b886cu);
    PR_CALL(0x800be108u,0x800b8874u);PR_CALL(0x8001c944u,0x800b887cu);PR_CALL(0x800bb7f8u,0x800b8884u);
    PR_A0=0x5000u;PR_A1=0;PR_CALL(0x80024f64u,0x800b8890u);
    PR_CALL(0x800bcd8cu,0x800b8898u);PR_CALL(0x800b7c28u,0x800b88a0u);PR_CALL(0x800b89f4u,0x800b88a8u);
    PR_V0=0x80050000u;uint32_t ra=PR_R(PR_SP+0x10u);PR_W(0x80050104u,0);PR_RA=ra;PR_SP+=0x18u;
}

static void battle_post_resource_camera_mode(BattleEntryCall *entry)
{
    PR_SP-=0x18u;PR_V0=0x800c0000u;PR_V1=0x800c0000u;PR_W(0x800c3cc0u,PR_A0);
    PR_V0=1;PR_W(0x800c3cbcu,PR_V0);PR_V0=2;PR_W(PR_SP+0x14u,PR_RA);
    int coords=PR_A0==PR_V0;PR_W(PR_SP+0x10u,PR_S0);
    if(coords) {
        PR_A0=0x800d0000u;PR_A1=0x80070000u;
        uint32_t x=PR_H(0x800d30a0u);PR_A0+=0x30a0u;PR_V0=x<<16;
        PR_W(0x8006f99cu,PR_V0);PR_A1-=0x664u;
        uint32_t y=PR_H(PR_A0+2u),z=PR_H(PR_A0+4u);PR_V0=y;
        PR_V0<<=16;PR_V1=z;PR_V1<<=16;PR_W(PR_A1+4u,PR_V0);PR_W(PR_A1+8u,PR_V1);
        PR_A1=0x80070000u;x=PR_H(PR_A0+8u);PR_A0+=8u;PR_V0=x<<16;
        PR_W(0x8006f9acu,PR_V0);PR_A1-=0x654u;
        y=PR_H(PR_A0+2u);z=PR_H(PR_A0+4u);PR_V0=y;
        PR_V0<<=16;PR_V1=z;PR_V1<<=16;PR_W(PR_A1+4u,PR_V0);PR_W(PR_A1+8u,PR_V1);
    } else {
        PR_V0=4;int fixed=PR_A0==PR_V0;PR_S0=0x800c0000u;
        if(fixed){PR_V0=5;PR_W(PR_V1+0x3cbcu,PR_V0);}
        else {
            PR_A0=PR_R(PR_S0+0x3680u);
            if(PR_A0){PR_V0=PR_R(PR_A0+0xcu);PR_CALL(PR_V0,0x800bc3b0u);PR_W(PR_S0+0x3680u,0);}
            PR_S0=0x800c0000u;PR_A0=PR_R(PR_S0+0x3684u);
            if(PR_A0){PR_V0=PR_R(PR_A0+0xcu);PR_CALL(PR_V0,0x800bc3d8u);PR_W(PR_S0+0x3684u,0);}
        }
    }
    uint32_t ra=PR_R(PR_SP+0x14u),s0=PR_R(PR_SP+0x10u);PR_RA=ra;PR_SP+=0x18u;PR_S0=s0;
}

static void battle_post_resource_leaf(BattleEntryCall *entry,uint32_t target)
{
    switch(target) {
    case 0x80021b04u:PR_SH(PR_A0,PR_A1);PR_SH(PR_A0+2u,PR_A2);PR_SH(PR_A0+4u,PR_A3);break;
    case 0x800bed30u:PR_V0=0x800c0000u;PR_W(0x800c3e20u,0);PR_V0=0x800c0000u;PR_W(0x800c3610u,0);PR_V0=0x800d0000u;PR_SH(0x800d2e54u,0);break;
    case 0x800be108u:PR_V0=0x800d0000u;PR_W(0x800d2d68u,0);PR_V0=0x800c0000u;PR_W(0x800c374cu,0);break;
    case 0x800bcd8cu:PR_V0=0x800c0000u;PR_SH(0x800c3d14u,0);break;
    case 0x800b7c28u:PR_V0=0x800d0000u;PR_SB(0x800d2fdcu,0);break;
    case 0x800b89f4u:break; /* The complete original body is JR RA / NOP. */
    case 0x800bb7f8u: {
        PR_SP-=0x18u;PR_V1=0x800c0000u;PR_V0=0x200u;PR_W(0x800c3674u,PR_V0);
        PR_V1=0x800c0000u;PR_V0=UINT32_MAX;PR_W(0x800c3678u,PR_V0);
        PR_V0=0x800c0000u;PR_V1=0x800c0000u;PR_SB(0x800c3cc4u,0);PR_V0=1;PR_A0=0;
        PR_W(PR_SP+0x10u,PR_RA);PR_RA=0x800bb834u;PR_W(0x800c3cbcu,PR_V0);PR_CALL(0x800bc2f0u,0x800bb82cu);
        uint32_t ra=PR_R(PR_SP+0x10u);PR_SP+=0x18u;PR_RA=ra;break;
    }
    case 0x801e62e0u: {
        PR_SP-=0x18u;PR_W(PR_SP+0x10u,PR_S0);PR_S0=0x80060000u;PR_V0=1;PR_W(PR_SP+0x14u,PR_RA);
        PR_RA=0x801e62fcu;PR_SB(PR_S0-0x6e51u,PR_V0);PR_CALL(0x801e7098u,0x801e62f4u);PR_SB(PR_S0-0x6e51u,0);
        uint32_t ra=PR_R(PR_SP+0x14u),s0=PR_R(PR_SP+0x10u);PR_RA=ra;PR_SP+=0x18u;PR_S0=s0;break;
    }
    default:battle_entry_fail(entry,"unknown post-resource body",target);
    }
}

static const struct {uint32_t address,bytes;const char *sha;} battle_post_resource_bodies[]={
    {0x800b81bcu,200u,"8df8276a0d6628e529e23f1869d14687341f461ea165bda4e674e291030f996c"},
    {0x800b88c4u,304u,"a6ae34aa5c6e83bee9aa7a90fa9a1a17dfecaf9b896e0686f1df1a055a6da8f2"},
    {0x800b8840u,132u,"ff2f2f16205672a09b3f3b8f09a55c3b53c2ca4b39d6ed98b4c064845675ce2f"},
    {0x801e62e0u,52u,"e80067851f45d933a13a9f5ac8f192a3824ba74e035042f54e0b25bf17b579be"},
    {0x80021b04u,16u,"694486283a46f5f80c141894f7830f37a26ff26f7fe6c1784666b2dc72c14488"},
    {0x800bed30u,28u,"f5f087ff5ea0910ddcea7f2b6bee161c9e6ff76cb9fb798bb9ba2a18b8422e5c"},
    {0x800be108u,20u,"fa93ffdd3ccfafbd62eab4b4e3033dd6f37a3f431392b65a10d784159fb2dc3b"},
    {0x800bcd8cu,12u,"a81b55e283409968611a81b4f925ae82c83522d772a061871b084038d3b0565c"},
    {0x800b7c28u,12u,"54d7b84e955d6ad6bf9849046eac871d9a387a5c612bc500bc314bfbcf007b2a"},
    {0x800b89f4u,8u,"6d64edf91449c1b17746c1ef18afa2eb25c70bdf1322ab3df5a2630993b7e2f1"},
    {0x800bb7f8u,76u,"2e96081cfa5897ed4f20971b1fd66842c314f098425dbe88474c69dc509d52ae"},
    {0x800bc2f0u,264u,"8473314fa525ff6ba642454ae1186b9c2f4ec8c3bdac129ddbabf2aaeb269007"},
};
static int battle_post_resource_target(uint32_t target)
{
    for(unsigned i=0;i<sizeof(battle_post_resource_bodies)/sizeof(battle_post_resource_bodies[0]);i++)
        if(battle_post_resource_bodies[i].address==target)return 1;
    return 0;
}
static int battle_post_resource_identity(uint32_t address,unsigned bytes,const char *sha)
{
    const uint8_t *code=PSX_ADDR(address);
    if(address==0x80021b04u) {
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(address);
        if(!image||!image->bytes||image->size<offset+bytes)return 0;
        code=image->bytes+offset;
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=sha[i*2]||file1_hex_digit(digest[i]&15u)!=sha[i*2+1])return 0;
    return 1;
}
static int battle_post_resource_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_post_resource_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_POST_RESOURCE");if(control&&strcmp(control,"0")==0)return 0;
    for(unsigned i=0;i<sizeof(battle_post_resource_bodies)/sizeof(battle_post_resource_bodies[0]);i++)
        if(battle_post_resource_bodies[i].address==target&&!battle_post_resource_identity(target,battle_post_resource_bodies[i].bytes,battle_post_resource_bodies[i].sha))return 0;
    if(target==0x801e62e0u&&
       (!battle_post_resource_identity(0x801e4000u,32u,"6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140")||
        !battle_post_resource_identity(0x801e5840u,228u,"e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2")))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x800b81bcu)battle_post_resource_run(&entry);
    else if(target==0x800b88c4u)battle_post_resource_renderer(&entry);
    else if(target==0x800b8840u)battle_post_resource_reset(&entry);
    else if(target==0x800bc2f0u)battle_post_resource_camera_mode(&entry);
    else battle_post_resource_leaf(&entry,target);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef PR_R
#undef PR_H
#undef PR_B
#undef PR_W
#undef PR_SH
#undef PR_SB
#undef PR_V0
#undef PR_V1
#undef PR_A0
#undef PR_A1
#undef PR_A2
#undef PR_A3
#undef PR_S0
#undef PR_S1
#undef PR_SP
#undef PR_RA
#undef PR_CALL
