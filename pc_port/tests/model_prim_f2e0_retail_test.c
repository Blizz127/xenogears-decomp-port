/* Retail 8002F2E0 instructions versus the decomp-owned C body.
 * Both sides use PsyCross GTE; this is not a PS1 hardware oracle. */
#include "../src/battle_mips_runtime.c"
#include "psx/gtereg.h"
#include <sys/mman.h>

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

u8 *D_80059424;
u32 D_8005953C;
u32 D_80059498;
u32 D_80059568;
s32 D_80059578;
s32 D_80050100;
s32 D_800500F8;
s32 D_800500FC;

extern s32 func_8002F2E0(u8 *, s32) __attribute__((weak));

#define HOST_VERTEX  0x00100000u
#define HOST_COMMAND 0x00180000u
#define HOST_OUTPUT_MAP 0x00200000u
#define HOST_OUTPUT  (HOST_OUTPUT_MAP + 0x20u)
#define HOST_NORMAL  0x00280000u
#define HOST_OT      0x00300000u
#define GUEST_VERTEX HOST_VERTEX
#define GUEST_COMMAND HOST_COMMAND
#define GUEST_OUTPUT HOST_OUTPUT
#define GUEST_NORMAL HOST_NORMAL
#define GUEST_OT HOST_OT
#define STACK 0x801ff000u
#define REGION_SIZE 0x80000u

typedef struct Fixture {
    const char *name;
    s32 count;
    s16 xyz[4][3];
    u16 index[3];
    u32 color;
    s16 normal[3];
    s32 shift;
    u32 xmax;
    u32 ymax_packed;
    s32 zsf3;
} Fixture;

static const Fixture fixtures[] = {
    {"count0-prefetch", 0, {{-30,-20,800},{30,-20,800},{0,30,800},{0,0,0}},
     {1,2,3}, 0x20d04020u, {4096,0,0}, 2, 320, 224u << 16, 1365},
    {"front-lit", 1, {{-30,-20,800},{30,-20,800},{0,30,800},{0,0,0}},
     {1,2,3}, 0x20c06030u, {0,0,4096}, 2, 320, 224u << 16, 1365},
    {"backface-partial-xy", 1, {{-30,-20,800},{0,30,800},{30,-20,800},{0,0,0}},
     {1,2,3}, 0x2080a040u, {0,4096,0}, 2, 320, 224u << 16, 1365},
    {"otz-zero-normal-delay", 1, {{-30,-20,800},{30,-20,800},{0,30,800},{0,0,0}},
     {1,2,3}, 0x20f02010u, {-4096,0,0}, 2, 320, 224u << 16, 0},
    {"screen-edge", 1, {{-640,-20,800},{640,-20,800},{0,30,800},{0,0,0}},
     {1,2,3}, 0x202060a0u, {0,0,-4096}, 1, 160, 112u << 16, 1365},
    {"packed-index-high-half", 2, {{-40,-25,900},{0,35,900},{40,-25,900},{8,4,950}},
     {3,0xe001,2}, 0x2040b070u, {1024,-2048,3072}, 3, 320, 224u << 16, 1365},
    {"three-command-lookahead", 3, {{-50,-25,700},{0,40,850},{50,-25,1000},{20,15,750}},
     {2,3,1}, 0x20e09020u, {-3000,2000,1000}, 31, 320, 224u << 16, 1365},
};

static void map_native(uintptr_t address)
{
    void *p = mmap((void *)address, REGION_SIZE, PROT_READ | PROT_WRITE,
                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p != (void *)address) {
        perror("mmap fixed low native fixture");
        exit(1);
    }
    assert(address < 0x01000000u && address + REGION_SIZE <= 0x01000000u);
}

static void reset_gte(const Fixture *f)
{
    memset(&gteRegs, 0, sizeof(gteRegs));
    CTC2(4096, 0); CTC2(4096, 2); CTC2(4096, 4);
    CTC2(1024, 7);
    CTC2(4096, 8); CTC2(4096, 10); CTC2(4096, 12);
    CTC2(4096, 16); CTC2(4096, 18); CTC2(4096, 20);
    CTC2(0x00302010, 21); CTC2(0x00403020, 22); CTC2(0x00504030, 23);
    CTC2(160u << 16, 24); CTC2(112u << 16, 25); CTC2(512, 26);
    CTC2(0x100, 27); CTC2(0x200000, 28); CTC2((u32)f->zsf3, 29);
}

