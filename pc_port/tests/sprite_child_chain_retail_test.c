/*
 * Retail-differential tests for the LINKED child-sprite spawn chain in
 * pc_port/src/game_overrides.c (strong port-owned overrides listed in
 * pc_port/port_owned_overrides.txt):
 *   SUITE_23A48  func_80023A48 child-sprite allocation (0x80023A48-0x80023B84)
 *   SUITE_23B84  func_80023B84 opcode-0xE0 child spawn  (0x80023B84-0x80023FD8)
 * Retail runs on the MIPS adapter; the callees it reaches are controlled host
 * stubs shared by the oracle and the native body.  Compared per case: return
 * value, the full written fixture (parent sprite, wrapper/child sprite, parent
 * and child transform bases, pools), D_800591AC, and the ordered call log with
 * all arguments and a fixture snapshot at every call.
 *
 * Host pointers vs retail: D_8006BE10 / D_8005A474 are host arrays in the port
 * but fixed retail addresses; the harness maps the retail addresses onto the
 * host arrays and normalises the host addresses back to the retail ones before
 * comparing (pointer-width is the only declared representation difference).
 */
#include "sprite_retail_harness.h"

#include "common.h"

u8 D_800591AC;
u8 D_8006BE10[0x40];
u8 D_8005A474[0x40];

static u32 norm(u32 p)
{
    if (p == (u32)(uintptr_t)D_8006BE10) { return 0x8006BE10u; }
    if (p == (u32)(uintptr_t)D_8005A474) { return 0x8005A474u; }
    return p;
}

struct Fixture {
    u8 parent[0x100];
    u8 parentBase[0x40];
    u8 wrapper[0x200];     /* AnimTask: +0x38 sprite, +0xB4.. transform base */
    u8 pkg[0x40];
    u8 entry[0x10];
};
static struct Fixture fx, init, exp;
#define W(off) (fx.wrapper + (off))

struct Call {
    u32 kind;
    u32 args[5];
    u8 mem[sizeof(struct Fixture)];
    u8 flag;
};
#define MAXCALL 8
static struct Call calls[MAXCALL], want[MAXCALL];
static unsigned nCalls;
static s32 gType, gMode, gFrames;

static void snap(struct Call* p)
{
    unsigned i;

    memcpy(p->mem, &fx, sizeof(fx));
    for (i = 0; i + 4 <= sizeof(fx); i += 4) {
        u32 w;
        memcpy(&w, p->mem + i, 4);
        w = norm(w);
        memcpy(p->mem + i, &w, 4);
    }
    p->flag = D_800591AC;
}

static void record(u32 kind, u32 a, u32 b, u32 c, u32 d, u32 e)
{
    struct Call* p;

    if (nCalls >= MAXCALL) {
        fprintf(stderr, "CHAIN FAIL call overflow\n");
        exit(2);
    }
    p = &calls[nCalls++];
    p->kind = kind;
    p->args[0] = norm(a); p->args[1] = norm(b); p->args[2] = norm(c);
    p->args[3] = norm(d); p->args[4] = norm(e);
    snap(p);
}

/* ---- controlled callees ------------------------------------------------- */
void* func_800233A4(void* pOwner, int size)
{
    record(10, (u32)(uintptr_t)pOwner, (u32)size, 0, 0, 0);
    W(0x38)[0x31] ^= 0x31;
    return fx.wrapper;
}
void func_80023950(void* sp)
{
    record(11, (u32)(uintptr_t)sp, 0, 0, 0, 0);
    *(u32*)((u8*)sp + 0x20) = (u32)(uintptr_t)(W(0xB4));
    *(u32*)(W(0xB4) + 0x30) = 0x77u;
}
void func_80023958(void* sp)
{
    record(12, (u32)(uintptr_t)sp, 0, 0, 0, 0);
    *(u32*)((u8*)sp + 0x20) = (u32)(uintptr_t)(W(0xB4));
    *(u32*)(W(0xB4) + 0x34) = 0x58u;
}
void func_800239F4(void* sp)
{
    record(13, (u32)(uintptr_t)sp, 0, 0, 0, 0);
    *(u32*)((u8*)sp + 0x20) = (u32)(uintptr_t)(W(0xB4));
    *(u32*)(W(0xB4) + 0x38) = 0x9Fu;
}
s32 func_8001EE74(void* pkg0)
{
    record(14, (u32)(uintptr_t)pkg0, 0, 0, 0, 0);
    return gFrames + (s32)(((u32)(uintptr_t)pkg0 >> 4) & 3);
}
s32 func_80023440(void* pScript)
{
    record(15, (u32)(uintptr_t)pScript, 0, 0, 0, 0);
    return gType;
}
/* the linked override takes only (type); retail also passes a1 (script) which
 * its callee never reads -- record the type only. */
