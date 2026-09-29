/* Differential test for three src/menu/main/misc.c port bodies that used to
 * be bare INCLUDE_ASM with no C body anywhere (func_801CA8C0, func_801E4A28,
 * func_801E4D10). Each is run twice from identical input state: once as the
 * native ported C body (extracted verbatim from the live source by the .sh
 * runner, so this test cannot silently drift from what actually ships),
 * once as the real retail MIPS bytes from disc/menu.bin executed by the
 * project's own MIPS interpreter (PcPortMipsRun). Final memory state must
 * match exactly.
 *
 * (A fourth candidate, func_801D261C, was pulled back out of this pass: it
 * treats g_Menu itself as a flat byte array at fixed retail offsets, and
 * this test's own investigation found that SystemMenu's compiled layout on
 * this 64-bit host build drifts from retail by up to 0xA0 bytes -- several
 * fields are native (8-byte) pointers where retail had 4-byte ones. A body
 * built on that assumption would corrupt unrelated SystemMenu fields at
 * runtime; see the comment left in misc.c at its INCLUDE_ASM site.)
 *
 * func_801CA8C0 also touches a struct with this exact property (MenuUnk2
 * embeds a TIM_IMAGE full of pointers before the fields it uses), so this
 * test cannot compare the two sides with a flat struct memcmp either --
 * the host struct's compiled offsets do not equal retail's numeric ones.
 * Comparisons below always go through the host struct's own named field
 * (letting the compiler resolve its real offset) against the retail side's
 * literal numeric offset, never a whole-struct byte compare.
 *
 * Two PSX library calls the retail bytes make (strcat, memcpy) are bridged
 * to real host implementations operating on the emulated bus, since the PSX
 * SDK library code itself is not part of menu.bin.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "system/menu.h"
#include "battle_mips_adapter.h"

#define MENU_BASE 0x801c5000u
#define RAM_SIZE (2u * 1024u * 1024u)

SystemMenu *g_Menu;
/* include/main/game.h (pulled in transitively via system/menu.h) already
 * declares `extern GameState g_GameState;`, so this test cannot also
 * declare a plain `unsigned char g_GameState[]` in the same translation
 * unit -- conflicting types for the same symbol. The actual storage is
 * provided by linking the real pc_port/src/data_game_state.c (the runner
 * script compiles it in) unmodified; this test only ever touches it
 * through byte-array aliases bound via __asm__, the same trick misc.c's
 * own port body already uses for func_801E433C et al. */
extern u8 MenuEquipmentStateStorage[0x4600] __asm__("g_GameState");
static u8 *const g_GameStateBytes = MenuEquipmentStateStorage;

static unsigned char ram[RAM_SIZE];
static unsigned gear_calls_native, gear_calls_interp;

/* Forward declarations for the already-ported dependencies func_801E4D10
 * calls; stubbed further down as call-counting spies on both sides -- not
 * re-verified in this test, see file header. */
void func_801E41C0(s32 arg0, u8 idx);
void func_801E4258(void *pCtx, u8 idx);
void func_801E42AC(s32 arg0, u8 idx);
void func_801E433C(void *resourceArg, u8 gearId);

/* Real retail bytes for the rodata table func_801CA8C0 reads, sliced
 * straight out of disc/menu.bin by the .sh runner -- generated, not
 * guessed. */
#include "rodata.inc"

#include "bodies.inc"

/* ---- stubs shared by the native call path ---- */
void func_801E41C0(s32 arg0, u8 idx) { (void)arg0; (void)idx; gear_calls_native++; }
void func_801E4258(void *pCtx, u8 idx) { (void)pCtx; (void)idx; gear_calls_native++; }
void func_801E42AC(s32 arg0, u8 idx) { (void)arg0; (void)idx; gear_calls_native++; }
void func_801E433C(void *resourceArg, u8 gearId) { (void)resourceArg; (void)gearId; gear_calls_native++; }

/* ---- emulated PSX bus over `ram`, PSX-address-mapped exactly like the
 * project's other retail-oracle tests (menu_card_wait_test.c et al). ---- */
static uint8_t *bptr(uint32_t a, unsigned w) {
    a &= 0x1fffffff;
    assert((uint64_t)a + w <= sizeof ram);
    return ram + a;
}
static int read_bus(void *o, uint32_t a, unsigned w, uint32_t *v) {
    (void)o;
    uint8_t *p = bptr(a, w);
    *v = 0;
    for (unsigned i = 0; i < w; ++i) *v |= (uint32_t)p[i] << (8 * i);
    return 0;
}
static int write_bus(void *o, uint32_t a, unsigned w, uint32_t v) {
    (void)o;
    uint8_t *p = bptr(a, w);
    for (unsigned i = 0; i < w; ++i) p[i] = (uint8_t)(v >> (8 * i));
    return 0;
}
static void wr32(uint32_t a, uint32_t v) { write_bus(NULL, a, 4, v); }

