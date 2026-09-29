/*
 * Retail certificate for func_80024730 (0x80024730-0x800248D4, 105 insns) in
 * src/slus_006.64/system/temp1.c.
 *
 * The retail slice runs on the MIPS adapter with jtbl_800186A4 mapped from the
 * retail disc image (so the case targets are retail bytes, including the
 * 0x80024788 entry that the built jump table placeholder cannot express), and
 * the three reached callees (func_800BC158, TimerWorkListSetTaskCallback,
 * func_80025224) bridged to controlled host stubs. The shipped C then runs over
 * the same fixture with the same stubs, and both are compared on:
 *   - the full call sequence with every argument (the case-7 callback is
 *     normalised between the retail address 0x80022E8C and the host symbol),
 *   - a fixture snapshot taken at each call,
 *   - the final fixture memory.
 * State word bits 13-16 are swept over all 14 dispatched modes plus the two
 * ignored values (sixteen values total).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "battle_mips_adapter.h"

extern void func_80024730(u8* pOwner);
extern void func_80022E8C(void);

u32 D_8006F99C[4];
u32 D_8006F9AC[4];

/* func_80022E8C (this TU) is only address-taken here; give its body's two
 * references a home so the real symbol can stay the one the C passes. */
s32 D_800592EC;
void func_80022DF4(void) {}

static u8 s_ram[0x200000];
static u8 s_jtbl[0x40];

static u8 owner[0x100], initial[0x100], expectedMem[0x100];

struct Call {
    u32 kind;
    u32 args[3];
    u8 memory[sizeof(owner)];
};

static struct Call calls[4], wantCalls[4];
static unsigned nCalls;

/* 0x80022E8C is `func_80022E8C`'s retail address; the C passes the host
 * symbol's address for the same function, so normalise both to the address. */
static u32 callbackToken(const void* p)
{
    if (p == NULL) {
        return 0;
    }
    if ((u32)(uintptr_t)p == 0x80022E8Cu ||
        (uintptr_t)p == (uintptr_t)(void*)&func_80022E8C) {
        return 0x80022E8Cu;
    }
    return (u32)(uintptr_t)p;
}

static void record(u32 kind, u32 a, u32 b, u32 c)
{
    struct Call* p;

    if (nCalls >= 4) {
        fprintf(stderr, "ANIM DISPATCH FAIL call overflow\n");
        exit(1);
    }
    p = &calls[nCalls++];
    p->kind = kind;
    p->args[0] = a;
    p->args[1] = b;
    p->args[2] = c;
    memcpy(p->memory, owner, sizeof(owner));
}

static int s_bcCalls;

void func_800BC158(void* pOwner)
{
    s_bcCalls++;
    record(0, (u32)(uintptr_t)pOwner, 0, 0);
    ((u8*)pOwner)[0x41] ^= 0x33;
}

void TimerWorkListSetTaskCallback(void* pTask, void (*pCallback)(void*))
{
    record(1, (u32)(uintptr_t)pTask, callbackToken((const void*)pCallback), 0);
}

void func_80025224(void* pTask, int handlerIndex)
{
    record(2, (u32)(uintptr_t)pTask, (u32)handlerIndex, 0);
}