static int fixture_address(u32 address, unsigned width)
{
    const u32 bases[] = {HOST_VERTEX, HOST_COMMAND, HOST_OUTPUT_MAP, HOST_NORMAL, HOST_OT};
    for (unsigned i = 0; i < sizeof(bases) / sizeof(bases[0]); i++)
        if (address >= bases[i] && (uint64_t)address + width <= bases[i] + REGION_SIZE)
            return 1;
    return 0;
}

static int fixture_read(void *opaque, u32 address, unsigned width, u32 *value)
{
    BattleMipsRuntime *runtime = opaque;
    if (!fixture_address(address, width)) return runtime_read(runtime, address, width, value);
    *value = 0;
    for (unsigned i = 0; i < width; i++)
        *value |= (u32)*((u8 *)(uintptr_t)address + i) << (i * 8);
    return 0;
}

static int fixture_write(void *opaque, u32 address, unsigned width, u32 value)
{
    BattleMipsRuntime *runtime = opaque;
    if (!fixture_address(address, width)) return runtime_write(runtime, address, width, value);
    for (unsigned i = 0; i < width; i++)
        *((u8 *)(uintptr_t)address + i) = (u8)(value >> (i * 8));
    return 0;
}

static void put_vertex(u8 *base, unsigned index, const s16 xyz[3])
{
    memcpy(base + index * 8 + 0, &xyz[0], 2);
    memcpy(base + index * 8 + 2, &xyz[1], 2);
    memcpy(base + index * 8 + 4, &xyz[2], 2);
    store_le(base + index * 8 + 6, 2, 0x5aa5);
}

static void prepare_region(u8 *vertex, u8 *command, u8 *output, u8 *normal,
                           u8 *ot, const Fixture *f)
{
    memset(vertex, 0x39, REGION_SIZE);
    memset(command, 0x5a, REGION_SIZE);
    memset(output - 32, 0xa5, REGION_SIZE);
    memset(normal, 0x6c, REGION_SIZE);
    memset(ot, 0, REGION_SIZE);
    for (unsigned i = 0; i < 4; i++) put_vertex(vertex, i + 1, f->xyz[i]);
    for (unsigned i = 0; i <= (unsigned)f->count; i++) {
        unsigned rotate = i & 3u;
        u16 i0 = f->index[(0 + rotate) % 3];
        u16 i1 = f->index[(1 + rotate) % 3];
        u16 i2 = f->index[(2 + rotate) % 3];
        store_le(command + i * 8, 4, (u32)i0 | ((u32)i1 << 16));
        store_le(command + i * 8 + 4, 2, i2);
        store_le(command + i * 8 + 6, 2, 0x9b00u + i);
        store_le(normal + i * 8, 2, (u16)(f->normal[0] + i * 97));
        store_le(normal + i * 8 + 2, 2, (u16)(f->normal[1] - i * 53));
        store_le(normal + i * 8 + 4, 2, (u16)(f->normal[2] + i * 31));
        store_le(normal + i * 8 + 6, 2, 0x7e00u + i);
        store_le(output + i * 32 + 4, 4, f->color ^ (i * 0x0003070bu));
    }
    for (unsigned i = 0; i < 4096; i++)
        store_le(ot + i * 4, 4, 0x00c00000u + i * 4);
}

static int compare_bytes(const char *name, const char *what, const u8 *actual,
                         const u8 *expected, size_t size)
{
    if (!memcmp(actual, expected, size)) return 1;
    for (size_t i = 0; i < size; i++) if (actual[i] != expected[i]) {
        fprintf(stderr, "MODEL PRIM F2E0 FAIL %s %s byte=%zu native=%02x retail=%02x\n",
                name, what, i, actual[i], expected[i]);
        break;
    }
    return 0;
}

