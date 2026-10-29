/* First battle loader module: UI primitives, party glyphs and text buffers.
 * Original frames and live owners remain on the production memory bus. */
#define BATTLE_STAGE3_UI_ADDRESS 0x801e5924u
#define BATTLE_STAGE3_GLYPHS_ADDRESS 0x801e5ee8u
#define BATTLE_STAGE3_PANELS_ADDRESS 0x801e5d2cu
#define BATTLE_STAGE3_TEXT_ADDRESS 0x801e5e78u
#define BATTLE_STAGE3_FIRST_ADDRESS 0x801e6290u
#define BATTLE_STAGE3_SECOND_ADDRESS 0x801e62b8u

#define M3_R8(a) battle_entry_read(entry,(a),1)
#define M3_R16(a) battle_entry_read(entry,(a),2)
#define M3_R32(a) battle_entry_read(entry,(a),4)
#define M3_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define M3_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define M3_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define M3_CALL(target,site,...) battle_entry_call(entry,(target),(site), \
    sizeof((uint32_t[]){__VA_ARGS__})/sizeof(uint32_t),(uint32_t[]){__VA_ARGS__})
#define M3_CALL0(target,site) battle_entry_call(entry,(target),(site),0,NULL)
#define M3_S0 (entry->cpu->gpr[16])
#define M3_S1 (entry->cpu->gpr[17])
#define M3_S2 (entry->cpu->gpr[18])
#define M3_S3 (entry->cpu->gpr[19])
#define M3_S4 (entry->cpu->gpr[20])
#define M3_S5 (entry->cpu->gpr[21])
#define M3_S6 (entry->cpu->gpr[22])
#define M3_S7 (entry->cpu->gpr[23])
#define M3_FP (entry->cpu->gpr[30])
#define M3_V0 (entry->cpu->gpr[2])
#define M3_V1 (entry->cpu->gpr[3])
#define M3_OWNER() M3_R32(0x800c3ea4u)
#define M3_STATE() M3_R32(0x800d2d28u)

