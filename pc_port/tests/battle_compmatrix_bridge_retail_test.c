/* Executes retail CompMatrix instructions against the same GTE core as the
 * native SDK, then checks real runtime symbol resolution, typed pointer
 * dispatch, every output byte and the guest return pointer. */
#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
void PcPort_PadVblankPump(void) {}
unsigned int g_PcPortPresentedFrames;

char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *packet, int single)
{ (void)packet; (void)single; abort(); }


static int low_read(void *ctx, uint32_t a, unsigned n, uint32_t *v) {
 if (a < 0x200000u && a+n <= 0x200000u) { *v=load_le(g_PsxRam+a,n); return 0; }
 return runtime_read(ctx,a,n,v);
}
static int low_write(void *ctx, uint32_t a, unsigned n, uint32_t v) {
 if (a < 0x200000u && a+n <= 0x200000u) { store_le(g_PsxRam+a,n,v);return 0; }
 return runtime_write(ctx,a,n,v);
}


int main(void) {
 FILE *f=fopen("disc/SLUS_006.64","rb");assert(f);
 assert(!fseek(f,0x8004931cu-0x8000f800u,SEEK_SET));
 assert(fread(PSX_ADDR(0x8004931cu),1,0x160,f)==0x160);assert(!fclose(f));
 BattleMipsRuntime native={0},retail={0};initialize_runtime(&native);load_host_ranges(&retail);
 assert(find_function(&native,0x8004931cu));
 unsigned total=0;uint32_t seed=1234567;
 for(unsigned fixture=0;fixture<256;fixture++) for(unsigned domain=0;domain<5;domain++) for(unsigned alias=0;alias<4;alias++) {
 uint8_t initial[128],expected[128];
 for(unsigned i=0;i<128;i++){seed=seed*1664525u+1013904223u;initial[i]=(uint8_t)(seed>>24);}
 if(fixture==0)memset(initial,0,128);
 for(unsigned m=0;m<2;m++) for(unsigned axis=0;axis<3;axis++) store_le(initial+m*32+20+axis*4,4,(uint32_t)(int32_t)(int16_t)load_le(initial+m*32+20+axis*4,2));
 uint8_t *backing=domain==3?g_PsxScratchpad+0x100:g_PsxRam;
 uint32_t base=domain==0?0:domain==1?0x80000000u:domain==2?0xa0000000u:domain==3?0x1f800100u:(uint32_t)(uintptr_t)backing;
 uint32_t a=base,b=alias==3?a:base+32,d=alias==1?a:alias==2?b:base+64;
 memcpy(backing,initial,128);
 PcPortMipsCpu cpu;initialize_cpu(&cpu,&retail);cpu.bus.bridge=NULL;cpu.bus.read=low_read;cpu.bus.write=low_write;
 cpu.gpr[4]=a;cpu.gpr[5]=b;cpu.gpr[6]=d;
 int rc=PcPortMipsRun(&cpu,0x8004931c,BATTLE_HALT_PC,1000);
 assert(rc==PC_PORT_MIPS_HALTED);assert(cpu.gpr[2]==d);memcpy(expected,backing,128);
 memcpy(backing,initial,128);initialize_cpu(&cpu,&native);cpu.gpr[4]=a;cpu.gpr[5]=b;cpu.gpr[6]=d;
 rc=PcPortMipsRun(&cpu,0x8004931c,BATTLE_HALT_PC,1000);
 if(rc!=PC_PORT_MIPS_HALTED||cpu.gpr[2]!=d||memcmp(expected,backing,128)) {
 fprintf(stderr,"COMPMATRIX FAIL fixture=%u domain=%u alias=%u rc=%d v0=%08x expected=%08x %s\n",fixture,domain,alias,rc,cpu.gpr[2],d,cpu.error);return 1;
 }
 assert(native.bridge_cpu==NULL);total++;
 }
 /* Reproduce the captured low-pointer call with valid zero-filled RAM. */
 memset(g_PsxRam+0xc,0,32);memset(g_PsxRam+0x1408,0,32);memset(g_PsxScratchpad,0xa5,32);
 PcPortMipsCpu cpu;initialize_cpu(&cpu,&native);cpu.gpr[4]=0xc;cpu.gpr[5]=0x1408;cpu.gpr[6]=0x1f800000;
 assert(PcPortMipsRun(&cpu,0x8004931c,BATTLE_HALT_PC,1000)==PC_PORT_MIPS_HALTED);
 for(unsigned i=0;i<32;i++)assert(g_PsxScratchpad[i]==0);
 assert(cpu.gpr[2]==0x1f800000);
 printf("COMPMATRIX PASS %u retail comparisons plus captured address pattern: five pointer domains, four alias modes, full 32-byte output and return address\n",total);
 return 0;
}
