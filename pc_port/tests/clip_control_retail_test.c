#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "field_clip_control.h"
#include "battle_mips_adapter.h"
#pragma weak PcPort_FieldClipControlStep
static uint8_t ram[0x200000];
static struct {uint32_t object[80];uint16_t script[4];} fixture,initial,expected;
static uint8_t*address(uint32_t a,unsigned w){if(a>=0x80000000u&&(uint64_t)a+w<=0x80200000u)return ram+(a&0x1fffff);uintptr_t lo=(uintptr_t)&fixture;if(a>=lo&&(uint64_t)a+w<=lo+sizeof(fixture))return(uint8_t*)(uintptr_t)a;return NULL;}
static int rd(void*u,uint32_t a,unsigned w,uint32_t*v){(void)u;uint8_t*p=address(a,w);if(!p)return-1;*v=0;for(unsigned i=0;i<w;++i)*v|=(uint32_t)p[i]<<(8*i);return 0;}
static int wr(void*u,uint32_t a,unsigned w,uint32_t v){(void)u;uint8_t*p=address(a,w);if(!p)return-1;for(unsigned i=0;i<w;++i)p[i]=(uint8_t)(v>>(8*i));return 0;}
static int supported(unsigned opcode){uint32_t target;assert(!rd(NULL,0x801dc040+opcode*4,4,&target));return opcode<4||(opcode>=0x20&&opcode<=0x22)||opcode>=113||target==0x801e5974;}
static void run(unsigned word,unsigned duration,unsigned counter,int32_t ticks,int32_t limit,unsigned test){
 memset(&fixture,0xa5,sizeof(fixture));fixture.script[0]=(uint16_t)word;fixture.script[1]=(uint16_t)duration;*(uint16_t*)((uint8_t*)fixture.object+0x40)=(uint16_t)counter;*(uint16_t*)((uint8_t*)fixture.object+0x42)=(uint16_t)counter;
 PcPortFieldClipControl state={.object=(uint8_t*)fixture.object,.stream=(uint32_t)(uintptr_t)fixture.script,.limit=limit,.ticks=ticks,.running=1,.postprocess=0,.operand=0xaaaa};
 initial=fixture;PcPortFieldClipControl before=state;
 if(!supported(word&255)){
  if(PcPort_FieldClipControlStep(&state)!=0||memcmp(&state,&before,sizeof(state))||memcmp(&fixture,&initial,sizeof(fixture))){fprintf(stderr,"CLIP CONTROL FAIL unhandled instruction modified or accepted: %04x\n",word);assert(0);}return;
 }
 PcPortMipsBus bus={.read=rd,.write=wr};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[16]=cpu.gpr[19]=state.stream;cpu.gpr[20]=(uint32_t)(uintptr_t)state.object;cpu.gpr[29]=0x801ff000;
 wr(NULL,0x801ff0d0,4,limit);wr(NULL,0x801ff0d8,4,ticks);wr(NULL,0x801ff0e8,4,1);wr(NULL,0x801ff0f0,4,0);
 if(PcPortMipsRun(&cpu,0x801e3d44,0x801e5974,1000)!=PC_PORT_MIPS_HALTED){fprintf(stderr,"CLIP CONTROL FAIL oracle %s\n",cpu.error);assert(0);}
 expected=fixture;uint32_t expectedTicks,expectedRunning,expectedPost,expectedOperand;
 rd(NULL,0x801ff0d8,4,&expectedTicks);rd(NULL,0x801ff0e8,4,&expectedRunning);rd(NULL,0x801ff0f0,4,&expectedPost);rd(NULL,0x801ff06c,2,&expectedOperand);
 fixture=initial;
 if(PcPort_FieldClipControlStep(&state)!=1||state.stream!=cpu.gpr[19]||(uint32_t)state.ticks!=expectedTicks||(uint32_t)state.running!=expectedRunning||(uint32_t)state.postprocess!=expectedPost||state.operand!=expectedOperand||memcmp(&fixture,&expected,sizeof(fixture))){fprintf(stderr,"CLIP CONTROL FAIL test=%u word=%04x duration=%u counter=%u ticks=%d limit=%d\n",test,word,duration,counter,ticks,limit);assert(0);}
}
int main(void){
 if(!PcPort_FieldClipControlStep){fputs("CLIP CONTROL FAIL missing native body\n",stderr);return 1;}
 FILE*f=fopen("disc/disc1.bin","rb");assert(f);for(unsigned i=0;i<25;++i){assert(!fseek(f,(231361+i)*2352L+24,SEEK_SET));assert(fread(ram+0x1dc000+i*2048,1,2048,f)==2048);}assert(!fclose(f));
 const uint16_t values[]={0,0x7fff,0x8000,0xffff};const int32_t limits[]={-1,0,1,4,0x100,0x400,0x405,0x80000000};unsigned tests=0;
 for(unsigned word=0;word<65536;++word)for(unsigned seed=0;seed<8;++seed)run(word,values[seed%4],values[(seed+1)%4],seed&1?1:-1,limits[seed],tests++);
 for(unsigned duration=0;duration<65536;++duration){run(1,duration,(uint16_t)(duration-1),1,0,tests++);run(1,duration,duration,65536,0,tests++);run(0xff22,duration,(uint16_t)(duration-1),3,0x404,tests++);run(0x0122,duration,(uint16_t)(duration-1),3,0x404,tests++);}
 printf("CLIP CONTROL PASS %u retail/state-preservation cases; control subset only, other handlers explicitly required\n",tests);
}