static int bridge(void *o, PcPortMipsCpu *c, uint32_t t) {
    (void)o;
    switch (t) {
    case 0x8003fa78u: { /* strcat(dest, src) */
        uint32_t dest = c->gpr[4], src = c->gpr[5];
        uint32_t d = dest;
        while (bptr(d, 1)[0]) d++;
        for (;;) {
            uint8_t ch = bptr(src, 1)[0];
            bptr(d, 1)[0] = ch;
            if (!ch) break;
            d++; src++;
        }
        c->gpr[2] = dest;
        return 1;
    }
    case 0x8003f968u: { /* memcpy(dest, src, n) */
        uint32_t dest = c->gpr[4], src = c->gpr[5], n = c->gpr[6];
        memcpy(bptr(dest, n), bptr(src, n), n);
        c->gpr[2] = dest;
        return 1;
    }
    case 0x801e41c0u:
    case 0x801e4258u:
    case 0x801e42acu:
    case 0x801e433cu:
        gear_calls_interp++;
        c->gpr[2] = 0;
        return 1;
    default:
        return 0;
    }
}

static uint32_t run_interp(uint32_t entry, uint32_t a0, uint32_t a1) {
    PcPortMipsBus bus = {.read = read_bus, .write = write_bus, .bridge = bridge};
    PcPortMipsCpu c;
    PcPortMipsCpuInit(&c, &bus);
    c.gpr[4] = a0;
    c.gpr[5] = a1;
    c.gpr[29] = 0x801ff000u;
    c.gpr[31] = 0xfffffffcu;
    assert(PcPortMipsRun(&c, entry, 0xfffffffcu, 200000) == PC_PORT_MIPS_HALTED);
    return c.gpr[2];
}

/* g_Menu (pointer var) lives at 0x800625a0; the SystemMenu it points to and
 * the scratch structs it in turn points to are placed well clear of
 * menu.bin's own image (loaded at 0x801c5000..~0x801ea908) and of
 * g_GameState (0x8006d634..+0x4600). */
#define GMENU_VAR_ADDR   0x800625a0u
#define SYSMENU_ADDR     0x80100000u
#define MENUUNK2_ADDR    0x80110000u
#define SNAPSHOT_ADDR    0x80130000u
#define GAMESTATE_ADDR   0x8006d634u

static void load_menu_bin(void) {
    FILE *f = fopen("disc/menu.bin", "rb");
    assert(f);
    size_t n = fread(ram + (MENU_BASE & 0x1fffffff), 1, RAM_SIZE - (MENU_BASE & 0x1fffffff), f);
    assert(n > 0x25000 && n < 0x30000); /* sanity: full menu.bin, not a truncated read */
    fclose(f);
}

static void seed_rng(uint32_t seed, unsigned char *buf, size_t n) {
    uint32_t s = seed;
    for (size_t i = 0; i < n; i++) {
        s = s * 1103515245u + 12345u;
        buf[i] = (unsigned char)(s >> 16);
    }
}

