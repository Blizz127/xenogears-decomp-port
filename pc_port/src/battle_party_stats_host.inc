/* Native party-stat initialization and the optional special-mode adjustment.
 * These are data algorithms over the live battle bus, with packed 32-bit
 * pointers. No original instruction fetch, decode or execution occurs here. */
#define BATTLE_PARTY_STATS_ADDRESS 0x80097d5cu
#define BATTLE_PARTY_STATS_BYTES 0xbd0u
#define BATTLE_PARTY_STATS_SHA256 \
    "6813bf2a7b6ea40846d956932bb1015fd19b3408c82d7ae907fb6c01f72355cf"
#define BATTLE_PARTY_SPECIAL_ADDRESS 0x8009b098u
#define BATTLE_PARTY_SPECIAL_BYTES 0x6cu
#define BATTLE_PARTY_SPECIAL_SHA256 \
    "a30035fef75e13f9887bb001b8551364c3f9b02818124c6dd5b19df256a5c28c"

#define PS_R8(a) battle_entry_read(entry,(a),1)
#define PS_R16(a) battle_entry_read(entry,(a),2)
#define PS_R32(a) battle_entry_read(entry,(a),4)
#define PS_W8(a,v) battle_entry_write(entry,(a),1,(v))
#define PS_W16(a,v) battle_entry_write(entry,(a),2,(v))
#define PS_W32(a,v) battle_entry_write(entry,(a),4,(v))
#define PS_RECORD PS_R32(0x800c3e00u)
#define PS_STATS PS_R32(0x800d2d6cu)
#define PS_BASE PS_R32(0x800c34b0u)
#define PS_GAME 0x8006d634u

static void battle_party_effect_costs(BattleEntryCall *entry,uint32_t slot,int halve)
{
    for (unsigned table=0;table<2;table++) {
        uint32_t offset=table ? 0x223bu+slot*0x690u : 0x106bu+slot*0x5f0u;
        unsigned count=table ? 42 : 38;
        for (unsigned j=0;j<count;j++) {
            uint32_t address=PS_BASE+offset+j*0x28u;
            uint32_t cost=PS_R8(address);
            PS_W8(address,halve ? (cost+1u)/2u : cost*2u);
        }
    }
}

