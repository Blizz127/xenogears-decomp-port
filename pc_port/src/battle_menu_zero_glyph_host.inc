/* Exact restricted wrapper and zero-count renderer, no SDK calls. */
static void zero_glyph_renderer(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=80u;r[5]=(r[5]<<1)+r[4];
 const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)battle_entry_write(e,r[29]+76u-i*4u,4,r[regs[i]]);
 battle_entry_write(e,r[29]+36u,4,0);r[2]=battle_entry_read(e,r[5]+4u,2);r[9]=battle_entry_read(e,r[29]+96u,2);battle_entry_write(e,r[29]+92u,4,r[7]);battle_entry_write(e,r[29]+16u,2,r[9]);r[9]=battle_entry_read(e,r[29]+100u,2);r[2]+=r[4];battle_entry_write(e,r[29]+32u,4,r[2]);battle_entry_write(e,r[29]+24u,2,r[9]);r[2]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[2],2);r[3]=battle_entry_read(e,r[29]+104u,2);
 if(r[2])battle_entry_fail(e,"nonempty renderer unsupported",0x800267bcu);
 r[9]=battle_entry_read(e,r[29]+32u,4);r[2]=(uint32_t)(int32_t)(int16_t)battle_entry_read(e,r[9],2);
 for(unsigned i=0;i<10;i++)r[regs[i]]=battle_entry_read(e,r[29]+76u-i*4u,4);r[29]+=80u;
}
static void zero_glyph_wrapper(BattleEntryCall *e)
{
 uint32_t *r=e->cpu->gpr;r[29]-=40u;r[3]=r[4];r[8]=r[5];r[6]=(uint32_t)(int32_t)(int16_t)r[6];r[7]=(uint32_t)(int32_t)(int16_t)r[7];r[4]=battle_entry_read(e,0x800d2f5cu,4);r[2]=0x1000u;battle_entry_write(e,r[29]+20u,4,r[7]);r[7]=battle_entry_read(e,0x800ccb34u,4);r[5]=r[3];battle_entry_write(e,r[29]+16u,4,r[6]);r[6]=r[8];battle_entry_write(e,r[29]+32u,4,r[31]);r[31]=0x80076a5cu;battle_entry_write(e,r[29]+24u,4,r[2]);battle_phase_call(e,0x8002675cu,0x80076a54u);r[31]=battle_entry_read(e,r[29]+32u,4);r[29]+=40u;
}

/* Restricted continuation after a glyph wrapper returns zero primitives. */
static void zero_glyph_continue(BattleEntryCall *e)
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
 if(r[4]!=0xffffu)battle_entry_fail(e,"another glyph unsupported",0x8008dbb0u);
 r[9]=battle_entry_read(e,r[29]+56u,4)+1u;battle_entry_write(e,r[29]+56u,4,r[9]);r[9]=battle_entry_read(e,r[29]+64u,4)+2u;battle_entry_write(e,r[29]+64u,4,r[9]);r[9]=battle_entry_read(e,r[29]+48u,4);r[2]=r[9]+2u;r[9]=battle_entry_read(e,r[29]+56u,4);r[2]=(int32_t)r[9]<(int32_t)r[2];r[23]+=2u;
 if(!r[2])battle_entry_fail(e,"missing second sentinel",0x8008dbf0u);
 r[9]=battle_entry_read(e,r[29]+64u,4);r[3]=battle_entry_read(e,r[9],2);r[2]=255u;if(r[3]!=255u)battle_entry_fail(e,"second group unsupported",0x8008d69cu);
 r[2]=battle_entry_read(e,0x800ccb34u,4);const unsigned regs[]={31,30,23,22,21,20,19,18,17,16};for(unsigned i=0;i<10;i++)r[regs[i]]=battle_entry_read(e,r[29]+108u-i*4u,4);r[29]+=112u;
}

/* Guard for the complete zero-descriptor path. */
static int zero_glyph_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{return zero_glyph_scoped_try_host(runtime,cpu,target);}