static void battle_stage3_ui_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x40u;
    entry->cpu->gpr[29]=sp; M3_V1=1; M3_V0=M3_STATE();
    M3_W32(sp+0x3cu,entry->cpu->gpr[31]);
    for (unsigned r=0;r<7;r++) M3_W32(sp+0x38u-r*4u,entry->cpu->gpr[22-r]);
    M3_W8(M3_V0+0x7cu,M3_V1);
    M3_W8(M3_STATE()+0x7du,M3_V1); M3_W8(M3_STATE()+0x7eu,M3_V1);
    uint32_t base=M3_OWNER(),table=M3_R32(0x800d2f5cu);
    M3_CALL(0x80026338u,0x801e59c0u,table,0x5c,base+0xa234u,base+0xa238u,
            base+0xa23cu,base+0xa240u,base+0xa244u,base+0xa248u);
    base=M3_OWNER(); M3_S2=0;
    M3_CALL(0x80043a58u,0x801e59e8u,M3_R32(base+0xa23cu),M3_R32(base+0xa240u));
    base=M3_OWNER(); uint32_t x=M3_R32(base+0xa23cu),y=M3_R32(base+0xa240u);
    M3_S5=0x80; M3_W16(base+0xa2aeu,M3_V0);
    M3_CALL(0x80043a58u,0x801e5a20u,x,y-1u);
    base=M3_OWNER(); M3_S4=0x4f;
    x=M3_R32(base+0xa23cu); y=M3_R32(base+0xa240u); M3_S6=0x740;
    M3_W16(base+0xa2acu,M3_V0); M3_CALL(0x80043a58u,0x801e5a5cu,x,y-2u);
    base=M3_OWNER(); M3_S1=0;
    x=M3_R32(base+0xa23cu); y=M3_R32(base+0xa240u); M3_S3=0x5a0;
    M3_W16(base+0xa2b2u,M3_V0); M3_CALL(0x80043a58u,0x801e5a98u,x,y-3u);
    M3_V1=M3_OWNER(); M3_S0=0; M3_W16(M3_V1+0xa2b0u,M3_V0);
    do {
        M3_CALL(0x80043cd8u,0x801e5ac0u,M3_OWNER()+M3_S3);
        M3_CALL(0x80043c24u,0x801e5ad4u,M3_OWNER()+M3_S3,0);
        const uint32_t colors[]={0x5a4,0x5a5,0x5a6,0x5b0,0x5b1,0x5b2};
        const uint32_t dark[]={0x5bc,0x5bd,0x5be,0x5c8,0x5c9,0x5ca};
        for (unsigned i=0;i<6;i++) M3_W8(M3_OWNER()+M3_S0+colors[i],M3_S5);
        for (unsigned i=0;i<6;i++) M3_W8(M3_OWNER()+M3_S0+dark[i],0);
        base=M3_OWNER();
        M3_CALL(0x80043a1cu,0x801e5bf8u,M3_R32(base+0xa238u),0,
                M3_R32(base+0xa244u),M3_R32(base+0xa248u));
        M3_V1=M3_OWNER(); uint32_t primitive=M3_V1+M3_S6;
        M3_V1+=M3_S0; M3_W16(M3_V1+0x5bau,M3_V0);
        M3_CALL(0x80043cc4u,0x801e5c14u,primitive);
        M3_W8(M3_OWNER()+M3_S1+0x754u,M3_S4);
        M3_W8(M3_OWNER()+M3_S1+0x755u,M3_S4);
        base=M3_OWNER(); M3_S2++; M3_W8(base+M3_S1+0x756u,M3_S4);
        base=M3_OWNER(); M3_S3+=0x34u; M3_W8(base+M3_S1+0x75cu,M3_S4);
        base=M3_OWNER(); M3_S6+=0x24u; M3_W8(base+M3_S1+0x75du,M3_S4);
        base=M3_OWNER(); M3_S0+=0x34u; M3_W8(base+M3_S1+0x75eu,M3_S4);
        M3_S1+=0x24u;
    } while ((int32_t)M3_S2<8);
    M3_S2=0; M3_S1=0xff; M3_S0=0x908;
    do {
        M3_CALL(0x80043d78u,0x801e5cb4u,M3_OWNER()+M3_S0);
        M3_V0=M3_OWNER(); M3_V1=M3_S2<<4; M3_W8(M3_V0+M3_V1+0x90cu,M3_S1);
        M3_V0=M3_OWNER(); M3_S2++; M3_W8(M3_V0+M3_V1+0x90du,M3_S1);
        M3_V0=M3_OWNER(); M3_S0+=0x10u; M3_V1+=M3_V0;
        M3_V0=(int32_t)M3_S2<12; M3_W8(M3_V1+0x90eu,M3_S1);
    } while (M3_V0);
    entry->cpu->gpr[31]=M3_R32(sp+0x3cu);
    for (unsigned r=0;r<7;r++) entry->cpu->gpr[22-r]=M3_R32(sp+0x38u-r*4u);
    entry->cpu->gpr[29]=sp+0x40u;
}

static uint32_t battle_stage3_glyph_x(BattleEntryCall *entry,uint32_t add)
{
    uint32_t mode=M3_R8(0x800d3280u);
    return (uint32_t)(int32_t)(int16_t)(M3_R16(M3_S6+(mode*3u+M3_S1)*2u)+add+M3_S3);
}

