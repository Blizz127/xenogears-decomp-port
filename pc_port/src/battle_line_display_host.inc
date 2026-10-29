/* Complete 80088B80 caller and two packet finishers. Fixed host operations,
 * checked ordered bus accesses, live call state and original guest frames.
 * The two scoped void services use their actual C signatures and native
 * frames; coordinate words never pass through pointer translation. */
static uint32_t battle_line_asr(uint32_t value, unsigned bits)
{
    return (value >> bits) | ((value & 0x80000000u) ? (~0u << (32u-bits)) : 0);
}

static void battle_line_finish_list(BattleEntryCall *entry, uint32_t count,
                                    uint32_t latch, uint32_t offset,
                                    uint32_t target, uint32_t site)
{
    uint32_t *r=entry->cpu->gpr;
    r[2]=battle_entry_read(entry,r[5]+count,1);
    int active=(int32_t)r[2]>0;
    r[17]=0;
    if (!active) return;
    do {
        uint32_t slot=battle_entry_read(entry,r[5]+latch,1);
        r[2]=r[17]<<1;
        r[3]=slot;
        r[2]+=r[3];
        r[4]=r[2]<<2; r[4]+=r[2]; r[4]<<=3; r[4]+=offset;
        r[31]=site+8u; r[4]+=r[5];
        battle_phase_call(entry,target,site);
        r[5]=0x800d0000u;
        r[5]=battle_entry_read(entry,0x800d2db4u,4);
        uint32_t limit=battle_entry_read(entry,r[5]+count,1);
        r[17]+=1u; r[2]=limit;
        r[2]=(int32_t)r[17]<(int32_t)r[2];
    } while (r[2]);
}

