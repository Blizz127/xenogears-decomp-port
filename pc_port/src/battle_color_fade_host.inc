/* Complete fade-task setup, timed color update and paired-list cleanup.
 * All arithmetic is compiled host code. GTE register access preserves the
 * existing renderer state; the fixed interpolation uses no opcode dispatcher. */
#define CF_R8(a) battle_entry_read(entry,(a),1)
#define CF_R32(a) battle_entry_read(entry,(a),4)
#define CF_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define CF_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define CF_V0 (entry->cpu->gpr[2])
#define CF_V1 (entry->cpu->gpr[3])
#define CF_A0 (entry->cpu->gpr[4])
#define CF_A1 (entry->cpu->gpr[5])
#define CF_A2 (entry->cpu->gpr[6])
#define CF_A3 (entry->cpu->gpr[7])
#define CF_S0 (entry->cpu->gpr[16])
#define CF_S1 (entry->cpu->gpr[17])
#define CF_S2 (entry->cpu->gpr[18])
#define CF_S3 (entry->cpu->gpr[19])
#define CF_S4 (entry->cpu->gpr[20])
#define CF_S5 (entry->cpu->gpr[21])
#define CF_S6 (entry->cpu->gpr[22])
#define CF_SP (entry->cpu->gpr[29])
#define CF_RA (entry->cpu->gpr[31])
#define CF_CALL(t,s) battle_color_fade_call(entry,(t),(s))

static void battle_color_fade_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    CF_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-color-fade] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"color-fade callee stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)battle_entry_fail(entry,"pending color-fade callee return",target);
}

/* Fixed GPF(sf=0,lm=0) math. The inputs are the words just written to IR0..3,
 * so obtaining their signed halves does not introduce extra MFC2 read effects.
 * Preserve upper halves until each actual SWC2 read sign-extends its register.
 * The product of signed 16-bit inputs fits int32; no MAC overflow is possible. */
