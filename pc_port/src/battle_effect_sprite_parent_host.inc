/* Complete BB350 outside one explicit native allocation/list ABI.
 * This staging parent requires opt-in. Its helper keeps the existing native
 * heap/list representation; it does not claim original allocator CPU parity. */
extern void *g_Heap;
extern unsigned int g_HeapIsErrorHandlerOff;

static int battle_effect_sprite_parent_enabled(void)
{
    const char *control=getenv("XENO_BATTLE_NATIVE_EFFECT_SPRITE_PARENT");
    return control&&strcmp(control,"1")==0;
}

static int battle_effect_sprite_parent_identity(uint32_t target,unsigned bytes,
                                                 const char *expected)
{
    const uint8_t *code=PSX_ADDR(target);
    if(target<BATTLE_BASE) {
        const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
        size_t offset=XENO_RD_SLUS_OFF(target);
        if(!image||!image->bytes||image->size<offset+bytes)return 0;
        code=image->bytes+offset;
    }
    uint8_t digest[32];file1_digest(code,bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=expected[i*2]||
           file1_hex_digit(digest[i]&15u)!=expected[i*2+1])return 0;
    return 1;
}

static int battle_effect_sprite_tasks_target(uint32_t target,uint32_t caller)
{
    return target==0x8001d1d8u&&caller==0x800bb3d8u;
}

static void battle_effect_sprite_tasks_run(BattleEntryCall *entry)
{
    PcPortMipsCpu *cpu=entry->cpu;
    /* Exactly one declared o32 stack argument. The original parent delay
     * store has already committed; rejection never rolls that prefix back. */
    uint32_t free_callback=battle_entry_read(entry,cpu->gpr[29]+0x10u,4);
    if(cpu->gpr[4]!=0x19cu||cpu->gpr[5]!=0||cpu->gpr[6]!=0x800bb13cu||
       cpu->gpr[7]!=0x800bb248u||free_callback!=0x800bb314u)
        battle_entry_fail(entry,"unsupported effect sprite allocation ABI",0x8001d1d8u);
    const ResolvedFunction *resolved=find_function(entry->runtime,0x8001d1d8u);
    if(!resolved||!resolved->host||!resolved->name||
       strcmp(resolved->name,"WorkListsAddTasks")!=0)
        battle_entry_fail(entry,"missing typed effect sprite task service",0x8001d1d8u);
    uintptr_t heap=(uintptr_t)g_Heap;
    /* The existing C routine dereferences its allocation without a NULL
     * branch. Adopt its fatal-error allocation policy, not its unchecked
     * NULL-return mode. No list mutation occurs for a rejected precondition. */
    if(g_HeapIsErrorHandlerOff||!heap||heap>UINT32_MAX||(heap&3u)||
       !host_address_valid(entry->runtime,heap,8u,1))
        battle_entry_fail(entry,"unsupported native effect sprite heap",0x8001d1d8u);
    typedef void (*Callback)(void *);
    struct WorkListEntry *(*create)(uint32_t,struct WorkListEntry *,Callback,Callback,Callback)=
        (struct WorkListEntry *(*)(uint32_t,struct WorkListEntry *,Callback,Callback,Callback))resolved->host;
    struct WorkListEntry *allocation=create(0x19cu,NULL,
        (Callback)(uintptr_t)cpu->gpr[6],(Callback)(uintptr_t)cpu->gpr[7],
        (Callback)(uintptr_t)free_callback);
    uintptr_t address=(uintptr_t)allocation;
    if(!address||address>UINT32_MAX||(address&3u)||
       !host_address_valid(entry->runtime,address,0x19cu,1))
        battle_entry_fail(entry,"invalid native effect sprite allocation",0x8001d1d8u);
    /* Keep the host token: native list heads and packed links already use
     * this domain, and all later sprite accesses use the canonical bus. */
    cpu->gpr[2]=(uint32_t)address;
}

static int battle_effect_sprite_tasks_try_host(BattleMipsRuntime *runtime,
                                                PcPortMipsCpu *cpu,uint32_t target)
{
    if(!battle_effect_sprite_tasks_target(target,cpu->gpr[31])||
       runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0)||!battle_effect_sprite_parent_enabled())return 0;
    if(!battle_effect_sprite_parent_identity(0x8001d1d8u,192u,
       "e3cdb57ad6c31bfc1a9540248e0b7fda1172fe6142340d3dd17e41d6374fde4e"))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_effect_sprite_tasks_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#define EP_R(a) battle_entry_read(entry,(a),4)
