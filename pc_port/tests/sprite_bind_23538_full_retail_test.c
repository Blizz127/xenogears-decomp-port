/*
 * Full-body retail-differential test for func_80023538 (sprite animation bind /
 * transform reset, 0x80023538-0x80023804) in src/slus_006.64/system/temp1.c.
 *
 * Unlike sprite_bind_transform_retail_test.c (helper call order + two marker
 * bytes), this compares the ENTIRE written guest state: sprite, transform base,
 * +0x7C state block, animation record, the globals, and an ordered call log of
 * the three reached callees (SpriteSetScale 0x80022000, SpriteComputeTransform
 * Matrix 0x80022090, func_800234AC) with a fixture snapshot at every call.
 * The callees are controlled stubs shared by both runs.
 *
 * Inputs: animation flag word (bits 0-1 -> A8 mode, 2-7 signed scale, 11/12/13
 * gates), +0x2/+0x4 script offsets, speed denominator +0xAC>>7, +0x82 and
 * D_80059198 scale terms, base/state pointers present or null, +0x3C mode 1
 * branch with +0x40 bit 20 and base+0x34, +0xA8 bit 0, flip/left-facing bits
 * (+0xAC bits 2/3, +0x3C bits 3/4) and D_800591AD gate.
 * Operand ranges avoid signed 32-bit overflow in the port's multiplications
 * (wrap is compared separately when T23538_WRAP=1; not part of the UBSan run).
 */
#include "sprite_retail_harness.h"

#include "common.h"

extern void func_80023538(void* pSpriteData, void* pAnimation);

u8 D_800591AD;
s32 D_800591A8;
s32 D_80059198;

struct Fixture {
    u32 sprite[64];
    u8 base[0x80];
    u8 state[0x20];
    u8 anim[0x40];
};
static struct Fixture fx, init, exp;

struct Call {
    u32 kind;
    u32 args[2];
    u8 mem[sizeof(struct Fixture)];
    u8 gate;
};
#define MAXCALL 6
static struct Call calls[MAXCALL], want[MAXCALL];
static unsigned nCalls;
static int wrapMode;

static void record(u32 kind, u32 a, u32 b)
{
    struct Call* p;

    if (nCalls >= MAXCALL) {
        fprintf(stderr, "T23538 FAIL call overflow\n");
        exit(2);
    }
    p = &calls[nCalls++];
    p->kind = kind;
    p->args[0] = a;
    p->args[1] = b;
    memcpy(p->mem, &fx, sizeof(fx));
    p->gate = D_800591AD;
}

void SpriteSetScale(void* s, short scale)
{
    record(1, (u32)(uintptr_t)s, (u32)(u16)scale);
    fx.base[0x10] = (u8)scale;
    fx.base[0x11] ^= 0x11;
    D_800591AD = (u8)((scale & 1) ? 0 : 2);
}
void SpriteComputeTransformMatrix(void* s)
{
    record(2, (u32)(uintptr_t)s, 0);
    fx.base[0x14] = D_800591AD;
    fx.base[0x15] ^= 0x5C;
}
void func_800234AC(void* s)
{
    record(3, (u32)(uintptr_t)s, 0);
    ((u8*)fx.sprite)[0x37] ^= 0x77;
}

static int bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    (void)u;
    if (t == 0x80022000u) {
        SpriteSetScale((void*)(uintptr_t)c->gpr[4], (short)c->gpr[5]);
    } else if (t == 0x80022090u) {
        SpriteComputeTransformMatrix((void*)(uintptr_t)c->gpr[4]);
    } else if (t == 0x800234ACu) {
        func_800234AC((void*)(uintptr_t)c->gpr[4]);
    } else {
        return 0;
    }
    return 1;
}

static const u16 kDenom[] = { 2, 3, 0x37, 0x80, 0x100, 0x101, 0x800, 0xFFF };
static const s16 k82[] = { 0, 1, -1, 0x1000, -0x1000, 0x2345, 0x7FFF, -0x5555 };
static const u16 kOff[] = { 0, 2, 4, 0x10, 0x3E, 0x7FFF, 0xFFFE, 0x1234 };

