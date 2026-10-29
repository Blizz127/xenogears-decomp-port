/*
 * Retail-differential test for func_800248D4 (sprite animation-script tick,
 * 0x800248D4-0x80024F20) as shipped in src/slus_006.64/system/temp1.c.
 *
 * The retail function runs on the MIPS adapter; the native port body runs on a
 * byte-identical fixture.  The callees (func_8001D2B0, func_80022D44,
 * func_800245D8, func_800C11CC, func_8001FBE4 and the sprite +0x68 callback)
 * are controlled host stubs shared by both runs, so the comparison covers the
 * unit's own decoding/arithmetic/ordering, not the callee bodies.  The stack
 * helper leaves the retail E4/dedicated handlers reach run as retail code.
 *
 * Compared per case: the whole sprite fixture, the global timer and gate, the
 * ordered call log (kind + args + a fixture snapshot at each call).
 * Opcode sweep: all 256 opcodes x NVAR pseudo-random sprite states (wait
 * timer 0, speed 0..0xFFF, sub-index wrap, flip/left-facing bits in +0x3C /
 * +0xAC, frame pose, +0x68 callback present/absent, ...), multi-frame
 * progression through the 0x01 wait opcodes following the tested opcode.
 * Every opcode runs natively against retail; there is no skip list and no
 * known-divergence list.
 */
#include "sprite_retail_harness.h"

#include "common.h"

extern void func_800248D4(void* pSpriteData);

u8 D_800591AD;
s32 g_WorkListCurTimer;
u8 D_8004FC40[256];


#define SPR_OFF 0
struct Fixture {
    u32 sprite[128];
    u8 table[512];
    u32 pad[4];
};
static struct Fixture fx, init, exp;
static u8 script[0x20000];
#define PC0 (script + 0x10000)

struct Call {
    u32 kind;
    u32 args[3];
    u8 mem[sizeof(struct Fixture)];
    s32 timer;
    u8 gate;
};
#define MAXCALL 24
static struct Call calls[MAXCALL], wantCalls[MAXCALL];
static unsigned nCalls;

static void record(u32 kind, u32 a, u32 b, u32 c)
{
    struct Call* p;

    if (nCalls >= MAXCALL) {
        fprintf(stderr, "T248D4 FAIL call overflow\n");
        exit(2);
    }
    p = &calls[nCalls++];
    p->kind = kind;
    p->args[0] = a;
    p->args[1] = b;
    p->args[2] = c;
    memcpy(p->mem, &fx, sizeof(fx));
    p->timer = g_WorkListCurTimer;
    p->gate = D_800591AD;
}

#define SB(off) ((u8*)fx.sprite + (off))

void func_800C11CC(void* d)
{
    record(0, (u32)(uintptr_t)d, 0, 0);
    SB(0x31)[0] ^= 0x11;
}
void func_8001D2B0(void* d, s32 frame)
{
    record(1, (u32)(uintptr_t)d, (u32)(u16)frame, 0); /* callee param is s16 */
    *(u16*)(SB(0x30)) = (u16)frame;
    SB(0x32)[0] ^= 0x22;
}
void func_80022D44(void* d)
{
    record(2, (u32)(uintptr_t)d, 0, 0);
    SB(0x33)[0] ^= 0x33;
}
void func_800245D8(void* d, s32 anim)
{
    record(3, (u32)(uintptr_t)d, (u32)anim, 0);
    *(u32*)SB(0x10) ^= 0xDEADBEEFu;
    *(u32*)SB(0x64) = (u32)(uintptr_t)(script + 0x8000);
}
void func_8001FBE4(void* d, u32 op, void* operands)
{
    record(4, (u32)(uintptr_t)d, op, (u32)(uintptr_t)operands);
    SB(0x35)[0] = (u8)op;
}
static void cb68(void* d)
{
    record(5, (u32)(uintptr_t)d, 0, 0);
    *(u32*)SB(0x0C) += 0x101;
}