s32 func_80023468(s32 type)
{
    record(16, (u32)type, 0, 0, 0, 0);
    return gMode;
}
void func_80023538(void* pSprite, void* pAnim)
{
    record(17, (u32)(uintptr_t)pSprite, (u32)(uintptr_t)pAnim, 0, 0, 0);
    ((u8*)pSprite)[0x33] ^= 0x55;
}
void func_80024730(void* pWrapper)
{
    record(18, (u32)(uintptr_t)pWrapper, 0, 0, 0, 0);
    ((u8*)pWrapper)[0x37] ^= 0xAA;
}

#ifdef SUITE_23B84
void* func_80023A48(s32 type, s32 mode, void* pkg, s32 size, void* owner)
{
    record(19, (u32)type, (u32)mode, (u32)(uintptr_t)pkg, (u32)size, (u32)(uintptr_t)owner);
    *(u32*)(W(0x38) + 0x24) = (u32)(uintptr_t)(fx.pkg);
    *(u32*)(W(0x38) + 0x20) = (mode == 0) ? 0u : (u32)(uintptr_t)(W(0xB4));
    W(0x38)[0x2F] ^= 0x13;
    return fx.wrapper;
}
#endif

static int bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    (void)u;
    switch (t) {
    case 0x800233A4u: c->gpr[2] = (u32)(uintptr_t)func_800233A4((void*)(uintptr_t)c->gpr[4], (int)c->gpr[5]); return 1;
    case 0x80023950u: func_80023950((void*)(uintptr_t)c->gpr[4]); return 1;
    case 0x80023958u: func_80023958((void*)(uintptr_t)c->gpr[4]); return 1;
    case 0x800239F4u: func_800239F4((void*)(uintptr_t)c->gpr[4]); return 1;
    case 0x8001EE74u: c->gpr[2] = (u32)func_8001EE74((void*)(uintptr_t)c->gpr[4]); return 1;
#ifdef SUITE_23B84
    case 0x80023440u: c->gpr[2] = (u32)func_80023440((void*)(uintptr_t)c->gpr[4]); return 1;
    case 0x80023468u: c->gpr[2] = (u32)func_80023468((s32)c->gpr[4]); return 1;
    case 0x80023538u: func_80023538((void*)(uintptr_t)c->gpr[4], (void*)(uintptr_t)c->gpr[5]); return 1;
    case 0x80024730u: func_80024730((void*)(uintptr_t)c->gpr[4]); return 1;
    case 0x80023A48u: {
        u32 owner;
        if (h_rd(NULL, c->gpr[29] + 16, 4, &owner) != 0) { return -1; }
        c->gpr[2] = (u32)(uintptr_t)func_80023A48((s32)c->gpr[4], (s32)c->gpr[5], (void*)(uintptr_t)c->gpr[6],
                                                  (s32)c->gpr[7], (void*)(uintptr_t)owner);
        return 1;
    }
#endif
    default: return 0;
    }
}

static void fill(u32* s)
{
    unsigned i;
    u32* p = (u32*)&fx;

    for (i = 0; i < sizeof(fx) / 4; ++i) {
        p[i] = h_lcg(s);
    }
    for (i = 0; i < sizeof(D_8006BE10) / 4; ++i) {
        ((u32*)D_8006BE10)[i] = h_lcg(s);
        ((u32*)D_8005A474)[i] = h_lcg(s);
    }
}

__attribute__((unused)) static void normfx(void* m)
{
    unsigned j;

    for (j = 0; j + 4 <= sizeof(fx); j += 4) {
        u32 w;
        memcpy(&w, (u8*)m + j, 4);
        w = norm(w);
        memcpy((u8*)m + j, &w, 4);
    }
}