static void make_case(unsigned v)
{
    u32 s = v * 0x9E3779B1u + 77u;
    u32 w, flags;
    unsigned i, sv;

    v = (v * 2654435761u) ^ (v * 40503u << 11) ^ (v >> 3) ^ 0x5A5A5A5Au; /* spread selector bits */

    for (i = 0; i < sizeof(fx) / 4; ++i) {
        ((u32*)&fx)[i] = h_lcg(&s);
    }
    sv = (v >> 3) % 64u;                           /* signed 6-bit scale */
    flags = (u32)((v & 3) | (sv << 2) | (((v >> 9) & 1) << 11) | (((v >> 10) & 1) << 12) |
                  (((v >> 11) & 1) << 13) | ((h_lcg(&s) & 3u) << 14));
    *(u16*)fx.anim = (u16)flags;
    *(u16*)(fx.anim + 2) = kOff[(v >> 12) % 8u];
    *(u16*)(fx.anim + 4) = kOff[(v >> 7) % 8u];
    w = fx.sprite[0xAC / 4];
    w = (w & ~(0xFFFu << 7)) | ((u32)kDenom[(v >> 5) % 8u] << 7);
    if (v & 0x1000) { w |= 0xC; } else { w &= ~0xCu; }   /* flip / left-facing bits */
    fx.sprite[0xAC / 4] = w;
    *(s16*)((u8*)fx.sprite + 0x82) = k82[(v >> 15) % 8u];
    D_80059198 = (s32)((v >> 4) % 4u);
    D_800591A8 = (s32)(h_lcg(&s) & 0x7FFFu) - 100;
    D_800591AD = (v % 3 == 0) ? 0 : (u8)(1 + (v % 4));
    fx.sprite[0x20 / 4] = (v % 5 == 0) ? 0u : (u32)(uintptr_t)fx.base;
    fx.sprite[0x7C / 4] = (v % 7 == 0) ? 0u : (u32)(uintptr_t)fx.state;
    w = fx.sprite[0x3C / 4];
    w = (w & ~3u) | ((v % 4 == 1) ? 1u : (v % 3));
    if (v & 0x4000) { w |= 0x18; } else { w &= ~0x18u; }
    fx.sprite[0x3C / 4] = w;
    if (v & 0x2000) { fx.sprite[0x40 / 4] |= 0x100000u; } else { fx.sprite[0x40 / 4] &= ~0x100000u; }
    *(u32*)(fx.base + 0x34) = (v & 0x800) ? 0u : (u32)(0x1000 + v);
    if (v & 0x80000) { fx.sprite[0xA8 / 4] |= 1u; } else { fx.sprite[0xA8 / 4] &= ~1u; }
    if (!wrapMode) {
        /* Keep every port multiplication inside signed 32 bits (the port body
         * uses plain s32 arithmetic; retail wraps).  Halve the +0x82 term. */
        for (;;) {
            s32 work = D_80059198 + 1, t, sc = (s32)(sv & 0x20 ? (sv | ~0x3Fu) : sv) * 1024;
            s32 d = (s32)((fx.sprite[0xAC / 4] >> 7) & 0xFFF), f;
            int64_t prod;

            t = work * work * *(s16*)((u8*)fx.sprite + 0x82);
            if (t < 0) { t += 0xFFF; }
            f = ((0x10000 / d) * (0x10000 / d)) >> 8;
            prod = (int64_t)sc * (t >> 12) * f;
            if (prod >= -0x7FFFFFFF && prod <= 0x7FFFFFFF) { break; }
            *(s16*)((u8*)fx.sprite + 0x82) /= 2;
        }
    }
    if (wrapMode) {
        w = fx.sprite[0xAC / 4];
        w = (w & ~(0xFFFu << 7)) | ((u32)(1 + (v % 3)) << 7);
        fx.sprite[0xAC / 4] = w;
        *(s16*)((u8*)fx.sprite + 0x82) = (s16)(0x7000 + v);
        D_80059198 = (s32)(v % 100);
    }
}

int main(int argc, char** argv)
{
    unsigned n = 40000, v, fails = 0, ncomp = 0;
    char first[200] = "";
    unsigned firstv = 0;

    if (argc > 1) {
        n = (unsigned)atoi(argv[1]);
    }
    wrapMode = getenv("T23538_WRAP") != NULL;
    h_load_retail();
    h_region(&fx, sizeof(fx));
    h_map(0x800591ADu, &D_800591AD, 1);
    h_map(0x800591A8u, &D_800591A8, 4);
    h_map(0x80059198u, &D_80059198, 4);

    for (v = 0; v < n; ++v) {
        PcPortMipsBus bus;
        PcPortMipsCpu cpu;
        u8 wantGate;
        unsigned wantN;

        make_case(v);
        init = fx;
        memset(calls, 0, sizeof(calls));
        nCalls = 0;
        memset(&bus, 0, sizeof(bus));
        bus.read = h_rd;
        bus.write = h_wr;
        bus.bridge = bridge;
        PcPortMipsCpuInit(&cpu, &bus);
        cpu.gpr[4] = (u32)(uintptr_t)fx.sprite;
        cpu.gpr[5] = (u32)(uintptr_t)fx.anim;
        cpu.gpr[29] = 0x801FF000u;
        cpu.gpr[31] = 0xFFFFFFFCu;
        if (PcPortMipsRun(&cpu, 0x80023538u, 0xFFFFFFFCu, 20000) != PC_PORT_MIPS_HALTED) {
            fprintf(stderr, "T23538 FAIL oracle v=%u: %s\n", v, cpu.error);
            return 2;
        }
        exp = fx;
        memcpy(want, calls, sizeof(calls));
        wantGate = D_800591AD;
        wantN = nCalls;
        make_case(v);
        memset(calls, 0, sizeof(calls));
        nCalls = 0;
        func_80023538(fx.sprite, fx.anim);
        ++ncomp;
        if (getenv("T23538_DBG") && v < 12) { printf("v=%u +1C=%d +82=%d d=%u\n", v, *(s32*)((u8*)fx.sprite+0x1C), *(s16*)((u8*)fx.sprite+0x82), (fx.sprite[0xAC/4]>>7)&0xFFF); }
        if (nCalls != wantN || memcmp(&fx, &exp, sizeof(fx)) != 0 || memcmp(calls, want, sizeof(calls)) != 0 ||
            D_800591AD != wantGate) {
            if (fails == 0) {
                unsigned i;
                firstv = v;
                snprintf(first, sizeof(first), "calls native %u retail %u", nCalls, wantN);
                for (i = 0; i < sizeof(fx); ++i) {
                    if (((u8*)&fx)[i] != ((u8*)&exp)[i]) {
                        snprintf(first, sizeof(first), "final byte +0x%x (sprite<0x100, base<0x180, state<0x1A0) native %02x retail %02x; calls %u/%u", i,
                                 ((u8*)&fx)[i], ((u8*)&exp)[i], nCalls, wantN);
                        break;
                    }
                }
            }
            ++fails;
        }
    }
    if (fails) {
        printf("T23538 DIVERGES %u/%u cases; first v=%u: %s\n", fails, n, firstv, first);
        return 1;
    }
    printf("T23538 PASS %u retail-oracle comparisons (full fixture + call log%s)\n", ncomp, wrapMode ? ", wrap mode" : "");
    return 0;
}
