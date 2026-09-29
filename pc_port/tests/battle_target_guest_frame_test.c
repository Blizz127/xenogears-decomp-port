/* Retail-only alias contract. Split-stack mode is an intentionally incorrect
 * memory model, not a native implementation or accepted runtime fallback. */
#include "battle_mips_adapter.h"
#include "battle_target_list_ram.h"
#include "battle_target_selection_ram.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
enum { SP=0x1FF000, FRAME=SP-0x50 };
static uint8_t ram[0x200000], shadow[0x50];
static uint8_t expected_ram[sizeof(ram)];
static unsigned split, saves;
static PcPortMipsCpu cpu;
static const unsigned save_offsets[]={0x48,0x38,0x4C,0x44,0x40,0x3C};
static const unsigned save_regs[]={20,16,31,19,18,17};
static uint32_t initial[32];
static uint32_t load(const uint8_t *p, unsigned width) {
    uint32_t value=0;
    for (unsigned i=0; i<width; i++) value|=(uint32_t)p[i]<<(8*i);
    return value;
}
static void put(uint8_t *p, unsigned width, uint32_t value) {
    for (unsigned i=0; i<width; i++) p[i]=value>>(8*i);
}
static int read_bus(void *unused, uint32_t address, unsigned width, uint32_t *value) {
    (void)unused;
    unsigned at=address&0x1FFFFFFF;
    if ((uint64_t)at+width>sizeof(ram)) return -1;
    /* Only the list sees its shadow frame; eligibility sees authoritative RAM.
     * This models the visibility failure of substituting host-local storage. */
    int eligibility=cpu.pc>=0x80083FF4u && cpu.pc<0x80084108u;
    if (split && !eligibility && at>=FRAME && at+width<=SP)
        *value=load(shadow+at-FRAME,width);
    else *value=load(ram+at,width);
    return 0;
}
static int write_bus(void *unused, uint32_t address, unsigned width, uint32_t value) {
    (void)unused;
    unsigned at=address&0x1FFFFFFF;
    if ((uint64_t)at+width>sizeof(ram)) return -1;
    if (saves<6) {
        assert(at==FRAME+save_offsets[saves] && width==4);
        assert(value==initial[save_regs[saves]]);
        saves++;
    }
    if (at>=FRAME && at+width<=SP) {
        assert((width==1 && ((at>=FRAME+0x10 && at<FRAME+0x1C) ||
                            (at>=FRAME+0x20 && at<FRAME+0x2C))) ||
               (width==4 && at>=FRAME+0x38 && at<=FRAME+0x4C));
        put(split ? shadow+at-FRAME : ram+at,width,value);
    } else {
        assert(width==1 && (at==0xD3274 || (at>=0xC3E90 && at<0xC3E9C)));
        put(ram+at,width,value);
    }
    return 0;
}
static unsigned run(unsigned slot, unsigned alias, unsigned split_mode) {
    memset(ram,0,sizeof(ram));
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x14504,SEEK_SET));
    assert(fread(ram+0x83FF4,1,0x114,f)==0x114);
    assert(!fseek(f,0x146F0,SEEK_SET));
    assert(fread(ram+0x841E0,1,0x368,f)==0x368); fclose(f);
    unsigned fill=slot==0x10 ? 0 : 0xA5;
    memset(ram+FRAME,fill,0x50); memcpy(shadow,ram+FRAME,sizeof(shadow));
    for (unsigned i=3; i<11; i++) ram[0xD2DCC+i]=1;
    put(ram+0xD3364,4,(alias ? 0xA0000000u : 0x80000000u)+FRAME+slot-0x140);
    PcPortMipsBus bus={.read=read_bus,.write=write_bus};
    PcPortMipsCpuInit(&cpu,&bus);
    for (unsigned i=0; i<32; i++) initial[i]=0x12340000u+i*0x100;
    initial[29]=0x80000000u+SP; initial[31]=0xFFFFFFFC;
    memcpy(cpu.gpr,initial,sizeof(initial)); cpu.gpr[0]=0; cpu.gpr[4]=0;
    split=split_mode==1; saves=0;
    if (split_mode==2) assert(PcPortBattleTargetListRam(ram,sizeof(ram),&cpu)==0);
    else {
        assert(PcPortMipsRun(&cpu,0x800841E0,0xFFFFFFFC,10000)==0);
        assert(saves==6);
    }
    assert(cpu.gpr[29]==initial[29] && cpu.gpr[31]==initial[31]);
    for (unsigned i=16; i<=20; i++) assert(cpu.gpr[i]==initial[i]);
    unsigned count=ram[0xD3274];
    assert(cpu.gpr[2]==(count ? 3u : 255u));
    for (unsigned i=0; i<12; i++) assert(ram[0xC3E90+i]==(i<count ? i+3 : 255));
    const uint8_t *frame=split ? shadow : ram+FRAME;
    for (unsigned i=0; i<0x50; i++)
        if (i<0x10 || (i>=0x1C && i<0x20) || (i>=0x2C && i<0x38))
            assert(frame[i]==fill);
    if (split_mode==0) memcpy(expected_ram,ram,sizeof(ram));
    if (split_mode==2) assert(memcmp(expected_ram,ram,sizeof(ram))==0);
    return count;
}
static int strict_read(void *unused, uint32_t address, unsigned width, uint32_t *value) {
    (void)unused;
    if (!((address>=0x80000000u && address<0x80200000u) ||
          (address>=0xA0000000u && address<0xA0200000u))) return -1;
    unsigned at=address&0x1FFFFFFF;
    if ((uint64_t)at+width>sizeof(ram) || address%width) return -1;
    *value=load(ram+at,width); return 0;
}
static int strict_write(void *unused, uint32_t address, unsigned width, uint32_t value) {
    uint32_t ignored;
    if (strict_read(unused,address,width,&ignored)) return -1;
    put(ram+(address&0x1FFFFFFF),width,value); return 0;
}
static void failure_prefix(void) {
    memset(ram,0,sizeof(ram));
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x146F0,SEEK_SET));
    assert(fread(ram+0x841E0,1,0x368,f)==0x368); fclose(f);
    ram[0xD3274]=0xA5;
    PcPortMipsBus bus={.read=strict_read,.write=strict_write};
    PcPortMipsCpuInit(&cpu,&bus);
    cpu.gpr[29]=0x80000020; cpu.gpr[31]=0xFFFFFFFC;
    PcPortMipsCpu native=cpu;
    /* Save original RAM separately from the expected failed retail prefix. */
    static uint8_t before[sizeof(ram)]; memcpy(before,ram,sizeof(ram));
    assert(PcPortMipsRun(&cpu,0x800841E0,0xFFFFFFFC,10000)!=0);
    assert(strstr(cpu.error,"write8 fault at 0x7fffffe0"));
    assert(ram[0xD3274]==0);
    memcpy(expected_ram,ram,sizeof(ram)); memcpy(ram,before,sizeof(ram));
    assert(PcPortBattleTargetListRam(ram,sizeof(ram),&native)==-1);
    assert(ram[0xD3274]==0);
    assert(memcmp(expected_ram,ram,sizeof(ram))==0);
    assert(native.gpr[29]==cpu.gpr[29]);
    puts("GUEST FRAME invalid-scratch retail/native failure-prefix PASS");
}
static void broad_list_cases(void) {
    static uint8_t before[sizeof(ram)];
    uint8_t eligibility_code[0x114], list_code[0x368], selection_code[0xC4];
    FILE *f=fopen("disc/battle.bin","rb"); assert(f);
    assert(!fseek(f,0x14504,SEEK_SET));
    assert(fread(eligibility_code,1,sizeof(eligibility_code),f)==sizeof(eligibility_code));
    assert(!fseek(f,0x146F0,SEEK_SET));
    assert(fread(list_code,1,sizeof(list_code),f)==sizeof(list_code));
    assert(!fseek(f,0x14F8C,SEEK_SET));
    assert(fread(selection_code,1,sizeof(selection_code),f)==sizeof(selection_code)); fclose(f);
    const unsigned actors[]={0,2,3,255,0x101};
    unsigned cases=0;
    for (unsigned selection=2; selection-- > 0;)
    for (unsigned ai=0; ai<5; ai++) for (unsigned mask=0; mask<256; mask++)
    for (unsigned mode=0; mode<4; mode++) for (unsigned rank=0; rank<4; rank++) {
        memset(ram,0,sizeof(ram));
        memcpy(ram+0x83FF4,eligibility_code,sizeof(eligibility_code));
        memcpy(ram+0x841E0,list_code,sizeof(list_code));
        memcpy(ram+0x84A7C,selection_code,sizeof(selection_code));
        if (selection) {
            for (unsigned i=0; i<0x4100; i++) ram[0x110000+i]=(i*73+mask)&255;
            for (unsigned i=0; i<0x4100; i++) ram[0x120000+i]=(i*91+mask)&255;
            put(ram+0xC3EAC,4,0x80110000);
            ram[0x11003C+(actors[ai]&255)*64]=(mask+rank)&255;
            ram[0x12003C+(actors[ai]&255)*64]=rank==0 ? 7 : 255;
        }
        memset(ram+FRAME,0xA5,0x50);
        for (unsigned i=0; i<256; i++)
            ram[0xC3EB4+i*28]=mode==0 ? 0 : mode==1 ? i%3 :
                                  mode==2 ? (i/3)%3 : (i*i+mask)%4;
        unsigned actor_group=ram[0xC3EB4+(actors[ai]&255)*28];
        put(ram+0xD3364,4,0x80100000);
        for (unsigned i=0; i<11; i++) {
            unsigned bit=(mask>>(i%8))&1;
            ram[0xD2DCC+i]=mode==0 ? bit : 1;
            ram[0xC3EB7+i*28]=mode==1 ? !bit : 0;
            ram[0xD32A1+i*8]=mode==2 ? i&1 : 0;
            unsigned status=mode==2 && !bit ? 0x8000 : 0;
            put(ram+0xCCD64+i*0x170,2,status);
            put(ram+0xCCE08+i*0x170,2,status);
            unsigned value=rank==0 ? i*6000 : rank==1 ? 65535-i*6000 :
                           rank==2 ? 0x8000 : (i&1 ? 0xFFFF : 0);
            put(ram+0xCCD34+i*0x170,2,value);
        }
        if (mode==3) for (unsigned group=0; group<4; group++)
            ram[0x100140+actor_group*64+group*8]=(mask>>group)&1;
        PcPortMipsBus bus={.read=strict_read,.write=strict_write};
        PcPortMipsCpuInit(&cpu,&bus);
        for (unsigned i=16; i<=23; i++) cpu.gpr[i]=0xAABB0000+i*257;
        int saved_overlap=!selection && ai==0 && mask==255 && mode==0 && rank==0;
        int owner_overlap=selection && ai==0 && mask==255 && mode==0 && rank<2;
        int found_overlap=selection && ai==0 && mask==0 && mode==0 && rank==0;
        int found_wrap=selection && ai==2 && mask==255 && mode==0 && rank==1;
        if (found_overlap) ram[0x11003C]=7;
        cpu.gpr[4]=actors[ai];
        cpu.gpr[29]=found_wrap ? 0x800C3ED8u : found_overlap ? 0x800C3ED0u : owner_overlap ? 0x800C3EB8u :
                   saved_overlap ? 0x800C3EA8u : 0x80000000u+SP;
        if (owner_overlap) cpu.gpr[17]=0x80120000; /* save overwrites packed owner */
        cpu.gpr[31]=0xFFFFFFFC;
        PcPortMipsCpu native=cpu;
        memcpy(before,ram,sizeof(ram));
        assert(PcPortMipsRun(&cpu,selection ? 0x80084A7C : 0x800841E0,0xFFFFFFFC,10000)==0);
        memcpy(expected_ram,ram,sizeof(ram)); memcpy(ram,before,sizeof(ram));
        int rc=selection ? PcPortBattleTargetSelectionRam(ram,sizeof(ram),&native) :
                           PcPortBattleTargetListRam(ram,sizeof(ram),&native);
        assert(rc==0);
        if (memcmp(expected_ram,ram,sizeof(ram)) || (!selection && native.gpr[2]!=cpu.gpr[2])) {
            fprintf(stderr,"PACKED LIST mismatch selection=%u actor=%u mask=%u mode=%u rank=%u\n",
                    selection,actors[ai],mask,mode,rank);
            assert(0);
        }
        for (unsigned i=16; i<=23; i++) assert(native.gpr[i]==cpu.gpr[i]);
        assert(native.gpr[29]==cpu.gpr[29] && native.gpr[31]==cpu.gpr[31]);
        if (saved_overlap) {
            assert(native.gpr[16]==0x06050403 && native.gpr[17]==0x0A090807);
            assert(native.gpr[18]==0xFFFFFFFF);
        }
        /* Nested s0 is list[8..11]=FFFFFFFF. Nested RA replaces the context
         * owner with80084AB0; its actor3 target at84BAC is zero. Matching zero
         * wraps s0 to zero, so fallback writes list[0]=2 at owner+2E8. */
        if (found_wrap) assert(ram[0x84D98]==2);
        cases++;
    }
    printf("PACKED LIST/SELECTION full-RAM differential PASS %u cases\n",cases);
}
int main(void) {
    broad_list_cases();
    failure_prefix();
    const unsigned slots[]={0x10,0x20,0x38};
    for (unsigned alias=0; alias<2; alias++) for (unsigned i=0; i<3; i++) {
        unsigned retail=run(slots[i],alias,0);
        assert(run(slots[i],alias,2)==retail);
        unsigned broken=run(slots[i],alias,1);
        assert(retail==(i ? 8u : 0u));
        assert(broken==(i ? 0u : 8u));
        printf("GUEST FRAME alias=%u slot=%02X retail=%u split=%u DIFFERENCE PROVEN\n",
               alias,slots[i],retail,broken);
    }
    /* A selected failure must preserve prior frame/list writes, not roll back
     * or silently restart guest execution. Invalid owner is reached lazily. */
    memset(ram,0,sizeof(ram));
    memset(ram+0xC3E90,0xA5,12);
    ram[0xD3274]=0xA5; ram[0xD2DCC+3]=1;
    memset(&cpu,0,sizeof(cpu));
    cpu.gpr[29]=0x80000000u+SP; cpu.gpr[31]=0x12345678;
    cpu.gpr[2]=0xDEADBEEF;
    assert(PcPortBattleTargetListRam(ram,sizeof(ram),&cpu)==-1);
    assert(cpu.gpr[29]==0x80000000u+FRAME && cpu.gpr[2]==0xDEADBEEF);
    assert(ram[0xD3274]==0);
    for (unsigned i=0; i<12; i++) assert(ram[0xC3E90+i]==255);
    assert(load(ram+FRAME+0x4C,4)==0x12345678);
    assert(PcPortBattleTargetListRam(NULL,sizeof(ram),&cpu)==-1);
    assert(PcPortBattleTargetListRam(ram,sizeof(ram),NULL)==-1);
    puts("GUEST FRAME native full-RAM parity and retained-failure-prefix PASS");
    puts("GUEST FRAME retail writes/restoration and six split-stack controls PASS");
}