static const char* firstdiff(char* buf, size_t n, unsigned wantN, u32 got, u32 ret)
{
    unsigned i, j;

    snprintf(buf, n, "ret native %08x retail %08x; calls %u/%u", got, ret, nCalls, wantN);
    for (i = 0; i < MAXCALL; ++i) {
        if (memcmp(&calls[i], &want[i], sizeof(calls[i])) != 0) {
            if (calls[i].kind != want[i].kind || memcmp(calls[i].args, want[i].args, 20) != 0) {
                snprintf(buf, n, "call[%u] kind %u/%u args n=%08x,%08x,%08x,%08x,%08x r=%08x,%08x,%08x,%08x,%08x", i,
                         calls[i].kind, want[i].kind, calls[i].args[0], calls[i].args[1], calls[i].args[2],
                         calls[i].args[3], calls[i].args[4], want[i].args[0], want[i].args[1], want[i].args[2],
                         want[i].args[3], want[i].args[4]);
                return buf;
            }
            for (j = 0; j < sizeof(fx); ++j) {
                if (calls[i].mem[j] != want[i].mem[j]) {
                    snprintf(buf, n, "call[%u] snapshot byte +0x%x (parent<0x100,pbase<0x140,wrapper<0x340) native %02x retail %02x", i, j, calls[i].mem[j], want[i].mem[j]);
                    return buf;
                }
            }
            snprintf(buf, n, "call[%u] D_800591AC native %u retail %u", i, calls[i].flag, want[i].flag);
            return buf;
        }
    }
    for (j = 0; j < sizeof(fx); ++j) {
        u8 a = ((u8*)&fx)[j], b = ((u8*)&exp)[j];
        if (a != b) {
            snprintf(buf, n, "final byte +0x%x (parent<0x100,pbase<0x140,wrapper<0x340) native %02x retail %02x", j, a, b);
            return buf;
        }
    }
    return buf;
}

#ifdef SUITE_23A48
extern void* func_80023A48(s32 type, s32 mode, void* pkg, s32 size, void* owner);

static int run(unsigned v)
{
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;
    u32 s = v * 0x9E3779B1u + 3u, got, ret;
    s32 type = (s32)(v % 16u), mode, size;
    u32 owner = (v & 1) ? 0u : (u32)(uintptr_t)fx.parent;
    u32 pkg;
    unsigned wantN;
    char buf[300];

    mode = (s32)((v / 16u) % 3u);
    size = (s32)((v / 48u) % 7u) * 12 - 20;
    gFrames = (s32)((v / 337u) % 5u);
    fill(&s);
    pkg = (u32)(uintptr_t)fx.pkg;
    if (mode == 1 && (type == 5 || type == 6) && (v & 2)) {
        pkg = 0u;                       /* caller package ignored for type 5/6 */
    }
    init = fx;
    memset(calls, 0, sizeof(calls));
    nCalls = 0;
    memset(&bus, 0, sizeof(bus));
    bus.read = h_rd; bus.write = h_wr; bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (u32)type; cpu.gpr[5] = (u32)mode; cpu.gpr[6] = pkg; cpu.gpr[7] = (u32)size;
    cpu.gpr[29] = 0x801FF000u; cpu.gpr[31] = 0xFFFFFFFCu;
    h_wr(NULL, 0x801FF000u + 16, 4, owner);
    if (PcPortMipsRun(&cpu, 0x80023A48u, 0xFFFFFFFCu, 4000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "CHAIN 23A48 FAIL oracle v=%u: %s\n", v, cpu.error);
        exit(2);
    }
    ret = cpu.gpr[2];
    exp = fx;
    normfx(&exp);
    memcpy(want, calls, sizeof(calls));
    wantN = nCalls;

    fx = init;
    memset(calls, 0, sizeof(calls));
    nCalls = 0;
    got = (u32)(uintptr_t)func_80023A48(type, mode, (void*)(uintptr_t)pkg, size, (void*)(uintptr_t)owner);
    normfx(&fx);
    if (got != ret || nCalls != wantN || memcmp(&fx, &exp, sizeof(fx)) != 0 ||
        memcmp(calls, want, sizeof(calls)) != 0) {
        static unsigned shown;
        if (shown++ < 3) {
            printf("CHAIN 23A48 DIVERGES v=%u type=%d mode=%d size=%d owner=%08x pkg=%08x: %s\n", v, (int)type, (int)mode,
                   (int)size, owner, pkg, firstdiff(buf, sizeof(buf), wantN, got, ret));
        }
        return 1;
    }
    return 0;
}
#define SUITE_NAME "23A48"
#endif

