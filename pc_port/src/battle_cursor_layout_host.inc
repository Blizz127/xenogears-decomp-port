/* Experimental cursor layout for bounded counts and slot. */
static void battle_cursor_layout_run(BattleEntryCall *entry,uint32_t expected_work,uint32_t expected_frame,uint32_t expected_slot)
{
    uint32_t *r=entry->cpu->gpr;r[8]=r[5];r[4]&=0xffu;r[2]=r[8]&0xffu;int unequal=r[4]!=r[2];r[29]-=8u;if(!unequal)r[8]=r[5]-1u;
    r[2]=r[4]-1u;r[6]=0;
    if((int32_t)r[2]>0){
        r[7]=0x800c0000u;uint32_t work=battle_entry_read(entry,r[7]+0x3ea4u,4);r[10]=12;r[7]=work;if(work!=expected_work)battle_entry_fail(entry,"cursor work changed",work);r[9]=r[2];r[3]=0x800c3202u;r[2]=(r[4]*3u)<<1;r[5]=r[2]+r[3];
        do {r[4]=r[6]<<1;r[2]=0x800d0000u;uint32_t slot=battle_entry_read(entry,r[2]+0xffffcb34u,4);r[6]++;if(slot!=expected_slot)battle_entry_fail(entry,"cursor slot changed",slot);r[2]=(r[4]+slot)<<4;r[2]+=r[7];battle_entry_write(entry,r[2]+0x910u,2,r[10]);
            r[2]=0x800d0000u;slot=battle_entry_read(entry,r[2]+0xffffcb34u,4);uint32_t glyph=battle_entry_read(entry,r[5],1);if(slot!=expected_slot)battle_entry_fail(entry,"cursor slot changed",slot);r[2]=slot+r[4];r[2]<<=4;r[2]+=r[7];r[3]=glyph+0x5eu;battle_entry_write(entry,r[2]+0x912u,2,r[3]);
            r[2]=0x800d0000u;slot=battle_entry_read(entry,r[2]+0xffffcb34u,4);r[3]=0x12u;if(slot!=expected_slot)battle_entry_fail(entry,"cursor slot changed",slot);r[2]=r[4]+slot;r[2]<<=4;r[2]+=r[7];battle_entry_write(entry,r[2]+0x914u,2,r[3]);
            r[3]=0x800d0000u;slot=battle_entry_read(entry,r[3]+0xffffcb34u,4);glyph=battle_entry_read(entry,r[5],1);r[3]=slot;if(slot!=expected_slot)battle_entry_fail(entry,"cursor slot changed",slot);r[4]+=r[3];r[4]<<=4;r[4]+=r[7];r[2]=glyph+0x5eu;battle_entry_write(entry,r[4]+0x916u,2,r[2]);
            int repeat=(int32_t)r[6]<(int32_t)r[9];r[2]=repeat;r[5]++;if(!repeat)break;
        }while(1);
    }
    r[2]=0x800d0000u;r[2]=battle_entry_read(entry,r[2]+0x2d28u,4);if(r[2]!=expected_frame)battle_entry_fail(entry,"cursor frame changed",r[2]);battle_entry_write(entry,r[2]+0x97u,1,r[8]);
    r[3]=0x800d0000u;uint32_t frame=battle_entry_read(entry,r[3]+0x2d28u,4);r[2]=0x800d0000u;r[3]=frame;if(frame!=expected_frame)battle_entry_fail(entry,"cursor frame changed",frame);r[2]=battle_entry_read(entry,r[2]+0xffffcb34u,1);
    battle_entry_write(entry,r[3]+0x98u,1,r[2]);r[29]+=8u;
}

/* Two through seven options, slots zero or one, and safe disjoint inputs. */
static int battle_cursor_layout_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    uint32_t count=cpu->gpr[4]&0xffu;if(target!=0x8007fbe0u || cpu->gpr[31]!=0x800806f0u || count<2u || count>7u || runtime->force_interpret ||
       !runtime->initialized || g_ActiveBattleRuntime!=runtime || runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_CURSOR_LAYOUT_EXPERIMENT");if(!control || strcmp(control,"1")!=0)return 0;
    uint8_t *code=resolve_memory(runtime,target,264u,0);if(!code)return 0;static const char expected[]="617d0424dd52399ef39d24a3934b662acb61c2b440844fc1f3542fa60fff8c5e";
    uint8_t digest[32];file1_digest(code,264u,digest);for(unsigned i=0;i<sizeof(digest);i++)if(file1_hex_digit(digest[i]>>4)!=expected[2*i] || file1_hex_digit(digest[i]&15u)!=expected[2*i+1])return 0;
    uint8_t *protect[10];unsigned widths[10],n=0;protect[n]=code;widths[n++]=264;
    const uint32_t globals[]={0x800d2d28u,0x800c3ea4u,0x800ccb34u};uint32_t frame,work,slot;
    for(unsigned i=0;i<3;i++){protect[n]=resolve_memory(runtime,globals[i],4,0);if(!protect[n])return 0;widths[n++]=4;}
    if(runtime_read(runtime,globals[0],4,&frame)!=0 || runtime_read(runtime,globals[1],4,&work)!=0 || runtime_read(runtime,globals[2],4,&slot)!=0 || slot>1u || frame<BATTLE_BUS_KUSEG_RAM_END || frame>UINT32_MAX-0x98u || work<BATTLE_BUS_KUSEG_RAM_END || (work&1u))return 0;
    uint32_t last=0x916u+(((count-2u)*2u+slot)<<4);if(work>UINT32_MAX-last-1u)return 0;
    for(unsigned i=0;i<count-1u;i++){protect[n]=resolve_memory(runtime,0x800c3202u+count*6u+i,1,0);if(!protect[n])return 0;widths[n++]=1;}
    for(unsigned i=0;i<count-1u;i++)for(unsigned j=0;j<4;j++){uint32_t addr=work+0x910u+((i*2u+slot)<<4)+j*2u;uint8_t *out=resolve_memory(runtime,addr,2,1);if(!out)return 0;uintptr_t a=(uintptr_t)out;
        for(unsigned q=0;q<n;q++){uintptr_t b=(uintptr_t)protect[q];if(a<b+widths[q] && b<a+2u)return 0;}}
    for(unsigned i=0;i<2;i++){uint8_t *out=resolve_memory(runtime,frame+0x97u+i,1,1);if(!out)return 0;uintptr_t a=(uintptr_t)out;
        for(unsigned q=0;q<n;q++){uintptr_t b=(uintptr_t)protect[q];if(a<b+widths[q] && b<a+1u)return 0;}}
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_cursor_layout_run(&entry,work,frame,slot);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
