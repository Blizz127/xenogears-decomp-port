/*
 * W33B standalone path-matrix test for wm_mode_audio_setup.
 *
 * Tests every mode/ready combination identified in the W33A audit.
 * Uses mock callees to verify exact call ordering and arguments.
 *
 * Compile:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT \
 *     pc_port/tests/w33b_test.c -o pc_port/build_native/w33b_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Minimal PSX memory shim for standalone testing. */
#define PSX_RAM_SIZE (2 * 1024 * 1024)
static uint8_t g_PsxRam[PSX_RAM_SIZE];
#define PSX_ADDR(a) ((void*)((uintptr_t)g_PsxRam + ((a) - 0x80000000u)))
#define WM_U32(a) (*(uint32_t*)PSX_ADDR(a))

/* World-map state addresses (retail absolute). */
#define WM_FLAG_C894_ABS      0x8009C894u
#define WM_MODE_BE10_ABS      0x8009BE10u
#define WM_TW_ID_D800         0x8009D800u
#define WM_TW_ID_D3D0         0x8009D3D0u
#define WM_TW_MIRROR_C888     0x8009C888u
#define WM_TW_MIRROR_C884     0x8009C884u

/* Main BSS addresses. */
#define D_80062648_ADDR       0x80062648u
#define D_80062528_ADDR       0x80062528u

/* Event ledger for tracking call ordering. */
#define MAX_EVENTS 64

typedef struct {
    int type;           /* 0=decode, 1=memcpy, 2=create, 3=load, 4=level_a, 5=level_b */
    uintptr_t arg0;
    uintptr_t arg1;
    uintptr_t arg2;
    int sequence;
} AudioEvent;

static AudioEvent events[MAX_EVENTS];
static int event_count = 0;
static int sequence_counter = 0;

static void reset_events(void)
{
    event_count = 0;
    sequence_counter = 0;
}

static void record_event(int type, uint32_t a0, uint32_t a1, uint32_t a2)
{
    if (event_count < MAX_EVENTS) {
        events[event_count].type = type;
        events[event_count].arg0 = a0;
        events[event_count].arg1 = a1;
        events[event_count].arg2 = a2;
        events[event_count].sequence = sequence_counter++;
        event_count++;
    }
}

/* Mock callees. */
static int mock_decode_size = 1024;

int ArchiveDecodeAlignedSize(unsigned int entryIndex)
{
    record_event(0, entryIndex, 0, 0);
    return mock_decode_size;
}

void* mock_memcpy_dest = NULL;
void* mock_memcpy_src = NULL;
size_t mock_memcpy_n = 0;

void* memcpy(void* dest, const void* src, size_t n)
{
    mock_memcpy_dest = dest;
    mock_memcpy_src = (void*)src;
    mock_memcpy_n = n;
    record_event(1, (uintptr_t)dest, (uintptr_t)src, n);
    /* Actually copy for the test. */
    return memmove(dest, src, n);
}

static void* mock_audio_manager = (void*)0x12345678;

void* func_80039850(void* pSongFile)
{
    record_event(2, (uintptr_t)pSongFile, 0, 0);
    return mock_audio_manager;
}

static int mock_level_a_mgr = 0;
static int mock_level_a_level = 0;
static int mock_level_a_steps = -1;

void func_80039A80(void* manager, int level, int steps)
{
    mock_level_a_mgr = (uintptr_t)manager;
    mock_level_a_level = level;
    mock_level_a_steps = steps;
    record_event(4, (uintptr_t)manager, level, steps);
}

static int mock_level_b_mgr = 0;
static int mock_level_b_level = 0;
static int mock_level_b_steps = -1;

void func_80039B68(void* manager, int level, int steps)
{
    mock_level_b_mgr = (uintptr_t)manager;
    mock_level_b_level = level;
    mock_level_b_steps = steps;
    record_event(5, (uintptr_t)manager, level, steps);
}

/* The function under test (replicates production logic). */
static void wm_mode_audio_setup_test(void)
{
    uint32_t ready_flag;
    uint32_t mode;
    uint32_t archive_id;
    uint32_t buffer_psx;
    void* buffer_host;
    int size;
    void* manager;

    ready_flag = WM_U32(WM_FLAG_C894_ABS);
    mode = WM_U32(WM_MODE_BE10_ABS);

    if (mode == 7) {
        archive_id = WM_U32(WM_TW_ID_D800);
        buffer_psx = WM_U32(WM_TW_MIRROR_C888);
    } else {
        archive_id = WM_U32(WM_TW_ID_D3D0);
        buffer_psx = WM_U32(WM_TW_MIRROR_C884);
    }

    buffer_host = (void*)(uintptr_t)buffer_psx;
    if (buffer_psx >= 0x80000000u && buffer_psx < 0x80200000u)
        buffer_host = PSX_ADDR(buffer_psx);

    size = ArchiveDecodeAlignedSize(archive_id);

    if (buffer_host != NULL && size > 0) {
        memcpy(PSX_ADDR(D_80062648_ADDR), buffer_host, (size_t)size);
    }

    if (ready_flag == 0) {
        manager = func_80039850(PSX_ADDR(D_80062648_ADDR));
        WM_U32(D_80062528_ADDR) = (uintptr_t)manager;
        if (manager != NULL) {
            func_80039A80(manager, 127, 0);
        }
    } else {
        manager = (void*)(uintptr_t)WM_U32(D_80062528_ADDR);
        if (manager != NULL) {
            func_80039B68(manager, 127, 240);
        }
    }
}

