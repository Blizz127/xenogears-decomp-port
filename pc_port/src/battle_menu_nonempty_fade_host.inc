/* Instruction-state translation of the bounded fade renderer. */
static void fade_child_nonempty(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr; r[29]-=64;
 battle_entry_write(e,r[29]+52,4,r[23]);r[23]=0;
 battle_entry_write(e,r[29]+48,4,r[22]);r[22]=0;
 battle_entry_write(e,r[29]+32,4,r[18]);r[18]=0;
 battle_entry_write(e,r[29]+44,4,r[21]);r[21]=0x641c;
 battle_entry_write(e,r[29]+40,4,r[20]);r[20]=0;
 battle_entry_write(e,r[29]+56,4,r[31]);battle_entry_write(e,r[29]+36,4,r[19]);battle_entry_write(e,r[29]+28,4,r[17]);battle_entry_write(e,r[29]+24,4,r[16]);
 do {
  r[4]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[18]+r[4]+0xd0,4);r[2]<<=1;
  if((int32_t)r[2]>0){
   r[16]=0;r[17]=r[20];r[19]=r[21];
   do {
    r[5]=1;r[3]=battle_entry_read(e,0x800c3ea4u,4);r[2]=battle_entry_read(e,r[4]+0xa3,1);r[3]+=r[19];r[2]+=r[16];r[4]=r[3]+r[2]*40u;r[31]=0x8008d3c0u;
    r[2]=battle_entry_read(e,r[4]+7,1)|2u;battle_entry_write(e,r[4]+7,1,r[2]);
    r[5]=0;r[2]=battle_entry_read(e,0x800d2d28u,4);r[3]=battle_entry_read(e,0x800c3ea4u,4);r[2]=battle_entry_read(e,r[2]+0xa3,1);r[3]+=r[19];r[2]+=r[16];r[4]=r[3]+r[2]*40u;r[31]=0x8008d3f4u;
    r[2]=battle_entry_read(e,r[4]+7,1)&0xfeu;battle_entry_write(e,r[4]+7,1,r[2]);
    r[4]=battle_entry_read(e,0x800d2d28u,4);r[3]=battle_entry_read(e,r[4]+0xa3,1);r[5]=battle_entry_read(e,0x800c3ea4u,4);r[3]+=r[16];r[2]=r[3]*40u+r[17]+r[5];r[3]=battle_entry_read(e,r[2]+0x6432,2)|0x20u;battle_entry_write(e,r[2]+0x6432,2,r[3]);
    r[3]=battle_entry_read(e,r[4]+0xa3,1)+r[16];r[4]=r[3]*40u;r[2]=r[4]+r[17]+r[5];r[6]=battle_entry_read(e,r[2]+0x6420,1);r[3]=r[17]+r[5];
    if(r[6]){
     r[2]=battle_entry_read(e,0x800c3eacu,4);r[3]+=r[4];r[2]=battle_entry_read(e,r[2]+0x2e0,1);r[23]=1;r[2]=r[6]-(r[2]<<4);battle_entry_write(e,r[3]+0x6420,1,r[2]);
     for(unsigned color=1;color<3;color++){
      r[2]=battle_entry_read(e,0x800d2d28u,4);r[4]=battle_entry_read(e,0x800c3ea4u,4);r[2]=battle_entry_read(e,r[2]+0xa3,1);r[4]+=r[17];r[2]+=r[16];r[3]=r[2]*40u;r[2]=battle_entry_read(e,0x800c3eacu,4);r[2]=battle_entry_read(e,r[2]+0x2e0,1);r[3]+=r[4];r[2]=r[6]-(r[2]<<4);battle_entry_write(e,r[3]+0x6420u+color,1,r[2]);
     }
    }
    r[4]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[18]+r[4]+0xd0,4);r[16]+=2;r[2]=(int32_t)r[16]<(int32_t)(r[2]<<1);
   }while(r[2]);
  }
  r[18]+=4;r[21]+=0xfa0;r[22]++;r[2]=(int32_t)r[22]<2;r[20]+=0xfa0;
 }while(r[2]);
 r[2]=r[23]&0xffu;if(!r[2]){r[2]=battle_entry_read(e,0x800d2d28u,4);battle_entry_write(e,r[2]+0xcb,1,0);}
 const unsigned regs[]={31,23,22,21,20,19,18,17,16};const unsigned off[]={56,52,48,44,40,36,32,28,24};
 for(unsigned i=0;i<9;i++)r[regs[i]]=battle_entry_read(e,r[29]+off[i],4);r[29]+=64;
}

