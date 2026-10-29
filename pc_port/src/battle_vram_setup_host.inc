/* Complete battle VRAM-strip backup and texture-window initialization.
 * Fixed host loops group the four strips and four atlas records; guest data
 * accesses and register lifetimes retain the original observable ordering. */
#define VS_R16(a) battle_entry_read(entry,(a),2)
#define VS_R32(a) battle_entry_read(entry,(a),4)
#define VS_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define VS_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define VS_AT (entry->cpu->gpr[1])
#define VS_V0 (entry->cpu->gpr[2])
#define VS_V1 (entry->cpu->gpr[3])
#define VS_A0 (entry->cpu->gpr[4])
#define VS_A1 (entry->cpu->gpr[5])
#define VS_A2 (entry->cpu->gpr[6])
#define VS_A3 (entry->cpu->gpr[7])
#define VS_S0 (entry->cpu->gpr[16])
#define VS_S1 (entry->cpu->gpr[17])
#define VS_S2 (entry->cpu->gpr[18])
#define VS_S3 (entry->cpu->gpr[19])
#define VS_S4 (entry->cpu->gpr[20])
#define VS_S5 (entry->cpu->gpr[21])
#define VS_S6 (entry->cpu->gpr[22])
#define VS_S7 (entry->cpu->gpr[23])
#define VS_SP (entry->cpu->gpr[29])
#define VS_RA (entry->cpu->gpr[31])
#define VS_CALL(t,s) battle_vram_setup_call(entry,(t),(s))

static void battle_vram_setup_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    VS_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-vram-setup] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"VRAM setup callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending VRAM setup callee return",target);
}
static uint32_t battle_vram_setup_field(BattleEntryCall *entry,uint32_t base,uint32_t offset,unsigned width)
{
    VS_AT=base+0x10000u;return battle_entry_read(entry,VS_AT+offset-0x10000u,width);
}
static void battle_vram_setup_store(BattleEntryCall *entry,uint32_t base,uint32_t offset,unsigned width,uint32_t value)
{
    VS_AT=base+0x10000u;battle_entry_write(entry,VS_AT+offset-0x10000u,width,value);
}
#define VS_FIELD(b,o,w) battle_vram_setup_field(entry,(b),(o),(w))
#define VS_STORE(b,o,w,v) battle_vram_setup_store(entry,(b),(o),(w),(v))

