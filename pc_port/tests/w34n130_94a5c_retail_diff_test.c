/* W34N130: wm_80094A5C (world-map diagonal collision router) versus the retail
 * instructions from the user's world_map.bin.
 *
 * Both sides get the same boundary doubles: NormalClip computes the GTE NCLIP
 * of its packed arguments, the four private probes and the tail helpers are
 * recorded and return scripted values.  For thousands of base/direction pairs
 * around 0x80000 grid lines the test compares the result, every NormalClip
 * argument, the probe call order and arguments, the tail calls and the
 * scratch workspace 0x1F800000..0x1F8000B0.  -DWM_94A5C_MUTANT_WS_BYTE2
 * restores the former misread of WS+8 as WS+2 and must fail. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"
#include "world_map_func_94a5c.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define HALT UINT32_C(0x80001000)
#define MAXEV 64
typedef struct { uint32_t kind, a0, a1, a2, a3; } Ev;
static Ev s_ev[MAXEV];
static int s_nev;
static uint32_t s_probe_ret[5];   /* by helper id 1..4 */
static int32_t s_94060_ret;

static void ev(uint32_t k, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    if (s_nev < MAXEV) { s_ev[s_nev].kind = k; s_ev[s_nev].a0 = a0; s_ev[s_nev].a1 = a1;
        s_ev[s_nev].a2 = a2; s_ev[s_nev].a3 = a3; }
    s_nev++;
}

long NormalClip(long a, long b, long c) { (void)a; (void)b; (void)c; abort(); }
static int32_t nclip(uint32_t p0, uint32_t p1, uint32_t p2)
{
    int64_t x0 = (int16_t)(p0 & 0xFFFF), y0 = (int16_t)(p0 >> 16);
    int64_t x1 = (int16_t)(p1 & 0xFFFF), y1 = (int16_t)(p1 >> 16);
    int64_t x2 = (int16_t)(p2 & 0xFFFF), y2 = (int16_t)(p2 >> 16);
    return (int32_t)(x0 * y1 + x1 * y2 + x2 * y0 - x0 * y2 - x1 * y0 - x2 * y1);
}

/* ---- port-side doubles (WM_94A5C_TEST_TRACE / WM_94A5C_TEST_HOOK) ---- */
void wm_private_test_93354(u32 addr) { ev(10, addr, 0, 0, 0); }
s32 wm_private_test_93f18(u32 addr) { ev(11, addr, 0, 0, 0); return 0; }
s32 wm_private_test_94060(s32 mode, s32 code) { ev(12, (u32)mode, (u32)code, 0, 0); return s_94060_ret; }
#define PROBE(id, name) \
s32 name(u32 b, u32 d, u32 w, s32 m) { ev(id, b, d, w, (u32)m); return (s32)s_probe_ret[id]; }
PROBE(1, wm_private_test_9443C)
PROBE(2, wm_private_test_945C8)
PROBE(3, wm_private_test_94750)
PROBE(4, wm_private_test_948D8)
static s32 port_nclip(s32 a0, s32 a1, s32 a2, s32 a3)
{
    (void)a3;
    ev(20, (u32)a0, (u32)a1, (u32)a2, 0);
    return nclip((u32)a0, (u32)a1, (u32)a2);
}

