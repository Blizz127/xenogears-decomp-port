/* Up to four glyphs with missing character mappings; no draw calls. */
static int missing_mapped_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target!=0x8008d598u||cpu->gpr[31]!=0x80080758u||(cpu->gpr[6]&255u)||runtime->force_interpret||!runtime->initialized||g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_MISSING_MAPPED_GLYPH_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[12];size_t len[]={0x69c,4,4,4,4,4,34,4,1,1,1,1};p[0]=resolve_memory(runtime,target,0x69c,0);if(!p[0])return 0;
 const char *hash="9a6f5146f9ff7abf044add18fddda4ef4878a2b2e1b0f0054a1e31706a043c61";uint8_t digest[32];file1_digest(p[0],0x69c,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[3]=resolve_memory(runtime,0x800ccb34u,4,0);p[7]=resolve_memory(runtime,0x800c3eacu,4,0);if(!p[1]||!p[2]||!p[3]||!p[7])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),fade=load_le(p[7],4);if(source>UINT32_MAX-3u||(state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[4]=resolve_memory(runtime,source,4,0);if(!p[4]||p[4][0]>1||p[4][1]!=255||p[4][2]>7)return 0;
 p[5]=resolve_memory(runtime,0x800c2f4cu+p[4][2]*4u,4,0);if(!p[5])return 0;uint32_t glyph=load_le(p[5],4);if((glyph&1u)||glyph>UINT32_MAX-33u)return 0;p[6]=resolve_memory(runtime,glyph,34,0);if(!p[6])return 0;
 uint32_t id[4];unsigned count=0;for(unsigned i=0;i<4;i++){id[i]=load_le(p[6]+i*8u,2);if(id[i]<0x4000u||id[i]==0xffffu)return 0;count++;if(load_le(p[6]+(i+1u)*8u,2)==0xffffu)break;}if(load_le(p[6]+count*8u,2)!=0xffffu)return 0;
 uint64_t out[4]={0,0,0,0};for(unsigned i=0;i<count;i++){uint64_t lookup=(uint64_t)fade+((cpu->gpr[4]&255u)<<6)+(id[i]&255u);out[i]=(uint64_t)fade+0x2c0u+(id[i]&3u);if(lookup>UINT32_MAX||out[i]>UINT32_MAX)return 0;p[8+i]=resolve_memory(runtime,(uint32_t)lookup,1,0);if(!p[8+i]||*p[8+i]!=255)return 0;}
 uint32_t addr[25];unsigned widths[25],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;widths[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64},sizes[]={2,2,1,1,4,4,4,4,4};for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];widths[n++]=sizes[i];}
 addr[n]=state+0xd0u;widths[n++]=4;addr[n]=state+0xd4u;widths[n++]=4;for(unsigned i=0;i<count;i++){addr[n]=(uint32_t)out[i];widths[n++]=1;}
 uint8_t *w[25];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],widths[i],1);if(!w[i]||resolve_memory(runtime,addr[i],widths[i],0)!=w[i])return 0;for(unsigned j=0;j<8u+count;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+widths[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+widths[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+widths[i])return 0;}
 BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure))return -1;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[4][i]);battle_entry_write(&e,frame+24u+i,1,p[4][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 for(unsigned i=0;i<count;i++)battle_entry_write(&e,(uint32_t)out[i],1,255);battle_entry_write(&e,frame+56u,4,frame+25u);battle_entry_write(&e,frame+64u,4,frame+18u);
 r[2]=load_le(p[3],4);r[3]=255;r[4]=0xffffu;r[5]=id[count-1u]&3u;r[6]=2;r[7]=frame+26u;r[8]=0x800c3000u+slot*4u;r[9]=frame+18u;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
