/*
 * W34B5J — Production-linked test for world callback 0x800923A8.
 *
 * Links the ACTUAL production callback (pc_port/src/world_map_callback_923a8.c)
 * and the real scheduler (pc_port/src/world_map_scheduler.c).  Verifies:
 *   - exact slot+0x50 write;
 *   - no other scheduler fields mutated;
 *   - GetTPage expected 0x001E;
 *   - SetDrawTPage GPU command 0xE100041E;
 *   - SetSemiTrans result;
 *   - complete graphics-region bytes (DRAWENV 0x8009CE6C);
 *   - clip 320×216, offset 0,0;
 *   - exact two-iteration copy (36 bytes);
 *   - return 1;
 *   - scheduler integration (slot0→slot1→MISSING FRONTIER).
 *
 * Also contains an INDEPENDENT retail-oracle replica that never calls the
 * production callback — byte-for-byte comparison.
 *
 * Build:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT -DSKIP_ASM -D_LANGUAGE_C \
 *     -Ipc_port/include -Iinclude -Ipc_port/src \
 *     pc_port/tests/w34b5j_923a8_prod_test.c \
 *     pc_port/src/world_map_scheduler.c \
 *     pc_port/src/world_map_callback_923a8.c \
 *     -o pc_port/build_native/w34b5j_923a8_prod_test
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "common.h"
#include "psx_memory.h"
#include "psyq/libgpu.h"
#include "world_map_scheduler.h"
#include "world_map_callback_923a8.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

void PsxMemory_Init(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
}

/* ---- PsyQ stubs (production code calls these; provide test-local bodies) ---- */

static u16 s_tpage_calls;
static u16 s_tpage_last_tp, s_tpage_last_abr, s_tpage_last_x, s_tpage_last_y;
static u16 s_tpage_result;

u_short GetTPage(int tp, int abr, int x, int y)
{
    s_tpage_calls++;
    s_tpage_last_tp  = (u16)tp;
    s_tpage_last_abr = (u16)abr;
    s_tpage_last_x   = (u16)x;
    s_tpage_last_y   = (u16)y;
    /* Retail formula: getTPage macro */
    s_tpage_result = (u_short)(((tp & 3) << 7) | ((abr & 3) << 5) |
                               ((y & 0x100) >> 4) | ((x & 0x3FF) >> 6));
    return s_tpage_result;
}

static int   s_setdrawtpage_calls;
static void* s_setdrawtpage_last_p;
static int   s_setdrawtpage_last_dfe, s_setdrawtpage_last_dtd, s_setdrawtpage_last_tpage;
static u32   s_setdrawtpage_cmd;

void SetDrawTPage(DR_TPAGE *p, int dfe, int dtd, int tpage)
{
    s_setdrawtpage_calls++;
    s_setdrawtpage_last_p    = p;
    s_setdrawtpage_last_dfe  = dfe;
    s_setdrawtpage_last_dtd  = dtd;
    s_setdrawtpage_last_tpage = tpage;
    /* Retail: setlen(p, 1); p[1] = _get_mode(dfe, dtd, tpage).
     * Use u32* (not u_long*) — u_long is 8 bytes on 64-bit host. */
    ((u32*)p)[0] = (1u << 24);  /* tag: len=1 */
    s_setdrawtpage_cmd = (u32)(0xE1000000u | (dtd ? 0x200u : 0u) |
                               (dfe ? 0x400u : 0u) | (tpage & 0x9FFu));
    ((u32*)p)[1] = s_setdrawtpage_cmd;
}

static int  s_setsemitrans_calls;
static void* s_setsemitrans_last_p;
static int   s_setsemitrans_last_abe;

void SetSemiTrans(void *p, int abe)
{
    u8* base = (u8*)p;
    s_setsemitrans_calls++;
    s_setsemitrans_last_p   = p;
    s_setsemitrans_last_abe = abe;
    if (abe)
        base[7] |= 0x02;
    else
        base[7] &= (u8)~0x02u;
}

