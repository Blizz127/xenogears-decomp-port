/* Differential prover for the adopted battle RESULTS module routines.
 *
 * For every routine in pc_port/src/battle_results_host_leaves.inc this loads
 * the retail results module (extracted from ./disc at test time; argv[1]) at
 * guest 0x801DE000 and runs the routine twice from identical state:
 *
 *   retail  the MIPS bytes through the interpreter, force_interpret set;
 *   native  the same guest call through the production bridge with
 *           force_interpret clear, i.e. results_try_host(): identity gate,
 *           allowlist, dlsym-bound battle_results_func_XXXXXXXX body and the
 *           PcPort_BattleMipsResolve accessor.
 *
 * Both runs enter through PcPortMipsRun at the routine's jal target, so the
 * native side is reached exactly the way the interpreted caller reaches it.
 * Compared after each pair: the full v0, every other general register and
 * HI/LO (the scratch ones and HI/LO are seeded per case, so a body must leave
 * exactly the retail exit set and preserve the rest), all of
 * guest RAM (minus the interpreter's own frame below the entry SP), the whole
 * native g_GameState object and the scratchpad.  Every store either side makes
 * is also recorded by host address and must land inside one of those compared
 * windows, so a write into some other native copy cannot pass unseen.  The
 * native run must execute zero guest instructions and must have gone through
 * the accessor.
 *
 * Then gate controls: corrupting one immutable module byte (unobserved, or
 * through a KSEG1 alias store), a different module resident, a pending
 * delay-slot load, force_interpret, a foreign runtime and a non-allowlisted
 * results PC must all refuse native dispatch; restoring the bytes, an
 * identical-value store and writes to the excluded mutable module data must
 * not.  The accessor's fail-closed paths are checked too.
 *
 * Not covered: timing, and the order of guest accesses within a call (only
 * their end state is compared).
 */

#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include "psyq/libgpu.h"
#include "psyq/libgte.h"

#ifndef BATTLE_RUNTIME_SOURCE
#define BATTLE_RUNTIME_SOURCE "../src/battle_mips_runtime.c"
#endif
#include BATTLE_RUNTIME_SOURCE

/* Size of the production g_GameState object (pc_port/src/data_game_state.c);
 * the runner reads it from the linked object with nm -S. */
#ifndef RESULTS_GS_BYTES
#error "RESULTS_GS_BYTES must be the size of the linked g_GameState"
#endif

/* Guest RAM plus a guard tail, as in the overlay prover. */
uint8_t g_PsxRam[PSX_RAM_SIZE + 0x1000];
uint8_t g_PsxScratchpad[4096];
unsigned int MFC2(int reg) { (void)reg; abort(); }
unsigned int CFC2(int reg) { (void)reg; abort(); }
void MTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
void CTC2(unsigned int v, int reg) { (void)v; (void)reg; abort(); }
int doCOP2(int op) { (void)op; abort(); }
int g_GPUDisabledState;
unsigned g_PcPortPresentedFrames;
void ControllerPushState(void) {}
void ControllerPoll(void) {}
void PcPort_ServiceVblank(void) {}
void PcPort_PadVblankPump(void) {}
void PsyX_UpdateInput(void) {}
void PcPort_GodModeBeforeGuest(PcPortMipsCpu *cpu, uint32_t target);
void PcPort_GodModeBeforeGuest(PcPortMipsCpu *cpu, uint32_t target)
{
    (void)cpu;
    (void)target;
}
int PsyX_Sys_GetVBlankCount(void) { return 0; }
char PsyX_BeginScene(void) { return 1; }
void ClearSplits(void) { abort(); }
void DrawAllSplits(void) { abort(); }
void ParsePrimitivesLinkedList(u_long *p, int s) { (void)p; (void)s; abort(); }

/* Bumped by the runner's generated placeholders for callees nothing linked
 * provides.  An adopted results routine must never reach one. */
unsigned g_prover_placeholder_hits;

/* The production accessor, observed through -Wl,--wrap: the host TU's
 * references bind here, the definition stays the runtime's. */
uint8_t *__real_PcPort_BattleMipsResolve(uint32_t address, unsigned width,
                                         int write);
uint8_t *__wrap_PcPort_BattleMipsResolve(uint32_t address, unsigned width,
                                         int write);

#define MODULE_ADDR   0x801DE000u
#define MODULE_BYTES  0x6500u
#define MODULE_OFFSET (MODULE_ADDR - 0x80000000u)
#define STEP_LIMIT    20000000u
/* The interpreter's frame for a callee entered at BATTLE_STACK_TOP lives
 * below it; a native body keeps its frame on the host stack.  Only that
 * window is excluded -- the caller's argument slots at/above SP are compared. */
#define FRAME_SKIP_HI (BATTLE_STACK_TOP - 0x80000000u)
#define FRAME_SKIP_LO (FRAME_SKIP_HI - 0x1000u)
#define RANDOM_CASES  96u

static BattleMipsRuntime rt;
static PcPortMipsCpu cpu;
/* Guest SP at the routine's entry.  BATTLE_STACK_TOP unless a shaper moves
 * it: a routine with a stack argument (func_801E1370) must read it relative
 * to the real entry SP, so its shaper varies the SP (8-aligned, inside the
 * excluded frame window, which only the interpreter's own frame uses). */
static uint32_t case_sp = BATTLE_STACK_TOP;
static uint8_t module_image[MODULE_BYTES];
static uint8_t *native_gs;

static uint8_t before_ram[PSX_RAM_SIZE];
static uint8_t before_gs[RESULTS_GS_BYTES];
static uint8_t before_pad[4096];
static uint8_t retail_ram[PSX_RAM_SIZE];
static uint8_t retail_gs[RESULTS_GS_BYTES];
static uint8_t retail_pad[4096];

static unsigned failures;
static unsigned checks;
static unsigned controls;
static unsigned accessor_calls;
static unsigned stray_writes;
static uintptr_t stray_first;

static uint32_t rng_state;
static uint32_t rnd(void)
{
    rng_state = rng_state * 1664525u + 1013904223u;
    return rng_state >> 8;
}

static void fail(const char *name, const char *what, const char *detail)
{
    fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL %s %s %s\n", name, what,
            detail);
    failures++;
}

/* Every store either side makes must land in a compared window. */
static void note_write(const uint8_t *p, unsigned width)
{
    uintptr_t a = (uintptr_t)p;
    if (a >= (uintptr_t)g_PsxRam && a + width <= (uintptr_t)g_PsxRam + PSX_RAM_SIZE)
        return;
    if (a >= (uintptr_t)native_gs && a + width <= (uintptr_t)native_gs + RESULTS_GS_BYTES)
        return;
    if (a >= (uintptr_t)g_PsxScratchpad &&
        a + width <= (uintptr_t)g_PsxScratchpad + sizeof(g_PsxScratchpad))
        return;
    if (stray_writes++ == 0)
        stray_first = a;
}

uint8_t *__wrap_PcPort_BattleMipsResolve(uint32_t address, unsigned width,
                                         int write)
{
    uint8_t *p = __real_PcPort_BattleMipsResolve(address, width, write);
    accessor_calls++;
    if (write)
        note_write(p, width);
    return p;
}

static int recording_write(void *opaque, uint32_t address, unsigned width,
                           uint32_t value)
{
    uint8_t *p = resolve_memory(opaque, address, width, 1);
    if (p != NULL)
        note_write(p, width);
    return runtime_write(opaque, address, width, value);
}

/* Shape test state through the production mapping, like the code would. */
static void poke8(uint32_t address, uint8_t value)
{
    uint8_t *p = resolve_memory(&rt, address, 1, 0);
    if (p == NULL) {
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL harness poke 0x%08x\n",
                address);
        exit(2);
    }
    *p = value;
}

static void poke16(uint32_t address, uint16_t value)
{
    poke8(address, (uint8_t)value);
    poke8(address + 1u, (uint8_t)(value >> 8));
}

static void poke32(uint32_t address, uint32_t value)
{
    poke16(address, (uint16_t)value);
    poke16(address + 2u, (uint16_t)(value >> 16));
}

static void load_module_image(void)
{
    memcpy(g_PsxRam + MODULE_OFFSET, module_image, MODULE_BYTES);
}

/* Fresh random world, then the retail module on top. */
static void begin_case(uint32_t seed)
{
    size_t i;
    rng_state = seed;
    for (i = 0; i < PSX_RAM_SIZE; i++)
        g_PsxRam[i] = (uint8_t)rnd();
    for (i = 0; i < RESULTS_GS_BYTES; i++)
        native_gs[i] = (uint8_t)rnd();
    for (i = 0; i < sizeof(g_PsxScratchpad); i++)
        g_PsxScratchpad[i] = (uint8_t)rnd();
    load_module_image();
    case_sp = BATTLE_STACK_TOP;
}

/* ---- per-routine case shapers ------------------------------------------ */

/* func_801E24B0: ids D_800D2FE4[48], values D_800D2CB0[48], table
 * g_GameState+0x2026 (0x8006F65A)[150], destination g_GameState+0x1F90
 * (0x8006F5C4)[150].  Edge cases first, then random alphabets small enough
 * to produce many matches and duplicate ids (a later id must win). */
static void shape_801E24B0(unsigned index, uint32_t args[4])
{
    unsigned j, i, alphabet;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    switch (index) {
    case 0: /* all ids zero: nothing may be written */
        for (j = 0; j < 48; j++) poke8(0x800D2FE4u + j, 0);
        return;
    case 1: /* every id matches every entry: all 150 get values[47] */
        for (j = 0; j < 48; j++) poke8(0x800D2FE4u + j, 1);
        for (i = 0; i < 150; i++) poke8(0x8006F65Au + i, 1);
        return;
    case 2: /* 0xFF ids against a 0xFF table */
        for (j = 0; j < 48; j++) poke8(0x800D2FE4u + j, 0xFF);
        for (i = 0; i < 150; i++) poke8(0x8006F65Au + i, 0xFF);
        return;
    case 3: /* only j=0/i=0 and j=47/i=149 match */
        for (j = 0; j < 48; j++) poke8(0x800D2FE4u + j, (uint8_t)(0x40 + j));
        for (i = 0; i < 150; i++) poke8(0x8006F65Au + i, 0x10);
        poke8(0x8006F65Au + 0, 0x40);
        poke8(0x8006F65Au + 149, 0x40 + 47);
        return;
    case 4: /* duplicate id at j=3, 20, 47: the last one wins */
        for (j = 0; j < 48; j++) poke8(0x800D2FE4u + j, 0);
        poke8(0x800D2FE4u + 3, 5);
        poke8(0x800D2FE4u + 20, 5);
        poke8(0x800D2FE4u + 47, 5);
        for (i = 0; i < 150; i += 7) poke8(0x8006F65Au + i, 5);
        return;
    case 5: /* zero table entries never match a zero id (skipped) */
        for (j = 0; j < 48; j++) poke8(0x800D2FE4u + j, (uint8_t)(j & 1 ? 0 : 9));
        for (i = 0; i < 150; i++) poke8(0x8006F65Au + i, 0);
        return;
    case 6: /* the guest bytes behind the native table hold the ids, the
             * native table does not: only a correct mapping stays silent */
        for (j = 0; j < 48; j++) poke8(0x800D2FE4u + j, (uint8_t)(j + 1));
        for (i = 0; i < 150; i++) {
            poke8(0x8006F65Au + i, 0xEE);
            g_PsxRam[0x6F65Au + i] = (uint8_t)(i % 48 + 1);
        }
        return;
    default:
        break;
    }
    alphabet = (unsigned[]){2, 4, 8, 16, 256}[index % 5u];
    for (j = 0; j < 48; j++)
        poke8(0x800D2FE4u + j, (uint8_t)(rnd() % alphabet));
    for (i = 0; i < 150; i++)
        poke8(0x8006F65Au + i, (uint8_t)(rnd() % alphabet));
}

/* Guest pointers the shapers hand to pointer-driven routines: 4-aligned, in
 * plain guest RAM above the battle overlay (compared, not natively mirrored),
 * clear of the results module (0x801DE000..0x801E4500) and of the
 * interpreter's frame window below BATTLE_STACK_TOP, so every store lands in
 * a compared window.  `span` is how far past the pointer the routine reaches. */
static uint32_t guest_buffer(uint32_t span)
{
    for (;;) {
        uint32_t p = 0x800D8000u + ((rnd() % (0x801D0000u - 0x800D8000u)) & ~3u);
        if (p + span <= 0x801DE000u)
            return p;
    }
}

/* func_801DF910: arguments only (a0/a1 masked bytes, a2 a full 32-bit DIV
 * divisor); writes 0x801E44CC..0x801E44E4.  a2 == 0 exercises the R3000A
 * divide-by-zero result, -1 and 0x80000000 the signed edges.  In play a2 is
 * func_801DFA38's 0..255 result. */