static int fade_nonempty_try_host(BattleMipsRuntime *runtime, PcPortMipsCpu *cpu, uint32_t target)
{
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || !(cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_NONEMPTY_FADE_EXPERIMENT");if(!control || strcmp(control,"1"))return 0;
 const uint32_t code_addr[]={0x8008d598u,0x8008dc00u,0x8008d328u,0x80043bfcu,0x80043c24u};
 const size_t code_len[]={84,52,0x270,40,40};
 const char *hash[]={"721cf358c208576a33672068455c5f4df433d0c7759724b5adde6a7e5ad5d103","61514086c2d8b8fdadba183d3d3f0461d4fc1292e1fcd364633d87fa21597b7d","e4d026d93cfab31685205b6b89e7fd235653b6891f76fa2dbc3b15e51749ef48","f7bd3db94e23b108242338e61ab13d8b1f6877d7a227b90ed3f11c550612804b","0c8699692a9c170bc55cb4e3171f8001f6b2f19d33b1e8c6cbb722317bf88377"};
 const uint8_t *protect[12];size_t plen[12];
 for(unsigned i=0;i<5;i++){protect[i]=resolve_memory(runtime,code_addr[i],code_len[i],0);plen[i]=code_len[i];if(!protect[i])return 0;uint8_t digest[32];file1_digest(protect[i],code_len[i],digest);for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=hash[i][j*2] || file1_hex_digit(digest[j]&15)!=hash[i][j*2+1])return 0;}
 uint32_t sp=cpu->gpr[29];if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+176u)return 0;
 protect[5]=resolve_memory(runtime,0x800d2d28u,4,0);protect[6]=resolve_memory(runtime,0x800c3ea4u,4,0);protect[7]=resolve_memory(runtime,0x800c3eacu,4,0);plen[5]=plen[6]=plen[7]=4;
 if(!protect[5] || !protect[6] || !protect[7])return 0;
 uint32_t state=load_le(protect[5],4),base=load_le(protect[6],4),fade=load_le(protect[7],4);if((state&3)||state>UINT32_MAX-0xd7u||fade>UINT32_MAX-0x2e0u)return 0;
 protect[8]=resolve_memory(runtime,state+0xd0u,8,0);plen[8]=8;protect[9]=resolve_memory(runtime,state+0xa3u,1,0);plen[9]=1;protect[10]=resolve_memory(runtime,fade+0x2e0u,1,0);plen[10]=1;
 if(!protect[8] || !protect[9] || !protect[10])return 0;
 unsigned count[2]={load_le(protect[8],4),load_le(protect[8]+4,4)},slot=*protect[9];if(count[0]>7 || count[1]>7 || !(count[0]+count[1]) || slot>1)return 0;
 uint8_t *writes[48];size_t wlen[48];uint32_t waddr[48];unsigned n=0;
 waddr[n]=state+0xcbu;wlen[n++]=1;
 for(unsigned i=0;i<19;i++){waddr[n]=sp-(i<10?4u+i*4u:112u+(uint32_t[]){8,12,16,20,24,28,32,36,40}[i-10]);wlen[n++]=4;}
 for(unsigned g=0;g<2;g++)for(unsigned k=0;k<count[g];k++){
  uint64_t p=(uint64_t)base+0x641cu+g*0xfa0u+(slot+2u*k)*40u;if(p>UINT32_MAX-24u || (p&1u))return 0;
  waddr[n]=(uint32_t)p+4u;wlen[n++]=4;waddr[n]=(uint32_t)p+22u;wlen[n++]=2;
 }
 for(unsigned i=0;i<n;i++){
  writes[i]=resolve_memory(runtime,waddr[i],wlen[i],1);if(!writes[i] || !resolve_memory(runtime,waddr[i],wlen[i],0))return 0;
  for(unsigned j=0;j<11;j++)if((uintptr_t)writes[i]<(uintptr_t)protect[j]+plen[j] && (uintptr_t)protect[j]<(uintptr_t)writes[i]+wlen[i])return 0;
  for(unsigned j=0;j<i;j++)if((uintptr_t)writes[i]<(uintptr_t)writes[j]+wlen[j] && (uintptr_t)writes[j]<(uintptr_t)writes[i]+wlen[i])return 0;
 }
 BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure))return -1;
 uint32_t *r=cpu->gpr;r[29]-=112;r[6]&=255u;const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};
 for(unsigned i=0;i<10;i++)battle_entry_write(&entry,r[29]+108u-i*4u,4,r[regs[i]]);
 r[31]=0x8008d5d4u;fade_child_nonempty(&entry);
 r[2]=battle_entry_read(&entry,0x800d2d28u,4);r[2]=battle_entry_read(&entry,r[2]+0xa3u,1);
 for(unsigned i=0;i<10;i++)r[regs[i]]=battle_entry_read(&entry,r[29]+108u-i*4u,4);r[29]+=112;
 cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
