/*
 * Retail certificate for the first decompiled battle run,
 * 0x800BF6CC..0x800BF730 in src/battle/main.c:
 *
 *   func_800BF6CC  0x2C bytes  call func_800BD2E4 while D_800C3610 != 0
 *   func_800BF6F8  0x28 bytes  D_80059464 - func_800BF720()
 *   func_800BF720  0x10 bytes  (D_800D2D68 != 0)
 *
 * The three retail slices are read from disc/battle.bin and executed on the
 * MIPS adapter; the only bridged symbol is the out-of-overlay func_800BD2E4.
 * The shipped C from src/battle/main.c then runs over the same globals and both
 * are compared on return values and the observed call trace. (The matching
 * build already proves these bodies byte-identical - battle.bin keeps its pin -
 * so this test guards them against future toolchain drift.)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "battle_mips_adapter.h"

extern void func_800BF6CC(void);
extern u32 func_800BF6F8(void);
extern u32 func_800BF720(void);
extern u16 func_80089BEC(u8 index);
extern u16 func_80089C08(u8 index);
extern u32 func_80089C24(u8 index);
extern u32 func_80089C48(u8 index);
extern u32 func_80089C6C(u32 mask, u8 index);
extern u32 func_80089C9C(u32 mask, u8 index);
extern void func_8008AB4C(void);
extern void func_8008AB70(void);
extern void func_8008AB94(void);

u32 D_800C3610;
u32 D_80059464;
u32 D_800D2D68;
/* 16-entry u16 tables, 0x20 apart in retail (0x800C3468 / 0x800C3448). */
u16 D_800C3468[16];
u16 D_800C3448[16];

static u8 s_ram[0x200000];
static unsigned s_checks;

static unsigned s_bcCalls;
static int s_archiveCalls;
static int s_archiveDir;
static int s_archiveEntry;

void func_800BD2E4(void)
{
    s_bcCalls++;
}

int ArchiveSetIndex(int directoryIndex, int entryIndex)
{
    s_archiveCalls++;
    s_archiveDir = directoryIndex;
    s_archiveEntry = entryIndex;
    return 0;
}

static int rd_bytes(u8* p, unsigned w, u32* v)
{
    unsigned i;

    *v = 0;
    for (i = 0; i < w; ++i) {
        *v |= (u32)p[i] << (i * 8);
    }
    return 0;
}

static int wr_bytes(u8* p, unsigned w, u32 v)
{
    unsigned i;

    for (i = 0; i < w; ++i) {
        p[i] = (u8)(v >> (i * 8));
    }
    return 0;
}

static int rd(void* u, u32 a, unsigned w, u32* v)
{
    unsigned i;

    (void)u;
    if (a == 0x800C3610u && w <= 4) {
        return rd_bytes((u8*)&D_800C3610, w, v);
    }
    if (a == 0x80059464u && w <= 4) {
        return rd_bytes((u8*)&D_80059464, w, v);
    }
    if (a == 0x800D2D68u && w <= 4) {
        return rd_bytes((u8*)&D_800D2D68, w, v);
    }
    if (a >= 0x800C3468u && (uint64_t)a + w <= 0x800C3468u + sizeof(D_800C3468)) {
        return rd_bytes((u8*)D_800C3468 + (a - 0x800C3468u), w, v);
    }
    if (a >= 0x800C3448u && (uint64_t)a + w <= 0x800C3448u + sizeof(D_800C3448)) {
        return rd_bytes((u8*)D_800C3448 + (a - 0x800C3448u), w, v);
    }
    if (a < 0x80000000u || (uint64_t)a + w > 0x80200000u) {
        return -1;
    }
    *v = 0;
    for (i = 0; i < w; ++i) {
        *v |= (u32)s_ram[(a + i) & 0x1FFFFFu] << (i * 8);
    }
    return 0;
}