static int bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    (void)u;
    switch (t) {
    case 0x800C11CCu: func_800C11CC((void*)(uintptr_t)c->gpr[4]); return 1;
    case 0x8001D2B0u: func_8001D2B0((void*)(uintptr_t)c->gpr[4], (s32)c->gpr[5]); return 1;
    case 0x80022D44u: func_80022D44((void*)(uintptr_t)c->gpr[4]); return 1;
    case 0x800245D8u: func_800245D8((void*)(uintptr_t)c->gpr[4], (s32)c->gpr[5]); return 1;
    case 0x8001FBE4u: func_8001FBE4((void*)(uintptr_t)c->gpr[4], c->gpr[5], (void*)(uintptr_t)c->gpr[6]); return 1;
    default: break;
    }
    if (t == (u32)(uintptr_t)cb68) {
        cb68((void*)(uintptr_t)c->gpr[4]);
        return 1;
    }
    return 0;
}

static const u32 kStaleS3[] = { 0, 1, 5, 0x10, 0x7FF, 0xFFFFFFFFu, 0x80000000u, 0x01234567u, 0x100, 0xFFFFFF00u };
extern u32 g_SpriteTickEntryS3;

static const u8 kOperands[][2] = {
    { 3, 0 }, { 0xFD, 0xFF }, { 5, 0 }, { 0xFF, 0xFF }, { 1, 0 }, { 0x80, 0 },
    { 0x7F, 0 }, { 0, 4 }, { 0x12, 0x80 }, { 0x34, 0x0C }, { 0x05, 0x7B }, { 0, 1 },
    { 0xC3, 0x84 }, { 0x00, 0xA0 }, { 0x9F, 0x13 }, { 0x2A, 0xF8 },
};
static const u16 kSpeeds[] = { 0, 0x100, 0x37, 0xFFF, 0x800, 0x101, 0x1, 0x2C0 };

