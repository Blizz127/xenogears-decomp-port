/*
 * Retail-differential scenario test: the field save-point bob loop.
 *
 * Map 15 (Mountain Path) actor 60 binds sprite package 5. Its animation 0
 * spawns a type-2 child (opcode 0xE0) whose script, after the one-time model
 * load, loops forever:
 *
 *   A3 00            gravity 0
 *   BC 96            snap position to the parent (sub-command 0x16)
 *   B4 06 A6 FF ...  push a loop count, then a -1 Y-velocity impulse
 *   B4 05 CE .. 30   push a count, spin (CE) and wait one frame, E4 loops
 *   ...              the mirrored +1 impulses, a 2-frame pause, E1 back
 *
 * Each frame the owning AnimTask (func_80022DF4) runs AnimScriptTick and
 * then func_80022CDC, whose fly path (+0x3C bit 26) integrates Y += vel.
 *
 * Both sides run that pair of calls for many frames from one byte-identical
 * fixture: retail AnimScriptTick (0x80023210) / func_80022CDC (0x80022CDC)
 * from disc/SLUS_006.64 on the port MIPS adapter (func_800248D4,
 * func_8001FBE4, the +0x8C stack helpers, func_80022B2C and func_80022CAC
 * all execute as retail code), and the native port bodies from
 * src/slus_006.64/system/temp1.c and animation_scripts.c. After every frame
 * the whole fixture (sprite, parent, transform block, aux block, script) and
 * the shared globals must be identical.
 *
 * Scenarios:
 *   disc  - the bytes of the real loop, extracted at test time from
 *           disc/disc1.bin by the runner script (never stored in source);
 *   synth - an authored loop of the same shape with other counts/operands.
 *
 * A trajectory summary (sprite Y halfword and Y velocity ranges over the
 * first and last 400 frames) is printed, so a drift is visible as numbers,
 * not only as a memcmp failure. --native-only (used with SAVEPT_TEMP1 by
 * the runner) traces an arbitrary body without the oracle. A body that
 * corrupts the sprite can divide by a zeroed +0xAC speed field natively;
 * that ends the native-only trace (SIGFPE), never the differential.
 */
#include "sprite_retail_harness.h"

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

extern void AnimScriptTick(void* pSpriteData);
extern void func_80022CDC(u8* pSprite);

/* Native globals the units read; the oracle sees the same objects. */
s32 D_80059198;
u8 D_800591AD;
s32 g_WorkListCurTimer;
extern const u8 D_8004FC40[256]; /* animation_scripts.c port copy */