#define EP_H(a) battle_entry_read(entry,(a),2)
#define EP_B(a) battle_entry_read(entry,(a),1)
#define EP_W(a,v) battle_entry_write(entry,(a),4,(v))
#define EP_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define EP_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define EP_V0 (entry->cpu->gpr[2])
#define EP_V1 (entry->cpu->gpr[3])
#define EP_A0 (entry->cpu->gpr[4])
#define EP_A1 (entry->cpu->gpr[5])
#define EP_A2 (entry->cpu->gpr[6])
#define EP_A3 (entry->cpu->gpr[7])
#define EP_S0 (entry->cpu->gpr[16])
#define EP_S1 (entry->cpu->gpr[17])
#define EP_S2 (entry->cpu->gpr[18])
#define EP_S3 (entry->cpu->gpr[19])
#define EP_S4 (entry->cpu->gpr[20])
#define EP_S5 (entry->cpu->gpr[21])
#define EP_S6 (entry->cpu->gpr[22])
#define EP_S7 (entry->cpu->gpr[23])
#define EP_SP (entry->cpu->gpr[29])
#define EP_FP (entry->cpu->gpr[30])
#define EP_RA (entry->cpu->gpr[31])

static void battle_effect_sprite_parent_call(BattleEntryCall *entry,
                                               uint32_t target,uint32_t site)
{
    PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED)
        battle_entry_fail(entry,"effect sprite parent child stopped",target);
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending effect sprite child return",target);
}