static void battle_line_display_run(BattleEntryCall *entry)
{
    uint32_t *r=entry->cpu->gpr;
    r[2]=0x800d0000u;
    uint32_t p=battle_entry_read(entry,0x800d2d28u,4);
    r[29]-=0x50u; r[2]=p;
    battle_entry_write(entry,r[29]+0x48u,4,r[31]);
    battle_entry_write(entry,r[29]+0x44u,4,r[21]);
    battle_entry_write(entry,r[29]+0x40u,4,r[20]);
    battle_entry_write(entry,r[29]+0x3cu,4,r[19]);
    battle_entry_write(entry,r[29]+0x38u,4,r[18]);
    battle_entry_write(entry,r[29]+0x34u,4,r[17]);
    battle_entry_write(entry,r[29]+0x30u,4,r[16]);
    r[2]=battle_entry_read(entry,r[2]+0xadu,1);
    if (r[2]) {
        r[2]=0x800c0000u;
        r[2]=battle_entry_read(entry,0x800c207cu,1);
        int done=r[2]!=0; r[4]=0;
        if (done) {
            r[31]=0x80088bd4u; r[5]=0x63u;
            battle_phase_call(entry,0x8001bd40u,0x80088bccu);
            r[2]&=0xffu; r[2]=r[2]<0x60u;
            int no_new_line=r[2]!=0; r[4]=0;
            if (!no_new_line) {
                r[31]=0x80088becu; r[5]=4;
                battle_phase_call(entry,0x8001bd40u,0x80088be4u);
                r[4]=0; r[5]=4; r[16]=r[2]&0xffu;
                r[17]=0x800c2054u; r[16]<<=2;
                r[31]=0x80088c0cu; r[16]+=r[17];
                battle_phase_call(entry,0x8001bd40u,0x80088c04u);
                r[2]&=0xffu; r[2]<<=2; r[17]+=r[2];
                p=battle_entry_read(entry,r[16],4);
                r[2]=0x800d0000u; r[6]=p;
                p=battle_entry_read(entry,0x800d2db4u,4);
                uint32_t q=battle_entry_read(entry,r[17]+0x14u,4); r[2]=p;
                p=battle_entry_read(entry,r[2]+0x5d9cu,2); r[7]=q;
                q=battle_entry_read(entry,r[2]+0x5d9eu,2); r[4]=p;
                r[31]=0x80088c38u; r[5]=q;
                battle_phase_call(entry,0x8008887cu,0x80088c30u);
            }
            r[2]=0x800c0000u;
            r[2]=battle_entry_read(entry,0x800c207cu,1);
            done=r[2]!=0;
        }
        if (!done) {
            r[31]=0x80088c54u;
            battle_phase_call(entry,0x80088990u,0x80088c4cu);
            r[3]=0x800c0000u;
            p=battle_entry_read(entry,0x800c2080u,4);
            r[5]=0x800d0000u; r[3]=p;
            r[5]=battle_entry_read(entry,0x800d2db4u,4);
            if ((int32_t)r[3]<0) r[3]+=0xffu;
            r[3]=battle_line_asr(r[3],8);
            r[2]=0x800c0000u;
            p=battle_entry_read(entry,0x800c3a7cu,2);
            r[4]=0x800c0000u; r[2]=p;
            p=battle_entry_read(entry,0x800c2084u,4);
            r[2]+=r[3]; r[4]=p;
            int negative=(int32_t)r[4]<0;
            battle_entry_write(entry,r[5]+0x5d9cu,2,r[2]);
            if (negative) r[4]+=0xffu;
            r[2]=0x800c0000u;
            p=battle_entry_read(entry,0x800c3a80u,2);
            r[3]=battle_line_asr(r[4],8); r[2]=p; r[2]+=r[3];
            battle_entry_write(entry,r[5]+0x5d9eu,2,r[2]);
        }
        r[5]=0x800d0000u;
        p=battle_entry_read(entry,0x800d2db4u,4);
        r[4]=0xb9u; r[5]=p;
        p=battle_entry_read(entry,r[5]+0x5d9cu,2);
        uint32_t q=battle_entry_read(entry,r[5]+0x5d9eu,2);
        r[6]=(uint32_t)(int32_t)(int16_t)p;
        r[31]=0x80088cc4u; r[7]=(uint32_t)(int32_t)(int16_t)q;
        r[5]+=0xd20u; battle_phase_call(entry,0x80076a10u,0x80088cbcu);
        r[3]=0x800d0000u;
        p=battle_entry_read(entry,0x800d2db4u,4);
        r[16]=0x800d0000u; r[3]=p; r[16]=0x800ccb34u;
        battle_entry_write(entry,r[3]+0x5d72u,1,r[2]);
        r[3]=0x800d0000u;
        p=battle_entry_read(entry,0x800d2db4u,4);
        q=battle_entry_read(entry,r[16],1); r[3]=p;
        r[6]=0xa0u; r[2]=q;
        battle_entry_write(entry,r[3]+0x5d94u,1,r[2]);
        r[5]=0x800d0000u;
        p=battle_entry_read(entry,0x800d2db4u,4);
        r[7]=0x64u; r[5]=p;
        p=battle_entry_read(entry,r[5]+0x5d9cu,2);
        r[5]+=0x3c0u; r[4]=p; r[4]&=0xfu;
        r[31]=0x80088d0cu; r[4]+=0xa9u;
        battle_phase_call(entry,0x80076a10u,0x80088d04u);
        r[3]=0x800d0000u;
        r[3]=battle_entry_read(entry,0x800d2db4u,4);
        battle_entry_write(entry,r[3]+0x5d71u,1,r[2]);
        r[3]=0x800d0000u;
        p=battle_entry_read(entry,0x800d2db4u,4);
        q=battle_entry_read(entry,r[16],1); r[3]=p; r[2]=q;
        battle_entry_write(entry,r[3]+0x5d93u,1,r[2]);
        r[5]=0x800d0000u;
        p=battle_entry_read(entry,0x800d2db4u,4); r[4]=0x82u; r[5]=p;
        p=battle_entry_read(entry,r[5]+0x5d9cu,2);
        q=battle_entry_read(entry,r[5]+0x5d9eu,2); r[6]=(uint32_t)(int32_t)(int16_t)p;
        r[31]=0x80088d4cu; r[7]=(uint32_t)(int32_t)(int16_t)q;
        r[5]+=0xfa0u; battle_phase_call(entry,0x80076a10u,0x80088d44u);
        r[3]=0x800d0000u;
        r[3]=battle_entry_read(entry,0x800d2db4u,4);
        battle_entry_write(entry,r[3]+0x5d73u,1,r[2]);
        r[3]=0x800d0000u;
        p=battle_entry_read(entry,0x800d2db4u,4);
        q=battle_entry_read(entry,r[16],1); r[3]=p; r[2]=q;
        battle_entry_write(entry,r[3]+0x5d95u,1,r[2]);
        r[5]=0x800d0000u;
        r[5]=battle_entry_read(entry,0x800d2db4u,4);
        battle_line_finish_list(entry,0x5d72u,0x5d94u,0xd20u,0x80076b68u,0x80088da8u);
        battle_line_finish_list(entry,0x5d73u,0x5d95u,0xfa0u,0x80076b68u,0x80088dfcu);
        r[5]=0x800d0000u;
        r[5]=battle_entry_read(entry,0x800d2db4u,4);
        battle_line_finish_list(entry,0x5d71u,0x5d93u,0x3c0u,0x80076b68u,0x80088e5cu);
        r[2]=0x800d0000u;
        r[2]=battle_entry_read(entry,0x800d2db4u,4); r[5]=r[2]+0x32a0u;
        p=battle_entry_read(entry,r[2]+0x5d9eu,2);
        q=battle_entry_read(entry,r[2]+0x5d9cu,2); r[7]=p;
        r[4]=r[7]&0xfu; r[6]=(uint32_t)(int32_t)(int16_t)q;
        r[4]+=0xc9u; r[7]<<=16; r[31]=0x80088eb0u;
        r[7]=battle_line_asr(r[7],16);
        battle_phase_call(entry,0x80076a10u,0x80088ea8u);
        r[3]=0x800d0000u;
        r[3]=battle_entry_read(entry,0x800d2db4u,4);
        battle_entry_write(entry,r[3]+0x5d96u,1,r[2]);
        r[3]=0x800d0000u;
        p=battle_entry_read(entry,0x800d2db4u,4);
        r[2]=0x800d0000u; r[3]=p;
        r[2]=battle_entry_read(entry,0x800ccb34u,1);
        battle_entry_write(entry,r[3]+0x5d97u,1,r[2]);
        r[5]=0x800d0000u;
        r[5]=battle_entry_read(entry,0x800d2db4u,4);
        battle_line_finish_list(entry,0x5d96u,0x5d97u,0x32a0u,0x80076bf0u,0x80088f10u);
        r[2]=0x800c0000u;
        r[2]=battle_entry_read(entry,0x800c207cu,1);
        done=r[2]!=0; r[17]=0;
        if (!done) {
            r[21]=0xa0000u; r[20]=0;
            do {
                r[18]=0; r[19]=0x820000u;
                do {
                    r[4]=0; r[31]=0x80088f68u; r[5]=9;
                    battle_phase_call(entry,0x8001bd40u,0x80088f60u);
                    r[2]&=0xffu; r[4]=r[2]+0xbau;
                    r[6]=battle_line_asr(r[19],16); r[7]=battle_line_asr(r[21],16);
                    r[16]=r[20]+r[18]; r[18]+=1u;
                    r[5]=r[16]<<2; r[5]+=r[16]; r[5]<<=4;
                    r[2]=0x800d0000u; p=battle_entry_read(entry,0x800d2db4u,4);
                    r[5]+=0x46a0u; r[2]=p; r[31]=0x80088fa0u; r[5]+=r[2];
                    battle_phase_call(entry,0x80076a10u,0x80088f98u);
                    r[2]=0x800d0000u; p=battle_entry_read(entry,0x800ccb34u,4);
                    r[16]<<=1; r[2]=p; r[16]+=r[2];
                    r[4]=r[16]<<2; r[4]+=r[16]; r[4]<<=3;
                    r[2]=0x800d0000u; p=battle_entry_read(entry,0x800d2db4u,4);
                    r[4]+=0x46a0u; r[2]=p; r[31]=0x80088fd0u; r[4]+=r[2];
                    battle_phase_call(entry,0x80076b68u,0x80088fc8u);
                    r[2]=0x60000u; r[19]+=r[2]; r[2]=(int32_t)r[18]<10;
                    int again=r[2]!=0; r[2]=0xbd0000u;
                    if (!again) break;
                } while (1);
                r[21]+=r[2]; r[17]+=1u; r[2]=(int32_t)r[17]<2;
                r[20]+=10u;
            } while (r[2]);
            r[3]=0x800d0000u; p=battle_entry_read(entry,0x800d2db4u,4);
            r[2]=0x800d0000u; r[3]=p;
            r[2]=battle_entry_read(entry,0x800ccb34u,1);
            battle_entry_write(entry,r[3]+0x5d98u,1,r[2]);
        }
    }
    p=battle_entry_read(entry,r[29]+0x48u,4);
    uint32_t q=battle_entry_read(entry,r[29]+0x44u,4); r[31]=p;
    p=battle_entry_read(entry,r[29]+0x40u,4); r[21]=q;
    q=battle_entry_read(entry,r[29]+0x3cu,4); r[20]=p;
    p=battle_entry_read(entry,r[29]+0x38u,4); r[19]=q;
    q=battle_entry_read(entry,r[29]+0x34u,4); r[18]=p;
    p=battle_entry_read(entry,r[29]+0x30u,4); r[17]=q;
    r[29]+=0x50u; r[16]=p;
}