static void battle_stage3_glyphs_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x38u; entry->cpu->gpr[29]=sp;
    M3_W32(sp+0x14u,M3_S1); M3_S1=0;
    M3_W32(sp+0x28u,M3_S6); M3_S6=0x800c3254u;
    M3_W32(sp+0x20u,M3_S4); M3_S4=0;
    M3_W32(sp+0x1cu,M3_S3); M3_S3=0;
    M3_W32(sp+0x30u,M3_FP); M3_FP=0x818;
    M3_W32(sp+0x24u,M3_S5); M3_S5=0;
    M3_W32(sp+0x2cu,M3_S7); M3_S7=0;
    M3_W32(sp+0x34u,entry->cpu->gpr[31]); M3_W32(sp+0x18u,M3_S2); M3_W32(sp+0x10u,M3_S0);
    do {
        if (M3_R8(0x800c3eb6u+M3_S7)!=0x7fu) {
            uint32_t state=M3_STATE(),owner=M3_OWNER();
            uint32_t count=M3_R8(state+M3_S1+0x78u);
            M3_CALL(0x80076a6cu,0x801e5fa8u,0x52,owner+M3_S5+count*80u,battle_stage3_glyph_x(entry,0x44),0x24);
            state=M3_STATE()+M3_S1; M3_V1=M3_R8(state+0x78u)+M3_V0; M3_W8(state+0x78u,M3_V1);
            state=M3_STATE(); owner=M3_OWNER(); M3_S0=M3_R8(state+M3_S1+0x78u);
            uint32_t output=owner+M3_S5+M3_S0*80u,x=battle_stage3_glyph_x(entry,0x44);
            M3_S0<<=1; M3_CALL(0x80076a6cu,0x801e602cu,0x53,output,x,0x24);
            state=M3_STATE()+M3_S1; M3_V1=M3_R8(state+0x78u)+M3_V0; M3_W8(state+0x78u,M3_V1);
            if ((int32_t)M3_S0<(int32_t)(M3_R8(M3_STATE()+M3_S1+0x78u)*2u)) {
                M3_S2=M3_S5;
                do {
                    owner=M3_OWNER(); uint32_t slot=M3_R32(0x800ccb34u);
                    M3_CALL(0x80076c34u,0x801e60a4u,owner+M3_S2+(M3_S0+slot)*40u);
                    count=M3_R8(M3_STATE()+M3_S1+0x78u); M3_S0+=2u;
                } while ((int32_t)M3_S0<(int32_t)(count*2u));
            }
            M3_CALL(0x80076a10u,0x801e6114u,M3_S1+0x61u,M3_OWNER()+M3_FP,battle_stage3_glyph_x(entry,0x1c),0x14);
            M3_S0=M3_S4+0x835cu;
            M3_CALL(0x80076a10u,0x801e6164u,0x90,M3_OWNER()+M3_S0,battle_stage3_glyph_x(entry,0x38),0x27);
            M3_V1=M3_OWNER()+M3_S4; M3_W8(M3_V1+0x853eu,M3_V0);
            M3_S0+=M3_OWNER();
            M3_CALL(0x80076a10u,0x801e61c8u,0x91,M3_S0+0xa0u,battle_stage3_glyph_x(entry,0x3c),0x27);
            M3_V1=M3_OWNER()+M3_S4; M3_W8(M3_V1+0x853fu,M3_V0);
            M3_V0=M3_OWNER()+M3_S4; M3_V1=M3_R8(0x800ccb34u); M3_W8(M3_V0+0x853cu,M3_V1);
        }
        M3_S4+=0x1e4u; M3_S3+=0x60u; M3_FP+=0x50u; M3_S5+=0x1e0u; M3_S1++; M3_S7+=0x1cu;
    } while ((int32_t)M3_S1<3);
    M3_V1=M3_STATE(); M3_V0=M3_R8(0x800ccb34u); M3_W8(M3_V1+0xa2u,M3_V0);
    M3_V1=M3_STATE(); M3_V0=M3_R8(0x800ccb34u); M3_W8(M3_V1+0x83u,M3_V0);
    entry->cpu->gpr[31]=M3_R32(sp+0x34u); M3_FP=M3_R32(sp+0x30u);
    for (unsigned r=0;r<8;r++) entry->cpu->gpr[23-r]=M3_R32(sp+0x2cu-r*4u);
    entry->cpu->gpr[29]=sp+0x38u;
}