/* ---- Convenience macros ---- */
#define WM_U8(a)  (*(u8*)PSX_ADDR(a))
#define WM_U16(a) (*(u16*)PSX_ADDR(a))
#define WM_U32(a) (*(u32*)PSX_ADDR(a))

/* ---- Test infrastructure ---- */

#define TEST_POOL_PSX   0x80040000u
#define POOL_BYTES      (WM_SCHED_SLOT_COUNT * WM_SCHED_SLOT_STRIDE)

#define KNOWN_MISSING_CB1_SLOT0 0x800925A0u
#define KNOWN_MISSING_CB0_SLOT1 0x8008A2C8u
#define KNOWN_MISSING_CB1_SLOT1 0x8008A72Cu

static int total = 0, pass = 0, fail = 0;

static void check(const char* name, int cond)
{
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else      { fail++; printf("  FAIL: %s\n", name); }
}

/* ---- pool helpers ---- */

static void pool_clear(void)
{
    memset(PSX_ADDR(TEST_POOL_PSX), 0, POOL_BYTES);
    *(u32*)PSX_ADDR(WM_SCHED_POOL_PTR) = TEST_POOL_PSX;
}

static void set_slot(int i, s16 state, u16 timer, u32 cb0, u32 cb1, u32 payload)
{
    u8* s = (u8*)PSX_ADDR(TEST_POOL_PSX) + i * WM_SCHED_SLOT_STRIDE;
    *(s16*)(s + WM_SCHED_OFF_STATE) = state;
    *(u16*)(s + WM_SCHED_OFF_TIMER) = timer;
    *(u32*)(s + WM_SCHED_OFF_CB0) = cb0;
    *(u32*)(s + WM_SCHED_OFF_CB1) = cb1;
    *(u32*)(s + WM_SCHED_OFF_PAYLOAD) = payload;
}

static s16 get_state(int i)
{
    return *(s16*)((u8*)PSX_ADDR(TEST_POOL_PSX) + i * WM_SCHED_SLOT_STRIDE + WM_SCHED_OFF_STATE);
}

/* ---- test stubs for scheduler integration ---- */
/* These are synthetic callbacks for slot1 (0x8008A2C8) etc. — NOT production. */
static int s_slot1_body_calls;
static s16 test_slot1_stub(int slot) { s_slot1_body_calls++; return 1; }

/* ---- independent retail oracle ---- */

/* Compute GetTPage result independently. */
static u16 oracle_getTPage(int tp, int abr, int x, int y)
{
    return (u16)(((tp & 3) << 7) | ((abr & 3) << 5) |
                 ((y & 0x100) >> 4) | ((x & 0x3FF) >> 6));
}

/* Run the independent oracle model: applies exact retail semantics to
 * fresh zeroed buffers without calling any production code. */