/* ---- retail side ---- */
static void* bus_ptr(uint32_t a, unsigned w)
{
    if ((a >> 20) == 0x1F8u && (a & 0xFFFFFu) + w <= 0x400u) return PSX_ADDR(a); /* same alias as the port */
    if (a >= 0x80000000u && a + w <= 0x80200000u) return PSX_ADDR(a);
    return NULL;
}
static int bus_read(void* o, uint32_t a, unsigned w, uint32_t* v)
{ void* p = bus_ptr(a, w); (void)o; if (!p) return -1; *v = 0; memcpy(v, p, w); return 0; }
static int bus_write(void* o, uint32_t a, unsigned w, uint32_t v)
{ void* p = bus_ptr(a, w); (void)o; if (!p) return -1; memcpy(p, &v, w); return 0; }
static int bridge(void* o, PcPortMipsCpu* c, uint32_t t)
{
    uint32_t* g = c->gpr;
    (void)o;
    switch (t) {
    case 0x80093354u: ev(10, g[4], 0, 0, 0); g[2] = 0; return 1;
    case 0x80093F18u: ev(11, g[4], 0, 0, 0); g[2] = 0; return 1;
    case 0x80094060u: ev(12, g[4], g[5], 0, 0); g[2] = (uint32_t)s_94060_ret; return 1;
    case 0x8009443Cu: ev(1, g[4], g[5], g[6], g[7]); g[2] = s_probe_ret[1]; return 1;
    case 0x800945C8u: ev(2, g[4], g[5], g[6], g[7]); g[2] = s_probe_ret[2]; return 1;
    case 0x80094750u: ev(3, g[4], g[5], g[6], g[7]); g[2] = s_probe_ret[3]; return 1;
    case 0x800948D8u: ev(4, g[4], g[5], g[6], g[7]); g[2] = s_probe_ret[4]; return 1;
    case 0x8004A70Cu: ev(20, g[4], g[5], g[6], 0); g[2] = (uint32_t)nclip(g[4], g[5], g[6]); return 1;
    default: return 0;
    }
}

static uint32_t rng = 12345u;
static uint32_t rnd(void) { rng = rng * 1103515245u + 12345u; return rng >> 8; }

#define BASE UINT32_C(0x800D7000)
#define DIR  UINT32_C(0x800D7010)
static void put(uint32_t a, uint32_t v) { memcpy(PSX_ADDR(a), &v, 4); }

