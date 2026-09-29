/* OT contract test, not a visual/GTE hardware oracle. Both wrappers use the
 * same production matrix helpers. A test-only projection boundary supplies
 * controlled XY/depth/FLAG results to cover every sort bucket and rejection.
 * No test boundary is compiled into the game. */
#include "common.h"
#include "field/main.h"
#include "field/particles.h"
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include "psx/gtereg.h"
#include <inline_c.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

u8 g_PsxRam[PSX_RAM_SIZE];
RenderContext *g_FieldCurRenderContext;
int g_FieldCurRenderContextIndex;
s32 D_80050100;
extern void FieldParticleRender(ParticlePrimitive *, MATRIX *, s16, s32, VECTOR *, s32);
extern void FieldMatrixCopyTransform(MATRIX *, MATRIX *);

#define CONTEXT 0x80160020u
#define PARTICLE 0x80180020u
#define MATRIX_IN 0x80190000u
#define SCALE_IN 0x80190100u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu
#define CONTEXT_BYTES (sizeof(RenderContext) + 0x40)
static u8 expected_context[CONTEXT_BYTES];
static s32 projection_depth;
static u32 projection_flag;
static unsigned projection_calls;
static const u32 xy[4] = {0x00100020, 0x0030fff0, 0xffe00040, 0xffd0ffc0};

/* Projection results are deliberately isolated from the ordering contract.
 * Native long outputs and guest word outputs have different ABI widths. */
long RotAverage4(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
                 long *x0, long *x1, long *x2, long *x3, long *p, long *flag)
{
    assert(b == a + 1 && c == a + 2 && d == a + 3 && p == flag);
    long *outputs[] = {x0, x1, x2, x3};
    for (unsigned i = 0; i < 4; i++) memcpy(outputs[i], &xy[i], 4);
    *p = 17;
    *flag = (s32)projection_flag;
    projection_calls++;
    return projection_depth;
}

static u8 *address_bytes(u32 address, unsigned width)
{
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (address >= base && (uint64_t)address + width <= base + sizeof(g_PsxRam))
        return (u8 *)(uintptr_t)address;
    if (address >= 0x80000000u && (uint64_t)address + width <= 0x80200000u)
        return PSX_ADDR(address);
    return NULL;
}
static int read_bus(void *context, u32 address, unsigned width, u32 *value)
{
    (void)context;
    const u8 *p = address_bytes(address, width);
    if (!p) return -1;
    *value = 0;
    for (unsigned i = 0; i < width; i++) *value |= (u32)p[i] << (i * 8);
    return 0;
}
static int write_bus(void *context, u32 address, unsigned width, u32 value)
{
    (void)context;
    u8 *p = address_bytes(address, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; i++) p[i] = value >> (i * 8);
    return 0;
}
static u32 word(u32 address)
{ u32 value; assert(read_bus(NULL, address, 4, &value) == 0); return value; }
static void put(u32 address, u32 value)
{ assert(write_bus(NULL, address, 4, value) == 0); }
static void *pointer(u32 address)
{ void *p = address_bytes(address, 1); assert(p); return p; }

