/* Actual native atlas, packet and CPU VRAM-mirror services. Original CPU
 * execution is confined to the oracle leg, including original atlas/GetTPage/
 * SetDrawMode bodies. StoreImage uses the existing backend in both legs;
 * copied pixels are also checked against an independently filled VRAM image.
 * Original SDK volatile registers and its 48-byte scratch frames are excluded. */
#define main vram_comparison_main
#include "battle_vram_setup_host_test.c"
#undef main
extern unsigned short vram[1024*512];
extern void func_80026338(uint8_t *,int32_t,uint32_t *,int32_t *,int32_t *,int32_t *,int32_t *,int32_t *);
static uint8_t sdk[0x2300],atlas_code[172];
static unsigned abi_calls;
static RECT16 *abi_rectangle;
static u_long *abi_pixels;
static int abi_result;
static int probe_store(RECT16 *r,u_long *p)
{abi_calls++;abi_rectangle=r;abi_pixels=p;return abi_result;}
static void abi_setup(PcPortMipsCpu *cpu,int cached)
{
 memset(&rt,0,sizeof(rt));rt.initialized=1;g_ActiveBattleRuntime=&rt;observing=0;candidate=1;
 rt.functions[0]=(ResolvedFunction){0x800448f8u,probe_store,"StoreImage"};rt.function_count=1;
 rt.data[0]=(ResolvedData){0x80050000u,0x20000u,shared,"shared"};rt.data_count=1;
 rt.host_ranges[0]=(HostRange){(uintptr_t)host,(uintptr_t)host+sizeof(host),1};rt.host_range_count=1;
 if(cached){BridgeCallCacheSlot *slot=bridge_cache_slot(&rt,0x800448f8u);slot->target=0x800448f8u;slot->state=1;slot->entry=rt.functions[0];}
 initialize_cpu(cpu,&rt);cpu->gpr[2]=0x13579bdfu;cpu->gpr[3]=0x2468ace0u;cpu->gpr[29]=0x1e000000u;
 abi_calls=0;abi_rectangle=NULL;abi_pixels=NULL;abi_result=-12345;
}
static void rectangle_at(uint32_t address,int x,int y,int w,int h)
{fixture_store(address,2,(uint16_t)x);fixture_store(address+2u,2,(uint16_t)y);fixture_store(address+4u,2,(uint16_t)w);fixture_store(address+6u,2,(uint16_t)h);}
static void abi_run(PcPortMipsCpu *cpu,int success,unsigned calls_expected,uint32_t result)
{
 uint32_t before[32];memcpy(before,cpu->gpr,sizeof(before));PcPortMipsExecutionStats stats=PcPortMipsExecutionStatsGet();
 int rc=PcPortMipsRun(cpu,0x800448f8u,BATTLE_HALT_PC,100);
 assert(rc==(success?PC_PORT_MIPS_HALTED:PC_PORT_MIPS_UNRESOLVED_CALL));
 assert(!cpu->steps&&!rt.bridge_cpu&&PcPortMipsExecutionStatsGet().interpreted_instructions==stats.interpreted_instructions);
 if(success)before[2]=result;
 assert(!memcmp(before,cpu->gpr,sizeof(before))&&abi_calls==calls_expected);
}
static unsigned interfaces(void)
{
 const uint32_t addresses[]={0x80140100u,0xa0140100u,0x80340100u,0x00140100u,0x80050100u,0x1f800100u,(uint32_t)(uintptr_t)(host+0x100)};
 const uint32_t bad_rect[]={0x80100101u,0x1e000000u,0x801ffffcu,0x1f800ffcu,(uint32_t)(uintptr_t)(host+sizeof(host)-4u)};
 const uint32_t bad_pixels[]={0x80100101u,0x1e000000u,0x801fff00u,0x1f800f00u,(uint32_t)(uintptr_t)(host+sizeof(host)-256u)};
 const int invalid[][4]={{-1,0,198,1},{0,-1,198,1},{0,0,-1,1},{0,0,1,-1},{1024,0,1,1},{0,512,1,1},{1000,0,25,1},{0,511,1,2},{0,0,1025,1},{0,0,1,513}};
 unsigned total=0;
 for(int cached=0;cached<2;cached++){
  PcPortMipsCpu cpu;
  for(unsigned a=0;a<7;a++)for(unsigned b=0;b<7;b++)for(unsigned sign=0;sign<2;sign++){
   abi_setup(&cpu,cached);cpu.gpr[4]=addresses[a];cpu.gpr[5]=addresses[b];rectangle_at(cpu.gpr[4],1,200,198,1);abi_result=sign?-12345:0x1234567;
   RECT16 *r=module_memory_pointer(&rt,cpu.gpr[4],8,0);u_long *p=module_memory_pointer(&rt,cpu.gpr[5],396,1);
   abi_run(&cpu,1,1,(uint32_t)abi_result);assert(abi_rectangle==r&&abi_pixels==p);total++;
  }
  for(unsigned i=0;i<sizeof(invalid)/sizeof(invalid[0]);i++){
   abi_setup(&cpu,cached);cpu.gpr[4]=addresses[0];cpu.gpr[5]=addresses[1];rectangle_at(cpu.gpr[4],invalid[i][0],invalid[i][1],invalid[i][2],invalid[i][3]);abi_run(&cpu,0,0,0);total++;
  }
  for(unsigned i=0;i<5;i++){
   abi_setup(&cpu,cached);cpu.gpr[4]=bad_rect[i];cpu.gpr[5]=addresses[0];abi_run(&cpu,0,0,0);total++;
   abi_setup(&cpu,cached);cpu.gpr[4]=addresses[0];cpu.gpr[5]=bad_pixels[i];rectangle_at(cpu.gpr[4],1,200,198,1);abi_run(&cpu,0,0,0);total++;
  }
  for(unsigned i=0;i<3;i++){
   abi_setup(&cpu,cached);cpu.gpr[4]=addresses[0];cpu.gpr[5]=addresses[1];rectangle_at(cpu.gpr[4],i==2?1024:0,i==2?512:0,i==1?3:0,i==0?3:0);abi_run(&cpu,1,0,0);total++;
  }
  abi_setup(&cpu,cached);cpu.gpr[4]=addresses[0];cpu.gpr[5]=addresses[6];rectangle_at(cpu.gpr[4],1,200,198,1);rt.host_ranges[0].writable=0;abi_run(&cpu,0,0,0);total++;
  abi_setup(&cpu,cached);cpu.gpr[4]=addresses[0];cpu.gpr[5]=addresses[1];rectangle_at(cpu.gpr[4],1,200,198,1);cpu.load_valid=1;cpu.load_reg=4;abi_run(&cpu,0,0,0);total++;
  abi_setup(&cpu,cached);cpu.gpr[4]=addresses[0];cpu.gpr[5]=0x801e5000u;rectangle_at(cpu.gpr[4],1,200,198,1);rt.file1.verified=1;abi_run(&cpu,1,1,(uint32_t)abi_result);assert(!rt.file1.verified);total++;
 }
 return total;
}
static int real_bridge(void *opaque,PcPortMipsCpu *cpu,uint32_t target)
{
 if(!candidate&&target!=0x800448f8u)return 0;
 return runtime_bridge(opaque,cpu,target);
}
static uint16_t pixel(unsigned x,unsigned y,unsigned salt)
{return (uint16_t)(x*23u+y*173u+salt*997u+(x^y)*41u);}
static unsigned backend_copies(void)
{
 const int rectangles[][4]={{1,200,198,1},{0,0,1024,512},{1017,508,7,4},{350,5,17,9},{0,0,0,0},{1024,512,0,0},{10,20,16,16}};
 const uint32_t destinations[]={0x80100000u,0xa0100000u,0x80300000u,0x00100000u,(uint32_t)(uintptr_t)host,0x80050100u,0x1f800100u};
 unsigned total=0;
 for(unsigned r=0;r<7;r++)for(unsigned d=0;d<7;d++)for(int cached=0;cached<2;cached++){
  unsigned bytes=(unsigned)rectangles[r][2]*(unsigned)rectangles[r][3]*2u;if(bytes>4096u&&d>=5)continue;
  PcPortMipsCpu cpu;abi_setup(&cpu,cached);rt.functions[0].host=StoreImage;
  if(cached)bridge_cache_slot(&rt,0x800448f8u)->entry=rt.functions[0];
  cpu.gpr[4]=0x800d0000u;cpu.gpr[5]=destinations[d];rectangle_at(cpu.gpr[4],rectangles[r][0],rectangles[r][1],rectangles[r][2],rectangles[r][3]);
  for(unsigned y=0;y<512;y++)for(unsigned x=0;x<1024;x++)vram[y*1024u+x]=pixel(x,y,r*13u+d);
  uint8_t *dst=module_memory_pointer(&rt,cpu.gpr[5],bytes?bytes:1u,1);assert(dst);snapshot();uint8_t *expected=NULL;
  const struct {uint8_t *a,*b;size_t size;} regions[]={{g_PsxRam,want_ram,sizeof(g_PsxRam)},{shared,want_shared,sizeof(shared)},{g_PsxScratchpad,want_pad,sizeof(g_PsxScratchpad)},{host,want_host,sizeof(host)}};
  for(unsigned i=0;i<4;i++)if((uintptr_t)dst>=(uintptr_t)regions[i].a&&(uintptr_t)dst+bytes<=(uintptr_t)regions[i].a+regions[i].size)expected=regions[i].b+(dst-regions[i].a);
  assert(expected);
  for(int y=0;y<rectangles[r][3];y++)for(int x=0;x<rectangles[r][2];x++){
   uint16_t value=pixel((unsigned)(rectangles[r][0]+x),(unsigned)(rectangles[r][1]+y),r*13u+d);unsigned offset=(unsigned)(y*rectangles[r][2]+x)*2u;expected[offset]=(uint8_t)value;expected[offset+1u]=(uint8_t)(value>>8);
  }
  PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();int rc=PcPortMipsRun(&cpu,0x800448f8u,BATTLE_HALT_PC,100);
  assert(rc==PC_PORT_MIPS_HALTED&&!cpu.steps&&!rt.bridge_cpu&&cpu.gpr[2]==0&&cpu.gpr[3]==0x2468ace0u&&PcPortMipsExecutionStatsGet().interpreted_instructions==before.interpreted_instructions);
  assert(memory_equal());
  for(unsigned y=0;y<512;y++)for(unsigned x=0;x<1024;x++)assert(vram[y*1024u+x]==pixel(x,y,r*13u+d));
  total++;
 }
 return total;
}
static void prepare_real(PcPortMipsCpu *cpu)
{
 setup(cpu);observing=0;compare=0;cpu->bus.bridge=real_bridge;
 memcpy(PSX_ADDR(0x80043a1cu),sdk,sizeof(sdk));memcpy(PSX_ADDR(0x80026338u),atlas_code,sizeof(atlas_code));
 fixture_store(0x800568d0u,1,0);*(uint8_t *)PSX_ADDR(0x800568d0u)=0;
 rt.functions[0]=(ResolvedFunction){0x80026338u,func_80026338,"func_80026338"};
 rt.functions[1]=(ResolvedFunction){0x80043a1cu,GetTPage,"GetTPage"};
 rt.functions[2]=(ResolvedFunction){0x800448f8u,StoreImage,"StoreImage"};
 rt.functions[3]=(ResolvedFunction){0x800454dcu,SetDrawMode,"SetDrawMode"};rt.function_count=4;
 fixture_store(owner+0xa238u,4,seed%3u);fixture_store(owner+0xa244u,4,640u+seed*3u);fixture_store(owner+0xa248u,4,256u+seed*5u);
 const unsigned indices[]={0x4b,0x50,0x4d,0x4e};
 for(unsigned i=0;i<4;i++){
  uint32_t offset=0x200u+i*0x40u,entry=table+offset,item=entry+4u;
  fixture_store(table+indices[i]*2u+4u,2,offset);fixture_store(entry,2,1);
  fixture_store(item,2,seed*29u+i*31u);fixture_store(item+2u,2,seed+i*7u);
  fixture_store(item+0x10u,2,seed&1u);fixture_store(item+0x12u,2,160u+seed);fixture_store(item+0x14u,2,480u);
  fixture_store(item+0x16u,2,0x380u+i*64u);fixture_store(item+0x18u,2,0x100u);
 }
 for(unsigned y=0;y<512;y++)for(unsigned x=0;x<1024;x++)vram[y*1024u+x]=pixel(x,y,seed);
}
static void verify_pixels(void)
{
 const uint32_t offsets[]={0x8970u,0x8fa0u,0x95d0u,0x9c00u};
 const unsigned ys[]={ticks-1u,ticks,ticks-2u,ticks-3u};
 for(unsigned line=0;line<4;line++)for(unsigned copy=0;copy<4;copy++){
  const uint8_t *p=module_memory_pointer(&rt,owner+offsets[line]+copy*0x18cu,396,0);assert(p);
  for(unsigned x=0;x<198;x++)assert(load_le(p+x*2u,2)==pixel(x+1u,ys[line],seed));
 }
 for(unsigned y=0;y<512;y++)for(unsigned x=0;x<1024;x++)assert(vram[y*1024u+x]==pixel(x,y,seed));
}
static void exclude_sdk_frame(uint32_t entry_sp)
{
 uint8_t *p=module_memory_pointer(&rt,entry_sp-0x78u,48,0);assert(p);
 const struct {uint8_t *actual,*wanted;size_t size;} regions[]={{g_PsxRam,want_ram,sizeof(g_PsxRam)},{shared,want_shared,sizeof(shared)},{g_PsxScratchpad,want_pad,sizeof(g_PsxScratchpad)},{host,want_host,sizeof(host)}};
 for(unsigned i=0;i<4;i++)if((uintptr_t)p>=(uintptr_t)regions[i].actual&&(uintptr_t)p+48u<=(uintptr_t)regions[i].actual+regions[i].size){
  memcpy(regions[i].wanted+(p-regions[i].actual),p,48);return;
 }
 abort();
}
static void integration_case(void)
{
 PcPortMipsCpu original,native;prepare_real(&original);candidate=0;
 int rc=PcPortMipsRun(&original,targets[0],BATTLE_HALT_PC,100000);if(rc!=PC_PORT_MIPS_HALTED)fail(original.error,original.pc);
 verify_pixels();snapshot();prepare_real(&native);candidate=1;uint32_t entry_sp=native.gpr[29];
 PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);
 rc=PcPortMipsRun(&native,targets[0],BATTLE_HALT_PC,100000);assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 if(rc!=PC_PORT_MIPS_HALTED||native.steps||rt.bridge_cpu||native_gte_commands||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("native integration",native.pc);
 verify_pixels();exclude_sdk_frame(entry_sp);
 if(!memory_equal()){
  fprintf(stderr,"entry_sp=%08x GTE_equal=%d\n",entry_sp,!memcmp(&want_gte,&gteRegs,sizeof(gteRegs)));
  const struct {const char *name;uint8_t *a,*b;size_t size;} regions[]={{"RAM",want_ram,g_PsxRam,sizeof(g_PsxRam)},{"shared",want_shared,shared,sizeof(shared)},{"scratch",want_pad,g_PsxScratchpad,sizeof(g_PsxScratchpad)},{"host",want_host,host,sizeof(host)}};
  for(unsigned region=0;region<4;region++){
   unsigned count=0;
   for(size_t i=0;i<regions[region].size;i++)if(regions[region].a[i]!=regions[region].b[i]){
    if(count<32)fprintf(stderr,"%s +%zx original=%02x native=%02x\n",regions[region].name,i,regions[region].a[i],regions[region].b[i]);
    count++;
   }
   fprintf(stderr,"%s total differences=%u\n",regions[region].name,count);
  }
  fail("actual native pixels/atlas/packets and memory outside SDK frame",seed);
 }
 for(unsigned i=16;i<=23;i++)assert(original.gpr[i]==native.gpr[i]);
 assert(original.gpr[29]==native.gpr[29]&&original.gpr[31]==native.gpr[31]);
}
int main(void)
{
 load_inputs();FILE *f=fopen("disc/SLUS_006.64","rb");assert(f&&fseek(f,0x80043a1cu-0x8000f800u,SEEK_SET)==0&&fread(sdk,1,sizeof(sdk),f)==sizeof(sdk));
 assert(fseek(f,0x80026338u-0x8000f800u,SEEK_SET)==0&&fread(atlas_code,1,sizeof(atlas_code),f)==sizeof(atlas_code)&&fclose(f)==0);
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);unsigned abi=interfaces();unsigned copies=backend_copies();assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);
 printf("VRAM BACKEND COPY PASS %u independently patterned multi-row/full-screen/edge/empty/domain copies with exact memory guards\n",copies);
 printf("VRAM READBACK ABI PASS %u concrete signatures, domains, cached dispatch, spans, empty reads, pending and invalidation checks\n",abi);fflush(stdout);
 unsigned total=0;const unsigned domains[]={0,1,2,3,5};mutation=stop_at=fault_at=kind=0;
 for(seed=0;seed<8;seed++)for(unsigned d=0;d<5;d++)for(selection=0;selection<5;selection++)for(stack_kind=0;stack_kind<4;stack_kind++){
  ticks=240u+seed;task_domain=domains[d];integration_case();total++;
 }
 printf("VRAM SETUP INTEGRATION PASS %u actual native atlas/packet/VRAM cases; original atlas/GetTPage/SetDrawMode oracle; native interpreted instructions=0; only SDK volatile state and 48-byte scratch frames excluded\n",total);
 return 0;
}
