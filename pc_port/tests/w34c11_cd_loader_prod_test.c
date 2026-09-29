/* W34C11 — CD record-list loader (0x8009699C) + 0x800967E4 wiring certificate.
 * Oracle: retail record contract with a simulated disc image. */
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_966cc.h"
#include "world_map_helper_96130.h"
#include "world_map_helper_9623c.h"
#include "world_map_helper_96328.h"

static int s_failures;
#define ASSERT_MSG(c, n, ...) do { if (!(c)) { s_failures++; fprintf(stderr, "ASSERTION %s FAILED: ", n); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); } } while (0)
static void sw(u32 a, u32 v) { memcpy(PSX_ADDR(a), &v, 4); }
static u32 lw(u32 a) { u32 v; memcpy(&v, PSX_ADDR(a), 4); return v; }

/* ---- simulated CD ---- */
static int s_lba = -1; static int s_pending;
static u8 s_sector_byte(int lba, int i) { return (u8)((lba * 131 + i * 7 + (i >> 8)) & 0xFF); }
void* CdIntToPos(int i, void* p) { u8* l = (u8*)p; l[0] = (u8)(i & 0xFF); l[1] = (u8)((i >> 8) & 0xFF); l[2] = (u8)((i >> 16) & 0xFF); l[3] = 0; return p; }
int CdControl(u8 com, u8* param, u8* result) { (void)result; if (com == 2) s_lba = param[0] | (param[1] << 8) | (param[2] << 16); return 1; }
static u32* s_read_buf; static int s_read_n;
int CdRead(int sectors, u32* buf, int mode) { (void)mode; s_read_buf = buf; s_read_n = sectors; s_pending = 1; return 1; }
int CdReadSync(int mode, u8* result) { (void)mode; (void)result; if (!s_pending) return 0; for (int s = 0; s < s_read_n; s++) { u8* d = (u8*)s_read_buf + s * 2048; for (int i = 0; i < 2048; i++) d[i] = s_sector_byte(s_lba, i); s_lba++; } s_pending = 0; return 0; }
static u32 s_c3d8[2]; static int s_c3d8_n; u32 func_8002C3D8(void) { return s_c3d8[(s_c3d8_n++) & 1]; }
static int s_pc_calls; void wm_800966CC(u32 t) { (void)t; s_pc_calls++; }
int Vsync(int m) { (void)m; return 0; }
u32 wm_80096668_circular_distance(void)
{
    u32 head = lw(0x8009BE44u);
    u32 tail = lw(0x8009BCB8u);
    return (head - tail) & 0xFu;
}
/* PC-file loader externs (linked from 966cc.o, never exercised here) */
uintptr_t PCopen(char* n, int f, int p) { (void)n; (void)f; (void)p; return (uintptr_t)-1; }
int PClseek(uintptr_t fd, int o, int m) { (void)fd; (void)o; (void)m; return 0; }
int PCread(uintptr_t fd, char* b, int l) { (void)fd; (void)b; (void)l; return 0; }
int PCclose(uintptr_t fd) { (void)fd; return 0; }

#define LIST 0x800A0000u
#define BUF1 0x800B0000u
#define BUF2 0x800C0000u
#define BUF3 0x800D0000u
static void seed(void)
{
    PsxMemory_Init();
    /* records: {sector,size,buf}: 0x710 (partial), 0x1000 (two full), 0x900 (full + partial), terminator */
    u32 recs[] = { 0x4410Cu, 0x710u, BUF1,  0x50000u, 0x1000u, BUF2,  0x60001u, 0x900u, BUF3,  0u, 0u, 0u };
    for (u32 i = 0; i < 12u; i++) sw(LIST + i * 4u, recs[i]);
    memset(PSX_ADDR(BUF1), 0xA5, 0x1000); memset(PSX_ADDR(BUF2), 0xA5, 0x1100); memset(PSX_ADDR(BUF3), 0xA5, 0x1000);
    sw(0x8009D7D4u, 0x800E0000u); memset(PSX_ADDR(0x800E0000u), 0x5A, 0x800);
    sw(0x8009BCB8u, 5u); for (u32 i = 0; i < 16u; i++) sw(0x8009D788u + i * 4u, 0x11110000u + i);
    sw(0x8009D788u + 5u * 4u, LIST); sw(0x8009CD44u, 0u); s_pc_calls = 0;
}
static int buf_matches(u32 buf, int lba, u32 len, u32 off)
{ const u8* b = (const u8*)PSX_ADDR(buf); for (u32 i = 0; i < len; i++) if (b[i] != s_sector_byte(lba + (int)((off + i) >> 11), (int)((off + i) & 0x7FF))) return 0; return 1; }
static int canary(u32 a, u32 n, u8 v) { const u8* b = (const u8*)PSX_ADDR(a); for (u32 i = 0; i < n; i++) if (b[i] != v) return 0; return 1; }

