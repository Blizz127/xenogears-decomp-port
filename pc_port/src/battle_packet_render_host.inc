/* Complete packed model packet dispatcher. Original descriptor words and
 * callback tokens stay guest values; native legacy LP64 rows are separate. */
#define PR_R32(a) battle_entry_read(entry,(a),4)
#define PR_R16(a) battle_entry_read(entry,(a),2)
#define PR_R8(a) battle_entry_read(entry,(a),1)
#define PR_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define PR_AT (entry->cpu->gpr[1])
#define PR_V0 (entry->cpu->gpr[2])
#define PR_V1 (entry->cpu->gpr[3])
#define PR_A0 (entry->cpu->gpr[4])
#define PR_A1 (entry->cpu->gpr[5])
#define PR_A2 (entry->cpu->gpr[6])
#define PR_A3 (entry->cpu->gpr[7])
#define PR_T0 (entry->cpu->gpr[8])
#define PR_S0 (entry->cpu->gpr[16])
#define PR_S1 (entry->cpu->gpr[17])
#define PR_S2 (entry->cpu->gpr[18])
#define PR_S3 (entry->cpu->gpr[19])
#define PR_S4 (entry->cpu->gpr[20])
#define PR_S5 (entry->cpu->gpr[21])
#define PR_SP (entry->cpu->gpr[29])
#define PR_RA (entry->cpu->gpr[31])
#define PR_CALL(t,s) battle_packet_render_call(entry,(t),(s))

static void battle_packet_render_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    PR_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-packet-render] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"packet render callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0){
        if((!battle_triangle_walkers_target(target)&&!battle_quad_walkers_target(target)&&!battle_triangle_max_target(target)&&!battle_quad_max_target(target)&&!battle_triangle_min_target(target)&&!battle_quad_min_target(target)&&!battle_lit_ft_target(target))||callee.load_reg!=23||callee.load_mask!=0)
            battle_entry_fail(entry,"pending packet render callee return",target);
        entry->cpu->load_valid=1;entry->cpu->load_reg=23;
        entry->cpu->load_value=callee.load_value;entry->cpu->load_mask=0;
    }
}
static void battle_packet_render_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    PR_SP-=0x30u;PR_W32(PR_SP+0x14u,PR_S1);PR_S1=PR_A1;PR_A1=0x80050000u;
    p=PR_R32(PR_A1+0x104u);PR_W32(PR_SP+0x10u,PR_S0);PR_A1=p;PR_S0=PR_A0;
    PR_W32(PR_SP+0x20u,PR_S4);PR_S4=PR_A2;PR_W32(PR_SP+0x24u,PR_S5);PR_S5=PR_A3;
    PR_W32(PR_SP+0x28u,PR_RA);PR_W32(PR_SP+0x1cu,PR_S3);PR_W32(PR_SP+0x18u,PR_S2);
    if(PR_A1){
        PR_CALL(0x8003101cu,0x8002c73cu);int culled=PR_V0!=0;PR_V0=0;
        if(culled)goto restore;
    }
    p=PR_R16(PR_S0+6u);q=PR_R32(PR_S0+0x10u);PR_S3=p;
    p=PR_R32(PR_S0+0x18u);PR_V1=q;q=PR_R32(PR_S0+0xcu);PR_A0=p;
    p=PR_R32(PR_S0+8u);PR_A1=q;q=PR_R16(PR_S0+4u);PR_A2=p;
    PR_V0=0x80060000u;PR_A3=q;p=PR_R32(PR_V0-0x6a40u);PR_AT=0x80060000u;PR_V0=p;
    PR_W32(PR_AT-0x6bdcu,PR_S1);PR_AT=0x80060000u;PR_W32(PR_AT-0x6a98u,PR_S4);
    PR_S3--;PR_V0+=PR_A3;PR_AT=0x80060000u;PR_W32(PR_AT-0x6a40u,PR_V0);
    PR_V0=0xffffffffu;PR_AT=0x80060000u;PR_W32(PR_AT-0x6ad8u,PR_V1);
    PR_AT=0x80060000u;PR_W32(PR_AT-0x6b68u,PR_A0);PR_AT=0x80060000u;PR_W32(PR_AT-0x6ad4u,PR_A1);
    PR_AT=0x80060000u;PR_W32(PR_AT-0x6ac4u,PR_A2);
    int empty=PR_S3==PR_V0;PR_V0=PR_S5<<2;
    if(!empty){
        PR_V1=0x80018980u;PR_S4=PR_V0+PR_V1;
        do{
            PR_S1=0x80060000u;PR_S1=PR_R32(PR_S1-0x6ad8u);PR_V1=PR_R8(PR_S1);
            PR_V0=PR_V1<<2;PR_V0+=PR_V1;PR_V0<<=3;PR_V1=0x8004fe50u;PR_S0=PR_V0+PR_V1;
            PR_V0=PR_S5<6u;
            if(PR_V0){
                PR_V0=PR_R32(PR_S4);
                /* The live table may be permuted by a child. Each original
                 * case is native control flow; unknown jumps refuse. */
                switch(PR_V0){
                case 0x8002c80cu:PR_S2=PR_R32(PR_S0);break;
                case 0x8002c818u:PR_S2=PR_R32(PR_S0+4u);break;
                case 0x8002c824u:PR_S2=PR_R32(PR_S0+8u);break;
                case 0x8002c830u:PR_S2=PR_R32(PR_S0+0xcu);break;
                case 0x8002c83cu:PR_S2=PR_R32(PR_S0+0x10u);break;
                case 0x8002c848u:PR_S2=PR_R32(PR_S0+0x14u);break;
                default:battle_entry_fail(entry,"unconverted packet render internal jump",PR_V0);
                }
            }
            PR_A0=0x80060000u;p=PR_R32(PR_A0-0x6ad8u);q=(uint32_t)(int32_t)(int16_t)PR_R16(PR_S1+2u);
            PR_A0=p;PR_A0+=4u;PR_A1=q;PR_AT=0x80060000u;PR_W32(PR_AT-0x6ad8u,PR_A0);
            PR_RA=0x8002c86cu;PR_S3--;PR_CALL(PR_S2,0x8002c864u);
            p=(uint32_t)(int32_t)(int16_t)PR_R16(PR_S1+2u);
            if(entry->cpu->load_valid){entry->cpu->gpr[23]=entry->cpu->load_value;entry->cpu->load_valid=0;}
            q=PR_R32(PR_S0+0x1cu);PR_V1=p;PR_V0=q;
            int64_t product=(int64_t)(int32_t)PR_V1*(int64_t)(int32_t)PR_V0;
            entry->cpu->lo=(uint32_t)product;entry->cpu->hi=(uint32_t)((uint64_t)product>>32);
            PR_V0=0x80060000u;p=PR_R32(PR_V0-0x6ad8u);PR_T0=entry->cpu->lo;PR_V0=p;PR_V0+=PR_T0;
            PR_AT=0x80060000u;PR_W32(PR_AT-0x6ad8u,PR_V0);PR_V0=0xffffffffu;
        }while(PR_S3!=PR_V0);
    }
    PR_V0=1;