#define GUARD(name) \
    void name(void) { fputs("SAVEPT FAIL unexpected native callee " #name "\n", stderr); abort(); }

struct Fixture {
    u8 sprite[0x100];
    u8 parent[0x100];
    u8 base[0x40];
    u8 aux[0x20];
    u8 script[0x100];
} __attribute__((aligned(16)));
static struct Fixture fx, before, retail;
static s32 retailTimer;

/* Frame-select callee (opcodes 0x00-0x2F). The retail loop never reaches it;
 * a body that mis-decodes the script can (r6.7.2's 0xB4 lands on a 0x00
 * operand byte). Counted, no effect, so --native-only can still trace. */
static unsigned nFrameSelect;
static unsigned traceFrames;
void func_8001D2B0(void* p, s32 frame) { (void)p; (void)frame; ++nFrameSelect; }

static void put16(u8* p, u16 v) { memcpy(p, &v, 2); }
static void put32(u8* p, u32 v) { memcpy(p, &v, 4); }
static s16 get16s(const u8* p) { s16 v; memcpy(&v, p, 2); return v; }
static s32 get32s(const u8* p) { s32 v; memcpy(&v, p, 4); return v; }
static u32 get32(const u8* p) { u32 v; memcpy(&v, p, 4); return v; }

static int bridge(void* u, PcPortMipsCpu* c, u32 t)
{
    (void)u;
    (void)c;
    /* Everything this loop reaches is main-executable code. An overlay
     * target (func_800BA8F4 floor query, func_800C11CC) is a test failure. */
    if (t >= 0x80080000u && t < 0x80200000u) {
        return -1;
    }
    return 0;
}

static void retail_call(u32 entry, void* arg)
{
    PcPortMipsBus bus;
    PcPortMipsCpu cpu;
    int rc;

    memset(&bus, 0, sizeof(bus));
    bus.read = h_rd;
    bus.write = h_wr;
    bus.bridge = bridge;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = (u32)(uintptr_t)arg;
    cpu.gpr[29] = 0x801FF000u;
    cpu.gpr[31] = 0xFFFFFFFCu;
    rc = PcPortMipsRun(&cpu, entry, 0xFFFFFFFCu, 200000);
    if (rc != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SAVEPT FAIL retail 0x%08X did not return: %s\n", entry, cpu.error);
        exit(2);
    }
}

static void setup(const u8* code, unsigned len, unsigned start)
{
    u8* s = fx.sprite;
    u8* par = fx.parent;

    memset(&fx, 0, sizeof(fx));
    if (len > sizeof(fx.script)) {
        fprintf(stderr, "SAVEPT FAIL script too long\n");
        exit(2);
    }
    memcpy(fx.script, code, len);

    /* Parent = actor 60's sprite at its field position (732,-8,-840). */
    put32(par + 0x0, (u32)732 << 16);
    put32(par + 0x4, (u32)(s32)-8 << 16);
    put32(par + 0x8, (u32)(s32)-840 << 16);

    /* Child: type-2 model sprite, fly bit (+0x3C bit 26), speed 0x100,
     * scale 0x1000, no +0x3A factor, stack index 0x10, wait 1 so the first
     * AnimScriptTick runs the script. Position starts off-parent so the
     * BC 96 snap is observable. */
    put32(s + 0x0, (u32)700 << 16);
    put32(s + 0x4, (u32)(s32)-40 << 16);
    put32(s + 0x8, (u32)(s32)-800 << 16);
    put32(s + 0x20, (u32)(uintptr_t)fx.base);
    put32(s + 0x3C, (1u << 26) | 2u);
    put32(s + 0x64, (u32)(uintptr_t)(fx.script + start));
    put32(s + 0x70, (u32)(uintptr_t)fx.parent);
    put32(s + 0x7C, (u32)(uintptr_t)fx.aux);
    put16(s + 0x82, 0x1000);
    s[0x8C] = 0x10;
    put16(s + 0x9E, 1);
    put32(s + 0xA8, 0);
    put32(s + 0xAC, 0x100u << 7);
    D_80059198 = 0;
    D_800591AD = 0;
    g_WorkListCurTimer = 0;
}

struct Window { s16 minY, maxY; s32 minVel, maxVel; };

static void window_add(struct Window* w, int first)
{
    s16 y = get16s(fx.sprite + 0x6);
    s32 v = get32s(fx.sprite + 0x10);

    if (first || y < w->minY) w->minY = y;
    if (first || y > w->maxY) w->maxY = y;
    if (first || v < w->minVel) w->minVel = v;
    if (first || v > w->maxVel) w->maxVel = v;
}

/* Returns the number of diverging frames (0 = identical every frame). */
static unsigned run_scenario(const char* name, const u8* code, unsigned len,
                             unsigned start, unsigned loop, unsigned frames,
                             int native_only)
{
    struct Window head = {0, 0, 0, 0}, tail = {0, 0, 0, 0};
    unsigned f;
    unsigned firstBad = 0;
    unsigned nbad = 0;

    setup(code, len, start);
    nFrameSelect = 0;
    (void)loop;

    for (f = 0; f < frames; ++f) {
        before = fx;
        {
            s32 t0 = g_WorkListCurTimer;

            if (!native_only) {
                retail_call(0x80023210u, fx.sprite); /* AnimScriptTick */
                retail_call(0x80022CDCu, fx.sprite); /* func_80022CDC */
                retail = fx;
                retailTimer = g_WorkListCurTimer;
                fx = before;
                g_WorkListCurTimer = t0;
            }
        }
        AnimScriptTick(fx.sprite);
        func_80022CDC(fx.sprite);
        if (!native_only &&
            (memcmp(&fx, &retail, sizeof(fx)) != 0 || g_WorkListCurTimer != retailTimer)) {
            if (nbad == 0) {
                unsigned i;

                firstBad = f;
                for (i = 0; i < sizeof(fx); ++i) {
                    if (((u8*)&fx)[i] != ((u8*)&retail)[i]) {
                        break;
                    }
                }
                printf("SAVEPT DIVERGE scenario=%s frame=%u first byte +0x%X "
                       "native=%02X retail=%02X | native Y=%d vel=%d pc=+0x%X | "
                       "retail Y=%d vel=%d pc=+0x%X\n",
                       name, f, i, ((u8*)&fx)[i], ((u8*)&retail)[i],
                       get16s(fx.sprite + 0x6), get32s(fx.sprite + 0x10),
                       (unsigned)(get32(fx.sprite + 0x64) - (u32)(uintptr_t)fx.script),
                       get16s(retail.sprite + 0x6), get32s(retail.sprite + 0x10),
                       (unsigned)(get32(retail.sprite + 0x64) - (u32)(uintptr_t)fx.script));
            }
            ++nbad;
            /* Continue on the retail state so the summary is retail's path. */
            fx = retail;
            g_WorkListCurTimer = retailTimer;
        }
        if (f < traceFrames) {
            printf("SAVEPT TRACE %s f=%u pc=+0x%X sp=0x%02X Y=%d vel=%d wait=%d AC=%08X A8=%08X\n",
                   name, f, (unsigned)(get32(fx.sprite + 0x64) - (u32)(uintptr_t)fx.script),
                   fx.sprite[0x8C], get16s(fx.sprite + 0x6), get32s(fx.sprite + 0x10),
                   get16s(fx.sprite + 0x9E), get32(fx.sprite + 0xAC), get32(fx.sprite + 0xA8));
            fflush(stdout);
        }
        if (f < 400) window_add(&head, f == 0);
        if (f >= frames - 400) window_add(&tail, f == frames - 400);
    }
    printf("SAVEPT %s scenario=%s frames=%u first400 Y=[%d,%d] vel=[%d,%d] "
           "last400 Y=[%d,%d] vel=[%d,%d] final Y=%d vel=%d parentY=%d frame-selects=%u\n",
           native_only ? "NATIVE-ONLY" : (nbad ? "FAIL" : "PASS"), name, frames,
           head.minY, head.maxY, head.minVel, head.maxVel,
           tail.minY, tail.maxY, tail.minVel, tail.maxVel,
           get16s(fx.sprite + 0x6), get32s(fx.sprite + 0x10), get16s(fx.parent + 0x6),
           nFrameSelect);
    if (nbad) {
        printf("SAVEPT FAIL scenario=%s %u/%u frames diverged, first at frame %u\n",
               name, nbad, frames, firstBad);
    }
    return nbad;
}

static void synth(u8* out, unsigned* len)
{
    /* Same shape as the disc loop, authored operands/counts. */
    static const u8 k[] = {
        0xA3, 0x00,             /* 00 gravity 0 */
        0xBC, 0x96,             /* 02 loop: snap to parent */
        0xB4, 0x02,             /* 04 push 2 */
        0xA6, 0xFE,             /* 06 L1: impulse -2 */
        0xB4, 0x03,             /* 08 push 3 */
        0xCE, 0x01, 0x00,       /* 0A L2: spin */
        0x31,                   /* 0D wait 2 */
        0xE4, 0xFC, 0xFF,       /* 0E -> 0A */
        0xE4, 0xF5, 0xFF,       /* 11 -> 06 */
        0xB4, 0x02,             /* 14 push 2 */
        0xA6, 0x02,             /* 16 L3: impulse +2 */
        0xB4, 0x03,             /* 18 push 3 */
        0xCE, 0x01, 0x00,       /* 1A L4: spin */
        0x31,                   /* 1D wait 2 */
        0xE4, 0xFC, 0xFF,       /* 1E -> 1A */
        0xE4, 0xF5, 0xFF,       /* 21 -> 16 */
        0xE1, 0xDE, 0xFF,       /* 24 -> 02 */
    };
    memcpy(out, k, sizeof(k));
    *len = sizeof(k);
}

int main(int argc, char** argv)
{
    static u8 code[0x100];
    unsigned len = 0;
    unsigned bad = 0;
    unsigned frames = 1200;
    int native_only = 0;
    int i;

    (void)h_lcg; /* shared harness helper, unused here */
    h_load_retail();
    if (memcmp(D_8004FC40, h_ram + (0x8004FC40u & 0x1FFFFFu), 256) != 0) {
        fprintf(stderr, "SAVEPT FAIL native D_8004FC40 differs from retail\n");
        return 2;
    }
    h_region(&fx, sizeof(fx));
    h_map(0x80059198u, &D_80059198, 4);
    h_map(0x800591ADu, &D_800591AD, 1);
    h_map(0x80059428u, &g_WorkListCurTimer, 4);

    for (i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--native-only")) {
            native_only = 1;
        } else if (!strncmp(argv[i], "--trace=", 8)) {
            traceFrames = (unsigned)strtoul(argv[i] + 8, NULL, 0);
        } else if (!strncmp(argv[i], "--frames=", 9)) {
            frames = (unsigned)strtoul(argv[i] + 9, NULL, 0);
        }
    }

    synth(code, &len);
    bad += run_scenario("synth", code, len, 0, 2, frames, native_only);

    for (i = 1; i < argc; ++i) {
        FILE* f;

        if (argv[i][0] == '-') {
            continue;
        }
        f = fopen(argv[i], "rb");
        if (f == NULL) {
            fprintf(stderr, "SAVEPT FAIL cannot open %s\n", argv[i]);
            return 2;
        }
        len = (unsigned)fread(code, 1, sizeof(code), f);
        fclose(f);
        /* Extracted from A3 00 (offset 0); BC 96 at offset 2 is the loop. */
        bad += run_scenario("disc-map15-pkg5", code, len, 0, 2, frames, native_only);
    }
    printf("SAVEPT %s: %u diverging frames\n", bad ? "FAIL" : "PASS", bad);
    return bad ? 1 : 0;
}

/* Link guards: native callees the bob loop must never reach (the units are
 * whole TUs; these are not implementations and never enter the port). */
GUARD(ApplyMatrixSV) GUARD(doCOP2) GUARD(func_8001CE74) GUARD(func_8001D034)
GUARD(func_8001D3F4) GUARD(func_8001D4E8) GUARD(func_8001EE68)
GUARD(func_8001EE74) GUARD(func_8001F6B0) GUARD(func_8001FB30) GUARD(func_80022DF4)
GUARD(func_80025180) GUARD(func_80025224) GUARD(func_8002C3E8) GUARD(func_8002C59C)
GUARD(func_8002C8CC) GUARD(func_8002CB54) GUARD(func_8002CC10) GUARD(func_80039E60)
GUARD(func_800B2AEC) GUARD(HeapAlloc) GUARD(HeapChangeCurrentUser) GUARD(HeapFree)
GUARD(MFC2) GUARD(MTC2) GUARD(MulMatrix0) GUARD(ratan2)
GUARD(rcos) GUARD(ReadGeomOffset) GUARD(RotMatrix) GUARD(RotTransSV)
GUARD(rsin) GUARD(ScaleMatrix) GUARD(ScaleMatrixL) GUARD(SetRotMatrix)
GUARD(SetTransMatrix) GUARD(TimerWorkListAddTask) GUARD(TimerWorkListRemoveTask) GUARD(TimerWorkListSetTaskCallback)
GUARD(TransMatrix) GUARD(WorkListAddTask) GUARD(WorkListRemoveTask) GUARD(WorkListTaskSetOnFreeCallback)
/* Link-only data the reached paths never touch (zeroed). */
u8 D_80018644[64] __attribute__((aligned(16))); u8 D_8004FBB8[64] __attribute__((aligned(16))); u8 D_800591A8[64] __attribute__((aligned(16))); u8 D_800591AC[64] __attribute__((aligned(16)));
u8 D_800591AF[64] __attribute__((aligned(16))); u8 D_800591B0[64] __attribute__((aligned(16))); u8 D_800591B3[64] __attribute__((aligned(16))); u8 D_800592E4[64] __attribute__((aligned(16)));
u8 D_800592E8[64] __attribute__((aligned(16))); u8 D_800592EA[64] __attribute__((aligned(16))); u8 D_800592EC[64] __attribute__((aligned(16))); u8 D_8005A474[64] __attribute__((aligned(16)));
u8 D_8006BE10[64] __attribute__((aligned(16))); u8 D_8006F99C[64] __attribute__((aligned(16))); u8 D_8006F9AC[64] __attribute__((aligned(16))); u8 D_800C3EB0[64] __attribute__((aligned(16)));
u8 g_PsxRam[0x300000];
GUARD(func_800BA8F4)
GUARD(func_800BC158)
GUARD(func_800C11CC)
