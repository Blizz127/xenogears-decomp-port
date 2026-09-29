/* Execute retail FAE8 (or F4B4) bytes and the native production TU with identical
 * inputs. Both share PsyCross GTE: not an independent PS1 hardware oracle. */
#include "common.h"
#include "psyq/libgte.h"
#include "psx/inline_c.h"
#include "../src/battle_mips_runtime.c"
#include "psx/gtereg.h"
#include <sys/mman.h>

#ifdef TEST_F4B4
#define NATIVE_ENTRY func_8002F4B4
#define LABEL "MODEL PRIM F4B4"
#define RETAIL_ENTRY 0x8002f4b4u
#else
#define NATIVE_ENTRY func_8002FAE8
#define LABEL "MODEL PRIM FAE8"
#define RETAIL_ENTRY 0x8002fae8u
#endif

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
/* compat.o's real pump is the sole definition; with a frozen vblank count it
 * early-returns before touching pad state (a conflicting no-op stub here
 * breaks the link with multiple-definition). */
int PsyX_Sys_GetVBlankCount(void) { return 0; }
void PsyX_UpdateInput(void) {}
void ControllerPoll(void) {}
void ControllerPushState(void) {}
char PsyX_BeginScene(void) { abort(); }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long* p, int s) { (void)p; (void)s; abort(); }
u8* D_80059424;
u32 D_8005953C, D_80059498, D_80059568, D_8005952C;
s32 D_80059578, D_80050100, D_800500F8, D_800500FC;
extern s32 NATIVE_ENTRY(u8*, s32) __attribute__((weak));

#define VERT 0x100000u
#define CMD 0x180000u
#define OUT_MAP 0x200000u
#define OUT (OUT_MAP + 32u)
#define NORMAL 0x280000u
#define OT 0x300000u
#define SIZE 0x80000u
#define STACK 0x801ff000u
static const u32 bases[] = {VERT, CMD, OUT_MAP, NORMAL, OT};
static u8 expected_output[256], expected_ot[SIZE], initial_sources[3][SIZE];
static const char* names[] = {
    "count0-terminal-rtpt", "front-lit", "backface-no-stores",
    "otz-zero-no-stores", "screen-reject", "three-normal-cursors",
    "packed-lookahead-v0-v1", "fourth-vertex-depth", "gte-flag-not-lzcr"
};