static void shape_801DF910(unsigned index, uint32_t args[4])
{
    static const uint32_t edges[8][3] = {
        {0u, 0u, 0u},           /* 0/0 twice */
        {255u, 0u, 0u},         /* negative delta, divisor 0 */
        {0u, 255u, 255u},
        {200u, 100u, 1u},       /* large quotients through the magic mult */
        {37u, 201u, 0xFFFFFFFFu},
        {201u, 37u, 0x80000000u},
        {0xFFFFFF10u, 0x12345620u, 7u}, /* dirty upper bits are masked */
        {99u, 99u, 0u},         /* zero delta, 0/0 */
    };
    unsigned j;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    if (index < 8) {
        args[0] = edges[index][0];
        args[1] = edges[index][1];
        args[2] = edges[index][2];
        return;
    }
    switch (index % 4u) {
    case 0: args[2] = rnd() & 0xFFu; break;   /* the real 0..255 range */
    case 1: args[2] = rnd() % 4u; break;      /* 0..3, zero included */
    case 2: args[2] = rnd() ^ (rnd() << 24); break; /* any 32-bit divisor */
    default: args[2] = (rnd() & 0xFFu) | ((rnd() & 1u) ? 0x80000000u : 0u); break;
    }
}

/* func_801DFA38: a0 & 0xFF selects 7+7 bytes of D_800CDD10 / D_800CDD28
 * (guest BSS, 8-byte rows).  Read-only. */
static void shape_801DFA38(unsigned index, uint32_t args[4])
{
    unsigned j, row, alphabet;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    row = args[0] & 0xFFu;
    switch (index) {
    case 0: /* all zero: the maximum stays the initial 0 */
        for (j = 0; j < 0x820u; j++) poke8(0x800CDD10u + j, 0);
        args[0] = 0;
        return;
    case 1: /* all 0xFF at the last row */
        for (j = 0; j < 0x820u; j++) poke8(0x800CDD10u + j, 0xFF);
        args[0] = 0xFF;
        return;
    case 2: /* max in the first byte of the first window only */
        for (j = 0; j < 7; j++) {
            poke8(0x800CDD10u + row * 8u + j, 1);
            poke8(0x800CDD28u + row * 8u + j, 1);
        }
        poke8(0x800CDD10u + row * 8u, 0xC0);
        return;
    case 3: /* max in the last byte of the second window only */
        for (j = 0; j < 7; j++) {
            poke8(0x800CDD10u + row * 8u + j, 2);
            poke8(0x800CDD28u + row * 8u + j, 2);
        }
        poke8(0x800CDD28u + row * 8u + 6u, 0x81);
        return;
    case 4: /* dirty upper bits; byte 7 of each row (not read) is the max */
        args[0] = 0xABCDEF05u;
        poke8(0x800CDD10u + 5u * 8u + 7u, 0xFF);
        poke8(0x800CDD28u + 5u * 8u + 7u, 0xFF);
        return;
    default:
        break;
    }
    alphabet = (unsigned[]){2, 3, 16, 256}[index % 4u];
    for (j = 0; j < 7; j++) {
        poke8(0x800CDD10u + row * 8u + j, (uint8_t)(rnd() % alphabet));
        poke8(0x800CDD28u + row * 8u + j, (uint8_t)(rnd() % alphabet));
    }
}

/* func_801E1590: (a0 ids[8], a1 counts[8], a2 kinds[8]) guest buffers --
 * in play sp+24 / sp+40 / sp+32 of 0x801E1690's frame -- and the 8 pending
 * drops D_800CDCF4 (kinds) / D_800CDCFC (ids).  kinds[] is not cleared by
 * retail, so it keeps its random bytes. */
static void shape_801E1590(unsigned index, uint32_t args[4])
{
    unsigned j, alphabet;
    uint32_t base;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    base = guest_buffer(0x20u);
    args[0] = base;       /* ids    = sp+24 */
    args[2] = base + 8u;  /* kinds  = sp+32 */
    args[1] = base + 16u; /* counts = sp+40 */
    switch (index) {
    case 0: /* no pending drop: only the clearing runs */
        for (j = 0; j < 8; j++) poke8(0x800CDCFCu + j, 0);
        return;
    case 1: /* 8 identical (kind, id): one slot counting 8 */
        for (j = 0; j < 8; j++) {
            poke8(0x800CDCF4u + j, 3);
            poke8(0x800CDCFCu + j, 0x41);
        }
        return;
    case 2: /* 8 distinct: 8 slots */
        for (j = 0; j < 8; j++) {
            poke8(0x800CDCF4u + j, (uint8_t)j);
            poke8(0x800CDCFCu + j, (uint8_t)(0x10 + j));
        }
        return;
    case 3: /* same id, alternating kinds; one zero id in the middle */
        for (j = 0; j < 8; j++) {
            poke8(0x800CDCF4u + j, (uint8_t)(j & 1));
            poke8(0x800CDCFCu + j, j == 4 ? 0 : 0x22);
        }
        return;
    case 4: /* stale kinds[] equal to every pending kind */
        for (j = 0; j < 8; j++) {
            poke8(0x800CDCF4u + j, 2);
            poke8(0x800CDCFCu + j, (uint8_t)(1 + (j % 3u)));
            poke8(args[2] + j, 2);
        }
        return;
    case 5: /* separate, non-adjacent buffers */
        args[0] = guest_buffer(8u);
        args[1] = guest_buffer(8u);
        args[2] = guest_buffer(8u);
        break;
    case 6: /* counts overlapping ids (aliasing) */
        args[1] = base + 4u;
        break;
    case 7: /* the kinds buffer is the pending-kinds table itself */
        args[2] = 0x800CDCF4u;
        break;
    default:
        if (index % 3u == 0) {
            args[0] = guest_buffer(8u);
            args[1] = guest_buffer(8u);
            args[2] = guest_buffer(8u);
        }
        break;
    }
    alphabet = (unsigned[]){2, 3, 5, 256}[index % 4u];
    for (j = 0; j < 8; j++) {
        poke8(0x800CDCF4u + j, (uint8_t)(rnd() % alphabet));
        poke8(0x800CDCFCu + j, (uint8_t)(rnd() % alphabet));
    }
}

/* func_801E2888 reads W = word 0x801E44C4 (character records W+620+164*n and
 * gear records W+2424+164*m) and S = word 0x801E44C8 (three 0x170-byte slot
 * records), D_800D2D24[3] (0xFF = empty slot) and the D_800CCE42 row flags.
 * `wide` keeps every index byte so W must leave room for 255*164. */
static void shape_801E2888(unsigned index, uint32_t args[4])
{
    unsigned j, k;
    uint32_t w, s;
    int wide = index >= 8 && index % 4u == 0;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    w = guest_buffer(wide ? 0xB000u : 0x1800u);
    s = guest_buffer(0x500u);
    if (index == 6) {
        /* Records inside the native g_GameState copy: only the production
         * mapping reads and writes the same bytes as the interpreter. */
        w = 0x8006D700u;
        s = 0x8006E900u;
    } else if (index == 7 || (index >= 8 && index % 7u == 0)) {
        s = w + ((rnd() % 0x40u) << 2); /* slot records alias W's */
    }
    poke32(0x801E44C4u, w);
    poke32(0x801E44C8u, s);
    for (k = 0; k < 3; k++) {
        uint32_t slot = s + 0x170u * k;
        uint8_t kind = (uint8_t)(rnd() % 11u);
        uint8_t gear = (uint8_t)(rnd() % 18u);
        poke8(0x800D2D24u + k, (uint8_t)((rnd() & 3u) == 0 ? 0xFF : k));
        if (wide) {
            kind = (uint8_t)rnd();
            gear = (uint8_t)rnd();
        } else if ((rnd() & 3u) == 0) {
            kind = 7; /* the EXP /100 path */
        }
        if ((rnd() & 7u) == 0)
            kind = (uint8_t)(gear + 11u); /* A == B */
        poke8(slot + 0x56u, kind);
        poke8(slot + 0xA0u, gear);
        if ((rnd() & 3u) == 0)
            poke32(slot + 0x104u, (uint32_t[]){0xFFFFFFFFu, 0u, 98u, 99u}[rnd() & 3u]);
    }
    switch (index) {
    case 0: /* every slot empty */
        for (k = 0; k < 3; k++) poke8(0x800D2D24u + k, 0xFF);
        break;
    case 1: /* kind 7 + flag: 0xFFFFFFFF + 1 wraps to 0, stored 0 -> 1 */
        poke8(0x800D2D24u, 0);
        poke8(s + 0x56u, 7);
        poke8(0x800CCE42u, 0x80);
        poke32(s + 0x104u, 0xFFFFFFFFu);
        break;
    case 2: /* 99/100 == 0 -> 1, and the 0xC000 override */
        poke8(0x800D2D24u + 1u, 1);
        poke8(s + 0x170u + 0x56u, 7);
        poke8(0x800CCE42u + 0x170u, 0xFF);
        poke32(s + 0x170u + 0x104u, 98u);
        poke16(s + 0x170u + 0x7Cu, 0x4000u);
        break;
    case 3: /* A == B for every slot */
        for (k = 0; k < 3; k++) {
            poke8(0x800D2D24u + k, (uint8_t)k);
            poke8(s + 0x170u * k + 0xA0u, (uint8_t)(3u + k));
            poke8(s + 0x170u * k + 0x56u, (uint8_t)(14u + k));
        }
        break;
    case 4: /* gear bounds: 7 and 17 skip, 16 and 8 run */
        for (k = 0; k < 3; k++) poke8(0x800D2D24u + k, (uint8_t)k);
        poke8(s + 0xA0u, 7);
        poke8(s + 0x170u + 0xA0u, 17);
        poke8(s + 0x2E0u + 0xA0u, (rnd() & 1u) ? 16 : 8);
        break;
    case 5: /* the /10 path on a 0xFFFFFFFF value, all slots */
        for (k = 0; k < 3; k++) {
            uint8_t gear = (uint8_t)(k * 6u);
            poke8(0x800D2D24u + k, (uint8_t)k);
            poke8(s + 0x170u * k + 0xA0u, gear);
            poke16(s + 0x170u * k + 0xA4u + 0x7Cu, 0x8000u);
            poke32(w + 2424u + 164u * gear + 0x64u, 0xFFFFFFFFu);
        }
        break;
    default:
        break;
    }
}

/* func_801E403C: W = word 0x801E44C4 (states W+0x16C0+32*i+0x17, levels
 * (W+620+164*i)[0x62]), T = word 0x801E44E8 (thresholds T+0x110*i+0xCC..CE),
 * pad flag g_GameState u16 0x8006F8EA bit 0x4000; i = 0..10. */
static void shape_801E403C(unsigned index, uint32_t args[4])
{
    unsigned j, i;
    uint32_t w, tbl;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    w = guest_buffer(0x2000u); /* 0x1818 used; room for the overlap case */
    tbl = guest_buffer(0xC00u);
    if (index == 5) {
        w = 0x8006D700u;   /* both inside the native g_GameState copy */
        tbl = 0x8006E200u;
    } else if (index >= 6 && index % 5u == 0) {
        tbl = w + ((rnd() % 0x120u) << 2); /* overlapping tables */
    }
    poke32(0x801E44C4u, w);
    poke32(0x801E44E8u, tbl);
    for (i = 0; i < 11; i++) {
        uint32_t state = w + 0x16C0u + 32u * i + 0x17u;
        uint32_t level = w + 620u + 164u * i + 0x62u;
        uint32_t row = tbl + 0x110u * i;
        switch (index) {
        case 0: poke8(state, 0); break;
        case 1: poke8(state, 3); poke8(level, 0xFF); poke8(row + 0xCCu, 0); break;
        case 2: /* level == threshold promotes, threshold - 1 does not */
            poke8(state, (uint8_t)(3u + i % 3u));
            poke8(row + 0xCCu + i % 3u, 40);
            poke8(level, (uint8_t)((i & 4u) ? 39 : 40));
            break;
        case 3: /* state 6: level 49/50 x pad flag set/clear */
            poke8(state, 6);
            poke8(level, (uint8_t)((i & 1u) ? 50 : 49));
            break;
        case 4: poke8(state, (uint8_t)(i & 1u ? 7 : (i & 2u ? 8 : 0xFF))); break;
        default:
            poke8(state, (uint8_t)((rnd() & 1u) ? 3u + rnd() % 4u : rnd() % 9u));
            if ((rnd() & 3u) == 0) poke8(state, (uint8_t)rnd());
            poke8(level, (uint8_t)((rnd() & 1u) ? 49u + rnd() % 3u : rnd()));
            break;
        }
    }
    if (index == 3)
        poke16(0x8006F8EAu, (uint16_t)((rnd() & 1u) ? 0x4000u : 0xBFFFu));
    else if (index >= 6)
        poke16(0x8006F8EAu, (uint16_t)(rnd() ^ ((rnd() & 1u) ? 0x4000u : 0u)));
}

