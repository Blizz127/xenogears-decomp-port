/* Complete battle status scan and its three live-table mask helpers.
 * These are compiled bodies; no original CPU instructions are decoded. */
#define SS_R(a) battle_entry_read(entry,(a),4)
#define SS_H(a) battle_entry_read(entry,(a),2)
#define SS_B(a) battle_entry_read(entry,(a),1)
#define SS_W(a,v) battle_entry_write(entry,(a),4,(v))
#define SS_SH(a,v) battle_entry_write(entry,(a),2,(v))
#define SS_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define SS_AT (entry->cpu->gpr[1])
#define SS_V0 (entry->cpu->gpr[2])
#define SS_V1 (entry->cpu->gpr[3])
#define SS_A0 (entry->cpu->gpr[4])
#define SS_A1 (entry->cpu->gpr[5])
#define SS_S0 (entry->cpu->gpr[16])
#define SS_S1 (entry->cpu->gpr[17])
#define SS_S2 (entry->cpu->gpr[18])
#define SS_S3 (entry->cpu->gpr[19])
#define SS_S4 (entry->cpu->gpr[20])
#define SS_S5 (entry->cpu->gpr[21])
#define SS_S6 (entry->cpu->gpr[22])
#define SS_SP (entry->cpu->gpr[29])
#define SS_RA (entry->cpu->gpr[31])

static void battle_status_mask_run(BattleEntryCall *entry,uint32_t target)
{
    if(target==0x80089c9cu) {
        SS_A1&=0xffu;SS_V0=SS_A1<16u;int valid=SS_V0!=0;SS_V0=SS_A1<<1;
        if(valid) {SS_AT=0x800c0000u+SS_V0;SS_V0=SS_H(SS_AT+0x3448u);SS_V0&=SS_A0;}
        else SS_V0=0;
    } else {
        SS_A0&=0xffu;SS_A0<<=1;SS_AT=0x800c0000u+SS_A0;
        SS_V0=SS_H(SS_AT+0x3448u);
        if(target==0x80089c48u){SS_V0=~SS_V0;SS_V0&=0xffffu;}
    }
}

static void battle_status_scan_call(BattleEntryCall *entry,uint32_t target,uint32_t site)
{
    SS_RA=site+8u;PcPortMipsCpu callee=*entry->cpu;
    cpu_meter_mark_entry(&callee,target);
    int rc=PcPortMipsRun(&callee,target,site+8u,battle_step_limit());
    memcpy(entry->cpu->gpr,callee.gpr,sizeof(callee.gpr));
    memcpy(entry->cpu->cp0,callee.cp0,sizeof(callee.cp0));
    entry->cpu->hi=callee.hi;entry->cpu->lo=callee.lo;entry->cpu->steps=callee.steps;
    if(rc!=PC_PORT_MIPS_HALTED) {
        fprintf(stderr,"[xeno-port][battle-status-scan] callee 0x%08x from 0x%08x: %s\n",target,site,callee.error);
        battle_entry_fail(entry,"status mask helper stopped",target);
    }
    if(callee.load_valid&&callee.load_reg!=0)
        battle_entry_fail(entry,"pending status mask helper return",target);
}

