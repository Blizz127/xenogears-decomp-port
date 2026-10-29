static int nonfade_empty_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_NONFADE_EMPTY_EXPERIMENT");if(!control || strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint32_t ca[]={0x8008d598u,0x8008dbf8u};const unsigned cl[]={0x10c,0x3c};
 const char *hash[]={"1dc2f293704c07ccb76c2f27b19dc7705e3eabab11b5e8f84ad118cf0287135e","f2112b2cba612e493a00240435d2a85ed9a48f55ffd627293b2302ddfc2bdbb0"};
 const uint8_t *p[6];size_t plen[]={0x10c,0x3c,4,4,4,4};
 for(unsigned i=0;i<2;i++){p[i]=resolve_memory(runtime,ca[i],cl[i],0);if(!p[i])return 0;uint8_t digest[32];file1_digest(p[i],cl[i],digest);for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=hash[i][j*2]||file1_hex_digit(digest[j]&15)!=hash[i][j*2+1])return 0;}
 p[2]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[3]=resolve_memory(runtime,0x800d2d28u,4,0);p[4]=resolve_memory(runtime,0x800ccb34u,4,0);if(!p[2]||!p[3]||!p[4])return 0;
 uint32_t source=load_le(p[2],4),state=load_le(p[3],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[5]=resolve_memory(runtime,source,4,0);if(!p[5]||p[5][0]!=255)return 0;
 uint32_t addr[22];unsigned len[22],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;len[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned wid[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];len[n++]=wid[i];}addr[n]=state+0xd0u;len[n++]=4;addr[n]=state+0xd4u;len[n++]=4;
 uint8_t *w[22];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],len[i],1);if(!w[i])return 0;
  for(unsigned j=0;j<6;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+plen[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+len[i])return 0;
  for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+len[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+len[i])return 0;
 }
 BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure))return -1;uint32_t *r=cpu->gpr;uint32_t frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[5][i]);battle_entry_write(&e,frame+24u+i,1,p[5][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[2]=load_le(p[4],4);r[3]=255u;r[5]=frame+20u;r[6]=2;r[7]=frame+26u;r[8]=0x800c3000u+slot*4u;r[9]=frame+16u;
 cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