static void make_case(unsigned op, unsigned v, unsigned* gate, unsigned* nopr)
{
    u32 s = op * 0x9E3779B1u + v * 0x85EBCA6Bu + 1u;
    u32 w;
    unsigned i, pair, spd, sub;

    for (i = 0; i < 128; ++i) {
        fx.sprite[i] = h_lcg(&s);
    }
    for (i = 0; i < sizeof(fx.table); ++i) {
        fx.table[i] = (u8)h_lcg(&s);
    }
    fx.pad[0] = fx.pad[1] = fx.pad[2] = fx.pad[3] = 0x5A5A5A5Au;
    pair = (v + (v >> 4)) % (sizeof(kOperands) / 2);
    spd = (v * 7 + (v >> 3)) % (sizeof(kSpeeds) / 2);
    sub = (v % 11 == 0) ? 0x3Fu : (v % 5 == 0) ? 0u : (v % 9 == 0) ? 0x3Eu : (h_lcg(&s) & 0x3Fu);

    *(u16*)SB(0x9E) = 0;
    w = fx.sprite[0xAC / 4];
    w = (w & ~(0xFFFu << 7)) | ((u32)kSpeeds[spd] << 7);
    /* AC bit 2 / bit 3 = flip, bit 6 = double tick: keep the random bits. */
    fx.sprite[0xAC / 4] = w;
    w = fx.sprite[0xA8 / 4];
    w = (w & ~(0x3Fu << 22)) | (sub << 22);
    if (v & 0x10) { w |= 0x30000000u; } else { w &= ~0x30000000u; }
    fx.sprite[0xA8 / 4] = w;
    w = fx.sprite[0x3C / 4];
    w = (w & ~3u) | (v % 3);
    fx.sprite[0x3C / 4] = w;
    *(u32*)SB(0x60) = (u32)(uintptr_t)fx.table;
    *(u32*)SB(0x68) = (v & 1) ? (u32)(uintptr_t)cb68 : 0u;
    *(s8*)SB(0xAF) = (v & 2) ? 0x3F : (s8)((h_lcg(&s) & 0x1F) - 4);
    *(s8*)SB(0xB0) = (v & 4) ? (s8)-3 : (s8)(h_lcg(&s) & 0x1F);
    *(s32*)SB(0x10) = (v & 8) ? -5 : (s32)(h_lcg(&s) & 0xFFFF);
    *(s16*)SB(0x6) = (s16)(h_lcg(&s) & 0x3F);
    *(s16*)SB(0x84) = (v & 0x20) ? *(s16*)SB(0x6) : (s16)((v & 0x40) ? *(s16*)SB(0x6) + 1 : *(s16*)SB(0x6) - 1);
    *(s8*)SB(0x8C) = (s8)(((v >> 3) % 9) - 4);
    {
        s8 si = *(s8*)SB(0x8C);
        SB(0x8E)[si] = (v & 0x80) ? 0 : (u8)(1 + (v % 3));
    }
    memset(script, 0x01, sizeof(script));
    PC0[0] = (u8)op;
    PC0[1] = kOperands[pair][0];
    PC0[2] = kOperands[pair][1];
    *(u32*)SB(0x64) = (u32)(uintptr_t)PC0;
    if ((op == 0xE1 || op == 0xE4 || op == 0xD4 || op == 0xE2) && PC0[1] == 0 && PC0[2] == 0) {
        PC0[2] = 1;
    }
    /* 0xFA: operand 0 = variable index, jump offset = PC0[2] | PC0[3]<<8. */
    if (op == 0xFA) {
        PC0[3] = (u8)(v % 4 == 0 ? 0xFF : v % 4 == 1 ? 0 : 1);
        if (PC0[2] == 0 && PC0[3] == 0) {
            PC0[3] = 1;
        }
    }
    /* variable slots with bit7 set live at the +0x88 base. */
    *(u32*)SB(0x88) = (u32)(uintptr_t)fx.table + (h_lcg(&s) & 0x3Fu);
    if (op == 0x98) {
        /* linked sprite (+0x70): none / animation mismatch / match with
         * +0xA8 bits 28-29 == 2 or not. */
        u8* link = fx.table + 0x10;

        *(u32*)SB(0x70) = (v % 4 == 0) ? 0u : (u32)(uintptr_t)link;
        link[0xAF] = (v % 4 == 1) ? (u8)(*(u8*)SB(0x8D) + 1) : *(u8*)SB(0x8D);
        link[0xA8 + 3] = (u8)((link[0xA8 + 3] & 0xCF) | ((v & 8) ? 0x20 : 0x10));
    }
    if (op == 0x85) {
        /* pc is rebuilt from the popped 24 bits: keep them a valid script
         * address and out of the window that aliases the index byte. */
        s8 si = (s8)(v % 7);
        u32 target = (u32)(uintptr_t)(PC0 + 0x40 + (v & 0xF));

        if ((uintptr_t)PC0 >= 0x1000000u) {
            fprintf(stderr, "T248D4 FAIL script not below 16MiB\n");
            exit(2);
        }
        *(s8*)SB(0x8C) = si;
        SB(0x8E)[si] = (u8)target;
        SB(0x8F)[si] = (u8)(target >> 8);
        SB(0x90)[si] = (u8)(target >> 16);
    }
    *gate = (v % 13 == 0);
    D_800591AD = (u8)*gate;
    g_WorkListCurTimer = (s32)(v & 0x7);
    *nopr = 0;
}

static unsigned ncomp, nfail_total;

static const char* diff_desc(char* buf, size_t n)
{
    unsigned i;
    if (nCalls != 0 && memcmp(calls, wantCalls, sizeof(calls)) != 0) {
        for (i = 0; i < MAXCALL; ++i) {
            if (memcmp(&calls[i], &wantCalls[i], sizeof(struct Call)) != 0) {
                unsigned j;
                if (calls[i].kind != wantCalls[i].kind || memcmp(calls[i].args, wantCalls[i].args, 12) != 0) {
                    snprintf(buf, n, "call[%u] kind %u/%u args %08x,%08x,%08x vs retail %08x,%08x,%08x", i,
                             calls[i].kind, wantCalls[i].kind, calls[i].args[0], calls[i].args[1], calls[i].args[2],
                             wantCalls[i].args[0], wantCalls[i].args[1], wantCalls[i].args[2]);
                    return buf;
                }
                for (j = 0; j < sizeof(fx); ++j) {
                    if (calls[i].mem[j] != wantCalls[i].mem[j]) {
                        snprintf(buf, n, "call[%u] snapshot byte +0x%x native %02x retail %02x", i, j,
                                 calls[i].mem[j], wantCalls[i].mem[j]);
                        return buf;
                    }
                }
                snprintf(buf, n, "call[%u] timer/gate", i);
                return buf;
            }
        }
    }
    if (nCalls != 0 || 1) {
        for (i = 0; i < sizeof(fx); ++i) {
            if (((u8*)&fx)[i] != ((u8*)&exp)[i]) {
                snprintf(buf, n, "final byte +0x%x native %02x retail %02x", i, ((u8*)&fx)[i], ((u8*)&exp)[i]);
                return buf;
            }
        }
    }
    snprintf(buf, n, "global timer/gate or call count");
    return buf;
}