int main(void)
{
    static uint8_t ws_seed[0xB0], ws_port[0xB0];
    static Ev ev_port[MAXEV];
    FILE* f = fopen("disc/world_map.bin", "rb");
    long size;
    int n, bad = 0, cases_seen[16] = {0};

    if (!f) { fputs("W34N130 FAIL disc/world_map.bin missing\n", stderr); return 2; }
    fseek(f, 0, SEEK_END); size = ftell(f); fseek(f, 0, SEEK_SET);
    if (size <= 0 || fread(PSX_ADDR(0x8006FAF0u), 1, (size_t)size, f) != (size_t)size) {
        fputs("W34N130 FAIL read world_map.bin\n", stderr); fclose(f); return 2;
    }
    fclose(f);
    wm_80094A5C_set_probe(port_nclip);

    for (n = 0; n < 20000; n++) {
        uint32_t cell_x = 0x7000000u + (rnd() & 0x3Fu) * 0x80000u;
        uint32_t cell_z = 0x2800000u + (rnd() & 0x3Fu) * 0x80000u;
        int32_t off_x = (int32_t)(rnd() % 0x6000u) - 0x3000;
        int32_t off_z = (int32_t)(rnd() % 0x6000u) - 0x3000;
        static const int32_t dirs[][2] = { {-2896, 2896}, {2896, 2896}, {2896, -2896}, {-2896, -2896},
                                           {4096, 0}, {0, 4096}, {-4096, 0}, {0, -4096}, {3784, -1567}, {-1567, 3784} };
        const int32_t* d = dirs[rnd() % 10u];
        int32_t scale = 0x8000;
        int32_t mode = (rnd() & 1u) ? 1 : 2;
        uint32_t k;
        int nport;
        s32 rport;
        uint32_t rret;
        PcPortMipsBus bus = {0};
        PcPortMipsCpu cpu;

        for (k = 1; k <= 4; k++) s_probe_ret[k] = (rnd() % 4u == 0u) ? (rnd() % 3u) + 1u : 0u;
        s_94060_ret = (int32_t)(rnd() & 1u);
        for (k = 0; k < sizeof(ws_seed); k++) ws_seed[k] = (uint8_t)rnd();

        put(BASE + 0, cell_x + (uint32_t)off_x); put(BASE + 4, 0xFFED0000u);
        put(BASE + 8, cell_z + (uint32_t)off_z); put(BASE + 12, 0);
        put(DIR + 0, (uint32_t)d[0]); put(DIR + 4, 0); put(DIR + 8, (uint32_t)d[1]); put(DIR + 12, 0);

        memcpy(PSX_ADDR(0x1F800000u), ws_seed, sizeof(ws_seed));
        s_nev = 0;
        rport = wm_80094A5C(BASE, DIR, scale, mode);
        nport = s_nev; memcpy(ev_port, s_ev, sizeof(ev_port));
        memcpy(ws_port, PSX_ADDR(0x1F800000u), sizeof(ws_port));

        memcpy(PSX_ADDR(0x1F800000u), ws_seed, sizeof(ws_seed));
        s_nev = 0;
        bus.read = bus_read; bus.write = bus_write; bus.bridge = bridge;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = BASE; cpu.gpr[5] = DIR; cpu.gpr[6] = (uint32_t)scale; cpu.gpr[7] = (uint32_t)mode;
        cpu.gpr[29] = 0x801FF000u; cpu.gpr[31] = HALT;
        if (PcPortMipsRun(&cpu, 0x80094A5Cu, HALT, 20000u) != PC_PORT_MIPS_HALTED) {
            fprintf(stderr, "W34N130 FAIL retail run %d did not return\n", n); return 2;
        }
        rret = cpu.gpr[2];
        {
            int32_t bx = (int32_t)(cell_x + (uint32_t)off_x), bz = (int32_t)(cell_z + (uint32_t)off_z);
            int32_t nx = bx + (int32_t)(((int64_t)d[0] * scale) >> 12), nz = bz + (int32_t)(((int64_t)d[1] * scale) >> 12);
            int t = ((nx >> 19) < (bx >> 19) ? 2 : (bx >> 19) < (nx >> 19) ? 1 : 0) |
                    ((nz >> 19) < (bz >> 19) ? 8 : (bz >> 19) < (nz >> 19) ? 4 : 0);
            cases_seen[t]++;
        }
        if ((uint32_t)rport != rret || nport != s_nev || memcmp(ev_port, s_ev, sizeof(Ev) * (size_t)(nport < MAXEV ? nport : MAXEV)) != 0 ||
            memcmp(ws_port, PSX_ADDR(0x1F800000u), sizeof(ws_port)) != 0) {
            if (bad < 5) {
                int e;
                fprintf(stderr, "W34N130 FAIL case %d base=(%08x,%08x) dir=(%d,%d) mode=%d port=%d retail=%d events %d/%d\n",
                        n, cell_x + (uint32_t)off_x, cell_z + (uint32_t)off_z, d[0], d[1], mode, rport, (int32_t)rret, nport, s_nev);
                {
                    uint32_t wp[12], wr[12]; int q;
                    memcpy(wp, ws_port, sizeof(wp)); memcpy(wr, PSX_ADDR(0x1F800000u), sizeof(wr));
                    for (q = 0; q < 12; q++)
                        if (wp[q] != wr[q]) fprintf(stderr, "   ws+%02x port=%08x retail=%08x\n", q * 4, wp[q], wr[q]);
                }
                for (e = 0; e < nport || e < s_nev; e++)
                    if (e < MAXEV)
                        fprintf(stderr, "   ev%d port=(%u %08x %08x %08x %08x) retail=(%u %08x %08x %08x %08x)\n", e,
                                ev_port[e].kind, ev_port[e].a0, ev_port[e].a1, ev_port[e].a2, ev_port[e].a3,
                                s_ev[e].kind, s_ev[e].a0, s_ev[e].a1, s_ev[e].a2, s_ev[e].a3);
            }
            bad++;
        }
    }
    printf("W34N130 cases seen:");
    for (n = 0; n < 16; n++) if (cases_seen[n]) printf(" %d:%d", n, cases_seen[n]);
    printf("\n");
    if (bad) { fprintf(stderr, "W34N130 FAIL %d of 20000 inputs differ from retail\n", bad); return 1; }
    puts("W34N130 PASS 20000 inputs: result, NormalClip arguments, probe/tail calls and workspace equal retail");
    return 0;
}
