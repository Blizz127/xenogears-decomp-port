/* Scoped snapshots make vblank callbacks unable to invalidate preflight. */
typedef struct OneGlyphGuard {
 BattleMipsRuntime *runtime;const uint8_t *ptr[156];size_t len[156];uint32_t addr[156];uint8_t digest[156][32];unsigned protected_count;uint32_t shade;
 uint32_t stack_addr,state_addr,packet_addr;uint8_t *stack,*a,*b,*packet[50];
 const XenoRetailFileSlot *image;const uint8_t *image_bytes;size_t image_size;uint32_t saved[10],arg0;unsigned descriptor_count,current,packet_count,glyph_counts[32],glyph_total;uint32_t shades[32],extra_raw[25],extra_resolved[25],extra_desc[25],child_saved[10];uint32_t second_glyph,second_resolved,third_glyph,third_resolved,fourth_glyph,fourth_resolved,fifth_glyph,fifth_resolved,sixth_glyph,sixth_resolved,seventh_glyph,seventh_resolved,resolved_glyph,map_addr,map_value,map_addr2,map_value2,map_addr3,map_value3,map_addr4,map_value4;uint8_t *map_output,*map_output2,*map_output3,*map_output4;unsigned map_written,map_initial,map_written2,map_initial2,map_written3,map_initial3,map_written4,map_initial4,map_sequence[32],dynamic_map_count,dynamic_addr[32],dynamic_value[32],dynamic_initial[32],dynamic_written[32];uint8_t *dynamic_output[32];unsigned allow_reused_mapping,second_mapped,third_mapped,fourth_mapped,map_first_later;
} OneGlyphGuard;
static _Thread_local OneGlyphGuard *one_glyph_active;
static int one_glyph_guard_current(OneGlyphGuard *g,PcPortMipsCpu *cpu,uint32_t target)
{
 BattleMipsRuntime *runtime=g->runtime;if(!runtime->initialized||runtime->force_interpret||g_ActiveBattleRuntime!=runtime)return 0;
 if(!g->image||g->image->state<=0||g->image->bytes!=g->image_bytes||g->image->size!=g->image_size)return 0;
 for(unsigned i=0;i<g->protected_count;i++){
  if((g->addr[i]||i>=49u)&&resolve_memory(runtime,g->addr[i],g->len[i],0)!=g->ptr[i])return 0;
  uint8_t digest[32];file1_digest(g->ptr[i],g->len[i],digest);if(memcmp(digest,g->digest[i],32))return 0;
 }
 if(resolve_memory(runtime,g->stack_addr,232,0)!=g->stack||resolve_memory(runtime,g->stack_addr,232,1)!=g->stack ||
    resolve_memory(runtime,g->state_addr,4,0)!=g->a||resolve_memory(runtime,g->state_addr,4,1)!=g->a ||
    resolve_memory(runtime,g->state_addr+4u,4,0)!=g->b||resolve_memory(runtime,g->state_addr+4u,4,1)!=g->b )return 0;
 const unsigned off[]={4,8,12,16,20,24,28,32,36,40,96,94,88,87,80,72,64,56,48,132,136,120,128,156,160,164,168,172,176,180,184,188,192,196,140,216,208,200};
 const unsigned widths[]={4,4,4,4,4,4,4,4,4,4,2,2,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,2,4};
 for(unsigned i=0;i<sizeof(off)/sizeof(off[0]);i++){uint32_t a=g->stack_addr+232u-off[i];uint8_t *p=g->stack+232u-off[i];if(resolve_memory(runtime,a,widths[i],0)!=p||resolve_memory(runtime,a,widths[i],1)!=p)return 0;}
 unsigned packet_count=g->packet_count;
 for(unsigned n=0;n<packet_count;n++){
  uint32_t address=g->packet_addr+n*80u;uint8_t *packet=g->packet[n];
  if(resolve_memory(runtime,address,40,0)!=packet||resolve_memory(runtime,address,40,1)!=packet)return 0;
  for(unsigned i=0;i<40;i++)if(resolve_memory(runtime,address+i,1,0)!=packet+i||resolve_memory(runtime,address+i,1,1)!=packet+i)return 0;
  for(unsigned i=0;i<40;i+=2)if(resolve_memory(runtime,address+i,2,0)!=packet+i||resolve_memory(runtime,address+i,2,1)!=packet+i)return 0;
 }
 if(g->dynamic_map_count){for(unsigned i=0;i<g->dynamic_map_count;i++){uint8_t *ptr=g->dynamic_output[i];if(resolve_memory(runtime,g->dynamic_addr[i],1,0)!=ptr||resolve_memory(runtime,g->dynamic_addr[i],1,1)!=ptr)return 0;unsigned expected=g->dynamic_initial[i];for(unsigned j=0;j<g->dynamic_map_count;j++)if(g->dynamic_output[j]==ptr&&g->dynamic_written[j])expected=g->dynamic_value[j];if(*ptr!=expected)return 0;}}else if(g->allow_reused_mapping){uint8_t *ptrs[4]={g->map_output,g->map_output2,g->map_output3,g->map_output4};uint32_t addrs[4]={g->map_addr,g->map_addr2,g->map_addr3,g->map_addr4},values[4]={g->map_value,g->map_value2,g->map_value3,g->map_value4};unsigned written[4]={g->map_written,g->map_written2,g->map_written3,g->map_written4},initial[4]={g->map_initial,g->map_initial2,g->map_initial3,g->map_initial4};for(unsigned i=0;i<4;i++)if(ptrs[i]){if(resolve_memory(runtime,addrs[i],1,0)!=ptrs[i]||resolve_memory(runtime,addrs[i],1,1)!=ptrs[i])return 0;unsigned expected=initial[i];for(unsigned j=0;j<4;j++)if(ptrs[j]==ptrs[i]&&written[j])expected=values[j];if(*ptrs[i]!=expected)return 0;}}else{
 if(g->map_output&&(resolve_memory(runtime,g->map_addr,1,0)!=g->map_output||resolve_memory(runtime,g->map_addr,1,1)!=g->map_output||*g->map_output!=(g->map_written?g->map_value:g->map_initial)))return 0;
 if(g->map_output2&&(resolve_memory(runtime,g->map_addr2,1,0)!=g->map_output2||resolve_memory(runtime,g->map_addr2,1,1)!=g->map_output2||*g->map_output2!=(g->map_written2?g->map_value2:g->map_initial2)))return 0;
 if(g->map_output3&&(resolve_memory(runtime,g->map_addr3,1,0)!=g->map_output3||resolve_memory(runtime,g->map_addr3,1,1)!=g->map_output3||*g->map_output3!=(g->map_written3?g->map_value3:g->map_initial3)))return 0;
 if(g->map_output4&&(resolve_memory(runtime,g->map_addr4,1,0)!=g->map_output4||resolve_memory(runtime,g->map_addr4,1,1)!=g->map_output4||*g->map_output4!=(g->map_written4?g->map_value4:g->map_initial4)))return 0;
 }
 uint32_t frame=g->stack_addr+120u,ra=cpu->gpr[31];uint8_t *pf=g->stack+120u;unsigned goff=g->current*8u;uint32_t expected_glyph=g->current>=7?g->extra_resolved[g->current-7]:g->current==6?g->seventh_resolved:g->current==5?g->sixth_resolved:g->current==4?g->fifth_resolved:g->current==3?g->fourth_resolved:g->current==2?g->third_resolved:g->current?g->second_resolved:g->resolved_glyph;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};
 for(unsigned i=0;i<10;i++)if(load_le(pf+108u-i*4u,4)!=g->saved[i])return 0;
 if(load_le(pf+16u,2)!=g->ptr[6][0]||load_le(pf+18u,2)!=255u||pf[24]!=g->ptr[6][2]||pf[25]!=g->ptr[6][3]||
    load_le(pf+32u,4)!=((g->arg0&255u)<<6)||load_le(pf+40u,4)!=frame+16u||load_le(pf+48u,4)!=frame+24u||
    load_le(pf+56u,4)!=frame+24u||load_le(pf+64u,4)!=frame+16u)return 0;
 unsigned group=g->ptr[6][0],buffer=load_le(g->ptr[12],4),prefix=0;for(unsigned i=0;i<g->current;i++)prefix+=g->glyph_counts[i];int style=(ra==0x8008d8c4u||ra==0x8008d90cu);
 if(load_le(g->a,4)!=(group==0?prefix+(style?g->descriptor_count:0u):0u)||load_le(g->b,4)!=(group==1?prefix+(style?g->descriptor_count:0u):0u))return 0;
 if(ra==0x8008d7b0u||ra==0x80076a5cu){
  if(cpu->gpr[16]!=g->current*8u||cpu->gpr[17]!=prefix*2u||cpu->gpr[20]!=g->current*4u||cpu->gpr[22]!=frame+16u||cpu->gpr[23]!=0||cpu->gpr[30]!=frame+24u)return 0;
 }
 if(ra==0x8008d7b0u){
  if(target!=0x80076a10u||cpu->gpr[29]!=frame||cpu->gpr[4]!=expected_glyph||cpu->gpr[5]!=g->packet_addr-buffer*40u+prefix*80u||
     cpu->gpr[6]!=(uint32_t)(int32_t)(int16_t)load_le(g->ptr[8]+goff+2,2)||cpu->gpr[7]!=(uint32_t)(int32_t)(int16_t)load_le(g->ptr[8]+goff+4,2))return 0;
 }else if(ra==0x80076a5cu || (ra>=0x8002686cu&&ra<=0x800268acu)){
  uint8_t *wf=g->stack+80u;
  if(load_le(wf+16u,4)!=(uint32_t)(int32_t)(int16_t)load_le(g->ptr[8]+goff+2,2)||load_le(wf+20u,4)!=(uint32_t)(int32_t)(int16_t)load_le(g->ptr[8]+goff+4,2)||
     load_le(wf+24u,4)!=0x1000u||load_le(wf+32u,4)!=0x8008d7b0u)return 0;
  if(ra==0x80076a5cu){for(unsigned i=1;i<10;i++)if(cpu->gpr[regs[i]]!=g->child_saved[i])return 0;if(target!=0x8002675cu||cpu->gpr[29]!=g->stack_addr+80u||cpu->gpr[4]!=load_le(g->ptr[9],4)||cpu->gpr[5]!=expected_glyph||cpu->gpr[6]!=g->packet_addr-buffer*40u+prefix*80u||cpu->gpr[7]!=buffer)return 0;}
  else{
   if(cpu->gpr[29]!=g->stack_addr||load_le(g->stack+32u,4)!=(g->current>=7?g->extra_desc[g->current-7]:g->current==6?g->addr[47]:g->current==5?g->addr[44]:g->current==4?g->addr[41]:g->current==3?g->addr[37]:g->current==2?g->addr[30]:g->current?g->addr[28]:g->addr[11])||load_le(g->stack+36u,4)>=g->descriptor_count||load_le(g->stack+92u,4)!=buffer||
      load_le(g->stack+16u,2)!=load_le(g->ptr[8]+goff+2,2)||load_le(g->stack+24u,2)!=load_le(g->ptr[8]+goff+4,2))return 0;
   for(unsigned i=0;i<10;i++)if(load_le(g->stack+76u-i*4u,4)!=g->child_saved[i])return 0;
   unsigned completed=load_le(g->stack+36u,4);uint32_t current_packet=g->packet_addr+(prefix+completed)*80u;
   if(cpu->gpr[16]!=current_packet||cpu->gpr[17]!=(g->current>=7?g->extra_desc[g->current-7]:g->current==6?g->addr[47]:g->current==5?g->addr[44]:g->current==4?g->addr[41]:g->current==3?g->addr[37]:g->current==2?g->addr[30]:g->current?g->addr[28]:g->addr[11])+4u+completed*28u||cpu->gpr[20]!=0x1000u)return 0;
   if((target==0x80043cb0u||target==0x80043bfcu||target==0x80043c24u)&&cpu->gpr[4]!=current_packet)return 0;
  }
 }else if(style){
  if(cpu->gpr[29]!=frame||cpu->gpr[16]!=frame+16u||(cpu->gpr[17]&1u)||cpu->gpr[17]<prefix*2u||cpu->gpr[17]>=(prefix+g->descriptor_count)*2u||cpu->gpr[18]!=g->shade||cpu->gpr[19]!=frame+16u||cpu->gpr[20]!=g->current*4u||cpu->gpr[21]!=0x800ccb34u||cpu->gpr[22]!=frame+16u||cpu->gpr[23]!=0||cpu->gpr[30]!=frame+24u||cpu->gpr[4]!=g->packet_addr+cpu->gpr[17]*40u||cpu->gpr[5]!=(ra==0x8008d8c4u?1u:0u))return 0;
 }else return 0;
 return 1;
}