static void battle_vram_setup_body(BattleEntryCall *entry)
{
    VS_SP-=0x48u;VS_W32(VS_SP+0x3cu,VS_S7);VS_S7=0x8950u;
    VS_W32(VS_SP+0x38u,VS_S6);VS_S6=0x8970u;VS_V0=1;
    VS_W32(VS_SP+0x20u,VS_S0);VS_S0=0x800c0000u;VS_S0=VS_R32(0x800c3ea4u);VS_V1=0xc6u;
    VS_W32(VS_SP+0x24u,VS_S1);VS_S1=0x8fa0u;VS_W32(VS_SP+0x28u,VS_S2);VS_S2=0x95d0u;
    VS_W32(VS_SP+0x40u,VS_RA);VS_W32(VS_SP+0x34u,VS_S5);VS_W32(VS_SP+0x30u,VS_S4);VS_W32(VS_SP+0x2cu,VS_S3);
    VS_A0=VS_S0+VS_S7;VS_S6+=VS_S0;VS_A1=VS_S6;
    for(unsigned i=0;i<4;i++){VS_STORE(VS_S0,0x8950u+i*8u,2,VS_V0);VS_STORE(VS_S0,0x8956u+i*8u,2,VS_V0);}
    VS_V0=VS_FIELD(VS_S0,0xa240u,2);VS_S1+=VS_S0;
    for(unsigned i=0;i<4;i++)VS_STORE(VS_S0,0x8954u+i*8u,2,VS_V1);
    VS_V1=VS_FIELD(VS_S0,0xa240u,2);VS_V0--;VS_STORE(VS_S0,0x8952u,2,VS_V0);
    VS_V0=VS_FIELD(VS_S0,0xa240u,2);VS_S2+=VS_S0;VS_STORE(VS_S0,0x895au,2,VS_V1);
    VS_V1=VS_FIELD(VS_S0,0xa240u,2);VS_V0-=2u;VS_V1-=3u;
    VS_STORE(VS_S0,0x8962u,2,VS_V0);VS_V0=0x9c00u;VS_STORE(VS_S0,0x896au,2,VS_V1);
    VS_S0+=VS_V0;VS_CALL(0x800448f8u,0x80077aecu);
    const unsigned destinations[]={22,17,18,16},rectangles[]={23,21,20,19};
    for(unsigned column=1;column<4;column++) {
        VS_A1=entry->cpu->gpr[destinations[column]];VS_A0=0x800c0000u;VS_A0=VS_R32(0x800c3ea4u);
        entry->cpu->gpr[rectangles[column]]=0x8950u+column*8u;
        VS_A0+=entry->cpu->gpr[rectangles[column]];VS_CALL(0x800448f8u,0x80077b04u+(column-1u)*0x18u);
    }
    for(unsigned copy=1;copy<4;copy++)for(unsigned column=0;column<4;column++) {
        VS_A0=0x800c0000u;VS_A0=VS_R32(0x800c3ea4u);
        VS_A1=entry->cpu->gpr[destinations[column]]+copy*0x18cu;
        VS_A0+=entry->cpu->gpr[rectangles[column]];VS_CALL(0x800448f8u,0x80077b48u+(copy-1u)*0x50u+column*0x14u);
    }
    const uint32_t indices[]={0x4bu,0x50u,0x4du,0x4eu};
    for(unsigned record=0;record<4;record++) {
        uint32_t start=0xa24cu+record*0x18u;
        VS_A1=indices[record];VS_A2=start;VS_A3=start+4u;VS_V0=start+8u;
        VS_V1=0x800c0000u;VS_V1=VS_R32(0x800c3ea4u);VS_A0=0x800d0000u;VS_A0=VS_R32(0x800d2f5cu);
        VS_A2+=VS_V1;VS_A3+=VS_V1;VS_V0+=VS_V1;VS_W32(VS_SP+0x10u,VS_V0);
        VS_V0=start+12u;VS_V0+=VS_V1;VS_W32(VS_SP+0x14u,VS_V0);
        VS_V0=start+16u;VS_V0+=VS_V1;VS_W32(VS_SP+0x18u,VS_V0);
        VS_V0=start+20u;VS_V1+=VS_V0;VS_RA=0x80077c84u+record*0x58u;VS_W32(VS_SP+0x1cu,VS_V1);
        VS_CALL(0x80026338u,0x80077c7cu+record*0x58u);
    }
    VS_A2=0x100u;VS_V1=0x800d0000u;VS_V1=VS_R32(0x800d2d28u);
    VS_A1=0x800c0000u;VS_A1=VS_R32(0x800c3ea4u);VS_V0=0x3c0u;
    VS_W16(VS_V1+2u,0);VS_W16(VS_V1,0);VS_W16(VS_V1+6u,VS_A2);VS_W16(VS_V1+4u,VS_A2);
    VS_STORE(VS_A1,0xa25cu,4,VS_V0);VS_V0=0x3c8u;VS_STORE(VS_A1,0xa274u,4,VS_V0);
    VS_V0=VS_FIELD(VS_A1,0xa244u,2);VS_A0=0x34u;VS_STORE(VS_A1,0xa260u,4,VS_A0);VS_STORE(VS_A1,0xa278u,4,VS_A0);
    VS_V0&=0x3fu;VS_V0<<=1;VS_W16(VS_V1+8u,VS_V0);
    uint32_t pending=VS_FIELD(VS_A1,0xa248u,2);VS_W16(VS_V1+0xcu,VS_A2);VS_V0=pending;
    VS_W16(VS_V1+0xeu,VS_A2);VS_W16(VS_V1+0xau,VS_V0);
    VS_V0=VS_FIELD(VS_A1,0xa25cu,2);VS_A0=0x10u;VS_V0&=0x3fu;VS_V0<<=1;VS_W16(VS_V1+0x10u,VS_V0);
    VS_V0=VS_FIELD(VS_A1,0xa260u,2);VS_A2=8;VS_W16(VS_V1+0x14u,VS_A2);VS_W16(VS_V1+0x16u,VS_A0);VS_W16(VS_V1+0x12u,VS_V0);
    VS_V0=VS_FIELD(VS_A1,0xa274u,2);VS_V0&=0x3fu;VS_V0<<=1;VS_W16(VS_V1+0x18u,VS_V0);
    pending=VS_FIELD(VS_A1,0xa278u,2);VS_W16(VS_V1+0x1cu,VS_A2);VS_V0=pending;
    VS_W16(VS_V1+0x1eu,VS_A0);VS_W16(VS_V1+0x1au,VS_V0);
    VS_V0=VS_FIELD(VS_A1,0xa28cu,2);VS_V0&=0x3fu;VS_V0<<=1;VS_V0+=0xeu;VS_W16(VS_V1+0x20u,VS_V0);
    pending=VS_FIELD(VS_A1,0xa290u,2);VS_W16(VS_V1+0x24u,VS_A0);VS_V0=pending;VS_W16(VS_V1+0x22u,VS_V0);VS_W16(VS_V1+0x26u,VS_A2);
    VS_V0=VS_FIELD(VS_A1,0xa2a4u,2);VS_V0&=0x3fu;VS_V0<<=1;VS_V0+=0xeu;VS_W16(VS_V1+0x28u,VS_V0);
    pending=VS_FIELD(VS_A1,0xa2a8u,2);VS_W16(VS_V1+0x2cu,VS_A0);VS_V0=pending;VS_W16(VS_V1+0x2eu,VS_A2);VS_W16(VS_V1+0x2au,VS_V0);
    const uint32_t page_sites[]={0x80077f1cu,0x80077f80u,0x80077fe4u,0x80078044u};
    const uint32_t mode_sites[]={0x80077f4cu,0x80077fb0u,0x80078010u,0x80078070u};
    for(unsigned packet=0;packet<4;packet++) {
        if(packet){VS_V0=0x800c0000u;VS_V0=VS_R32(0x800c3ea4u);}
        uint32_t base=packet?VS_V0:VS_A1;
        VS_A0=VS_FIELD(base,0xa238u,4);VS_A2=VS_FIELD(base,0xa244u,4);VS_A3=VS_FIELD(base,0xa248u,4);
        VS_A1=0;VS_CALL(0x80043a1cu,page_sites[packet]);
        VS_A1=0;VS_A2=0;VS_A3=VS_V0&0xffffu;VS_V1=0x8908u+packet*12u;
        VS_A0=0x800c0000u;VS_A0=VS_R32(0x800c3ea4u);VS_V0=0x800d0000u;VS_V0=VS_R32(0x800d2d28u);
        VS_A0+=VS_V1;if(packet<2)VS_V0+=8u;
        VS_RA=mode_sites[packet]+8u;VS_W32(VS_SP+0x10u,VS_V0);VS_CALL(0x800454dcu,mode_sites[packet]);
    }
    const unsigned saved[]={31,23,22,21,20,19,18,17,16};pending=VS_R32(VS_SP+0x40u);
    for(unsigned i=1;i<9;i++){uint32_t next=VS_R32(VS_SP+0x40u-i*4u);entry->cpu->gpr[saved[i-1]]=pending;pending=next;}
    VS_SP+=0x48u;VS_S0=pending;
}
static int battle_vram_setup_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x80077990u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_VRAM_SETUP");if(control&&strcmp(control,"0")==0)return 0;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),1816u,digest);
    const char *expected="8ecb274a1cfafa6c298546d68e180b82aa784dfd69f3899287d38ee8e4382a16";
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_vram_setup_body(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef VS_R16
#undef VS_R32
#undef VS_W16
#undef VS_W32
#undef VS_AT
#undef VS_V0
#undef VS_V1
#undef VS_A0
#undef VS_A1
#undef VS_A2
#undef VS_A3
#undef VS_S0
#undef VS_S1
#undef VS_S2
#undef VS_S3
#undef VS_S4
#undef VS_S5
#undef VS_S6
#undef VS_S7
#undef VS_SP
#undef VS_RA
#undef VS_CALL
#undef VS_FIELD
#undef VS_STORE