static void battle_stage3_panels_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x38u; entry->cpu->gpr[29]=sp;
    M3_W32(sp+0x2cu,M3_S3); M3_S3=0;
    M3_W32(sp+0x30u,M3_S4); M3_S4=0xff;
    M3_W32(sp+0x28u,M3_S2); M3_S2=0x63f8;
    M3_W32(sp+0x24u,M3_S1); M3_S1=0x63c8;
    M3_W32(sp+0x20u,M3_S0); M3_S0=0;
    M3_W32(sp+0x34u,entry->cpu->gpr[31]);
    M3_W16(sp+0x1au,0); M3_W16(sp+0x18u,0); M3_W16(sp+0x1eu,0x100); M3_W16(sp+0x1cu,0x100);
    do {
        M3_CALL(0x80043c9cu,0x801e5d78u,M3_OWNER()+M3_S1);
        M3_W8(M3_OWNER()+M3_S0+0x63ccu,M3_S4);
        uint32_t base=M3_OWNER(); M3_S3++; M3_W8(base+M3_S0+0x63cdu,M3_S4);
        M3_W8(M3_OWNER()+M3_S0+0x63ceu,M3_S4);
        base=M3_OWNER(); M3_S0+=0x18u; M3_CALL(0x80043bfcu,0x801e5dc8u,base+M3_S1,1);
        base=M3_OWNER(); uint32_t x=M3_R32(base+0xa244u),y=M3_R32(base+0xa248u); M3_S1+=0x18u;
        M3_CALL(0x80043a1cu,0x801e5df8u,0,2,x,y);
        uint32_t page=M3_V0&0xffffu; M3_V0=sp+0x18u;
        M3_CALL(0x800454dcu,0x801e5e1cu,M3_OWNER()+M3_S2,0,0,page,sp+0x18u);
        M3_V0=(int32_t)M3_S3<2; M3_S2+=0xcu;
    } while (M3_V0);
    M3_W8(M3_OWNER()+0x6415u,0); M3_V1=M3_OWNER(); M3_V0=0xff;
    M3_W32(M3_V1+0x6410u,M3_V0); M3_W8(M3_V1+0x6416u,0);
    entry->cpu->gpr[31]=M3_R32(sp+0x34u);
    for (unsigned r=0;r<5;r++) entry->cpu->gpr[20-r]=M3_R32(sp+0x30u-r*4u);
    entry->cpu->gpr[29]=sp+0x38u;
}

static void battle_stage3_text_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x28u; entry->cpu->gpr[29]=sp;
    M3_W32(sp+0x1cu,M3_S1); M3_S1=0;
    M3_W32(sp+0x18u,M3_S0); M3_S0=0x800c3e5cu; M3_W32(sp+0x20u,entry->cpu->gpr[31]);
    do {
        M3_CALL(0x8008ac00u,0x801e5e94u,4); M3_W32(M3_S0,M3_V0);
        M3_CALL(0x800338d8u,0x801e5ea0u,M3_S1);
        uint32_t text=M3_V0,buffer=M3_R32(M3_S0); M3_S0+=4u; M3_S1++;
        M3_CALL(0x80034eacu,0x801e5ebcu,text,buffer,2,0);
        M3_V0=(int32_t)M3_S1<10;
    } while (M3_V0);
    entry->cpu->gpr[31]=M3_R32(sp+0x20u); M3_S1=M3_R32(sp+0x1cu); M3_S0=M3_R32(sp+0x18u);
    entry->cpu->gpr[29]=sp+0x28u;
}