/* Private native dispatch for this preflighted single-glyph graph. */
static int one_glyph_context_bridge(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 if(!one_glyph_active||one_glyph_active->runtime!=runtime)return BRIDGE_HOOKS_NONE;
 uint32_t ra=cpu->gpr[31];
 if(ra!=0x8008d7b0u && ra!=0x80076a5cu && ra!=0x8002686cu && ra!=0x80026878u && ra!=0x80026884u && ra!=0x80026898u && ra!=0x800268acu && ra!=0x8008d8c4u && ra!=0x8008d90cu)return BRIDGE_HOOKS_NONE;
 if(!one_glyph_guard_current(one_glyph_active,cpu,target))return -1;
 BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure))return -1;
 if(target==0x80076a10u && ra==0x8008d7b0u){const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};one_glyph_active->child_saved[0]=0x80076a5cu;for(unsigned i=1;i<10;i++)one_glyph_active->child_saved[i]=cpu->gpr[regs[i]];if(one_glyph_active->descriptor_count==0)zero_glyph_wrapper(&e);else battle_glyph_wrapper_run(&e,target);return 1;}
 if(target==0x8002675cu && ra==0x80076a5cu){if(one_glyph_active->descriptor_count==0)zero_glyph_renderer(&e);else battle_glyph_render_run(&e);return 1;}
 if(one_glyph_active->descriptor_count==0)return -1;
 if((target==0x80043cb0u && ra==0x8002686cu)||(target==0x80043bfcu && ra==0x80026878u)||(target==0x80043c24u && ra==0x80026884u)||(target==0x80043a1cu && ra==0x80026898u)||(target==0x80043a58u && ra==0x800268acu)||(target==0x80043bfcu && ra==0x8008d8c4u)||(target==0x80043c24u && ra==0x8008d90cu)){battle_atlas_gpu_run(&e,target);return 1;}
 return -1;
}

/* Bounded parent continuation for up to seven primitives. */
static void glyph_continue_one_primitive(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;
 r[3]=battle_entry_read(e,r[22],2);r[4]=battle_entry_read(e,0x800d2d28u,4);r[3]=(r[3]<<2)+r[4];r[4]=battle_entry_read(e,r[3]+0xd0u,4);r[2]+=r[4];battle_entry_write(e,r[3]+0xd0u,4,r[2]);
 r[2]=battle_entry_read(e,r[30],1);r[9]=0x800c2f4cu;r[2]=battle_entry_read(e,r[9]+r[2]*4u,4);r[16]+=r[2];r[4]=battle_entry_read(e,r[16]+6u,2);r[2]=battle_entry_read(e,r[16]+6u,2);r[3]=r[4]&255u;r[2]=r[2]<0x2000u;r[18]=r[3]>>1;
 if(!r[2]){r[3]=r[4]&15u;r[2]=battle_entry_read(e,0x800c3eacu,4);r[9]=battle_entry_read(e,r[29]+32u,4);r[3]<<=1;r[2]+=r[9];r[3]+=r[2];r[2]=battle_entry_read(e,r[3]+0x1cu,2);r[18]=16u;if(!r[2]){r[2]=r[4]&0xf0u;r[18]=r[2]>>1;}}
 if(r[18]){
 r[2]=battle_entry_read(e,r[22],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[19]=battle_entry_read(e,r[29]+40u,4);r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);r[16]=r[23]+r[19];
 if(!r[2])battle_entry_fail(e,"no primitives returned",0x8008d868u);
 r[21]=0x800ccb34u;unsigned iterations=0;do{if(iterations++>=7)battle_entry_fail(e,"primitive limit exceeded",0x8008d87cu);r[5]=1;r[3]=battle_entry_read(e,r[16],2);r[4]=battle_entry_read(e,0x800c3ea4u,4);r[2]=r[3]*0xfa0u+0x641cu;r[3]=battle_entry_read(e,r[21],4);r[2]+=r[4];r[3]+=r[17];r[4]=r[2]+r[3]*40u;r[31]=0x8008d8c4u;
 battle_phase_call(e,0x80043bfcu,0x8008d8bcu);
 r[5]=0;r[3]=battle_entry_read(e,r[16],2);r[4]=battle_entry_read(e,0x800c3ea4u,4);r[2]=r[3]*0xfa0u+0x641cu;r[3]=battle_entry_read(e,r[21],4);r[2]+=r[4];r[3]+=r[17];r[4]=r[2]+r[3]*40u;r[31]=0x8008d90cu;battle_phase_call(e,0x80043c24u,0x8008d904u);
 for(unsigned color=0;color<3;color++){
  r[2]=battle_entry_read(e,0x800ccb34u,4);r[4]=battle_entry_read(e,r[16],2);r[2]+=r[17];r[3]=r[2]*40u;r[2]=r[4]*0xfa0u;r[4]=battle_entry_read(e,0x800c3ea4u,4);r[2]+=r[4];r[3]+=r[2];battle_entry_write(e,r[3]+0x6420u+color,1,r[18]);
 }
 r[2]=battle_entry_read(e,r[21],4);r[4]=battle_entry_read(e,r[16],2);r[2]+=r[17];r[3]=r[2]*40u;r[2]=r[4]*0xfa0u;r[4]=battle_entry_read(e,0x800c3ea4u,4);r[3]+=r[2];r[4]+=r[3];r[2]=battle_entry_read(e,r[4]+0x6432u,2)|0x20u;battle_entry_write(e,r[4]+0x6432u,2,r[2]);
 r[2]=battle_entry_read(e,r[16],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[17]+=2;r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);r[16]=r[23]+r[19];}while(r[2]);
 r[20]+=4;
 }else{
  r[2]=battle_entry_read(e,r[22],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[6]=battle_entry_read(e,r[29]+40u,4);r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);if(!r[2])battle_entry_fail(e,"no primitives returned",0x8008da7cu);
  r[7]=0x80u;r[5]=r[23]+r[6];unsigned iterations=0;do{if(iterations++>=7)battle_entry_fail(e,"primitive limit exceeded",0x8008da88u);for(unsigned color=0;color<3;color++){
   r[2]=battle_entry_read(e,0x800ccb34u,4);r[4]=battle_entry_read(e,r[5],2);r[2]+=r[17];r[3]=r[2]*40u;r[2]=r[4]*0xfa0u;r[4]=battle_entry_read(e,0x800c3ea4u,4);r[2]+=r[4];r[3]+=r[2];battle_entry_write(e,r[3]+0x6420u+color,1,r[7]);
  }
  r[2]=battle_entry_read(e,r[5],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[17]+=2;r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);}while(r[2]);r[20]+=4;
 }
 r[2]=battle_entry_read(e,r[30],1);r[9]=0x800c2f4cu;r[2]=(r[2]<<2)+r[9];r[3]=battle_entry_read(e,r[2],4);r[2]=(r[20]<<1)+r[3];r[4]=battle_entry_read(e,r[2],2);r[2]=(int32_t)r[4]<0x4000;if(r[4]!=0xffffu)battle_entry_fail(e,"another glyph unsupported",0x8008dbb0u);
 r[9]=battle_entry_read(e,r[29]+56u,4)+1u;battle_entry_write(e,r[29]+56u,4,r[9]);r[9]=battle_entry_read(e,r[29]+64u,4)+2u;battle_entry_write(e,r[29]+64u,4,r[9]);r[9]=battle_entry_read(e,r[29]+48u,4);r[2]=r[9]+2u;r[9]=battle_entry_read(e,r[29]+56u,4);r[2]=(int32_t)r[9]<(int32_t)r[2];r[23]+=2u;if(!r[2])battle_entry_fail(e,"missing second sentinel",0x8008dbf0u);
 r[9]=battle_entry_read(e,r[29]+64u,4);r[3]=battle_entry_read(e,r[9],2);r[2]=255u;if(r[3]!=255u)battle_entry_fail(e,"second group unsupported",0x8008d69cu);
 r[2]=battle_entry_read(e,0x800ccb34u,4);const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)r[regs[i]]=battle_entry_read(e,r[29]+108u-i*4u,4);r[29]+=112u;
}

