/* Opcode 0xBC sub-command 0x22 differential.
 *
 * Retail (SLUS 0x8001FBE4, jtbl_800185A8 index 0x22 -> 0x8002049C -> the
 * shared .L80020550 sub-0x14 body) is executed on the MIPS adapter with the
 * player sprite pointer D_800C3E1C seeded in guest RAM; the shipped native
 * func_8001FBE4 runs on the identical fixture. Both must leave the sprite
 * bytes identical. ApplyMatrixSV is bridged to one deterministic host
 * function shared by both sides so the camera-relative tail is exercised
 * rather than stubbed.
 *
 * Inputs swept: player X/Y/Z, +0x38 height (including odd/even/sign extremes),
 * sprite +0x3F camera flag, operand bit 6 destination, and both pointer
 * encodings the port must accept in D_800C3E1C (guest address / native host
 * pointer inside g_PsxRam). */
#include "battle_mips_adapter.h"
#include "psx_memory.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;

extern void func_8001FBE4(void*, u32, void*);
extern uint8_t g_PsxRam[];    /* defined by the shared guard object */
extern uint32_t D_8004FBB8[]; /* host MATRIX mirror, guard object */

/* Deterministic stand-in: the retail slice reaches it through the bus bridge,
 * the native body through the linked symbol, so the camera-relative tail is
 * compared against the same transform. */
void* ApplyMatrixSV(void* m, void* v0, void* v1);
void* ApplyMatrixSV(void* m, void* v0, void* v1) {
    const s16* s = (const s16*)v0;
    s16* d = (s16*)v1;
    (void)m;
    d[0] = (s16)(s[0] + 0x111);
    d[1] = (s16)(s[1] + 0x222);
    d[2] = (s16)(s[2] + 0x333);
    return v1;
}

#define SPRITE 0x80100200u
#define PLAYER 0x80100400u
#define OPS    0x80100600u
#define STACK  0x801ff000u
#define HALT   0xfffffffcu
#define D_PLAYER_SLOT 0x800C3E1Cu
#define SPRITE_LEN 0x100u

static u8 initial[SPRITE_LEN];
static u8 expected[SPRITE_LEN];
static unsigned cases;

static u8* bus_ptr(u32 v) {
    uintptr_t base = (uintptr_t)g_PsxRam;
    if (v >= base && (uint64_t)v < base + PSX_RAM_SIZE)
        return (u8*)(uintptr_t)v;
    if ((v & 0xFFE00000u) == 0x80000000u || (v & 0xFFE00000u) == 0xA0000000u)
        return (u8*)PSX_ADDR(v);
    return NULL;
}
static int rd(void* o, u32 a, unsigned w, u32* v) {
    u8* p = bus_ptr(a);
    (void)o;
    if (p == NULL)
        return -1;
    *v = 0;
    for (unsigned i = 0; i < w; ++i)
        *v |= (u32)p[i] << (i * 8);
    return 0;
}
static int wr(void* o, u32 a, unsigned w, u32 v) {
    u8* p = bus_ptr(a);
    (void)o;
    if (p == NULL)
        return -1;
    for (unsigned i = 0; i < w; ++i)
        p[i] = (u8)(v >> (i * 8));
    return 0;
}
static int bridge(void* o, PcPortMipsCpu* c, u32 t) {
    (void)o;
    if (t == 0x80049D3Cu) { /* ApplyMatrixSV */
        ApplyMatrixSV(bus_ptr(c->gpr[4]), bus_ptr(c->gpr[5]),
                      bus_ptr(c->gpr[6]));
        c->gpr[2] = c->gpr[6];
        return 1;
    }
    return 0;
}

static void fail(const char* what, u32 pos, u16 h, unsigned cam, unsigned dst,
                 u32 mode) {
    fprintf(stderr,
            "SPRITE BC22 FAIL case=%u %s pos=%08x h=%04x cam=%u dst=%u mode=%u\n",
            cases, what, pos, h, cam, dst, mode);
    exit(1);
}

