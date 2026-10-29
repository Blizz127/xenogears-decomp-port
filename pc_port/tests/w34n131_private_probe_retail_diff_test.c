/* W34N131: the four private collision probes 0x8009443C / 0x800945C8 /
 * 0x80094750 / 0x800948D8 (world_map_private_collision.c) versus the retail
 * instructions from the user's world_map.bin.  93354 / 93F18 / 94060 are
 * recorded boundary doubles with scripted results on both sides; the test
 * compares the result, the call sequence and the 0x1F800000..0x1F800040
 * workspace for random bases around 0x80000 grid lines and directions. */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "battle_mips_adapter.h"
#include "world_map_private_collision.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#define HALT UINT32_C(0x80001000)
#define MAXEV 16
typedef struct { uint32_t k, a0, a1, w[4]; } Ev;
static Ev s_ev[MAXEV];
static int s_nev;
static int32_t s_code[2], s_blk[2];
static int s_ncode, s_nblk;

static void ev(uint32_t k, uint32_t a0, uint32_t a1)
{
    if (s_nev < MAXEV) {
        s_ev[s_nev].k = k; s_ev[s_nev].a0 = a0; s_ev[s_nev].a1 = a1;
        if (k != 12) memcpy(s_ev[s_nev].w, PSX_ADDR(a0), 16); else memset(s_ev[s_nev].w, 0, 16);
    }
    s_nev++;
}
void wm_private_test_93354(u32 a) { ev(10, a, 0); }
s32 wm_private_test_93f18(u32 a) { ev(11, a, 0); return s_code[s_ncode++ & 1]; }
s32 wm_private_test_94060(s32 m, s32 c) { ev(12, (u32)m, (u32)c); return s_blk[s_nblk++ & 1]; }

static void* bus_ptr(uint32_t a, unsigned w)
{
    if ((a >> 20) == 0x1F8u && (a & 0xFFFFFu) + w <= 0x400u) return PSX_ADDR(a);
    if (a >= 0x80000000u && a + w <= 0x80200000u) return PSX_ADDR(a);
    return NULL;
}
static int bus_read(void* o, uint32_t a, unsigned w, uint32_t* v)
{ void* p = bus_ptr(a, w); (void)o; if (!p) return -1; *v = 0; memcpy(v, p, w); return 0; }
static int bus_write(void* o, uint32_t a, unsigned w, uint32_t v)
{ void* p = bus_ptr(a, w); (void)o; if (!p) return -1; memcpy(p, &v, w); return 0; }
static int bridge(void* o, PcPortMipsCpu* c, uint32_t t)
{
    (void)o;
    if (t == 0x80093354u) { ev(10, c->gpr[4], 0); c->gpr[2] = 0; return 1; }
    if (t == 0x80093F18u) { ev(11, c->gpr[4], 0); c->gpr[2] = (uint32_t)s_code[s_ncode++ & 1]; return 1; }
    if (t == 0x80094060u) { ev(12, c->gpr[4], c->gpr[5]); c->gpr[2] = (uint32_t)s_blk[s_nblk++ & 1]; return 1; }
    return 0;
}
static uint32_t rng = 777u;
static uint32_t rnd(void) { rng = rng * 1103515245u + 12345u; return rng >> 8; }
static void put(uint32_t a, uint32_t v) { memcpy(PSX_ADDR(a), &v, 4); }