static int one_glyph_preflight_capture(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target,OneGlyphGuard *guard,unsigned descriptor_count)
{
 if(target!=0x8008d598u||cpu->gpr[31]!=0x80080758u||(cpu->gpr[6]&255u)||runtime->force_interpret||!runtime->initialized||g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg))return 0;
 const char *control=getenv(descriptor_count==22?"XENO_BATTLE_NATIVE_MANY_GLYPHS_EXPERIMENT":descriptor_count==21?"XENO_BATTLE_NATIVE_SEVEN_GLYPHS_EXPERIMENT":descriptor_count==20?"XENO_BATTLE_NATIVE_SIX_GLYPHS_EXPERIMENT":descriptor_count==19?"XENO_BATTLE_NATIVE_FIVE_GLYPHS_EXPERIMENT":descriptor_count==18?"XENO_BATTLE_NATIVE_FOUR_MAPPED_EXPERIMENT":descriptor_count==17?"XENO_BATTLE_NATIVE_FOUR_ONE_MAPPED_EXPERIMENT":descriptor_count==16?"XENO_BATTLE_NATIVE_FOUR_GLYPHS_EXPERIMENT":descriptor_count==15?"XENO_BATTLE_NATIVE_THREE_MAPPINGS_EXPERIMENT":descriptor_count==14?"XENO_BATTLE_NATIVE_THREE_SECOND_THIRD_MAPPED_EXPERIMENT":descriptor_count==12?"XENO_BATTLE_NATIVE_THREE_FIRST_SECOND_MAPPED_EXPERIMENT":descriptor_count==13?"XENO_BATTLE_NATIVE_THREE_FIRST_THIRD_MAPPED_EXPERIMENT":descriptor_count==11?"XENO_BATTLE_NATIVE_TWO_MAPPINGS_EXPERIMENT":descriptor_count==10?"XENO_BATTLE_NATIVE_THIRD_MAPPED_GLYPH_EXPERIMENT":descriptor_count==9?"XENO_BATTLE_NATIVE_SECOND_MAPPED_THREE_GLYPHS_EXPERIMENT":descriptor_count>=8?"XENO_BATTLE_NATIVE_SECOND_MAPPED_GLYPH_EXPERIMENT":descriptor_count==7?"XENO_BATTLE_NATIVE_THREE_MAPPED_GLYPHS_EXPERIMENT":descriptor_count>=6?"XENO_BATTLE_NATIVE_TWO_MAPPED_GLYPHS_EXPERIMENT":(descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)?"XENO_BATTLE_NATIVE_THREE_GLYPHS_EXPERIMENT":descriptor_count==4?"XENO_BATTLE_NATIVE_TWO_GLYPHS_EXPERIMENT":descriptor_count==3?"XENO_BATTLE_NATIVE_TWO_ZERO_GLYPHS_EXPERIMENT":descriptor_count==2?"XENO_BATTLE_NATIVE_MAPPED_GLYPH_EXPERIMENT":descriptor_count?"XENO_BATTLE_NATIVE_ONE_GLYPH_EXPERIMENT":"XENO_BATTLE_NATIVE_ZERO_GLYPH_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+232u||slot>1)return 0;
 const uint8_t *p[156];size_t len[156]={0x69c,92,688,4,4,4,4,4,10,4,2,32,4,688,20,40,40,60,24,20,40,40,60,24,4,2,1,2,32,2,32,4,4,1,1,1,2,32,4,4,2,32,4,2,32,4,2,32,4};unsigned protected_count=24;uint32_t shade,second_glyph=0,second_resolved=0,map_raw=0,map_value=0,second_index=0,second_desc=0,third_glyph=0,third_resolved=0,third_index=0,third_desc=0,fourth_glyph=0,fourth_resolved=0,fourth_index=0,fourth_desc=0,fifth_glyph=0,fifth_resolved=0,fifth_index=0,fifth_desc=0,sixth_glyph=0,sixth_resolved=0,sixth_index=0,sixth_desc=0,seventh_glyph=0,seventh_resolved=0,seventh_index=0,seventh_desc=0,mapping_pos=7;uint32_t fade_addr=0,second_fade_addr=0,third_fade_addr=0,fourth_fade_addr=0,fifth_fade_addr=0,sixth_fade_addr=0,seventh_fade_addr=0,second_shade=0,third_shade=0,fourth_shade=0,fifth_shade=0,sixth_shade=0,seventh_shade=0,map_lookup=0,map_addr=0,map_lookup2=0,map_addr2=0,map_value2=0,map_lookup3=0,map_addr3=0,map_value3=0,map_lookup4=0,map_addr4=0,map_value4=0;uint8_t *map_output=NULL,*map_output2=NULL,*map_output3=NULL,*map_output4=NULL;unsigned map_sequence[32]={0},stream_count=0,dynamic_count=0;uint32_t dynamic_addr[32]={0},dynamic_lookup[32]={0},dynamic_value[32]={0};uint8_t *dynamic_output[32]={0};uint32_t extra_raw[25]={0},extra_resolved[25]={0},extra_index[25]={0},extra_desc[25]={0},extra_fade[25]={0},extra_shade[25]={0};unsigned extra_count[25]={0};
 p[0]=resolve_memory(runtime,target,0x69c,0);p[1]=resolve_memory(runtime,0x80076a10u,92,0);
 const XenoRetailFileSlot *image=XenoRetailData_File(XENO_RD_SLUS);size_t off=XENO_RD_SLUS_OFF(0x8002675cu);
 if(!p[0]||!p[1]||!image||!image->bytes||image->size<off+688u)return 0;p[2]=image->bytes+off;
 const char *hash[]={"9a6f5146f9ff7abf044add18fddda4ef4878a2b2e1b0f0054a1e31706a043c61","22075ecff68520daca6dde56b3fb491d01dded56e381ba8b77b42b1950eb5e92","400dc2a71aa7726c0fa7002e4a220faedbfae0107ad138fb4d1e192e22f2f1dd"};
 for(unsigned i=0;i<3;i++){uint8_t digest[32];file1_digest(p[i],len[i],digest);for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=hash[i][j*2]||file1_hex_digit(digest[j]&15)!=hash[i][j*2+1])return 0;}
 p[3]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[4]=resolve_memory(runtime,0x800d2d28u,4,0);p[5]=resolve_memory(runtime,0x800c3ea4u,4,0);p[9]=resolve_memory(runtime,0x800d2f5cu,4,0);p[12]=resolve_memory(runtime,0x800ccb34u,4,0);p[13]=resolve_memory(runtime,0x8002675cu,688,0);
 if(!p[3]||!p[4]||!p[5]||!p[9]||!p[12]||!p[13])return 0;
 uint32_t src=load_le(p[3],4),state=load_le(p[4],4),base=load_le(p[5],4),table=load_le(p[9],4);if(src>UINT32_MAX-3u||(state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[6]=resolve_memory(runtime,src,4,0);if(!p[6]||p[6][0]>1||p[6][1]!=255||p[6][2]>7)return 0;
 p[7]=resolve_memory(runtime,0x800c2f4cu+p[6][2]*4u,4,0);if(!p[7])return 0;uint32_t glyph=load_le(p[7],4);if((glyph&1u)||glyph>UINT32_MAX-9u)return 0;
 p[8]=resolve_memory(runtime,glyph,10,0);if(!p[8])return 0;uint32_t raw_id=load_le(p[8],2),id=raw_id,style=load_le(p[8]+6,2);
 if(descriptor_count==22){unsigned n=0;for(;n<=32;n++){uint64_t at=(uint64_t)glyph+n*8u;if(at>UINT32_MAX-1u)return 0;const uint8_t *q=resolve_memory(runtime,(uint32_t)at,2,0);if(!q)return 0;uint32_t ch=load_le(q,2);if(ch==0xffffu)break;if(n==32)return 0;}if(n<7||n>32)return 0;stream_count=n;size_t span=n*8u+2u;if((uint64_t)glyph+span-1u>UINT32_MAX)return 0;p[8]=resolve_memory(runtime,glyph,span,0);len[8]=span;if(!p[8])return 0;for(unsigned i=0;i<span;i+=2u)if(resolve_memory(runtime,glyph+i,2,0)!=p[8]+i)return 0;for(unsigned i=0;i<n;i++){uint32_t ch=load_le(p[8]+i*8u,2);if(ch==0xffffu||load_le(p[8]+i*8u+6u,2)>=0x4000u)return 0;if(i>=7)extra_raw[i-7]=extra_resolved[i-7]=ch;}if(load_le(p[8]+n*8u,2)!=0xffffu)return 0;second_glyph=second_resolved=load_le(p[8]+8,2);third_glyph=third_resolved=load_le(p[8]+16,2);fourth_glyph=fourth_resolved=load_le(p[8]+24,2);fifth_glyph=fifth_resolved=load_le(p[8]+32,2);sixth_glyph=sixth_resolved=load_le(p[8]+40,2);seventh_glyph=seventh_resolved=load_le(p[8]+48,2);unsigned mapped=0;for(unsigned i=0;i<n;i++)if(load_le(p[8]+i*8u,2)>=0x4000u)mapped++;}else if(descriptor_count>=3){unsigned stream_len=descriptor_count==21?58u:descriptor_count==20?50u:descriptor_count==19?42u:descriptor_count>=16?34u:(descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)?26u:18u;if(glyph>UINT32_MAX-(stream_len-1u))return 0;p[8]=resolve_memory(runtime,glyph,stream_len,0);len[8]=stream_len;if(!p[8])return 0;second_glyph=load_le(p[8]+8,2);if(((descriptor_count>=17?raw_id==0xffffu:(descriptor_count==6||descriptor_count==7||descriptor_count==11||descriptor_count==12||descriptor_count==13||descriptor_count==15)?(raw_id<0x4000u||raw_id==0xffffu):raw_id>=0x4000u)||(descriptor_count>=17?second_glyph==0xffffu:(descriptor_count==8||descriptor_count==9||descriptor_count==11||descriptor_count==12||descriptor_count==14||descriptor_count==15)?(second_glyph<0x4000u||second_glyph==0xffffu):second_glyph>=0x4000u)||(descriptor_count>=4?(style>=0x4000u||load_le(p[8]+14,2)>=0x4000u):(style>=0x2000u||load_le(p[8]+14,2)>=0x2000u))||((descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)?((descriptor_count>=17?load_le(p[8]+16,2)==0xffffu:(descriptor_count==10||descriptor_count==13||descriptor_count==14||descriptor_count==15)?(load_le(p[8]+16,2)<0x4000u||load_le(p[8]+16,2)==0xffffu):load_le(p[8]+16,2)>=0x4000u)||load_le(p[8]+22,2)>=0x4000u||(descriptor_count>=16?((descriptor_count==16&&load_le(p[8]+24,2)>=0x4000u)||load_le(p[8]+24,2)==0xffffu||load_le(p[8]+30,2)>=0x4000u||(descriptor_count>=19?(load_le(p[8]+32,2)==0xffffu||load_le(p[8]+38,2)>=0x4000u||(descriptor_count>=20?(load_le(p[8]+40,2)==0xffffu||load_le(p[8]+46,2)>=0x4000u||(descriptor_count==21?(load_le(p[8]+48,2)==0xffffu||load_le(p[8]+54,2)>=0x4000u||load_le(p[8]+56,2)!=0xffffu):load_le(p[8]+48,2)!=0xffffu)):load_le(p[8]+40,2)!=0xffffu)):load_le(p[8]+32,2)!=0xffffu)):load_le(p[8]+24,2)!=0xffffu)):load_le(p[8]+16,2)!=0xffffu)))return 0;if(descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)third_glyph=load_le(p[8]+16,2);if(descriptor_count>=16)fourth_glyph=load_le(p[8]+24,2);if(descriptor_count>=19)fifth_glyph=load_le(p[8]+32,2);if(descriptor_count>=20)sixth_glyph=load_le(p[8]+40,2);if(descriptor_count==21)seventh_glyph=load_le(p[8]+48,2);seventh_resolved=seventh_glyph;sixth_resolved=sixth_glyph;fifth_resolved=fifth_glyph;fourth_resolved=fourth_glyph;third_resolved=third_glyph;second_resolved=second_glyph;if(descriptor_count>=17){uint32_t raw[7]={raw_id,second_glyph,third_glyph,fourth_glyph,fifth_glyph,sixth_glyph,seventh_glyph};unsigned mapped=0;for(unsigned i=0;i<(descriptor_count==22?stream_count:descriptor_count>=21?7u:descriptor_count==20?6u:descriptor_count==19?5u:4u);i++)if(raw[i]>=0x4000u){mapping_pos=i;mapped++;}if(descriptor_count==17?mapped!=1:descriptor_count==18?(mapped==0||mapped>4):mapped>4)return 0;}}
 else if(style>=0x4000u||load_le(p[8]+8,2)!=0xffffu)return 0;
 if(descriptor_count==2||(descriptor_count>=6&&(descriptor_count<=15||descriptor_count==17))){
  unsigned bi=descriptor_count>=6?32u:24u,li=descriptor_count>=6?33u:26u;map_raw=descriptor_count==17?(mapping_pos==0?raw_id:mapping_pos==1?second_glyph:mapping_pos==2?third_glyph:fourth_glyph):descriptor_count==10?third_glyph:(descriptor_count==8||descriptor_count==9||descriptor_count==14)?second_glyph:raw_id;if(map_raw<0x4000u||map_raw==0xffffu)return 0;p[bi]=resolve_memory(runtime,0x800c3eacu,4,0);if(!p[bi])return 0;
  uint64_t base=load_le(p[bi],4),lookup=base+((cpu->gpr[4]&255u)<<6)+(map_raw&255u),output=base+0x2c0u+(map_raw&3u);if(lookup>UINT32_MAX||output>UINT32_MAX)return 0;
  map_lookup=(uint32_t)lookup;map_addr=(uint32_t)output;p[li]=resolve_memory(runtime,map_lookup,1,0);map_output=resolve_memory(runtime,map_addr,1,1);if(!p[li]||!map_output||resolve_memory(runtime,map_addr,1,0)!=map_output||*p[li]==255u)return 0;map_value=*p[li];if(descriptor_count==17){if(mapping_pos==0)id=map_value;else if(mapping_pos==1)second_resolved=map_value;else if(mapping_pos==2)third_resolved=map_value;else fourth_resolved=map_value;}else if(descriptor_count==10)third_resolved=map_value;else if(descriptor_count==8||descriptor_count==9||descriptor_count==14)second_resolved=map_value;else id=map_value;
  if(descriptor_count==2){p[25]=p[24];len[25]=4;fade_addr=0x800c3eacu;protected_count=27;}
 }else if(descriptor_count!=18&&descriptor_count!=19&&descriptor_count!=20&&descriptor_count!=21&&descriptor_count!=22&&id>=0x4000u)return 0;
 if(descriptor_count>=11&&descriptor_count<=15){uint32_t raw2=(descriptor_count==13||descriptor_count==14)?third_glyph:second_glyph;uint64_t base=load_le(p[32],4),lookup=base+((cpu->gpr[4]&255u)<<6)+(raw2&255u),output=base+0x2c0u+(raw2&3u);if(lookup>UINT32_MAX||output>UINT32_MAX)return 0;map_lookup2=(uint32_t)lookup;map_addr2=(uint32_t)output;p[34]=resolve_memory(runtime,map_lookup2,1,0);map_output2=resolve_memory(runtime,map_addr2,1,1);if(!p[34]||!map_output2||resolve_memory(runtime,map_addr2,1,0)!=map_output2||*p[34]==255u)return 0;map_value2=*p[34];if(descriptor_count==13||descriptor_count==14)third_resolved=map_value2;else second_resolved=map_value2;}
 if(descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21){p[32]=resolve_memory(runtime,0x800c3eacu,4,0);if(!p[32])return 0;uint32_t raw[32]={raw_id,second_glyph,third_glyph,fourth_glyph,fifth_glyph,sixth_glyph,seventh_glyph};for(unsigned i=7;i<stream_count;i++)raw[i]=extra_raw[i-7];unsigned slots[4]={33,34,35,39};uint32_t *addresses[4]={&map_addr,&map_addr2,&map_addr3,&map_addr4},*lookups[4]={&map_lookup,&map_lookup2,&map_lookup3,&map_lookup4},*values[4]={&map_value,&map_value2,&map_value3,&map_value4};uint8_t **outputs[4]={&map_output,&map_output2,&map_output3,&map_output4};for(unsigned i=0;i<4;i++){p[slots[i]]=p[9];len[slots[i]]=4;}unsigned n=0;for(unsigned i=0;i<(descriptor_count==22?stream_count:descriptor_count>=21?7u:descriptor_count==20?6u:descriptor_count==19?5u:4u);i++)if(raw[i]>=0x4000u){uint64_t base=load_le(p[32],4),lookup=base+((cpu->gpr[4]&255u)<<6)+(raw[i]&255u),output=base+0x2c0u+(raw[i]&3u);if(lookup>UINT32_MAX||output>UINT32_MAX)return 0;*lookups[n]=(uint32_t)lookup;*addresses[n]=(uint32_t)output;p[slots[n]]=resolve_memory(runtime,*lookups[n],1,0);len[slots[n]]=1;*outputs[n]=resolve_memory(runtime,*addresses[n],1,1);if(!p[slots[n]]||!*outputs[n]||resolve_memory(runtime,*addresses[n],1,0)!=*outputs[n]||*p[slots[n]]==255u)return 0;*values[n]=*p[slots[n]];map_sequence[i]=n+1u;if(i==0)id=*values[n];else if(i==1)second_resolved=*values[n];else if(i==2)third_resolved=*values[n];else if(i==3)fourth_resolved=*values[n];else if(i==4)fifth_resolved=*values[n];else if(i==5)sixth_resolved=*values[n];else if(i==6)seventh_resolved=*values[n];else extra_resolved[i-7]=*values[n];n++;}}
 if(descriptor_count==22){p[32]=resolve_memory(runtime,0x800c3eacu,4,0);if(!p[32])return 0;p[33]=p[34]=p[35]=p[39]=p[9];len[33]=len[34]=len[35]=len[39]=4;for(unsigned i=0;i<stream_count;i++){uint32_t raw=load_le(p[8]+i*8u,2);if(raw<0x4000u)continue;uint64_t base=load_le(p[32],4),lookup=base+((cpu->gpr[4]&255u)<<6)+(raw&255u),output=base+0x2c0u+(raw&3u);if(lookup>UINT32_MAX||output>UINT32_MAX)return 0;unsigned slot=124u+dynamic_count;dynamic_lookup[dynamic_count]=(uint32_t)lookup;dynamic_addr[dynamic_count]=(uint32_t)output;p[slot]=resolve_memory(runtime,(uint32_t)lookup,1,0);len[slot]=1;dynamic_output[dynamic_count]=resolve_memory(runtime,(uint32_t)output,1,1);if(!p[slot]||!dynamic_output[dynamic_count]||resolve_memory(runtime,(uint32_t)output,1,0)!=dynamic_output[dynamic_count]||*p[slot]==255u)return 0;uint32_t value=*p[slot];dynamic_value[dynamic_count]=value;map_sequence[i]=++dynamic_count;if(i==0)id=value;else if(i==1)second_resolved=value;else if(i==2)third_resolved=value;else if(i==3)fourth_resolved=value;else if(i==4)fifth_resolved=value;else if(i==5)sixth_resolved=value;else if(i==6)seventh_resolved=value;else extra_resolved[i-7]=value;}}
 if(descriptor_count==15){uint64_t base=load_le(p[32],4),lookup=base+((cpu->gpr[4]&255u)<<6)+(third_glyph&255u),output=base+0x2c0u+(third_glyph&3u);if(lookup>UINT32_MAX||output>UINT32_MAX)return 0;map_lookup3=(uint32_t)lookup;map_addr3=(uint32_t)output;p[35]=resolve_memory(runtime,map_lookup3,1,0);map_output3=resolve_memory(runtime,map_addr3,1,1);if(!p[35]||!map_output3||resolve_memory(runtime,map_addr3,1,0)!=map_output3||*p[35]==255u)return 0;map_value3=*p[35];third_resolved=map_value3;}
 shade=(style&255u)>>1;if(style>=0x2000u){
  p[24]=resolve_memory(runtime,0x800c3eacu,4,0);if(!p[24])return 0;uint64_t fade=(uint64_t)load_le(p[24],4)+((cpu->gpr[4]&255u)<<6)+0x1cu+((style&15u)<<1);
  if(fade>UINT32_MAX-1u||(fade&1u))return 0;fade_addr=(uint32_t)fade;p[25]=resolve_memory(runtime,fade_addr,2,0);len[25]=2;if(!p[25])return 0;protected_count=descriptor_count==2?27:26;shade=load_le(p[25],2)?16u:((style&0xf0u)>>1);
 }
 uint64_t index=(uint64_t)table+id*2u+4u;if(index>UINT32_MAX-1u || (index&1u))return 0;p[10]=resolve_memory(runtime,(uint32_t)index,2,0);if(!p[10])return 0;
 uint64_t desc=(uint64_t)table+load_le(p[10],2);if(desc>UINT32_MAX-31u||(desc&1u))return 0;p[11]=resolve_memory(runtime,(uint32_t)desc,32,0);if(!p[11])return 0;
 unsigned actual_count=load_le(p[11],2);if(descriptor_count==2?(actual_count>7):descriptor_count==1?(actual_count<1||actual_count>7):descriptor_count==22?(actual_count>50):descriptor_count>=4?(actual_count>7):(actual_count!=0))return 0;
 if(actual_count>1){len[11]=4u+actual_count*28u;if(desc>UINT32_MAX-(len[11]-1u))return 0;p[11]=resolve_memory(runtime,(uint32_t)desc,len[11],0);if(!p[11]||load_le(p[11],2)!=actual_count)return 0;}
 p[29]=p[30]=p[31]=p[38]=p[42]=p[45]=p[48]=p[9];unsigned second_count=0;if(descriptor_count>=3){uint64_t ix=(uint64_t)table+second_resolved*2u+4u;if(ix>UINT32_MAX-1u||(ix&1u))return 0;second_index=(uint32_t)ix;p[27]=resolve_memory(runtime,second_index,2,0);if(!p[27])return 0;uint64_t dd=(uint64_t)table+load_le(p[27],2);if(dd>UINT32_MAX-31u||(dd&1u))return 0;second_desc=(uint32_t)dd;p[28]=resolve_memory(runtime,second_desc,32,0);if(!p[28])return 0;second_count=load_le(p[28],2);if(descriptor_count==3?second_count!=0:second_count>(descriptor_count==22?50u:7u))return 0;if(second_count>1){len[28]=4u+second_count*28u;if(dd>UINT32_MAX-(len[28]-1u))return 0;p[28]=resolve_memory(runtime,second_desc,len[28],0);if(!p[28]||load_le(p[28],2)!=second_count)return 0;}p[24]=p[9];p[25]=p[9];p[26]=p[9];len[24]=len[25]=len[26]=4;second_shade=(load_le(p[8]+14,2)&255u)>>1;if(descriptor_count>=4&&(style>=0x2000u||load_le(p[8]+14,2)>=0x2000u||((descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)&&load_le(p[8]+22,2)>=0x2000u)||(descriptor_count>=16&&load_le(p[8]+30,2)>=0x2000u)||(descriptor_count>=19&&load_le(p[8]+38,2)>=0x2000u)||(descriptor_count>=20&&load_le(p[8]+46,2)>=0x2000u)||(descriptor_count==21&&load_le(p[8]+54,2)>=0x2000u))){p[24]=resolve_memory(runtime,0x800c3eacu,4,0);if(!p[24])return 0;uint32_t basefade=load_le(p[24],4);for(unsigned n=0;n<(descriptor_count>=21?7u:descriptor_count==20?6u:descriptor_count==19?5u:descriptor_count>=16?4u:(descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)?3u:2u);n++){uint32_t st=load_le(p[8]+6+n*8u,2);if(st>=0x2000u){uint64_t fa=(uint64_t)basefade+((cpu->gpr[4]&255u)<<6)+0x1cu+((st&15u)<<1);if(fa>UINT32_MAX-1u||(fa&1u))return 0;uint32_t address=(uint32_t)fa;unsigned slot=n==6?48u:n==5?45u:n==4?42u:n==3?38u:n==2?31u:25u+n;p[slot]=resolve_memory(runtime,address,2,0);len[slot]=2;if(!p[slot])return 0;uint32_t sv=load_le(p[slot],2)?16u:((st&0xf0u)>>1);if(n==0){fade_addr=address;shade=sv;}else if(n==1){second_fade_addr=address;second_shade=sv;}else if(n==2){third_fade_addr=address;third_shade=sv;}else if(n==3){fourth_fade_addr=address;fourth_shade=sv;}else if(n==4){fifth_fade_addr=address;fifth_shade=sv;}else if(n==5){sixth_fade_addr=address;sixth_shade=sv;}else{seventh_fade_addr=address;seventh_shade=sv;}}}}protected_count=29;}
 unsigned third_count=0;if(descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22){uint64_t ix=(uint64_t)table+third_resolved*2u+4u;if(ix>UINT32_MAX-1u||(ix&1u))return 0;third_index=(uint32_t)ix;p[29]=resolve_memory(runtime,third_index,2,0);if(!p[29])return 0;uint64_t dd=(uint64_t)table+load_le(p[29],2);if(dd>UINT32_MAX-31u||(dd&1u))return 0;third_desc=(uint32_t)dd;p[30]=resolve_memory(runtime,third_desc,32,0);if(!p[30])return 0;third_count=load_le(p[30],2);if(third_count>(descriptor_count==22?50u:7u))return 0;if(third_count>1){len[30]=4u+third_count*28u;if(dd>UINT32_MAX-(len[30]-1u))return 0;p[30]=resolve_memory(runtime,third_desc,len[30],0);if(!p[30]||load_le(p[30],2)!=third_count)return 0;}if(load_le(p[8]+22,2)<0x2000u)third_shade=(load_le(p[8]+22,2)&255u)>>1;protected_count=32;}if(descriptor_count>=6&&(descriptor_count<=15||descriptor_count==17)){if(descriptor_count==6||descriptor_count==8||descriptor_count==11)len[29]=len[30]=4;protected_count=descriptor_count==15?36:descriptor_count>=11?35:34;}
 unsigned fourth_count=0;if(descriptor_count>=16){for(unsigned i=descriptor_count>=17?34u:32u;i<36;i++){if(descriptor_count!=18&&descriptor_count!=19&&descriptor_count!=20&&descriptor_count!=21&&descriptor_count!=22){p[i]=p[9];len[i]=4;}}uint64_t ix=(uint64_t)table+fourth_resolved*2u+4u;if(ix>UINT32_MAX-1u||(ix&1u))return 0;fourth_index=(uint32_t)ix;p[36]=resolve_memory(runtime,fourth_index,2,0);if(!p[36])return 0;uint64_t dd=(uint64_t)table+load_le(p[36],2);if(dd>UINT32_MAX-31u||(dd&1u))return 0;fourth_desc=(uint32_t)dd;p[37]=resolve_memory(runtime,fourth_desc,32,0);if(!p[37])return 0;fourth_count=load_le(p[37],2);if(fourth_count>(descriptor_count==22?50u:7u))return 0;if(fourth_count>1){len[37]=4u+fourth_count*28u;if(dd>UINT32_MAX-(len[37]-1u))return 0;p[37]=resolve_memory(runtime,fourth_desc,len[37],0);if(!p[37]||load_le(p[37],2)!=fourth_count)return 0;}if(load_le(p[8]+30,2)<0x2000u)fourth_shade=(load_le(p[8]+30,2)&255u)>>1;protected_count=descriptor_count==18?40:39;}
 unsigned fifth_count=0;if(descriptor_count>=19){uint64_t ix=(uint64_t)table+fifth_resolved*2u+4u;if(ix>UINT32_MAX-1u||(ix&1u))return 0;fifth_index=(uint32_t)ix;p[40]=resolve_memory(runtime,fifth_index,2,0);if(!p[40])return 0;uint64_t dd=(uint64_t)table+load_le(p[40],2);if(dd>UINT32_MAX-31u||(dd&1u))return 0;fifth_desc=(uint32_t)dd;p[41]=resolve_memory(runtime,fifth_desc,32,0);if(!p[41])return 0;fifth_count=load_le(p[41],2);if(fifth_count>(descriptor_count==22?50u:7u))return 0;if(fifth_count>1){len[41]=4u+fifth_count*28u;if(dd>UINT32_MAX-(len[41]-1u))return 0;p[41]=resolve_memory(runtime,fifth_desc,len[41],0);if(!p[41]||load_le(p[41],2)!=fifth_count)return 0;}if(load_le(p[8]+38,2)<0x2000u)fifth_shade=(load_le(p[8]+38,2)&255u)>>1;protected_count=43;}
 unsigned sixth_count=0;if(descriptor_count>=20){uint64_t ix=(uint64_t)table+sixth_resolved*2u+4u;if(ix>UINT32_MAX-1u||(ix&1u))return 0;sixth_index=(uint32_t)ix;p[43]=resolve_memory(runtime,sixth_index,2,0);if(!p[43])return 0;uint64_t dd=(uint64_t)table+load_le(p[43],2);if(dd>UINT32_MAX-31u||(dd&1u))return 0;sixth_desc=(uint32_t)dd;p[44]=resolve_memory(runtime,sixth_desc,32,0);if(!p[44])return 0;sixth_count=load_le(p[44],2);if(sixth_count>(descriptor_count==22?50u:7u))return 0;if(sixth_count>1){len[44]=4u+sixth_count*28u;if(dd>UINT32_MAX-(len[44]-1u))return 0;p[44]=resolve_memory(runtime,sixth_desc,len[44],0);if(!p[44]||load_le(p[44],2)!=sixth_count)return 0;}if(load_le(p[8]+46,2)<0x2000u)sixth_shade=(load_le(p[8]+46,2)&255u)>>1;protected_count=46;}
 unsigned seventh_count=0;if(descriptor_count>=21){uint64_t ix=(uint64_t)table+seventh_resolved*2u+4u;if(ix>UINT32_MAX-1u||(ix&1u))return 0;seventh_index=(uint32_t)ix;p[46]=resolve_memory(runtime,seventh_index,2,0);if(!p[46])return 0;uint64_t dd=(uint64_t)table+load_le(p[46],2);if(dd>UINT32_MAX-31u||(dd&1u))return 0;seventh_desc=(uint32_t)dd;p[47]=resolve_memory(runtime,seventh_desc,32,0);if(!p[47])return 0;seventh_count=load_le(p[47],2);if(seventh_count>(descriptor_count==22?50u:7u))return 0;if(seventh_count>1){len[47]=4u+seventh_count*28u;if(dd>UINT32_MAX-(len[47]-1u))return 0;p[47]=resolve_memory(runtime,seventh_desc,len[47],0);if(!p[47]||load_le(p[47],2)!=seventh_count)return 0;}if(load_le(p[8]+54,2)<0x2000u)seventh_shade=(load_le(p[8]+54,2)&255u)>>1;protected_count=49;}
 unsigned extra_total=0;if(descriptor_count==22){for(unsigned j=0;j<stream_count-7u;j++){unsigned ixslot=49u+3u*j,ds=ixslot+1u,fs=ixslot+2u;uint64_t ix=(uint64_t)table+extra_resolved[j]*2u+4u;if(ix>UINT32_MAX-1u||(ix&1u))return 0;extra_index[j]=(uint32_t)ix;p[ixslot]=resolve_memory(runtime,extra_index[j],2,0);len[ixslot]=2;if(!p[ixslot])return 0;uint64_t dd=(uint64_t)table+load_le(p[ixslot],2);if(dd>UINT32_MAX-31u||(dd&1u))return 0;extra_desc[j]=(uint32_t)dd;p[ds]=resolve_memory(runtime,extra_desc[j],32,0);len[ds]=32;if(!p[ds])return 0;extra_count[j]=load_le(p[ds],2);if(extra_count[j]>50)return 0;if(extra_count[j]>1){len[ds]=4u+extra_count[j]*28u;if(dd>UINT32_MAX-(len[ds]-1u))return 0;p[ds]=resolve_memory(runtime,extra_desc[j],len[ds],0);if(!p[ds]||load_le(p[ds],2)!=extra_count[j])return 0;}extra_total+=extra_count[j];uint32_t st=load_le(p[8]+(7u+j)*8u+6u,2);p[fs]=p[9];len[fs]=4;extra_shade[j]=(st&255u)>>1;if(st>=0x2000u){uint64_t fa=(uint64_t)load_le(p[32],4)+((cpu->gpr[4]&255u)<<6)+0x1cu+((st&15u)<<1);if(fa>UINT32_MAX-1u||(fa&1u))return 0;extra_fade[j]=(uint32_t)fa;p[fs]=resolve_memory(runtime,extra_fade[j],2,0);len[fs]=2;if(!p[fs])return 0;extra_shade[j]=load_le(p[fs],2)?16u:((st&0xf0u)>>1);}}unsigned extra_end=49u+3u*(stream_count-7u);for(unsigned i=extra_end;i<124u;i++){p[i]=p[9];len[i]=4;}for(unsigned i=dynamic_count;i<32;i++){p[124u+i]=p[9];len[124u+i]=4;}protected_count=156;}
 const uint32_t sdk_addr[]={0x80043cb0u,0x80043bfcu,0x80043c24u,0x80043a1cu,0x80043a58u};
 const char *sdk_hash[]={"b9c80a1f0571ac7390790b7eaa055a21fb9fe4c1c94374385bc021661bf9e535","f7bd3db94e23b108242338e61ab13d8b1f6877d7a227b90ed3f11c550612804b","0c8699692a9c170bc55cb4e3171f8001f6b2f19d33b1e8c6cbb722317bf88377","ca97bb54edac0e68e757a06d6a25eb10c8fea745f5234450b15d0997d759bf42","879cc65128f4db04b58c281f0867c888d51875da3e7ff5bbb309a1595a4f2457"};
 for(unsigned i=0;i<5;i++){
  size_t pos=XENO_RD_SLUS_OFF(sdk_addr[i]);if(image->size<pos+len[14+i])return 0;p[14+i]=image->bytes+pos;p[19+i]=resolve_memory(runtime,sdk_addr[i],len[19+i],0);if(!p[19+i])return 0;
  uint8_t digest[32];file1_digest(p[14+i],len[14+i],digest);for(unsigned j=0;j<32;j++)if(file1_hex_digit(digest[j]>>4)!=sdk_hash[i][j*2]||file1_hex_digit(digest[j]&15)!=sdk_hash[i][j*2+1])return 0;
 }
 uint32_t buffer=load_le(p[12],4);if(buffer>1)return 0;unsigned total_count=actual_count+second_count+third_count+fourth_count+fifth_count+sixth_count+seventh_count+extra_total;if(total_count>(descriptor_count==22?50u:7u))return 0;unsigned packet_count=total_count?total_count:1u;
 uint64_t dest=(uint64_t)base+p[6][0]*0xfa0u+0x641cu+buffer*40u;if(dest>UINT32_MAX-((packet_count-1u)*80u+39u)||(dest&1u))return 0;
 uint8_t *packet[50]={0};for(unsigned n=0;n<packet_count;n++){
  uint32_t address=(uint32_t)dest+n*80u;packet[n]=resolve_memory(runtime,address,40,1);if(!packet[n]||resolve_memory(runtime,address,40,0)!=packet[n])return 0;
  for(unsigned i=0;i<40;i++)if(resolve_memory(runtime,address+i,1,0)!=packet[n]+i||resolve_memory(runtime,address+i,1,1)!=packet[n]+i)return 0;
  for(unsigned i=0;i<40;i+=2)if(resolve_memory(runtime,address+i,2,0)!=packet[n]+i||resolve_memory(runtime,address+i,2,1)!=packet[n]+i)return 0;
 }
 uint8_t *stack=resolve_memory(runtime,sp-232u,232u,1);if(!stack||resolve_memory(runtime,sp-232u,232u,0)!=stack)return 0;
 /* Every actual access must resolve inside the same frame mapping. */
 const unsigned offsets[]={4,8,12,16,20,24,28,32,36,40,96,94,88,87,80,72,64,56,48,132,136,120,128,156,160,164,168,172,176,180,184,188,192,196,140,216,208,200};
 const unsigned widths[]={4,4,4,4,4,4,4,4,4,4,2,2,1,1,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,2,2,4};
 for(unsigned i=0;i<sizeof(offsets)/sizeof(offsets[0]);i++){
  uint8_t *at=stack+232u-offsets[i];
  if(resolve_memory(runtime,sp-offsets[i],widths[i],0)!=at || resolve_memory(runtime,sp-offsets[i],widths[i],1)!=at)return 0;
 }
 uint8_t *a=resolve_memory(runtime,state+0xd0u,4,1),*b=resolve_memory(runtime,state+0xd4u,4,1);if(!a||!b||resolve_memory(runtime,state+0xd0u,4,0)!=a||resolve_memory(runtime,state+0xd4u,4,0)!=b)return 0;
 uint8_t *writes[85]={stack,a,b};size_t wlen[85]={232,4,4};unsigned nw=3;for(unsigned i=0;i<packet_count;i++){writes[nw]=packet[i];wlen[nw++]=40;}unsigned map_start=nw;if(map_output){writes[nw]=map_output;wlen[nw++]=1;}if(map_output2){writes[nw]=map_output2;wlen[nw++]=1;}if(map_output3){writes[nw]=map_output3;wlen[nw++]=1;}if(map_output4){writes[nw]=map_output4;wlen[nw++]=1;}for(unsigned i=0;i<dynamic_count;i++){writes[nw]=dynamic_output[i];wlen[nw++]=1;}for(unsigned i=0;i<nw;i++){
  for(unsigned j=0;j<protected_count;j++)if((uintptr_t)writes[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)writes[i]+wlen[i])return 0;
  for(unsigned j=0;j<i;j++)if((uintptr_t)writes[i]<(uintptr_t)writes[j]+wlen[j]&&(uintptr_t)writes[j]<(uintptr_t)writes[i]+wlen[i]){if(descriptor_count>=18&&descriptor_count<=22&&i>=map_start&&j>=map_start&&writes[i]==writes[j])continue;return 0;}
 }
 if(guard){
  uint32_t addr[156]={target,0x80076a10u,0,0x800c3000u+slot*4u,0x800d2d28u,0x800c3ea4u,src,0x800c2f4cu+p[6][2]*4u,glyph,0x800d2f5cu,(uint32_t)index,(uint32_t)desc,0x800ccb34u,0x8002675cu,0,0,0,0,0,0,0,0,0,0,0x800c3eacu,fade_addr,map_lookup,second_index,second_desc,third_index,third_desc,0x800d2f5cu,0x800c3eacu,map_lookup,map_lookup2,map_lookup3,fourth_index,fourth_desc,0x800d2f5cu,map_lookup4,fifth_index,fifth_desc,0x800d2f5cu,sixth_index,sixth_desc,0x800d2f5cu,seventh_index,seventh_desc,0x800d2f5cu};if(descriptor_count>=21&&load_le(p[8]+54,2)>=0x2000u)addr[48]=seventh_fade_addr;if(descriptor_count>=20&&load_le(p[8]+46,2)>=0x2000u)addr[45]=sixth_fade_addr;if(descriptor_count>=19&&load_le(p[8]+38,2)>=0x2000u)addr[42]=fifth_fade_addr;if(descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22){if(!map_output)addr[33]=0x800d2f5cu;if(!map_output2)addr[34]=0x800d2f5cu;if(!map_output3)addr[35]=0x800d2f5cu;if(!map_output4)addr[39]=0x800d2f5cu;}if(descriptor_count>=16&&load_le(p[8]+30,2)>=0x2000u)addr[38]=fourth_fade_addr;if(descriptor_count>=16&&descriptor_count!=18&&descriptor_count!=19&&descriptor_count!=20&&descriptor_count!=21&&descriptor_count!=22)for(unsigned i=descriptor_count==17?34u:32u;i<36;i++)addr[i]=0x800d2f5cu;if(descriptor_count==6||descriptor_count==8||descriptor_count==11)addr[29]=addr[30]=0x800d2f5cu;if(descriptor_count>=3){unsigned sf=load_le(p[8]+14,2);addr[24]=(descriptor_count>=4&&(style>=0x2000u||sf>=0x2000u||((descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)&&load_le(p[8]+22,2)>=0x2000u)||(descriptor_count>=16&&load_le(p[8]+30,2)>=0x2000u)||(descriptor_count>=19&&load_le(p[8]+38,2)>=0x2000u)||(descriptor_count>=20&&load_le(p[8]+46,2)>=0x2000u)||(descriptor_count==21&&load_le(p[8]+54,2)>=0x2000u)))?0x800c3eacu:0x800d2f5cu;addr[25]=(descriptor_count>=4&&style>=0x2000u)?fade_addr:0x800d2f5cu;addr[26]=(descriptor_count>=4&&sf>=0x2000u)?second_fade_addr:0x800d2f5cu;if((descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)&&load_le(p[8]+22,2)>=0x2000u)addr[31]=third_fade_addr;}
  if(descriptor_count==22){for(unsigned j=0;j<stream_count-7u;j++){addr[49u+3u*j]=extra_index[j];addr[50u+3u*j]=extra_desc[j];addr[51u+3u*j]=load_le(p[8]+(7u+j)*8u+6u,2)>=0x2000u?extra_fade[j]:0x800d2f5cu;}for(unsigned i=49u+3u*(stream_count-7u);i<124u;i++)addr[i]=0x800d2f5cu;for(unsigned i=0;i<32;i++)addr[124u+i]=i<dynamic_count?dynamic_lookup[i]:0x800d2f5cu;}for(unsigned i=0;i<5;i++)addr[19+i]=sdk_addr[i];guard->runtime=runtime;guard->current=0;guard->glyph_counts[0]=actual_count;guard->glyph_counts[1]=second_count;guard->glyph_counts[2]=third_count;guard->glyph_counts[3]=fourth_count;guard->glyph_counts[4]=fifth_count;guard->glyph_counts[5]=sixth_count;guard->glyph_counts[6]=seventh_count;guard->seventh_glyph=seventh_glyph;guard->seventh_resolved=seventh_resolved;guard->shades[6]=descriptor_count>=21?seventh_shade:0;guard->sixth_glyph=sixth_glyph;guard->sixth_resolved=sixth_resolved;guard->shades[5]=descriptor_count>=20?sixth_shade:0;guard->fifth_glyph=fifth_glyph;guard->fifth_resolved=fifth_resolved;guard->shades[4]=descriptor_count>=19?fifth_shade:0;guard->fourth_glyph=fourth_glyph;guard->fourth_resolved=fourth_resolved;guard->fourth_mapped=(descriptor_count==17&&mapping_pos==3);guard->shades[3]=descriptor_count>=16?fourth_shade:0;guard->glyph_total=descriptor_count==22?stream_count:descriptor_count>=21?7u:descriptor_count==20?6u:descriptor_count==19?5u:descriptor_count>=16?4u:(descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)?3u:2u;guard->third_glyph=third_glyph;guard->third_resolved=third_resolved;guard->third_mapped=(descriptor_count==10||descriptor_count==13||descriptor_count==14||descriptor_count==15||(descriptor_count==17&&mapping_pos==2));guard->map_first_later=(descriptor_count==14);guard->shades[2]=(descriptor_count==5||descriptor_count==7||descriptor_count==9||descriptor_count==10||descriptor_count==12||descriptor_count==13||descriptor_count==14||descriptor_count==15||descriptor_count==16||descriptor_count==17||descriptor_count==18||descriptor_count==19||descriptor_count==20||descriptor_count==21||descriptor_count==22)?third_shade:0;guard->packet_count=packet_count;guard->shades[0]=shade;guard->shades[1]=descriptor_count>=3?second_shade:0;guard->second_glyph=second_glyph;guard->second_resolved=second_resolved;guard->second_mapped=(descriptor_count==8||descriptor_count==9||descriptor_count==11||descriptor_count==12||descriptor_count==14||descriptor_count==15||(descriptor_count==17&&mapping_pos==1));guard->map_addr2=map_addr2;guard->map_value2=map_value2;guard->map_output2=map_output2;guard->map_initial2=map_output2?*map_output2:0;guard->map_written2=0;guard->map_addr3=map_addr3;guard->map_value3=map_value3;guard->map_output3=map_output3;guard->map_initial3=map_output3?*map_output3:0;guard->map_written3=0;guard->map_addr4=map_addr4;guard->map_value4=map_value4;guard->map_output4=map_output4;guard->map_initial4=map_output4?*map_output4:0;guard->map_written4=0;guard->allow_reused_mapping=(descriptor_count>=18&&descriptor_count<=22);guard->dynamic_map_count=dynamic_count;for(unsigned i=0;i<dynamic_count;i++){guard->dynamic_addr[i]=dynamic_addr[i];guard->dynamic_value[i]=dynamic_value[i];guard->dynamic_output[i]=dynamic_output[i];guard->dynamic_initial[i]=*dynamic_output[i];guard->dynamic_written[i]=0;}for(unsigned i=0;i<32;i++)guard->map_sequence[i]=map_sequence[i];if(descriptor_count==22)for(unsigned j=0;j<stream_count-7u;j++){guard->glyph_counts[7u+j]=extra_count[j];guard->shades[7u+j]=extra_shade[j];guard->extra_raw[j]=extra_raw[j];guard->extra_resolved[j]=extra_resolved[j];guard->extra_desc[j]=extra_desc[j];}guard->map_value=map_value;guard->map_written=0;guard->map_initial=map_output?*map_output:0;guard->resolved_glyph=id;guard->map_addr=map_addr;guard->map_output=map_output;guard->descriptor_count=actual_count;guard->protected_count=protected_count;guard->shade=shade;guard->image=image;guard->image_bytes=image->bytes;guard->image_size=image->size;
  for(unsigned i=0;i<protected_count;i++){guard->ptr[i]=p[i];guard->len[i]=len[i];guard->addr[i]=addr[i];file1_digest(p[i],len[i],guard->digest[i]);}
  const unsigned saved_regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)guard->saved[i]=cpu->gpr[saved_regs[i]];guard->arg0=cpu->gpr[4];
  guard->stack_addr=sp-232u;guard->state_addr=state+0xd0u;guard->packet_addr=(uint32_t)dest;guard->stack=stack;guard->a=a;guard->b=b;for(unsigned i=0;i<packet_count;i++)guard->packet[i]=packet[i];
 }
 return 1;
}
static int one_glyph_preflight(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target){return one_glyph_preflight_capture(runtime,cpu,target,NULL,1);}