/* func_801E41B4: c_k = D_800CCD3E[0x170*k] (guest BSS), level byte
 * 0x8006D902 + 164*c, badge u16 0x8006ECF8 + 32*c.  c <= 100 keeps the badge
 * inside the native g_GameState copy, c >= 112 puts it in guest RAM above the
 * overlay base; 101..111 would land in other main-exe data no prover window
 * covers, so they are not generated (party ids in play are 0..10). */
static void shape_801E41B4(unsigned index, uint32_t args[4])
{
    static const uint8_t levels[] = {0, 1, 49, 50, 59, 60, 69, 70, 99, 255};
    unsigned j, k;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    for (k = 0; k < 3; k++) {
        uint8_t id;
        switch (index) {
        case 0: id = 0; break;                       /* same id on all rows */
        case 1: id = (uint8_t)(1u + k); break;
        case 2: id = (uint8_t)(4u + k); break;
        case 3: id = (uint8_t)(112u + 71u * k); break; /* guest-RAM badges */
        case 4: id = (uint8_t)(98u + k); break;        /* top of the native copy */
        default:
            switch (rnd() % 4u) {
            case 0: id = (uint8_t)(rnd() % 101u); break;
            case 1: id = (uint8_t)(112u + rnd() % 144u); break;
            default: id = (uint8_t)(rnd() % 11u); break;
            }
            break;
        }
        poke8(0x800CCD3Eu + 0x170u * k, id);
        if (index == 0)
            poke8(0x8006D902u, 99);
        else if (index == 1)
            poke8(0x8006D902u + 164u * id, (uint8_t[]){50, 60, 70}[k]);
        else if (index == 2)
            poke8(0x8006D902u + 164u * id, (uint8_t[]){49, 59, 69}[k]);
        else
            poke8(0x8006D902u + 164u * id, levels[rnd() % sizeof(levels)]);
        if (index >= 5 && (rnd() & 3u) == 0)
            poke8(0x8006D902u + 164u * id, (uint8_t)rnd());
    }
}

/* ---- leaves batch 3 ---------------------------------------------------- */

static uint32_t dirty_byte_arg(uint8_t value)
{
    return (rnd() & 1u) ? (rnd() << 8) | value : value;
}

/* func_801E1370(id a0, count a1, keys a2, values a3, n = byte at entry
 * sp+0x10).  In play the tables are the five pairs in the native g_GameState
 * copy that 0x801E1444 passes; the shaper also uses guest buffers, aliasing
 * tables and every n.  The argument word's upper bytes are random: only the
 * lbu'd low byte may count. */
static void shape_801E1370(unsigned index, uint32_t args[4])
{
    static const uint32_t tables[5][3] = {
        {0x8006F3D0u, 0x8006F36Cu, 100u}, {0x8006F4FCu, 0x8006F434u, 200u},
        {0x8006F65Au, 0x8006F5C4u, 150u}, {0x8006F754u, 0x8006F6F0u, 100u},
        {0x8006F84Eu, 0x8006F7B8u, 150u},
    };
    unsigned j, which = rnd() % 5u;
    uint32_t keys, vals, n;
    uint8_t id;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    case_sp = BATTLE_STACK_TOP - 8u * (rnd() % 64u);
    keys = tables[which][0];
    vals = tables[which][1];
    n = tables[which][2];
    if (index >= 8 && index % 3u != 0) {
        keys = guest_buffer(0x108u) + (rnd() & 3u);
        vals = guest_buffer(0x108u) + 4u + (rnd() & 3u);
        if (index % 7u == 1)
            vals = keys + (rnd() % 9u) - 4u; /* overlapping tables */
        n = (uint32_t[]){0u, 1u, 2u, 3u, 8u, 100u, 150u, 200u, 255u,
                         rnd() & 0xFFu}[rnd() % 10u];
    }
    id = (uint8_t)((uint32_t[]){0u, 1u, 0xFFu, rnd(), rnd() & 3u}[rnd() % 5u]);
    args[0] = dirty_byte_arg(id);
    args[1] = (uint32_t[]){rnd() ^ (rnd() << 24), rnd() & 0xFFu, 99u, 100u, 1u,
                           0u}[rnd() % 6u];
    for (j = 0; j < 256u; j++) /* every real table fits below 0x8006F98C */
        poke8(keys + j, (uint8_t)((uint32_t[]){0u, id, rnd(), rnd() & 3u,
                                               id}[rnd() % 5u]));
    switch (index) {
    case 0: /* n == 0: nothing is read past the argument */
        n = 0;
        break;
    case 1: /* hit, masked sum < 100: the FULL a1 is added (sb keeps the low byte) */
        args[0] = 0x41u;
        args[1] = 0x12345605u;
        for (j = 0; j < n; j++) poke8(keys + j, 0x30);
        poke8(keys + 5u, 0x41);
        poke8(vals + 5u, 10);
        break;
    case 2: /* hit, masked sum >= 100 stores 99 */
        args[0] = 0xFFFFFF41u;
        args[1] = 5u;
        for (j = 0; j < n; j++) poke8(keys + j, 0x30);
        poke8(keys + 7u, 0x41);
        poke8(vals + 7u, 95);
        break;
    case 3: /* miss, the only free key is the last one */
        args[0] = 0x42u;
        for (j = 0; j < n; j++) poke8(keys + j, 0x30);
        poke8(keys + n - 1u, 0);
        break;
    case 4: /* miss, table full (keys 1..200, id 0xF0): no store */
        args[0] = 0xF0u;
        for (j = 0; j < n; j++) poke8(keys + j, (uint8_t)(1u + j % 200u));
        break;
    case 5: /* id 0 matches the first zero key as a hit */
        args[0] = 0xAB00u;
        args[1] = 0xFFFFFF63u;
        for (j = 0; j < n; j++) poke8(keys + j, (uint8_t)(j < 9u ? 7u : 0u));
        break;
    case 6: /* n = 255 in a word with every upper bit set, guest tables */
        keys = guest_buffer(0x100u);
        vals = guest_buffer(0x100u);
        n = 255u;
        break;
    case 7: /* values overlap keys by one byte */
        keys = guest_buffer(0x104u);
        vals = keys + 1u;
        n = (uint32_t)(1u + rnd() % 255u);
        break;
    default:
        break;
    }
    args[2] = keys;
    args[3] = vals;
    poke32(case_sp + 0x10u, (index == 6 ? 0xFFFFFF00u : rnd() << 8) | n);
}

/* func_801E2EB0(share a0, slot a1, mode a2), both s16-truncated.  Reads the
 * slot record weights (word 0x801E44C8 + 0x170*(s16)a1 + 0x158/0x159), the
 * 0x8000 flag at (word 0x801E44C4) + 0x22B6 (g_GameState in play) and the
 * 0x1000/0x2000 flags at (word 0x801E44EC) + 0x32; writes 0x801E44F0/F4
 * and word 0x801E44C8 + 8*(s16)a1 + 0xFE8/0xFEC.  Slot indices cover the
 * real 0..2, the early path's 0xFF, and negative s16 values. */
static void shape_801E2EB0(unsigned index, uint32_t args[4])
{
    static const uint32_t slots[] = {0u, 1u, 2u, 0xFFu, 0xFFFFu, 0xFFFEu,
                                     0x10002u, 0xABCD0001u};
    static const uint32_t modes[] = {0u, 0u, 0u, 1u, 0x10001u, 0xFFFF0001u,
                                     2u, 0xFFFFu};
    static const uint8_t weights[] = {0u, 1u, 2u, 3u, 255u};
    uint32_t t1, g, e;
    int k;

    args[0] = (uint32_t[]){0u, 1u, 3u, 0xFFFFFFFFu, 0x55555556u, rnd() ^ (rnd() << 24),
                           rnd() % 100000u}[rnd() % 7u];
    args[1] = slots[rnd() % 8u];
    args[2] = (rnd() & 1u) ? modes[rnd() % 8u] : rnd() ^ (rnd() << 24);
    args[3] = rnd();
    t1 = guest_buffer(0x18000u) + 0x400u;
    g = (rnd() & 3u) ? 0x8006D634u : guest_buffer(0x2400u);
    e = guest_buffer(0x40u);
    if (index == 7 || (index >= 8 && index % 9u == 0))
        e = 0x8006E000u; /* the flag record inside the native copy */
    poke32(0x801E44C8u, t1);
    poke32(0x801E44C4u, g);
    poke32(0x801E44ECu, e);
    for (k = -2; k <= 3; k++) {
        poke8(t1 + 0x170u * (uint32_t)k + 0x158u, weights[rnd() % 5u]);
        poke8(t1 + 0x170u * (uint32_t)k + 0x159u, weights[rnd() % 5u]);
    }
    poke8(t1 + 0x170u * 0xFFu + 0x158u, (uint8_t)rnd());
    poke8(t1 + 0x170u * 0xFFu + 0x159u, (uint8_t)rnd());
    poke16(g + 0x22B6u, (uint16_t)(rnd() & ((rnd() & 1u) ? 0x7FFFu : 0xFFFFu)));
    poke16(e + 0x32u, (uint16_t)((uint32_t[]){0u, 0x1000u, 0x2000u, 0x3000u,
                                              rnd()}[rnd() % 5u]));
    switch (index) {
    case 0: /* the caller's early path: a2 = 1, a1 = 0xFF */
        args[1] = 0xFFu;
        args[2] = 1u;
        break;
    case 1: /* s16 truncation of the mode: 0x10001 is 1 */
        args[2] = 0x10001u;
        break;
    case 2: /* both weights 0 -> 1/1, the full 32-bit share */
        args[0] = 0xFFFFFFFFu;
        args[1] = 1u;
        args[2] = 0u;
        poke8(t1 + 0x170u + 0x158u, 0);
        poke8(t1 + 0x170u + 0x159u, 0);
        break;
    case 3: /* the 0x8000 override with a zero share: both 0 -> 1 */
        args[0] = 0u;
        args[1] = 2u;
        args[2] = 0u;
        poke16(g + 0x22B6u, 0x8000u);
        poke16(e + 0x32u, 0);
        break;
    case 4: /* both x1.5 boosts on huge values (32-bit wrap) */
        args[0] = 0xFFFFFFF0u;
        args[1] = 0u;
        args[2] = 0u;
        poke16(g + 0x22B6u, 0x8000u);
        poke16(e + 0x32u, 0x3000u);
        break;
    case 5: /* slot -1 (a1 = 0xFFFF): records below the slot base */
        args[1] = 0xFFFFu;
        args[2] = 0u;
        break;
    case 6: /* weights 255/255 */
        args[1] = 0u;
        args[2] = 0u;
        poke8(t1 + 0x158u, 255);
        poke8(t1 + 0x159u, 255);
        break;
    case 7: /* a0 = 0, no flags: both 0 -> 1 */
        args[0] = 0u;
        args[2] = 0u;
        poke16(g + 0x22B6u, 0);
        poke16(e + 0x32u, 0);
        break;
    default:
        break;
    }
}

/* A character index k = a0 & 0xFF for the 0x8006xxxx per-character tables
 * (stores at 0x8006ECF4 + 32k + 0..6, 0x8006D634 + 32k + 0x16C2..0x16C6):
 * k <= 100 keeps them in the native g_GameState copy, k >= 112 puts them in
 * guest RAM above the overlay base; 101..111 would store into main-exe data
 * between the copy's end (0x8006F98C) and BATTLE_BASE that no prover window
 * covers, so they are not generated (party kinds in play are 0..10). */
static uint8_t shape_kind(void)
{
    switch (rnd() % 4u) {
    case 0: return (uint8_t)(rnd() % 101u);
    case 1: return (uint8_t)(112u + rnd() % 144u);
    default: return (uint8_t)(rnd() % 11u);
    }
}

/* Level byte 0x8006D902 + 164k.  k = 51/52 put it in that same uncovered
 * main-exe data (0x8006F9AE / 0x8006FA52): the routines only read it there,
 * so the harness leaves those bytes alone rather than poke host data. */
static void poke_level(uint8_t k, uint8_t level)
{
    if (k != 51u && k != 52u)
        poke8(0x8006D902u + 164u * k, level);
}

/* Shared world for the 0x801E3A18 helpers: D = word 0x801E44E8 (data rows
 * 0x110 apart), G = word 0x801E44C4 (g_GameState in play, else a guest
 * buffer), E = word 0x801E44EC (the slot record), the 0x4000 pad flag, the
 * learned mask 0x8006ECF4 + 32k and the level 0x8006D902 + 164k. */
static uint32_t helper_d, helper_g, helper_e;