static int in_fixture(u32 address, unsigned width)
{
    for (unsigned i = 0; i < sizeof(bases)/sizeof(bases[0]); ++i)
        if (address >= bases[i] && (uint64_t)address + width <= bases[i] + SIZE)
            return 1;
    return 0;
}
static int read_fixture(void* opaque, u32 address, unsigned width, u32* value)
{
    if (!in_fixture(address, width)) return runtime_read(opaque, address, width, value);
    *value = load_le((u8*)(uintptr_t)address, width);
    return 0;
}
static int write_fixture(void* opaque, u32 address, unsigned width, u32 value)
{
    if (!in_fixture(address, width)) return runtime_write(opaque, address, width, value);
    store_le((u8*)(uintptr_t)address, width, value);
    return 0;
}
static void reset_gte(unsigned fixture)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    CTC2(4096, 0); CTC2(4096, 2); CTC2(4096, 4); CTC2(1024, 7);
    CTC2(4096, 8); CTC2(4096, 10); CTC2(4096, 12);
    CTC2(4096, 16); CTC2(4096, 18); CTC2(4096, 20);
    CTC2(160u << 16, 24); CTC2(112u << 16, 25); CTC2(512, 26);
    CTC2(256, 27); CTC2(0x200000, 28);
    CTC2(fixture == 3 ? 0 : 1365, 29);
    CTC2(fixture == 3 ? 0 : 1024, 30);
    MTC2(0x37112233, 6);
}
static unsigned prepare(unsigned fixture)
{
    const s16 xyz[4][3] = {{-30,-20,800},{30,-20,800},{-30,20,800},{30,20,800}};
    unsigned count = fixture == 0 ? 0 : (fixture == 5 || fixture == 6 ? 3 : 1);
    memset((void*)VERT, 0x39, SIZE);
    memset((void*)CMD, 0x5a, SIZE);
    memset((void*)OUT_MAP, 0xa5, SIZE);
    memset((void*)NORMAL, 0x6c, SIZE);
    memset((void*)OT, 0, SIZE);
    memset(g_PsxScratchpad, 0x93, sizeof(g_PsxScratchpad));
    for (unsigned i = 0; i < 4; ++i) {
        u8* v = (u8*)VERT + (i + 1) * 8;
        for (unsigned axis = 0; axis < 3; ++axis)
            store_le(v + axis * 2, 2, (u16)xyz[i][axis]);
    }
    if (fixture == 7) store_le((u8*)VERT + 4*8 + 4, 2, 5000);
    if (fixture == 8) store_le((u8*)VERT + 4*8 + 4, 2, (u16)-1024);
    for (unsigned i = 0; i <= count; ++i) {
        u32 v0 = fixture == 6 && i != 0 ? 0xe001 : 1;
        u32 v1 = fixture == 6 ? 0xe002 : (fixture == 2 ? 3 : 2);
        store_le((u8*)CMD + i*8, 4, v0 | (v1 << 16));
        store_le((u8*)CMD + i*8 + 4, 2, fixture == 2 ? 2 : 3);
        store_le((u8*)CMD + i*8 + 6, 2, 4);
        store_le((u8*)NORMAL + i*8, 2, 1024 + i*97);
        store_le((u8*)NORMAL + i*8 + 2, 2, 2048 - i*53);
        store_le((u8*)NORMAL + i*8 + 4, 2, 3072 + i*31);
        store_le((u8*)OUT + i*40 + 4, 4, 0xab123456u ^ (i*0x3070b));
    }
    for (unsigned i = 0; i < 4096; ++i)
        store_le((u8*)OT + i*4, 4, 0xc00000u + i*4);
#ifdef TEST_F4B4
    /* Distinct normals at vertex indices, including the full-width V0
     * used by fixture 6; this path has no per-face normal cursor. */
    const u32 indices[] = {1,2,3,0xe001};
    for (unsigned i = 0; i < 4; ++i) {
        u8* n = (u8*)NORMAL + indices[i]*8;
        store_le(n,2,600+i*421);
        store_le(n+2,2,1700-i*213);
        store_le(n+4,2,2100+i*113);
    }
#endif
    return count;
}
static int equal_bytes(const char* name, const char* region, const void* actual,
                        const void* expected, size_t size)
{
    if (!memcmp(actual, expected, size)) return 1;
    const u8* a = actual; const u8* e = expected;
    for (size_t i = 0; i < size; ++i) if (a[i] != e[i]) {
        fprintf(stderr, LABEL " FAIL %s %s byte=%zu native=%02x retail=%02x\n",
                name, region, i, a[i], e[i]);
        break;
    }
    return 0;
}
static int run_fixture(unsigned fixture)
{
    const char* name = names[fixture];
    PcPortMipsCpu cpu;
    BattleMipsRuntime runtime = {0};
    unsigned count = prepare(fixture);
    s32 shift = fixture == 6 ? 31 : 2;
    u32 xbound = fixture == 4 ? 0 : 320;
    const u32 sources[] = {VERT, CMD, NORMAL};
    for (unsigned i = 0; i < 3; ++i) memcpy(initial_sources[i], (void*)(uintptr_t)sources[i], SIZE);
    const u32 addresses[] = {0x8005953c,0x80059424,0x80059568,0x80059498,
                             0x80059578,0x80050100,0x800500f8,0x800500fc};
    const u32 values[] = {VERT,OUT,OT,NORMAL,7,(u32)shift,xbound,224u<<16};
    for (unsigned i = 0; i < 8; ++i) store_le(PSX_ADDR(addresses[i]), 4, values[i]);
    store_le(PSX_ADDR(0x8005952c),4,NORMAL);
    reset_gte(fixture);
    initialize_cpu(&cpu, &runtime);
    cpu.bus.read = read_fixture; cpu.bus.write = write_fixture; cpu.bus.bridge = NULL;
    cpu.gpr[4] = CMD; cpu.gpr[5] = count; cpu.gpr[29] = STACK;
    for (unsigned i = 16; i <= 23; ++i) cpu.gpr[i] = 0x6f000000u+i;
    if (PcPortMipsRun(&cpu, RETAIL_ENTRY, BATTLE_HALT_PC, 5000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, LABEL " FAIL retail %s: %s\n", name, cpu.error); return 0;
    }
    assert(cpu.gpr[29] == STACK);
    for (unsigned i = 16; i <= 23; ++i) assert(cpu.gpr[i] == 0x6f000000u+i);
    u32 cursor = load_le(PSX_ADDR(0x80059424),4);
    u32 normals = load_le(PSX_ADDR(0x80059498),4);
    u32 emitted = load_le(PSX_ADDR(0x80059578),4);
    u32 result = cpu.gpr[2];
    GTERegisters expected_gte = gteRegs;
    u8 expected_scratch[sizeof(g_PsxScratchpad)];
    memcpy(expected_scratch,g_PsxScratchpad,sizeof(expected_scratch));
    memcpy(expected_output, (void*)OUT_MAP, sizeof(expected_output));
    memcpy(expected_ot, (void*)OT, SIZE);
    if (fixture == 1) {
        assert(emitted == 8);
        assert(load_le(expected_output+32,4) != 0xa5a5a5a5u);
        assert(load_le(expected_output+36,4) != 0xab123456u);
    }
    if (fixture == 2 || fixture == 3 || fixture == 4) {
#ifdef TEST_F4B4
        assert(emitted == 7);
        assert(load_le(expected_output+32,4) == 0xa5a5a5a5u);
        if (fixture == 4) assert(load_le(expected_output+40,4) == 0xa5a5a5a5u);
        else assert(load_le(expected_output+40,4) != 0xa5a5a5a5u);
        if (fixture == 2) assert(load_le(expected_output+64,4) == 0xa5a5a5a5u);
        if (fixture == 3) assert(load_le(expected_output+64,4) != 0xa5a5a5a5u);
#else
        assert(emitted == (fixture == 3 ? 8u : 7u));
        assert(load_le(expected_output+40,4) == 0xa5a5a5a5u);
        assert(load_le(expected_output+64,4) == 0xa5a5a5a5u);
        assert(load_le(expected_output+32,4) == 0xa5a5a5a5u);
#endif
    }
    prepare(fixture);
    D_8005953C=VERT; D_80059424=(u8*)OUT; D_80059568=OT; D_80059498=NORMAL;
    D_8005952C=NORMAL;
    D_80059578=7; D_80050100=shift; D_800500F8=xbound; D_800500FC=224u<<16;
    reset_gte(fixture);
    u32 actual = (u32)NATIVE_ENTRY((u8*)CMD, (s32)count);
    if (actual != result || (u32)(uintptr_t)D_80059424 != cursor ||
        D_80059498 != normals || (u32)D_80059578 != emitted ||
        !equal_bytes(name,"packet/guards",(void*)OUT_MAP,expected_output,sizeof(expected_output)) ||
        !equal_bytes(name,"OT",(void*)OT,expected_ot,SIZE) ||
        !equal_bytes(name,"GTE",&gteRegs,&expected_gte,sizeof(gteRegs)) ||
        !equal_bytes(name,"scratchpad",g_PsxScratchpad,expected_scratch,sizeof(expected_scratch))) {
        fprintf(stderr,LABEL " FAIL %s result=%x/%x cursor=%x/%x normals=%x/%x emitted=%u/%u\n",
            name,actual,result,(u32)(uintptr_t)D_80059424,cursor,D_80059498,normals,(u32)D_80059578,emitted);
        return 0;
    }
    for (unsigned i = 0; i < 3; ++i)
        if (!equal_bytes(name,"source",(void*)(uintptr_t)sources[i],initial_sources[i],SIZE)) return 0;
    return 1;
}
int main(void)
{
    if (!NATIVE_ENTRY) { fputs(LABEL " FAIL missing production owner\n",stderr); return 1; }
    FILE* disc=fopen("disc/SLUS_006.64","rb");
    assert(disc && fseek(disc,0x800,SEEK_SET)==0);
    assert(fread(PSX_ADDR(0x80010000),1,0x476e4,disc)==0x476e4);
    assert(fclose(disc)==0);
    for (unsigned i=0;i<sizeof(bases)/sizeof(bases[0]);++i)
        assert(mmap((void*)(uintptr_t)bases[i],SIZE,PROT_READ|PROT_WRITE,
                    MAP_PRIVATE|MAP_ANONYMOUS|MAP_FIXED_NOREPLACE,-1,0)==(void*)(uintptr_t)bases[i]);
    for (unsigned i=0;i<sizeof(names)/sizeof(names[0]);++i) if (!run_fixture(i)) return 1;
    printf(LABEL " RETAIL PASS cases=%zu packets/OT/cursors/sources/GTE/scratchpad\n",sizeof(names)/sizeof(names[0]));
    return 0;
}