/* Run the bounded parent and its scoped native glyph services. */
static int one_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,1))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_ONE_GLYPH_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);glyph_continue_one_primitive(&e);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int zero_glyph_scoped_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,0))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_ZERO_GLYPH_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);zero_glyph_continue(&e);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int mapped_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,2))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_MAPPED_GLYPH_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5])return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 battle_entry_write(&e,guard.map_addr,1,guard.resolved_glyph);guard.map_written=1;
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);if(guard.descriptor_count==0)zero_glyph_continue(&e);else glyph_continue_one_primitive(&e);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static void two_zero_continue(BattleEntryCall *e,OneGlyphGuard *guard)
{
 uint32_t *r=e->cpu->gpr;
 r[3]=battle_entry_read(e,r[22],2);r[4]=battle_entry_read(e,0x800d2d28u,4);r[3]=(r[3]<<2)+r[4];r[4]=battle_entry_read(e,r[3]+0xd0u,4);r[2]+=r[4];battle_entry_write(e,r[3]+0xd0u,4,r[2]);
 r[2]=battle_entry_read(e,r[30],1);r[9]=0x800c2f4cu;r[2]=battle_entry_read(e,r[9]+r[2]*4u,4);r[16]+=r[2];r[4]=battle_entry_read(e,r[16]+6u,2);r[2]=battle_entry_read(e,r[16]+6u,2);r[3]=r[4]&255u;r[2]=r[2]<0x2000u;r[18]=r[3]>>1;
 if(!r[2]){r[3]=r[4]&15u;r[2]=battle_entry_read(e,0x800c3eacu,4);r[9]=battle_entry_read(e,r[29]+32u,4);r[3]<<=1;r[2]+=r[9];r[3]+=r[2];r[2]=battle_entry_read(e,r[3]+0x1cu,2);r[18]=16u;if(!r[2]){r[2]=r[4]&0xf0u;r[18]=r[2]>>1;}}
 if(r[18]){
 r[2]=battle_entry_read(e,r[22],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[19]=battle_entry_read(e,r[29]+40u,4);r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);r[16]=r[23]+r[19];
 if(r[2])battle_entry_fail(e,"unexpected primitive",0x8008d868u);
 }else{
  r[2]=battle_entry_read(e,r[22],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[6]=battle_entry_read(e,r[29]+40u,4);r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);if(r[2])battle_entry_fail(e,"unexpected primitive",0x8008da7cu);
 }
 r[20]+=4;r[2]=battle_entry_read(e,r[30],1);r[9]=0x800c2f4cu;r[2]=(r[2]<<2)+r[9];r[3]=battle_entry_read(e,r[2],4);r[2]=(r[20]<<1)+r[3];r[4]=battle_entry_read(e,r[2],2);r[2]=(int32_t)r[4]<0x4000;
 if(r[4]!=0xffffu){
  if(guard->current!=0||r[4]!=guard->second_glyph)battle_entry_fail(e,"unexpected next glyph",0x8008dbb0u);guard->current=1;
  r[16]=r[20]<<1;r[9]=0x800c2f4cu;r[2]=battle_entry_read(e,r[22],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[5]=(r[2]<<2)+r[3];r[3]=r[2]*0xfa0u+0x641cu;r[2]=battle_entry_read(e,0x800c3ea4u,4);r[8]=battle_entry_read(e,r[5]+0xd0u,4);r[3]+=r[2];r[5]=r[8]*80u;r[2]=battle_entry_read(e,r[30],1);r[2]=battle_entry_read(e,r[9]+r[2]*4u,4);r[5]+=r[3];r[2]+=r[16];r[6]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[2]+2u,2);r[7]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[2]+4u,2);r[31]=0x8008d7b0u;r[17]=r[8]<<1;
  battle_phase_call(e,0x80076a10u,0x8008d7a8u);two_zero_continue(e,guard);return;
 }
 r[9]=battle_entry_read(e,r[29]+56u,4)+1u;battle_entry_write(e,r[29]+56u,4,r[9]);r[9]=battle_entry_read(e,r[29]+64u,4)+2u;battle_entry_write(e,r[29]+64u,4,r[9]);r[9]=battle_entry_read(e,r[29]+48u,4);r[2]=r[9]+2u;r[9]=battle_entry_read(e,r[29]+56u,4);r[2]=(int32_t)r[9]<(int32_t)r[2];r[23]+=2u;
 if(!r[2])battle_entry_fail(e,"missing second sentinel",0x8008dbf0u);
 r[9]=battle_entry_read(e,r[29]+64u,4);r[3]=battle_entry_read(e,r[9],2);r[2]=255u;if(r[3]!=255u)battle_entry_fail(e,"second group unsupported",0x8008d69cu);
 r[2]=battle_entry_read(e,0x800ccb34u,4);const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)r[regs[i]]=battle_entry_read(e,r[29]+108u-i*4u,4);r[29]+=112u;
}