static void battle_party_stats_run(BattleEntryCall *entry)
{
    uint32_t sp=entry->cpu->gpr[29]-0x20u;
    entry->cpu->gpr[29]=sp; /* Original leaf reserves a frame without storing in it. */
    for (uint32_t slot=0;slot<3;slot++) {
        if (PS_R8(0x800d2d24u+slot)==0x7fu) continue;
        uint32_t record=PS_BASE+slot*0x170u;
        uint32_t sub=record+0x148u;
        PS_W32(0x800c3e00u,record);
        PS_W32(0x800d2d6cu,record+0xa4u);
        if (PS_R8(0x800d2d24u+slot)==9) {
            PS_W8(PS_GAME+0x886u,9); PS_W8(record+0x56u,9);
        }
        if (PS_R8(0x800d2d24u+slot)==10) {
            record=PS_RECORD;
            PS_W8(PS_GAME+0x92au,10); PS_W8(record+0x56u,10);
        }

        record=PS_RECORD;
        uint32_t initial_attack=PS_R8(record+0x56u)==4 ?
            PS_R8(record+4u)+PS_R8(record+0x1cu) : PS_R8(record+0x58u)+PS_R8(record+4u);
        PS_W8(PS_BASE+slot*8u+0x1028u,initial_attack);
        record=PS_RECORD;
        PS_W8(PS_BASE+slot*8u+0x1029u,PS_R8(record+0x5eu));
        record=PS_RECORD;
        PS_W8(PS_BASE+slot*8u+0x102au,PS_R8(record+0x59u)+PS_R8(record+0x2du));
        const unsigned row_fields[]={0x5f,0x5b,0x5c,0x5a};
        for (unsigned j=0;j<4;j++) {
            record=PS_RECORD;
            PS_W8(PS_BASE+slot*8u+0x102bu+j,PS_R8(record+row_fields[j]));
        }
        record=PS_RECORD;
        uint32_t base=PS_BASE;
        PS_W32(base+slot*8u+0xfd0u,PS_R32(record+0x3cu));
        PS_W32(base+slot*8u+0xfd4u,PS_R32(record+0x40u));
        PS_W16(base+slot*4u+0x1000u,PS_R16(record+0x4eu));
        PS_W16(base+slot*4u+0x1002u,PS_R16(record+0x52u));

        PS_W8(record+0x58u,PS_R8(record+0x58u)+PS_R8(record+0x28u));
        uint32_t next_record=PS_RECORD;
        PS_W16(record+0x34u,0);
        record=next_record;
        PS_W8(record+0x59u,PS_R8(record+0x59u)+PS_R8(record+0x29u));
        const unsigned bonus_fields[]={0x5a,0x5b,0x5c,0x5e,0x5f};
        for (unsigned j=0;j<5;j++) {
            record=PS_RECORD;
            PS_W8(record+bonus_fields[j],PS_R8(record+bonus_fields[j])+PS_R8(record+bonus_fields[j]-0x30u));
        }
        record=PS_RECORD;
        if (PS_R8(record+0x5au)>16) PS_W8(record+0x5au,16);
        record=PS_RECORD;
        if (PS_R16(record+0x32u)&0x100u) {
            PS_W8(record+0x5eu,PS_R8(record+0x5eu)+(PS_R8(record+0x5eu)>>2));
            record=PS_RECORD;
            PS_W8(record+0x5fu,PS_R8(record+0x5fu)+(PS_R8(record+0x5fu)>>2));
        }

        record=PS_RECORD;
        /* Products precede all four stores; each halfword wraps before its
         * subsequent cap. Unsigned input bounds keep each product positive. */
        uint32_t hp_delta=PS_R16(record+0x4eu)*PS_R8(record+0x30u)/20u;
        uint32_t mp_product=PS_R16(record+0x52u)*PS_R8(record+0x31u);
        uint32_t mp_delta=mp_product/20u;
        uint64_t last_product=(uint64_t)mp_product*0x66666667u;
        entry->cpu->hi=(uint32_t)(last_product>>32);
        entry->cpu->lo=(uint32_t)last_product;
        PS_W16(record+0x4cu,PS_R16(record+0x4cu)+hp_delta);
        PS_W16(record+0x50u,PS_R16(record+0x50u)+mp_delta);
        PS_W16(record+0x4eu,PS_R16(record+0x4eu)+hp_delta);
        uint32_t current_hp=PS_R16(record+0x4cu);
        PS_W16(record+0x52u,PS_R16(record+0x52u)+mp_delta);
        if (current_hp>999) PS_W16(record+0x4cu,999);
        record=PS_RECORD;
        if (PS_R16(record+0x50u)>99) { PS_W16(record+0x50u,99); record=PS_RECORD; }
        if (PS_R16(record+0x4eu)>999) PS_W16(record+0x4eu,999);
        record=PS_RECORD;
        if (PS_R16(record+0x52u)>99) PS_W16(record+0x52u,99);

        base=PS_BASE+slot*0x170u;
        PS_W8(base+0x158u,5); PS_W8(base+0x159u,5); PS_W8(sub,0);
        uint32_t stats=PS_STATS;
        uint32_t attack=PS_R16(stats+0x70u)+PS_R16(stats+0x40u);
        uint32_t defense=PS_R16(stats+0x72u)+PS_R16(stats+0x42u);
        uint32_t extra=PS_R16(stats+0x44u);
        PS_W16(stats+0x70u,attack);
        uint32_t accuracy=PS_R8(stats+0x9cu);
        PS_W16(stats+0x72u,defense);
        accuracy+=PS_R8(stats+0x4cu); extra+=PS_R16(stats+0x46u);
        PS_W8(stats+0x9cu,accuracy);
        extra+=PS_R16(stats+0x68u);
        uint32_t next_stats=PS_STATS;
        PS_W16(stats+0x68u,extra);
        stats=next_stats;
        PS_W8(stats+0x9fu,PS_R8(stats+0x9fu)+PS_R8(stats+0x4du));
        stats=PS_STATS;
        PS_W8(stats+0x98u,PS_R8(stats+0x98u)+PS_R8(stats+0x4eu)-PS_R8(stats+0x4au));
        stats=PS_STATS;
        PS_W8(stats+0x9eu,PS_R8(stats+0x9eu)+PS_R8(stats+0x54u));
        stats=PS_STATS;
        uint32_t gear_bonus=PS_R8(stats+0x4fu);
        if (gear_bonus!=0 && PS_R8(PS_RECORD+0xa0u)!=0x12u) {
            /* Original multiplies wrap to 32 bits, then the first quotient
             * wraps to a halfword before rounding down to a multiple of ten. */
            uint32_t scaled=(PS_R32(stats+0x64u)/10u)*gear_bonus*2u;
            uint32_t row=PS_BASE+slot*0x690u+0x2814u;
            PS_W16(row,scaled/9u);
            uint32_t truncated=PS_R16(row);
            last_product=(uint64_t)truncated*0xcccccccdu;
            entry->cpu->hi=(uint32_t)(last_product>>32);
            entry->cpu->lo=(uint32_t)last_product;
            PS_W16(row,truncated/10u);
            PS_W16(row,PS_R16(row)*10u);
        }

        PS_W8(sub+1u,0);
        const unsigned element_masks[]={0x1c00,0x380,0x70};
        for (unsigned j=0;j<3;j++) {
            uint32_t flags=PS_GAME+0x16c4u+PS_R8(PS_RECORD+0x56u)*0x20u;
            if (PS_R16(flags)&element_masks[j]) PS_W8(sub+1u,j+1u);
        }
        stats=PS_STATS; PS_W8(stats+0x3fu,PS_R8(stats+0x74u));
        stats=PS_STATS; PS_W8(stats+0x3eu,PS_R8(stats+0x74u));
        stats=PS_STATS; PS_W8(stats+0x3fu,PS_R8(stats+0x3fu)+PS_R8(stats+0x56u));
        stats=PS_STATS; PS_W8(stats+0x3eu,PS_R8(stats+0x3eu)+PS_R8(stats+0x56u));
        if (PS_R16(PS_RECORD+0x8au)&0x2000u) battle_party_effect_costs(entry,slot,0);
        if (PS_R16(PS_RECORD+0x32u)&0x4000u) battle_party_effect_costs(entry,slot,1);
        record=PS_RECORD;
        if (PS_R8(record+0x62u)>=50) {
            uint32_t flags=PS_GAME+0x16c4u+PS_R8(record+0x56u)*0x20u;
            PS_W16(flags,PS_R16(flags)|8u);
        }
        record=PS_RECORD;
        if (PS_R8(record+0x56u)==7) {
            stats=PS_STATS;
            PS_W32(stats+0x60u,PS_R16(record+0x4cu)*50u);
            PS_W32(stats+0x64u,PS_R16(record+0x4eu)*50u);
            PS_W8(stats+0x3cu,PS_R8(record+0x58u));
            record=PS_RECORD; stats=PS_STATS;
            PS_W16(stats+0x70u,PS_R8(record+0x59u)*12u);
            PS_W16(stats+0x72u,PS_R8(record+0x5cu)*6u);
            PS_W8(stats+0x98u,PS_R8(record+0x5au));
            stats=PS_STATS;
            uint32_t bits=PS_R16(stats+0x7eu); record=PS_RECORD;
            PS_W16(stats+0x7eu,bits|0x3c4u);
            if (PS_R16(record+0x82u)&0x2000u) PS_W16(stats+0x7eu,bits|0x13c4u);
        }
        record=PS_RECORD;
        if (PS_R8(record+0xa0u)==0x0fu) {
            uint32_t flags=PS_GAME+0x16dau+PS_R8(record+0x56u)*0x20u;
            PS_W16(flags,PS_R16(flags)&0x8fffu);
            record=PS_RECORD;
        }
        if (PS_R8(record+0xa0u)==0x12u) {
            stats=PS_STATS;
            PS_W16(record+0x7au,0x238); PS_W16(stats+0x38u,0x26ac); PS_W16(stats+0x3au,0x26ac);
            PS_W8(sub+1u,0); PS_W16(PS_GAME+0x173au,0x8000);
        }
        if (PS_R16(PS_GAME+0x1930u)>=0xe7u) {
            uint32_t flag=PS_R8(PS_GAME+0x2355u);
            if (!(flag&0x80u)) {
                PS_W8(PS_GAME+0x89au,0x27); PS_W8(PS_GAME+0x834u,0x1e); PS_W8(PS_GAME+0x2355u,flag|0x80u);
            }
        }
    }
    PS_W8(0x800c34adu,3);
    for (uint32_t i=0;i<3;i++)
        if (PS_R8(0x800d2d24u+i)==0x7fu) PS_W8(0x800c34adu,PS_R8(0x800c34adu)-1u);
    uint32_t base=PS_BASE;
    for (uint32_t i=0;i<3;i++) {
        PS_W32(base+i*4u+0x5f54u,0); PS_W32(base+i*4u+0x5f60u,0);
    }
    entry->cpu->gpr[2]=3; entry->cpu->gpr[3]=base;
    entry->cpu->gpr[29]=sp+0x20u;
}

