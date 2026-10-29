/* Fixed synthetic comparison of the actual host instrument leaf with retail
 * CPU execution. This does not prove live bank ownership or sound timing. */
#include "slus_oracle.h"
extern void func_8003E5BC(int, void *);
extern void SoundAssignVoiceToChannelAndStop(void *, uint32_t);
extern void SoundReleaseVoiceFromChannel(void *, uint32_t);
extern void func_8003EB5C(void);
extern void *g_pSoundSpuRegisters;
void *g_SoundChannels[24];
uint32_t g_SoundKeyOnFlags;
uint32_t g_SoundKeyOffFlags;
/* Place the retail-width flag immediately before an inaccessible page. */
__asm__(".pushsection .voice_flag_guard,\"aw\",@nobits\n"
        ".balign 4096\n.zero 4092\n"
        ".globl g_unk_VoicesNeedingProcessing\ng_unk_VoicesNeedingProcessing: .zero 4\n"
        ".globl voice_flag_guard_page\nvoice_flag_guard_page: .zero 4096\n.popsection\n");
extern int32_t g_unk_VoicesNeedingProcessing;
extern unsigned char voice_flag_guard_page[];
static const Glob voice_globals[] = {
    { "channel 22", &g_SoundChannels[22], 0x80062584u, 8, 1 },
    { "channel 23", &g_SoundChannels[23], 0x80062588u, 8, 1 },
    { "key on", &g_SoundKeyOnFlags, 0x800594fcu, 4, 0 },
    { "dirty voices", &g_unk_VoicesNeedingProcessing, 0x80059554u, 4, 0 },
};
static const Glob flush_globals[] = {
    { "processing", &g_unk_VoicesNeedingProcessing, 0x80059554u, 4, 0 },
    { "key off", &g_SoundKeyOffFlags, 0x80059550u, 4, 0 },
    { "SPU registers", &g_pSoundSpuRegisters, 0x800508e4u, 8, 1 },
};
static void setup(uint32_t which)
{
    uint8_t *element=lo(0x100),*bank=lo(0x400);
    memset(element,0xa5,0x158);memset(bank,0,0x100);put16(element,0x1234u);
    put32(element+0x2c,lo_addr(0x400));put32(bank+0x28,0x12340u);
    uint8_t *instrument=bank+0x30u+16u*(which-1u);
    put32(instrument,0x123u);put16(instrument+4u,0x45u);put16(instrument+6u,0x67u);
    put32(instrument+8u,0x15345678u);put16(instrument+0x0cu,0x765u);
    A[0]=which-1u;A[1]=lo_addr(0x100);
}
static uint32_t native_instrument(void) {func_8003E5BC((int)A[0],(void *)(uintptr_t)A[1]);return 0;}
static void setup_voice(uint32_t which)
{
    uint8_t *candidate=lo(0x100),*old=lo(0x200);
    memset(candidate,0x5a,0x20);memset(old,0xa5,0x20);
    put16(candidate+4,2);put16(old+4,which==3?5:1);
    if(which==6) {put16(candidate+4,0xfffdu);put16(old+4,0xfffcu);}
    if(which==7) {put16(candidate+4,0xfffcu);put16(old+4,0xfffdu);}
    g_SoundChannels[22]=which==1?candidate:which==2?NULL:old;
    g_SoundChannels[23]=lo(0x300);
    g_SoundKeyOnFlags=0xffffffffu;g_unk_VoicesNeedingProcessing=0;
    A[0]=lo_addr(0x100);A[1]=which==5?24u:22u;
}
static uint32_t native_assign(void) {SoundAssignVoiceToChannelAndStop((void *)(uintptr_t)A[0],A[1]);return 0;}
static void setup_release(uint32_t which)
{
    setup_voice(which);A[1]=which==4?24u:22u;
}
static uint32_t native_release(void) {SoundReleaseVoiceFromChannel((void *)(uintptr_t)A[0],A[1]);return 0;}
static void setup_flush(uint32_t which)
{
    memset(lo(0x400),0x5a,0x200);g_pSoundSpuRegisters=lo(0x400);
    g_unk_VoicesNeedingProcessing=which==2?(1u<<22):0;
    g_SoundKeyOffFlags=which==3?(1u<<4):0;
}
static uint32_t native_flush(void) {func_8003EB5C();return 0;}
int main(void)
{
    const Fn f={"func_8003E5BC",0x8003e5bcu,0,setup,native_instrument};
    cur_what="NATIVE SOUND INSTRUMENT";
    map_low();load_slus(cur_what);globs=NULL;nglobs=0;
    for(unsigned i=1;i<=2;i++) {
        if(run_case(&f,i,0))return 1;
        assert(lo(0x126)[0]==i-1u);
        assert(*(uint32_t *)lo(0x14c)==0x12c58u && *(uint32_t *)lo(0x150)==0xb40u);
        assert(*(uint16_t *)lo(0x100)==0x9234u && *(uint16_t *)lo(0x16c)==0x67u);
    }
    puts("native sound instrument: two fixed synthetic CPU comparisons PASS");
    const Fn voice={"SoundAssignVoiceToChannelAndStop",0x8003e724u,0,setup_voice,native_assign};
    globs=voice_globals;nglobs=sizeof(voice_globals)/sizeof(voice_globals[0]);
    assert(((uintptr_t)voice_flag_guard_page&4095u)==0);
    assert(mprotect(voice_flag_guard_page,4096,PROT_NONE)==0);
    for(unsigned i=1;i<=7;i++) {
        if(run_case(&voice,i,0))return 1;
        assert(g_SoundChannels[23]==lo(0x300));
        if(i==1||i==2||i==4||i==6)assert((uint32_t)g_unk_VoicesNeedingProcessing==(1u<<22));
        else assert(g_unk_VoicesNeedingProcessing==0);
        if(i==2||i==4||i==6) {
            assert(g_SoundChannels[22]==lo(0x100) && *(uint16_t *)lo(0x100)==22u);
            assert(*(uint16_t *)lo(0x106)==0xffffu && !(g_SoundKeyOnFlags&(1u<<22)));
        }
    }
    puts("native sound voice assignment: fixed CPU comparisons and 32-bit boundary PASS");
    const Fn release={"SoundReleaseVoiceFromChannel",0x8003e83cu,0,setup_release,native_release};
    for(unsigned i=1;i<=4;i++) {
        if(run_case(&release,i,0))return 1;
        assert(g_SoundChannels[23]==lo(0x300));
        if(i==1) {
            assert(g_SoundChannels[22]==NULL && g_unk_VoicesNeedingProcessing==(1u<<22));
            assert(!(g_SoundKeyOnFlags&(1u<<22)));
        } else assert(g_unk_VoicesNeedingProcessing==0 && g_SoundKeyOnFlags==0xffffffffu);
    }
    puts("native sound voice release: four fixed synthetic CPU comparisons PASS");
    const Fn flush={"func_8003EB5C",0x8003eb5cu,0,setup_flush,native_flush};
    globs=flush_globals;nglobs=sizeof(flush_globals)/sizeof(flush_globals[0]);
    for(unsigned i=1;i<=3;i++) {
        if(run_case(&flush,i,0))return 1;
        assert(g_unk_VoicesNeedingProcessing==0 && g_SoundKeyOffFlags==0);
    }
    assert(mprotect(voice_flag_guard_page,4096,PROT_READ|PROT_WRITE)==0);
    puts("native sound voice flush: three fixed synthetic CPU comparisons and 32-bit boundary PASS");
    return 0;
}
