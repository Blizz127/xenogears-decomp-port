static int nonfade_no_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_NONFADE_NO_GLYPH_EXPERIMENT");if(!control || strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint32_t ca[]={0x8008d598u,0x8008dbf8u,0x8008d6a4u,0x8008dbb8u};const unsigned cl[]={0x10c,0x3c,0x34,0x40};
 const char *hash[]={"1dc2f293704c07ccb76c2f27b19dc7705e3eabab11b5e8f84ad118cf0287135e","f2112b2cba612e493a00240435d2a85ed9a48f55ffd627293b2302ddfc2bdbb0","53cb05463b444ef58df301350b47dc352f8bb1715f45c5c30aa016e3b49420d2","3ab0c515c315d0de789aade88129d1e4045b645d883cbf939fc65b1cecfb4e1c"};
 const uint8_t *p[12];size_t plen[]={0x10c,0x3c,0x34,0x40,4,4,4,4,4,2,4,2};
 for(unsigned i=0;i<4;i++){p[i]=resolve_memory(runtime,ca[i],cl[i],0);if(!p[i])return 0;uint8_t digest[32];file1_digest(p[i],cl[i],digest);for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=hash[i][j*2]||file1_hex_digit(digest[j]&15)!=hash[i][j*2+1])return 0;}
 p[4]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[5]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800ccb34u,4,0);if(!p[4]||!p[5]||!p[6])return 0;
 uint32_t source=load_le(p[4],4),state=load_le(p[5],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[7]=resolve_memory(runtime,source,4,0);if(!p[7]||p[7][0]==255 || p[7][2]>7 || (p[7][1]!=255 && p[7][3]>7))return 0;
 p[8]=resolve_memory(runtime,0x800c2f4cu+p[7][2]*4u,4,0);if(!p[8])return 0;
 uint32_t glyph=load_le(p[8],4);if(glyph&1u)return 0;p[9]=resolve_memory(runtime,glyph,2,0);if(!p[9]||load_le(p[9],2)!=0xffffu)return 0;
 unsigned two=p[7][1]!=255; if(two){p[10]=resolve_memory(runtime,0x800c2f4cu+p[7][3]*4u,4,0);if(!p[10])return 0;glyph=load_le(p[10],4);if(glyph&1u)return 0;p[11]=resolve_memory(runtime,glyph,2,0);if(!p[11]||load_le(p[11],2)!=0xffffu)return 0;}
 uint32_t addr[22];unsigned len[22],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;len[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned wid[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];len[n++]=wid[i];}addr[n]=state+0xd0u;len[n++]=4;addr[n]=state+0xd4u;len[n++]=4;
 uint8_t *w[22];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],len[i],1);if(!w[i])return 0;
  for(unsigned j=0;j<(two?12u:10u);j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+plen[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+len[i])return 0;
  for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+len[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+len[i])return 0;
 }
 BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure))return -1;uint32_t *r=cpu->gpr;uint32_t frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[7][i]);battle_entry_write(&e,frame+24u+i,1,p[7][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 battle_entry_write(&e,frame+56u,4,frame+25u);battle_entry_write(&e,frame+64u,4,frame+18u);
 if(two){battle_entry_write(&e,frame+56u,4,frame+26u);battle_entry_write(&e,frame+64u,4,frame+20u);}
 r[2]=load_le(p[6],4);r[3]=p[7][1];r[4]=0xffffu;r[5]=frame+20u;r[6]=2;r[7]=frame+26u;r[8]=0x800c3000u+slot*4u;r[9]=frame+(two?26u:18u);
 cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