static int wr(void* u, u32 a, unsigned w, u32 v)
{
    unsigned i;

    (void)u;
    if (a == 0x800C3610u && w <= 4) {
        return wr_bytes((u8*)&D_800C3610, w, v);
    }
    if (a == 0x80059464u && w <= 4) {
        return wr_bytes((u8*)&D_80059464, w, v);
    }
    if (a == 0x800D2D68u && w <= 4) {
        return wr_bytes((u8*)&D_800D2D68, w, v);
    }
    if (a >= 0x800C3468u && (uint64_t)a + w <= 0x800C3468u + sizeof(D_800C3468)) {
        return wr_bytes((u8*)D_800C3468 + (a - 0x800C3468u), w, v);
    }
    if (a >= 0x800C3448u && (uint64_t)a + w <= 0x800C3448u + sizeof(D_800C3448)) {
        return wr_bytes((u8*)D_800C3448 + (a - 0x800C3448u), w, v);
    }
    if (a < 0x80000000u || (uint64_t)a + w > 0x80200000u) {
        return -1;
    }
    for (i = 0; i < w; ++i) {
        s_ram[(a + i) & 0x1FFFFFu] = (u8)(v >> (i * 8));
    }
    return 0;
}

static int bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    (void)u;
    if (t == 0x800BD2E4u) {
        func_800BD2E4();
        c->gpr[2] = 0;
    } else if (t == 0x80028470u) {
        c->gpr[2] = (u32)ArchiveSetIndex((int)c->gpr[4], (int)c->gpr[5]);
    } else {
        return 0;
    }
    return 1;
}

static u32 run_oracle2(u32 entry, u32 a0, u32 a1)
{
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;

    bus.read = rd;
    bus.write = wr;
    bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = a0;
    cpu.gpr[5] = a1;
    cpu.gpr[29] = 0x801FF000u;
    cpu.gpr[31] = 0xFFFFFFFCu;
    if (PcPortMipsRun(&cpu, entry, 0xFFFFFFFCu, 2000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "BATTLE LEAF FAIL oracle %s @%08x\n", cpu.error, entry);
        exit(1);
    }
    return cpu.gpr[2];
}

static u32 run_oracle(u32 entry)
{
    return run_oracle2(entry, 0, 0);
}

static void expect_eq(const char* field, u32 actual, u32 expected, unsigned seed)
{
    s_checks++;
    if (actual != expected) {
        fprintf(stderr, "BATTLE LEAF FAIL %s seed=%u actual=%08x expected=%08x\n",
                field, seed, actual, expected);
        exit(1);
    }
}