/* Guard for the complete zero-descriptor path. */

static int two_zero_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,3))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_TWO_ZERO_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_zero_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static void glyph_sequence_write_map(BattleEntryCall *e,OneGlyphGuard *g,unsigned pos){if(g->dynamic_map_count){unsigned i=g->map_sequence[pos]-1u;if(i>=g->dynamic_map_count)battle_entry_fail(e,"invalid mapping sequence",0);battle_entry_write(e,g->dynamic_addr[i],1,g->dynamic_value[i]);g->dynamic_written[i]=1;return;}switch(g->map_sequence[pos]){case 1:battle_entry_write(e,g->map_addr,1,g->map_value);g->map_written=1;break;case 2:battle_entry_write(e,g->map_addr2,1,g->map_value2);g->map_written2=1;break;case 3:battle_entry_write(e,g->map_addr3,1,g->map_value3);g->map_written3=1;break;case 4:battle_entry_write(e,g->map_addr4,1,g->map_value4);g->map_written4=1;break;default:battle_entry_fail(e,"invalid mapping sequence",0);}}

static void two_mixed_continue(BattleEntryCall *e,OneGlyphGuard *guard)
{
 uint32_t *r=e->cpu->gpr;
 r[3]=battle_entry_read(e,r[22],2);r[4]=battle_entry_read(e,0x800d2d28u,4);r[3]=(r[3]<<2)+r[4];r[4]=battle_entry_read(e,r[3]+0xd0u,4);r[2]+=r[4];battle_entry_write(e,r[3]+0xd0u,4,r[2]);
 r[2]=battle_entry_read(e,r[30],1);r[9]=0x800c2f4cu;r[2]=battle_entry_read(e,r[9]+r[2]*4u,4);r[16]+=r[2];r[4]=battle_entry_read(e,r[16]+6u,2);r[2]=battle_entry_read(e,r[16]+6u,2);r[3]=r[4]&255u;r[2]=r[2]<0x2000u;r[18]=r[3]>>1;
 if(!r[2]){r[3]=r[4]&15u;r[2]=battle_entry_read(e,0x800c3eacu,4);r[9]=battle_entry_read(e,r[29]+32u,4);r[3]<<=1;r[2]+=r[9];r[3]+=r[2];r[2]=battle_entry_read(e,r[3]+0x1cu,2);r[18]=16u;if(!r[2]){r[2]=r[4]&0xf0u;r[18]=r[2]>>1;}}
 if(r[18]){
 r[2]=battle_entry_read(e,r[22],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[19]=battle_entry_read(e,r[29]+40u,4);r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);r[16]=r[23]+r[19];
 if(r[2]){r[21]=0x800ccb34u;unsigned iterations=0;do{if(iterations++>=50)battle_entry_fail(e,"primitive limit exceeded",0x8008d87cu);r[5]=1;r[3]=battle_entry_read(e,r[16],2);r[4]=battle_entry_read(e,0x800c3ea4u,4);r[2]=r[3]*0xfa0u+0x641cu;r[3]=battle_entry_read(e,r[21],4);r[2]+=r[4];r[3]+=r[17];r[4]=r[2]+r[3]*40u;r[31]=0x8008d8c4u;
 battle_phase_call(e,0x80043bfcu,0x8008d8bcu);
 r[5]=0;r[3]=battle_entry_read(e,r[16],2);r[4]=battle_entry_read(e,0x800c3ea4u,4);r[2]=r[3]*0xfa0u+0x641cu;r[3]=battle_entry_read(e,r[21],4);r[2]+=r[4];r[3]+=r[17];r[4]=r[2]+r[3]*40u;r[31]=0x8008d90cu;battle_phase_call(e,0x80043c24u,0x8008d904u);
 for(unsigned color=0;color<3;color++){
  r[2]=battle_entry_read(e,0x800ccb34u,4);r[4]=battle_entry_read(e,r[16],2);r[2]+=r[17];r[3]=r[2]*40u;r[2]=r[4]*0xfa0u;r[4]=battle_entry_read(e,0x800c3ea4u,4);r[2]+=r[4];r[3]+=r[2];battle_entry_write(e,r[3]+0x6420u+color,1,r[18]);
 }
 r[2]=battle_entry_read(e,r[21],4);r[4]=battle_entry_read(e,r[16],2);r[2]+=r[17];r[3]=r[2]*40u;r[2]=r[4]*0xfa0u;r[4]=battle_entry_read(e,0x800c3ea4u,4);r[3]+=r[2];r[4]+=r[3];r[2]=battle_entry_read(e,r[4]+0x6432u,2)|0x20u;battle_entry_write(e,r[4]+0x6432u,2,r[2]);
 r[2]=battle_entry_read(e,r[16],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[17]+=2;r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);r[16]=r[23]+r[19];}while(r[2]);}
 r[20]+=4;
 }else{
  r[2]=battle_entry_read(e,r[22],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[6]=battle_entry_read(e,r[29]+40u,4);r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);if(r[2]){
  r[7]=0x80u;r[5]=r[23]+r[6];unsigned iterations=0;do{if(iterations++>=50)battle_entry_fail(e,"primitive limit exceeded",0x8008da88u);for(unsigned color=0;color<3;color++){
   r[2]=battle_entry_read(e,0x800ccb34u,4);r[4]=battle_entry_read(e,r[5],2);r[2]+=r[17];r[3]=r[2]*40u;r[2]=r[4]*0xfa0u;r[4]=battle_entry_read(e,0x800c3ea4u,4);r[2]+=r[4];r[3]+=r[2];battle_entry_write(e,r[3]+0x6420u+color,1,r[7]);
  }
  r[2]=battle_entry_read(e,r[5],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[2]=battle_entry_read(e,r[3]+r[2]*4u+0xd0u,4);r[17]+=2;r[2]=(int32_t)r[17]<(int32_t)(r[2]<<1);}while(r[2]);}r[20]+=4;
 }
 r[2]=battle_entry_read(e,r[30],1);r[9]=0x800c2f4cu;r[2]=(r[2]<<2)+r[9];r[3]=battle_entry_read(e,r[2],4);r[2]=(r[20]<<1)+r[3];r[4]=battle_entry_read(e,r[2],2);r[2]=(int32_t)r[4]<0x4000;if(r[4]!=0xffffu){
  if(guard->current+1u>=guard->glyph_total||r[4]!=(guard->current==0?guard->second_glyph:guard->current==1?guard->third_glyph:guard->current==2?guard->fourth_glyph:guard->current==3?guard->fifth_glyph:guard->current==4?guard->sixth_glyph:guard->current==5?guard->seventh_glyph:guard->extra_raw[guard->current-6]))battle_entry_fail(e,"unexpected next glyph",0x8008dbb0u);guard->current++;guard->descriptor_count=guard->glyph_counts[guard->current];guard->shade=guard->shades[guard->current];if(guard->map_sequence[guard->current]){glyph_sequence_write_map(e,guard,guard->current);r[4]=guard->current==1?guard->second_resolved:guard->current==2?guard->third_resolved:guard->current==3?guard->fourth_resolved:guard->current==4?guard->fifth_resolved:guard->current==5?guard->sixth_resolved:guard->current==6?guard->seventh_resolved:guard->extra_resolved[guard->current-7];}else if((guard->second_mapped&&guard->current==1)||(guard->third_mapped&&guard->current==2)||(guard->fourth_mapped&&guard->current==3)){if(guard->map_output3&&guard->current==2){battle_entry_write(e,guard->map_addr3,1,guard->map_value3);guard->map_written3=1;}else if(guard->map_output2&&!(guard->map_first_later&&guard->current==1)){battle_entry_write(e,guard->map_addr2,1,guard->map_value2);guard->map_written2=1;}else{battle_entry_write(e,guard->map_addr,1,guard->map_value);guard->map_written=1;}r[4]=guard->current==1?guard->second_resolved:guard->current==2?guard->third_resolved:guard->current==3?guard->fourth_resolved:guard->current==4?guard->fifth_resolved:guard->current==5?guard->sixth_resolved:guard->current==6?guard->seventh_resolved:guard->extra_resolved[guard->current-7];}
  r[16]=r[20]<<1;r[9]=0x800c2f4cu;r[2]=battle_entry_read(e,r[22],2);r[3]=battle_entry_read(e,0x800d2d28u,4);r[5]=(r[2]<<2)+r[3];r[3]=r[2]*0xfa0u+0x641cu;r[2]=battle_entry_read(e,0x800c3ea4u,4);r[8]=battle_entry_read(e,r[5]+0xd0u,4);r[3]+=r[2];r[5]=r[8]*80u;r[2]=battle_entry_read(e,r[30],1);r[2]=battle_entry_read(e,r[9]+r[2]*4u,4);r[5]+=r[3];r[2]+=r[16];r[6]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[2]+2u,2);r[7]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[2]+4u,2);r[31]=0x8008d7b0u;r[17]=r[8]<<1;
  battle_phase_call(e,0x80076a10u,0x8008d7a8u);two_mixed_continue(e,guard);return;
 }
 r[9]=battle_entry_read(e,r[29]+56u,4)+1u;battle_entry_write(e,r[29]+56u,4,r[9]);r[9]=battle_entry_read(e,r[29]+64u,4)+2u;battle_entry_write(e,r[29]+64u,4,r[9]);r[9]=battle_entry_read(e,r[29]+48u,4);r[2]=r[9]+2u;r[9]=battle_entry_read(e,r[29]+56u,4);r[2]=(int32_t)r[9]<(int32_t)r[2];r[23]+=2u;if(!r[2])battle_entry_fail(e,"missing second sentinel",0x8008dbf0u);
 r[9]=battle_entry_read(e,r[29]+64u,4);r[3]=battle_entry_read(e,r[9],2);r[2]=255u;if(r[3]!=255u)battle_entry_fail(e,"second group unsupported",0x8008d69cu);
 r[2]=battle_entry_read(e,0x800ccb34u,4);const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)r[regs[i]]=battle_entry_read(e,r[29]+108u-i*4u,4);r[29]+=112u;
}


