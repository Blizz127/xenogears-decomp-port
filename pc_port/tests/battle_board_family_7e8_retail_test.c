/*
 * Retail certificate for the battle board/panel accessor family
 * 0x8007E934..0x8007EF6C (the 14 bodies landed so far; see
 * docs/ai_context/ACTIVE_HANDOFF.md for the group layout).
 *
 * Every entry runs twice on the same fixture: once as the retail slice on the
 * MIPS adapter (tables, scalars and the board pointer mapped to their retail
 * addresses) and once as the shipped C from src/battle/main*.c. The comparison
 * covers the returned value and the fixture bytes the function may touch.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "battle_mips_adapter.h"

typedef u32 (*FnP)(u8**);
typedef u32 (*FnPI)(u8**, u8);
typedef u32 (*FnV)(void);
typedef u32 (*FnI)(u8);

extern u32 func_8007E934(void);
extern u32 func_8007E954(u8**, u8);
extern u32 func_8007E9D0(u8**, u8);
extern u32 func_8007EA4C(u8**, u8);
extern u32 func_8007EAC8(u8**, u8);
extern u32 func_8007EB50(u8**, u8);
extern u32 func_8007EBD8(u8**, u8);
extern u32 func_8007EC54(u8**, u8);
extern u32 func_8007ECDC(u8**, u8);
extern u32 func_8007ED58(u8**, u8);
extern u32 func_8007EE70(u8**);
extern u32 func_8007EEA8(u8**);
extern u32 func_8007EED0(void);
extern u32 func_8007EF44(u8);
extern void func_80078508(void* pBuffer);
extern void func_8007B958(u8**, u8);
extern void func_8007E8AC(u8**);
extern void func_8007E8E0(u8**, u8);
extern u32 func_8007E98C(u8**, u8);
extern u32 func_8007EA08(u8**, u8);
extern u32 func_8007EA84(u8**, u8);
extern u32 func_8007EB08(u8**, u8);
extern u32 func_8007EB90(u8**, u8);
extern u32 func_8007EC10(u8**, u8);
extern u32 func_8007EC94(u8**, u8);
extern u32 func_8007ED14(u8**, u8);
extern u32 func_8007ED98(u8**, u8);
extern u32 func_8007EDE0(u8**, u8);
extern u32 func_8007EE28(u8**, u8);
extern u32 func_8007EEE8(void);
extern void func_8007B914(u8**, u8);
extern void func_8007E780(u8**, u8);

/* Retail places these three symbols 0x10 apart (0x800D3410 / 0x800D3420 /
 * 0x800D3430); keep that spacing so one contiguous mapping serves them all. */
/* One fixture block with the three retail symbols pinned at their 0x10 spacing
 * (0x800D3410 / 0x800D3420 / 0x800D3430); declaration order is not stable
 * across optimisation levels, so the offsets are explicit. */
u8 s_tableBlock[0x120];
__asm__(".globl D_800D3410\n.set D_800D3410, s_tableBlock + 0x00\n");
__asm__(".globl D_800D3420\n.set D_800D3420, s_tableBlock + 0x10\n");
__asm__(".globl D_800D3430\n.set D_800D3430, s_tableBlock + 0x20\n");
extern u8 D_800D3410[];
extern u8 D_800D3420[];
extern u8 D_800D3430[];
u16 D_800CCD64[0x300]; /* 0x170-byte entries x 2 + slack */
u8 D_800D301C[0x100];
u8 D_800C3EB7[0x200];
u16 D_800D39DC;
u8 D_800D2DC0;
u16 D_800C3448[0x40];
extern u32 func_80079E7C(u16 v);
u8 D_800D3364Buf[0x800];
u16 D_8005A3A0[0x100];
u8* D_800D3364 = D_800D3364Buf;

static u8 s_ram[0x200000];
static unsigned s_checks;
static unsigned s_spyCalls;
static u8 s_board[8];
static u8* s_boardPtr = s_board;

