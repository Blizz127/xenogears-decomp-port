/* Complete startup optional-resource table binding, 801E7EC4..801E7F4C.
 * Address words stay canonical. Both table and count are live loop inputs. */
#define RBD_R(a) battle_entry_read(entry,(a),4)
#define RBD_W(a,v) battle_entry_write(entry,(a),4,(v))
#define RBD_SB(a,v) battle_entry_write(entry,(a),1,(v))
#define RBD_AT (entry->cpu->gpr[1])
#define RBD_V0 (entry->cpu->gpr[2])
#define RBD_V1 (entry->cpu->gpr[3])
#define RBD_A0 (entry->cpu->gpr[4])
#define RBD_A1 (entry->cpu->gpr[5])
#define RBD_A2 (entry->cpu->gpr[6])
#define RBD_SP (entry->cpu->gpr[29])

static void battle_resource_bind_run(BattleEntryCall *entry)
{
    RBD_V0=1;RBD_AT=0x800d0000u;RBD_W(0x800d3344u,RBD_A0);
    RBD_AT=0x800d0000u;RBD_W(0x800d39ccu,RBD_A1);
    RBD_AT=0x800d0000u;RBD_W(0x800d3348u,RBD_A2);
    RBD_AT=0x800d0000u;RBD_SB(0x800d2f64u,RBD_V0);
    RBD_SP-=8u;
    if(RBD_A1) {
        RBD_V1=0;
        if((int32_t)RBD_A2>0) {
            RBD_A0=0;
            do {
                RBD_V0=0x800d0000u;
                uint32_t table=RBD_R(0x800d39ccu);
                RBD_V1++;RBD_V0=RBD_A0+table;
                RBD_SB(RBD_V0+13u,0);
                RBD_V0=0x800d0000u;RBD_V0=RBD_R(0x800d3348u);
                RBD_V0=(int32_t)RBD_V1<(int32_t)RBD_V0;
                RBD_A0+=14u;
            } while(RBD_V0);
        }
    }
    if(!RBD_A2) {
        RBD_AT=0x800d0000u;RBD_W(0x800d3344u,0);
        RBD_AT=0x800d0000u;RBD_W(0x800d39ccu,0);
    }
    RBD_SP+=8u;
}

static int battle_resource_bind_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if(target!=0x801e7ec4u||runtime->force_interpret||!runtime->initialized||
       g_ActiveBattleRuntime!=runtime||runtime->bridge_cpu!=cpu||
       (cpu->load_valid&&cpu->load_reg!=0))return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_RESOURCE_BIND");
    if(control&&strcmp(control,"0")==0)return 0;
    static const struct {uint32_t address,bytes;const char *sha;} regions[]={
        {0x801e7ec4u,136u,"45a9f8e1446f80ddabfa3bdb465dd076f5f197913a13e41e5a661e590484c470"},
        {0x801e4000u,32u,"6fc819dea0b8d42dac0ba2f1b42420a7fdf7bf6f99bc354b391b4e9a58905140"},
        {0x801e5840u,228u,"e2327a010fef1500ce2d6db7269aded468d84706732157b66a53d443c65194b2"}
    };
    for(unsigned r=0;r<sizeof(regions)/sizeof(regions[0]);r++) {
        uint8_t digest[32];file1_digest(PSX_ADDR(regions[r].address),regions[r].bytes,digest);
        for(unsigned i=0;i<sizeof(digest);i++)
            if(file1_hex_digit(digest[i]>>4)!=regions[r].sha[i*2]||file1_hex_digit(digest[i]&15u)!=regions[r].sha[i*2+1])return 0;
    }
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};if(setjmp(entry.failure)!=0)return -1;
    battle_resource_bind_run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}

#undef RBD_R
#undef RBD_W
#undef RBD_SB
#undef RBD_AT
#undef RBD_V0
#undef RBD_V1
#undef RBD_A0
#undef RBD_A1
#undef RBD_A2
#undef RBD_SP