static void battle_color_fade_product(uint32_t factor,const uint32_t delta[3])
{
    uint32_t flags=0,rgb=MFC2(6)&0xff000000u;
    for(unsigned i=0;i<3;i++) {
        int32_t product=(int32_t)(int16_t)factor*(int32_t)(int16_t)delta[i];
        int32_t ir=product,color=product>>4;
        if(ir>32767){ir=32767;flags|=1u<<(24u-i);}
        else if(ir< -32768){ir= -32768;flags|=1u<<(24u-i);}
        if(color>255){color=255;flags|=1u<<(21u-i);}
        else if(color<0){color=0;flags|=1u<<(21u-i);}
        MTC2((uint32_t)product,(int)(25u+i));
        MTC2((delta[i]&0xffff0000u)|(uint16_t)ir,(int)(9u+i));
        rgb|=(uint32_t)color<<(i*8u);
    }
    MTC2(MFC2(21),20);MTC2(MFC2(22),21);MTC2(rgb,22);CTC2(flags,31);
}
static void battle_color_fade_divide(PcPortMipsCpu *cpu,uint32_t numerator,uint32_t denominator)
{
    int32_t n=(int32_t)numerator,d=(int32_t)denominator;
    if(d==0){cpu->lo=n<0?1u:UINT32_MAX;cpu->hi=numerator;}
    else if(n==INT32_MIN&&d== -1){cpu->lo=(uint32_t)INT32_MIN;cpu->hi=0;}
    else {cpu->lo=(uint32_t)(n/d);cpu->hi=(uint32_t)(n%d);}
}
static void battle_color_fade_update(BattleEntryCall *entry)
{
    CF_SP-=0x28u;CF_A1=CF_A0;CF_W32(CF_SP+0x20u,CF_RA);
    CF_V0=CF_R32(CF_A1+0x3cu);int timed=CF_V0!=0;CF_A2=CF_A1;
    if(!timed) {
        uint32_t green=CF_R8(CF_A1+0x43u),red=CF_R8(CF_A1+0x42u);CF_V1=green;
        CF_W8(CF_A1+0x49u,CF_V1);CF_V0=red;
        uint32_t blue=CF_R8(CF_A1+0x44u),current_green=CF_R8(CF_A1+0x49u);CF_V1=blue;
        CF_W8(CF_A1+0x48u,CF_V0);CF_A0=current_green;CF_V0|=CF_A0;
        CF_W8(CF_A1+0x4au,CF_V1);CF_V1|=CF_V0;
        int visible=CF_V1!=0;CF_V0=0x800c0000u;
        if(!visible){CF_A0=CF_R32(0x800c3558u);CF_V0=CF_R32(CF_A0+0xcu);CF_CALL(CF_V0,0x800b3714u);}
    } else {
        uint32_t target=CF_R8(CF_A2+0x42u),start=CF_R8(CF_A2+0x45u);CF_V1=target;
        CF_V0--;CF_A0=start;CF_W32(CF_A2+0x3cu,CF_V0);CF_V1-=CF_A0;CF_W32(CF_SP+0x10u,CF_V1);
        for(unsigned i=1;i<3;i++) {
            uint32_t to=CF_R8(CF_A2+0x42u+i),from=CF_R8(CF_A2+0x45u+i);CF_V0=to;CF_V1=from;
            CF_V0-=CF_V1;CF_W32(CF_SP+0x10u+i*4u,CF_V0);
        }
        CF_V0=CF_R32(CF_A2+0x3cu);CF_V0=(uint32_t)((int32_t)CF_V0>>1);MTC2(CF_V0,8);
        uint32_t factor=CF_V0,delta[3];CF_V0=CF_SP+0x10u;
        for(unsigned i=0;i<3;i++){delta[i]=CF_R32(CF_V0+i*4u);MTC2(delta[i],(int)(9u+i));}
        battle_color_fade_product(factor,delta);
        for(unsigned i=0;i<3;i++)CF_W32(CF_V0+i*4u,MFC2((int)(9u+i)));
        for(unsigned i=0;i<3;i++) {
            uint32_t duration=CF_R32(CF_A2+0x38u),product=CF_R32(CF_SP+0x10u+i*4u);
            CF_V0=(uint32_t)((int32_t)duration>>1);CF_V1=product;
            battle_color_fade_divide(entry->cpu,CF_V1,CF_V0);CF_V1=entry->cpu->lo;
            CF_W32(CF_SP+0x10u+i*4u,CF_V1);
        }
        for(unsigned i=0;i<3;i++) {
            uint32_t to=CF_R8(CF_A2+0x42u+i),fraction=CF_R8(CF_SP+0x10u+i*4u);
            CF_V0=to;CF_V1=fraction;CF_V0-=CF_V1;CF_W8(CF_A2+0x48u+i,CF_V0);
        }
    }
    uint32_t ra=CF_R32(CF_SP+0x20u);CF_SP+=0x28u;CF_RA=ra;
}
static void battle_color_fade_remove(BattleEntryCall *entry)
{
    CF_SP-=0x18u;CF_W32(CF_SP+0x10u,CF_S0);CF_S0=CF_A0;CF_W32(CF_SP+0x14u,CF_RA);
    CF_A0=CF_S0+0x1cu;CF_CALL(0x8001cb48u,0x800b384cu);
    CF_A0=CF_S0;CF_CALL(0x8001cd94u,0x800b3854u);
    CF_V0=0x800c0000u;CF_W32(0x800c3558u,0);
    uint32_t ra=CF_R32(CF_SP+0x14u),s0=CF_R32(CF_SP+0x10u);CF_RA=ra;CF_SP+=0x18u;CF_S0=s0;
}
static void battle_color_fade_setup(BattleEntryCall *entry)
{
    CF_V0=0x800d0000u;uint32_t inhibit=CF_R8(0x800d3638u);CF_SP-=0x30u;CF_V0=inhibit;
    CF_W32(CF_SP+0x1cu,CF_S3);uint32_t blue=CF_R8(CF_SP+0x40u);CF_W32(CF_SP+0x18u,CF_S2);CF_S3=blue;
    CF_S2=CF_A0;CF_W32(CF_SP+0x28u,CF_S6);CF_S6=CF_A1;
    CF_W32(CF_SP+0x20u,CF_S4);CF_S4=CF_A2;CF_W32(CF_SP+0x24u,CF_S5);CF_S5=CF_A3;
    CF_W32(CF_SP+0x2cu,CF_RA);CF_W32(CF_SP+0x14u,CF_S1);int disabled=CF_V0!=0;CF_W32(CF_SP+0x10u,CF_S0);
    if(!disabled) {
        CF_V0=0x800c0000u;CF_V0=CF_R8(0x800c355cu);int alternate=CF_V0!=0;CF_S2<<=1;
        int created=0;
        if(alternate) {
            CF_V0=0x800c0000u;CF_V0=CF_R32(0x800c3558u);int existing=CF_V0!=0;CF_V0=0x800c0000u;
            if(existing)CF_S0=CF_R32(0x800c3554u);
            else {CF_V0=0x800c0000u;CF_S0=0x800c3c00u;CF_W32(0x800c3554u,CF_S0);created=1;}
        } else {
            CF_V1=0x800c0000u;CF_A0=CF_R32(0x800c3558u);int existing=CF_A0!=0;CF_S0=CF_A0;
            if(!existing){CF_V0=0x800c0000u;CF_S0=0x800c3c50u;CF_W32(0x800c3558u,CF_S0);created=1;}
        }
        if(created) {
            CF_A0=0;CF_A1=CF_S0;CF_CALL(0x8001cc18u,0x800b3a70u);
            CF_A0=CF_S0;CF_S1=CF_S0+0x1cu;CF_A1=CF_S1;CF_CALL(0x8001ca58u,0x800b3a80u);
            CF_A0=0x7fffffffu;CF_V1=0x80060000u;
            uint32_t flags=CF_R32(CF_S0+0x14u),counted=CF_R8(0x800591acu);CF_V0=flags&CF_A0;CF_V1=counted;
            CF_W32(CF_S0+0x14u,CF_V0);
            if(CF_V1){CF_V1=0x80060000u;CF_V0=CF_R32(0x80059464u);CF_V0--;CF_W32(0x80059464u,CF_V0);}
            CF_A0=CF_S0;CF_A1=0x800b36bcu;CF_CALL(0x8001cd6cu,0x800b3ac4u);
            CF_A0=CF_S1;CF_A1=0x800b3878u;CF_CALL(0x8001cd64u,0x800b3ad4u);
            CF_A0=CF_S0;CF_A1=0x800b383cu;CF_CALL(0x8001cd74u,0x800b3ae4u);
            CF_W32(CF_S0+4u,CF_S0);CF_W32(CF_S0+0x20u,CF_S0);CF_W8(CF_S0+0x40u,0);
            CF_W8(CF_S0+0x45u,0);CF_W8(CF_S0+0x46u,0);CF_W8(CF_S0+0x47u,0);
        } else {
            uint32_t red=CF_R8(CF_S0+0x48u),green=CF_R8(CF_S0+0x49u);CF_V0=red;
            uint32_t blue_now=CF_R8(CF_S0+0x4au);CF_V1=green;CF_W8(CF_S0+0x45u,CF_V0);CF_A0=blue_now;
            CF_W8(CF_S0+0x46u,CF_V1);CF_W8(CF_S0+0x47u,CF_A0);
        }
        CF_A0=CF_S0;CF_W8(CF_A0+0x41u,CF_S6);CF_W8(CF_A0+0x42u,CF_S4);
        CF_W8(CF_A0+0x43u,CF_S5);CF_W8(CF_A0+0x44u,CF_S3);CF_W32(CF_A0+0x38u,CF_S2);
        CF_RA=0x800b3b40u;CF_W32(CF_A0+0x3cu,CF_S2);CF_CALL(0x800b36bcu,0x800b3b38u);
    }
    const unsigned registers[]={31,22,21,20,19,18,17,16};uint32_t pending=CF_R32(CF_SP+0x2cu);
    for(unsigned i=1;i<8;i++){uint32_t next=CF_R32(CF_SP+0x2cu-i*4u);entry->cpu->gpr[registers[i-1]]=pending;pending=next;}
    CF_SP+=0x30u;CF_S0=pending;
}
static int battle_color_fade_target(uint32_t target)
{return target==0x800b39c0u||target==0x800b36bcu||target==0x800b383cu||target==0x8001cd64u||target==0x8001cd74u;}
static int battle_color_fade_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_color_fade_target(target)||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_COLOR_FADE");if(control&&strcmp(control,"0")==0)return 0;
    const uint8_t *code;unsigned bytes;const char *expected;
    if(target==0x8001cd64u||target==0x8001cd74u) {
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t offset=XENO_RD_SLUS_OFF(target);
        if(!image||!image->bytes||image->size<offset+8u)return 0;
        code=image->bytes+offset;bytes=8;
        expected=target==0x8001cd64u?"6b5d5f1a0f2b7c721b6d3106c6530c0974e5b76ba7599b0e84edc41c452f5385":"80cd297c0a26df8ec5f387cc5cb1e8039897493be7456a05b23f5b596b2181aa";
    } else {
        code=PSX_ADDR(target);bytes=target==0x800b39c0u?428u:target==0x800b36bcu?384u:60u;
        expected=target==0x800b39c0u?"01ca07dd594ac760407b9f6af4e15392cd704e880f45b2e702d252eeadb22c27":target==0x800b36bcu?
            "1de6b281e156a3477dc5cfb8e402b0f8a03e8a39fb228bf09a05c3dc335c503f":"eafac70a3a3d04c69ac4806429b1960ac03d4ca162386888805c00237e208041";
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x800b39c0u)battle_color_fade_setup(&entry);
    else if(target==0x800b36bcu)battle_color_fade_update(&entry);
    else if(target==0x800b383cu)battle_color_fade_remove(&entry);
    else battle_entry_write(&entry,cpu->gpr[4]+(target==0x8001cd64u?8u:12u),4,cpu->gpr[5]);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef CF_R8
#undef CF_R32
#undef CF_W8
#undef CF_W32
#undef CF_V0
#undef CF_V1
#undef CF_A0
#undef CF_A1
#undef CF_A2
#undef CF_A3
#undef CF_S0
#undef CF_S1
#undef CF_S2
#undef CF_S3
#undef CF_S4
#undef CF_S5
#undef CF_S6
#undef CF_SP
#undef CF_RA
#undef CF_CALL
