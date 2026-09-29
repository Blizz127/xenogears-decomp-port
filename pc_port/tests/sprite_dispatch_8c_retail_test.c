#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
extern void func_8001FBE4(void*,uint32_t,void*);
static int calls,angle_calls; static uint32_t point_a,point_b; static int16_t angle_seen;
int32_t func_80023124(int32_t a,int32_t b){++angle_calls;point_a=(uint32_t)a;point_b=(uint32_t)b;return 0x8a5;}
void XenoTestAnimationSetAngle(void*p,int16_t a){assert(p);++calls;angle_seen=a;}
void XenoTestAnimationApplyAngle(void*p,int16_t a){assert(p);++calls;assert(a==angle_seen);}
static void put16(uint8_t*p,uint16_t v){p[0]=v;p[1]=(uint8_t)(v>>8);}
static void put32(uint8_t*p,uint32_t v){memcpy(p,&v,4);}
int main(void){uint8_t p[0x100],ops[2];uint8_t *other=mmap(NULL,0x1000,PROT_READ|PROT_WRITE,MAP_PRIVATE|MAP_ANONYMOUS|MAP_32BIT,-1,0);assert(other!=MAP_FAILED);memset(p,0,sizeof p);memset(other,0,0x20);ops[0]=ops[1]=0;
 put16(p+2,0xfff0);put16(p+10,0x8010);put16(other+2,0x0123);put16(other+10,0xfedc);put32(p+0x74,(uint32_t)(uintptr_t)other);
 func_8001FBE4(p,0x8c,ops);assert(angle_calls==1&&calls==2&&angle_seen==(int16_t)0x8a5);assert(point_a==0x8010fff0u&&point_b==0xfedc0123u);
 memset(p,0,sizeof p);calls=angle_calls=0;func_8001FBE4(p,0x8c,ops);assert(calls==0&&angle_calls==0);munmap(other,0x1000);puts("SPRITE 8C PASS point packing/call order/NULL gate");}
