/* Component checks for retained battle-to-resident service bindings. */
extern uint32_t g_RandomSeed;
int g_GameSceneMapNum;
extern uint8_t func_8001BD40(uint8_t, uint8_t);
extern uint16_t func_80089C08(uint8_t);

static void tracked_zero(void *dst, size_t len) { memset(dst, 0, len); }
static void unexpected_leaf(void) { abort(); }

static int no_native_oracle(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    (void)opaque;
    (void)cpu;
    (void)target;
    return 0;
}

static PcPortMipsCpu prepare_service(uint32_t a, uint32_t b)
{
    PcPortMipsCpu cpu;
    begin_call(&cpu, BATTLE_HALT_PC);
    cpu.gpr[4] = a;
    cpu.gpr[5] = b;
    return cpu;
}

static void run_service(PcPortMipsCpu *cpu, uint32_t address, int native)
{
    if (!native)
        cpu->bus.bridge = no_native_oracle;
    PcPortMipsExecutionStats before = PcPortMipsExecutionStatsGet();
    int status = PcPortMipsRun(cpu, address, BATTLE_HALT_PC, 1000);
    if (status != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "resident service %08x native=%d status=%d pc=%08x: %s\n",
                address, native, status, cpu->pc, cpu->error);
        abort();
    }
    PcPortMipsExecutionStats after = PcPortMipsExecutionStatsGet();
    if (native) {
        assert(cpu->steps == 0 && after.interpreted_instructions == before.interpreted_instructions);
        assert(after.native_bridges == before.native_bridges + 1);
    } else {
        assert(cpu->steps > 0 && after.native_bridges == before.native_bridges);
    }
}

static void check_service_memory(void)
{
    static uint8_t native_region[128];
    const uint32_t bases[] = {0x80120000, 0xa0120000, 0x120000, 0x80060000, 0x60000, 0x1f800100, (uint32_t)(uintptr_t)native_region};
    assert((uintptr_t)native_region <= UINT32_MAX);
    runtime.host_ranges[0]=(HostRange){(uintptr_t)native_region,(uintptr_t)(native_region+sizeof(native_region)),1};
    runtime.host_range_count=1;
    for (size_t i=0; i<sizeof(bases)/sizeof(*bases); ++i) {
        uint32_t base=bases[i];
        uint8_t *storage=module_memory_pointer(&runtime,base,96,1);
        assert(storage);
        for (unsigned direction=0;direction<2;++direction) {
            uint8_t expected[96];
            for(unsigned k=0;k<sizeof(expected);++k) expected[k]=storage[k]=(uint8_t)(k*7);
            unsigned dst=direction?7:0,src=direction?0:7;
            memmove(expected+dst,expected+src,64);
            PcPortMipsCpu cpu=prepare_service(base+dst,base+src);
            cpu.gpr[6]=64;
            run_service(&cpu,0x8003f99c,1);
            assert(cpu.gpr[2]==base+dst && !cpu.gpr[3] && memcmp(storage,expected,96)==0);
            memset(expected+3,0,39);
            cpu=prepare_service(base+3,39);cpu.gpr[2]=0x12345678;cpu.gpr[3]=0xabcdef98;
            run_service(&cpu,0x8003f8e8,1);
            assert(cpu.gpr[2]==0x12345678 && cpu.gpr[3]==0xabcdef98 && memcmp(storage,expected,96)==0);
        }
    }
    const uint32_t truncated[]={0x801ffff0,0x1f800ff0};
    for(unsigned k=0;k<2;++k) {
        PcPortMipsCpu cpu=prepare_service(truncated[k],0x80120000);
        cpu.gpr[6]=32;
        assert(runtime_bridge(&runtime,&cpu,0x8003f99c)<0);
        cpu.gpr[5]=UINT32_MAX;
        assert(runtime_bridge(&runtime,&cpu,0x8003f8e8)<0);
    }
    PcPortMipsCpu cpu=prepare_service(0,0);
    assert(runtime_bridge(&runtime,&cpu,0x8003f99c)==1 && cpu.gpr[2]==0);
    assert(runtime_bridge(&runtime,&cpu,0x8003f8e8)==1);
    for(unsigned k=0;k<256;++k) {
        /* Do not mask the argument on the caller side. */
        store_le(PSX_ADDR(0x800c3448+2*k),2,k*251+17);
        cpu=prepare_service(0xfedcba00|k,0);run_service(&cpu,0x80089c08,1);
        assert(cpu.gpr[2]==(uint16_t)(k*251+17));
    }
}