int main(void)
{
    seed(); wm_8009699C(LIST);
    ASSERT_MSG(buf_matches(BUF1, 0x4410C, 0x710u, 0), "partial_record", "0x710-byte record bytes differ from sector 0x4410C");
    ASSERT_MSG(canary(BUF1 + 0x710u, 0x8F0u, 0xA5), "partial_no_overrun", "bytes past 0x710 written");
    /* The spill holds the LAST partial sector's remainder: record 3's second
     * sector (lba 0x60002) from byte 0x100 on. */
    { const u8* sp = (const u8*)PSX_ADDR(0x800E0000u); int ok = 1; for (u32 i = 0; i < 0x700u; i++) if (sp[i] != s_sector_byte(0x60002, 0x100 + (int)i)) ok = 0;
      ASSERT_MSG(ok, "spill", "remainder of the last partial sector not spilled to *0x8009D7D4"); }
    ASSERT_MSG(buf_matches(BUF2, 0x50000, 0x1000u, 0), "two_full_sectors", "0x1000-byte record wrong");
    ASSERT_MSG(canary(BUF2 + 0x1000u, 0x100u, 0xA5), "full_no_overrun", "bytes past 0x1000 written");
    ASSERT_MSG(buf_matches(BUF3, 0x60001, 0x900u, 0), "full_plus_partial", "0x900-byte record wrong");
    ASSERT_MSG(canary(BUF3 + 0x900u, 0x700u, 0xA5), "mixed_no_overrun", "bytes past 0x900 written");
    ASSERT_MSG(lw(0x8009D788u + 5u * 4u) == 0u && lw(0x8009BCB8u) == 6u, "completion", "D788[5]=0x%08x BCB8=%u", lw(0x8009D788u + 5u * 4u), lw(0x8009BCB8u));
    ASSERT_MSG(lw(0x8009D788u + 6u * 4u) == 0x11110006u, "other_entries_intact", "D788[6] touched");
    ASSERT_MSG(lw(0x8009CD44u) == 0u, "state_idle", "state=%u", lw(0x8009CD44u));

    /* 967E4 wiring: ready -> D788[BCB8] -> CD loader; not ready -> C624 -> PC loader. */
    seed(); s_c3d8[0] = 0; s_c3d8[1] = 0; s_c3d8_n = 0; sw(0x8009BE44u, 9u);
    wm_800967E4();
    ASSERT_MSG(buf_matches(BUF1, 0x4410C, 0x710u, 0) && lw(0x8009BCB8u) == 6u && s_pc_calls == 0, "ready_wiring", "ready path did not run the CD loader on D788[BCB8]");
    seed(); s_c3d8[0] = 7; s_c3d8[1] = 7; s_c3d8_n = 0; sw(0x8009C624u + 5u * 4u, 0x80070000u);
    wm_800967E4();
    ASSERT_MSG(s_pc_calls == 1 && lw(0x8009C624u + 5u * 4u) == 0u && lw(0x8009BCB8u) == 6u && canary(BUF1, 0x10u, 0xA5), "not_ready_wiring", "not-ready path wrong (pc=%d C624[5]=0x%08x BCB8=%u)", s_pc_calls, lw(0x8009C624u + 5u * 4u), lw(0x8009BCB8u));

    /* ---- 9623C -> 96328 -> 963E4: records are queued into block BE44, then
     * published to D788[BE44] sorted ascending by sector. ---- */
    PsxMemory_Init();
    sw(0x8009BE08u, 0x800A4000u); sw(0x8009BE44u, 3u); sw(0x8009D808u, 0u);
    for (u32 i = 0; i < 16u; i++) sw(0x8009D788u + i * 4u, 0u);
    { u32 blk = 0x800A4000u + 3u * 1056u;
      memset(PSX_ADDR(blk - 64u), 0xC3, 64); memset(PSX_ADDR(blk + 1056u), 0xC3, 64);
      wm_8009623C(0x50000u, 0x710u, 0x80100000u); wm_8009623C(0x44121u, 0x710u, 0x80100710u);
      wm_8009623C(0x60001u, 0x900u, 0x80100E20u); wm_8009623C(0x44120u, 0x710u, 0x80101720u);
      wm_8009623C(0x4FFFFu, 0x710u, 0x80101E30u); wm_8009623C(0u, 0u, 0u);
      ASSERT_MSG(lw(0x8009D808u) == 6u, "queue_count", "D808=%u", lw(0x8009D808u));
      s32 r = wm_80096328();
      ASSERT_MSG(r == 0 && lw(0x8009D788u + 3u * 4u) == blk && lw(0x8009BE44u) == 4u && lw(0x8009D808u) == 0u,
                 "publish", "r=%d D788[3]=0x%08x BE44=%u", r, lw(0x8009D788u + 3u * 4u), lw(0x8009BE44u));
      static const u32 want[5][3] = { {0x44120u,0x710u,0x80101720u}, {0x44121u,0x710u,0x80100710u}, {0x4FFFFu,0x710u,0x80101E30u}, {0x50000u,0x710u,0x80100000u}, {0x60001u,0x900u,0x80100E20u} };
      for (u32 i = 0; i < 5u; i++)
          ASSERT_MSG(lw(blk + i * 12u) == want[i][0] && lw(blk + i * 12u + 4u) == want[i][1] && lw(blk + i * 12u + 8u) == want[i][2],
                     "sorted_by_sector", "record %u = (%x,%x,%x)", i, lw(blk + i * 12u), lw(blk + i * 12u + 4u), lw(blk + i * 12u + 8u));
      ASSERT_MSG(lw(blk + 5u * 12u) == 0u, "terminator_kept", "terminator lost");
      ASSERT_MSG(canary(blk - 64u, 64u, 0xC3) && canary(blk + 1056u, 64u, 0xC3), "block_canary", "sort wrote outside the block");
      /* empty block: not published */
      sw(0x8009BE44u, 5u); r = wm_80096328();
      ASSERT_MSG(r == -1 && lw(0x8009D788u + 5u * 4u) == 0u && lw(0x8009BE44u) == 5u, "empty_not_published", "r=%d D788[5]=0x%08x", r, lw(0x8009D788u + 5u * 4u));
    }

    if (s_failures) { fprintf(stderr, "W34C11 CD loader certificate: %d failure(s)\n", s_failures); return 1; }
    printf("W34C11 CD loader certificate PASS\n"); return 0;
}
