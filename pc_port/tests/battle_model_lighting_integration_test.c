/* Real packet callbacks, descriptor dispatcher, lighting and GTE composition.
 * Allocation is avoided by a zero allocation-size field; no child is modeled. */
#define main lighting_body_main
#include "battle_model_lighting_host_test.c"
#undef main
static unsigned packet_row,packet_mode,packet_dispatch,packet_flags,packet_count,packet_controls,packet_alias,packet_indices,packet_shared_globals;
static uint32_t packet_model,packet_source,packet_commands,packet_output,packet_vertices,packet_normals,packet_light;
static const unsigned lighting_rows[]={0,1,2,3,6,7,8,9,10,11,14,15};
static void packet_defaults(void)
{
 defaults();packet_row=0;packet_mode=3;packet_dispatch=0;packet_flags=1;packet_count=1;
 packet_controls=packet_alias=packet_indices=packet_shared_globals=0;
}
static int packet_textured(unsigned row)
{return row==1||row==3||row==7||row==9||row==11||row==15;}
static uint32_t descriptor(unsigned row,unsigned offset)
{
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);assert(image);
 return load_le(image->bytes+XENO_RD_SLUS_OFF(0x8004fe50u)+row*40u+offset,4);
}
static void packet_setup(PcPortMipsCpu *cpu)
{
 uint32_t callback=descriptor(packet_row,0x18u);int bi=body_index(callback);assert(bi>=0&&bodies[bi].address==callback);
 kind=packet_dispatch?10u:(unsigned)bi;setup(cpu);observing=0;
 if(packet_shared_globals){rt.data[1]=(ResolvedData){0x80059000u,0x1000u,shared+0x18000u,"canonical-packet-globals"};rt.data_count=2;}
 packet_model=token(0x20u);packet_source=token(0x400u);packet_commands=token(0x800u);
 packet_vertices=token(0xa00u);packet_normals=token(0xb00u);packet_light=token(0xc00u);packet_output=token(0xe00u);
 if(packet_alias==1)packet_output=packet_source;
 if(packet_alias==2)packet_light=packet_source;
 if(packet_alias==3)packet_source=initial_sp-0x180u;
 if(packet_alias==4)packet_output=initial_sp-0x140u;
 if(packet_alias==5)packet_output=packet_normals;
 if(packet_alias==6)packet_light=packet_output+4u;
 const int16_t points[][3]={{0,0,128},{0,0,0},{128,0,0},{0,128,0},{0,0,128}};
 for(int i=-1;i<4;i++)for(unsigned j=0;j<4;j++){
  put(packet_vertices+(uint32_t)(i*8)+j*2u,2,j<3?(uint16_t)points[i+1][j]:0x6a5b);
  put(packet_normals+(uint32_t)(i*8)+j*2u,2,j<3?(uint16_t)((i+1)%3==(int)j?(i<1?-4096:4096):0):0x7968);
 }
 for(unsigned i=0;i<0x80u;i+=2)put(packet_light+i,2,i%8u==4?4096:i%8u==6?0x9876:0);
 for(unsigned i=0;i<0x80u;i+=4)put(packet_source+i,4,0x34123456u+i+seed);
 unsigned stride=descriptor(packet_row,0x20u),command_stride=descriptor(packet_row,0x1cu);
 uint32_t src=packet_source,cmd=packet_commands+(packet_dispatch?4u:0u);
 for(unsigned n=0;n<packet_count;n++){
  if(packet_controls&&packet_textured(packet_row)){put(src,4,0xc4004567u);put(src+4u,4,0xc80089abu);src+=8u;}
  put(src,4,0x34123456u+n);put(src+4u,2,0x2345);put(src+6u,2,0x6789);put(src+8u,2,0xabcd);put(src+10u,2,0xef01);
  for(unsigned i=0;i<4;i++)put(cmd+i*2u,2,packet_indices==1?(uint16_t)((int)i-1):packet_indices==2?(i==0?0x8000u:i==1?0x7fffu:i):i);
  src+=stride;cmd+=command_stride;
 }
 put(0x80059424u,4,packet_output);put(0x80059538u,4,packet_source);put(0x80059528u,4,packet_commands);
 put(0x8005953cu,4,packet_vertices);put(0x8005952cu,4,packet_normals);put(0x80059498u,4,packet_light);
 put(0x80050108u,4,seed%3u);put(0x8005010cu,4,seed%2u);put(0x80059310u,4,0xabc00017u);put(0x80059314u,4,0xdef01230u);
 put(0x80059308u,2,0x9876);put(0x8005930cu,2,0x4567);put(0x800595c0u,4,0xfffffff0u);
 cpu->gpr[4]=packet_source;cpu->gpr[5]=packet_commands;cpu->gpr[6]=packet_mode;
 if(packet_dispatch){
  put(packet_model,2,packet_flags);put(packet_model+4u,2,4);put(packet_model+6u,2,1);
  put(packet_model+8u,4,packet_vertices);put(packet_model+0xcu,4,packet_normals);put(packet_model+0x10u,4,packet_commands);
  put(packet_model+0x14u,4,packet_source);put(packet_model+0x18u,4,packet_light);put(packet_model+0x30u,4,0);
  put(packet_commands,1,packet_row);put(packet_commands+2u,2,packet_count);
  cpu->gpr[4]=packet_model;cpu->gpr[5]=packet_output;cpu->gpr[21]=3;
 }
 accesses=events=call_count=legacy_calls=mutation_seen=0;observing=1;
}
static int packet_case(void)
{
 PcPortMipsCpu original,native;packet_setup(&original);candidate=compare=0;oracle_cpu=&original;
 uint32_t target=bodies[kind].address;int original_rc=PcPortMipsRun(&original,target,BATTLE_HALT_PC,200000);
 expected_accesses=accesses;expected_events=events;expected_calls=call_count;snapshot();
 packet_setup(&native);candidate=compare=1;PcPortMipsExecutionStats before=PcPortMipsExecutionStatsGet();
 assert(setenv("XENO_DISABLE_CPU_INTERPRETER","1",1)==0);int rc=PcPortMipsRun(&native,target,BATTLE_HALT_PC,200000);
 assert(unsetenv("XENO_DISABLE_CPU_INTERPRETER")==0);observing=0;
 if(original_rc==PC_PORT_MIPS_BUDGET||(original_rc==PC_PORT_MIPS_HALTED)!=(rc==PC_PORT_MIPS_HALTED))fail("packet status",(uint32_t)rc);
 for(unsigned i=0;i<32;i++)if(original.gpr[i]!=native.gpr[i]){fprintf(stderr,"packet GPR%u native=%08x original=%08x original_pc=%08x\n",i,native.gpr[i],original.gpr[i],original.pc);fail("packet register",i);}
 if(original.hi!=native.hi||original.lo!=native.lo||memcmp(original.cp0,native.cp0,sizeof(native.cp0))||!memory_equal())fail("packet full final state",packet_row);
 if(original_rc==PC_PORT_MIPS_HALTED&&(original.load_valid||native.load_valid))fail("packet pending terminal load",native.load_reg);
 if(native.steps||rt.bridge_cpu||accesses!=expected_accesses||events!=expected_events||call_count!=expected_calls||legacy_calls||PcPortMipsExecutionStatsGet().interpreted_instructions!=before.interpreted_instructions)fail("packet native instructions/counts",packet_row);
 if(fault_at&&accesses!=fault_at)fail("packet unreached fault",fault_at);
 last_oracle=original;last_native=native;return original_rc;
}
static unsigned packet_comparisons(void)
{
 unsigned total=0;
 for(unsigned row=0;row<12;row++)for(unsigned d=0;d<6;d++)for(unsigned mode=0;mode<8;mode++){
  packet_defaults();packet_row=lighting_rows[row];domain_kind=d;packet_mode=mode;stack_kind=d%4u;seed=row*17u+d;packet_shared_globals=d%2u;
  if(packet_case()!=PC_PORT_MIPS_HALTED)fail("packet normal completion",row);
  total++;
 }
 for(unsigned row=0;row<12;row++)for(unsigned mode=0;mode<4;mode++)for(unsigned fl=0;fl<4;fl++){
  packet_defaults();packet_row=lighting_rows[row];packet_dispatch=1;packet_mode=mode;packet_flags=fl;packet_count=2;packet_controls=1;
  packet_shared_globals=fl%2u;seed=row+fl;
  if(packet_case()!=PC_PORT_MIPS_HALTED)fail("dispatcher normal completion",row);
  total++;
 }
 for(unsigned row=0;row<12;row++)for(unsigned a=1;a<=6;a++){
  packet_defaults();packet_row=lighting_rows[row];packet_alias=a;packet_case();total++;
 }
 for(unsigned row=0;row<12;row++)for(unsigned ix=1;ix<3;ix++){
  packet_defaults();packet_row=lighting_rows[row];packet_indices=ix;packet_case();total++;
 }
 packet_defaults();printf("MODEL LIGHTING PACKET PASS %u actual compositions across12 rows/9 lighting callbacks, dispatcher and controls\n",total);return total;
}
static unsigned packet_faults(void)
{
 unsigned total=0;
 for(unsigned row=0;row<12;row++){
  packet_defaults();packet_row=lighting_rows[row];packet_shared_globals=row%2u;packet_case();unsigned n=expected_accesses;
  for(fault_at=1;fault_at<=n;fault_at++){packet_case();total++;}
 }
 packet_defaults();packet_dispatch=1;packet_row=1;packet_controls=1;packet_case();unsigned n=expected_accesses;
 for(fault_at=1;fault_at<=n;fault_at++){packet_case();total++;}
 packet_defaults();printf("MODEL LIGHTING PACKET FAULT PASS %u ordered actual-composition failures\n",total);return total;
}
static unsigned packet_refusals(void)
{
 unsigned total=0;
 for(unsigned row=0;row<12;row++){
  packet_defaults();packet_row=lighting_rows[row];packet_case();unsigned n=expected_calls;
  for(stop_at=1;stop_at<=n;stop_at++){packet_case();total++;}
 }
 packet_defaults();printf("MODEL LIGHTING PACKET CHILD PASS %u actual-composition refusals\n",total);return total;
}
static unsigned packet_semantics(void)
{
 unsigned total=0;const unsigned headers[]={4,7,6,9,6,9,4,9,8,12,8,12};
 for(unsigned row=0;row<12;row++){
  packet_defaults();packet_row=lighting_rows[row];if(packet_case()!=PC_PORT_MIPS_HALTED)fail("packet witness status",row);
  if(peek(packet_output+3u,1)!=headers[row]||peek(packet_output+7u,1)!=0x34u||last_native.gpr[2]!=1||!events)fail("independent packet header/code/GTE",row);
  if(peek(0x80059424u,4)!=packet_output||peek(0x80059538u,4)!=packet_source)fail("callback leaves dispatcher cursors",row);
  total++;
 }
 packet_defaults();printf("MODEL LIGHTING PACKET SEMANTICS PASS %u header/code/cursor witnesses\n",total);return total;
}
int main(int argc,char **argv)
{
 if(argc>1&&strncmp(argv[1],"--packet",8)!=0)return lighting_body_main(argc,argv);
 if(argc>1){
  if(!strcmp(argv[1],"--packet-basic")){packet_defaults();packet_case();return 0;}
  if(!strcmp(argv[1],"--packet-faults"))return packet_faults()?0:1;
  if(!strcmp(argv[1],"--packet-cases"))return packet_comparisons()?0:1;
  if(!strcmp(argv[1],"--packet-semantics"))return packet_semantics()?0:1;
  return 2;
 }
 if(lighting_body_main(argc,argv))return 1;
 if(!packet_comparisons()||!packet_faults()||!packet_refusals()||!packet_semantics())return 1;
 puts("MODEL LIGHTING INTEGRATION ALL PASS native interpreted instructions=0; all covered children actual; no full-game claim");return 0;
}