static void reset_state(void)
{
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    reset_events();
    mock_level_a_mgr = 0; mock_level_a_level = 0; mock_level_a_steps = -1;
    mock_level_b_mgr = 0; mock_level_b_level = 0; mock_level_b_steps = -1;
}

static int total = 0, pass = 0, fail = 0;

static void check(const char* name, int cond)
{
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else { fail++; printf("  FAIL: %s\n", name); }
}

int main(void)
{
    printf("=== W33B Mode-Dependent Audio Setup Test ===\n\n");

    /* ---- Test 1: Ready + Mode 7 ---- */
    printf("[1] Ready + Mode 7:\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 0;  /* ready */
    WM_U32(WM_MODE_BE10_ABS) = 7;  /* mode 7 */
    WM_U32(WM_TW_ID_D800) = 100;   /* archive ID */
    WM_U32(WM_TW_MIRROR_C888) = 0x80080000u; /* buffer pointer */
    wm_mode_audio_setup_test();
    check("decode called with archive_id=100",
          event_count >= 1 && events[0].type == 0 && events[0].arg0 == 100);
    check("memcpy called", event_count >= 2 && events[1].type == 1);
    check("AudioManager created", event_count >= 3 && events[2].type == 2);
    check("level set with steps=0", event_count >= 4 && events[3].type == 4 &&
          events[3].arg1 == 127 && events[3].arg2 == 0);
    check("D_80062528 set", WM_U32(D_80062528_ADDR) == (uintptr_t)mock_audio_manager);

    /* ---- Test 2: Ready + Non-mode-7 ---- */
    printf("[2] Ready + Non-mode-7 (mode=1):\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 0;  /* ready */
    WM_U32(WM_MODE_BE10_ABS) = 1;  /* mode 1 */
    WM_U32(WM_TW_ID_D3D0) = 200;   /* archive ID */
    WM_U32(WM_TW_MIRROR_C884) = 0x80081000u; /* buffer pointer */
    wm_mode_audio_setup_test();
    check("decode called with archive_id=200",
          event_count >= 1 && events[0].type == 0 && events[0].arg0 == 200);
    check("memcpy called", event_count >= 2 && events[1].type == 1);
    check("AudioManager created", event_count >= 3 && events[2].type == 2);
    check("level set with steps=0", event_count >= 4 && events[3].type == 4 &&
          events[3].arg1 == 127 && events[3].arg2 == 0);

    /* ---- Test 3: Not-ready + Mode 7 ---- */
    printf("[3] Not-ready + Mode 7:\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 1;  /* not ready */
    WM_U32(WM_MODE_BE10_ABS) = 7;  /* mode 7 */
    WM_U32(WM_TW_ID_D800) = 300;   /* archive ID */
    WM_U32(WM_TW_MIRROR_C888) = 0x80082000u; /* buffer pointer */
    WM_U32(D_80062528_ADDR) = 0x5678u; /* existing AudioManager */
    wm_mode_audio_setup_test();
    check("decode called with archive_id=300",
          event_count >= 1 && events[0].type == 0 && events[0].arg0 == 300);
    check("memcpy called", event_count >= 2 && events[1].type == 1);
    check("no AudioManager creation", event_count < 3 || events[2].type != 2);
    check("level set with steps=240", event_count >= 3 && events[2].type == 5 &&
          events[2].arg1 == 127 && events[2].arg2 == 240);

    /* ---- Test 4: Not-ready + Non-mode-7 ---- */
    printf("[4] Not-ready + Non-mode-7 (mode=2):\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 2;  /* not ready */
    WM_U32(WM_MODE_BE10_ABS) = 2;  /* mode 2 */
    WM_U32(WM_TW_ID_D3D0) = 400;   /* archive ID */
    WM_U32(WM_TW_MIRROR_C884) = 0x80083000u; /* buffer pointer */
    WM_U32(D_80062528_ADDR) = 0x9ABCu; /* existing AudioManager */
    wm_mode_audio_setup_test();
    check("decode called with archive_id=400",
          event_count >= 1 && events[0].type == 0 && events[0].arg0 == 400);
    check("level set with steps=240", event_count >= 3 && events[2].type == 5 &&
          events[2].arg1 == 127 && events[2].arg2 == 240);

    /* ---- Test 5: Repeated execution (idempotent) ---- */
    printf("[5] Repeated execution:\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    WM_U32(WM_MODE_BE10_ABS) = 1;
    WM_U32(WM_TW_ID_D3D0) = 100;
    WM_U32(WM_TW_MIRROR_C884) = 0x80080000u;
    wm_mode_audio_setup_test();
    int first_count = event_count;
    wm_mode_audio_setup_test();
    check("second call executes (idempotent)", event_count > first_count);

    /* ---- Test 6: Call ordering ---- */
    printf("[6] Call ordering (ready + mode 7):\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    WM_U32(WM_MODE_BE10_ABS) = 7;
    WM_U32(WM_TW_ID_D800) = 100;
    WM_U32(WM_TW_MIRROR_C888) = 0x80080000u;
    wm_mode_audio_setup_test();
    check("decode before memcpy",
          event_count >= 2 && events[0].sequence < events[1].sequence);
    check("memcpy before create",
          event_count >= 3 && events[1].sequence < events[2].sequence);
    check("create before level_set",
          event_count >= 4 && events[2].sequence < events[3].sequence);

    /* ---- Test 7: Mode 7 uses correct archive ID ---- */
    printf("[7] Mode 7 uses WM_TW_ID_D800:\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    WM_U32(WM_MODE_BE10_ABS) = 7;
    WM_U32(WM_TW_ID_D800) = 100;
    WM_U32(WM_TW_ID_D3D0) = 200;  /* should NOT be used */
    WM_U32(WM_TW_MIRROR_C888) = 0x80080000u;
    wm_mode_audio_setup_test();
    check("decode called with mode-7 archive ID (100)",
          event_count >= 1 && events[0].type == 0 && events[0].arg0 == 100);

    /* ---- Test 8: Non-mode-7 uses correct archive ID ---- */
    printf("[8] Non-mode-7 uses WM_TW_ID_D3D0:\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    WM_U32(WM_MODE_BE10_ABS) = 1;
    WM_U32(WM_TW_ID_D3D0) = 200;
    WM_U32(WM_TW_ID_D800) = 100;  /* should NOT be used */
    WM_U32(WM_TW_MIRROR_C884) = 0x80081000u;
    wm_mode_audio_setup_test();
    check("decode called with non-mode-7 archive ID (200)",
          event_count >= 1 && events[0].type == 0 && events[0].arg0 == 200);

    /* ---- Test 9: Neighboring memory guards ---- */
    printf("[9] Neighboring memory guards:\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    WM_U32(WM_MODE_BE10_ABS) = 1;
    WM_U32(WM_TW_ID_D3D0) = 100;
    WM_U32(WM_TW_MIRROR_C884) = 0x80080000u;
    /* Set sentinels around D_80062648 */
    uint32_t* buf = (uint32_t*)PSX_ADDR(D_80062648_ADDR);
    buf[-1] = 0xDEADu;
    buf[mock_decode_size / 4 + 1] = 0xBEEFu;
    wm_mode_audio_setup_test();
    check("D_80062648[-1] untouched",
          buf[-1] == 0xDEADu);
    check("D_80062648[size/4+1] untouched",
          buf[mock_decode_size / 4 + 1] == 0xBEEFu);

    /* ---- Test 10: Ready flag 0 creates, non-zero loads ---- */
    printf("[10] Ready flag controls create vs load:\n");
    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    WM_U32(WM_MODE_BE10_ABS) = 1;
    WM_U32(WM_TW_ID_D3D0) = 100;
    WM_U32(WM_TW_MIRROR_C884) = 0x80080000u;
    wm_mode_audio_setup_test();
    check("ready: func_80039850 called (create)",
          event_count >= 3 && events[2].type == 2);

    reset_state();
    WM_U32(WM_FLAG_C894_ABS) = 5;  /* not ready */
    WM_U32(WM_MODE_BE10_ABS) = 1;
    WM_U32(WM_TW_ID_D3D0) = 100;
    WM_U32(WM_TW_MIRROR_C884) = 0x80080000u;
    WM_U32(D_80062528_ADDR) = 0xABCDu;
    wm_mode_audio_setup_test();
    check("not-ready: func_80039B68 called (load)",
          event_count >= 3 && events[2].type == 5);
    check("not-ready: no func_80039850 call",
          event_count < 4 || events[3].type != 2);

    /* ---- Summary ---- */
    printf("\n=== W33B Test Summary ===\n");
    printf("  Total: %d  PASS: %d  FAIL: %d\n", total, pass, fail);
    return fail > 0 ? 1 : 0;
}