static u8* address(u32 a, unsigned w)
{
    uintptr_t lo = (uintptr_t)owner;

    if (a >= 0x800186A4u && (uint64_t)a + w <= 0x800186A4u + sizeof(s_jtbl)) {
        return s_jtbl + (a - 0x800186A4u);
    }
    if (a >= 0x8006F99Cu && (uint64_t)a + w <= 0x8006F99Cu + 0x10u) {
        return (u8*)D_8006F99C + (a - 0x8006F99Cu);
    }
    if (a >= 0x8006F9ACu && (uint64_t)a + w <= 0x8006F9ACu + 0x10u) {
        return (u8*)D_8006F9AC + (a - 0x8006F9ACu);
    }
    if (a >= 0x80000000u && (uint64_t)a + w <= 0x80200000u) {
        return s_ram + (a & 0x1FFFFFu);
    }
    if (a >= lo && (uint64_t)a + w <= lo + sizeof(owner)) {
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
    if (t == 0x800BC158u) {
        func_800BC158((void*)(uintptr_t)c->gpr[4]);
    } else if (t == 0x8001CD6Cu) {
        TimerWorkListSetTaskCallback((void*)(uintptr_t)c->gpr[4],
                                     (void (*)(void*))(uintptr_t)c->gpr[5]);
    } else if (t == 0x80025224u) {
        func_80025224((void*)(uintptr_t)c->gpr[4], (int)c->gpr[5]);
    } else {
        return 0;
    }
    return 1;
}

static void fail(const char* what, unsigned mode, unsigned seed, unsigned idx)
{
    fprintf(stderr, "ANIM DISPATCH FAIL %s mode=%u seed=%u at=%u\n", what, mode,
            seed, idx);
    exit(1);
}

int main(void)
{
    FILE* f;
    unsigned cases = 0;
    unsigned mode, seed, i;

    f = fopen("disc/SLUS_006.64", "rb");
    if (f == NULL) {
        fprintf(stderr, "ANIM DISPATCH FAIL cannot open disc/SLUS_006.64\n");
        return 1;
    }
    if (fseek(f, 0x80024730 - 0x8000F800, SEEK_SET) != 0 ||
        fread(s_ram + 0x24730, 1, 0x1A4, f) != 0x1A4 ||
        fseek(f, 0x800186A4 - 0x8000F800, SEEK_SET) != 0 ||
        fread(s_jtbl, 1, 0x3C, f) != 0x3C) {
        fprintf(stderr, "ANIM DISPATCH FAIL cannot read the retail slice\n");
        return 1;
    }
    fclose(f);

    /* Every dispatched target must be a label inside the retail slice. */
    for (i = 0; i < 15; ++i) {
        u32 target;
        memcpy(&target, s_jtbl + i * 4, 4);
        if (target < 0x80024730u || target >= 0x800248D4u) {
            fprintf(stderr, "ANIM DISPATCH FAIL jtbl[%u]=%08x outside slice\n", i,
                    target);
            return 1;
        }
    }

    for (mode = 0; mode < 16; ++mode)
        for (seed = 0; seed < 8; ++seed) {
            PcPortMipsBus bus;
            PcPortMipsCpu cpu;
            u32 flags;

            for (i = 0; i < sizeof(owner); ++i) {
                owner[i] = (u8)(0x5A + i * 3 + seed * 7);
            }
            D_8006F99C[0] = 0xAAAA0000u + seed;
            D_8006F99C[1] = 0xAAAA0100u + seed;
            D_8006F99C[2] = 0xAAAA0200u + seed;
            D_8006F9AC[0] = 0xBBBB0000u + seed;
            D_8006F9AC[1] = 0xBBBB0100u + seed;
            D_8006F9AC[2] = 0xBBBB0200u + seed;
            flags = 0x13570000u | 0x00010000u;
            flags = (flags & 0xFFFE1FFFu) | (mode << 13);
            memcpy(owner + 0x78, &flags, 4);

            memcpy(initial, owner, sizeof(owner));
            memset(calls, 0, sizeof(calls));
            nCalls = 0;
            s_bcCalls = 0;

            bus.read = rd;
            bus.write = wr;
            bus.bridge = bridge;
            PcPortMipsCpuInit(&cpu, &bus);
            cpu.gpr[4] = (u32)(uintptr_t)owner;
            cpu.gpr[29] = 0x801FF000u;
            cpu.gpr[31] = 0xFFFFFFFCu;
            if (PcPortMipsRun(&cpu, 0x80024730, 0xFFFFFFFCu, 4000) !=
                PC_PORT_MIPS_HALTED) {
                fprintf(stderr, "ANIM DISPATCH FAIL oracle %s\n", cpu.error);
                return 1;
            }
            memcpy(expectedMem, owner, sizeof(owner));
            memcpy(wantCalls, calls, sizeof(calls));

            memcpy(owner, initial, sizeof(owner));
            nCalls = 0;
            memset(calls, 0, sizeof(calls));
            func_80024730(owner);

            if (memcmp(owner, expectedMem, sizeof(owner)) != 0 ||
                memcmp(calls, wantCalls, sizeof(calls)) != 0) {
                unsigned off;
                for (off = 0; off < sizeof(owner); ++off) {
                    if (owner[off] != expectedMem[off]) {
                        fprintf(stderr,
                                "ANIM DISPATCH mem diff @%02x c=%02x o=%02x\n",
                                off, owner[off], expectedMem[off]);
                        break;
                    }
                }
                for (off = 0; off < 4; ++off) {
                    if (memcmp(&calls[off], &wantCalls[off], sizeof(calls[0])) !=
                        0) {
                        fprintf(stderr,
                                "ANIM DISPATCH call[%u] kind=%u/%u args=%08x,%08x"
                                " vs %08x,%08x\n",
                                off, calls[off].kind, wantCalls[off].kind,
                                calls[off].args[0], calls[off].args[1],
                                wantCalls[off].args[0], wantCalls[off].args[1]);
                        break;
                    }
                }
                fail("compare", mode, seed, off);
            }
            ++cases;
        }

    printf("ANIM DISPATCH 24730 PASS %u retail-oracle comparisons "
           "(memory/call-snapshot; retail jtbl; controlled callees)\n", cases);
    return 0;
}