static void shape_helper_world(unsigned index, uint8_t k)
{
    helper_d = guest_buffer(0x11300u);
    helper_g = (rnd() & 3u) ? 0x8006D634u : guest_buffer(0x3800u);
    helper_e = guest_buffer(0xA0u);
    if (index % 11u == 10u) {
        /* rows and record inside the native copy (k <= 10 fits) */
        helper_d = 0x8006D640u;
        helper_e = 0x8006F800u;
        if (k > 10u)
            helper_d = guest_buffer(0x11300u);
    }
    poke32(0x801E44E8u, helper_d);
    poke32(0x801E44C4u, helper_g);
    poke32(0x801E44ECu, helper_e);
    poke16(0x8006F8EAu, (uint16_t)rnd());
    poke_level(k, (uint8_t)((uint32_t[]){0u, 1u, 50u, 99u, 255u, rnd()}[rnd() % 6u]));
}

/* Read test state through the production mapping (not the accessor under
 * observation). */
static uint8_t peek8(uint32_t address)
{
    uint8_t *p = resolve_memory(&rt, address, 1, 0);
    if (p == NULL) {
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL harness peek 0x%08x\n",
                address);
        exit(2);
    }
    return *p;
}

static uint16_t peek16(uint32_t address)
{
    return (uint16_t)(peek8(address) | (peek8(address + 1u) << 8));
}

static uint32_t peek32(uint32_t address)
{
    return (uint32_t)peek16(address) | ((uint32_t)peek16(address + 2u) << 16);
}

static uint8_t level_of(uint8_t k)
{
    return peek8(0x8006D902u + 164u * k);
}

/* func_801E3BE0(kind a0): candidates i < 7 (13 with pad flag 0x4000), learned
 * mask, per-candidate level gate D[0x100+i], 7 requirements D.u16[14i+2j]
 * against E.u16[0x90+2j].  Gates and requirements are drawn around the
 * actual level / stats so the equal and one-off cases all occur. */
static void shape_801E3BE0(unsigned index, uint32_t args[4])
{
    uint8_t k = shape_kind(), level;
    uint32_t row, i, j;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    if (index == 5) k = 200;
    if (index == 6) k = 100;
    shape_helper_world(index, k);
    row = helper_d + 0x110u * k;
    level = level_of(k);
    poke16(0x8006ECF4u + 32u * k, (uint16_t)((uint32_t[]){0u, 0xFFFFu, rnd(),
                                                          rnd() & rnd()}[rnd() % 4u]));
    for (i = 0; i < 13; i++) {
        poke8(row + 0x100u + i, (uint8_t)((uint32_t[]){level, level + 1u, level - 1u, 0u,
                                                       rnd()}[rnd() % 5u]));
        for (j = 0; j < 7; j++) {
            uint16_t stat = peek16(helper_e + 0x90u + 2u * j);
            poke16(row + 14u * i + 2u * j,
                   (uint16_t)((uint32_t[]){0u, stat, stat + 1u, stat - 1u, rnd()}[rnd() % 5u]));
        }
    }
    switch (index) {
    case 0: /* 7 candidates, nothing learned, everything passes: learns i = 0, returns 0 */
        poke16(0x8006F8EAu, 0);
        poke16(0x8006ECF4u + 32u * k, 0);
        for (i = 0; i < 13; i++) {
            poke8(row + 0x100u + i, 0);
            for (j = 0; j < 7; j++) poke16(row + 14u * i + 2u * j, 0);
        }
        break;
    case 1: /* 13 candidates, all but i = 12 learned: learns 12 */
        poke16(0x8006F8EAu, 0x4000u);
        poke16(0x8006ECF4u + 32u * k, 0xFFF0u);
        poke8(row + 0x100u + 12u, 0);
        for (j = 0; j < 7; j++) poke16(row + 14u * 12u + 2u * j, 0);
        break;
    case 2: /* the first unlearned candidate's gate is above the level: stop */
        poke16(0x8006ECF4u + 32u * k, 0x8000u);
        poke8(row + 0x101u, (uint8_t)(level + 1u));
        if (level == 0xFFu) {
            poke8(row + 0x101u, 0xFF);
            poke_level(k, 0xFE);
        }
        break;
    case 3: /* candidate 0 fails only its last requirement (stat = req - 1),
             * candidate 1 meets every one exactly */
        poke16(0x8006F8EAu, 0);
        poke16(0x8006ECF4u + 32u * k, 0);
        poke8(row + 0x100u, 0);
        poke8(row + 0x101u, 0);
        for (j = 0; j < 7; j++) {
            uint16_t stat = peek16(helper_e + 0x90u + 2u * j);
            if (stat == 0xFFFFu) poke16(helper_e + 0x90u + 2u * j, stat = 0xFFFEu);
            poke16(row + 2u * j, j == 6 ? (uint16_t)(stat + 1u) : stat);
            poke16(row + 14u + 2u * j, stat);
        }
        break;
    case 4: /* everything learned: none, 0 */
        poke16(0x8006ECF4u + 32u * k, 0xFFFFu);
        break;
    default:
        break;
    }
    args[0] = dirty_byte_arg(k);
}

/* func_801E3D54(kind a0): list D[0xF0 + i] (0xFF ends it, at most 12)
 * against the level, award bit 0x8000>>i in G + 32k + 0x16C2. */
static void shape_801E3D54(unsigned index, uint32_t args[4])
{
    uint8_t k = shape_kind(), level;
    uint32_t row, i, j;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    if (index == 5) k = 180;
    shape_helper_world(index, k);
    row = helper_d + 0x110u * k;
    level = level_of(k);
    poke16(helper_g + 32u * k + 0x16C2u,
           (uint16_t)((uint32_t[]){0u, 0xFFFFu, rnd(), rnd() & rnd()}[rnd() % 4u]));
    for (i = 0; i < 12; i++)
        poke8(row + 0xF0u + i, (uint8_t)((uint32_t[]){level, level + 1u, level - 1u, 0u,
                                                      0xFFu, rnd()}[rnd() % 6u]));
    switch (index) {
    case 0: poke8(row + 0xF0u, 0xFF); break;           /* empty list: 0 */
    case 1: /* 12 entries above the level, no terminator: 0, a2 = 0x8 */
        if (level == 0xFFu) poke_level(k, level = 0xFE);
        for (i = 0; i < 12; i++) poke8(row + 0xF0u + i, (uint8_t)(level + 1u));
        break;
    case 2: /* entry 0 == level, bit clear: returns 1 */
        poke8(row + 0xF0u, level);
        poke16(helper_g + 32u * k + 0x16C2u, 0x7FFFu);
        break;
    case 3: /* first four bits already set: returns 5 */
        for (i = 0; i < 6; i++) poke8(row + 0xF0u + i, 0);
        poke16(helper_g + 32u * k + 0x16C2u, 0xF000u);
        break;
    case 4: /* last entry only: returns 12 */
        if (level == 0xFFu) poke_level(k, level = 0xFE);
        for (i = 0; i < 11; i++) poke8(row + 0xF0u + i, (uint8_t)(level + 1u));
        poke8(row + 0xFBu, level);
        poke16(helper_g + 32u * k + 0x16C2u, 0);
        break;
    default:
        break;
    }
    args[0] = dirty_byte_arg(k);
}

/* func_801E3E14(kind a0): list D[0xD0 + i] (0xFF ends it, at most 9),
 * entry e tests bit (e-1)&31 of G + 32k + 0x16C0, sets bit (i+3) of
 * G + 32k + 0x16C4. */
static void shape_801E3E14(unsigned index, uint32_t args[4])
{
    uint8_t k = shape_kind();
    uint32_t row, i, j;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    shape_helper_world(index, k);
    row = helper_d + 0x110u * k;
    poke16(helper_g + 32u * k + 0x16C0u,
           (uint16_t)((uint32_t[]){0u, 0xFFFFu, rnd(), rnd() & rnd()}[rnd() % 4u]));
    poke16(helper_g + 32u * k + 0x16C4u, (uint16_t)rnd());
    for (i = 0; i < 9; i++)
        poke8(row + 0xD0u + i, (uint8_t)((uint32_t[]){0u, 1u, 16u, 17u, 32u, 33u, 0xFFu,
                                                      rnd(), 1u + rnd() % 16u}[rnd() % 9u]));
    switch (index) {
    case 0: poke8(row + 0xD0u, 0xFF); break;       /* empty: returns 0xFE */
    case 1: /* 9 entries, no terminator, every bit set */
        poke16(helper_g + 32u * k + 0x16C0u, 0xFFFFu);
        poke16(helper_g + 32u * k + 0x16C4u, 0);
        for (i = 0; i < 9; i++) poke8(row + 0xD0u + i, (uint8_t)(1u + i));
        break;
    case 2: /* e = 0 (srav by 31 tests bit 0), e = 33 (shift 0 tests 0x8000) */
        poke16(helper_g + 32u * k + 0x16C0u, 0x8001u);
        poke8(row + 0xD0u, 0);
        poke8(row + 0xD1u, 33);
        poke8(row + 0xD2u, 0xFF);
        break;
    case 3: /* e = 16 and 17: bit 0 and shifted out */
        poke16(helper_g + 32u * k + 0x16C0u, 0xFFFFu);
        poke8(row + 0xD0u, 16);
        poke8(row + 0xD1u, 17);
        break;
    default:
        break;
    }
    args[0] = dirty_byte_arg(k);
}

/* func_801E3EA4(): row 0 list D[0x950 + i] (0xFF ends it, at most 9) against
 * the level byte 0x8006DE22, bit 0x1000>>i of G + 0x17C4.  a0 is not an
 * input. */
static void shape_801E3EA4(unsigned index, uint32_t args[4])
{
    uint32_t i, j;
    uint8_t level;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    shape_helper_world(index, 8);
    level = (uint8_t)((uint32_t[]){0u, 1u, 40u, 99u, 255u, rnd()}[rnd() % 6u]);
    poke8(0x8006DE22u, level);
    poke16(helper_g + 0x17C4u,
           (uint16_t)((uint32_t[]){0u, 0xFFFFu, rnd(), 0x1FF0u}[rnd() % 4u]));
    for (i = 0; i < 9; i++)
        poke8(helper_d + 0x950u + i, (uint8_t)((uint32_t[]){level, level + 1u, level - 1u,
                                                            0u, 0xFFu, rnd()}[rnd() % 6u]));
    switch (index) {
    case 0: poke8(helper_d + 0x950u, 0xFF); break; /* returns D + 0 */
    case 1: /* nine entries at the level, mask clear: sets 0x1000..0x10 */
        for (i = 0; i < 9; i++) poke8(helper_d + 0x950u + i, level);
        poke16(helper_g + 0x17C4u, 0);
        break;
    case 2: /* entries one above the level: nothing (a0 not written) */
        if (level == 0xFFu) poke8(0x8006DE22u, level = 0xFE);
        for (i = 0; i < 9; i++) poke8(helper_d + 0x950u + i, (uint8_t)(level + 1u));
        break;
    default:
        break;
    }
}

/* func_801E3F28(kind a0): list D[0xE0 + i] (0 ends it, at most 13), entry e
 * tests bit (e-1)&31 of G + 32k + 0x16C2, sets bit i of G + 32k + 0x16C6. */
static void shape_801E3F28(unsigned index, uint32_t args[4])
{
    uint8_t k = shape_kind();
    uint32_t row, i, j;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    shape_helper_world(index, k);
    row = helper_d + 0x110u * k;
    poke16(helper_g + 32u * k + 0x16C2u,
           (uint16_t)((uint32_t[]){0u, 0xFFFFu, rnd(), rnd() & rnd()}[rnd() % 4u]));
    poke16(helper_g + 32u * k + 0x16C6u, (uint16_t)rnd());
    for (i = 0; i < 13; i++)
        poke8(row + 0xE0u + i, (uint8_t)((uint32_t[]){0u, 1u, 16u, 17u, 33u, 0xFFu,
                                                      rnd(), 1u + rnd() % 16u}[rnd() % 8u]));
    switch (index) {
    case 0: poke8(row + 0xE0u, 0); break;          /* empty: returns -1 */
    case 1: /* 13 entries, no terminator, every bit set */
        poke16(helper_g + 32u * k + 0x16C2u, 0xFFFFu);
        poke16(helper_g + 32u * k + 0x16C6u, 0);
        for (i = 0; i < 13; i++) poke8(row + 0xE0u + i, (uint8_t)(1u + i));
        break;
    case 2: /* e = 0xFF (shift 30) and 33 (shift 0) */
        poke16(helper_g + 32u * k + 0x16C2u, 0x8000u);
        poke8(row + 0xE0u, 0xFF);
        poke8(row + 0xE1u, 33);
        poke8(row + 0xE2u, 0);
        break;
    default:
        break;
    }
    args[0] = dirty_byte_arg(k);
}