static void run(u32 x, u32 y, u32 z, u16 h38, unsigned camera,
                unsigned destA0, unsigned hostPointer) {
    u8* sprite = (u8*)PSX_ADDR(SPRITE);
    u8* player = (u8*)PSX_ADDR(PLAYER);
    u8* ops = (u8*)PSX_ADDR(OPS);
    u32 op0 = 0x80u | 0x22u | (destA0 ? 0x40u : 0x00u);

    for (unsigned i = 0; i < SPRITE_LEN; ++i)
        sprite[i] = (u8)(i * 7u + x + h38);
    sprite[0x3F] = (u8)(camera ? 1u : 0u);

    for (unsigned i = 0; i < 0x40; ++i)
        player[i] = (u8)(i * 13u + y);
    memcpy(player + 0x2, &x, 2);
    memcpy(player + 0x6, &y, 2);
    memcpy(player + 0xA, &z, 2);
    memcpy(player + 0x38, &h38, 2);

    ops[0] = (u8)op0;
    ops[1] = 0;

    u32 pointer = hostPointer ? (u32)(uintptr_t)player : PLAYER;
    memcpy(PSX_ADDR(D_PLAYER_SLOT), &pointer, 4);

    /* Native tail adds the low halfwords of D_8004FBB8; keep the host mirror
     * byte-identical to the disc image the retail slice reads. */
    memcpy(D_8004FBB8, PSX_ADDR(0x8004FBB8), 0x20);

    memcpy(initial, sprite, SPRITE_LEN);

    PcPortMipsBus bus = {.read = rd, .write = wr, .bridge = bridge};
    PcPortMipsCpu cpu;
    PcPortMipsCpuInit(&cpu, &bus);
    cpu.gpr[4] = SPRITE;
    cpu.gpr[5] = 0x80000000u | 0xBCu;
    cpu.gpr[6] = OPS;
    cpu.gpr[29] = STACK;
    cpu.gpr[31] = HALT;
    if (PcPortMipsRun(&cpu, 0x8001fbe4u, HALT, 8000) != PC_PORT_MIPS_HALTED) {
        fprintf(stderr, "SPRITE BC22 oracle pc=%08x %s\n", cpu.pc, cpu.error);
        exit(2);
    }
    memcpy(expected, sprite, SPRITE_LEN);

    /* Reset only the sprite; the seeded globals stay for the native run. */
    memcpy(sprite, initial, SPRITE_LEN);
    func_8001FBE4(sprite, 0x80000000u | 0xBCu, ops);

    if (memcmp(sprite, expected, SPRITE_LEN)) {
        for (unsigned i = 0; i < SPRITE_LEN; ++i)
            if (sprite[i] != expected[i])
                fprintf(stderr, "  diff +%02x native=%02x retail=%02x\n",
                        i, sprite[i], expected[i]);
        fail("sprite bytes", x, h38, camera, destA0, hostPointer);
    }
    cases++;
}

int main(void) {
    FILE* image;
    u32 entry;

    assert((uintptr_t)g_PsxRam + PSX_RAM_SIZE <= UINT32_MAX);
    assert((uintptr_t)PSX_ADDR(PLAYER) <= UINT32_MAX);

    image = fopen("disc/SLUS_006.64", "rb");
    if (image == NULL)
        return 2;
    if (fseek(image, 0x800, SEEK_SET) != 0)
        return 2;
    if (fread(PSX_ADDR(0x80010000), 1, 0x49800u, image) < 0x48000u)
        return 2;
    fclose(image);

    /* jtbl_800185A8 index 0x22 must still point at the 0x22 entry. */
    if (rd(NULL, 0x800185A8u + 0x22u * 4u, 4, &entry) != 0 ||
        entry != 0x8002049Cu) {
        fprintf(stderr, "SPRITE BC22 FAIL dispatch entry=%08x\n", entry);
        return 1;
    }

    {
        const u32 xs[] = {0x0000, 0x1234, 0xF000, 0xFFFF, 0x7FFF, 0x8000};
        const u16 hs[] = {0x0000, 0x0001, 0x0002, 0x0003, 0xFFFF, 0x8000,
                          0x7FFF};
        for (unsigned xi = 0; xi < 6; ++xi)
            for (unsigned hi = 0; hi < 7; ++hi)
                for (unsigned camera = 0; camera < 2; ++camera)
                    for (unsigned dst = 0; dst < 2; ++dst)
                        for (unsigned mode = 0; mode < 2; ++mode)
                            run(xs[xi], xs[(xi + 2) % 6], xs[(xi + 4) % 6],
                                hs[hi], camera, dst, mode);
    }

    printf("SPRITE BC22 PASS cases=%u\n", cases);
    return 0;
}