static void battle_status_scan_run(BattleEntryCall *entry)
{
    SS_SP-=0x30u;SS_W(SS_SP+0x14u,SS_S1);SS_S1=0;
    SS_W(SS_SP+0x18u,SS_S2);SS_S2=0;SS_W(SS_SP+0x10u,SS_S0);SS_S0=0;
    SS_W(SS_SP+0x2cu,SS_RA);SS_W(SS_SP+0x28u,SS_S6);SS_W(SS_SP+0x24u,SS_S5);
    SS_W(SS_SP+0x20u,SS_S4);SS_W(SS_SP+0x1cu,SS_S3);
    SS_AT=0x800d0000u;SS_SH(0x800d39dcu,0);
    do {
        SS_AT=0x800d0000u+SS_S1;SS_V0=SS_B(0x800d2dccu+SS_S1);
        if(SS_V0) {
            SS_AT=0x800d0000u+SS_S0;SS_V1=SS_H(0x800ccd64u+SS_S0);
            SS_V0=SS_V1&0xc000u;int blocked=SS_V0!=0;SS_V0=SS_V1&0x8000u;
            if(blocked) {
                SS_S2+=0x1cu;
                if(SS_V0){SS_AT=0x800d0000u+SS_S0;SS_SH(0x800ccd34u+SS_S0,0);}
                SS_V0=0xffu;SS_AT=0x800d0000u+SS_S1;SS_SB(0x800d2de4u+SS_S1,SS_V0);
                SS_S1++;goto party_next;
            }
            SS_AT=0x800c0000u+SS_S2;SS_V0=SS_B(0x800c3eb7u+SS_S2);
            if(!SS_V0) {
                SS_A0=SS_S1&0xffu;battle_status_scan_call(entry,0x80089c08u,0x800725dcu);
                SS_V1=0x800d0000u;SS_V1=SS_H(0x800d39dcu);SS_V1|=SS_V0;
                SS_AT=0x800d0000u;SS_SH(0x800d39dcu,SS_V1);
            }
        }
        SS_S2+=0x1cu;SS_S1++;
party_next:
        SS_V0=(int32_t)SS_S1<3;SS_S0+=0x170u;
    } while(SS_V0);
    SS_S1=3;SS_S6=0xffu;SS_S5=0x800c3d1bu;SS_S4=0;SS_S2=0x450u;SS_S3=0x54u;
    do {
        SS_AT=0x800d0000u+SS_S4;SS_V0=SS_B(0x800d2dcfu+SS_S4);
        if(!SS_V0)goto enemy_advance;
        SS_AT=0x800c0000u+SS_S3;SS_V0=SS_B(0x800c3eb8u+SS_S3);
        if(!SS_V0) {
            SS_AT=0x800d0000u+SS_S2;SS_V0=SS_H(0x800ccd64u+SS_S2);SS_V0&=0xc000u;SS_S0=SS_S1&0xffu;
            if(SS_V0) {
                SS_A0=0x800c0000u;SS_A0=SS_H(0x800c3608u);SS_A1=SS_S0;
                battle_status_scan_call(entry,0x80089c9cu,0x80072680u);
                SS_V0&=0xffffu;SS_A0=SS_S0;if(SS_V0)goto enemy_available;
                SS_AT=0x800d0000u+SS_S2;SS_SH(0x800ccd34u+SS_S2,0);
                SS_AT=0x800d0000u+SS_S4;SS_SB(0x800d2de7u+SS_S4,SS_S6);
                SS_S5+=4u;goto enemy_next;
            }
        } else {
            SS_AT=0x800d0000u+SS_S2;SS_V0=SS_H(0x800cce08u+SS_S2);SS_V0&=0xc000u;SS_S0=SS_S1&0xffu;
            if(SS_V0) {
                SS_A0=0x800c0000u;SS_A0=SS_H(0x800c3608u);SS_A1=SS_S0;
                battle_status_scan_call(entry,0x80089c9cu,0x800726d8u);
                SS_V0&=0xffffu;SS_A0=SS_S0;if(SS_V0)goto enemy_available;
                SS_AT=0x800d0000u+SS_S2;SS_W(0x800ccdecu+SS_S2,0);
                SS_AT=0x800d0000u+SS_S4;SS_SB(0x800d2de7u+SS_S4,SS_S6);
                SS_S5+=4u;goto enemy_next;
            }
        }
        SS_AT=0x800c0000u+SS_S3;SS_V0=SS_B(0x800c3eb7u+SS_S3);
        if(SS_V0)goto enemy_advance;
        SS_V0=SS_B(SS_S5);int unavailable=SS_V0!=0;SS_A0=SS_S1&0xffu;
        if(unavailable)goto enemy_advance;
enemy_available:
        battle_status_scan_call(entry,0x80089c08u,0x80072734u);
        SS_V1=0x800d0000u;SS_V1=SS_H(0x800d39dcu);SS_V1|=SS_V0;
        SS_AT=0x800d0000u;SS_SH(0x800d39dcu,SS_V1);
enemy_advance:
        SS_S5+=4u;
enemy_next:
        SS_S4++;SS_S2+=0x170u;SS_S1++;SS_V0=(int32_t)SS_S1<11;SS_S3+=0x1cu;
    } while(SS_V0);
    SS_V0=0x800d0000u;SS_V0=SS_H(0x800d39dcu);SS_V0&=0x7f8u;
    int enemies=SS_V0!=0;SS_V0=1;
    if(!enemies){SS_AT=0x800c0000u;SS_SB(0x800c48eau,SS_V0);}
    SS_V0=0x800d0000u;SS_V0=SS_H(0x800d39dcu);SS_V0&=7u;
    int party=SS_V0!=0;SS_V0=0x81u;
    if(!party){SS_AT=0x800c0000u;SS_SB(0x800c48eau,SS_V0);}
    SS_V0=0x800c0000u;SS_V0=SS_B(0x800c48eau);SS_S1=0;
    if(!SS_V0) {
        SS_S2=0x800d0000u;
        uint32_t mask=SS_H(0x800d39dcu);SS_S3=0;SS_S2=mask;
        do {
            SS_A0=SS_S2;SS_S0=SS_S1&0xffu;SS_A1=SS_S0;
            battle_status_scan_call(entry,0x80089c9cu,0x800727d8u);
            SS_V0&=0xffffu;int selected=SS_V0!=0;SS_S1++;
            if(selected) {
                SS_AT=0x800d0000u+SS_S3;SS_V0=SS_H(0x800ccd68u+SS_S3);SS_V0&=0x1000u;
                int sleeping=SS_V0!=0;SS_V0=(int32_t)SS_S1<11;
                if(sleeping) {
                    SS_A0=SS_S0;battle_status_scan_call(entry,0x80089c48u,0x80072808u);
                    SS_V0&=0xffffu;SS_S2&=SS_V0;SS_V0=(int32_t)SS_S1<11;
                }
            } else SS_V0=(int32_t)SS_S1<11;
            SS_S3+=0x170u;
        } while(SS_V0);
        int remaining=SS_S2!=0;SS_S1=0;
        if(!remaining) {
            SS_S0=0;
            do {
                SS_A0=0x800d0000u;SS_A0=SS_H(0x800d39dcu);SS_A1=SS_S1&0xffu;
                battle_status_scan_call(entry,0x80089c9cu,0x80072838u);SS_V0&=0xffffu;
                if(SS_V0) {
                    SS_AT=0x800d0000u+SS_S0;SS_V1=SS_H(0x800ccd68u+SS_S0);SS_V0=SS_V1&0x1000u;
                    int sleeping=SS_V0!=0;SS_V0=SS_V1&0xefffu;
                    if(sleeping){SS_AT=0x800d0000u+SS_S0;SS_SH(0x800ccd68u+SS_S0,SS_V0);break;}
                }
                SS_S1++;SS_V0=(int32_t)SS_S1<11;SS_S0+=0x170u;
            } while(SS_V0);
        }
    }
    uint32_t ra=SS_R(SS_SP+0x2cu),s6=SS_R(SS_SP+0x28u);SS_RA=ra;
    uint32_t s5=SS_R(SS_SP+0x24u);SS_S6=s6;
    uint32_t s4=SS_R(SS_SP+0x20u);SS_S5=s5;
    uint32_t s3=SS_R(SS_SP+0x1cu);SS_S4=s4;
    uint32_t s2=SS_R(SS_SP+0x18u);SS_S3=s3;
    uint32_t s1=SS_R(SS_SP+0x14u);SS_S2=s2;
    uint32_t s0=SS_R(SS_SP+0x10u);SS_S1=s1;SS_SP+=0x30u;SS_S0=s0;
}

