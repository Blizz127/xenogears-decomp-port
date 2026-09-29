/* Particle bank storage and lifetime versus the supplied field instructions.
 * Heap and primitive initialization are observed dependency boundaries; the
 * buffers below are synthetic ABI fixtures, never replacement game assets. */
#include "common.h"
#include "field/particles.h"
#include "battle_mips_adapter.h"
#include "psx_memory.h"
#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

u8 g_PsxRam[PSX_RAM_SIZE];
u8 native_bank_storage[0x440] __attribute__((aligned(8)));
asm(".globl g_FieldDefaultParticleBanks\n"
    ".set g_FieldDefaultParticleBanks, native_bank_storage + 0x20\n");
int g_FieldParticleBankIndex, g_FieldParticleCurActor;
short g_FieldParticleActorIDs[NUM_PARTICLES];
u_char g_FieldParticleStatuses[NUM_PARTICLES];
ParticleBank *g_FieldParticleBanks[NUM_PARTICLES];
extern void FieldInitializeDefaultParticleBanks(short);
extern int FieldInitializeParticleBanks(int);
extern void FieldParticlesStop(int), FieldParticlesStopBanks(int), FieldParticlesFree(int);

#define DEFAULTS 0x800b02ccu
#define BANK_INDEX 0x800b0044u
#define CUR_ACTOR 0x800adb44u
#define STATUSES 0x800b14b0u
#define ACTORS 0x800b0108u
#define BANKS 0x800c3918u
#define HEAP 0x80140000u
#define STACK 0x801ff000u
#define HALT 0xfffffffcu

typedef struct Boundary {
    u32 alloc_count, alloc_size[9], alloc_flags[9];
    u32 free_count, freed[9];
    u32 init_count, initialized[32][3];
    u32 user_count, user_tag, user_types;
} Boundary;
static Boundary boundary;

void *HeapAlloc(u_int size, u_int flags)
{
    unsigned index = boundary.alloc_count++;
    assert(index < 9 && size <= 0xfc0);
    boundary.alloc_size[index] = size;
    boundary.alloc_flags[index] = flags;
    return PSX_ADDR(HEAP + index * 0x1000 + 0x20);
}
u_int HeapFree(void *pointer)
{
    assert(boundary.free_count < 9);
    boundary.freed[boundary.free_count++] = (uintptr_t)pointer;
    return 0;
}
void HeapChangeCurrentUser(u_int tag, char **types)
{
    boundary.user_count++;
    boundary.user_tag = tag;
    boundary.user_types = (uintptr_t)types;
}
void FieldInitializeParticlePrimitive(void *particle, s32 shape, s32 abr)
{
    unsigned index = boundary.init_count++;
    assert(index < 32);
    boundary.initialized[index][0] = (uintptr_t)particle;
    boundary.initialized[index][1] = shape;
    boundary.initialized[index][2] = abr;
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
    for (unsigned i = 0; i < width; i++) *value |= (u32)p[i] << (8 * i);
    return 0;
}
static int write_bus(void *context, u32 address, unsigned width, u32 value)
{
    (void)context;
    u8 *p = address_bytes(address, width);
    if (!p) return -1;
    for (unsigned i = 0; i < width; i++) p[i] = value >> (8 * i);
    return 0;
}
static u32 word(u32 address)
{ u32 value; assert(read_bus(NULL, address, 4, &value) == 0); return value; }
static void put(u32 address, u32 value)
{ assert(write_bus(NULL, address, 4, value) == 0); }

