/* Scoped, opt-in retail TransMatrix body for the popup callback. */
static int popup_transmatrix_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 if(target!=0x80049d9cu || cpu->gpr[31]!=0x800bceccu || runtime->force_interpret ||
    !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu ||
    (cpu->load_valid && cpu->load_reg!=0))return 0;
 const char *enabled=getenv("XENO_BATTLE_NATIVE_POPUP_TRANS_MATRIX_EXPERIMENT");
 if(!enabled || strcmp(enabled,"1"))return 0;
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);
 size_t off=XENO_RD_SLUS_OFF(target);const unsigned n=36u;
 uint8_t *code=resolve_memory(runtime,target,n,0);
 if(!image || !image->bytes || image->size<off+n || !code)return 0;
 if(memcmp(code,image->bytes+off,n)) {
  /* Native-only execution leaves main-executable text unmapped in guest RAM. */
  if(!PcPortMipsInterpreterDisabled())return 0;
  for(unsigned i=0;i<n;i++)if(code[i])return 0;
 }
 static const char expected[]="e7c83d5bccd6bc52a2f15133283c464864c5087e31234fa07c436e2946e8eac4";
 uint8_t digest[32];file1_digest(image->bytes+off,n,digest);
 for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
 uint32_t matrix=cpu->gpr[4],vector=cpu->gpr[5];
 if(((matrix|vector)&3u) || matrix>UINT32_MAX-31u || vector>UINT32_MAX-11u)return 0;
 uint8_t *in=resolve_memory(runtime,vector,12u,0),*out=resolve_memory(runtime,matrix+20u,12u,1);
 if(!in || !out || resolve_memory(runtime,matrix+20u,12u,0)!=out)return 0;
 for(unsigned i=0;i<3;i++)if(resolve_memory(runtime,vector+4u*i,4u,0)!=in+4u*i ||
     resolve_memory(runtime,matrix+20u+4u*i,4u,1)!=out+4u*i ||
     resolve_memory(runtime,matrix+20u+4u*i,4u,0)!=out+4u*i)return 0;
 uintptr_t a=(uintptr_t)out,b=(uintptr_t)code;if(a<b+n && b<a+12u)return 0;
 /* Retail loads all three words before the first write, including aliases. */
 uint32_t v[3]={load_le(in,4),load_le(in+4u,4),load_le(in+8u,4)};
 BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure))return -1;
 for(unsigned i=0;i<3;i++)cpu->gpr[8u+i]=v[i];
 for(unsigned i=0;i<3;i++)battle_entry_write(&e,matrix+20u+4u*i,4,v[i]);
 cpu->gpr[2]=matrix;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