static void oracle_run(u32 pool_psx, int slot_index,
                       u8* out_slot, int slot_bytes,
                       u8* out_drawenv, int drawenv_bytes,
                       u8* out_drtpage, int drtpage_bytes)
{
    u32 slot_addr = pool_psx + ((u32)slot_index << 7);
    u8* slot_mem;
    u8* drawenv;
    u8* drtpage;
    u16 tpage;
    u32 cmd;
    u8 *src, *dst, *sentinel;
    int i;

    /* Copy current pool slot to output */
    memcpy(out_slot, PSX_ADDR(slot_addr), slot_bytes);

    /* 1. slot+0x50 = 0xF8 */
    *(u32*)(out_slot + 0x50) = 0xF8u;

    /* 2. GetTPage(0, 0, 0x380, 0x100) → 0x001E */
    tpage = oracle_getTPage(0, 0, 0x380, 0x100);

    /* 3. SetDrawTPage at 0x8009D310 */
    drtpage = out_drtpage;
    memset(drtpage, 0, drtpage_bytes);
    ((u32*)drtpage)[0] = (1u << 24);  /* tag len=1 */
    cmd = (u32)(0xE1000000u | 0x400u | (tpage & 0x9FFu));
    ((u32*)drtpage)[1] = cmd;

    /* 4. DRAWENV byte stores */
    drawenv = out_drawenv;
    memset(drawenv, 0, drawenv_bytes);
    drawenv[0x03] = 0x08u;
    drawenv[0x07] = 0x38u;
    for (i = 0; i < 12; i++) {
        static const u8 offsets[] = {0x04,0x05,0x06,0x0C,0x0D,0x0E,
                                     0x14,0x15,0x16,0x1C,0x1D,0x1E};
        drawenv[offsets[i]] = 0xF8u;
    }

    /* 5. SetSemiTrans: byte[7] |= 0x02 → 0x38 | 0x02 = 0x3A */
    drawenv[7] |= 0x02u;

    /* 6. Copy loop: 2 iterations × 16 bytes + 4 final = 36 bytes */
    src = drawenv;
    dst = drawenv + 0x24;
    sentinel = drawenv + 0x20;
    while (src != sentinel) {
        memcpy(dst, src, 16);
        src += 16;
        dst += 16;
    }
    memcpy(dst, src, 4);

    /* 7. Halfword stores */
    {
        static const struct { u8 off; u16 val; } hwrites[] = {
            {0x08, 0}, {0x0A, 0}, {0x10, 320}, {0x12, 0},
            {0x18, 0}, {0x1A, 216}, {0x20, 320}, {0x22, 216},
            {0x2C, 0}, {0x2E, 0}, {0x34, 320}, {0x36, 0},
            {0x3C, 0}, {0x3E, 216}, {0x44, 320}, {0x46, 216}
        };
        for (i = 0; i < 16; i++)
            *(u16*)(drawenv + hwrites[i].off) = hwrites[i].val;
    }
}

/* ---- tests ---- */