#ifdef SUITE_23B84
extern void* func_80023B84(void* pSprite, void* pScript, void* pPkg);

static int run(unsigned v)
{
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;
    u32 s = v * 0x9E3779B1u + 5u, got, ret, w;
    unsigned wantN;
    u8 wantFlag, flag0;
    char buf[300];
    u8* P = fx.parent;

    gType = (s32)(v % 16u);
    gMode = (s32)((v / 16u) % 3u);
    fill(&s);
    D_800591AC = (u8)(1 + (v & 3));
    w = *(u32*)(P + 0xB0);
    if (v & 4) { w |= 0x100u; } else { w &= ~0x100u; }
    if (v & 8) { w |= 0x200u; } else { w &= ~0x200u; }
    *(u32*)(P + 0xB0) = w;
    w = *(u32*)(P + 0xA8);
    if (v & 0x10) { w |= 1u; } else { w &= ~1u; }
    *(u32*)(P + 0xA8) = w;
    w = *(u32*)(P + 0xAC);
    if (v & 0x20) { w |= 0xCu; } else if (v & 0x40) { w = (w & ~0xCu) | 4u; } else { w &= ~0xCu; }
    *(u32*)(P + 0xAC) = w;
    w = *(u32*)(P + 0x3C);
    if (v & 0x80) { w |= 0x18u; } else { w &= ~0x18u; }
    *(u32*)(P + 0x3C) = w;
    *(u32*)(P + 0x20) = (u32)(uintptr_t)fx.parentBase;
    *(u32*)(P + 0x6C) = (v & 0x100) ? 0u : (u32)(uintptr_t)fx.entry;
    memset(calls, 0, sizeof(calls));
    nCalls = 0;
    init = fx;
    flag0 = D_800591AC;
    memset(&bus, 0, sizeof(bus));
    bus.read = h_rd; bus.write = h_wr; bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (u32)(uintptr_t)P; cpu.gpr[5] = (u32)(uintptr_t)fx.entry; cpu.gpr[6] = (u32)(uintptr_t)fx.pkg;
    cpu.gpr[29] = 0x801FF000u; cpu.gpr[31] = 0xFFFFFFFCu;
    if (PcPortMipsRun(&cpu, 0x80023B84u, 0xFFFFFFFCu, 20000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "CHAIN 23B84 FAIL oracle v=%u: %s\n", v, cpu.error);
        exit(2);
    }
    ret = cpu.gpr[2];
    exp = fx;
    memcpy(want, calls, sizeof(calls));
    wantN = nCalls;
    wantFlag = D_800591AC;
    fx = init;
    D_800591AC = flag0;
    memset(calls, 0, sizeof(calls));
    nCalls = 0;
    got = (u32)(uintptr_t)func_80023B84(P, fx.entry, fx.pkg);
    if (got != ret || nCalls != wantN || memcmp(&fx, &exp, sizeof(fx)) != 0 || D_800591AC != wantFlag ||
        memcmp(calls, want, sizeof(calls)) != 0) {
        static unsigned shown;
        if (shown++ < 3) {
            printf("CHAIN 23B84 DIVERGES v=%u type=%d mode=%d: %s\n", v, (int)gType, (int)gMode,
                   firstdiff(buf, sizeof(buf), wantN, got, ret));
        }
        return 1;
    }
    return 0;
}
#define SUITE_NAME "23B84"
#endif

int main(int argc, char** argv)
{
    unsigned n = 20000, v, fails = 0;

    if (argc > 1) {
        n = (unsigned)atoi(argv[1]);
    }
    h_load_retail();
    h_region(&fx, sizeof(fx));
    h_map(0x800591ACu, &D_800591AC, 1);
    h_map(0x8006BE10u, D_8006BE10, sizeof(D_8006BE10));
    h_map(0x8005A474u, D_8005A474, sizeof(D_8005A474));
    for (v = 0; v < n; ++v) {
        fails += (unsigned)run(v);
    }
    if (fails) {
        printf("CHAIN " SUITE_NAME " DIVERGES %u/%u cases\n", fails, n);
        return 1;
    }
    printf("CHAIN " SUITE_NAME " PASS %u retail-oracle comparisons (return/full fixture/globals/call log)\n", n);
    return 0;
}
