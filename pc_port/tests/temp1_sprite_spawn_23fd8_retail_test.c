/*
 * Retail certificate for func_80023FD8 (0x80023FD8-0x80024294, 175 insns)
 * in src/slus_006.64/system/temp1.c.
 *
 * The retail slice is loaded from disc/SLUS_006.64 and executed on the MIPS
 * adapter (pc_port/src/battle_mips_adapter.c) with the five callees the retail
 * body reaches bridged to controlled host stubs. The shipped C is then run
 * over the same fixture with the same stubs and the two are compared on:
 *   - the returned wrapper (low 32 bits, i.e. the PSX-visible value),
 *   - the full call sequence with every argument,
 *   - a memory snapshot taken at each call,
 *   - the final fixture memory.
 * Fixture layout mirrors the retail reads: package+0x10 -> script table,
 * table[index*2+2] -> entry byte offset, wrapper+0x38 -> node, node+0x24 ->
 * "source" word re-stored by the tail, optional D_800591AD / D_800C3E1C
 * player-state inheritance block.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "battle_mips_adapter.h"

extern u8* func_80023FD8(s32 index, u8* pAnimData, s16* pPosition, s32 extraSize);

u8 D_800591AD;
s32 D_800591A8;
u32 D_800C3E1C;

static u8 s_ram[0x200000];

static struct {
    u32 package[8];
    u32 alternate[8];
    u32 scripts[32];
    u32 wrapper[128];
    u32 parent[64];
    s16 position[4];
} fixture, initial, expected;

struct Call {
    u32 kind;
    u32 args[5];
    u8 memory[sizeof(fixture)];
};

static struct Call calls[5], wantCalls[5];
static unsigned nCalls;
static s32 typeValue;

static void record(u32 kind, u32 a, u32 b, u32 c, u32 d, u32 e)
{
    struct Call* p;

    if (nCalls >= 5) {
        fprintf(stderr, "SPRITE SPAWN FAIL call overflow\n");
        exit(1);
    }
    p = &calls[nCalls++];
    p->kind = kind;
    p->args[0] = a;
    p->args[1] = b;
    p->args[2] = c;
    p->args[3] = d;
    p->args[4] = e;
    memcpy(p->memory, &fixture, sizeof(fixture));
}

/* Controlled callees: same definitions feed both the C run and the MIPS
 * bridge, so the comparison isolates the unit under test (offsets, masks,
 * argument passing, ordering) rather than the callee bodies, which have their
 * own certificates. */
s32 func_80023440(void* pData)
{
    record(0, (u32)(uintptr_t)pData, 0, 0, 0, 0);
    return typeValue;
}

s32 func_80023468(s32 type, s32 arg1)
{
    record(1, (u32)type, (u32)arg1, 0, 0, 0);
    return (s32)(((u32)type ^ (u32)arg1) % 3u);
}

void* func_80023A48(s32 type, s32 mode, u8* pAnimData, s32 extraSize, u8* pCallback)
{
    record(2, (u32)type, (u32)mode, (u32)(uintptr_t)pAnimData,
           (u32)extraSize, (u32)(uintptr_t)pCallback);
    /* Publish the word the tail re-reads from node+0x24. */
    *(u32*)((u8*)&fixture.wrapper[0] + 0x5C) = (u32)(uintptr_t)&fixture.alternate;
    return &fixture.wrapper[0];
}

void func_80023538(void* pSpriteData, void* pAnimation)
{
    record(3, (u32)(uintptr_t)pSpriteData, (u32)(uintptr_t)pAnimation, 0, 0, 0);
    ((u8*)pSpriteData)[0x33] ^= 0x55;
}

void func_80024730(u8* pOwner)
{
    record(4, (u32)(uintptr_t)pOwner, 0, 0, 0, 0);
    ((u8*)pOwner)[0x37] ^= 0xAA;
}

static u8* address(u32 a, unsigned w)
{
    uintptr_t lo = (uintptr_t)&fixture;

    if (a == 0x800591ADu && w == 1) {
        return &D_800591AD;
    }
    if (a == 0x800591A8u && w <= 4) {
        return (u8*)&D_800591A8;
    }
    if (a == 0x800C3E1Cu && w <= 4) {
        return (u8*)&D_800C3E1C;
    }
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) {
        return s_ram + (a & 0x1FFFFFu);
    }
    if (a >= lo && (uint64_t)a + w <= lo + sizeof(fixture)) {
        return (u8*)(uintptr_t)a;
    }
    return NULL;
}

static int rd(void* u, u32 a, unsigned w, u32* v)
{
    u8* p;
    unsigned i;

    (void)u;
    p = address(a, w);
    if (p == NULL) {
        return -1;
    }
    *v = 0;
    for (i = 0; i < w; ++i) {
        *v |= (u32)p[i] << (i * 8);
    }
    return 0;
}

static int wr(void* u, u32 a, unsigned w, u32 v)
{
    u8* p;
    unsigned i;

    (void)u;
    p = address(a, w);
    if (p == NULL) {
        return -1;
    }
    for (i = 0; i < w; ++i) {
        p[i] = (u8)(v >> (i * 8));
    }
    return 0;
}

