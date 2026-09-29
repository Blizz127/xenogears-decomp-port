#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "field_clip_control.h"
#include "battle_mips_adapter.h"
#pragma weak PcPort_FieldClipDataStep
uint32_t func_801E6910(uint8_t *obj, int32_t index, uint32_t *flags)
{ (void)obj; (void)index; *flags = 0; return 0; }
void func_801DEF10(uint8_t *root, uint8_t *pose)
{ (void)root; (void)pose; }
void func_801DFE8C(uint8_t *pool, uint8_t *root)
{ (void)pool; (void)root; }
int32_t func_801E632C(uint8_t *object)
{ (void)object; return -1; }
void func_801DF52C(uint8_t *pool, uint8_t *root, int32_t index, int32_t mask)
{ (void)pool; (void)root; (void)index; (void)mask; }
uint32_t func_801DF7F4(uint8_t *pool, uint8_t *root, uint8_t *pose, int32_t loop, int32_t tag)
{ (void)pool; (void)root; (void)pose; (void)loop; (void)tag; return 0; }
void func_801E5C74(uint8_t *object, uint8_t *data, int32_t loop)
{ (void)object; (void)data; (void)loop; }
void func_801E6974(uint8_t *object, uint8_t *pool, uint8_t *node, int32_t flags, int32_t mode, int32_t tag, int32_t loop, int32_t sx, int32_t sy, int32_t sz, int32_t ex, int32_t ey, int32_t ez, int32_t duration)
{ (void)object; (void)pool; (void)node; (void)flags; (void)mode; (void)tag; (void)loop; (void)sx; (void)sy; (void)sz; (void)ex; (void)ey; (void)ez; (void)duration; }
void func_801E6D94(uint8_t *object, uint8_t *node, int32_t flags)
{ (void)object; (void)node; (void)flags; }
uint32_t D_801E8670[10];
uint32_t D_801E85CC;
uint32_t func_801DF0B4(uint8_t *pool, uint8_t *root, uint8_t *pose,
                     int32_t duration, int32_t absolute, int32_t loop, int32_t tag)
{
 (void)pool; (void)root; (void)pose; (void)duration;
 (void)absolute; (void)loop; (void)tag;
 assert(!"opcode 13 requires the focused blend fixture"); return 0;
}
int32_t func_801DC848(uint8_t *root, int32_t scale)
{ (void)root; (void)scale; return 0; }
int32_t func_801DC5C0(uint8_t *root, int32_t scale)
{ (void)root; (void)scale; return 0; }
uint32_t func_801E6830(uint8_t *object, int32_t selector, uint16_t *mask)
{ (void)object; (void)selector; *mask = 0; return 0; }
static uint8_t ram[0x200000];
static int alias_offset = -1;
static int null_object;
static int position_mode = -1;
static unsigned verified_cases;
static struct {uint32_t object[80],node[31*5];uint16_t script[4];} fixture,initial,expected;
static uint8_t*address(uint32_t a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(uint8_t*)(uintptr_t)a;return NULL;}
static int rd(void*u,uint32_t a,unsigned w,uint32_t*v){(void)u;uint8_t*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;}
static int wr(void*u,uint32_t a,unsigned w,uint32_t v){(void)u;uint8_t*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(uint8_t)(v>>(8*i));return 0;}
static const unsigned opcodes[]={0x0c,0x1e,0x24,0x2e,0x30,0x31,0x32,0x33,0x34,0x36,0x37,0x3b,0x48,0x49,0x4b,0x4c,0x4d,0x4e,0x50,0x54,0x55,0x56,0x5c,0x5d,0x5e,0x5f,0x63,0x64,0x6b,0x6d};
static int supported(unsigned opcode){for(unsigned i=0;i<sizeof(opcodes)/sizeof(*opcodes);++i)if(opcodes[i]==opcode)return 1;return 0;}
static void run(unsigned word,unsigned duration,unsigned counter,int32_t ticks,int32_t limit,unsigned test){
 memset(&fixture,0xa5,sizeof(fixture));fixture.script[0]=(uint16_t)word;fixture.script[1]=(uint16_t)duration;*(uint16_t*)((uint8_t*)fixture.object+0x40)=(uint16_t)counter;*(uint16_t*)((uint8_t*)fixture.object+0x42)=(uint16_t)counter;
 fixture.script[2]=(uint16_t)counter;fixture.script[3]=(uint16_t)(duration^counter);
 uint8_t* root=(uint8_t*)fixture.node+2*124;
 *(uint32_t*)((uint8_t*)fixture.object+4)=(uint32_t)(uintptr_t)root;
 *(uint16_t*)((uint8_t*)fixture.object+0x1c)=(uint16_t)(duration^0x8000);
 *(uint16_t*)(root+0x50)=(uint16_t)counter;
 if(position_mode>=0)for(unsigned axis=0;axis<3;++axis){
  int16_t target=(int16_t)(duration+axis*8191u);
  *(uint16_t*)((uint8_t*)fixture.object+0x88+axis*2)=(uint16_t)target;
  *(uint32_t*)(root+0x5c+axis*4)=(uint32_t)(int32_t)target+((position_mode&(1<<axis))?0x10000u:0);
 }
 if((word&255)==0x5d||(word&255)==0x6b)fixture.script[1]=(uint16_t)(int16_t)((int)(duration%5)-2);
 uint8_t* stream = (uint8_t*)fixture.script;
 if(alias_offset >= 0){stream=(uint8_t*)&fixture+alias_offset;memcpy(stream,fixture.script,sizeof(fixture.script));}
 if((word&255)==0x31){
  uint8_t* loop=duration&1?(uint8_t*)fixture.object+0xb0:(uint8_t*)fixture.script+4;
  if(loop<stream+8&&loop+4>stream)loop=(uint8_t*)fixture.object+0x100;
  uint16_t displacement=(uint16_t)(int16_t)(loop-stream),header=(uint16_t)((counter<<8)|0x30),count=(uint16_t)duration;
  memcpy(stream+2,&displacement,2);memcpy(loop,&header,2);memcpy(loop+2,&count,2);
 }
 PcPortFieldClipControl state={.object=null_object?NULL:(uint8_t*)fixture.object,.stream=(uint32_t)(uintptr_t)stream,.limit=limit,.ticks=ticks,.running=1,.postprocess=0,.operand=0xaaaa};
 initial=fixture;PcPortFieldClipControl before=state;
 if((word&255)==0x08 || (word&255)==0x0a || (word&255)==0x0b ||
    (word&255)==0x0d || (word&255)==0x0e || (word&255)==0x10 ||
    (word&255)==0x11 || (word&255)==0x13 || (word&255)==0x18 || (word&255)==0x19 ||
    (word&255)==0x1d || (word&255)==0x1f || (word&255)==0x23 || (word&255)==0x25 ||
    (word&255)==0x26 || (word&255)==0x27)return; /* focused fixtures */
 ++verified_cases;
 if(!supported(word&255)){
  if(PcPort_FieldClipDataStep(&state)!=0||memcmp(&state,&before,sizeof(state))||memcmp(&fixture,&initial,sizeof(fixture))){fprintf(stderr,"CLIP DATA FAIL unhandled instruction modified or accepted: %04x\n",word);assert(0);}return;
 }
 PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[16]=cpu.gpr[19]=state.stream;cpu.gpr[20]=(uint32_t)(uintptr_t)state.object;cpu.gpr[29]=0x801ff000;
 wr(NULL,0x801ff0d0,4,limit);wr(NULL,0x801ff0d8,4,ticks);wr(NULL,0x801ff0e8,4,1);wr(NULL,0x801ff0f0,4,0);
 if(PcPortMipsRun(&cpu,0x801e3d44,0x801e5974,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"CLIP DATA FAIL oracle %s\n",cpu.error);assert(0);}
 expected=fixture;uint32_t expectedTicks,expectedRunning,expectedPost,expectedOperand;
 rd(NULL,0x801ff0d8,4,&expectedTicks);rd(NULL,0x801ff0e8,4,&expectedRunning);rd(NULL,0x801ff0f0,4,&expectedPost);rd(NULL,0x801ff06c,2,&expectedOperand);
 fixture=initial;
 if(PcPort_FieldClipDataStep(&state)!=1||state.object!=before.object||state.limit!=before.limit||state.stream!=cpu.gpr[19]||(uint32_t)state.ticks!=expectedTicks||(uint32_t)state.running!=expectedRunning||(uint32_t)state.postprocess!=expectedPost||state.operand!=expectedOperand||memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"CLIP DATA FAIL test=%u word=%04x duration=%u counter=%u ticks=%d limit=%d alias=%d null=%d\n",test,word,duration,counter,ticks,limit,alias_offset,null_object);assert(0);}
}
int main(void){
 if(!PcPort_FieldClipDataStep){fputs("CLIP DATA FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 const uint16_t values[]={0,0x7fff,0x8000,0xffff};unsigned tests=0;
 for(unsigned word=0;word<65536;++word)for(unsigned seed=0;seed<4;++seed)run(word,values[seed],values[(seed+1)%4],seed&1?1:-1,0,tests++);
 for(unsigned operand=0;operand<65536;++operand)for(unsigned k=0;k<sizeof(opcodes)/sizeof(*opcodes);++k)run(((operand&255)<<8)|opcodes[k],operand,(uint16_t)(operand*8191u),1,0,tests++);
 /* Every aligned object/node stream location that preserves the root pointer.
  * Includes write-before-read timer reset and self-modifying payload overlap. */
 for(alias_offset=0x10;alias_offset<=(int)(sizeof(fixture.object)+sizeof(fixture.node)-sizeof(fixture.script));alias_offset+=2)
  for(unsigned k=0;k<sizeof(opcodes)/sizeof(*opcodes);++k)
   for(unsigned seed=0;seed<4;++seed)
    run((seed*85u<<8)|opcodes[k],values[seed],values[(seed+1)%4],-1,-1,tests++);
 alias_offset=-1;
 for(position_mode=0;position_mode<8;++position_mode)
  for(unsigned displacement=0;displacement<65536;++displacement)
   run(0xfb5c,displacement,0,1,0,tests++);
 position_mode=-1;
 for(unsigned threshold=0;threshold<256;++threshold)
  for(unsigned count=0;count<65536;++count)
   run(0x31,count,threshold,1,0,tests++);
 null_object=1;
 for(unsigned parameter=0;parameter<256;++parameter)run((parameter<<8)|0x24,0,0,1,0,tests++);
 printf("CLIP DATA PASS %u verified retail/state-preservation cases; 30 direct handlers; dependency opcodes have focused fixtures\n",verified_cases);
}
