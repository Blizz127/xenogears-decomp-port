/* Distinct register/FIFO values supplement the original-body comparisons. */
#define main fade_comparison_main
#include "battle_color_fade_host_test.c"
#undef main
int main(void)
{
 const uint32_t words[]={0,1,2,15,16,17,255,256,0x7fff,0x8000,0x8001,0xffff,0x12347fff,0xabcd8000,0x7654ffff,0xffffffffu};
 unsigned total=0;
 for(unsigned f=0;f<16;f++)for(unsigned r=0;r<16;r++)for(unsigned g=0;g<16;g++){
  uint32_t state=0xabcdef01u+f*739u+r*313u+g*127u;
  for(unsigned i=0;i<sizeof(gteRegs);i++){state=state*1664525u+1013904223u;((uint8_t *)&gteRegs)[i]=(uint8_t)(state>>24);}
  uint32_t delta[]={words[r],words[g],words[(f+r+g)%16]};
  MTC2(words[f],8);for(unsigned i=0;i<3;i++)MTC2(delta[i],(int)(9u+i));GTERegisters initial=gteRegs;
  color_fade_backend_command(0x4b90003d);GTERegisters expected=gteRegs;gteRegs=initial;
  battle_color_fade_product(words[f],delta);
  if(memcmp(&gteRegs,&expected,sizeof(gteRegs)))fail("distinct graphics register/FIFO state",total);
  total++;
 }
 printf("COLOR FADE GRAPHICS PASS %u full register/FIFO comparisons, native command dispatch absent\n",total);return 0;
}