static void battle_line_finish(BattleEntryCall *entry,uint32_t site,uint32_t bits)
{
    uint32_t *r=entry->cpu->gpr;
    r[29]-=0x18u;
    battle_entry_write(entry,r[29]+0x10u,4,r[16]);
    battle_entry_write(entry,r[29]+0x14u,4,r[31]);
    r[31]=site+8u; r[16]=r[4];
    battle_phase_call(entry,0x80076ac8u,site);
    uint32_t page=battle_entry_read(entry,r[16]+0x16u,2);
    r[2]=0x80u; r[3]=page;
    battle_entry_write(entry,r[16]+4u,1,r[2]);
    battle_entry_write(entry,r[16]+5u,1,r[2]);
    battle_entry_write(entry,r[16]+6u,1,r[2]);
    r[3]|=bits; battle_entry_write(entry,r[16]+0x16u,2,r[3]);
    uint32_t ra=battle_entry_read(entry,r[29]+0x14u,4);
    uint32_t s0=battle_entry_read(entry,r[29]+0x10u,4); r[31]=ra;
    r[29]+=0x18u; r[16]=s0;
}
static void battle_line_finish20(BattleEntryCall *e) { battle_line_finish(e,0x80076b74u,0x20u); }
static void battle_line_finish40(BattleEntryCall *e) { battle_line_finish(e,0x80076bfcu,0x40u); }