static void battle_stage3_first_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x18u; entry->cpu->gpr[29]=sp;
    M3_W32(sp+0x10u,entry->cpu->gpr[31]);
    M3_CALL0(BATTLE_STAGE3_UI_ADDRESS,0x801e6298u);
    M3_CALL0(BATTLE_STAGE3_GLYPHS_ADDRESS,0x801e62a0u);
    entry->cpu->gpr[31]=M3_R32(sp+0x10u); entry->cpu->gpr[29]=sp+0x18u;
}

static void battle_stage3_second_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x18u; entry->cpu->gpr[29]=sp;
    M3_W32(sp+0x10u,entry->cpu->gpr[31]);
    M3_CALL0(BATTLE_STAGE3_PANELS_ADDRESS,0x801e62c0u);
    M3_CALL0(BATTLE_STAGE3_TEXT_ADDRESS,0x801e62c8u);
    entry->cpu->gpr[31]=M3_R32(sp+0x10u); entry->cpu->gpr[29]=sp+0x18u;
}

#undef M3_R8
#undef M3_R16
#undef M3_R32
#undef M3_W8
#undef M3_W16
#undef M3_W32
#undef M3_CALL
#undef M3_CALL0
#undef M3_S0
#undef M3_S1
#undef M3_S2
#undef M3_S3
#undef M3_S4
#undef M3_S5
#undef M3_S6
#undef M3_S7
#undef M3_FP
#undef M3_V0
#undef M3_V1
#undef M3_OWNER
#undef M3_STATE

static int battle_module_stage3_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    static const struct { uint32_t address,bytes; const char *identity; void (*run)(BattleEntryCall *); } bodies[]={
        {BATTLE_STAGE3_UI_ADDRESS,1032,"eb6de59953997d457fa43ffa9e676938dfbc460fe7dec30eafbb13d7578f7d07",battle_stage3_ui_run},
        {BATTLE_STAGE3_GLYPHS_ADDRESS,936,"bfe8cb6f57e203884e1f91cf8b418644bcc2e5a2029a25ed6a48676587967390",battle_stage3_glyphs_run},
        {BATTLE_STAGE3_PANELS_ADDRESS,332,"a55f783e86489c95862caaa80247d99e1f82fc1dba248955acd9c2e0b9e033ba",battle_stage3_panels_run},
        {BATTLE_STAGE3_TEXT_ADDRESS,112,"75e6e3ed87ee362311eb833ca9aab2ad90029d1be82fcc09a69491e81571f812",battle_stage3_text_run},
        {BATTLE_STAGE3_FIRST_ADDRESS,40,"8f85f32be383c109636da8edddc0c9827d21057dac4fd63def659880b48dad80",battle_stage3_first_run},
        {BATTLE_STAGE3_SECOND_ADDRESS,40,"93c6774a8112a49807a1848fdf7c121d31b77939639dcffc010e05d33673a031",battle_stage3_second_run}
    };
    unsigned selected;
    for (selected=0;selected<6;selected++) if (bodies[selected].address==target) break;
    if (selected==6 || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime ||
        runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_MODULE_STAGE3");
    if (control && strcmp(control,"0")==0) return 0;
    uint8_t digest[32];
    for (unsigned region=0;region<2;region++) {
        uint32_t address=region ? bodies[selected].address : BATTLE_MODULE_DISPATCH_ADDRESS;
        unsigned bytes=region ? bodies[selected].bytes : BATTLE_MODULE_DISPATCH_BYTES;
        const char *expected=region ? bodies[selected].identity : BATTLE_MODULE_DISPATCH_SHA256;
        file1_digest(PSX_ADDR(address),bytes,digest);
        for (unsigned i=0;i<sizeof(digest);i++)
            if (file1_hex_digit(digest[i]>>4)!=expected[i*2] ||
                file1_hex_digit(digest[i]&15u)!=expected[i*2+1]) return 0;
    }
    void (*volatile run)(BattleEntryCall *)=bodies[selected].run;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if (setjmp(entry.failure)!=0) return -1;
    run(&entry); cpu_meter_note_kind(CPU_METER_KIND_GENERIC); return 1;
}