static void check_service_rng(void)
{
    const uint32_t entries[]={0x8003fa38,0x8001bd40};
    const unsigned counts[]={512,65536};
    for(unsigned g=0;g<2;g++) for(unsigned k=0;k<counts[g];k++) {
        uint32_t seed=0x8badf00du+k*0x45d9f3bu;
        uint32_t a=0xf0000000u|(k>>8),b=0xa0000000u|(k&255);
        PcPortMipsCpu guest=prepare_service(a,b),host=prepare_service(a,b);
        g_RandomSeed=seed;run_service(&guest,entries[g],0);
        const uint32_t expected=g_RandomSeed;
        g_RandomSeed=seed;run_service(&host,entries[g],1);
        assert(host.gpr[2]==guest.gpr[2] && g_RandomSeed==expected);
    }
}

static void check_service_clut(void)
{
    const uint32_t v[]={0,1,0x7fff,0x8000,0xffff,UINT32_MAX,0x80000000,0x7fffffff};
    for(unsigned x=0;x<8;x++) for(unsigned y=0;y<8;y++) {
        PcPortMipsCpu guest=prepare_service(v[x],v[y]),host=prepare_service(v[x],v[y]);
        run_service(&guest,0x80043a58,0);run_service(&host,0x80043a58,1);
        assert(guest.gpr[2]==host.gpr[2]);
    }
}

static void check_service_draw(void)
{
    uint8_t *area=PSX_ADDR(0x80151000);
    for(unsigned k=0;k<520;k++) {
        uint32_t a=0x80151000,b=(! (k&1) || k<512)?a+32:a;
        if(k&2){a|=0x20000000u;b|=0x20000000u;}
        uint32_t target=k<512?0x8003f738:0x80043b48;
        uint8_t original[96]={0},expected[96];
        for(unsigned i=0;i<3;i++)store_le(original+2*i,2,k*32771u+i*3571u);
        store_le(original+(b-a),4,0x07ffffff);
        memcpy(area,original,sizeof(original));
        PcPortMipsCpu guest=prepare_service(a,b);
        run_service(&guest,target,0);memcpy(expected,area,sizeof(expected));
        memcpy(area,original,sizeof(original));
        PcPortMipsCpu host=prepare_service(k>=516?a&0x1fffffffu:a,k>=516?b&0x1fffffffu:b);
        run_service(&host,target,1);
        assert(host.gpr[2]==guest.gpr[2] && host.gpr[3]==guest.gpr[3] && host.hi==guest.hi && host.lo==guest.lo);
        assert(memcmp(expected,area,sizeof(expected))==0);
    }
}

static void check_retained_services(const char *slus)
{
    load_at(slus,0x8000f800,303104);
    const ResolvedFunction f[]={
        {0x8001bd40,(void*)func_8001BD40,"func_8001BD40"},
        {0x8003f738,(void*)unexpected_leaf,"RotMatrix"},
        {0x8003f8e8,(void*)tracked_zero,"bzero"},
        {0x8003f99c,(void*)memmove,"memmove"},
        {0x8003fa38,(void*)rand,"rand"},
        {0x80043a58,(void*)unexpected_leaf,"GetClut"},
        {0x80043b48,(void*)unexpected_leaf,"AddPrim"},
        {0x80089c08,(void*)func_80089C08,"func_80089C08"},
    };
    memcpy(runtime.functions,f,sizeof(f));runtime.function_count=sizeof(f)/sizeof(*f);
    runtime.data[0]=(ResolvedData){0x80050000,0xa1fc,PSX_ADDR(0x80050000),"resident globals"};
    runtime.data[1]=(ResolvedData){0x8005a1fc,4,(uint8_t*)&g_RandomSeed,"random seed"};
    runtime.data[2]=(ResolvedData){0x8005a200,0x5e00,PSX_ADDR(0x8005a200),"resident globals"};
    runtime.data[3]=(ResolvedData){0x80060000,0x10000,PSX_ADDR(0x80060000),"resident globals"};runtime.data_count=4;
    check_service_memory();
    check_service_rng();
    check_service_clut();
    check_service_draw();
    puts("resident checks passed: memory alias and bounds, 256 mask inputs, 512 random seeds, 65536 byte ranges, 64 CLUT coordinates, 512 matrices and 8 packet links");
}
