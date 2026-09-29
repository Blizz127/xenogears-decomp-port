/* Execute pinned retail effect callbacks and trig instructions/table, then
 * compare production native dispatch through its real dlsym initialization.
 * Compare defined return values and fault status, not scratch registers.
 * Every nonzero-divisor case must take the native shortcut, so falling back
 * to the same interpreter on both sides cannot pass as native coverage. */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

/* Deliberately non-page-aligned backing addresses (fixed by the runner) expose
 * accidental scalar-to-pointer translation even when its low bits would happen
 * to agree in a normally page-aligned executable. No production layout changes. */
uint8_t g_PsxRam[PSX_RAM_SIZE] __attribute__((section(".trig_psx_ram"), aligned(16)));
uint8_t g_PsxScratchpad[4096] __attribute__((section(".trig_scratch"), aligned(16)));
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames; /* printf-only OT diag counter (runtime) */
void PcPort_PadVblankPump(void) {}

char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *packet, int single)
{ (void)packet; (void)single; abort(); }

static const uint32_t entries[] = {0x800a3490u,0x800a3514u,0x800a3578u,0x800a35c8u};
static BattleMipsRuntime native, oracle;
static unsigned checks;
static void compare(uint32_t phase, uint32_t divisor, uint32_t base) {
    for(unsigned i=0;i<4;++i) {
        PcPortMipsCpu a,b;
        initialize_cpu(&a,&oracle); initialize_cpu(&b,&native);
        a.bus.bridge=NULL;
        a.gpr[4]=b.gpr[4]=phase; a.gpr[5]=b.gpr[5]=divisor; a.gpr[6]=b.gpr[6]=base;
        int expected=PcPortMipsRun(&a,entries[i],BATTLE_HALT_PC,256);
        int actual=PcPortMipsRun(&b,entries[i],BATTLE_HALT_PC,256);
        assert(expected==actual);
        if((int16_t)divisor==0) { assert(expected!=PC_PORT_MIPS_HALTED); }
        else {
            if(expected!=PC_PORT_MIPS_HALTED || a.gpr[2]!=b.gpr[2]) {
                fprintf(stderr,"effect %08x args=%08x,%08x,%08x native=%08x retail=%08x\n",
                        entries[i],phase,divisor,base,b.gpr[2],a.gpr[2]);abort();
            }
            /* Native servicing completes in fewer steps than the retail body. */
            assert(b.steps<a.steps);
        }
        ++checks;
    }
}
static void read_slice(FILE* f,uint32_t address,uint32_t origin,unsigned size) {
    assert(!fseek(f,address-origin,SEEK_SET));
    assert(fread(PSX_ADDR(address),1,size,f)==size);
}
int main(void) {
    FILE* f=fopen("disc/battle.bin","rb");assert(f);
    read_slice(f,0x800a3490u,0x8006faf0u,0x1b0);fclose(f);
    f=fopen("disc/SLUS_006.64","rb");assert(f);
    read_slice(f,0x8003f8b0u,0x8000f800u,0x38);
    read_slice(f,0x800523f0u,0x8000f800u,0x4000);fclose(f);
    initialize_runtime(&native);assert(native.initialized);
    static const uint32_t edges[]={0,1,2,31,32,33,0x7fff,0x8000,0xffff,0x80000001,0x1f800020,0xa000ffff,0xffffffff};
    for(unsigned a=0;a<sizeof(edges)/sizeof(*edges);++a)
    for(unsigned b=0;b<sizeof(edges)/sizeof(*edges);++b)
    for(unsigned c=0;c<sizeof(edges)/sizeof(*edges);++c) compare(edges[a],edges[b],edges[c]);
    for(unsigned a=0;a<65536;++a) compare(0x80000000u|a,7,0xa000001fu);
    printf("BATTLE EFFECT CALLBACKS PASS comparisons=%u (retail bytes, native dispatch, zero-divisor faults)\n",checks);
}