static void battle_effect_sprite_parent_run(BattleEntryCall *entry)
{
    uint32_t p,q;
    EP_SP-=0x40u;EP_W(EP_SP+0x2cu,EP_S5);EP_S5=EP_A0;
    EP_V0=0x800c0000u;EP_W(EP_SP+0x38u,EP_FP);EP_FP=EP_V0+0x3eb0u;
    EP_V0=EP_S5<<2;EP_W(EP_SP+0x34u,EP_S7);EP_S7=EP_V0+EP_FP;
    EP_V0=0x8c8cu;EP_W(EP_SP+0x30u,EP_S6);EP_S6=EP_S7+EP_V0;
    EP_W(EP_SP+0x3cu,EP_RA);EP_W(EP_SP+0x28u,EP_S4);
    EP_W(EP_SP+0x24u,EP_S3);EP_W(EP_SP+0x20u,EP_S2);
    EP_W(EP_SP+0x1cu,EP_S1);EP_W(EP_SP+0x18u,EP_S0);
    EP_V0=EP_R(EP_S6);EP_A0=0x19cu;if(EP_V0)goto restore;
    EP_S2=0x80060000u;EP_A1=0;EP_V0=0x800bb314u;
    EP_A2=0x800bb13cu;EP_A3=0x800c0000u;
    EP_S4=EP_B(EP_S2-0x6e54u);EP_A3=0x800bb248u;
    EP_SB(EP_S2-0x6e54u,0);
    EP_RA=0x800bb3d8u;EP_W(EP_SP+0x10u,EP_V0);
    battle_effect_sprite_parent_call(entry,0x8001d1d8u,0x800bb3d0u);
    EP_S1=EP_V0;EP_S0=EP_S1+0x38u;EP_A0=EP_S0;
    EP_W(EP_S0+0x6cu,EP_S1);EP_W(EP_S1+4u,EP_S0);
    EP_RA=0x800bb3f4u;EP_W(EP_S1+0x20u,EP_S0);
    battle_effect_sprite_parent_call(entry,0x80023804u,0x800bb3ecu);
    EP_RA=0x800bb3fcu;EP_A0=EP_S0;
    battle_effect_sprite_parent_call(entry,0x800239a0u,0x800bb3f4u);
    EP_V1=0xfffe1fffu;
    p=EP_R(EP_S0+0x40u);q=EP_R(EP_S0+0x7cu);
    EP_V0=p;EP_V0&=EP_V1;EP_A0=q;EP_V0|=0x8000u;
    EP_W(EP_S0+0x40u,EP_V0);
    EP_V0=EP_R(EP_S0+0x3cu);EP_S3=0xfffffffcu;
    EP_W(EP_S0+0x6cu,EP_S1);EP_V0&=EP_S3;EP_W(EP_S0+0x3cu,EP_V0);
    EP_V0=EP_R(EP_S0+0xa8u);EP_V1=0xfffffffeu;EP_V0&=EP_V1;
    EP_W(EP_S0+0xa8u,EP_V0);EP_V0=0x80060000u;EP_W(EP_A0+8u,0);
    p=EP_R(EP_S0+0x7cu);q=EP_H(0x800591a8u);EP_V1=p;
    EP_SH(EP_V1+0xcu,0);EP_V0=q;
    EP_V1=EP_S5<<3;EP_V1-=EP_S5;EP_V1<<=2;EP_V1+=EP_FP;
    EP_SH(EP_S0+0x82u,EP_V0);
    EP_V0=EP_H(EP_V1+0xeu);EP_V0<<=16;EP_W(EP_S1+0x38u,EP_V0);
    EP_V0=EP_H(EP_V1+0x10u);EP_A0=EP_S5;EP_W(EP_S0+4u,0);EP_V0<<=16;
    EP_RA=0x800bb48cu;EP_W(EP_S0+8u,EP_V0);
    battle_effect_sprite_parent_call(entry,0x800aa600u,0x800bb484u);
    EP_A0=EP_S0;EP_A1=0x2000u;EP_SH(EP_S0+0x36u,EP_V0);
    EP_V0=EP_H(EP_S0+0x36u);EP_V1=EP_A1;EP_SH(EP_S0+0x82u,EP_V1);
    EP_V0>>=1;EP_RA=0x800bb4b0u;EP_SH(EP_S0+0x38u,EP_V0);
    battle_effect_sprite_parent_call(entry,0x80022000u,0x800bb4a8u);
    EP_V1=0x3fff0000u;EP_V0=0x8006be10u;EP_W(EP_S0+0x24u,EP_V0);
    EP_V0=0x8cb8u;EP_V0+=EP_S7;EP_V1|=0xffffu;
    EP_W(EP_S6,EP_S0);EP_W(EP_V0,EP_S1);
    EP_A0=EP_R(EP_S0+0xa8u);EP_V0=EP_S5<<30;
    EP_W(EP_S0+0x4cu,0);EP_W(EP_S0+0x48u,0);EP_SB(EP_S2-0x6e54u,EP_S4);
    EP_A0&=EP_V1;EP_A0|=EP_V0;EP_V0=EP_S5>>2;
    EP_V1=EP_R(EP_S0+0xacu);EP_V0&=3u;EP_W(EP_S0+0xa8u,EP_A0);
    EP_V1&=EP_S3;EP_V1|=EP_V0;EP_W(EP_S0+0xacu,EP_V1);
restore:
    p=EP_R(EP_SP+0x3cu);q=EP_R(EP_SP+0x38u);EP_RA=p;
    p=EP_R(EP_SP+0x34u);EP_FP=q;
    q=EP_R(EP_SP+0x30u);EP_S7=p;
    p=EP_R(EP_SP+0x2cu);EP_S6=q;
    q=EP_R(EP_SP+0x28u);EP_S5=p;
    p=EP_R(EP_SP+0x24u);EP_S4=q;
    q=EP_R(EP_SP+0x20u);EP_S3=p;
    p=EP_R(EP_SP+0x1cu);EP_S2=q;
    q=EP_R(EP_SP+0x18u);EP_S1=p;
    EP_SP+=0x40u;EP_S0=q;
}

static int battle_effect_sprite_parent_try_host(BattleMipsRuntime *runtime,
                                                 PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x800bb350u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0)||!battle_effect_sprite_parent_enabled())return 0;
    if(!battle_effect_sprite_parent_identity(target,496u,
       "a0c09a350d171f71a4e7d61326df9c2f47d33287233d330a947532b1a548d8d7"))return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if(setjmp(entry.failure)!=0)return -1;
    battle_effect_sprite_parent_run(&entry);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
#undef EP_R
#undef EP_H
#undef EP_B
#undef EP_W
#undef EP_SH
#undef EP_SB
#undef EP_V0
#undef EP_V1
#undef EP_A0
#undef EP_A1
#undef EP_A2
#undef EP_A3
#undef EP_S0
#undef EP_S1
#undef EP_S2
#undef EP_S3
#undef EP_S4
#undef EP_S5
#undef EP_S6
#undef EP_S7
#undef EP_SP
#undef EP_FP
#undef EP_RA