static int dependency_bridge(void *context, PcPortMipsCpu *cpu, u32 target)
{
    (void)context;
    switch (target) {
    case 0x8003f738:
        cpu->gpr[2] = (uintptr_t)RotMatrix(pointer(cpu->gpr[4]), pointer(cpu->gpr[5]));
        return 1;
    case 0x80049dcc:
        cpu->gpr[2] = (uintptr_t)ScaleMatrix(pointer(cpu->gpr[4]), pointer(cpu->gpr[5]));
        return 1;
    case 0x8004931c:
        cpu->gpr[2] = (uintptr_t)CompMatrix(pointer(cpu->gpr[4]), pointer(cpu->gpr[5]), pointer(cpu->gpr[6]));
        return 1;
    case 0x8007409c:
        FieldMatrixCopyTransform(pointer(cpu->gpr[4]), pointer(cpu->gpr[5]));
        return 1;
    case 0x80049f8c: SetTransMatrix(pointer(cpu->gpr[4])); return 1;
    case 0x80049efc: SetRotMatrix(pointer(cpu->gpr[4])); return 1;
    case 0x8004a7bc:
        assert(cpu->gpr[5] == cpu->gpr[4] + 8 && cpu->gpr[6] == cpu->gpr[4] + 16 &&
               cpu->gpr[7] == cpu->gpr[4] + 24);
        for (unsigned i = 0; i < 4; i++) put(word(cpu->gpr[29] + 16 + i * 4), xy[i]);
        assert(word(cpu->gpr[29] + 32) == word(cpu->gpr[29] + 36));
        put(word(cpu->gpr[29] + 32), 17);
        put(word(cpu->gpr[29] + 36), projection_flag);
        cpu->gpr[2] = projection_depth;
        projection_calls++;
        return 1;
    default: return 0;
    }
}

static void reset_buffers(void)
{
    memset(PSX_ADDR(CONTEXT - 0x20), 0xa5, CONTEXT_BYTES);
    memset(PSX_ADDR(PARTICLE - 0x20), 0x5a, 0x100);
    ParticlePrimitive *p = PSX_ADDR(PARTICLE);
    p->position.vx = -8192; p->position.vy = 12288; p->position.vz = 16384;
    p->scale.vx = 512; p->scale.vy = 1024; p->scale.vz = 2048;
    p->color.r = 0x13; p->color.g = 0x57; p->color.b = 0x9b;
    memset(&gteRegs, 0, sizeof(gteRegs));
    projection_calls = 0;
}

static int compare(s32 depth, unsigned shift, s32 sort, unsigned index, s32 type, u32 flag)
{
    u8 expected_particle[0x100];
    GTERegisters expected_gte;
    u32 expected_data[32], actual_data[32];
    projection_depth = depth; projection_flag = flag;
    D_80050100 = shift; g_FieldCurRenderContextIndex = index;
    g_FieldCurRenderContext = PSX_ADDR(CONTEXT);
    put(0x80050100, shift); put(0x800adb08, index);
    put(0x800c426c, (uintptr_t)g_FieldCurRenderContext);
    reset_buffers();
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus, .bridge = dependency_bridge};
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (uintptr_t)PSX_ADDR(PARTICLE);
    cpu.gpr[5] = (uintptr_t)PSX_ADDR(MATRIX_IN);
    cpu.gpr[6] = 0x123; cpu.gpr[7] = sort;
    cpu.gpr[29] = STACK; cpu.gpr[31] = HALT;
    put(STACK + 16, (uintptr_t)PSX_ADDR(SCALE_IN)); put(STACK + 20, type);
    if (PcPortMipsRun(&cpu, 0x800a9b54, HALT, 1000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "PARTICLE OT retail failed: %s\n", cpu.error);
        return 0;
    }
    assert(projection_calls == 1 && cpu.gpr[29] == STACK);
    memcpy(expected_context, PSX_ADDR(CONTEXT - 0x20), CONTEXT_BYTES);
    memcpy(expected_particle, PSX_ADDR(PARTICLE - 0x20), 0x100);
    /* CompMatrix's SVECTOR padding can enter the backing VZ word. INLINE_C.C
     * MFC2 sign-extends VZ's low halfword on read; compare observable register
     * values, not the uninitialized high-half padding of that native helper. */
    for (unsigned i = 0; i < 32; i++) expected_data[i] = MFC2(i);
    expected_gte = gteRegs;
    reset_buffers();
    FieldParticleRender(PSX_ADDR(PARTICLE), PSX_ADDR(MATRIX_IN), 0x123,
                        sort, PSX_ADDR(SCALE_IN), type);
    assert(projection_calls == 1);
    const u8 *actual = PSX_ADDR(CONTEXT - 0x20);
    if (memcmp(actual, expected_context, CONTEXT_BYTES) != 0) {
        for (unsigned i = 0; i < CONTEXT_BYTES; i++) {
            if (actual[i] != expected_context[i]) {
                fprintf(stderr, "PARTICLE OT depth=%d shift=%u sort=%d buffer=%u type=%d context %+d native=%02x retail=%02x\n",
                    depth, shift, sort, index, type, (int)i - 0x20, actual[i], expected_context[i]);
                break;
            }
        }
        return 0;
    }
    assert(memcmp(PSX_ADDR(PARTICLE - 0x20), expected_particle, 0x100) == 0);
    for (unsigned i = 0; i < 32; i++) actual_data[i] = MFC2(i);
    if (memcmp(actual_data, expected_data, sizeof(actual_data)) != 0 ||
        memcmp(&gteRegs.CP2C, &expected_gte.CP2C, sizeof(gteRegs.CP2C)) != 0) {
        fprintf(stderr, "PARTICLE OT matrix state depth=%d shift=%u sort=%d buffer=%u type=%d\n",
                depth, shift, sort, index, type);
        const u32 *actual_words = actual_data;
        const u32 *expected_words = expected_data;
        for (unsigned i = 0; i < 32; i++)
            if (actual_words[i] != expected_words[i])
                fprintf(stderr, "  CP2D[%u] native=%08x retail=%08x\n", i, actual_words[i], expected_words[i]);
        actual_words = (const u32 *)&gteRegs.CP2C;
        expected_words = (const u32 *)&expected_gte.CP2C;
        for (unsigned i = 0; i < sizeof(gteRegs.CP2C) / 4; i++)
            if (actual_words[i] != expected_words[i])
                fprintf(stderr, "  CP2C[%u] native=%08x retail=%08x\n", i, actual_words[i], expected_words[i]);
        return 0;
    }
    return 1;
}