/* Native C service ABI: void return, preserved bridge registers, native
 * service frames. The complete caller overwrites volatile values before use.
 * These are exact edges; other callers retain their existing dispatch policy. */
static void battle_line_initialize_service(BattleEntryCall *entry)
{
    void (*initialize)(int32_t,int32_t,int32_t,int32_t) =
        (void (*)(int32_t,int32_t,int32_t,int32_t))
        dlsym(RTLD_DEFAULT,"battle_func_8008887C");
    if (!initialize || xeno_port_is_generated_stub("func_8008887C"))
        battle_entry_fail(entry,"missing native line initializer",0x8008887cu);
    uint32_t *r=entry->cpu->gpr;
    initialize((int32_t)r[4],(int32_t)r[5],(int32_t)r[6],(int32_t)r[7]);
}
static void battle_line_interpolate_service(BattleEntryCall *entry)
{
    void (*interpolate)(void)=(void (*)(void))dlsym(RTLD_DEFAULT,"func_80088990");
    if (!interpolate || xeno_port_is_generated_stub("func_80088990"))
        battle_entry_fail(entry,"missing native line interpolation",0x80088990u);
    interpolate();
}

static int battle_line_display_target(uint32_t target,uint32_t pc)
{
    if (target==0x80088b80u) return 1;
    if (target==0x8008887cu) return pc==0x80088c38u;
    if (target==0x80088990u) return pc==0x80088c54u;
    if (target==0x80076b68u)
        return pc==0x80088db0u || pc==0x80088e04u || pc==0x80088e64u || pc==0x80088fd0u;
    return target==0x80076bf0u && pc==0x80088f18u;
}
static int battle_line_display_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if (!battle_line_display_target(target,cpu->gpr[31]) || runtime->force_interpret ||
        !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
        (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_LINE_DISPLAY");
    if (!control || strcmp(control,"1")) return 0;
    unsigned length; const char *expected; void (*run)(BattleEntryCall *);
    if (target==0x80088b80u) {
        length=1208; expected="ea126e6349b05758235589045acf879bd07430341ad6e0d88622a43c485c09af";
        run=battle_line_display_run;
    } else if (target==0x8008887cu) {
        length=276; expected="ad5f4d5dca6a8fbbd4dec059441564c1bd7969a28600b71ccb8c9e8ca69dfc3c"; run=battle_line_initialize_service;
    } else if (target==0x80088990u) {
        length=496; expected="1fce2686a73065a8c7a4d0ae9b02844e3b7659e2ee245449cc7ba7d428277bda"; run=battle_line_interpolate_service;
    } else if (target==0x80076b68u) {
        length=68; expected="1c02ad3a351dbb7ce78ce014633d9cba6ed3d2676440edfa3d0485aba9a41a7a"; run=battle_line_finish20;
    } else {
        length=68; expected="34f1f744242d2fc33d2f4088c61636d10bddd99b100d57d06cc7ef97703a2ee1"; run=battle_line_finish40;
    }
    uint8_t digest[32]; file1_digest(PSX_ADDR(target),length,digest);
    for (unsigned i=0;i<32;i++)
        if (file1_hex_digit(digest[i]>>4)!=expected[2*i] ||
            file1_hex_digit(digest[i]&15u)!=expected[2*i+1]) return 0;
    return battle_phase_execute(runtime,cpu,run);
}