static int ot_is_initial(const u8 *ot)
{
    for (unsigned i = 0; i < 4096; i++)
        if (load_le(ot + i * 4, 4) != 0x00c00000u + i * 4) return 0;
    return 1;
}

static int run_fixture(const Fixture *f)
{
    PcPortMipsCpu cpu;
    BattleMipsRuntime runtime = {0};
    GTERegisters retail_gte;
    u8 expected_output[256], expected_ot[REGION_SIZE];
    u8 initial_vertex[256], initial_command[64], initial_normal[64];
    u32 expected_ret, expected_cursor, expected_normals;
    s32 expected_count;

    prepare_region((u8 *)HOST_VERTEX, (u8 *)HOST_COMMAND, (u8 *)HOST_OUTPUT,
                   (u8 *)HOST_NORMAL, (u8 *)HOST_OT, f);
    memcpy(initial_vertex, (void *)HOST_VERTEX, sizeof(initial_vertex));
    memcpy(initial_command, (void *)HOST_COMMAND, sizeof(initial_command));
    memcpy(initial_normal, (void *)HOST_NORMAL, sizeof(initial_normal));
    store_le(PSX_ADDR(0x8005953cu), 4, GUEST_VERTEX);
    store_le(PSX_ADDR(0x80059578u), 4, 7);
    store_le(PSX_ADDR(0x80059424u), 4, GUEST_OUTPUT);
    store_le(PSX_ADDR(0x80059568u), 4, GUEST_OT);
    store_le(PSX_ADDR(0x80059498u), 4, GUEST_NORMAL);
    store_le(PSX_ADDR(0x80050100u), 4, (u32)f->shift);
    store_le(PSX_ADDR(0x800500f8u), 4, f->xmax);
    store_le(PSX_ADDR(0x800500fcu), 4, f->ymax_packed);
    memset(PSX_ADDR(STACK - 64), 0xd3, 128);
    reset_gte(f);
    initialize_cpu(&cpu, &runtime);
    cpu.bus.read = fixture_read;
    cpu.bus.write = fixture_write;
    cpu.bus.bridge = NULL;
    cpu.gpr[4] = GUEST_COMMAND; cpu.gpr[5] = (u32)f->count;
    cpu.gpr[29] = STACK;
    for (unsigned i = 16; i <= 23; i++) cpu.gpr[i] = 0x6f000000u + i;
    if (PcPortMipsRun(&cpu, 0x8002f2e0u, BATTLE_HALT_PC, 5000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "MODEL PRIM F2E0 retail %s failed: %s\n", f->name, cpu.error);
        return 0;
    }
    assert(cpu.gpr[29] == STACK);
    for (unsigned i = 16; i <= 23; i++) assert(cpu.gpr[i] == 0x6f000000u + i);
    expected_ret = cpu.gpr[2];
    expected_cursor = load_le(PSX_ADDR(0x80059424u), 4);
    expected_normals = load_le(PSX_ADDR(0x80059498u), 4);
    expected_count = (s32)load_le(PSX_ADDR(0x80059578u), 4);
    memcpy(expected_output, (void *)(HOST_OUTPUT - 32), sizeof(expected_output));
    memcpy(expected_ot, (void *)HOST_OT, sizeof(expected_ot));
    retail_gte = gteRegs;
    if (!strcmp(f->name, "front-lit") &&
        (expected_count != 8 || ot_is_initial(expected_ot) ||
         (load_le(expected_output + 36, 4) & 0x00ffffffu) ==
             (f->color & 0x00ffffffu))) {
        fprintf(stderr, "MODEL PRIM F2E0 FAIL front-lit did not light/link/emit\n"); return 0;
    }
    if (!strcmp(f->name, "backface-partial-xy") &&
        (expected_count != 7 ||
         (!memcmp(expected_output + 40, "\xa5\xa5\xa5\xa5", 4)) ||
         (!memcmp(expected_output + 48, "\xa5\xa5\xa5\xa5", 4)))) {
        fprintf(stderr, "MODEL PRIM F2E0 FAIL backface branch/partial XY not reached\n"); return 0;
    }
    if (!strcmp(f->name, "otz-zero-normal-delay") &&
        (expected_count != 8 || !ot_is_initial(expected_ot) ||
         !memcmp(expected_output + 56, "\xa5\xa5\xa5\xa5", 4))) {
        fprintf(stderr, "MODEL PRIM F2E0 FAIL OTZ-zero branch not reached\n"); return 0;
    }
    if (!strcmp(f->name, "screen-edge") &&
        (expected_count != 7 || memcmp(expected_output + 40, "\xa5\xa5\xa5\xa5", 4))) {
        fprintf(stderr, "MODEL PRIM F2E0 FAIL screen reject branch not reached\n"); return 0;
    }

    prepare_region((u8 *)HOST_VERTEX, (u8 *)HOST_COMMAND, (u8 *)HOST_OUTPUT,
                   (u8 *)HOST_NORMAL, (u8 *)HOST_OT, f);
    D_8005953C = HOST_VERTEX; D_80059578 = 7;
    D_80059424 = (u8 *)HOST_OUTPUT; D_80059568 = HOST_OT;
    D_80059498 = HOST_NORMAL; D_80050100 = f->shift;
    D_800500F8 = (s32)f->xmax; D_800500FC = (s32)f->ymax_packed;
    reset_gte(f);
    u32 actual_ret = (u32)func_8002F2E0((u8 *)HOST_COMMAND, f->count);

    if (actual_ret != expected_ret || actual_ret != f->ymax_packed ||
        (u32)(uintptr_t)D_80059424 != expected_cursor ||
        D_80059498 != expected_normals ||
        D_80059578 != expected_count ||
        !compare_bytes(f->name, "packet/guards", (u8 *)HOST_OUTPUT - 32,
                       expected_output, sizeof(expected_output)) ||
        !compare_bytes(f->name, "OT", (u8 *)HOST_OT, expected_ot, sizeof(expected_ot)) ||
        !compare_bytes(f->name, "vertices", (u8 *)HOST_VERTEX,
                       initial_vertex, sizeof(initial_vertex)) ||
        !compare_bytes(f->name, "commands", (u8 *)HOST_COMMAND,
                       initial_command, sizeof(initial_command)) ||
        !compare_bytes(f->name, "normals", (u8 *)HOST_NORMAL,
                       initial_normal, sizeof(initial_normal)) ||
        memcmp(&gteRegs, &retail_gte, sizeof(gteRegs))) {
        fprintf(stderr, "MODEL PRIM F2E0 FAIL %s ret=%08x/%08x cursor=%08x/%08x "
                "normal=%08x/%08x emitted=%d/%d gte=%s\n", f->name,
                actual_ret, expected_ret, (u32)(uintptr_t)D_80059424,
                expected_cursor, D_80059498, expected_normals, D_80059578,
                expected_count, memcmp(&gteRegs, &retail_gte, sizeof(gteRegs)) ? "DIFFER" : "equal");
        return 0;
    }
    return 1;
}

int main(void)
{
    FILE *disc = fopen("disc/SLUS_006.64", "rb");
    assert(disc && fseek(disc, 0x800, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(0x80010000u), 1, 0x476e4, disc) == 0x476e4);
    assert(fclose(disc) == 0);
    if (!func_8002F2E0) {
        fputs("MODEL PRIM F2E0 FAIL missing production owner\n", stderr);
        return 1;
    }
    map_native(HOST_VERTEX); map_native(HOST_COMMAND); map_native(HOST_OUTPUT_MAP);
    map_native(HOST_NORMAL); map_native(HOST_OT);
    assert((HOST_VERTEX | HOST_COMMAND | HOST_OUTPUT | HOST_NORMAL | HOST_OT) < 0x01000000u);
    for (unsigned i = 0; i < sizeof(fixtures) / sizeof(fixtures[0]); i++)
        if (!run_fixture(&fixtures[i])) return 1;
    printf("MODEL PRIM F2E0 RETAIL PASS cases=%zu full-packet/guards/OT/globals/cursors/GTE/return\n",
           sizeof(fixtures) / sizeof(fixtures[0]));
    return 0;
}