restore:
    p=PR_R32(PR_SP+0x28u);q=PR_R32(PR_SP+0x24u);PR_RA=p;
    p=PR_R32(PR_SP+0x20u);PR_S5=q;q=PR_R32(PR_SP+0x1cu);PR_S4=p;
    p=PR_R32(PR_SP+0x18u);PR_S3=q;q=PR_R32(PR_SP+0x14u);PR_S2=p;
    p=PR_R32(PR_SP+0x10u);PR_S1=q;PR_SP+=0x30u;PR_S0=p;
}
static const struct {uint32_t target;unsigned bytes;const char *sha;} battle_packet_render_bodies[]={
    {0x8002c700u,460u,"021f56918c66256a915826be604fd928b8594bd8e618956e007fb862aaae51c0"},
    {0x80018980u,24u,"59465a6667d01c8a18329f57b85a5880ecb975a09a184b6902e44506e93aab6c"},
};
static int battle_packet_render_target(uint32_t target){return target==0x8002c700u;}
/* With this host switched off (XENO_BATTLE_NATIVE_PACKET_RENDER=0) and the
 * other caller hosts on, 8002C700 binds its legacy LP64 body (F1), which
 * follows the model's pointer words (+0x08 vertices, +0x0C, +0x10 command
 * groups, +0x18 normals) and the packet buffer (a1) as host pointers.  The
 * models the caller hosts build keep the retail guest words there, so such a
 * call is refused (a clean stop) instead of handed to the legacy body; a
 * legacy-built model (host words, -no-pie addresses >= 4 MB) still binds. */
static int battle_packet_render_guest_word(uint32_t w)
{
    return (w & 0xff800000u) == 0x80000000u || (w & 0xff800000u) == 0xa0000000u;
}
static int battle_packet_render_legacy_gate(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu)
{
    static const uint32_t kField[4]={0x08u,0x0cu,0x10u,0x18u};
    uint32_t word,i;
    /* The packet cursor may be retail's 24-bit physical form (< 2 MB). */
    int guest=battle_packet_render_guest_word(cpu->gpr[5])||cpu->gpr[5]<0x00200000u;
    for(i=0;i<4u&&!guest;i++)
        if(runtime_read(runtime,cpu->gpr[4]+kField[i],4,&word)==0&&battle_packet_render_guest_word(word))guest=1;
    if(!guest)return 0;
    fprintf(stderr,"[xeno-port][battle-packet-render] stop: 8002C700 with a guest-shaped model or buffer "
            "while XENO_BATTLE_NATIVE_PACKET_RENDER=0 (the legacy body would follow guest words as host pointers)\n");
    return -1;
}
static int battle_packet_render_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_packet_render_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_PACKET_RENDER");if(control&&strcmp(control,"0")==0)return battle_packet_render_legacy_gate(runtime,cpu);
    /* Main-executable code is intentionally absent from guest RAM. Verify
     * its private image; only the live rodata jump table belongs in RAM. */
    const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
    size_t offset=XENO_RD_SLUS_OFF(target);
    if(!image||image->size<offset+battle_packet_render_bodies[0].bytes)return 0;
    for(unsigned i=0;i<2;i++){
        const uint8_t *code=i?PSX_ADDR(battle_packet_render_bodies[i].target):image->bytes+offset;
        uint8_t digest[32];file1_digest(code,battle_packet_render_bodies[i].bytes,digest);
        const char *expected=battle_packet_render_bodies[i].sha;
        for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=expected[j*2]||file1_hex_digit(digest[j]&15u)!=expected[j*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_packet_render_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef PR_R32
#undef PR_R16
#undef PR_R8
#undef PR_W32
#undef PR_AT
#undef PR_V0
#undef PR_V1
#undef PR_A0
#undef PR_A1
#undef PR_A2
#undef PR_A3
#undef PR_T0
#undef PR_S0
#undef PR_S1
#undef PR_S2
#undef PR_S3
#undef PR_S4
#undef PR_S5
#undef PR_SP
#undef PR_RA
#undef PR_CALL
