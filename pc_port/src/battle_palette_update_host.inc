/* Complete palette-update coordinator. Guest color blocks, six-byte slots
 * and the renderer's scratch stack retain their original packed layout. */
#define PU_R32(a) battle_entry_read(entry,(a),4)
#define PU_R16(a) battle_entry_read(entry,(a),2)
#define PU_R8(a) battle_entry_read(entry,(a),1)
#define PU_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define PU_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define PU_AT (entry->cpu->gpr[1])
#define PU_V0 (entry->cpu->gpr[2])
#define PU_V1 (entry->cpu->gpr[3])
#define PU_A0 (entry->cpu->gpr[4])
#define PU_A1 (entry->cpu->gpr[5])
#define PU_A2 (entry->cpu->gpr[6])
#define PU_A3 (entry->cpu->gpr[7])
#define PU_T0 (entry->cpu->gpr[8])
#define PU_S0 (entry->cpu->gpr[16])
#define PU_S1 (entry->cpu->gpr[17])
#define PU_S2 (entry->cpu->gpr[18])
#define PU_S3 (entry->cpu->gpr[19])
#define PU_S4 (entry->cpu->gpr[20])
#define PU_S5 (entry->cpu->gpr[21])
#define PU_S6 (entry->cpu->gpr[22])
#define PU_S7 (entry->cpu->gpr[23])
#define PU_SP (entry->cpu->gpr[29])
#define PU_FP (entry->cpu->gpr[30])
#define PU_RA (entry->cpu->gpr[31])
#define PU_MULT(a,b) battle_target_camera_product(entry->cpu,(a),(b))
#define PU_CALL(t,s) battle_palette_update_call(entry,(t),(s))