static int dependency_bridge(void *context, PcPortMipsCpu *cpu, u32 target)
{
    (void)context;
    switch (target) {
    case 0x80031bdc:
        cpu->gpr[2] = (uintptr_t)HeapAlloc(cpu->gpr[4], cpu->gpr[5]);
        return 1;
    case 0x800320e8:
        cpu->gpr[2] = HeapFree((void *)(uintptr_t)cpu->gpr[4]);
        return 1;
    case 0x80032498:
        HeapChangeCurrentUser(cpu->gpr[4], (char **)(uintptr_t)cpu->gpr[5]);
        return 1;
    case 0x800a8eac:
        FieldInitializeParticlePrimitive((void *)(uintptr_t)cpu->gpr[4],
                                         cpu->gpr[5], cpu->gpr[6]);
        return 1;
    default:
        return 0;
    }
}
static s32 retail(u32 entry, s32 argument)
{
    PcPortMipsCpu cpu;
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus, .bridge = dependency_bridge};
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = argument;
    cpu.gpr[28] = 0x80059170u; cpu.gpr[29] = STACK; cpu.gpr[31] = HALT;
    if (PcPortMipsRun(&cpu, entry, HALT, 100000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "PARTICLE BANK retail %08x failed: %s\n", entry, cpu.error);
        assert(0);
    }
    assert(cpu.gpr[29] == STACK);
    return (s32)cpu.gpr[2];
}

static void equal_bytes(const u8 *native, const u8 *expected, size_t size)
{
    for (size_t i = 0; i < size; i++) {
        if (native[i] != expected[i]) {
            fprintf(stderr, "PARTICLE BANK byte +%zx native=%02x retail=%02x; native bank size=%zx\n",
                    i, native[i], expected[i], sizeof(ParticleBank));
            assert(0);
        }
    }
}

typedef struct Snapshot {
    Boundary calls;
    s32 result, actor, index;
    u8 defaults[0x440], heap[0x9000], statuses[64], actors[128];
    u32 banks[64];
} Snapshot;
static Snapshot expected, actual;

static void snapshot(Snapshot *out, int native, s32 result)
{
    memset(out, 0, sizeof(*out));
    out->calls = boundary;
    out->result = result;
    out->actor = native ? g_FieldParticleCurActor : (s32)word(CUR_ACTOR);
    out->index = native ? g_FieldParticleBankIndex : (s32)word(BANK_INDEX);
    memcpy(out->defaults, native ? native_bank_storage : PSX_ADDR(DEFAULTS - 0x20), sizeof(out->defaults));
    memcpy(out->heap, PSX_ADDR(HEAP), sizeof(out->heap));
    memcpy(out->statuses, native ? g_FieldParticleStatuses : PSX_ADDR(STATUSES), sizeof(out->statuses));
    memcpy(out->actors, native ? g_FieldParticleActorIDs : PSX_ADDR(ACTORS), sizeof(out->actors));
    for (unsigned i = 0; i < 64; i++)
        out->banks[i] = native ? (u32)(uintptr_t)g_FieldParticleBanks[i] : word(BANKS + i * 4);
}

static void reset(unsigned mask, unsigned free_slot)
{
    memset(&boundary, 0, sizeof(boundary));
    memset(PSX_ADDR(HEAP), 0x5a, 0x9000);
    memset(native_bank_storage, 0xa5, sizeof(native_bank_storage));
    memset(native_bank_storage + 0x20, 0, 0x3c0);
    for (unsigned i = 0; i < 8; i++) {
        u8 *bank = native_bank_storage + 0x20 + i * 0x78;
        u16 count = (mask & (1u << i)) ? 1 + i % 3 : 0;
        u16 shape = i - 4;
        u16 flags = 0xf880u | (i << 8);
        memcpy(bank + 6, &count, 2);
        memcpy(bank + 0x54, &shape, 2);
        memcpy(bank + 0x2a, &flags, 2);
        bank[4] = 77;
    }
    memcpy(PSX_ADDR(DEFAULTS - 0x20), native_bank_storage, sizeof(native_bank_storage));
    memset(g_FieldParticleStatuses, 2, sizeof(g_FieldParticleStatuses));
    if (free_slot < 64) g_FieldParticleStatuses[free_slot] = 0;
    memset(g_FieldParticleActorIDs, 0x33, sizeof(g_FieldParticleActorIDs));
    memset(g_FieldParticleBanks, 0, sizeof(g_FieldParticleBanks));
    memcpy(PSX_ADDR(STATUSES), g_FieldParticleStatuses, sizeof(g_FieldParticleStatuses));
    memcpy(PSX_ADDR(ACTORS), g_FieldParticleActorIDs, sizeof(g_FieldParticleActorIDs));
    memset(PSX_ADDR(BANKS), 0, 64 * 4);
    g_FieldParticleCurActor = -7; g_FieldParticleBankIndex = 3;
    put(CUR_ACTOR, -7); put(BANK_INDEX, 3);
}

