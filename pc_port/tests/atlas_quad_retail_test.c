/* Actual retail 80025FA8 instructions versus the decomp-owned C body.
 * Both use PsyCross GTE: this is not an independent hardware oracle. */
#include "../src/battle_mips_runtime.c"
#include "psx/gtereg.h"
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
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }
extern int func_80025FA8(uint8_t *, int, uint8_t *, int, int, int, int, int, int)
    __attribute__((weak));
extern int ReadGeomScreen(void);
extern void ReadGeomOffset(long *, long *);
extern MATRIX *ScaleMatrixL(MATRIX *, VECTOR *);

static int sdk_bridge(void *opaque, PcPortMipsCpu *cpu, uint32_t target)
{
    if (target >= 0x80025fa8u && target < 0x80026338u) return 0;
    if (target >= 0x80043a1cu && target < 0x80043d00u) return 0;
    return runtime_bridge(opaque, cpu, target);
}

static void initial_gte(void)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    CTC2(4096, 0); CTC2(4096, 2); CTC2(4096, 4);
    CTC2(123, 5); CTC2(456, 6); CTC2(789, 7);
    SetGeomOffset(160, 112); SetGeomScreen(512);
}

int main(void)
{
    BattleMipsRuntime runtime = {0};
    PcPortMipsCpu cpu;
    uint8_t initial_corners[32];
    FILE *disc = fopen("disc/SLUS_006.64", "rb");
    assert(disc && fseek(disc, 0x800, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(0x80010000u), 1, 0x476e4, disc) == 0x476e4);
    fclose(disc);
    memcpy(initial_corners, PSX_ADDR(0x8004fdc0u), sizeof(initial_corners));
    if (!func_80025FA8) { fputs("ATLAS QUAD FAIL missing owner\n", stderr); return 1; }
#define SDK(addr, fn) runtime.functions[runtime.function_count++] = (ResolvedFunction){addr, fn, #fn}
    SDK(0x8004960cu, PushMatrix); SDK(0x800496acu, PopMatrix);
    SDK(0x8004974cu, ScaleMatrixL); SDK(0x80049efcu, SetRotMatrix);
    SDK(0x80049f8cu, SetTransMatrix); SDK(0x8004a0bcu, ReadGeomOffset);
    SDK(0x8004a0dcu, ReadGeomScreen); SDK(0x8004a12cu, SetGeomOffset);
    SDK(0x8004a14cu, SetGeomScreen); SDK(0x8004a73cu, RotTransPers4);
    SDK(0x8004b18cu, RotMatrixZ);
    const int rotations[] = {0, 1024, 2048, 3072, 4095, -1};
    unsigned cases = 0;
    for (unsigned f = 0; f < 1536; f++) {
        uint8_t expected[256], corners[32], input[384];
        GTERegisters expected_gte;
        int count = f % 4, buffer = (f / 4) % 2;
        int rotation = rotations[(f / 32) % 6];
        int sx = ((f / 192) & 1) ? -2048 : 2048, sy = ((f / 384) & 1) ? -2048 : 2048;
        int index = (f / 768) ? 0x52 : 0;
        uint8_t *table = PSX_ADDR(0x80180000u), *packets = PSX_ADDR(0x80190008u);
        memset(table, 0, sizeof(input));
        store_le(table + index * 2 + 4, 2, 192); store_le(table + 192, 2, count);
        for (int i = 0; i < count; i++) {
            uint8_t *item = table + 196 + 28 * i;
            const uint16_t values[] = {index ? 124 : 0, index ? 247 : 0, 116, 8,
                (uint16_t)-56, (uint16_t)-5, 4, 0, 0, 128, 497, 991, 247, 0};
            for (unsigned j = 0; j < 14; j++) store_le(item + 2 * j, 2, values[j]);
            item[26] = (f >> 3) & 1; item[27] = (f >> 4) & 1;
        }
        memcpy(input, table, sizeof(input));
        memset(packets - 8, 0xa5, sizeof(expected));
        memcpy(PSX_ADDR(0x8004fdc0u), initial_corners, 32);
        initial_gte(); initialize_cpu(&cpu, &runtime); cpu.bus.bridge = sdk_bridge;
        cpu.gpr[4] = 0x80180000u; cpu.gpr[5] = index; cpu.gpr[6] = 0x80190008u;
        cpu.gpr[7] = buffer; cpu.gpr[29] = 0x801ff000u;
        const int args[] = {221, 34, sx, sy, rotation};
        for (unsigned i = 0; i < 5; i++) store_le(PSX_ADDR(0x801ff010u + i*4), 4, args[i]);
        if (PcPortMipsRun(&cpu, 0x80025fa8u, BATTLE_HALT_PC, 20000) != PC_PORT_MIPS_HALTED) {
            fprintf(stderr, "ATLAS RETAIL FAIL %s\n", cpu.error); return 1;
        }
        uint32_t result = cpu.gpr[2]; expected_gte = gteRegs;
        memcpy(expected, packets - 8, sizeof(expected));
        memcpy(corners, PSX_ADDR(0x8004fdc0u), sizeof(corners));
        memset(packets - 8, 0xa5, sizeof(expected));
        memcpy(PSX_ADDR(0x8004fdc0u), initial_corners, 32); initial_gte();
        int actual = func_80025FA8(table, index, packets, buffer, 221, 34, sx, sy, rotation);
        if ((uint32_t)actual != result || memcmp(expected, packets - 8, sizeof(expected)) ||
            memcmp(corners, PSX_ADDR(0x8004fdc0u), sizeof(corners)) ||
            memcmp(input, table, sizeof(input)) || memcmp(&expected_gte, &gteRegs, sizeof(gteRegs))) {
            fprintf(stderr, "ATLAS QUAD FAIL fixture=%u count=%d buffer=%d rotation=%d\n", f,count,buffer,rotation);
            for (unsigned i=0;i<sizeof(expected);i++) if(expected[i] != (packets-8)[i]) {
                fprintf(stderr,"packet byte %u: retail=%02x native=%02x\n",i,expected[i],(packets-8)[i]); break;
            }
            return 1;
        }
        cases++;
    }
    printf("ATLAS QUAD RETAIL PASS cases=%u packets/guards/input/corners/GTE/return\n", cases);
    return 0;
}