static int two_mixed_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,4))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_TWO_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
static int second_mapped_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,8))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_SECOND_MAPPED_GLYPH_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
static int two_mapped_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,6))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_TWO_MAPPED_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5]||load_le(p[5],2)<0x4000u||load_le(p[5],2)==0xffffu)return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 battle_entry_write(&e,guard.map_addr,1,guard.resolved_glyph);guard.map_written=1;
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
static int two_mappings_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,11))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_TWO_MAPPINGS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5]||load_le(p[5],2)<0x4000u||load_le(p[5],2)==0xffffu)return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 battle_entry_write(&e,guard.map_addr,1,guard.resolved_glyph);guard.map_written=1;
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
static int three_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,5))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_THREE_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int third_mapped_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,10))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_THIRD_MAPPED_GLYPH_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int second_mapped_three_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,9))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_SECOND_MAPPED_THREE_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int three_mapped_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,7))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_THREE_MAPPED_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5]||load_le(p[5],2)<0x4000u||load_le(p[5],2)==0xffffu)return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 battle_entry_write(&e,guard.map_addr,1,guard.resolved_glyph);guard.map_written=1;
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int three_first_second_mapped_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,12))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_THREE_FIRST_SECOND_MAPPED_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5]||load_le(p[5],2)<0x4000u||load_le(p[5],2)==0xffffu)return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 battle_entry_write(&e,guard.map_addr,1,guard.resolved_glyph);guard.map_written=1;
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int three_first_third_mapped_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,13))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_THREE_FIRST_THIRD_MAPPED_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5]||load_le(p[5],2)<0x4000u||load_le(p[5],2)==0xffffu)return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 battle_entry_write(&e,guard.map_addr,1,guard.resolved_glyph);guard.map_written=1;
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int three_second_third_mapped_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,14))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_THREE_SECOND_THIRD_MAPPED_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int three_mappings_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,15))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_THREE_MAPPINGS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5]||load_le(p[5],2)<0x4000u||load_le(p[5],2)==0xffffu)return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 battle_entry_write(&e,guard.map_addr,1,guard.resolved_glyph);guard.map_written=1;
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}


