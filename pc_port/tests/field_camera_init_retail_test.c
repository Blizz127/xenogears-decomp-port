/* Retail 8007254C..800726E8 against the extracted production C body.
 * Matrix helper boundary is controlled, not a RotMatrix/GTE parity claim. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"
typedef uint8_t u8; typedef int8_t s8; typedef uint16_t u16; typedef int16_t s16; typedef uint32_t u32; typedef int32_t s32;
typedef struct { s32 vx,vy,vz,pad; } VECTOR;
typedef struct { s16 m[3][3],pad; s32 t[3]; } MATRIX;
typedef struct { u8 prefix[0x20]; MATRIX camRotationMatrix; u8 suffix[0xf4]; } Scene;
typedef struct { s32 atStepDistance,eyeStepDistance; s16 targetAngleY,curAngleY; } Interpolation;
static _Alignas(16) u8 ram[0x200000];
#define OBJ(type,off) (*(type*)(ram+(off)))
#define g_Scene OBJ(Scene,0xaf990)
#define g_CamInterpolation OBJ(Interpolation,0xaf984)
#define g_CameraEye OBJ(VECTOR,0xaf880)
#define g_CameraAt OBJ(VECTOR,0xaf890)
#define g_CameraUp OBJ(VECTOR,0xaf8a0)
#define g_CameraEye2 OBJ(VECTOR,0xaf8b0)
#define g_CameraAt2 OBJ(VECTOR,0xaf8c0)
#define D_800AF8D0 OBJ(VECTOR,0xaf8d0)
#define D_800AF8E0 OBJ(s32,0xaf8e0)
#define D_800AF8E4 OBJ(s32,0xaf8e4)
#define D_800AF8E8 OBJ(s32,0xaf8e8)
#define g_FieldCameraMode OBJ(s16,0xaf934)
#define g_CamMovementFlags OBJ(u16,0xaf93c)
#define g_CamAtMovementDuration OBJ(s16,0xaf93e)
#define g_CamEyeMovementDuration OBJ(s16,0xaf960)
static unsigned calls,coverage[103],cases; static PcPortMipsCpu *active;
static void func_80070594(MATRIX *p) {
 if ((u8*)p!=ram+0xaf9b0) {fputs("matrix address mismatch\n",stderr);exit(1);}
 calls++;
 for(unsigned i=0;i<9;i++)((s16*)p)[i]=(i%4==0)?4096:0;
 for(unsigned i=0;i<3;i++)p->t[i]=0;
}
#include CAMERA_INIT_BODY
static int rd(void *o,u32 a,unsigned w,u32 *v){(void)o;if(a<0x80000000u||(uint64_t)a+w>0x80200000u)return -1;if(active&&a==active->pc&&w==4&&a>=0x8007254c&&a<0x800726e8)coverage[(a-0x8007254c)/4]++;*v=0;for(unsigned i=0;i<w;i++)*v|=(u32)ram[(a&0x1fffff)+i]<<(8*i);return 0;}
static int wr(void *o,u32 a,unsigned w,u32 v){(void)o;if(a<0x80000000u||(uint64_t)a+w>0x80200000u)return -1;for(unsigned i=0;i<w;i++)ram[(a&0x1fffff)+i]=v>>(8*i);return 0;}
static int bridge(void *o,PcPortMipsCpu *c,u32 target){(void)o;if(target!=0x80070594)return 0;func_80070594((MATRIX*)(ram+(c->gpr[4]&0x1fffff)));return 1;}
static u32 seed=0x12345678;static u32 random32(void){seed^=seed<<13;seed^=seed>>17;seed^=seed<<5;return seed;}
int main(int argc,char**argv){
 if(argc!=2)return 2;FILE*f=fopen(argv[1],"rb");if(!f)return 2;if(fread(ram+0x6faf0,1,260862,f)!=260862)return 2;fclose(f);
 for(unsigned n=0;n<4096;n++){
  u8 initial[0x500],expected[0x500];for(unsigned i=0;i<sizeof(initial);i++)initial[i]=n<256?n:random32();
  memcpy(ram+0xaf800,initial,sizeof(initial));calls=0;
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);cpu.gpr[29]=0x801ff000;cpu.gpr[31]=0xfffffffc;active=&cpu;
  if(PcPortMipsRun(&cpu,0x8007254c,0xfffffffc,1000)){fprintf(stderr,"retail error %s\n",cpu.error);return 1;}active=NULL;
  unsigned expected_calls=calls;memcpy(expected,ram+0xaf800,sizeof(expected));
  memcpy(ram+0xaf800,initial,sizeof(initial));calls=0;func_8007254C();
  if(calls!=expected_calls){fprintf(stderr,"CAMERA INIT FAIL case%u matrix calls native%u retail%u\n",n,calls,expected_calls);return 1;}
  for(unsigned i=0;i<sizeof(expected);i++)if(ram[0xaf800+i]!=expected[i]){fprintf(stderr,"CAMERA INIT FAIL case%u address%08x native%02x retail%02x\n",n,0x800af800+i,ram[0xaf800+i],expected[i]);return 1;}
  cases++;
 }
 for(unsigned i=0;i<103;i++)if(!coverage[i]){fprintf(stderr,"uncovered instruction %u\n",i);return 1;}
 printf("CAMERA INIT PASS %u cases; 103/103 retail instructions; full0x500-byte state and helper count\n",cases);return 0;
}