void func_80078508(void* pBuffer)
{
    s_spyCalls++;
    memset(pBuffer, 0x5A, 0x10);
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

static u8* mapped(u32 a, unsigned w)
{
    /* host fixtures the emulated code reaches through truncatable pointers
     * (the binary is linked -no-pie, so the low 32 bits round-trip) */
    uintptr_t lo = (uintptr_t)s_board;
    uintptr_t loPtr = (uintptr_t)&s_boardPtr;

    if ((uintptr_t)a >= lo && (uint64_t)a + w <= lo + sizeof(s_board)) {
        return (u8*)(uintptr_t)a;
    }
    if ((uintptr_t)a >= loPtr && (uint64_t)a + w <= loPtr + sizeof(s_boardPtr)) {
        return (u8*)(uintptr_t)a;
    }
    {
        /* 0x800D3410 / 0x800D3420 / 0x800D3430 are 0x10 apart in retail; the
         * three fixture objects above reproduce that spacing. */
        if (a >= 0x800D3410u && (uint64_t)a + w <= 0x800D3410u + sizeof(s_tableBlock)) {
            return s_tableBlock + (a - 0x800D3410u);
        }
    }
    if (a >= 0x800CCD64u && (uint64_t)a + w <= 0x800CCD64u + sizeof(D_800CCD64)) {
        return (u8*)D_800CCD64 + (a - 0x800CCD64u);
    }
    if (a >= 0x800D301Cu && (uint64_t)a + w <= 0x800D301Cu + sizeof(D_800D301C)) {
        return (u8*)D_800D301C + (a - 0x800D301Cu);
    }
    if (a >= 0x800C3EB7u && (uint64_t)a + w <= 0x800C3EB7u + sizeof(D_800C3EB7)) {
        return (u8*)D_800C3EB7 + (a - 0x800C3EB7u);
    }
    if (a == 0x800D39DCu && w <= 2) {
        return (u8*)&D_800D39DC;
    }
    if (a == 0x800D2DC0u && w == 1) {
        return &D_800D2DC0;
    }
    if (a == 0x800D3364u && w <= 4) {
        return (u8*)&D_800D3364;
    }
    if (a >= 0x8005A3A0u && (uint64_t)a + w <= 0x8005A3A0u + sizeof(D_8005A3A0)) {
        return (u8*)D_8005A3A0 + (a - 0x8005A3A0u);
    }
    {
        uintptr_t lo = (uintptr_t)D_800D3364Buf;
        if ((uintptr_t)a >= lo && (uint64_t)a + w <= lo + sizeof(D_800D3364Buf)) {
            return (u8*)(uintptr_t)a;
        }
    }
    return NULL;
}

static int rd(void* u, u32 a, unsigned w, u32* v)
{
    u8* p;
    unsigned i;

    (void)u;
    p = mapped(a, w);
    if (p != NULL) {
        return rd_bytes(p, w, v);
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
    u8* p;
    unsigned i;

    (void)u;
    p = mapped(a, w);
    if (p != NULL) {
        return wr_bytes(p, w, v);
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
    unsigned i;

    (void)u;
    if (t == 0x80079E7Cu) {
        c->gpr[2] = (u32)func_80079E7C((u16)c->gpr[4]);
        return 1;
    }
    if (t == 0x80078508u) {
        /* The guest buffer lives on the emulated stack: fill it in guest RAM
         * (the host spy above is only for the C run). */
        u32 address = c->gpr[4];

        s_spyCalls++;
        for (i = 0; i < 0x10; ++i) {
            s_ram[(address + i) & 0x1FFFFFu] = 0x5A;
        }
        c->gpr[2] = 0;
    } else {
        return 0;
    }
    return 1;
}

static u32 run_oracle(u32 entry, u32 a0, u32 a1)
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
    if (PcPortMipsRun(&cpu, entry, 0xFFFFFFFCu, 3000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "BOARD FAMILY FAIL oracle %s @%08x\n", cpu.error, entry);
        exit(1);
    }
    return cpu.gpr[2];
}

static void expect_eq(const char* field, u32 actual, u32 expected, unsigned seed,
                      unsigned index)
{
    s_checks++;
    if (actual != expected) {
        fprintf(stderr, "BOARD FAMILY FAIL %s seed=%u index=%u actual=%08x expected=%08x\n",
                field, seed, index, actual, expected);
        exit(1);
    }
}

int main(void)
{
    FILE* f;
    unsigned seed, index;

    f = fopen("disc/battle.bin", "rb");
    if (f == NULL) {
        fprintf(stderr, "BOARD FAMILY FAIL cannot open disc/battle.bin\n");
        return 1;
    }
    /* 0x8007E934..0x8007EF6C plus the row-copy setter at 0x8007B958 */
    if (fseek(f, 0x8007E934 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0x7E934, 1, 0x638, f) != 0x638 ||
        fseek(f, 0x8007B958 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0x7B958, 1, 0x34, f) != 0x34 ||
        fseek(f, 0x8007E8AC - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0x7E8AC, 1, 0x88, f) != 0x88 ||
        fseek(f, 0x8007B914 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0x7B914, 1, 0x44, f) != 0x44 ||
        fseek(f, 0x8007E780 - 0x8006FAF0, SEEK_SET) != 0 ||
        fread(s_ram + 0x7E780, 1, 0x40, f) != 0x40) {
        fprintf(stderr, "BOARD FAMILY FAIL cannot read the retail slice\n");
        return 1;
    }
    fclose(f);

    for (seed = 0; seed < 8; ++seed) {
        unsigned k;

        for (k = 0; k < 0x100; ++k) {
            {
                /* period 2 so that index and index+2 hit equal bytes (>= vs >) and
                 * index and index+2&3 can also be bit-disjoint (& vs |) */
                static const u8 pattern[4] = { 0x0F, 0xF0, 0x0F, 0xF0 };

                D_800D3430[k] = pattern[(k + seed) & 3];
            }
            ((u8*)D_800D3420)[k] = (u8)(0x10 + k * 3 + seed);
        }
        for (k = 0; k < 4; ++k) {
            ((u32*)D_800D3410)[k] = 0x40000000u + k * 0x101u + seed;
        }
        for (k = 0; k < sizeof(D_800CCD64) / 2; ++k) {
            D_800CCD64[k] = (u16)((k * 3 + seed) << 15);
        }
        for (k = 0; k < sizeof(D_800D301C); ++k) {
            D_800D301C[k] = (u8)(seed == 0 ? 0 : 1 + k);
        }
        for (k = 0; k < sizeof(D_8005A3A0) / 2; ++k) {
            D_8005A3A0[k] = (u16)(0x2000 + k * 7 + seed);
        }
        memset(D_800C3EB7, 0, sizeof(D_800C3EB7));
        /* 0x80 exactly at the offsets func_8007EF44 reads with `index + 3`
         * (0x1C stride) and 0 elsewhere: an all-equal fill would hide both the
         * `>> 7` and the `index + 3` mutants. */
        D_800C3EB7[0x54] = 0x80;
        D_800C3EB7[0x70] = 0x80;
        D_800C3EB7[0x8C] = 0x80;
        D_800C3EB7[0xA8] = 0x80;
        /* the 0x8007EEE8 scan walks the full 0x1C stride, so all eight slots
         * need the bit set for a stride mutant to change the result */
        D_800C3EB7[0xC4] = 0x80;
        D_800C3EB7[0xE0] = 0x80;
        D_800C3EB7[0xFC] = 0x80;
        D_800C3EB7[0x118] = 0x80;
        D_800D39DC = (u16)(seed * 0x21u);
        s_spyCalls = 0;
        /* keep every field inside the modelled tables: rows are 0x40 bytes,
         * the D_800CCD64 entries 0x170, and the board bytes at +1/+2 are the
         * row selectors. */

        /* func_8007E934: hands a 0x10-byte stack buffer to func_80078508. */
        s_spyCalls = 0;
        run_oracle(0x8007E934u, 0, 0);
        expect_eq("oracle.78508.calls", (u32)s_spyCalls, 1, seed, 0);
        s_spyCalls = 0;
        func_8007E934();
        expect_eq("c.78508.calls", (u32)s_spyCalls, 1, seed, 0);
    }

    for (seed = 0; seed < 8; ++seed) {
        unsigned k;

        for (k = 0; k < 0x10; ++k) {
            {
                /* period 2 so that index and index+2 hit equal bytes (>= vs >) and
                 * index and index+2&3 can also be bit-disjoint (& vs |) */
                static const u8 pattern[4] = { 0x0F, 0xF0, 0x0F, 0xF0 };

                D_800D3430[k] = pattern[(k + seed) & 3];
            }
            ((u8*)D_800D3420)[k] = (u8)(0x10 + k * 3 + seed);
        }
        for (k = 0; k < 4; ++k) {
            ((u32*)D_800D3410)[k] = 0x40000000u + k * 0x101u + seed;
        }
        for (k = 0; k < sizeof(D_800CCD64) / 2; ++k) {
            D_800CCD64[k] = (u16)((k * 3 + seed) << 15);
        }
        for (k = 0; k < sizeof(D_800D301C); ++k) {
            D_800D301C[k] = (u8)(seed == 0 ? 0 : 1 + k);
        }
        for (k = 0; k < sizeof(D_8005A3A0) / 2; ++k) {
            D_8005A3A0[k] = (u16)(0x2000 + k * 7 + seed);
        }
        memset(D_800C3EB7, 0, sizeof(D_800C3EB7));
        D_800C3EB7[0x54] = 0x80;
        D_800C3EB7[0x70] = 0x80;
        D_800C3EB7[0x8C] = 0x80;
        D_800C3EB7[0xA8] = 0x80;
        /* the 0x8007EEE8 scan walks the full 0x1C stride, so all eight slots
         * need the bit set for a stride mutant to change the result */
        D_800C3EB7[0xC4] = 0x80;
        D_800C3EB7[0xE0] = 0x80;
        D_800C3EB7[0xFC] = 0x80;
        D_800C3EB7[0x118] = 0x80;
        D_800D39DC = (u16)(seed * 0x21u);
        /* keep every field inside the modelled tables: rows are 0x40 bytes,
         * the D_800CCD64 entries 0x170, and the board bytes at +1/+2 are the
         * row selectors. */

        for (index = 0; index < 4; ++index) {
            u32 boardArg = (u32)(uintptr_t)&s_boardPtr;
            u32 a0 = boardArg, a1 = (u32)index;

            /* the board's row selectors follow the index under test so that
             * neighbouring pattern bytes are compared (catches & vs ^ etc.) */
            s_board[0] = (u8)(seed + 3);
            s_board[1] = (u8)index;
            s_board[2] = (u8)((index + 2) & 3);
            s_board[3] = (u8)(seed | 0x40);

            expect_eq("func_8007E954", func_8007E954(&s_boardPtr, (u8)index),
                      run_oracle(0x8007E954u, a0, a1), seed, index);
            expect_eq("func_8007E9D0", func_8007E9D0(&s_boardPtr, (u8)index),
                      run_oracle(0x8007E9D0u, a0, a1), seed, index);
            expect_eq("func_8007EA4C", func_8007EA4C(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EA4Cu, a0, a1), seed, index);
            expect_eq("func_8007EAC8", func_8007EAC8(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EAC8u, a0, a1), seed, index);
            expect_eq("func_8007EB50", func_8007EB50(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EB50u, a0, a1), seed, index);
            expect_eq("func_8007EBD8", func_8007EBD8(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EBD8u, a0, a1), seed, index);
            expect_eq("func_8007EC54", func_8007EC54(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EC54u, a0, a1), seed, index);
            expect_eq("func_8007ECDC", func_8007ECDC(&s_boardPtr, (u8)index),
                      run_oracle(0x8007ECDCu, a0, a1), seed, index);
            expect_eq("func_8007ED58", func_8007ED58(&s_boardPtr, (u8)index),
                      run_oracle(0x8007ED58u, a0, a1), seed, index);
            expect_eq("func_8007EE70", func_8007EE70(&s_boardPtr),
                      run_oracle(0x8007EE70u, a0, 0), seed, index);
            expect_eq("func_8007EEA8", func_8007EEA8(&s_boardPtr),
                      run_oracle(0x8007EEA8u, a0, 0), seed, index);
            expect_eq("func_8007EED0", func_8007EED0(),
                      run_oracle(0x8007EED0u, 0, 0), seed, index);
            {
                /* func_8007EF44 reads (index + 3) * 0x1C: clear the tail slots
                 * (which the 0x8007EEE8 scan needs set) so an `index + 4`
                 * mutation reads a different byte. */
                u8 tail[4];

                tail[0] = D_800C3EB7[0xC4];
                tail[1] = D_800C3EB7[0xE0];
                tail[2] = D_800C3EB7[0xFC];
                tail[3] = D_800C3EB7[0x118];
                D_800C3EB7[0xC4] = 0;
                D_800C3EB7[0xE0] = 0;
                D_800C3EB7[0xFC] = 0;
                D_800C3EB7[0x118] = 0;
                expect_eq("func_8007EF44", func_8007EF44((u8)index),
                          run_oracle(0x8007EF44u, (u32)index, 0), seed, index);
                D_800C3EB7[0xC4] = tail[0];
                D_800C3EB7[0xE0] = tail[1];
                D_800C3EB7[0xFC] = tail[2];
                D_800C3EB7[0x118] = tail[3];
            }
            expect_eq("func_8007E98C", func_8007E98C(&s_boardPtr, (u8)index),
                      run_oracle(0x8007E98Cu, a0, a1), seed, index);
            expect_eq("func_8007EA08", func_8007EA08(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EA08u, a0, a1), seed, index);
            expect_eq("func_8007EA84", func_8007EA84(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EA84u, a0, a1), seed, index);
            expect_eq("func_8007EB08", func_8007EB08(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EB08u, a0, a1), seed, index);
            expect_eq("func_8007EB90", func_8007EB90(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EB90u, a0, a1), seed, index);
            expect_eq("func_8007EC10", func_8007EC10(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EC10u, a0, a1), seed, index);
            expect_eq("func_8007EC94", func_8007EC94(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EC94u, a0, a1), seed, index);
            expect_eq("func_8007ED14", func_8007ED14(&s_boardPtr, (u8)index),
                      run_oracle(0x8007ED14u, a0, a1), seed, index);
            expect_eq("func_8007ED98", func_8007ED98(&s_boardPtr, (u8)index),
                      run_oracle(0x8007ED98u, a0, a1), seed, index);
            expect_eq("func_8007EDE0", func_8007EDE0(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EDE0u, a0, a1), seed, index);
            expect_eq("func_8007EE28", func_8007EE28(&s_boardPtr, (u8)index),
                      run_oracle(0x8007EE28u, a0, a1), seed, index);
            expect_eq("func_8007EEE8", func_8007EEE8(),
                      run_oracle(0x8007EEE8u, 0, 0), seed, index);
            {
                /* func_8007B914 stores the row word into D_8005A3A0[p[2]]. */
                u32 slot = (index + 2) & 3;
                u16 before = D_8005A3A0[slot];
                u16 oracleAfter, cAfter;

                run_oracle(0x8007B914u, a0, a1);
                oracleAfter = D_8005A3A0[slot];
                D_8005A3A0[slot] = before;
                func_8007B914(&s_boardPtr, (u8)index);
                cAfter = D_8005A3A0[slot];
                expect_eq("func_8007B914", cAfter, oracleAfter, seed, index);
            }
            {
                /* func_8007E8AC stores into the D_800D3364 buffer: compare the
                 * written byte, restoring it between the two runs. */
                u32 slotOff = ((u32)((index + 2) & 3) << 3) + ((u32)index << 6) + 0x140;
                u8 before = D_800D3364Buf[slotOff];
                u8 oracleAfter, cAfter;

                run_oracle(0x8007E8ACu, a0, 0);
                oracleAfter = D_800D3364Buf[slotOff];
                D_800D3364Buf[slotOff] = before;
                func_8007E8AC(&s_boardPtr);
                cAfter = D_800D3364Buf[slotOff];
                expect_eq("func_8007E8AC", cAfter, oracleAfter, seed, index);
            }
            {
                /* func_8007E8E0 passes the row word to func_80079E7C and stores
                 * the result + 1. func_80079E7C is now the shipped C body, so
                 * build a bit-per-index probe table: its return value is the
                 * lowest set bit of the row word, which pins the argument. */
                unsigned k;

                for (k = 0; k < 0x10; ++k) {
                    D_800C3448[k] = (u16)(1u << k);
                }
                D_800D2DC0 = 0xA5;
                run_oracle(0x8007E8E0u, a0, a1);
                {
                    u8 oracleByte = D_800D2DC0;

                    D_800D2DC0 = 0xA5;
                    func_8007E8E0(&s_boardPtr, (u8)index);
                    expect_eq("func_8007E8E0.byte", D_800D2DC0, oracleByte, seed, index);
                }
            }
        }
    }

    /* func_8007B958 copies row[p[1]] into row[p[2]]: distinct bytes here so a
     * swapped copy changes a different slot. */
    for (seed = 0; seed < 4; ++seed) {
        unsigned k;

        for (k = 0; k < 0x100; ++k) {
            ((u8*)D_800D3430)[k] = (u8)(0x10 * ((k & 3) + 1) + seed);
        }
        for (index = 0; index < 4; ++index) {
            u32 boardArg = (u32)(uintptr_t)&s_boardPtr;
            u32 dest = ((u32)index << 6) + ((index + 2) & 3);
            u32 src = ((u32)index << 6) + index;
            u8 before, oracleAfter, cAfter;

            s_board[1] = (u8)index;
            s_board[2] = (u8)((index + 2) & 3);
            before = ((u8*)D_800D3430)[dest];
            run_oracle(0x8007B958u, boardArg, (u32)index);
            oracleAfter = ((u8*)D_800D3430)[dest];
            expect_eq("oracle.8007B958.src", oracleAfter, ((u8*)D_800D3430)[src], seed, index);
            ((u8*)D_800D3430)[dest] = before;
            func_8007B958(&s_boardPtr, (u8)index);
            cAfter = ((u8*)D_800D3430)[dest];
            expect_eq("func_8007B958", cAfter, oracleAfter, seed, index);
        }
    }

    /* func_8007E780 divides the row word pRow[p[1]] by p[2] (row =
     * D_800D3410 + index*0x40, u32 slots). Run the oracle, snapshot the written
     * slot, restore it, then run the shipped C on the same fixture. */
    for (seed = 0; seed < 4; ++seed) {
        unsigned k;

        for (k = 0; k < sizeof(s_tableBlock); ++k) {
            s_tableBlock[k] = (u8)(0x31 * (k + 1) + seed);
        }
        for (index = 0; index < 4; ++index) {
            u32 boardArg = (u32)(uintptr_t)&s_boardPtr;
            unsigned slot = (seed + 1) & 3;
            unsigned div = 1 + ((seed + 2) & 3);
            u32 off = ((u32)index << 6) + slot * 4;
            u32 before, oracleAfter, cAfter;

            s_board[1] = (u8)slot;
            s_board[2] = (u8)div;
            before = *(u32*)((u8*)D_800D3410 + off);
            run_oracle(0x8007E780u, boardArg, (u32)index);
            oracleAfter = *(u32*)((u8*)D_800D3410 + off);
            expect_eq("oracle.8007E780.quotient", oracleAfter,
                      before / div, seed, index);
            *(u32*)((u8*)D_800D3410 + off) = before;
            func_8007E780(&s_boardPtr, (u8)index);
            cAfter = *(u32*)((u8*)D_800D3410 + off);
            expect_eq("func_8007E780", cAfter, oracleAfter, seed, index);
        }
    }

    printf("BOARD FAMILY 7E8 PASS checks=%u (retail slices on the MIPS adapter vs "
           "the shipped C)\n", s_checks);
    return 0;
}