/* func_801E3FB0(): E = word 0x801E44EC (+0x4E u16, +0x58 byte), G = word
 * 0x801E44C4 (+0xE30, +0xE58, +0xE64, +0xE66).  The alias case puts E+0x4E
 * on G+0xE64, so the re-read before the 0xE66 store sees the 0xE64 store. */
static void shape_801E3FB0(unsigned index, uint32_t args[4])
{
    uint32_t j;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    shape_helper_world(index, 7);
    poke16(helper_e + 0x4Eu, (uint16_t)((uint32_t[]){0u, 1u, 999u, 6553u, 6554u,
                                                     0xFFFFu, rnd()}[rnd() % 7u]));
    poke8(helper_e + 0x58u, (uint8_t)((uint32_t[]){0u, 4u, 5u, 254u, 255u,
                                                   rnd()}[rnd() % 6u]));
    switch (index) {
    case 0: poke16(helper_e + 0x4Eu, 0xFFFFu); poke8(helper_e + 0x58u, 255); break;
    case 1: poke16(helper_e + 0x4Eu, 0); poke8(helper_e + 0x58u, 0); break;
    case 2: /* E + 0x4E aliases G + 0xE64 */
        helper_e = helper_g + 0xE16u;
        poke32(0x801E44ECu, helper_e);
        break;
    case 3: /* E + 0x58 aliases G + 0xE58's low byte (stored first) */
        helper_e = helper_g + 0xE00u;
        poke32(0x801E44ECu, helper_e);
        break;
    default:
        break;
    }
}

/* Defined with --compose-3a18 below; also the leaf shaper of 0x801E3A18. */
static void shape_compose_3A18(unsigned index, uint32_t args[4]);

typedef struct LeafSpec {
    uint32_t address;
    void (*shape)(unsigned index, uint32_t args[4]);
    unsigned edge_cases;
} LeafSpec;

static const LeafSpec kSpecs[] = {
    {0x801E24B0u, shape_801E24B0, 7},
    {0x801DF910u, shape_801DF910, 8},
    {0x801DFA38u, shape_801DFA38, 5},
    {0x801E1590u, shape_801E1590, 8},
    {0x801E2888u, shape_801E2888, 8},
    {0x801E403Cu, shape_801E403C, 6},
    {0x801E41B4u, shape_801E41B4, 5},
    {0x801E1370u, shape_801E1370, 8},
    {0x801E2EB0u, shape_801E2EB0, 8},
    {0x801E3BE0u, shape_801E3BE0, 7},
    {0x801E3D54u, shape_801E3D54, 6},
    {0x801E3E14u, shape_801E3E14, 4},
    {0x801E3EA4u, shape_801E3EA4, 3},
    {0x801E3F28u, shape_801E3F28, 3},
    {0x801E3FB0u, shape_801E3FB0, 4},
    {0x801E3A18u, shape_compose_3A18, 11},
};

/* ---- running one side --------------------------------------------------- */

typedef struct SideResult {
    int rc;
    uint32_t v0;
    uint32_t saved[11]; /* s0-s7, gp, sp, fp */
    uint32_t gpr[32];
    uint32_t hi;
    uint32_t lo;
    uint64_t steps;
    uint64_t native_calls;
    unsigned accessor_calls;
    unsigned placeholder_hits;
} SideResult;

static sigjmp_buf fault_jmp;
static void fault_handler(int sig)
{
    (void)sig;
    siglongjmp(fault_jmp, 1);
}

static void prepare_cpu(const uint32_t args[4], uint32_t saved_seed)
{
    unsigned i;
    memset(&cpu, 0, sizeof(cpu));
    initialize_cpu(&cpu, &rt);
    cpu.bus.write = recording_write;
    for (i = 0; i < 4; i++)
        cpu.gpr[4 + i] = args[i];
    for (i = 16; i <= 23; i++)
        cpu.gpr[i] = saved_seed * (i + 1u);
    /* Scratch registers too: an exit set that misses a register the retail
     * bytes change, or clobbers one they keep, shows up as a difference. */
    for (i = 1; i <= 25; i++) {
        if (i == 2 || (i >= 4 && i <= 7) || (i >= 16 && i <= 23))
            continue;
        cpu.gpr[i] = (saved_seed ^ 0xC3A5u) * (i + 7u);
    }
    cpu.gpr[28] = 0x800C0000u + (saved_seed & 0xFFF0u);
    cpu.gpr[29] = case_sp;
    cpu.gpr[30] = saved_seed ^ 0x5A5A5A5Au;
    /* HI/LO as well: a body must reproduce a MULT/DIV's HI:LO and leave them
     * alone when its retail bytes do not touch them. */
    cpu.hi = saved_seed * 0x9E3779B1u;
    cpu.lo = ~saved_seed ^ 0x0F1E2D3Cu;
}

static int run_side(uint32_t target, const uint32_t args[4], int force,
                    uint32_t saved_seed, SideResult *out)
{
    unsigned i;
    uint64_t calls = rt.results.native_calls;
    unsigned accessed = accessor_calls;

    prepare_cpu(args, saved_seed);
    g_prover_placeholder_hits = 0;
    rt.force_interpret = force;
    if (sigsetjmp(fault_jmp, 1) != 0) {
        rt.force_interpret = 0;
        return -1;
    }
    out->rc = PcPortMipsRun(&cpu, target, BATTLE_HALT_PC, STEP_LIMIT);
    rt.force_interpret = 0;
    out->v0 = cpu.gpr[2];
    for (i = 0; i < 8; i++)
        out->saved[i] = cpu.gpr[16 + i];
    out->saved[8] = cpu.gpr[28];
    out->saved[9] = cpu.gpr[29];
    out->saved[10] = cpu.gpr[30];
    memcpy(out->gpr, cpu.gpr, sizeof(out->gpr));
    out->hi = cpu.hi;
    out->lo = cpu.lo;
    out->steps = cpu.steps;
    out->native_calls = rt.results.native_calls - calls;
    out->accessor_calls = accessor_calls - accessed;
    out->placeholder_hits = g_prover_placeholder_hits;
    return 0;
}

static void count_diffs(const uint8_t *a, const uint8_t *b, size_t n,
                        size_t skip_lo, size_t skip_hi, size_t *diffs,
                        size_t *first)
{
    size_t i;
    *diffs = 0;
    for (i = 0; i < n; i++) {
        if (i >= skip_lo && i < skip_hi)
            continue;
        if (a[i] != b[i]) {
            if (*diffs == 0)
                *first = i;
            (*diffs)++;
        }
    }
}

static unsigned compare_case(const char *name, uint32_t target,
                             const uint32_t args[4], uint32_t seed)
{
    SideResult retail, native;
    size_t diffs, first = 0;
    char detail[256];

    memcpy(before_ram, g_PsxRam, sizeof(before_ram));
    memcpy(before_gs, native_gs, RESULTS_GS_BYTES);
    memcpy(before_pad, g_PsxScratchpad, sizeof(before_pad));
    stray_writes = 0;

    if (run_side(target, args, 1, seed, &retail) != 0) {
        fail(name, "retail-fault", "the interpreted side faulted natively");
        return 0;
    }
    if (retail.rc != PC_PORT_MIPS_HALTED) {
        snprintf(detail, sizeof(detail), "rc=%d err=%s seed=%08x", retail.rc,
                 cpu.error, seed);
        fail(name, "retail-incomplete", detail);
        return 0;
    }
    if (retail.native_calls != 0 || retail.steps == 0) {
        fail(name, "retail-not-interpreted", "force_interpret did not interpret");
        return 0;
    }
    memcpy(retail_ram, g_PsxRam, sizeof(retail_ram));
    memcpy(retail_gs, native_gs, RESULTS_GS_BYTES);
    memcpy(retail_pad, g_PsxScratchpad, sizeof(retail_pad));

    memcpy(g_PsxRam, before_ram, sizeof(before_ram));
    memcpy(native_gs, before_gs, RESULTS_GS_BYTES);
    memcpy(g_PsxScratchpad, before_pad, sizeof(before_pad));

    if (run_side(target, args, 0, seed, &native) != 0) {
        fail(name, "native-fault", "native body faulted where retail completed");
        return 0;
    }
    if (native.rc != PC_PORT_MIPS_HALTED) {
        snprintf(detail, sizeof(detail), "rc=%d err=%s", native.rc, cpu.error);
        fail(name, "native-incomplete", detail);
        return 0;
    }
    if (native.native_calls != 1 || native.steps != 0) {
        snprintf(detail, sizeof(detail),
                 "native_calls=%llu guest_steps=%llu (identity/allowlist/bind refused?)",
                 (unsigned long long)native.native_calls,
                 (unsigned long long)native.steps);
        fail(name, "not-adopted", detail);
        return 0;
    }
    if (retail.placeholder_hits != 0 || native.placeholder_hits != 0) {
        fail(name, "placeholder", "a side reached a bridge placeholder callee");
        return 0;
    }
    if (retail.v0 != native.v0) {
        snprintf(detail, sizeof(detail), "v0 retail=%08x native=%08x seed=%08x",
                 retail.v0, native.v0, seed);
        fail(name, "return", detail);
        return 0;
    }
    if (memcmp(retail.saved, native.saved, sizeof(retail.saved)) != 0) {
        fail(name, "callee-saved", "s0-s7/gp/sp/fp differ");
        return 0;
    }
    {
        unsigned reg;
        for (reg = 1; reg < 32; reg++) {
            if (retail.gpr[reg] != native.gpr[reg]) {
                snprintf(detail, sizeof(detail),
                         "r%u retail=%08x native=%08x seed=%08x", reg,
                         retail.gpr[reg], native.gpr[reg], seed);
                fail(name, "exit-registers", detail);
                return 0;
            }
        }
    }
    if (retail.hi != native.hi || retail.lo != native.lo) {
        snprintf(detail, sizeof(detail),
                 "hi retail=%08x native=%08x lo retail=%08x native=%08x seed=%08x",
                 retail.hi, native.hi, retail.lo, native.lo, seed);
        fail(name, "hilo", detail);
        return 0;
    }
    count_diffs(retail_ram, g_PsxRam, PSX_RAM_SIZE, FRAME_SKIP_LO,
                FRAME_SKIP_HI, &diffs, &first);
    if (diffs != 0) {
        snprintf(detail, sizeof(detail),
                 "%zu bytes differ, first at guest %08x retail=%02x native=%02x seed=%08x",
                 diffs, (unsigned)(0x80000000u | first), retail_ram[first],
                 g_PsxRam[first], seed);
        fail(name, "ram", detail);
        return 0;
    }
    count_diffs(retail_gs, native_gs, RESULTS_GS_BYTES, 0, 0, &diffs, &first);
    if (diffs != 0) {
        snprintf(detail, sizeof(detail),
                 "%zu bytes differ, first at g_GameState+%04zx (guest %08x) "
                 "retail=%02x native=%02x seed=%08x",
                 diffs, first, (unsigned)(0x8006D634u + first), retail_gs[first],
                 native_gs[first], seed);
        fail(name, "native-copy", detail);
        return 0;
    }
    count_diffs(retail_pad, g_PsxScratchpad, sizeof(retail_pad), 0, 0, &diffs,
                &first);
    if (diffs != 0) {
        fail(name, "scratchpad", "scratchpad differs");
        return 0;
    }
    if (stray_writes != 0) {
        snprintf(detail, sizeof(detail),
                 "%u stores outside the compared windows, first host %p",
                 stray_writes, (void *)stray_first);
        fail(name, "uncompared-write", detail);
        return 0;
    }
    if (native.accessor_calls == 0) {
        fail(name, "accessor", "native body never used PcPort_BattleMipsResolve");
        return 0;
    }
    checks++;
    return 1;
}

/* ---- gate controls ------------------------------------------------------ */

/* The gate controls dispatch for real, so an adopted body runs each time it
 * is allowed to: its inputs (pointers in the mutable module words, argument
 * buffers) are re-shaped before every dispatch so that it runs on valid guest
 * state even after a control reloaded the module image. */
static const LeafSpec *gate_spec;
static uint32_t gate_args[4];

static void arm_gate(void)
{
    gate_spec->shape(gate_spec->edge_cases, gate_args);
    prepare_cpu(gate_args, 1);
}

/* 1 native, 0 interpret, as production's bridge decides for this PC. */
static int dispatch(uint32_t target)
{
    arm_gate();
    rt.bridge_cpu = &cpu;
    return runtime_bridge(&rt, &cpu, target);
}