int main(int argc, char** argv)
{
    unsigned nvar = 300, op, v;
    unsigned bad_ops = 0;
    s32 want_timer;
    u8 want_gate;

    if (argc > 1) {
        nvar = (unsigned)atoi(argv[1]);
    }
    h_load_retail();
    memcpy(D_8004FC40, h_ram + (0x8004FC40u & 0x1FFFFFu), 256);
    h_region(&fx, sizeof(fx));
    h_region(script, sizeof(script));
    h_map(0x800591ADu, &D_800591AD, 1);
    h_map(0x80059428u, &g_WorkListCurTimer, 4);

    for (op = 0; op < 256; ++op) {
        unsigned fails = 0, first_v = 0, nopr;
        char firstmsg[256] = "";
        unsigned gate;

        for (v = 0; v < nvar; ++v) {
            PcPortMipsBus bus;
            PcPortMipsCpu cpu;
            char buf[200];

            make_case(op, v, &gate, &nopr);
            init = fx;
            memset(calls, 0, sizeof(calls));
            nCalls = 0;
            bus.opaque = NULL;
            bus.read = h_rd;
            bus.write = h_wr;
            bus.bridge = bridge;
            bus.cop2_read = NULL;
            bus.cop2_write = NULL;
            bus.cop2_command = NULL;
            PcPortMipsCpuInit(&cpu, &bus);
            /* $s3 at entry is caller state in retail (read by 0x40-0x7F). */
            cpu.gpr[19] = g_SpriteTickEntryS3 = kStaleS3[v % (sizeof(kStaleS3) / 4)];
            cpu.gpr[4] = (u32)(uintptr_t)fx.sprite;
            cpu.gpr[29] = 0x801FF000u;
            cpu.gpr[31] = 0xFFFFFFFCu;
            if (PcPortMipsRun(&cpu, 0x800248D4u, 0xFFFFFFFCu, 20000) != PC_PORT_MIPS_HALTED) {
                fprintf(stderr, "T248D4 FAIL oracle op=0x%02X v=%u: %s\n", op, v, cpu.error);
                return 2;
            }
            exp = fx;
            memcpy(wantCalls, calls, sizeof(calls));
            want_timer = g_WorkListCurTimer;
            want_gate = D_800591AD;
            {
                unsigned wantN = nCalls;

                fx = init;
                make_case(op, v, &gate, &nopr); /* restores globals + script */
                fx = init;
                memset(calls, 0, sizeof(calls));
                nCalls = 0;
                func_800248D4(fx.sprite);
                ++ncomp;
                if (nCalls != wantN || memcmp(&fx, &exp, sizeof(fx)) != 0 ||
                    memcmp(calls, wantCalls, sizeof(calls)) != 0 ||
                    g_WorkListCurTimer != want_timer || D_800591AD != want_gate) {
                    if (fails == 0) {
                        first_v = v;
                        snprintf(firstmsg, sizeof(firstmsg), "%s (calls native %u retail %u, timer %d/%d)",
                                 diff_desc(buf, sizeof(buf)), nCalls, wantN, g_WorkListCurTimer, want_timer);
                    }
                    ++fails;
                }
            }
        }
        if (fails != 0) {
            ++bad_ops;
            printf("T248D4 DIVERGES");
            nfail_total += fails;
            printf(" op=0x%02X %u/%u cases; first v=%u: %s\n", op, fails, nvar, first_v, firstmsg);
        }
    }
    printf("T248D4 %s: %u comparisons, %u unexpected-diverging opcodes, %u divergent cases, all 256 opcodes executed natively\n",
           bad_ops ? "FAIL" : "PASS", ncomp, bad_ops, nfail_total);
    return bad_ops ? 1 : 0;
}