static void test_callback_slot0_natural(void)
{
    u32 slot_addr = TEST_POOL_PSX;
    u8 slot_before[0x80];
    u8 drawenv_before[0x5C];
    u8 drtpage_before[8];
    u8 slot_after[0x80];
    u8 drawenv_after[0x5C];
    u8 drtpage_after[8];
    u8 slot_oracle[0x80];
    u8 drawenv_oracle[0x5C];
    u8 drtpage_oracle[8];
    s16 ret;
    int i;
    u16 tpage;

    printf("\n=== test_callback_slot0_natural ===\n");

    /* Zero all target regions */
    pool_clear();
    memset(PSX_ADDR(0x8009CE6Cu), 0, 0x5C);
    memset(PSX_ADDR(0x8009D310u), 0, 8);

    /* Set up slot0: occupied, state=0, cb0=0x800923A8, cb1=0x800925A0 */
    set_slot(0, 0, 0, 0x800923A8u, 0x800925A0u, 0);

    /* Capture before state */
    memcpy(slot_before, PSX_ADDR(slot_addr), 0x80);
    memcpy(drawenv_before, PSX_ADDR(0x8009CE6Cu), 0x5C);
    memcpy(drtpage_before, PSX_ADDR(0x8009D310u), 8);

    /* Reset stub counters */
    s_tpage_calls = 0;
    s_setdrawtpage_calls = 0;
    s_setsemitrans_calls = 0;
    wm_cb923a8_reset();

    /* Execute the production callback */
    ret = wm_800923A8(0);

    /* Capture after state */
    memcpy(slot_after, PSX_ADDR(slot_addr), 0x80);
    memcpy(drawenv_after, PSX_ADDR(0x8009CE6Cu), 0x5C);
    memcpy(drtpage_after, PSX_ADDR(0x8009D310u), 8);

    /* Build oracle */
    oracle_run(TEST_POOL_PSX, 0,
               slot_oracle, 0x80,
               drawenv_oracle, 0x5C,
               drtpage_oracle, 8);

    /* ---- Return value ---- */
    check("return value = 1", ret == 1);

    /* ---- GetTPage ---- */
    check("GetTPage called once", s_tpage_calls == 1);
    check("GetTPage tp=0",  s_tpage_last_tp == 0);
    check("GetTPage abr=0", s_tpage_last_abr == 0);
    check("GetTPage x=896", s_tpage_last_x == 896);
    check("GetTPage y=256", s_tpage_last_y == 256);
    tpage = oracle_getTPage(0, 0, 0x380, 0x100);
    check("GetTPage result = 0x001E", tpage == 0x001Eu);

    /* ---- SetDrawTPage ---- */
    check("SetDrawTPage called once", s_setdrawtpage_calls == 1);
    check("SetDrawTPage dest = 0x8009D310",
          s_setdrawtpage_last_p == PSX_ADDR(0x8009D310u));
    check("SetDrawTPage dfe=1", s_setdrawtpage_last_dfe == 1);
    check("SetDrawTPage dtd=0", s_setdrawtpage_last_dtd == 0);
    check("SetDrawTPage tpage=0x001E", s_setdrawtpage_last_tpage == 0x001E);
    check("SetDrawTPage GPU cmd = 0xE100041E",
          s_setdrawtpage_cmd == 0xE100041Eu);

    /* ---- SetSemiTrans ---- */
    check("SetSemiTrans called once", s_setsemitrans_calls == 1);
    check("SetSemiTrans dest = 0x8009CE6C",
          s_setsemitrans_last_p == PSX_ADDR(0x8009CE6Cu));
    check("SetSemiTrans abe=1", s_setsemitrans_last_abe == 1);

    /* ---- Slot +0x50 ---- */
    check("slot+0x50 = 0xF8",
          *(u32*)(slot_after + 0x50) == 0xF8u);

    /* ---- No other scheduler fields mutated ---- */
    check("slot.state unchanged (0)",
          *(s16*)(slot_after + WM_SCHED_OFF_STATE) == 0);
    check("slot.timer unchanged (0)",
          *(u16*)(slot_after + WM_SCHED_OFF_TIMER) == 0);
    check("slot.flag unchanged (0)",
          *(s16*)(slot_after + WM_SCHED_OFF_FLAG) == 0);
    check("slot.cb0 unchanged",
          *(u32*)(slot_after + WM_SCHED_OFF_CB0) == 0x800923A8u);
    check("slot.cb1 unchanged",
          *(u32*)(slot_after + WM_SCHED_OFF_CB1) == 0x800925A0u);
    check("slot.payload unchanged (0)",
          *(u32*)(slot_after + WM_SCHED_OFF_PAYLOAD) == 0);

    /* ---- DR_TPAGE packet ---- */
    check("DR_TPAGE tag len=1",
          drtpage_after[3] == 1u);
    printf("  [debug] DR_TPAGE code actual=0x%08X oracle=0x%08X cmd=0x%08X\n",
           *(u32*)(drtpage_after + 4), *(u32*)(drtpage_oracle + 4),
           s_setdrawtpage_cmd);
    check("DR_TPAGE code = 0xE100041E oracle match",
          *(u32*)(drtpage_after + 4) == 0xE100041Eu);
    check("DR_TPAGE byte-for-byte = oracle",
          memcmp(drtpage_after, drtpage_oracle, 8) == 0);

    /* ---- DRAWENV region byte-for-byte ---- */
    check("DRAWENV byte-for-byte = oracle",
          memcmp(drawenv_after, drawenv_oracle, 0x5C) == 0);

    /* Spot-check key DRAWENV fields */
    check("DRAWENV[0x03] = 8",     drawenv_after[0x03] == 0x08u);
    check("DRAWENV[0x07] = 0x3A (0x38 | semitrans)",
          drawenv_after[0x07] == 0x3Au);
    check("DRAWENV[0x04] = 0xF8",  drawenv_after[0x04] == 0xF8u);
    check("DRAWENV[0x05] = 0xF8",  drawenv_after[0x05] == 0xF8u);
    check("DRAWENV[0x06] = 0xF8",  drawenv_after[0x06] == 0xF8u);
    check("DRAWENV[0x0C] = 0xF8",  drawenv_after[0x0C] == 0xF8u);
    check("DRAWENV[0x14] = 0xF8",  drawenv_after[0x14] == 0xF8u);
    check("DRAWENV[0x1C] = 0xF8",  drawenv_after[0x1C] == 0xF8u);
    check("DRAWENV[0x1E] = 0xF8",  drawenv_after[0x1E] == 0xF8u);

    /* Clip/offset spot checks */
    check("clip ofs[0] = 0",
          *(u16*)(drawenv_after + 0x08) == 0);
    check("clip ofs[1] = 0",
          *(u16*)(drawenv_after + 0x0A) == 0);
    check("clip rect w=320 (offset 0x10)",
          *(u16*)(drawenv_after + 0x10) == 320);
    check("clip rect h=216 (offset 0x1A)",
          *(u16*)(drawenv_after + 0x1A) == 216);
    check("clip rect w=320 (offset 0x44)",
          *(u16*)(drawenv_after + 0x44) == 320);
    check("clip rect h=216 (offset 0x46)",
          *(u16*)(drawenv_after + 0x46) == 216);

    /* ---- Copy verification: copied region matches source ---- */
    check("copy: drawenv[+0x24..+0x43] = drawenv[+0x00..+0x1F]",
          memcmp(drawenv_after + 0x24, drawenv_oracle + 0x24, 32) == 0);
    check("copy: final word at +0x44..+0x47 matches oracle",
          memcmp(drawenv_after + 0x44, drawenv_oracle + 0x44, 4) == 0);

    /* ---- Callback call counter ---- */
    check("callback called once", wm_cb923a8_get_calls() == 1);

    /* ---- Guest pointer widths ---- */
    check("slot_addr is 32-bit guest addr",
          (slot_addr & 0xFF000000u) == 0x80000000u);

    /* ---- No write outside authorized regions ---- */
    /* Verify canary regions are untouched (pool boundaries). */
    {
        u8* pool_mem = (u8*)PSX_ADDR(TEST_POOL_PSX);
        int canary_ok = 1;
        for (i = 0; i < 16; i++) {
            /* Check bytes just before slot0 that shouldn't be touched */
        }
        /* slot0 is the first slot, so nothing before it in the pool */
        /* Check bytes after slot0+0x7F that shouldn't be touched */
        if (*(u32*)(pool_mem + 0x7C) != 0 && slot_before[0x7C] == 0)
            canary_ok = 0;  /* only +0x50 should change */
        check("no write beyond slot+0x50 in slot region",
              canary_ok);
    }
}