#define CONTROL(label, condition) do { \
    controls++; \
    if (!(condition)) { \
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL control %s (line %d)\n", \
                label, __LINE__); \
        failures++; \
    } \
} while (0)

static void accessor_controls(void)
{
    ResultsCall call;
    volatile int stopped = 0;
    pid_t child;
    int status = 0;

    /* Inside an adopted call: unaligned and unmapped accesses unwind. */
    call.previous = NULL;
    results_call = &call;
    if (setjmp(call.failure) == 0)
        (void)PcPort_BattleMipsResolve(0x800D2FE5u, 2, 0);
    else
        stopped = 1;
    CONTROL("accessor unaligned stops the call", stopped);
    stopped = 0;
    if (setjmp(call.failure) == 0)
        (void)PcPort_BattleMipsResolve(0x00001000u, 4, 1);
    else
        stopped = 1;
    CONTROL("accessor unmapped stops the call", stopped);
    results_call = NULL;

    /* Outside one it aborts rather than hand back NULL. */
    child = fork();
    if (child == 0) {
        (void)PcPort_BattleMipsResolve(0x800D2FE6u, 4, 0);
        _exit(0);
    }
    CONTROL("accessor outside a call aborts",
            child > 0 && waitpid(child, &status, 0) == child &&
            WIFSIGNALED(status) && WTERMSIG(status) == SIGABRT);

    /* Exactly the interpreter's mapping, including the native g_GameState. */
    CONTROL("accessor maps g_GameState natively",
            PcPort_BattleMipsResolve(0x8006F65Au, 1, 0) == native_gs + 0x2026u &&
            PcPort_BattleMipsResolve(0x8006F5C4u, 1, 1) == native_gs + 0x1F90u);
    CONTROL("accessor maps battle BSS to guest RAM",
            PcPort_BattleMipsResolve(0x800D2FE4u, 1, 0) == g_PsxRam + 0xD2FE4u);
    CONTROL("accessor canonicalizes KSEG1",
            PcPort_BattleMipsResolve(0xA006F65Au, 1, 0) == native_gs + 0x2026u);
}

static void gate_controls(const LeafSpec *spec)
{
    uint32_t target = spec->address;
    static const uint32_t probes[] = {0x0000u, 0x44B0u, RESULTS_IMMUTABLE_BYTES - 1u};
    uint64_t generation;
    unsigned k;
    uint8_t saved;

    gate_spec = spec;
    begin_case(0xC0FFEEu);
    CONTROL("retail module verifies", results_identity_current(&rt, target));
    CONTROL("baseline native", dispatch(target) == 1);

    for (k = 0; k < sizeof(probes) / sizeof(probes[0]); k++) {
        uint8_t *byte = g_PsxRam + MODULE_OFFSET + probes[k];
        int probed = probes[k] < RESULTS_REJECT_PROBE ||
                     (probes[k] & ~(RESULTS_REJECT_PROBE - 1u)) ==
                         ((target - MODULE_ADDR) & ~(RESULTS_REJECT_PROBE - 1u));
        *byte ^= 0x01u; /* unobserved: no hook sees this */
        CONTROL("corrupt immutable byte refuses native", dispatch(target) == 0);
        *byte ^= 0x01u;
        if (probed) {
            CONTROL("restored module adopts again", dispatch(target) == 1);
        } else {
            /* A rejection is re-examined only when its probe windows change
             * or a hook invalidates it (battle_results_gate.inc): an
             * unobserved repair elsewhere keeps interpreting -- fail closed. */
            CONTROL("unobserved far repair stays interpreted", dispatch(target) == 0);
            results_note_archive(&rt, 0x80029690u);
            CONTROL("invalidated repair adopts again", dispatch(target) == 1);
        }
    }

    generation = rt.results.generation;
    saved = g_PsxRam[MODULE_OFFSET + 0x44B4u];
    CONTROL("observed store", runtime_write(&rt, 0x801E24B4u, 1, saved) == 0);
    CONTROL("observed store invalidates",
            rt.results.state == RESULTS_UNKNOWN && rt.results.generation != generation);
    CONTROL("identical bytes re-verify", dispatch(target) == 1);

    CONTROL("KSEG1 alias store", runtime_write(&rt, 0xA01E24B4u, 1, saved ^ 0x80u) == 0);
    CONTROL("KSEG1 alias store invalidates", rt.results.state == RESULTS_UNKNOWN);
    CONTROL("KSEG1-corrupted module refuses native", dispatch(target) == 0);
    CONTROL("KSEG1 restore", runtime_write(&rt, 0xA01E24B4u, 1, saved) == 0);
    CONTROL("KSEG1-restored module adopts", dispatch(target) == 1);

    generation = rt.results.generation;
    CONTROL("mutable flag store", runtime_write(&rt, 0x801E44C0u, 1, 1) == 0);
    CONTROL("mutable word store", runtime_write(&rt, 0x801E44FCu, 4, 0x12345678u) == 0);
    CONTROL("mutable data keeps the verdict",
            rt.results.state == RESULTS_VERIFIED && rt.results.generation == generation);
    CONTROL("mutable data still adopts", dispatch(target) == 1);

    results_note_archive(&rt, 0x800295d8u);
    CONTROL("archive read invalidates", rt.results.state == RESULTS_UNKNOWN);
    CONTROL("after archive read re-verifies", dispatch(target) == 1);

    memset(g_PsxRam + MODULE_OFFSET, 0, MODULE_BYTES);
    CONTROL("empty range refuses native", dispatch(target) == 0);
    memcpy(g_PsxRam + MODULE_OFFSET, module_image + 0x2000u, MODULE_BYTES - 0x2000u);
    CONTROL("other code resident refuses native", dispatch(target) == 0);
    load_module_image();
    CONTROL("reloaded module adopts", dispatch(target) == 1);

    arm_gate();
    cpu.load_valid = 1;
    cpu.load_reg = 4;
    rt.bridge_cpu = &cpu;
    CONTROL("pending delay-slot load interprets", runtime_bridge(&rt, &cpu, target) == 0);
    arm_gate();
    cpu.load_valid = 1;
    cpu.load_reg = 0;
    CONTROL("load into $zero does not block", runtime_bridge(&rt, &cpu, target) == 1);

    rt.force_interpret = 1;
    CONTROL("force_interpret interprets", dispatch(target) == 0);
    rt.force_interpret = 0;

    CONTROL("RESULTS-only control set", setenv("XENO_BATTLE_RESULTS_FORCE_INTERPRET", "1", 1) == 0);
    CONTROL("RESULTS-only control interprets", dispatch(target) == 0);
    CONTROL("RESULTS-only control does not force all bridges", rt.force_interpret == 0);
    CONTROL("RESULTS-only control clear", unsetenv("XENO_BATTLE_RESULTS_FORCE_INTERPRET") == 0);
    CONTROL("RESULTS adopts after control clears", dispatch(target) == 1);

    g_ActiveBattleRuntime = NULL;
    CONTROL("no active runtime interprets", dispatch(target) == 0);
    g_ActiveBattleRuntime = &rt;

    CONTROL("non-allowlisted results entry interprets",
            results_try_host(&rt, &cpu, 0x801E252Cu) == 0);
    CONTROL("still native afterwards", dispatch(target) == 1);
}

/* ---- CPU-fallback meter (XENO_CPU_FALLBACK_STATS) ------------------------
 *
 * --meter (run with XENO_CPU_FALLBACK_STATS=1): two metered "battles" over
 * one interpreted call of 0x801E3A18, whose three slots are shaped as kind 7
 * so each slot jal's func_801E3D54, func_801E3F28 and func_801E3FB0:
 *   retail  (force_interpret) 10 interpreted calls -- the entry plus 9 JALs
 *           the heuristic must recognise -- and exactly cpu.steps
 *           interpreted instructions, no native call;
 *   native  the same, with the adopted helpers taken natively: 1 interpreted
 *           call and 3 native results calls per allowlisted helper.
 * The normal run checks that the meter counts nothing outside a metered
 * battle, whatever the environment says. */
static int meter_expect(const char *what, unsigned long long got,
                        unsigned long long want)
{
    if (got == want)
        return 1;
    fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL meter %s got=%llu want=%llu\n",
            what, got, want);
    failures++;
    return 0;
}

static void shape_meter_3A18(uint32_t args[4])
{
    uint32_t slots, k, j;

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    shape_helper_world(0, 7);
    slots = guest_buffer(0x1100u); /* 3A18 stores results at +0x101C.. */
    poke32(0x801E44C8u, slots);
    for (k = 0; k < 3; k++) {
        poke16(slots + 0x170u * k + 0x7Cu, 0);   /* not dead */
        poke8(slots + 0x170u * k + 0x56u, 7);    /* kind 7 */
    }
}

static int meter_run(void)
{
    static const uint32_t helpers[3] = {0x801E3D54u, 0x801E3F28u, 0x801E3FB0u};
    const CpuFallbackMeter *m = &g_CpuFallbackMeter;
    uint32_t args[4], seed = 0x3A18u;
    unsigned i, s, adopted = 0;
    SideResult side;

    if (!cpu_meter_enabled()) {
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL meter: run --meter with "
                "XENO_CPU_FALLBACK_STATS=1\n");
        return 1;
    }
    unsigned caller_native = 0;
    for (i = 0; kResultsLeaves[i].name != NULL; i++) {
        for (s = 0; s < 3; s++)
            adopted += kResultsLeaves[i].address == helpers[s];
        caller_native |= kResultsLeaves[i].address == 0x801E3A18u;
    }
    for (s = 0; s < 2; s++) {
        int force = s == 0;
        begin_case(seed);
        shape_meter_3A18(args);
        cpu_meter_begin_battle();
        cpu_meter_mark_entry(&cpu, 0x801E3A18u);
        if (run_side(0x801E3A18u, args, force, seed, &side) != 0 ||
            side.rc != PC_PORT_MIPS_HALTED) {
            fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL meter run %s: %s\n",
                    force ? "retail" : "native", cpu.error);
            failures++;
            continue;
        }
        meter_expect("interpreted_instructions", m->interpreted_instructions,
                     side.steps);
        /* With 0x801E3A18 itself adopted the whole call is one native
         * results call (its helper calls are direct C calls inside it). */
        meter_expect("interpreted_calls", m->interpreted_calls,
                     force ? 10u : caller_native ? 0u : 10u - 3u * adopted);
        meter_expect("native_results_calls", m->native_results_calls,
                     force ? 0u : caller_native ? 1u : 3u * adopted);
        meter_expect("results bodies run", side.native_calls,
                     force ? 0u : caller_native ? 1u : 3u * adopted);
        meter_expect("native_main_exe_calls", m->native_main_exe_calls, 0);
        meter_expect("native_overlay_calls", m->native_overlay_calls, 0);
        meter_expect("native_file1_calls", m->native_file1_calls, 0);
        meter_expect("interpreted_targets", m->distinct_targets,
                     force ? 4u : caller_native ? 0u : 1u + 3u - adopted);
        cpu_meter_end_battle("returned");
        meter_expect("inactive after battle end", (unsigned long long)m->active, 0);
    }
    if (failures != 0)
        return 1;
    printf("BATTLE RESULTS DIFFERENTIAL METER PASS adopted_helpers=%u caller_native=%u\n",
           adopted, caller_native);
    return 0;
}

/* ---- --compose-3a18: model of 0x801E3A18 over the adopted helpers -------
 *
 * Retail side: 0x801E3A18 and its helpers fully interpreted (force).
 * Composed side: zero interpreted instructions.  A register-exact C model of
 * 0x801E3A18 (the one proven against the retail bytes under scripted helpers
 * by run_results_3a18_contract_test.sh) writes its guest frame and calls the
 * six adopted native helpers through production results_try_host, which
 * applies their exit-register sets.  Everything compare_case compares must
 * match, and the guest frame too (no FRAME_SKIP here).  This tests the
 * composition only; 0x801E3A18 is NOT on the allowlist and nothing here
 * dispatches it.  XENO_COMPOSE_3A18_MUTANT=drop-exit-regs keeps the caller's
 * temporaries across each helper and must fail. */
static unsigned compose_cover[6], compose_dead, compose_failed_call;
static int compose_drop_exit;

static uint32_t c_rd(uint32_t address, unsigned width)
{
    uint8_t *p = resolve_memory(&rt, address, width, 0);
    uint32_t v = 0;
    unsigned i;
    if (p == NULL)
        siglongjmp(fault_jmp, 1);
    for (i = 0; i < width; i++)
        v |= (uint32_t)p[i] << (8 * i);
    return v;
}

static void c_wr(uint32_t address, unsigned width, uint32_t value)
{
    uint8_t *p = resolve_memory(&rt, address, width, 1);
    unsigned i;
    if (p == NULL)
        siglongjmp(fault_jmp, 1);
    for (i = 0; i < width; i++)
        p[i] = (uint8_t)(value >> (8 * i));
    note_write(p, width);
}