static int bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    (void)u;
    if (t == 0x80023440u) {
        c->gpr[2] = (u32)func_80023440((void*)(uintptr_t)c->gpr[4]);
    } else if (t == 0x80023468u) {
        c->gpr[2] = (u32)func_80023468((s32)c->gpr[4], (s32)c->gpr[5]);
    } else if (t == 0x80023A48u) {
        u32 owner;
        if (rd(NULL, c->gpr[29] + 16, 4, &owner) != 0) {
            return 0;
        }
        c->gpr[2] = (u32)(uintptr_t)func_80023A48(
            (s32)c->gpr[4], (s32)c->gpr[5], (u8*)(uintptr_t)c->gpr[6],
            (s32)c->gpr[7], (u8*)(uintptr_t)owner);
    } else if (t == 0x80023538u) {
        func_80023538((void*)(uintptr_t)c->gpr[4], (void*)(uintptr_t)c->gpr[5]);
    } else if (t == 0x80024730u) {
        func_80024730((u8*)(uintptr_t)c->gpr[4]);
    } else {
        return 0;
    }
    return 1;
}

static void fail(const char* what, s32 a, s32 b, unsigned c, unsigned d, unsigned e)
{
    fprintf(stderr, "SPRITE SPAWN FAIL %s a=%d b=%d c=%u d=%u e=%u\n",
            what, (int)a, (int)b, c, d, e);
    exit(1);
}

int main(void)
{
    FILE* f;
    unsigned cases = 0;
    static const s32 indices[] = { -1, 0, 1, 7 };
    unsigned typeIter, parent, enabled, seed, idx, i;

    f = fopen("disc/SLUS_006.64", "rb");
    if (f == NULL) {
        fprintf(stderr, "SPRITE SPAWN FAIL cannot open disc/SLUS_006.64\n");
        return 1;
    }
    if (fseek(f, 0x80023fd8 - 0x8000f800, SEEK_SET) != 0 ||
        fread(s_ram + 0x23fd8, 1, 700, f) != 700) {
        fprintf(stderr, "SPRITE SPAWN FAIL cannot read the retail slice\n");
        return 1;
    }
    fclose(f);

    for (typeIter = 0; typeIter < 16; ++typeIter)
        for (parent = 0; parent < 3; ++parent)
            for (enabled = 0; enabled < 2; ++enabled)
                for (seed = 0; seed < 8; ++seed)
                    for (idx = 0; idx < 4; ++idx) {
        PcPortMipsBus bus;
        PcPortMipsCpu cpu;
        u8* got;
        u32 parentPtr;
        s32 extra;

        for (i = 0; i < sizeof(fixture) / 4; ++i) {
            ((u32*)&fixture)[i] = 0x12481248u * (i + seed);
        }
        fixture.package[4] = (u32)(uintptr_t)fixture.scripts;
        for (i = 0; i < 9; ++i) {
            ((u16*)fixture.scripts)[i] = (u16)(32 + i * 4);
        }
        typeValue = (s32)typeIter;
        D_800591AD = enabled ? 0x80 : 0;
        D_800591A8 = (s32)(seed * 71317u);
        parentPtr = parent == 0 ? 0u
                  : parent == 1 ? (u32)(uintptr_t)fixture.parent
                  : (u32)(uintptr_t)((u8*)fixture.wrapper + 0x38);
        D_800C3E1C = parentPtr;
        extra = (s32)(0xa55a0018u + seed);

        initial = fixture;
        memset(calls, 0, sizeof(calls));
        nCalls = 0;

        bus.read = rd;
        bus.write = wr;
        bus.bridge = bridge;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = (u32)indices[idx];
        cpu.gpr[5] = (u32)(uintptr_t)fixture.package;
        cpu.gpr[6] = (u32)(uintptr_t)fixture.position;
        cpu.gpr[7] = (u32)extra;
        cpu.gpr[29] = 0x801FF000u;
        cpu.gpr[31] = 0xFFFFFFFCu;
        if (PcPortMipsRun(&cpu, 0x80023fd8, 0xFFFFFFFCu, 4000) != PC_PORT_MIPS_HALTED) {
            fprintf(stderr, "SPRITE SPAWN FAIL oracle %s\n", cpu.error);
            return 1;
        }
        expected = fixture;
        memcpy(wantCalls, calls, sizeof(calls));
        if (nCalls != 5) {
            fail("oracle-callcount", (s32)nCalls, 0, 0, 0, 0);
        }

        fixture = initial;
        nCalls = 0;
        memset(calls, 0, sizeof(calls));
        got = func_80023FD8(indices[idx], (u8*)fixture.package, fixture.position, extra);

        if ((u32)(uintptr_t)got != cpu.gpr[2] || nCalls != 5 ||
            memcmp(&fixture, &expected, sizeof(fixture)) != 0 ||
            memcmp(calls, wantCalls, sizeof(calls)) != 0) {
            fail("compare", (s32)typeValue, (s32)parent, enabled, seed, idx);
        }
        ++cases;
    }

    printf("SPRITE SPAWN 23FD8 PASS %u retail-oracle comparisons "
           "(return/memory/call-snapshot; controlled callees)\n", cases);
    return 0;
}