int main(void)
{
    FILE *image = fopen("disc/field.bin", "rb");
    assert(image && (uintptr_t)g_PsxRam + sizeof(g_PsxRam) < UINT32_MAX);
    assert(fseek(image, 0x800a9274u - 0x8006faf0u, SEEK_SET) == 0);
    assert(fread(PSX_ADDR(0x800a9274u), 1, 0x8a8, image) == 0x8a8);
    fclose(image);
    const s16 actor_ids[] = {0, 1, 31, 127, 255, 256, 32767, -32768, -1};
    unsigned cases = 0;
    for (unsigned fill = 0; fill < 256; fill++) {
        s16 actor = actor_ids[fill % (sizeof(actor_ids) / sizeof(*actor_ids))];
        memset(native_bank_storage, fill, sizeof(native_bank_storage));
        memset(PSX_ADDR(DEFAULTS - 0x20), fill, sizeof(native_bank_storage));
        g_FieldParticleBankIndex = 7; put(BANK_INDEX, 7);
        retail(0x800a94a4u, actor);
        FieldInitializeDefaultParticleBanks(actor);
        equal_bytes(native_bank_storage, PSX_ADDR(DEFAULTS - 0x20), sizeof(native_bank_storage));
        assert(g_FieldParticleBankIndex == (s32)word(BANK_INDEX));
        cases++;
    }
    assert(sizeof(ParticleBank) == 0x78 && sizeof(ParticlePrimitive) == 0xc0);
    assert(offsetof(ParticleBank, pPrimitives) == 0x2c);
    assert(offsetof(ParticleBank, directions) == 0x30);
    assert(offsetof(ParticleBank, targetActorID) == 0x52);
    assert(offsetof(ParticleBank, scale) == 0x5a);
    assert(offsetof(ParticleBank, color) == 0x6a);
    assert(offsetof(ParticleBank, rotAngle) == 0x76);
    assert(offsetof(ParticlePrimitive, poly[1]) == 0x78);
    assert(offsetof(ParticlePrimitive, vertices) == 0xa0);
    for (unsigned mask = 0; mask < 256; mask++) {
        unsigned slot = mask % 64;
        for (unsigned operation = 0; operation < 4; operation++) {
            reset(mask, slot);
            s32 result = retail(0x800a99a8u, -123);
            if (operation == 1) retail(0x800a9374u, slot);
            if (operation == 2) retail(0x800a93ccu, slot);
            if (operation == 3) retail(0x800a92acu, slot);
            snapshot(&expected, 0, result);
            reset(mask, slot);
            result = FieldInitializeParticleBanks(-123);
            if (operation == 1) FieldParticlesStop(slot);
            if (operation == 2) FieldParticlesStopBanks(slot);
            if (operation == 3) FieldParticlesFree(slot);
            snapshot(&actual, 1, result);
            equal_bytes((u8 *)&actual, (u8 *)&expected, sizeof(actual));
            cases++;
        }
    }
    reset(255, 64);
    snapshot(&expected, 0, retail(0x800a99a8u, 17));
    reset(255, 64);
    snapshot(&actual, 1, FieldInitializeParticleBanks(17));
    equal_bytes((u8 *)&actual, (u8 *)&expected, sizeof(actual));
    cases++;
    printf("FIELD PARTICLE BANK RETAIL PASS cases=%u, defaults/layout/allocation/stop/free/guards\n", cases);
    return 0;
}