static void c_call(uint32_t *r, uint32_t target, uint32_t return_address)
{
    static const uint32_t helpers[6] = {0x801E3BE0u, 0x801E3D54u, 0x801E3E14u,
                                        0x801E3F28u, 0x801E3EA4u, 0x801E3FB0u};
    uint32_t keep[32];
    unsigned i;

    r[31] = return_address;
    memcpy(keep, r, sizeof(keep));
    rt.bridge_cpu = &cpu;
    if (results_try_host(&rt, &cpu, target) != 1)
        compose_failed_call++;
    if (compose_drop_exit) {
        for (i = 1; i < 32; i++)
            if (i != 2 && !(i >= 16 && i <= 23) && i != 29 && i != 30 && i != 31)
                r[i] = keep[i];
    }
    for (i = 0; i < 6; i++)
        compose_cover[i] += helpers[i] == target;
}

static void compose_3A18(void)
{
    uint32_t *r = cpu.gpr;
    uint32_t sp = r[29] = r[29] - 32u;

    c_wr(sp + 20u, 4, r[17]);
    c_wr(sp + 24u, 4, r[18]);
    c_wr(sp + 28u, 4, r[31]);
    c_wr(sp + 16u, 4, r[16]);
    r[17] = 0;
    r[18] = 10;
    r[16] = r[17] & 0xFFu;
    for (;;) {
        uint32_t s0 = r[16];

        r[3] = c_rd(0x801E44C8u, 4);
        r[2] = ((s0 * 3u) * 8u - s0) << 4;
        r[4] = r[2] + r[3];
        r[2] = c_rd(r[4] + 0x7Cu, 2);
        r[1] = 0x801E0000u;
        c_wr(0x801E44ECu, 4, r[4]);
        r[2] &= 0x8000u;
        if (r[2] != 0) {
            compose_dead++;
            goto next_slot;
        }
        r[4] = c_rd(r[4] + 0x56u, 1);
        r[2] = (int32_t)r[4] < 9;
        if (r[2] == 0) {
            r[2] = (int32_t)r[4] < 7;
        } else {
            r[2] = (int32_t)r[4] < 7;
            if (r[2] == 0)
                goto after_3BE0;
        }
        c_call(r, 0x801E3BE0u, 0x801E3A94u);
        r[4] = r[2];
        r[2] = r[4] & 0xFFu;
        if (r[2] != 0) {
            r[2] = c_rd(0x801E44C8u, 4) + r[16];
            c_wr(r[2] + 0x101Cu, 1, r[4]);
        }
after_3BE0:
        r[2] = c_rd(0x801E44ECu, 4);
        r[4] = c_rd(r[2] + 0x56u, 1);
        if (r[4] == r[18]) {
            r[2] = (int32_t)r[4] < 7;
            goto check_3E14;
        }
        r[2] = (int32_t)r[4] < 7;
        c_call(r, 0x801E3D54u, 0x801E3ADCu);
        r[4] = r[2];
        r[2] = r[4] & 0xFFu;
        r[3] = r[17] & 0xFFu;
        if (r[2] != 0) {
            r[2] = c_rd(0x801E44C8u, 4) + r[3];
            c_wr(r[2] + 0x101Fu, 1, r[4]);
        }
        r[2] = c_rd(0x801E44ECu, 4);
        r[4] = c_rd(r[2] + 0x56u, 1);
        r[2] = (int32_t)r[4] < 7;
check_3E14:
        if (r[2] != 0) {
            r[2] = (int32_t)r[4] < 9;
        } else {
            r[2] = (int32_t)r[4] < 9;
            if (r[2] != 0 || r[4] == r[18])
                goto check_3F28;
        }
        c_call(r, 0x801E3E14u, 0x801E3B38u);
check_3F28:
        r[2] = c_rd(0x801E44ECu, 4);
        r[4] = c_rd(r[2] + 0x56u, 1);
        r[2] = (int32_t)r[4] < 11;
        if (r[2] == 0) {
            r[2] = (int32_t)r[4] < 8;
        } else {
            r[2] = (int32_t)r[4] < 8;
            if (r[2] == 0)
                goto check_3EA4;
        }
        c_call(r, 0x801E3F28u, 0x801E3B68u);
check_3EA4:
        r[2] = c_rd(0x801E44ECu, 4);
        r[3] = c_rd(r[2] + 0x56u, 1);
        r[2] = 8;
        if (r[3] == r[2])
            c_call(r, 0x801E3EA4u, 0x801E3B8Cu);
        r[2] = c_rd(0x801E44ECu, 4);
        r[3] = c_rd(r[2] + 0x56u, 1);
        r[2] = 7;
        if (r[3] == r[2])
            c_call(r, 0x801E3FB0u, 0x801E3BB0u);
next_slot:
        r[17] += 1;
        r[2] = r[17] & 0xFFu;
        r[2] = r[2] < 3u;
        r[16] = r[17] & 0xFFu;
        if (r[2] == 0)
            break;
    }
    r[31] = c_rd(sp + 28u, 4);
    r[18] = c_rd(sp + 24u, 4);
    r[17] = c_rd(sp + 20u, 4);
    r[16] = c_rd(sp + 16u, 4);
    r[29] = sp + 32u;
}

static void shape_compose_3A18(unsigned index, uint32_t args[4])
{
    uint32_t slots, k, j;
    uint8_t kinds[3];

    for (j = 0; j < 4; j++)
        args[j] = rnd();
    for (k = 0; k < 3; k++)
        kinds[k] = (rnd() & 1u) ? (uint8_t)(rnd() % 13u) : shape_kind();
    shape_helper_world(index, kinds[0]);
    for (k = 1; k < 3; k++)
        poke_level(kinds[k], (uint8_t)rnd());
    slots = guest_buffer(0x1100u);
    poke32(0x801E44C8u, slots);
    for (k = 0; k < 3; k++) {
        poke16(slots + 0x170u * k + 0x7Cu,
               (uint16_t)((rnd() % 4u) ? (rnd() & 0x7FFFu) : (0x8000u | rnd())));
        poke8(slots + 0x170u * k + 0x56u, kinds[k]);
    }
}

static int compose_run(unsigned cases)
{
    const char *mutant = getenv("XENO_COMPOSE_3A18_MUTANT");
    volatile unsigned c, proven = 0;
    unsigned i;
    char detail[256];

    compose_drop_exit = mutant != NULL && strcmp(mutant, "drop-exit-regs") == 0;
    for (c = 0; c < cases; c++) {
        uint32_t seed = 0x801E3A18u ^ (0x9E3779B9u * (c + 1u));
        uint32_t args[4];
        SideResult retail;
        size_t diffs, first = 0;
        uint64_t calls_before;
        unsigned failed_before = compose_failed_call, reg;

        begin_case(seed);
        shape_compose_3A18(c, args);
        memcpy(before_ram, g_PsxRam, sizeof(before_ram));
        memcpy(before_gs, native_gs, RESULTS_GS_BYTES);
        memcpy(before_pad, g_PsxScratchpad, sizeof(before_pad));
        stray_writes = 0;
        if (run_side(0x801E3A18u, args, 1, seed, &retail) != 0 ||
            retail.rc != PC_PORT_MIPS_HALTED || retail.native_calls != 0) {
            fail("compose-3a18", "retail", cpu.error);
            continue;
        }
        memcpy(retail_ram, g_PsxRam, sizeof(retail_ram));
        memcpy(retail_gs, native_gs, RESULTS_GS_BYTES);
        memcpy(retail_pad, g_PsxScratchpad, sizeof(retail_pad));
        memcpy(g_PsxRam, before_ram, sizeof(before_ram));
        memcpy(native_gs, before_gs, RESULTS_GS_BYTES);
        memcpy(g_PsxScratchpad, before_pad, sizeof(before_pad));

        prepare_cpu(args, seed);
        cpu.gpr[31] = BATTLE_HALT_PC;
        calls_before = rt.results.native_calls;
        if (sigsetjmp(fault_jmp, 1) != 0) {
            fail("compose-3a18", "composed-fault", "composed side faulted");
            continue;
        }
        compose_3A18();
        if (compose_failed_call != failed_before || cpu.steps != 0) {
            fail("compose-3a18", "not-native", "a helper was refused or something interpreted");
            continue;
        }
        (void)calls_before;
        for (reg = 1; reg < 32; reg++) {
            if (retail.gpr[reg] != cpu.gpr[reg]) {
                snprintf(detail, sizeof(detail), "r%u retail=%08x composed=%08x seed=%08x",
                         reg, retail.gpr[reg], cpu.gpr[reg], seed);
                fail("compose-3a18", "exit-registers", detail);
                goto next_case;
            }
        }
        if (retail.hi != cpu.hi || retail.lo != cpu.lo) {
            fail("compose-3a18", "hilo", "HI/LO differ");
            continue;
        }
        count_diffs(retail_ram, g_PsxRam, PSX_RAM_SIZE, 0, 0, &diffs, &first);
        if (diffs != 0) {
            snprintf(detail, sizeof(detail), "%zu bytes differ, first at guest %08x seed=%08x",
                     diffs, (unsigned)(0x80000000u | first), seed);
            fail("compose-3a18", "ram", detail);
            continue;
        }
        count_diffs(retail_gs, native_gs, RESULTS_GS_BYTES, 0, 0, &diffs, &first);
        if (diffs != 0) {
            fail("compose-3a18", "native-copy", "g_GameState copy differs");
            continue;
        }
        if (memcmp(retail_pad, g_PsxScratchpad, sizeof(retail_pad)) != 0 || stray_writes != 0) {
            fail("compose-3a18", "scratchpad/stray", "scratchpad or uncompared write");
            continue;
        }
        proven++;
next_case:
        ;
    }
    printf("BATTLE RESULTS COMPOSE 3A18 coverage: 3BE0=%u 3D54=%u 3E14=%u 3F28=%u "
           "3EA4=%u 3FB0=%u dead=%u\n", compose_cover[0], compose_cover[1],
           compose_cover[2], compose_cover[3], compose_cover[4], compose_cover[5],
           compose_dead);
    for (i = 0; i < 6; i++)
        if (compose_cover[i] == 0) {
            fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL compose helper %u never called\n", i);
            failures++;
        }
    if (failures != 0 || proven != cases) {
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL compose-3a18 proven=%u of %u\n",
                proven, cases);
        return 1;
    }
    printf("BATTLE RESULTS COMPOSE 3A18 PASS cases=%u: model + adopted native helpers, "
           "zero interpreted instructions, identical to fully interpreted retail "
           "(registers, HI/LO, RAM incl. guest frame, native copy, scratchpad)\n", cases);
    return 0;
}

/* ---- --replay-3a18 DIR: real call-site state, interpreter disabled -------
 *
 * DIR holds what XENO_RESULTS_CAPTURE_3A18 wrote at real 0x801E3A18 calls
 * (3a18-<n>-ram/gs/pad/regs.bin; retail-derived, never in the tree).  For
 * each capture: verify the captured module is the pinned retail identity,
 * run 0x801E3A18 fully interpreted (retail), then from the identical state
 * natively with XENO_DISABLE_CPU_INTERPRETER=1, so any interpreted
 * instruction would be refused.  Requires identical registers, HI/LO, all
 * RAM (frame included), the native g_GameState copy and the scratchpad,
 * zero interpreted instructions and one native results call, and prints the
 * side effects retail produced (changed ranges). */
static int replay_load(const char *dir, unsigned n, const char *what, void *dst,
                       size_t size, size_t *got)
{
    char path[512];
    FILE *f;
    size_t r;

    snprintf(path, sizeof(path), "%s/3a18-%u-%s.bin", dir, n, what);
    f = fopen(path, "rb");
    if (f == NULL)
        return -1;
    r = fread(dst, 1, size, f);
    fclose(f);
    if (got != NULL)
        *got = r;
    return 0;
}

static int replay_side(const uint32_t regs[36], int force, SideResult *out)
{
    uint64_t calls = rt.results.native_calls;

    memset(&cpu, 0, sizeof(cpu));
    initialize_cpu(&cpu, &rt);
    cpu.bus.write = recording_write;
    memcpy(cpu.gpr, regs, sizeof(cpu.gpr));
    cpu.hi = regs[32];
    cpu.lo = regs[33];
    cpu.gpr[31] = BATTLE_HALT_PC;
    rt.force_interpret = force;
    if (sigsetjmp(fault_jmp, 1) != 0) {
        rt.force_interpret = 0;
        return -1;
    }
    out->rc = PcPortMipsRun(&cpu, regs[35], BATTLE_HALT_PC, STEP_LIMIT);
    rt.force_interpret = 0;
    memcpy(out->gpr, cpu.gpr, sizeof(out->gpr));
    out->hi = cpu.hi;
    out->lo = cpu.lo;
    out->steps = cpu.steps;
    out->native_calls = rt.results.native_calls - calls;
    return 0;
}