/* ---- Test 1: func_801CA8C0 ---- */
static void test_ca8c0(void) {
    /* Every possible (u8)arg0 truncation (0..255), plus values that
     * exercise the s32-to-u8 truncation itself (256, 1000, -3, -300). */
    static const s32 extra[] = {256, 1000, -3, -300};
    unsigned cases = 0;
    int i;

    for (i = 0; i < 256 + (int)(sizeof(extra) / sizeof(extra[0])); i++) {
        s32 arg0 = (i < 256) ? i : extra[i - 256];
        SystemMenu menu_host;
        MenuUnk2 card_host;
        MenuUnk2 card_ref;

        memset(&card_host, 0xA5, sizeof card_host);
        memcpy(card_host.unk4F80 + 0x7C, "XENOGEARS", 10);
        memset(&menu_host, 0, sizeof menu_host);
        menu_host.unk32C = &card_host;
        card_ref = card_host;

        g_Menu = &menu_host;
        func_801CA8C0(arg0);

        wr32(GMENU_VAR_ADDR, SYSMENU_ADDR);
        wr32(SYSMENU_ADDR + 0x32C, MENUUNK2_ADDR);
        /* card_ref (pre-call snapshot) seeds the interpreter's copy of the
         * card at the RETAIL numeric offset (0x4B98 etc are literal
         * immediates in the retail machine code, ground truth by
         * construction) -- not at whatever this host build's compiler
         * happened to place MenuUnk2's own unk4B98 field at (measured:
         * 0x4ba8, sixteen bytes further in, because MenuUnk2 embeds a
         * TIM_IMAGE full of host-width pointers ahead of it). */
        memcpy(bptr(MENUUNK2_ADDR + 0x4B98, sizeof card_ref.unk4B98),
               card_ref.unk4B98, sizeof card_ref.unk4B98);
        memcpy(bptr(MENUUNK2_ADDR + 0x4F80, sizeof card_ref.unk4F80),
               card_ref.unk4F80, sizeof card_ref.unk4F80);
        run_interp(0x801ca8c0u, (uint32_t)arg0, 0);

        assert(!memcmp(card_host.unk4B98, bptr(MENUUNK2_ADDR + 0x4B98, sizeof card_host.unk4B98),
                        sizeof card_host.unk4B98));
        assert(!memcmp(card_host.unk4F80, bptr(MENUUNK2_ADDR + 0x4F80, sizeof card_host.unk4F80),
                        sizeof card_host.unk4F80));
        cases++;
    }
    printf("PASS %u func_801CA8C0 retail/native fixtures\n", cases);
}

/* ---- Test 2: func_801E4A28 ---- */
static void test_e4a28(void) {
    unsigned cases = 0;
    uint32_t seed;

    for (seed = 1; seed <= 5; seed++) {
        unsigned char snap_host[0x2000];
        void *ret;
        uint32_t interp_ret;

        seed_rng(seed, g_GameStateBytes, 0x4600);
        memset(snap_host, 0, sizeof snap_host);
        memcpy(bptr(GAMESTATE_ADDR, 0x4600), g_GameStateBytes, 0x4600);
        memset(bptr(SNAPSHOT_ADDR, sizeof snap_host), 0, sizeof snap_host);

        ret = func_801E4A28(snap_host);
        assert(ret == snap_host + 0x1124);

        interp_ret = run_interp(0x801e4a28u, SNAPSHOT_ADDR, 0);
        assert(interp_ret == SNAPSHOT_ADDR + 0x1124);

        assert(!memcmp(snap_host, bptr(SNAPSHOT_ADDR, sizeof snap_host), sizeof snap_host));
        cases++;
    }
    printf("PASS %u func_801E4A28 retail/native fixtures\n", cases);
}

/* ---- Test 3: func_801E4D10 ---- */
static void test_e4d10(void) {
    /* static, not stack-local: func_801E4D10's retail arg0 is a 32-bit
     * register carrying a pointer (see func_801CB28C's forward declaration,
     * `extern void* func_801E4D10(s32, s32);`), and this test round-trips
     * it through an actual s32 the same way the real port does -- which
     * only stays lossless if the buffer's host address is below 4 GiB. A
     * stack address on a normal (PIE, ASLR) 64-bit Linux build is not; a
     * statically-allocated buffer in a -no-pie-linked test binary is. */
    static unsigned char snap_host[0x2000];
    unsigned cases = 0;
    uint32_t seed;

    for (seed = 1; seed <= 5; seed++) {
        void *ret;
        uint32_t interp_ret;

        seed_rng(seed + 100, snap_host, sizeof snap_host);
        seed_rng(seed + 200, g_GameStateBytes, 0x4600);
        memcpy(bptr(GAMESTATE_ADDR, 0x4600), g_GameStateBytes, 0x4600);
        memcpy(bptr(SNAPSHOT_ADDR, sizeof snap_host), snap_host, sizeof snap_host);

        gear_calls_native = 0;
        ret = func_801E4D10((s32)(intptr_t)snap_host, 0xdead);
        assert(ret == g_GameStateBytes + 0x1920);
        assert(gear_calls_native == 80); /* 20 iterations * 4 helper calls each */

        gear_calls_interp = 0;
        interp_ret = run_interp(0x801e4d10u, SNAPSHOT_ADDR, 0xdead);
        assert(interp_ret == GAMESTATE_ADDR + 0x1920);
        assert(gear_calls_interp == 80); /* 20 iterations * 4 helper calls each */

        assert(!memcmp(g_GameStateBytes, bptr(GAMESTATE_ADDR, 0x4600), 0x4600));
        cases++;
    }
    printf("PASS %u func_801E4D10 retail/native fixtures\n", cases);
}

int main(void) {
    load_menu_bin();
    test_ca8c0();
    test_e4a28();
    test_e4d10();
    return 0;
}
