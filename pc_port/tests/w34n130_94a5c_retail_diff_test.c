/* Run the owner-supplied world collision router on the CPU adapter with
 * deterministic terrain probes. These are isolated guest-code checks. */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "battle_mips_adapter.h"

enum { RAM_SIZE = 2 * 1024 * 1024, SCRATCH_SIZE = 1024 };
#define RAM_BASE UINT32_C(0x80000000)
#define SCRATCH_BASE UINT32_C(0x1f800000)
#define WORLD_BASE UINT32_C(0x8006faf0)
static uint8_t ram[RAM_SIZE];
static uint8_t scratch[SCRATCH_SIZE];

#define HALT UINT32_C(0x80001000)
#define MAXEV 64
typedef struct { uint32_t kind, a0, a1, a2, a3; } Ev;
static Ev s_ev[MAXEV];
static int s_nev;
static uint32_t s_probe_ret[5];   /* by helper id 1..4 */
static int32_t s_94060_ret;

static void ev(uint32_t k, uint32_t a0, uint32_t a1, uint32_t a2, uint32_t a3)
{
    if (s_nev >= MAXEV) abort();
    s_ev[s_nev++] = (Ev){ k, a0, a1, a2, a3 };
}

static int32_t nclip(uint32_t p0, uint32_t p1, uint32_t p2)
{
    int64_t x0 = (int16_t)(p0 & 0xFFFF), y0 = (int16_t)(p0 >> 16);
    int64_t x1 = (int16_t)(p1 & 0xFFFF), y1 = (int16_t)(p1 >> 16);
    int64_t x2 = (int16_t)(p2 & 0xFFFF), y2 = (int16_t)(p2 >> 16);
    return (int32_t)(x0 * y1 + x1 * y2 + x2 * y0 - x0 * y2 - x1 * y0 - x2 * y1);
}

static void *bus_ptr(uint32_t addr, unsigned width)
{
    if (!width || width > 4) return NULL;
    if (addr >= RAM_BASE && addr - RAM_BASE <= RAM_SIZE - width)
        return ram + (addr - RAM_BASE);
    if (addr >= SCRATCH_BASE && addr - SCRATCH_BASE <= SCRATCH_SIZE - width)
        return scratch + (addr - SCRATCH_BASE);
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
static void put(uint32_t addr, uint32_t value)
{
    uint8_t *dest = ram + (addr - RAM_BASE);
    for (unsigned i = 0; i < 4; ++i) dest[i] = (uint8_t)(value >> (8u * i));
}

/* The sequence boundaries and every 32-bit field have an explicit wire format.
 * Each digest covers the same observable on all 20,000 guest invocations. */
static uint64_t digest_bytes(uint64_t hash, const uint8_t *p, size_t n)
{
    for (size_t i = 0; i < n; ++i) hash = (hash ^ p[i]) * UINT64_C(1099511628211);
    return hash;
}

static uint64_t digest_word(uint64_t hash, uint32_t word)
{
    uint8_t bytes[4];
    for (unsigned i = 0; i < 4; ++i) bytes[i] = (uint8_t)(word >> (i * 8u));
    return digest_bytes(hash, bytes, sizeof bytes);
}

int main(int argc, char **argv)
{
    if (argc != 2) return 2;
    FILE *image = fopen(argv[1], "rb");
    if (!image) return 2;
    const size_t bytes = 180422;
    if (fread(ram + (WORLD_BASE - RAM_BASE), 1, bytes, image) != bytes ||
        fgetc(image) != EOF || ferror(image) || fclose(image)) {
        fputs("Cannot read the complete world overlay.\n", stderr);
        return 2;
    }

    uint8_t ws_seed[0xb0];
    uint64_t returns = UINT64_C(14695981039346656037);
    uint64_t calls = returns, workspace = returns;
    unsigned event_count = 0, clip_count = 0;
    for (int n = 0; n < 20000; ++n) {
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
        PcPortMipsBus bus = {0};
        PcPortMipsCpu cpu;

        for (k = 1; k <= 4; k++) s_probe_ret[k] = (rnd() % 4u == 0u) ? (rnd() % 3u) + 1u : 0u;
        s_94060_ret = (int32_t)(rnd() & 1u);
        for (k = 0; k < sizeof(ws_seed); k++) ws_seed[k] = (uint8_t)rnd();

        put(BASE + 0, cell_x + (uint32_t)off_x); put(BASE + 4, 0xFFED0000u);
        put(BASE + 8, cell_z + (uint32_t)off_z); put(BASE + 12, 0);
        put(DIR + 0, (uint32_t)d[0]); put(DIR + 4, 0); put(DIR + 8, (uint32_t)d[1]); put(DIR + 12, 0);

        memcpy(scratch, ws_seed, sizeof ws_seed);
        s_nev = 0;
        bus.read = bus_read; bus.write = bus_write; bus.bridge = bridge;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = BASE; cpu.gpr[5] = DIR; cpu.gpr[6] = (uint32_t)scale; cpu.gpr[7] = (uint32_t)mode;
        const uint32_t initial_sp = 0x801ff000u;
        cpu.gpr[29] = initial_sp; cpu.gpr[31] = HALT;
        if (PcPortMipsRun(&cpu, 0x80094A5Cu, HALT, 20000u) != PC_PORT_MIPS_HALTED) {
            fprintf(stderr, "W34N130 FAIL retail run %d did not return\n", n); return 2;
        }
        if (cpu.pc != HALT || cpu.gpr[29] != initial_sp || cpu.gpr[31] != HALT || !cpu.steps) {
            fprintf(stderr, "Guest call %d did not restore the expected caller state.\n", n);
            return 1;
        }
        returns = digest_word(returns, cpu.gpr[2]);
        calls = digest_word(calls, (uint32_t)s_nev);
        for (int i = 0; i < s_nev; ++i) {
            const Ev *event = &s_ev[i];
            calls = digest_word(calls, event->kind);
            calls = digest_word(calls, event->a0);
            calls = digest_word(calls, event->a1);
            calls = digest_word(calls, event->a2);
            calls = digest_word(calls, event->a3);
            if (event->kind == 20) ++clip_count;
        }
        event_count += (unsigned)s_nev;
        workspace = digest_bytes(workspace, scratch, sizeof ws_seed);
    }
    printf("20,000 guest collision calls; %u intercepted calls (%u clip); "
           "return %016" PRIx64 ", events %016" PRIx64 ", workspace %016" PRIx64 "\n",
           event_count, clip_count, returns, calls, workspace);
    /* Recorded with the owned overlay and the seeded terrain-service inputs above. */
    if (event_count != 66510 || clip_count != 3055 ||
        returns != UINT64_C(0x53891f63339d8316) ||
        calls != UINT64_C(0x80a161ff9ac8127c) ||
        workspace != UINT64_C(0x0aeab1dd592b2aba)) {
        fputs("Guest collision trace changed.\n", stderr);
        return 1;
    }
    return 0;
}