int main(void)
{
    FILE* f;
    unsigned seed;

    f = fopen("disc/battle.bin", "rb");
    if (f == NULL) {
        fprintf(stderr, "BATTLE LEAF FAIL cannot open disc/battle.bin\n");
        return 1;
    }
    /* battle vram 0x8006FAF0 == file 0x0 */
    if (fseek(f, 0x800BF6CC - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF6CC, 1, 0x2C, f) != 0x2C ||
        fseek(f, 0x800BF6F8 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF6F8, 1, 0x28, f) != 0x28 ||
        fseek(f, 0x800BF720 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0xBF720, 1, 0x10, f) != 0x10 ||
        fseek(f, 0x80089BEC - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0x89BEC, 1, 0xE0, f) != 0xE0 ||
        fseek(f, 0x8008AB4C - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0x8AB4C, 1, 0x6C, f) != 0x6C) {
        fprintf(stderr, "BATTLE LEAF FAIL cannot read the retail slices\n");
        return 1;
    }
    fclose(f);

    /* 0x80089BEC..0x80089CCC: two 16-entry u16 tables behind byte indices. */
    for (seed = 0; seed < 16; ++seed) {
        static const u32 indices[] = { 0, 1, 15, 16, 0x7F, 0xFF };
        unsigned k;

        for (k = 0; k < 16; ++k) {
            D_800C3468[k] = (u16)(0x1000u + k * 0x111u + seed);
            D_800C3448[k] = (u16)(0x8000u + k * 0x37u + seed);
        }
        for (k = 0; k < 6; ++k) {
            u8 index = (u8)indices[k];
            u32 mask = 0x1234u + seed;

            /* The plain readers have no range guard: only the 16 defined
             * entries are comparable (retail would read neighbouring memory). */
            if (index < 0x10) {
                expect_eq("c.89BEC", func_80089BEC(index),
                          run_oracle2(0x80089BEC, index, 0), seed);
                expect_eq("c.89C08", func_80089C08(index),
                          run_oracle2(0x80089C08, index, 0), seed);
                expect_eq("c.89C24", func_80089C24(index),
                          run_oracle2(0x80089C24, index, 0), seed);
                expect_eq("c.89C48", func_80089C48(index),
                          run_oracle2(0x80089C48, index, 0), seed);
            }
            expect_eq("c.89C6C", func_80089C6C(mask, index),
                      run_oracle2(0x80089C6C, mask, index), seed);
            expect_eq("c.89C9C", func_80089C9C(mask, index),
                      run_oracle2(0x80089C9C, mask, index), seed);
        }
    }

    /* 0x8008AB4C..0x8008ABB8: archive directory selectors. */
    for (seed = 0; seed < 3; ++seed) {
        static const u32 entries[] = { 0, 2, 3 };
        static const u32 addrs[] = { 0x8008AB4C, 0x8008AB70, 0x8008AB94 };

        s_archiveCalls = 0;
        s_archiveDir = -1;
        s_archiveEntry = -1;
        run_oracle(addrs[seed]);
        expect_eq("oracle.archive.calls", (u32)s_archiveCalls, 1, seed);
        expect_eq("oracle.archive.dir", (u32)s_archiveDir, 0x20, seed);
        expect_eq("oracle.archive.entry", (u32)s_archiveEntry, entries[seed], seed);

        s_archiveCalls = 0;
        s_archiveDir = -1;
        s_archiveEntry = -1;
        if (seed == 0) {
            func_8008AB4C();
        } else if (seed == 1) {
            func_8008AB70();
        } else {
            func_8008AB94();
        }
        expect_eq("c.archive.calls", (u32)s_archiveCalls, 1, seed);
        expect_eq("c.archive.dir", (u32)s_archiveDir, 0x20, seed);
        expect_eq("c.archive.entry", (u32)s_archiveEntry, entries[seed], seed);
    }

    for (seed = 0; seed < 16; ++seed) {
        u32 oracle6cc;

        D_800C3610 = (seed & 1) ? (0x1000u + seed) : 0;
        D_80059464 = 0x12340000u + seed * 0x111;
        D_800D2D68 = (seed & 2) ? (seed * 0x1000u) : 0;

        s_bcCalls = 0;
        oracle6cc = run_oracle(0x800BF6CC);
        expect_eq("oracle.6cc.bcCalls", s_bcCalls, (seed & 1) ? 1u : 0u, seed);
        expect_eq("oracle.6cc.ret", oracle6cc, 0, seed);
        expect_eq("oracle.6f8.ret", run_oracle(0x800BF6F8), D_80059464 - (D_800D2D68 != 0), seed);
        expect_eq("oracle.720.ret", run_oracle(0x800BF720), D_800D2D68 != 0, seed);

        s_bcCalls = 0;
        func_800BF6CC();
        expect_eq("c.6cc.bcCalls", s_bcCalls, (seed & 1) ? 1u : 0u, seed);
        expect_eq("c.6f8.ret", func_800BF6F8(), D_80059464 - (D_800D2D68 != 0), seed);
        expect_eq("c.720.ret", func_800BF720(), D_800D2D68 != 0, seed);
    }

    printf("BATTLE LEAF BF6CC PASS checks=%u (retail slices on the MIPS adapter "
           "vs the shipped C; controlled func_800BD2E4)\n", s_checks);
    return 0;
}
