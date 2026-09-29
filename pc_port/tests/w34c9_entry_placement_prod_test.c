/* W34C9 — fresh-entry placement certificate for wm_80073448. Oracle is the
 * retail contract: table rows {x,id,z,pad} stride 8 terminated by id==-1,
 * match on the sign-extended id, position = (x<<12, 0, z<<12) or zeros;
 * flag branch clears 0x2000, calls wm_8008DFF4(0x8009C5AC), copies EE66. */
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_73448.h"

static int s_failures;
#define ASSERT_MSG(c, n, ...) do { if (!(c)) { s_failures++; fprintf(stderr, "ASSERTION %s FAILED: ", n); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)
static void sh(u32 a, u16 v) { memcpy(PSX_ADDR(a), &v, 2); }
static void sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u32 lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }
static u16 lhu(u32 a) { u16 v; memcpy(&v, PSX_ADDR(a), 2); return v; }

static int s_dff4_calls; static u32 s_dff4_arg;
void wm_8008DFF4(u32 out_addr) { s_dff4_calls++; s_dff4_arg = out_addr; sw(out_addr, 0x11111111u); }

#define TBL 0x800B66ECu
static void seed(u16 flags)
{
    PsxMemory_Init();
    sw(0x8009D3F4u, TBL);
    /* rows: (x,id,z): asymmetric, includes negative x/z and a negative id */
    static const s16 rows[][4] = { {29947, 3, 11075, 0}, {-1234, -7, 8888, 0}, {500, 1, -600, 0}, {0, -1, 0, 0}, {9999, 1, 9999, 0} };
    for (u32 i = 0; i < 5u; i++) for (u32 j = 0; j < 4u; j++) sh(TBL + i * 8u + j * 2u, (u16)rows[i][j]);
    sh(0x8006EE68u, flags); sh(0x8006EE66u, 0x4A5Bu);
    sw(0x8009C5ACu, 0xDEADBEEFu); sw(0x8009C5B0u, 0xDEADBEEFu); sw(0x8009C5B4u, 0xDEADBEEFu);
    sw(0x8009C584u, 0xCAFEF00Du);
    memset(PSX_ADDR(0x8009C5B8u), 0xA5, 16); memset(PSX_ADDR(0x8009C59Cu), 0xA5, 16);
    s_dff4_calls = 0; s_dff4_arg = 0;
}
static void canaries(const char* n)
{
    const u8* a = (const u8*)PSX_ADDR(0x8009C5B8u); const u8* b = (const u8*)PSX_ADDR(0x8009C59Cu);
    ASSERT_MSG(a[0] == 0xA5 && a[15] == 0xA5 && b[0] == 0xA5 && b[15] == 0xA5, "canary", "%s: neighbours written", n);
}
int main(void)
{
    seed(0); wm_80073448(1);
    ASSERT_MSG(lw(0x8009C5ACu) == (500u << 12) && lw(0x8009C5B0u) == 0u && lw(0x8009C5B4u) == ((u32)-600 << 12),
               "match_third_row", "pos=(0x%08x,0x%08x,0x%08x)", lw(0x8009C5ACu), lw(0x8009C5B0u), lw(0x8009C5B4u));
    ASSERT_MSG(s_dff4_calls == 0 && lw(0x8009C584u) == 0xCAFEF00Du && lhu(0x8006EE68u) == 0u, "table_branch_only", "flag branch ran");
    canaries("match_third_row");

    seed(0); wm_80073448(3);
    ASSERT_MSG(lw(0x8009C5ACu) == (29947u << 12) && lw(0x8009C5B4u) == (11075u << 12), "match_first_row", "x=0x%08x z=0x%08x", lw(0x8009C5ACu), lw(0x8009C5B4u));

    seed(0); wm_80073448(-7);
    ASSERT_MSG(lw(0x8009C5ACu) == ((u32)-1234 << 12) && lw(0x8009C5B4u) == (8888u << 12), "match_negative_id", "x=0x%08x z=0x%08x", lw(0x8009C5ACu), lw(0x8009C5B4u));

    seed(0); wm_80073448(42);
    ASSERT_MSG(lw(0x8009C5ACu) == 0u && lw(0x8009C5B0u) == 0u && lw(0x8009C5B4u) == 0u, "no_match_zeros", "pos not zeroed");
    /* the row after the -1 terminator (id 1) must never be reached */
    seed(0); wm_80073448(1);
    ASSERT_MSG(lw(0x8009C5ACu) == (500u << 12), "terminator_respected", "matched past the -1 row");

    seed(0x2FFFu); wm_80073448(1);
    ASSERT_MSG(lhu(0x8006EE68u) == 0x0FFFu, "flag_cleared", "EE68=0x%04x", lhu(0x8006EE68u));
    ASSERT_MSG(s_dff4_calls == 1 && s_dff4_arg == 0x8009C5ACu, "dff4_call", "calls=%d arg=0x%08x", s_dff4_calls, s_dff4_arg);
    ASSERT_MSG(lw(0x8009C584u) == 0x4A5Bu, "ee66_copy", "C584=0x%08x", lw(0x8009C584u));
    ASSERT_MSG(lw(0x8009C5B4u) == 0xDEADBEEFu, "flag_branch_no_table", "table branch also ran");
    canaries("flag");

    if (s_failures) { fprintf(stderr, "W34C9 entry placement certificate: %d failure(s)\n", s_failures); return 1; }
    printf("W34C9 entry placement certificate PASS\n"); return 0;
}