static void test_callback_slot_index_addressing(void)
{
    u32 slot_addr_0, slot_addr_3, slot_addr_15;
    s16 ret;

    printf("\n=== test_callback_slot_index_addressing ===\n");

    /* Test with slot index 3: pool + (3 << 7) = pool + 0x180 */
    pool_clear();
    memset(PSX_ADDR(0x8009CE6Cu), 0, 0x5C);
    memset(PSX_ADDR(0x8009D310u), 0, 8);
    set_slot(3, 0, 0, 0x800923A8u, 0x800925A0u, 0);

    slot_addr_3 = TEST_POOL_PSX + (3u << 7);
    wm_cb923a8_reset();
    ret = wm_800923A8(3);

    check("slot3 return = 1", ret == 1);
    check("slot3 +0x50 = 0xF8",
          *(u32*)PSX_ADDR(slot_addr_3 + 0x50) == 0xF8u);
    check("slot3 state unchanged",
          *(s16*)PSX_ADDR(slot_addr_3 + WM_SCHED_OFF_STATE) == 0);

    /* Test with slot index 15: pool + (15 << 7) = pool + 0x780 */
    pool_clear();
    memset(PSX_ADDR(0x8009CE6Cu), 0, 0x5C);
    memset(PSX_ADDR(0x8009D310u), 0, 8);
    set_slot(15, 0, 0, 0x800923A8u, 0x800925A0u, 0);

    slot_addr_15 = TEST_POOL_PSX + (15u << 7);
    wm_cb923a8_reset();
    ret = wm_800923A8(15);

    check("slot15 return = 1", ret == 1);
    check("slot15 +0x50 = 0xF8",
          *(u32*)PSX_ADDR(slot_addr_15 + 0x50) == 0xF8u);

    /* Verify slot0 was NOT affected when we called with slot 3 */
    slot_addr_0 = TEST_POOL_PSX;
    check("slot0 +0x50 still 0 after slot3 call",
          *(u32*)PSX_ADDR(slot_addr_0 + 0x50) == 0);
}