static void battle_party_special_run(BattleEntryCall *entry)
{
    uint32_t base=0;
    for (uint32_t i=1;i<3;i++) {
        base=PS_BASE;
        uint32_t record=base+i*0x170u;
        PS_W16(record+0x4cu,100); PS_W16(record+0x4eu,100);
        PS_W8(record+0x5eu,20); PS_W8(record+0x5fu,15);
        PS_W16(record+0x7au,0x1fbf); PS_W8(record+0x149u,0);
    }
    entry->cpu->gpr[2]=0; entry->cpu->gpr[3]=base;
}

#undef PS_R8
#undef PS_R16
#undef PS_R32
#undef PS_W8
#undef PS_W16
#undef PS_W32
#undef PS_RECORD
#undef PS_STATS
#undef PS_BASE
#undef PS_GAME

static int battle_party_stats_try_host(BattleMipsRuntime *runtime,PcPortMipsCpu *cpu,uint32_t target)
{
    if ((target!=BATTLE_PARTY_STATS_ADDRESS && target!=BATTLE_PARTY_SPECIAL_ADDRESS) ||
        runtime->force_interpret || !runtime->initialized || g_ActiveBattleRuntime!=runtime ||
        runtime->bridge_cpu!=cpu || (cpu->load_valid && cpu->load_reg!=0)) return 0;
    const char *control=getenv("XENO_BATTLE_NATIVE_PARTY_STATS");
    if (control && strcmp(control,"0")==0) return 0;
    unsigned bytes=target==BATTLE_PARTY_STATS_ADDRESS ? BATTLE_PARTY_STATS_BYTES : BATTLE_PARTY_SPECIAL_BYTES;
    const char *identity=target==BATTLE_PARTY_STATS_ADDRESS ? BATTLE_PARTY_STATS_SHA256 : BATTLE_PARTY_SPECIAL_SHA256;
    uint8_t digest[32];file1_digest(PSX_ADDR(target),bytes,digest);
    for (unsigned i=0;i<sizeof(digest);i++)
        if (file1_hex_digit(digest[i]>>4)!=identity[i*2] ||
            file1_hex_digit(digest[i]&15u)!=identity[i*2+1]) return 0;
    void (*volatile run)(BattleEntryCall *)=target==BATTLE_PARTY_STATS_ADDRESS ?
        battle_party_stats_run : battle_party_special_run;
    BattleEntryCall entry={.runtime=runtime,.cpu=cpu};
    if (setjmp(entry.failure)!=0) return -1;
    run(&entry);cpu_meter_note_kind(CPU_METER_KIND_GENERIC);return 1;
}
