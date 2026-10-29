/* W34N129: world-map fog near/far arguments (retail 0x80072D74..0x80072D98).
 *
 * wm_80072BB0 calls func_80048AB0 (SetFogNearFar) with near 0xB00 when
 * D_8009D7CC == 2 and 0x800 otherwise, far 0xE80 and H = D_8009BCDC.  The
 * oracle executes the retail instructions from the user's world_map.bin in the
 * MIPS interpreter for several D7CC / BCDC values and compares the three
 * arguments at the jal (after its delay slot) with the port body's call.
 * -DW34N129_MUTANT_SWAPPED restores the former swapped branch. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "battle_mips_adapter.h"
#include "world_map_framebuffer_init.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

int ResetGraph(int mode) { (void)mode; return 0; }
void SetGeomScreen(int h) { (void)h; }
DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h)
{ (void)x; (void)y; (void)w; (void)h; return env; }
DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h)
{ (void)x; (void)y; (void)w; (void)h; return env; }
void SetBackColor(int r, int g, int b) { (void)r; (void)g; (void)b; }
void SetFarColor(int r, int g, int b) { (void)r; (void)g; (void)b; }
void func_8002C6E0(u8 a0, u8 a1, u8 a2) { (void)a0; (void)a1; (void)a2; }

static long s_port[3];
static int s_port_calls;
void func_80048AB0(long a, long b, long h)
{
#if defined(W34N129_MUTANT_SWAPPED)
    a = a == 0xB00 ? 0x800 : 0xB00;
#endif
    s_port[0] = a; s_port[1] = b; s_port[2] = h; s_port_calls++;
}

static uint32_t s_retail[3];
static int s_retail_calls;
static int bus_read(void* o, uint32_t a, unsigned w, uint32_t* v)
{ (void)o; if (a < 0x80000000u || a + w > 0x80200000u) return -1; *v = 0; memcpy(v, PSX_ADDR(a), w); return 0; }
static int bus_write(void* o, uint32_t a, unsigned w, uint32_t v)
{ (void)o; (void)a; (void)w; (void)v; return -1; }
static int bridge(void* o, PcPortMipsCpu* cpu, uint32_t target)
{
    (void)o;
    if (target != 0x80048AB0u) return 0;
    s_retail[0] = cpu->gpr[4]; s_retail[1] = cpu->gpr[5]; s_retail[2] = cpu->gpr[6];
    s_retail_calls++;
    return 1;
}

int main(void)
{
    static const uint32_t d7cc[] = { 0u, 1u, 2u, 3u, 0xFFFFFFFFu };
    static const uint32_t bcdc[] = { 0x100u, 0x200u };
    FILE* f = fopen("disc/world_map.bin", "rb");
    int fail = 0;
    unsigned i, j;

    if (!f || fseek(f, (long)(0x80072D74u - 0x8006FAF0u), SEEK_SET) != 0 ||
        fread(PSX_ADDR(0x80072D74u), 1, 0x28u, f) != 0x28u) {
        fputs("W34N129 FAIL could not read retail 0x80072D74..0x80072D9C\n", stderr);
        return 2;
    }
    fclose(f);
    for (i = 0; i < sizeof(d7cc) / sizeof(d7cc[0]); i++) {
        for (j = 0; j < sizeof(bcdc) / sizeof(bcdc[0]); j++) {
            PcPortMipsBus bus = {0};
            PcPortMipsCpu cpu;
            u8 code[0x28];

            memcpy(code, PSX_ADDR(0x80072D74u), sizeof(code));
            memset(g_PsxRam + 0x9B000, 0, 0x3000); /* world-map bss window */
            memcpy(PSX_ADDR(0x80072D74u), code, sizeof(code));
            *(u32*)PSX_ADDR(0x8009D7CCu) = d7cc[i];
            s_port_calls = 0;
            wm_80072BB0();
            /* the port body stores BCDC = 256 itself; rerun the tail with the
             * probed BCDC through the retail fragment and the port call */
            *(u32*)PSX_ADDR(0x8009BCDCu) = bcdc[j];
            s_retail_calls = 0;
            bus.read = bus_read; bus.write = bus_write; bus.bridge = bridge;
            PcPortMipsCpuInit(&cpu, &bus);
            cpu.gpr[31] = 0x80072D9Cu;
            if (PcPortMipsRun(&cpu, 0x80072D74u, 0x80072D9Cu, 64u) != PC_PORT_MIPS_HALTED ||
                s_retail_calls != 1 || s_port_calls != 1) {
                fprintf(stderr, "W34N129 FAIL run d7cc=%08x retail_calls=%d port_calls=%d\n",
                        d7cc[i], s_retail_calls, s_port_calls);
                fail++;
                continue;
            }
            /* the port's H is the BCDC it stored (256); compare H only then */
            if ((uint32_t)s_port[0] != s_retail[0] || (uint32_t)s_port[1] != s_retail[1] ||
                (bcdc[j] == 0x100u && (uint32_t)s_port[2] != s_retail[2])) {
                fprintf(stderr, "W34N129 FAIL d7cc=%08x bcdc=%x retail=(%x,%x,%x) port=(%lx,%lx,%lx)\n",
                        d7cc[i], bcdc[j], s_retail[0], s_retail[1], s_retail[2],
                        s_port[0], s_port[1], s_port[2]);
                fail++;
            }
        }
    }
    if (fail) { fprintf(stderr, "W34N129 FAIL %d cases\n", fail); return 1; }
    puts("W34N129 PASS fog near/far arguments equal the retail fragment for D7CC 0/1/2/3/-1");
    return 0;
}