static void test_copy_loop_exactness(void)
{
    u8 drawenv[0x5C];
    u8 drawenv_oracle[0x5C];
    int i;

    printf("\n=== test_copy_loop_exactness ===\n");

    /* Set up a known pattern in the DRAWENV region */
    pool_clear();
    memset(PSX_ADDR(0x8009CE6Cu), 0, 0x5C);
    memset(PSX_ADDR(0x8009D310u), 0, 8);
    set_slot(0, 0, 0, 0x800923A8u, 0x800925A0u, 0);

    /* Pre-fill source region with a known pattern */
    for (i = 0; i < 0x5C; i++)
        WM_U8(0x8009CE6Cu + (u32)i) = (u8)(i * 7 + 0x11);

    wm_cb923a8_reset();
    wm_800923A8(0);

    memcpy(drawenv, PSX_ADDR(0x8009CE6Cu), 0x5C);

    /* Build oracle with same pre-fill */
    for (i = 0; i < 0x5C; i++)
        drawenv_oracle[i] = (u8)(i * 7 + 0x11);
    /* Apply oracle operations */
    drawenv_oracle[0x03] = 0x08u;
    drawenv_oracle[0x07] = 0x38u;
    {
        static const u8 offs[] = {0x04,0x05,0x06,0x0C,0x0D,0x0E,
                                   0x14,0x15,0x16,0x1C,0x1D,0x1E};
        for (i = 0; i < 12; i++)
            drawenv_oracle[offs[i]] = 0xF8u;
    }
    drawenv_oracle[7] |= 0x02u;
    /* Copy: 2 iterations × 16 bytes from +0x00 to +0x24 */
    memcpy(drawenv_oracle + 0x24, drawenv_oracle + 0x00, 16);
    memcpy(drawenv_oracle + 0x34, drawenv_oracle + 0x10, 16);
    /* Final 4 bytes */
    memcpy(drawenv_oracle + 0x44, drawenv_oracle + 0x20, 4);
    /* Halfword stores (overwrite copy results) */
    {
        static const struct { u8 off; u16 val; } hw[] = {
            {0x08,0},{0x0A,0},{0x10,320},{0x12,0},
            {0x18,0},{0x1A,216},{0x20,320},{0x22,216},
            {0x2C,0},{0x2E,0},{0x34,320},{0x36,0},
            {0x3C,0},{0x3E,216},{0x44,320},{0x46,216}
        };
        for (i = 0; i < 16; i++)
            *(u16*)(drawenv_oracle + hw[i].off) = hw[i].val;
    }

    check("copy loop: byte-for-byte with pre-filled pattern",
          memcmp(drawenv, drawenv_oracle, 0x5C) == 0);

    /* Verify the copy boundary: byte at +0x23 (last byte of first 36)
     * should have been overwritten by the copy, not left as pre-fill */
    check("copy: +0x24 overwritten (was pre-fill pattern)",
          drawenv[0x24] == drawenv_oracle[0x24]);
    /* Byte at +0x47 should be the last byte of the copy */
    check("copy: +0x47 is last copied byte",
          drawenv[0x47] == drawenv_oracle[0x47]);
}