static int battle_status_scan_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if((target!=0x8007252cu&&target!=0x80089c08u&&target!=0x80089c48u&&target!=0x80089c9cu)||
       runtime->force_interpret||!runtime->initialized||g_ActiveBattleRuntime!=runtime||
       runtime->bridge_cpu!=cpu||(cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_STATUS_SCAN");if(control&&strcmp(control,"0")==0)return 0;
    static const struct {uint32_t address,bytes;const char *sha;} bodies[]={
        {0x8007252cu,908u,"00dee0919b05786d384471d7a10c6255872165986ab10bcaf1d53491e702a41c"},
        {0x80089c08u,28u,"74c04258a3cf89081e9e82cef4ed802aff2c03b830a9fc7bb204291532cb45c0"},
        {0x80089c48u,36u,"26147e029cf1e4b28352108e9b27c29001c27d922d5ddb9e813d0a0fab0e9bcf"},
        {0x80089c9cu,48u,"33fdf72ffb138a88c0e99fa9bf1805a931dd890167a8d071be06ff6eab525690"}
    };
    unsigned which=0;while(bodies[which].address!=target)which++;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),bodies[which].bytes,digest);
    for(unsigned i=0;i<sizeof(digest);i++)
        if(file1_hex_digit(digest[i]>>4)!=bodies[which].sha[i*2]||file1_hex_digit(digest[i]&15u)!=bodies[which].sha[i*2+1])return 0;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    if(target==0x8007252cu)battle_status_scan_run(&entry);else battle_status_mask_run(&entry,target);
    cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef SS_R
#undef SS_H
#undef SS_B
#undef SS_W
#undef SS_SH
#undef SS_SB
#undef SS_AT
#undef SS_V0
#undef SS_V1
#undef SS_A0
#undef SS_A1
#undef SS_S0
#undef SS_S1
#undef SS_S2
#undef SS_S3
#undef SS_S4
#undef SS_S5
#undef SS_S6
#undef SS_SP
#undef SS_RA