int main(void)
{
    assert((uintptr_t)g_PsxRam + sizeof(g_PsxRam) < 0x1000000u);
    MTC2(0x005a0004, 1); assert(MFC2(1) == 4);
    MTC2(5, 1); assert(MFC2(1) != 4); /* A meaningful VZ change remains visible. */
    FILE *file = fopen("disc/field.bin", "rb");
    assert(file && fseek(file, 0x800a9b54u - 0x8006faf0u, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(0x800a9b54u), 1, 0x3c4, file) == 0x3c4);
    assert(fclose(file) == 0);
    MATRIX *matrix = PSX_ADDR(MATRIX_IN);
    memset(matrix, 0, sizeof(*matrix));
    matrix->m[0][0] = matrix->m[1][1] = matrix->m[2][2] = 4096;
    VECTOR *scale = PSX_ADDR(SCALE_IN);
    scale->vx = scale->vy = scale->vz = 4096;
    unsigned cases = 0;
    for (s32 depth = 0; depth <= 4112; depth++) {
        for (s32 sort = 0; sort < 4; sort++) {
            for (unsigned index = 0; index < 2; index++) {
                if (!compare(depth, 0, sort, index, depth & 3, 0x80001000u)) return 1;
                cases++;
            }
        }
    }
    const s32 edges[] = {INT32_MIN, -65536, -17, -1, 0, 1, 16, 17, 4095, 4096, INT32_MAX};
    for (unsigned shift = 0; shift < 32; shift++) {
        for (unsigned e = 0; e < sizeof(edges) / sizeof(edges[0]); e++) {
            for (s32 sort = 0; sort < 4; sort++) {
                for (unsigned index = 0; index < 2; index++) {
                    if (!compare(edges[e], shift, sort, index, shift & 3, 0)) return 1;
                    cases++;
                }
            }
        }
    }
    printf("FIELD PARTICLE RENDER OT RETAIL PASS cases=%u, full context/particle/guards, shared matrix backend, controlled projection\n", cases);
    return 0;
}