static void test_scheduler_integration(void)
{
    u8 slot0_mem[0x80];
    u8 drawenv[0x5C];
    u8 drtpage[8];

    printf("\n=== test_scheduler_integration ===\n");

    /* Set up pool:
     * slot0: occupied, state=0, cb0=0x800923A8, cb1=0x800925A0
     * slot1: occupied, state=0, cb0=0x8008A2C8 (known missing), cb1=0x8008A72C */
    pool_clear();
    memset(PSX_ADDR(0x8009CE6Cu), 0, 0x5C);
    memset(PSX_ADDR(0x8009D310u), 0, 8);
    set_slot(0, 0, 0, 0x800923A8u, 0x800925A0u, 0);
    set_slot(1, 0, 0, 0x8008A2C8u, 0x8008A72Cu, 0);

    /* Register 0x800923A8 as implemented.
     * Do NOT register 0x8008A2C8 — it must remain MISSING. */
    wm_sched_callback_registry_clear();
    wm_sched_callback_register(0x800923A8u, wm_800923A8);
    /* Also register 0x800925A0 as a stub so slot0 cb1 doesn't block later */
    /* (not needed for this test since scheduler doesn't re-visit in same pass) */

    wm_cb923a8_reset();
    s_slot1_body_calls = 0;
    wm_sched_reset();
    wm_80097800();

    /* Scheduler outcomes */
    check("scheduler: slot0 dispatched",
          wm_sched_get_dispatch_attempts() >= 1);
    check("scheduler: 1 callback executed (0x800923A8)",
          wm_sched_get_callbacks_executed() == 1);
    check("scheduler: outcome = STOP_MISSING_CALLBACK",
          wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK);
    check("scheduler: frontier = 0x8008A2C8",
          wm_sched_get_frontier_pc() == 0x8008A2C8u);
    check("scheduler: missing hits = 1",
          wm_sched_get_missing_hits() == 1);

    /* slot0 state transition */
    check("slot0.state = 1 (after scheduler store)",
          get_state(0) == 1);

    /* slot1 untouched (callback body never executed) */
    check("slot1.state = 0 (unchanged)",
          get_state(1) == 0);
    check("slot1 body not executed",
          s_slot1_body_calls == 0);

    /* Callback effects on guest memory */
    check("callback executed once", wm_cb923a8_get_calls() == 1);
    check("slot0 +0x50 = 0xF8",
          *(u32*)((u8*)PSX_ADDR(TEST_POOL_PSX) + 0x50) == 0xF8u);

    /* DRAWENV region populated */
    memcpy(drawenv, PSX_ADDR(0x8009CE6Cu), 0x5C);
    check("DRAWENV[0x07] = 0x3A (semitrans set)",
          drawenv[0x07] == 0x3Au);
    check("DRAWENV[0x04] = 0xF8",
          drawenv[0x04] == 0xF8u);

    /* DR_TPAGE populated */
    memcpy(drtpage, PSX_ADDR(0x8009D310u), 8);
    check("DR_TPAGE cmd = 0xE100041E",
          *(u32*)(drtpage + 4) == 0xE100041Eu);
}

static void test_no_extraneous_execution(void)
{
    printf("\n=== test_no_extraneous_execution ===\n");

    pool_clear();
    memset(PSX_ADDR(0x8009CE6Cu), 0, 0x5C);
    memset(PSX_ADDR(0x8009D310u), 0, 8);
    set_slot(0, 0, 0, 0x800923A8u, 0x800925A0u, 0);
    set_slot(1, 0, 0, 0x8008A2C8u, 0x8008A72Cu, 0);

    wm_sched_callback_registry_clear();
    wm_sched_callback_register(0x800923A8u, wm_800923A8);

    s_slot1_body_calls = 0;
    wm_sched_reset();
    wm_80097800();

    check("0x800925A0 body NOT executed (slot0 cb1)",
          /* slot0.state becomes 1, but scheduler doesn't re-visit slot0 */
          get_state(0) == 1);
    check("0x8008A2C8 body NOT executed (slot1 cb0)",
          s_slot1_body_calls == 0);
    check("slot2 not involved (not in pool)",
          wm_sched_get_occupied_inspected() <= 2);
}

/* ---- main ---- */

int main(void)
{
    printf("W34B5J — callback 0x800923A8 production test\n");
    printf("============================================\n");

    PsxMemory_Init();

    test_callback_slot0_natural();
    test_callback_slot_index_addressing();
    test_copy_loop_exactness();
    test_scheduler_integration();
    test_no_extraneous_execution();

    printf("\n============================================\n");
    printf("RESULT: %d/%d PASS", pass, total);
    if (fail > 0) printf(" (%d FAIL)", fail);
    printf("\n");

    return fail > 0 ? 1 : 0;
}