static void battle_palette_update_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    PU_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED){
        fprintf(stderr,"[xeno-port][battle-palette-update] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"palette update callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending palette update callee return",target);
}

/* The fixed two/four-word copies use the original paired unaligned accesses.
 * A failed second load/store must not commit the preceding delayed load.
 * Even aligned tails perform both word reads and both read/modify/writes. */
static void battle_palette_update_store_left(BattleEntryCall *entry,uint32_t address,uint32_t value)
{
    unsigned shift=(3u-(address&3u))*8u;
    uint32_t old=PU_R32(address&~3u);
    uint32_t mask=~(UINT32_MAX>>shift);
    PU_W32(address&~3u,(old&mask)|(value>>shift));
}
static void battle_palette_update_store_right(BattleEntryCall *entry,uint32_t address,uint32_t value)
{
    unsigned shift=(address&3u)*8u;
    uint32_t old=PU_R32(address&~3u);
    uint32_t mask=~(UINT32_MAX<<shift);
    PU_W32(address&~3u,(old&mask)|(value<<shift));
}
static void battle_palette_update_copy_words(BattleEntryCall *entry,unsigned words)
{
    uint32_t complete=0;
    for(unsigned i=0;i<words;i++){
        uint32_t address=PU_A2+4u*i+3u;
        unsigned left_shift=(3u-(address&3u))*8u;
        uint32_t left=PU_R32(address&~3u);
        if(i)entry->cpu->gpr[1u+i]=complete;
        address=PU_A2+4u*i;
        unsigned right_shift=(address&3u)*8u;
        uint32_t right=PU_R32(address&~3u);
        uint32_t *reg=&entry->cpu->gpr[2u+i];
        *reg=(*reg&~(UINT32_MAX<<left_shift))|(left<<left_shift);
        complete=(*reg&~(UINT32_MAX>>right_shift))|(right>>right_shift);
    }
    battle_palette_update_store_left(entry,PU_A3+3u,PU_V0);
    entry->cpu->gpr[1u+words]=complete;
    battle_palette_update_store_right(entry,PU_A3,PU_V0);
    for(unsigned i=1;i<words;i++){
        battle_palette_update_store_left(entry,PU_A3+4u*i+3u,entry->cpu->gpr[2u+i]);
        battle_palette_update_store_right(entry,PU_A3+4u*i,entry->cpu->gpr[2u+i]);
    }
}
static void battle_palette_update_run(BattleEntryCall *entry)
{
    uint32_t p,q;int again;
    PU_SP-=0x50u;PU_A2=0x800d0000u;PU_A2+=0x3611u;
    PU_W32(PU_SP+0x4cu,PU_RA);PU_W32(PU_SP+0x48u,PU_FP);
    PU_W32(PU_SP+0x44u,PU_S7);PU_W32(PU_SP+0x40u,PU_S6);
    PU_W32(PU_SP+0x3cu,PU_S5);PU_W32(PU_SP+0x38u,PU_S4);
    PU_W32(PU_SP+0x34u,PU_S3);PU_W32(PU_SP+0x30u,PU_S2);
    PU_W32(PU_SP+0x2cu,PU_S1);PU_W32(PU_SP+0x28u,PU_S0);
    PU_V0=PU_R8(PU_A2);if(!PU_V0)goto restore;
    PU_V0=0x800d0000u;PU_V0=PU_R16(PU_V0+0x361au);if(!PU_V0)goto restore;
    PU_V1=0x800d0000u;p=(uint32_t)(int32_t)(int16_t)PU_R16(PU_V1+0x2d2cu);
    PU_V0=0x800c0000u;PU_V1=p;PU_V0=(uint32_t)(int32_t)(int16_t)PU_R16(PU_V0+0x3ea8u);
    PU_MULT(PU_V1,PU_V0);PU_A0=0x800d0000u;p=PU_R32(PU_A0+0x3608u);
    PU_A1=0x800d0000u;PU_A0=p;p=PU_R32(PU_A1+0x3604u);
    PU_W8(PU_A2,0);PU_A1=p;PU_A2=entry->cpu->lo;PU_RA=0x800a6b78u;PU_A2<<=1;
    PU_CALL(0x8003f968u,0x800a6b70u);
    PU_A3=0x800c0000u;p=PU_R32(PU_A3+0x3ac8u);PU_A2=0x800c0000u;PU_A3=p;
    PU_A2=PU_R32(PU_A2+0x3ac4u);PU_V0=(PU_A2|PU_A3)&3u;PU_T0=PU_A2+0x80u;
    if(PU_V0){
        do{battle_palette_update_copy_words(entry,4);PU_A2+=0x10u;again=PU_A2!=PU_T0;PU_A3+=0x10u;}while(again);
    }else{
        do{
            p=PU_R32(PU_A2);q=PU_R32(PU_A2+4u);PU_V0=p;
            p=PU_R32(PU_A2+8u);PU_V1=q;q=PU_R32(PU_A2+12u);PU_A0=p;
            PU_W32(PU_A3,PU_V0);PU_A1=q;PU_W32(PU_A3+4u,PU_V1);
            PU_W32(PU_A3+8u,PU_A0);PU_W32(PU_A3+12u,PU_A1);
            PU_A2+=0x10u;again=PU_A2!=PU_T0;PU_A3+=0x10u;
        }while(again);
    }
    battle_palette_update_copy_words(entry,2);
    PU_S2=0;PU_S7=0x800c0000u;PU_S7+=0x3aaeu;PU_FP=PU_S7-1u;PU_S6=0;
    do{
        PU_AT=0x800c0000u;PU_AT+=PU_S6;PU_V0=PU_R8(PU_AT+0x3aacu);
        if(PU_V0){
            PU_V1=PU_R8(PU_FP);PU_V0=(int32_t)PU_V1<4;
            int transform=PU_V0!=0;PU_V0=4;
            if(transform){
                PU_A1=0x800d0000u;p=PU_R32(PU_A1+0x3608u);PU_A0=0x800d0000u;PU_A1=p;
                p=(uint32_t)(int32_t)(int16_t)PU_R16(PU_A0+0x2d2cu);PU_V0=0x800c0000u;PU_A0=p;
                p=(uint32_t)(int32_t)(int16_t)PU_R16(PU_V0+0x3ea8u);PU_AT=0x800c0000u;PU_V0=p;PU_AT+=PU_S6;
                p=(uint32_t)(int32_t)(int8_t)PU_R8(PU_AT+0x3aafu);PU_MULT(PU_A0,PU_V0);PU_V1=p;
                PU_W32(PU_SP+0x10u,PU_V1);PU_AT=0x800c0000u;PU_AT+=PU_S6;
                PU_V0=(uint32_t)(int32_t)(int8_t)PU_R8(PU_AT+0x3ab0u);PU_W32(PU_SP+0x14u,PU_V0);
                PU_AT=0x800c0000u;PU_AT+=PU_S6;PU_V0=(uint32_t)(int32_t)(int8_t)PU_R8(PU_AT+0x3ab1u);
                PU_W32(PU_SP+0x18u,PU_V0);p=PU_R8(PU_FP);PU_A2=PU_A1;PU_V0=p;
                PU_W32(PU_SP+0x1cu,PU_V0);p=PU_R8(PU_S7);PU_A3=PU_A1;PU_V0=p;PU_A0=entry->cpu->lo;
                PU_RA=0x800a6d04u;PU_W32(PU_SP+0x20u,PU_V0);PU_CALL(0x80025d4cu,0x800a6cfcu);
            }else if(PU_V1==PU_V0){
                PU_V1=0x800d0000u;p=(uint32_t)(int32_t)(int16_t)PU_R16(PU_V1+0x2d2cu);
                PU_V0=0x800c0000u;PU_V1=p;PU_V0=(uint32_t)(int32_t)(int16_t)PU_R16(PU_V0+0x3ea8u);
                PU_MULT(PU_V1,PU_V0);p=PU_R8(PU_S7);PU_A2=0x800d0000u;PU_A1=p;
                p=PU_R32(PU_A2+0x3608u);PU_A3=0x800d0000u;PU_A2=p;
                p=PU_R32(PU_A3+0x3604u);PU_A0=entry->cpu->lo;PU_A3=p;PU_CALL(0x80026f44u,0x800a6d3cu);
            }
            PU_S0=0x800c0000u;p=PU_R32(PU_S0+0x3ac8u);PU_S3=0;PU_S0=p;
            PU_S5=0x280u;PU_S4=0x16dcu;PU_S1=0x173cu;
            do{
                PU_V0=0x800c0000u;PU_V0=PU_R32(PU_V0+0x3ea0u);PU_A2=PU_S0;
                if(PU_V0){
                    PU_S0+=4u;PU_A0=PU_S1+PU_V0;PU_A0+=4u;PU_A1=PU_S2;PU_CALL(0x800a6884u,0x800a6d7cu);
                    for(unsigned j=0;j<3;j++){
                        PU_A2=PU_S0;PU_S0+=4u;PU_A0=0x800c0000u;p=PU_R32(PU_A0+0x3ea0u);
                        PU_A1=PU_S2;PU_A0=p;PU_A0=PU_S1+PU_A0;PU_RA=0x800a6da4u+j*0x20u;PU_A0+=0xcu+j*8u;
                        PU_CALL(0x800a6884u,0x800a6d9cu+j*0x20u);
                    }
                    PU_A2=PU_S0;PU_S0+=4u;PU_A0=0x800c0000u;p=PU_R32(PU_A0+0x3ea0u);
                    PU_A1=PU_S2;PU_A0=p;PU_A0=PU_S4+PU_A0;PU_RA=0x800a6e04u;PU_A0+=4u;PU_CALL(0x800a6884u,0x800a6dfcu);
                }else PU_S0+=0x14u;
                PU_V0=0x800c0000u;PU_V0=PU_R32(PU_V0+0x3d50u);PU_A2=PU_S0;
                if(PU_V0){PU_S0+=4u;PU_A0=PU_S5+PU_V0;PU_A0+=4u;PU_A1=PU_S2;PU_CALL(0x800a6884u,0x800a6e30u);}
                else PU_S0+=4u;
                PU_S5+=0x18u;PU_S4+=0x18u;PU_S3++;PU_V0=(int32_t)PU_S3<4;again=PU_V0!=0;PU_S1+=0x24u;
            }while(again);
            PU_S3=0;PU_S1=0x2e0u;
            do{
                PU_V0=0x800c0000u;PU_V0=PU_R32(PU_V0+0x3d50u);PU_A2=PU_S0;
                if(PU_V0){
                    PU_S0+=4u;PU_A0=PU_S1+PU_V0;PU_A0+=4u;PU_A1=PU_S2;PU_CALL(0x800a6884u,0x800a6e84u);
                    for(unsigned j=0;j<3;j++){
                        PU_A2=PU_S0;PU_S0+=4u;PU_A0=0x800c0000u;p=PU_R32(PU_A0+0x3d50u);
                        PU_A1=PU_S2;PU_A0=p;PU_A0=PU_S1+PU_A0;PU_RA=0x800a6eacu+j*0x20u;PU_A0+=0xcu+j*8u;
                        PU_CALL(0x800a6884u,0x800a6ea4u+j*0x20u);
                    }
                }else PU_S0+=0x10u;
                PU_S3++;PU_V0=(int32_t)PU_S3<2;again=PU_V0!=0;PU_S1+=0x24u;
            }while(again);
            PU_A2=PU_S0;PU_S0+=4u;PU_A0=0x800d0000u;p=PU_R32(PU_A0+0x2d40u);
            PU_RA=0x800a6f20u;PU_A0=p;PU_A1=PU_S2;PU_CALL(0x800a6884u,0x800a6f18u);
            PU_A1=PU_S2;PU_A0=0x800d0000u;p=PU_R32(PU_A0+0x2d48u);
            PU_RA=0x800a6f34u;PU_A0=p;PU_A2=PU_S0;PU_CALL(0x800a6884u,0x800a6f2cu);
        }
        PU_S7+=6u;PU_FP+=6u;PU_S2++;PU_V0=(int32_t)PU_S2<4;again=PU_V0!=0;PU_S6+=6u;
    }while(again);
    PU_A1=0x800d0000u;p=PU_R32(PU_A1+0x3608u);PU_A0=0x800d0000u;PU_A1=p;PU_A0+=0x3628u;
    PU_CALL(0x80044894u,0x800a6f5cu);
restore:
    p=PU_R32(PU_SP+0x4cu);q=PU_R32(PU_SP+0x48u);PU_RA=p;
    p=PU_R32(PU_SP+0x44u);PU_FP=q;q=PU_R32(PU_SP+0x40u);PU_S7=p;
    p=PU_R32(PU_SP+0x3cu);PU_S6=q;q=PU_R32(PU_SP+0x38u);PU_S5=p;
    p=PU_R32(PU_SP+0x34u);PU_S4=q;q=PU_R32(PU_SP+0x30u);PU_S3=p;
    p=PU_R32(PU_SP+0x2cu);PU_S2=q;q=PU_R32(PU_SP+0x28u);PU_S1=p;
    PU_SP+=0x50u;PU_S0=q;
}
static const struct {uint32_t target;unsigned bytes;const char *sha;} battle_palette_update_bodies[]={
    {0x800a6ae8u,1200u,"f076ad04d4add04da01a44081e5b79deb0c45420029d8d0f2ffaec7dd0eb215d"},
};
static int battle_palette_update_target(uint32_t target)
{return target==0x800a6ae8u;}
static int battle_palette_update_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_palette_update_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_PALETTE_UPDATE");if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),battle_palette_update_bodies[0].bytes,digest);
    const char *expected=battle_palette_update_bodies[0].sha;
    for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=expected[j*2]||file1_hex_digit(digest[j]&15u)!=expected[j*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_palette_update_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef PU_R32
#undef PU_R16
#undef PU_R8
#undef PU_W32
#undef PU_W8
#undef PU_AT
#undef PU_V0
#undef PU_V1
#undef PU_A0
#undef PU_A1
#undef PU_A2
#undef PU_A3
#undef PU_T0
#undef PU_S0
#undef PU_S1
#undef PU_S2
#undef PU_S3
#undef PU_S4
#undef PU_S5
#undef PU_S6
#undef PU_S7
#undef PU_SP
#undef PU_FP
#undef PU_RA
#undef PU_MULT
#undef PU_CALL
