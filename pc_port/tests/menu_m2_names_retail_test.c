/* M2 card-buffer init retail differential: func_801C6400 (memory-card state
 * reset + current location name copied from archive file 1).
 *
 * Native C body (src/menu/main/misc.c, XENO_PC_PORT arm) vs retail machine
 * code on the interpreter oracle (TEST ORACLE ONLY), identical guest memory,
 * via lib/m2_retail_diff.h. MenuUnk2 (g_Menu->unk32C) is mapped as retail
 * offsets [0,0xB80) and [0xB94,0x5034) onto the host struct (the embedded
 * TIM_IMAGE is pointer-inflated on the host, see MENU_CARD_OFF).
 */
#include "common.h"
#include "system/menu.h"
#include "main/game.h"
#include "lib/m2_retail_diff.h"

#define A6400 0x801C6400u
#define E6400 0x801C65F4u
#define U2_GUEST 0x80130000u
#define BUF_GUEST 0x80160000u
#define GAME_GUEST 0x8006D634u
#define GAME_SPAN 0x2000u

SystemMenu *g_Menu;
static SystemMenu menu;
static MenuUnk2 u2;
static u8 fileBuf[0x8000];
u8 stateStorage[GAME_SPAN] __attribute__((aligned(8)));
__asm__(".globl g_GameState\n.set g_GameState, stateStorage\n");

static u32 TSETIDX, TDECSZ, THEAPALLOC, TREAD, TSYNC, THEAPFREE;
static unsigned bufSeed;

static void fill_file(unsigned seed) {
    unsigned i;
    m2_pattern(fileBuf, sizeof fileBuf, seed + 77u);
    for (i = 0; i < sizeof fileBuf; i++) {
        u8 b = fileBuf[i];
        /* Mostly text, with newlines and 2-byte codes (>= 0x80) mixed in. */
        if (b < 0x30)
            fileBuf[i] = 0x0A;
        else if (b < 0x34)
            fileBuf[i] = 0x80; /* boundary of the 2-byte code test */
        else if (b < 0x60 && (seed & 1u))
            fileBuf[i] = (u8)(0x80u + (b & 0x7F));
        else
            fileBuf[i] = (u8)(0x20u + (b % 0x5Fu));
    }
}

static u32 ret_cb(u32 idx, const u32 *a) {
    u32 t = m2_callees[idx].addr;
    if (t == TDECSZ)
        return 0x6000u + a[0];
    if (t == THEAPALLOC)
        return BUF_GUEST;
    if (t == TREAD) {
        fill_file(bufSeed);
        return 0;
    }
    return 0;
}

void ArchiveSetIndex(s32 a, s32 b) { M2_SPY(TSETIDX, (u32)a, (u32)b); }
s32 ArchiveDecodeAlignedSize(s32 a) { return (s32)M2_SPY(TDECSZ, (u32)a); }
void *HeapAlloc(s32 size, s32 b) { return m2_denorm(M2_SPY(THEAPALLOC, (u32)size, (u32)b)); }
s32 ArchiveReadFileToBuffer(s32 a, void *b, s32 c, s32 d) {
    return (s32)M2_SPY(TREAD, (u32)a, M2_U(b), (u32)c, (u32)d);
}
s32 ArchiveCdDataSync(s32 a) { return (s32)M2_SPY(TSYNC, (u32)a); }
u_int HeapFree(void *p) { return M2_SPY(THEAPFREE, M2_U(p)); }

extern void func_801C6400(void);
static unsigned cov6400[(E6400 - A6400) / 4];

static void run_one(unsigned seed) {
    static M2Snap before, nat, ora;
    static const u16 idx[] = { 0, 1, 2, 3, 7, 20, 41, 0xFFFF };
    m2_pattern(&menu, sizeof menu, seed + 1u);
    m2_pattern(&u2, sizeof u2, seed + 2u);
    m2_pattern(stateStorage, sizeof stateStorage, seed + 3u);
    m2_pattern(fileBuf, sizeof fileBuf, seed + 4u);
    menu.unk32C = &u2;
    bufSeed = seed;
    {
        u16 n = idx[seed % 8u];
        if (n == 0xFFFF)
            n = (u16)(seed % 60u);
        stateStorage[0x1930] = (u8)n;
        stateStorage[0x1931] = (u8)(n >> 8);
    }
    m2_save(&before);
    m2_trace_begin();
    g_Menu = &menu;
    func_801C6400();
    m2_trace_stash();
    m2_save(&nat);
    m2_restore(&before);
    m2_oracle(A6400, E6400, cov6400, NULL, 0);
    m2_save(&ora);
    m2_compare(&nat, &ora);
    m2_trace_compare();
    m2_cases++;
}

int main(void) {
    unsigned seed;

    m2_tag = "M2 NAMES";
    m2_load_code("disc/menu.bin");
    m2_require(m2_word(A6400) == 0x27BDFFE0u, "6400 pin");
    m2_region(&u2, U2_GUEST, 0xB80u, 1, "unk32C[0,B80)");
    m2_region(u2.unkB94, U2_GUEST + 0xB94u, (u32)(sizeof u2 - offsetof(MenuUnk2, unkB94)), 1,
              "unk32C[B94,5034)");
    m2_require(sizeof u2 - offsetof(MenuUnk2, unkB94) == 0x5034u - 0xB94u, "MenuUnk2 tail");
    m2_region(fileBuf, BUF_GUEST, sizeof fileBuf, 1, "file buffer");
    m2_region(stateStorage, GAME_GUEST, sizeof stateStorage, 1, "g_GameState");
    m2_menu(&menu, sizeof menu);
    M2_PTRFIELD(0x32C, menu.unk32C);

    TSETIDX = m2_jal_target(0x801C64F4u);
    TDECSZ = m2_jal_target(0x801C64FCu);
    THEAPALLOC = m2_jal_target(0x801C6508u);
    TREAD = m2_jal_target(0x801C6520u);
    TSYNC = m2_jal_target(0x801C6528u);
    THEAPFREE = m2_jal_target(0x801C65D0u);
    m2_callee(TSETIDX, "ArchiveSetIndex", 2, ret_cb);
    m2_callee(TDECSZ, "ArchiveDecodeAlignedSize", 1, ret_cb);
    m2_callee(THEAPALLOC, "HeapAlloc", 2, ret_cb);
    m2_callee(TREAD, "ArchiveReadFileToBuffer", 4, ret_cb);
    m2_callee(TSYNC, "ArchiveCdDataSync", 1, ret_cb);
    m2_callee(THEAPFREE, "HeapFree", 1, ret_cb);
    m2_scan_calls(A6400, E6400);

    for (seed = 0; seed < 64; seed++)
        run_one(seed);
    m2_coverage("6400", cov6400, (E6400 - A6400) / 4, A6400, NULL, 0);
    printf("M2 NAMES PASS cases=%u checks=%u\n", m2_cases, m2_checks);
    return 0;
}
