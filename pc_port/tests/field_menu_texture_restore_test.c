/* Bounded retail 8007A1D8..8007A21C restoration versus production statements. */
#include "battle_mips_adapter.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {int16_t x,y,w,h;} RECT;
static unsigned char ram[0x200000];
static uint16_t native_vram[1024*512],retail_vram[1024*512];
static uint16_t bottom[64*256],top[64*256];
static unsigned native_calls,retail_calls;
static void copy_rect(uint16_t *vram,const RECT *r,const uint16_t *pixels) {
 if(r->x<0||r->y<0||r->w!=64||r->h!=256||r->x+r->w>1024||r->y+r->h>512) abort();
 for(int y=0;y<r->h;y++)memcpy(vram+(r->y+y)*1024+r->x,pixels+y*r->w,r->w*2);
}
static int LoadImage(RECT *r,void *pixels){copy_rect(native_vram,r,pixels);native_calls++;return 0;}
static int DrawSync(int n){if(n)abort();return 0;}
#include "texture_restore.inc"
static int rd(void*u,uint32_t a,unsigned w,uint32_t*v){
 (void)u;if(a<0x80000000||(uint64_t)a+w>0x80200000)return -1;
 *v=0;for(unsigned i=0;i<w;i++)*v|=(uint32_t)ram[(a&0x1fffff)+i]<<(i*8);return 0;
}
static int wr(void*u,uint32_t a,unsigned w,uint32_t v){
 (void)u;if(a<0x80000000||(uint64_t)a+w>0x80200000)return -1;
 for(unsigned i=0;i<w;i++)ram[(a&0x1fffff)+i]=v>>(i*8);return 0;
}
static int bridge(void*u,PcPortMipsCpu*c,uint32_t target){
 (void)u;
 if(target==0x80044894){
  RECT r;memcpy(&r,ram+(c->gpr[4]&0x1fffff),8);
  uint32_t ptr=c->gpr[5];if(ptr!=0x80140000&&ptr!=0x80150000)return -1;
  copy_rect(retail_vram,&r,(uint16_t*)(ram+(ptr&0x1fffff)));retail_calls++;return 1;
 }
 if(target==0x800445d0)return 1;
 return 0;
}
int main(void){
 FILE*f=fopen("disc/field.bin","rb");if(!f||fread(ram+0x6faf0,1,260862,f)!=260862)return 2;fclose(f);
 for(unsigned seed=0;seed<32;seed++){
  for(unsigned i=0;i<1024*512;i++)native_vram[i]=retail_vram[i]=(uint16_t)(i*37+(i/1024)*13+seed*271);
  for(unsigned i=0;i<64*256;i++){bottom[i]=(uint16_t)(i*71+seed*131);top[i]=(uint16_t)(i*97+seed*337+0x3456);}
  memcpy(ram+0x140000,bottom,sizeof bottom);memcpy(ram+0x150000,top,sizeof top);
  memset(ram+0x1ff000,seed,128);native_calls=retail_calls=0;
  PcPortMipsBus bus={.read=rd,.write=wr,.bridge=bridge};PcPortMipsCpu cpu;PcPortMipsCpuInit(&cpu,&bus);
  cpu.gpr[29]=0x801ff000;cpu.gpr[21]=0x80140000;cpu.gpr[22]=0x80150000;
  if(PcPortMipsRun(&cpu,0x8007a1d8,0x8007a21c,100)){fprintf(stderr,"FAIL retail %s\n",cpu.error);return 1;}
  native_restore(bottom,top);
  if(native_calls!=2||retail_calls!=2||memcmp(native_vram,retail_vram,sizeof native_vram)){
   fprintf(stderr,"FAIL texture restoration seed=%u calls=%u/%u\n",seed,native_calls,retail_calls);return 1;
  }
 }
 puts("FIELD MENU TEXTURE RESTORE PASS cases=32 whole_VRAM=1048576");return 0;
}