static void replay_ranges(const char *label, const uint8_t *before, const uint8_t *after,
                          size_t n, uint32_t base)
{
    size_t i = 0, changed = 0, printed = 0;

    while (i < n) {
        size_t j;
        if (before[i] == after[i]) {
            i++;
            continue;
        }
        j = i;
        while (j < n && before[j] != after[j])
            j++;
        changed += j - i;
        if (printed++ < 24)
            printf("BATTLE RESULTS REPLAY 3A18   %s %08x..%08x (%zu bytes)\n", label,
                   (unsigned)(base + i), (unsigned)(base + j - 1), j - i);
        i = j;
    }
    printf("BATTLE RESULTS REPLAY 3A18   %s changed bytes=%zu ranges=%zu\n", label,
           changed, printed);
}

static int replay_run(const char *dir)
{
    static uint8_t loaded_ram[PSX_RAM_SIZE], loaded_pad[4096];
    static uint8_t loaded_gs[RESULTS_GS_BYTES];
    unsigned n, replayed = 0;

    for (n = 0; n < 4; n++) {
        uint32_t regs[36];
        size_t got_regs = 0, got_gs = 0;
        SideResult retail, native;
        size_t diffs, first = 0;
        unsigned reg;
        char detail[256];

        if (replay_load(dir, n, "regs", regs, sizeof(regs), &got_regs) != 0)
            break;
        if (got_regs != sizeof(regs) ||
            replay_load(dir, n, "ram", loaded_ram, sizeof(loaded_ram), NULL) != 0 ||
            replay_load(dir, n, "pad", loaded_pad, sizeof(loaded_pad), NULL) != 0 ||
            replay_load(dir, n, "gs", loaded_gs, sizeof(loaded_gs), &got_gs) != 0) {
            fail("replay-3a18", "capture", "incomplete capture files");
            continue;
        }
        memcpy(g_PsxRam, loaded_ram, sizeof(loaded_ram));
        memcpy(native_gs, loaded_gs, got_gs);
        memcpy(g_PsxScratchpad, loaded_pad, sizeof(loaded_pad));
        memcpy(loaded_gs, native_gs, RESULTS_GS_BYTES);
        if (regs[34] != 0) {
            fail("replay-3a18", "capture", "a delay-slot load was pending at the call");
            continue;
        }
        /* regs[35]: the captured routine (older 3A18-only captures: ~0). */
        if (regs[35] == 0xFFFFFFFFu)
            regs[35] = 0x801E3A18u;
        {
            unsigned k, listed = 0;
            for (k = 0; kResultsLeaves[k].name != NULL; k++)
                listed |= kResultsLeaves[k].address == regs[35];
            if (!listed) {
                fail("replay-3a18", "capture", "captured routine is not an adopted results body");
                continue;
            }
        }
        if (!results_identity_current(&rt, regs[35])) {
            fail("replay-3a18", "identity", "captured module is not the pinned retail image");
            continue;
        }
        printf("BATTLE RESULTS REPLAY 3A18 capture=%u routine=%08x sp=%08x ra=%08x "
               "a0=%08x a1=%08x a2=%08x a3=%08x gs_bytes=%zu\n", n, regs[35], regs[29],
               regs[31], regs[4], regs[5], regs[6], regs[7], got_gs);
        if (regs[35] == 0x801E3A18u)
            printf("BATTLE RESULTS REPLAY 3A18   slots=%08x kinds=%u/%u/%u\n",
                   peek32(0x801E44C8u), peek8(peek32(0x801E44C8u) + 0x56u),
                   peek8(peek32(0x801E44C8u) + 0x170u + 0x56u),
                   peek8(peek32(0x801E44C8u) + 0x2E0u + 0x56u));

        stray_writes = 0;
        unsetenv("XENO_DISABLE_CPU_INTERPRETER");
        if (replay_side(regs, 1, &retail) != 0 || retail.rc != PC_PORT_MIPS_HALTED ||
            retail.steps == 0 || retail.native_calls != 0) {
            fail("replay-3a18", "retail", cpu.error);
            continue;
        }
        memcpy(retail_ram, g_PsxRam, sizeof(retail_ram));
        memcpy(retail_gs, native_gs, RESULTS_GS_BYTES);
        memcpy(retail_pad, g_PsxScratchpad, sizeof(retail_pad));

        memcpy(g_PsxRam, loaded_ram, sizeof(loaded_ram));
        memcpy(native_gs, loaded_gs, RESULTS_GS_BYTES);
        memcpy(g_PsxScratchpad, loaded_pad, sizeof(loaded_pad));
        setenv("XENO_DISABLE_CPU_INTERPRETER", "1", 1);
        if (replay_side(regs, 0, &native) != 0) {
            unsetenv("XENO_DISABLE_CPU_INTERPRETER");
            fail("replay-3a18", "native-fault", "native body faulted");
            continue;
        }
        unsetenv("XENO_DISABLE_CPU_INTERPRETER");
        if (native.rc != PC_PORT_MIPS_HALTED || native.steps != 0 || native.native_calls != 1) {
            snprintf(detail, sizeof(detail), "rc=%d steps=%llu native_calls=%llu err=%s",
                     native.rc, (unsigned long long)native.steps,
                     (unsigned long long)native.native_calls, cpu.error);
            fail("replay-3a18", "not-interpreter-free", detail);
            continue;
        }
        for (reg = 1; reg < 32; reg++) {
            if (retail.gpr[reg] != native.gpr[reg]) {
                snprintf(detail, sizeof(detail), "r%u retail=%08x native=%08x", reg,
                         retail.gpr[reg], native.gpr[reg]);
                fail("replay-3a18", "exit-registers", detail);
                break;
            }
        }
        if (reg != 32)
            continue;
        if (retail.hi != native.hi || retail.lo != native.lo) {
            fail("replay-3a18", "hilo", "HI/LO differ");
            continue;
        }
        count_diffs(retail_ram, g_PsxRam, PSX_RAM_SIZE, 0, 0, &diffs, &first);
        if (diffs != 0) {
            snprintf(detail, sizeof(detail), "%zu bytes differ, first at guest %08x",
                     diffs, (unsigned)(0x80000000u | first));
            fail("replay-3a18", "ram", detail);
            continue;
        }
        count_diffs(retail_gs, native_gs, RESULTS_GS_BYTES, 0, 0, &diffs, &first);
        if (diffs != 0 || memcmp(retail_pad, g_PsxScratchpad, sizeof(retail_pad)) != 0 ||
            stray_writes != 0) {
            fail("replay-3a18", "native-copy/scratchpad/stray", "differs");
            continue;
        }
        printf("BATTLE RESULTS REPLAY 3A18 capture=%u routine=%08x PASS: retail %llu interpreted "
               "instructions; native 0 interpreted (CPU interpreter disabled), 1 native "
               "results call, fallback 0; registers, HI/LO, RAM, g_GameState copy and "
               "scratchpad identical. Observed side effects:\n", n, regs[35],
               (unsigned long long)retail.steps);
        replay_ranges("ram", loaded_ram, retail_ram, PSX_RAM_SIZE, 0x80000000u);
        replay_ranges("g_GameState", loaded_gs, retail_gs, RESULTS_GS_BYTES, 0x8006D634u);
        replayed++;
    }
    if (replayed == 0)
        fail("replay-3a18", "capture", "no complete capture in the directory");
    if (failures != 0) {
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL replay-3a18 total=%u\n", failures);
        return 1;
    }
    printf("BATTLE RESULTS REPLAY 3A18 PASS captures=%u\n", replayed);
    return 0;
}

/* An adopted address is dispatched whenever the guest PC equals it, so it
 * must be reachable only as a call: no branch, J, or data word (jump table)
 * in the module may target it. */
static void entry_reachability(uint32_t entry, const char *name)
{
    uint32_t offset;
    for (offset = 0; offset < RESULTS_IMMUTABLE_BYTES; offset += 4) {
        uint32_t word = load_le(module_image + offset, 4);
        uint32_t pc = MODULE_ADDR + offset;
        uint32_t op = word >> 26;
        char detail[96];
        if (word == entry) {
            snprintf(detail, sizeof(detail), "data word at %08x", pc);
            fail(name, "entry-reached-not-as-call", detail);
        }
        if (op == 2u && (((pc + 4u) & 0xF0000000u) | ((word & 0x03FFFFFFu) << 2)) == entry) {
            snprintf(detail, sizeof(detail), "J at %08x", pc);
            fail(name, "entry-reached-not-as-call", detail);
        }
        if (op == 1u || (op >= 4u && op <= 7u) || (op >= 0x14u && op <= 0x17u)) {
            uint32_t t = pc + 4u + (uint32_t)((int32_t)(int16_t)(word & 0xFFFFu) * 4);
            if (t == entry) {
                snprintf(detail, sizeof(detail), "branch at %08x", pc);
                fail(name, "entry-reached-not-as-call", detail);
            }
        }
    }
    controls++;
}

int main(int argc, char **argv)
{
    FILE *f;
    size_t i;
    unsigned spec_count = (unsigned)(sizeof(kSpecs) / sizeof(kSpecs[0]));

    if (argc != 2 && !(argc == 3 && (strcmp(argv[2], "--meter") == 0 ||
                                     strcmp(argv[2], "--compose-3a18") == 0)) &&
        !(argc == 4 && strcmp(argv[2], "--replay-3a18") == 0)) {
        fprintf(stderr, "usage: %s results_module.bin [--meter|--compose-3a18|"
                "--replay-3a18 DIR]\n", argv[0]);
        return 2;
    }
    f = fopen(argv[1], "rb");
    if (f == NULL || fread(module_image, 1, MODULE_BYTES, f) != MODULE_BYTES) {
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL cannot read %s\n", argv[1]);
        return 2;
    }
    fclose(f);

    memset(&rt, 0, sizeof(rt));
    g_ActiveBattleRuntime = &rt;
    initialize_runtime(&rt);
    for (i = 0; i < rt.data_count; i++) {
        if (strcmp(rt.data[i].name, "g_GameState") == 0)
            native_gs = rt.data[i].host;
    }
    if (native_gs == NULL || native_gs != dlsym(RTLD_DEFAULT, "g_GameState")) {
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL g_GameState is not "
                "bound to the linked production object\n");
        return 1;
    }
    signal(SIGSEGV, fault_handler);
    signal(SIGBUS, fault_handler);

    begin_case(1);
    if (!results_identity_current(&rt, MODULE_ADDR)) {
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL loaded module does not "
                "match the pinned identity\n");
        return 1;
    }

    if (argc == 4)
        return replay_run(argv[3]);
    if (argc == 3 && strcmp(argv[2], "--compose-3a18") == 0)
        return compose_run(4000u);
    if (argc == 3)
        return meter_run();

    accessor_controls();
    for (i = 0; kResultsLeaves[i].name != NULL; i++) {
        const BattleResultsLeaf *leaf = &kResultsLeaves[i];
        const LeafSpec *spec = NULL;
        unsigned s, c, proven = 0;

        for (s = 0; s < spec_count; s++) {
            if (kSpecs[s].address == leaf->address)
                spec = &kSpecs[s];
        }
        if (spec == NULL) {
            fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL UNPROVEN %s (no case "
                    "shaper in this test)\n", leaf->name);
            failures++;
            continue;
        }
        entry_reachability(leaf->address, leaf->name);
        for (c = 0; c < spec->edge_cases + RANDOM_CASES; c++) {
            uint32_t seed = leaf->address ^ (0x9E3779B9u * (c + 1u));
            uint32_t args[4];
            begin_case(seed);
            spec->shape(c, args);
            proven += compare_case(leaf->name, leaf->address, args, seed);
        }
        if (proven != spec->edge_cases + RANDOM_CASES) {
            fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL %s proven=%u of %u\n",
                    leaf->name, proven, spec->edge_cases + RANDOM_CASES);
            failures++;
        } else {
            printf("BATTLE RESULTS DIFFERENTIAL leaf %s cases=%u\n", leaf->name,
                   proven);
        }
        gate_controls(spec);
    }
    if (kResultsLeaves[0].name == NULL)
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL note: the allowlist is empty\n");
    /* Outside a metered battle the meter is inert, set or not. */
    CONTROL("cpu-fallback meter idle outside a battle",
            !g_CpuFallbackMeter.active && g_CpuFallbackMeter.interpreted_instructions == 0 &&
                g_CpuFallbackMeter.interpreted_calls == 0 &&
                g_CpuFallbackMeter.native_results_calls == 0 &&
                g_CpuFallbackMeter.native_main_exe_calls == 0 &&
                g_CpuFallbackMeter.native_overlay_calls == 0 &&
                g_CpuFallbackMeter.distinct_targets == 0);

    if (failures != 0) {
        fprintf(stderr, "BATTLE RESULTS DIFFERENTIAL FAIL total=%u\n", failures);
        return 1;
    }
    printf("BATTLE RESULTS DIFFERENTIAL PASS cases=%u controls=%u leaves=%u\n",
           checks, controls, (unsigned)(RESULTS_LEAF_SLOTS - 1u));
    return 0;
}