static int four_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,16))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_FOUR_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
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
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=load_le(p[5],2);r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int four_one_mapped_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,17))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_FOUR_ONE_MAPPED_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5])return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 if(guard.map_output&&!(guard.second_mapped||guard.third_mapped||guard.fourth_mapped)){battle_entry_write(&e,guard.map_addr,1,guard.map_value);guard.map_written=1;}
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int four_mapped_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,18))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_FOUR_MAPPED_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5])return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 if(guard.map_sequence[0])glyph_sequence_write_map(&e,&guard,0);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int five_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,19))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_FIVE_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5])return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 if(guard.map_sequence[0])glyph_sequence_write_map(&e,&guard,0);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int six_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,20))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_SIX_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5])return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 if(guard.map_sequence[0])glyph_sequence_write_map(&e,&guard,0);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int seven_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,21))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_SEVEN_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5])return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 if(guard.map_sequence[0])glyph_sequence_write_map(&e,&guard,0);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

static int many_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
 OneGlyphGuard guard;if(cpu->bus.bridge!=runtime_bridge || !one_glyph_preflight_capture(runtime,cpu,target,&guard,22))return 0;
 if(target!=0x8008d598u || cpu->gpr[31]!=0x80080758u || (cpu->gpr[6]&255u) || runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg))return 0;
 const char *control=getenv("XENO_BATTLE_NATIVE_MANY_GLYPHS_EXPERIMENT");if(!control||strcmp(control,"1"))return 0;
 uint32_t sp=cpu->gpr[29],slot=cpu->gpr[5]&255u;if((sp&3u)||sp<BATTLE_BUS_KUSEG_RAM_END+112u||slot>1)return 0;
 const uint8_t *p[7];size_t len[]={0x218,4,4,4,4,6,4};p[0]=resolve_memory(runtime,target,0x218,0);if(!p[0])return 0;
 const char *hash="289b8eea0bbbe8060909f22f0ca87af7f218f0700a24f62ba0c5d93909bc5d0a";uint8_t digest[32];file1_digest(p[0],0x218,digest);for(unsigned i=0;i<32;i++)if(file1_hex_digit(digest[i]>>4)!=hash[i*2]||file1_hex_digit(digest[i]&15)!=hash[i*2+1])return 0;
 p[1]=resolve_memory(runtime,0x800c3000u+slot*4u,4,0);p[2]=resolve_memory(runtime,0x800d2d28u,4,0);p[6]=resolve_memory(runtime,0x800c3ea4u,4,0);if(!p[1]||!p[2]||!p[6])return 0;
 uint32_t source=load_le(p[1],4),state=load_le(p[2],4),base=load_le(p[6],4);if(source>UINT32_MAX-3u || (state&3u)||state>UINT32_MAX-0xd7u)return 0;
 p[3]=resolve_memory(runtime,source,4,0);if(!p[3]||p[3][0]>1||p[3][2]>7)return 0;
 p[4]=resolve_memory(runtime,0x800c2f4cu+p[3][2]*4u,4,0);if(!p[4])return 0;uint32_t glyph=load_le(p[4],4);if(glyph&1u||glyph>UINT32_MAX-5u)return 0;
 p[5]=resolve_memory(runtime,glyph,6,0);if(!p[5])return 0;
 uint64_t prim=(uint64_t)base+p[3][0]*0xfa0u+0x641cu;if(prim>UINT32_MAX)return 0;
 uint32_t addr[21];unsigned wid[21],n=0;for(unsigned i=0;i<10;i++){addr[n]=sp-4u-i*4u;wid[n++]=4;}
 const unsigned off[]={16,18,24,25,32,40,48,56,64};const unsigned sizes[]={2,2,1,1,4,4,4,4,4};
 for(unsigned i=0;i<9;i++){addr[n]=sp-112u+off[i];wid[n++]=sizes[i];}addr[n]=state+0xd0u;wid[n++]=4;addr[n]=state+0xd4u;wid[n++]=4;
 uint8_t *w[21];for(unsigned i=0;i<n;i++){w[i]=resolve_memory(runtime,addr[i],wid[i],1);if(!w[i])return 0;for(unsigned j=0;j<7;j++)if((uintptr_t)w[i]<(uintptr_t)p[j]+len[j]&&(uintptr_t)p[j]<(uintptr_t)w[i]+wid[i])return 0;for(unsigned j=0;j<i;j++)if((uintptr_t)w[i]<(uintptr_t)w[j]+wid[j]&&(uintptr_t)w[j]<(uintptr_t)w[i]+wid[i])return 0;}
 OneGlyphGuard *previous_guard=one_glyph_active;BattleEntryCall e={.runtime=runtime,.cpu=cpu};if(setjmp(e.failure)){one_glyph_active=previous_guard;return -1;}one_glyph_active=&guard;uint32_t *r=cpu->gpr,frame=sp-112u;
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(&e,sp-4u-i*4u,4,r[regs[i]]);
 for(unsigned i=0;i<2;i++){battle_entry_write(&e,frame+16u+2u*i,2,p[3][i]);battle_entry_write(&e,frame+24u+i,1,p[3][i+2]);battle_entry_write(&e,state+0xd0u+4u*i,4,0);}
 battle_entry_write(&e,frame+48u,4,frame+24u);battle_entry_write(&e,frame+32u,4,(r[4]&255u)<<6);battle_entry_write(&e,frame+56u,4,frame+24u);battle_entry_write(&e,frame+40u,4,frame+16u);battle_entry_write(&e,frame+64u,4,frame+16u);
 if(guard.map_sequence[0])glyph_sequence_write_map(&e,&guard,0);
 r[29]=frame;r[2]=glyph;r[3]=(uint32_t)prim;r[4]=guard.resolved_glyph;r[5]=(uint32_t)prim;r[6]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+2,2);r[7]=(uint32_t)(int32_t)(int16_t)load_le(p[5]+4,2);r[8]=0;r[9]=0x800c2f4cu;r[16]=0;r[17]=0;r[20]=0;r[22]=frame+16u;r[23]=0;r[30]=frame+24u;r[31]=0x8008d7b0u;
 battle_phase_call(&e,0x80076a10u,0x8008d7a8u);two_mixed_continue(&e,&guard);one_glyph_active=previous_guard;cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