typedef s32 (*probe_fn)(u32, u32, u32, s32);
int main(void)
{
    static const uint32_t entry[4] = { 0x8009443Cu, 0x800945C8u, 0x80094750u, 0x800948D8u };
    static const probe_fn port[4] = { wm_8009443C, wm_800945C8, wm_80094750, wm_800948D8 };
    static uint8_t seed[0x40], wsp[0x40];
    static Ev evp[MAXEV];
    FILE* f = fopen("disc/world_map.bin", "rb");
    long size; int n, bad = 0;
    if (!f) { fputs("W34N131 FAIL disc/world_map.bin missing\n", stderr); return 2; }
    fseek(f, 0, SEEK_END); size = ftell(f); fseek(f, 0, SEEK_SET);
    if (fread(PSX_ADDR(0x8006FAF0u), 1, (size_t)size, f) != (size_t)size) { fclose(f); return 2; }
    fclose(f);
    for (n = 0; n < 40000; n++) {
        int p = (int)(rnd() & 3u), k, np; s32 rp; uint32_t rr;
        uint32_t bx = 0x7000000u + (rnd() & 0x3Fu) * 0x80000u + (rnd() % 0x80000u);
        uint32_t bz = 0x2800000u + (rnd() & 0x3Fu) * 0x80000u + (rnd() % 0x80000u);
        int32_t dx = (int32_t)(rnd() % 8193u) - 4096, dz = (int32_t)(rnd() % 8193u) - 4096;
        int32_t mode = (int32_t)(rnd() % 3u);
        PcPortMipsBus bus = {0}; PcPortMipsCpu cpu;
        if ((p < 2 && dx == 0) || (p >= 2 && dz == 0)) continue; /* retail traps on /0 */
        if (p == 0 && dx < 0) dx = -dx; if (p == 1 && dx > 0) dx = -dx;
        if (p == 2 && dz < 0) dz = -dz; if (p == 3 && dz > 0) dz = -dz;
        for (k = 0; k < 0x40; k++) seed[k] = (uint8_t)rnd();
        /* the caller seeds WS+0x10/+0x18 with the destination coordinates */
        memcpy(seed + 0x10, &(uint32_t){ bx + (uint32_t)(dx * 8) }, 4);
        memcpy(seed + 0x18, &(uint32_t){ bz + (uint32_t)(dz * 8) }, 4);
        put(0x800D7000u, bx); put(0x800D7004u, 0xFFED0000u); put(0x800D7008u, bz); put(0x800D700Cu, 0);
        put(0x800D7010u, (uint32_t)dx); put(0x800D7014u, 0); put(0x800D7018u, (uint32_t)dz); put(0x800D701Cu, 0);
        s_code[0] = (int32_t)(rnd() % 5u); s_code[1] = (int32_t)(rnd() % 5u);
        s_blk[0] = (int32_t)(rnd() & 1u) * (int32_t)((rnd() & 1u) ? 1 : 0x10000);
        s_blk[1] = (int32_t)(rnd() & 1u);

        memcpy(PSX_ADDR(0x1F800000u), seed, sizeof(seed));
        s_nev = 0; s_ncode = 0; s_nblk = 0;
        rp = port[p](0x800D7000u, 0x800D7010u, 0x1F800000u, mode);
        np = s_nev; memcpy(evp, s_ev, sizeof(evp)); memcpy(wsp, PSX_ADDR(0x1F800000u), sizeof(wsp));

        memcpy(PSX_ADDR(0x1F800000u), seed, sizeof(seed));
        s_nev = 0; s_ncode = 0; s_nblk = 0;
        bus.read = bus_read; bus.write = bus_write; bus.bridge = bridge;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = 0x800D7000u; cpu.gpr[5] = 0x800D7010u; cpu.gpr[6] = 0x1F800000u; cpu.gpr[7] = (uint32_t)mode;
        cpu.gpr[29] = 0x801FF000u; cpu.gpr[31] = HALT;
        if (PcPortMipsRun(&cpu, entry[p], HALT, 5000u) != PC_PORT_MIPS_HALTED) {
            fprintf(stderr, "W34N131 FAIL retail probe %08x did not return (case %d)\n", entry[p], n); return 2;
        }
        rr = cpu.gpr[2];
        if ((uint32_t)rp != rr || np != s_nev || memcmp(evp, s_ev, sizeof(Ev) * (size_t)np) != 0 ||
            memcmp(wsp, PSX_ADDR(0x1F800000u), sizeof(wsp)) != 0) {
            if (bad < 4) {
                uint32_t a[16], b[16]; int q;
                memcpy(a, wsp, 64); memcpy(b, PSX_ADDR(0x1F800000u), 64);
                fprintf(stderr, "W34N131 FAIL probe %08x case %d base=(%08x,%08x) dir=(%d,%d) mode=%d port=%d retail=%d ev %d/%d\n",
                        entry[p], n, bx, bz, dx, dz, mode, rp, (int32_t)rr, np, s_nev);
                for (q = 0; q < 16; q++) if (a[q] != b[q]) fprintf(stderr, "   ws+%02x port=%08x retail=%08x\n", q * 4, a[q], b[q]);
            }
            bad++;
        }
    }
    if (bad) { fprintf(stderr, "W34N131 FAIL %d inputs differ from retail\n", bad); return 1; }
    puts("W34N131 PASS 4 private probes x random inputs: result, calls and workspace equal retail");
    return 0;
}
