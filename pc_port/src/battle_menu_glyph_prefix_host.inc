/* Non-fade glyph prefix; the unconverted child is explicitly refused. */
static int nonfade_glyph_prefix_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_NONFADE_GLYPH_PREFIX_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5]||load_le(p[5],2)>=0x4000u)return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure))return -1;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);battle_entry_fail(&e,"glyph child returned without continuation",0x80076a10u);return -1;
}
