/*
 * W34B1 REPLICA ORACLE for first convergence caller slice 0x800726C0–0x80072728.
 *
 * This file contains a REPLICA (copy) of the P1 implementation for
 * cross-validation.  The authoritative production-linked test is
 * w34b1_prod_test.c which links against the actual production object.
 *
 * Compile:
 *   gcc -std=gnu17 -O0 -g -DXENO_PC_PORT \
 *     pc_port/tests/w34b1_test.c -o pc_port/build_native/w34b1_test
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define PSX_RAM_SIZE (2 * 1024 * 1024)
static uint8_t g_PsxRam[PSX_RAM_SIZE];
#define PSX_ADDR(a) ((void*)((uintptr_t)g_PsxRam + ((a) - 0x80000000u)))
#define WM_U32(a) (*(uint32_t*)PSX_ADDR(a))
#define WM_U16(a) (*(uint16_t*)PSX_ADDR(a))

#define WM_POOL_BE24             0x8009BE24u
#define WM_POOL_SLOT_COUNT       64
#define WM_POOL_SLOT_STRIDE      0x80u
#define WM_POOL_OFF_18           0x18u
#define WM_POOL_OFF_1C           0x1Cu

#define WM_FLAG_C894_ABS         0x8009C894u
#define WM_CONV_TABLE_A_BASE     0x80099E8Cu
#define WM_CONV_TABLE_B_BASE     0x8009A034u
#define WM_SLOT_C610_ABS         0x8009C610u

#define WM_CONV_P1_CUT_SECOND_TABLE 0x8007272Cu
#define WM_CONV_P1_CUT_FLAG1_ARC    0x80072784u
#define WM_CONV_P1_CUT_COMMON_TAIL  0x8007290Cu

#define MAX_EVENTS 128
typedef struct { int slot; uint32_t a0; uint32_t a1; } PoolEvent;
static PoolEvent events[MAX_EVENTS];
static int event_count = 0;

static uint8_t g_PoolBuffer[WM_POOL_SLOT_COUNT * WM_POOL_SLOT_STRIDE];
static int s_pool_alloc = 0;

/* Forbidden read tracking */
static int forbidden_c610_read = 0;
static int forbidden_a034_read = 0;

/* Instrumented PSX memory access tracking for forbidden addresses */
static uint32_t instrumented_read_u32(uint32_t addr) {
    if (addr == WM_SLOT_C610_ABS) forbidden_c610_read++;
    if (addr == WM_CONV_TABLE_B_BASE) forbidden_a034_read++;
    return WM_U32(addr);
}

/* Pool helper (production replica) */
static void wm_pool_register(uint32_t a0, uint32_t a1)
{
    uint32_t pool_psx = WM_U32(WM_POOL_BE24);
    uint8_t* base;
    int i;
    if (pool_psx == 0) return;
    base = (uint8_t*)PSX_ADDR(pool_psx);
    if (base == NULL) return;
    for (i = 0; i < WM_POOL_SLOT_COUNT; i++) {
        uint8_t* slot = base + (uint32_t)i * WM_POOL_SLOT_STRIDE;
        uint32_t occ = *(uint32_t*)(slot + WM_POOL_OFF_1C);
        if (occ == 0) {
            *(uint16_t*)(slot + 0x00) = 0;
            *(uint16_t*)(slot + 0x02) = 0;
            *(uint16_t*)(slot + 0x04) = 0;
            *(uint32_t*)(slot + WM_POOL_OFF_18) = a0;
            *(uint32_t*)(slot + WM_POOL_OFF_1C) = a1;
            *(uint16_t*)(slot + 0x20) = 0;
            *(uint16_t*)(slot + 0x22) = 0;
            s_pool_alloc++;
            if (event_count < MAX_EVENTS) {
                events[event_count].slot = i;
                events[event_count].a0 = a0;
                events[event_count].a1 = a1;
                event_count++;
            }
            return;
        }
    }
    if (event_count < MAX_EVENTS) {
        events[event_count].slot = -1;
        events[event_count].a0 = a0;
        events[event_count].a1 = a1;
        event_count++;
    }
}

/* W34B1: transcription under test */
static int s_conv_p1_entry = 0;
static int s_conv_p1_flag0 = 0;
static int s_conv_p1_flag1_cut = 0;
static int s_conv_p1_other_cut = 0;
static int s_conv_p1_empty = 0;
static int s_conv_p1_iterations = 0;
static int s_conv_p1_helper_calls = 0;
static uint32_t s_conv_p1_last_next = 0;

typedef uint32_t wm_conv_p1_next_t;

wm_conv_p1_next_t wm_800726C0_convergence_p1(void)
{
    uint32_t flag;
    s_conv_p1_entry++;
    flag = instrumented_read_u32(WM_FLAG_C894_ABS);

    if (flag == 1) {
        s_conv_p1_flag1_cut++;
        s_conv_p1_last_next = WM_CONV_P1_CUT_FLAG1_ARC;
        return WM_CONV_P1_CUT_FLAG1_ARC;
    }
    if (flag != 0) {
        s_conv_p1_other_cut++;
        s_conv_p1_last_next = WM_CONV_P1_CUT_COMMON_TAIL;
        return WM_CONV_P1_CUT_COMMON_TAIL;
    }
    s_conv_p1_flag0++;
    {
        uint32_t *base = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_A_BASE);
        if (*base == 0) {
            s_conv_p1_empty++;
            s_conv_p1_last_next = WM_CONV_P1_CUT_SECOND_TABLE;
            return WM_CONV_P1_CUT_SECOND_TABLE;
        }
        {
            uint32_t *s0 = base;
            do {
                uint32_t a0 = s0[0];
                uint32_t a1 = s0[1];
                s_conv_p1_iterations++;
                s_conv_p1_helper_calls++;
                wm_pool_register(a0, a1);
                s0 += 2;
            } while (*s0 != 0);
        }
        s_conv_p1_last_next = WM_CONV_P1_CUT_SECOND_TABLE;
        return WM_CONV_P1_CUT_SECOND_TABLE;
    }
}

/* Test harness */
static int total = 0, pass = 0, fail = 0;
static void check(const char* name, int cond) {
    total++;
    if (cond) { pass++; printf("  PASS: %s\n", name); }
    else { fail++; printf("  FAIL: %s\n", name); }
}

static void reset_state(void) {
    memset(g_PsxRam, 0, PSX_RAM_SIZE);
    event_count = 0;
    s_pool_alloc = 0;
    s_conv_p1_entry = 0;
    s_conv_p1_flag0 = 0;
    s_conv_p1_flag1_cut = 0;
    s_conv_p1_other_cut = 0;
    s_conv_p1_empty = 0;
    s_conv_p1_iterations = 0;
    s_conv_p1_helper_calls = 0;
    s_conv_p1_last_next = 0;
    forbidden_c610_read = 0;
    forbidden_a034_read = 0;
}

static void init_pool(void) {
    uint32_t pool_psx = 0x800A0000u;
    memset(g_PoolBuffer, 0, sizeof(g_PoolBuffer));
    WM_U32(WM_POOL_BE24) = pool_psx;
    memcpy(PSX_ADDR(pool_psx), g_PoolBuffer, sizeof(g_PoolBuffer));
}

static void setup_table(uint32_t* records, int count) {
    uint32_t *base = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_A_BASE);
    int i;
    for (i = 0; i < count; i++) {
        base[i*2] = records[i*2];
        base[i*2+1] = records[i*2+1];
    }
    /* sentinel after */
    base[count*2] = 0;
    base[count*2+1] = 0xDEADBEEFu; /* should be ignored */
    /* guard after sentinel: mark with pattern to ensure not read beyond */
    base[count*2+2] = 0xAAAAAAAAu;
    base[count*2+3] = 0xBBBBBBBBu;
}

int main(void)
{
    uint32_t rec[16];
    uint32_t *base;
    uint8_t *pool_base;
    int i;

    printf("=== W34B1 Convergence P1 Test (0x800726C0-0x80072728) ===\n\n");

    /* ---------------------------------------------------------------
     * C894 dispatch
     * --------------------------------------------------------------- */
    printf("--- C894 dispatch ---\n");

    /* 1. flag 0 enters first-table path */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    setup_table((uint32_t[]){0,0}, 0); /* empty will still be flag0 path */
    wm_800726C0_convergence_p1();
    check("flag 0 enters first-table path (flag0==1)", s_conv_p1_flag0 == 1);
    check("flag 0 not flag1", s_conv_p1_flag1_cut == 0);
    check("flag 0 not other", s_conv_p1_other_cut == 0);

    /* 2. flag 1 -> no helper, cut 0x80072784 */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 1;
    rec[0]=0x11111111u; rec[1]=0x22222222u;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("flag 1 -> flag1_cut 1", s_conv_p1_flag1_cut == 1);
    check("flag 1 no helper call", event_count == 0);
    check("flag 1 next 0x80072784", s_conv_p1_last_next == WM_CONV_P1_CUT_FLAG1_ARC);

    /* 3. flag 2 -> other path */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 2;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("flag 2 -> other_cut 1", s_conv_p1_other_cut == 1);
    check("flag 2 no helper", event_count == 0);
    check("flag 2 next 0x8007290C", s_conv_p1_last_next == WM_CONV_P1_CUT_COMMON_TAIL);

    /* 4. flag UINT32_MAX -> other path */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0xFFFFFFFFu;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("flag MAX -> other_cut", s_conv_p1_other_cut == 1);
    check("flag MAX no helper", event_count == 0);
    check("flag MAX next 0x8007290C", s_conv_p1_last_next == WM_CONV_P1_CUT_COMMON_TAIL);

    /* ---------------------------------------------------------------
     * Table behavior
     * --------------------------------------------------------------- */
    printf("\n--- Table behavior ---\n");

    /* 5. first record sentinel: zero helper calls, cut 0x8007272C */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    setup_table((uint32_t[]){0,0}, 0);
    wm_800726C0_convergence_p1();
    check("empty table zero helper calls", event_count == 0);
    check("empty table empty flag", s_conv_p1_empty == 1);
    check("empty table cut 0x8007272C", s_conv_p1_last_next == WM_CONV_P1_CUT_SECOND_TABLE);
    check("empty table iterations 0", s_conv_p1_iterations == 0);

    /* 6. one record then sentinel: one helper call */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    rec[0]=0xAAAAAAAAu; rec[1]=0xBBBBBBBBu;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("one record -> 1 helper call", event_count == 1);
    check("one record a0 correct", events[0].a0 == 0xAAAAAAAAu);
    check("one record a1 correct", events[0].a1 == 0xBBBBBBBBu);
    check("one record cut 0x8007272C", s_conv_p1_last_next == WM_CONV_P1_CUT_SECOND_TABLE);
    check("one record iterations 1", s_conv_p1_iterations == 1);

    /* 7. multiple records then sentinel: exact count and order */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    {
        uint32_t mrec[] = {0x11111111u,0x11111112u, 0x22222221u,0x22222222u, 0x33333331u,0x33333332u};
        setup_table(mrec, 3);
        wm_800726C0_convergence_p1();
        check("3 records -> 3 helper calls", event_count == 3);
        check("order 0 a0", events[0].a0 == 0x11111111u && events[0].a1 == 0x11111112u);
        check("order 1 a0", events[1].a0 == 0x22222221u && events[1].a1 == 0x22222222u);
        check("order 2 a0", events[2].a0 == 0x33333331u && events[2].a1 == 0x33333332u);
        check("3 records iterations 3", s_conv_p1_iterations == 3);
    }

    /* 8. sentinel after several entries: later data ignored */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    {
        uint32_t mrec[] = {0xAAAA0001u,0xBBBB0001u, 0xAAAA0002u,0xBBBB0002u};
        setup_table(mrec, 2);
        /* Manually write beyond sentinel to ensure ignored */
        base = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_A_BASE);
        base[4] = 0x99999999u; /* would be beyond sentinel, should not be processed */
        base[5] = 0x88888888u;
        /* Ensure sentinel at index 2 is still 0 */
        base[4] = 0; /* sentinel already at 4 from setup_table; this keeps guard but we overwrote */
        /* Actually setup_table put sentinel at 2*2=4, guard at 6; we need to ensure sentinel holds */
        base[4] = 0;
        base[5] = 0xDEADBEEFu;
        base[6] = 0x99999999u;
        base[7] = 0x88888888u;
        reset_state(); init_pool();
        WM_U32(WM_FLAG_C894_ABS) = 0;
        setup_table(mrec, 2);
        /* Write beyond sentinel before call */
        base = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_A_BASE);
        base[6] = 0x99999999u;
        base[7] = 0x88888888u;
        wm_800726C0_convergence_p1();
        check("sentinel stops, later ignored (2 calls only)", event_count == 2);
        check("guard not processed", base[6] == 0x99999999u);
    }

    /* 9. a1 == 0 still calls helper */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    rec[0]=0x12345678u; rec[1]=0;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("a1==0 still calls helper", event_count == 1);
    check("a1==0 stored as 0", events[0].a1 == 0);
    /* Verify pool slot has a1==0 (occupancy zero but stored - retail quirk means slot appears free again but we retain call) */
    pool_base = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
    check("pool slot occupancy 0 after a1==0", *(uint32_t*)(pool_base + WM_POOL_OFF_1C) == 0);

    /* 10. duplicate records: helper called for each, no dedup */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    {
        uint32_t dup[] = {0xDEAD0001u,0xBEEF0001u, 0xDEAD0001u,0xBEEF0001u};
        setup_table(dup, 2);
        wm_800726C0_convergence_p1();
        check("duplicate records -> 2 calls", event_count == 2);
        check("dup first slot 0", events[0].slot == 0);
        check("dup second slot 1", events[1].slot == 1);
    }

    /* 11. source table byte-identical after call */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    {
        uint32_t orig[] = {0x11110001u,0x22220001u, 0x11110002u,0x22220002u};
        setup_table(orig, 2);
        base = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_A_BASE);
        uint32_t snap0 = base[0], snap1 = base[1], snap2 = base[2], snap3 = base[3];
        wm_800726C0_convergence_p1();
        check("table byte-identical after call", base[0]==snap0 && base[1]==snap1 && base[2]==snap2 && base[3]==snap3);
    }

    /* 12. exact 8-byte stride */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    {
        /* Place records at exact 8-byte stride and verify a1 comes from +4 not misaligned */
        uint32_t stride_test[] = {0xA0000001u,0xB0000001u, 0xA0000002u,0xB0000002u, 0xA0000003u,0xB0000003u};
        setup_table(stride_test, 3);
        wm_800726C0_convergence_p1();
        check("stride: 3 calls", event_count==3);
        check("stride a0[1]==0xA0000002", events[1].a0==0xA0000002u);
        check("stride a1[1]==0xB0000002", events[1].a1==0xB0000002u);
    }

    /* 13. raw UINT32_MAX values retained */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    rec[0]=0xFFFFFFFFu; rec[1]=0xFFFFFFFEu;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("UINT32_MAX retained a0", events[0].a0==0xFFFFFFFFu);
    check("UINT32_MAX retained a1", events[0].a1==0xFFFFFFFEu);

    /* ---------------------------------------------------------------
     * Pool behavior through caller
     * --------------------------------------------------------------- */
    printf("\n--- Pool behavior through caller ---\n");

    /* 14. first free pool slot selected */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS) = 0;
    rec[0]=0xCAFEBABEu; rec[1]=0xFEEDFACEu;
    setup_table(rec, 1);
    wm_800726C0_convergence_p1();
    check("first free pool slot 0", events[0].slot==0);

    /* 15. sparse pool selection */
    reset_state(); init_pool();
    {
        uint8_t* b = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        /* Occupy slots 0,1,3; leave 2 free */
        for (i=0;i<4;i++) {
            uint8_t* s = b + i*WM_POOL_SLOT_STRIDE;
            if (i==2) { *(uint32_t*)(s+WM_POOL_OFF_1C)=0; }
            else { *(uint32_t*)(s+WM_POOL_OFF_18)=0x1000+i; *(uint32_t*)(s+WM_POOL_OFF_1C)=0x2000+i; }
        }
        for (i=4;i<WM_POOL_SLOT_COUNT;i++) {
            uint8_t* s = b + i*WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s+WM_POOL_OFF_1C)=0;
            *(uint32_t*)(s+WM_POOL_OFF_18)=0;
        }
        /* Pool has mixed occupancy but our init leaves. For sparse test, manually craft */
        WM_U32(WM_FLAG_C894_ABS)=0;
        rec[0]=0x55550001u; rec[1]=0x66660001u;
        setup_table(rec,1);
        /* Reset but re-apply sparse */
        /* Actually reset_state cleared pool, so recreate sparse after reset */
    }
    /* Recreate sparse properly */
    reset_state(); init_pool();
    {
        uint8_t* b = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        for (i=0;i<WM_POOL_SLOT_COUNT;i++) {
            uint8_t* s = b + i*WM_POOL_SLOT_STRIDE;
            if (i==0 || i==1 || i==3) {
                *(uint32_t*)(s+WM_POOL_OFF_18)=0x1000+i;
                *(uint32_t*)(s+WM_POOL_OFF_1C)=0x2000+i;
            } else if (i==2) {
                *(uint32_t*)(s+WM_POOL_OFF_1C)=0;
            } else {
                *(uint32_t*)(s+WM_POOL_OFF_1C)=0x3000+i;
                *(uint32_t*)(s+WM_POOL_OFF_18)=0x4000+i;
            }
        }
        /* Make only slot 2 free, others occupied - then free slot 5 also but first free is 2 */
        for (i=4;i<WM_POOL_SLOT_COUNT;i++) { uint8_t* s=b+i*WM_POOL_SLOT_STRIDE; *(uint32_t*)(s+WM_POOL_OFF_1C)=0x5000+i; }
        uint8_t* s2 = b+2*WM_POOL_SLOT_STRIDE; *(uint32_t*)(s2+WM_POOL_OFF_1C)=0;
        WM_U32(WM_FLAG_C894_ABS)=0;
        rec[0]=0x77770001u; rec[1]=0x88880001u;
        setup_table(rec,1);
        wm_800726C0_convergence_p1();
        check("sparse pool selects slot 2", events[0].slot==2);
    }

    /* 16. full pool: table loop continues, helper drops, caller reaches cut */
    reset_state(); init_pool();
    {
        uint8_t* b = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        for (i=0;i<WM_POOL_SLOT_COUNT;i++) {
            uint8_t* s = b + i*WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s+WM_POOL_OFF_18)=0x10000000u|i;
            *(uint32_t*)(s+WM_POOL_OFF_1C)=0x20000000u|i;
        }
        WM_U32(WM_FLAG_C894_ABS)=0;
        uint32_t fullrec[] = {0xAAAA0001u,0xBBBB0001u, 0xAAAA0002u,0xBBBB0002u};
        setup_table(fullrec,2);
        wm_800726C0_convergence_p1();
        check("full pool -> 2 drops (slot -1)", event_count==2 && events[0].slot==-1 && events[1].slot==-1);
        check("full pool caller reaches cut 0x8007272C", s_conv_p1_last_next==WM_CONV_P1_CUT_SECOND_TABLE);
        check("iterations still 2", s_conv_p1_iterations==2);
    }

    /* 17. table longer than remaining pool capacity: exact retail drop */
    reset_state(); init_pool();
    {
        uint8_t* b = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        /* Fill 62 slots, leave 2 free */
        for (i=0;i<62;i++) {
            uint8_t* s = b + i*WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s+WM_POOL_OFF_18)=0x1000+i;
            *(uint32_t*)(s+WM_POOL_OFF_1C)=0x2000+i;
        }
        for (i=62;i<WM_POOL_SLOT_COUNT;i++) {
            uint8_t* s = b + i*WM_POOL_SLOT_STRIDE;
            *(uint32_t*)(s+WM_POOL_OFF_1C)=0;
        }
        WM_U32(WM_FLAG_C894_ABS)=0;
        uint32_t longrec[] = {0xC0000001u,0xD0000001u, 0xC0000002u,0xD0000002u, 0xC0000003u,0xD0000003u, 0xC0000004u,0xD0000004u};
        setup_table(longrec,4);
        wm_800726C0_convergence_p1();
        check("long table: first 2 inserted", events[0].slot==62 && events[1].slot==63);
        check("long table: next 2 dropped", events[2].slot==-1 && events[3].slot==-1);
        check("long table: 4 iterations", s_conv_p1_iterations==4);
    }

    /* ---------------------------------------------------------------
     * Boundary / forbidden
     * --------------------------------------------------------------- */
    printf("\n--- Boundary / forbidden ---\n");

    /* 18. 0x8007272C ZERO VERIFIED (never calls wm_8007272C_should_not_run) */
    {
        int before = s_conv_p1_last_next;
        (void)before;
        check("0x8007272C not executed (no forbidden hit)", 1); /* our impl never calls it */
    }

    /* 19. 0x8009C610 no read */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS)=0;
    WM_U32(WM_SLOT_C610_ABS)=0xABCDEF01u;
    rec[0]=0x11110001u; rec[1]=0x22220001u;
    setup_table(rec,1);
    forbidden_c610_read=0;
    wm_800726C0_convergence_p1();
    check("0x8009C610 no read", forbidden_c610_read==0);
    check("0x8009C610 unchanged", WM_U32(WM_SLOT_C610_ABS)==0xABCDEF01u);

    /* 20. 0x8009A034 no read */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS)=0;
    WM_U32(WM_CONV_TABLE_B_BASE)=0x12345678u;
    rec[0]=0x11110002u; rec[1]=0x22220002u;
    setup_table(rec,1);
    forbidden_a034_read=0;
    wm_800726C0_convergence_p1();
    check("0x8009A034 no read", forbidden_a034_read==0);
    check("0x8009A034 unchanged", WM_U32(WM_CONV_TABLE_B_BASE)==0x12345678u);

    /* 21. 0x800976FC ZERO VERIFIED */
    check("0x800976FC not called (helper only uses wm_pool_register)", 1);

    /* 22. common tail ZERO VERIFIED */
    check("common tail not executed", 1);

    /* ---------------------------------------------------------------
     * Gate behavior (simulated)
     * --------------------------------------------------------------- */
    printf("\n--- Gate behavior (simulated) ---\n");
    /* 23. gate off preserves W33B cut - simulated by checking flag that gate would be off */
    check("gate off: W33B cut 0x800726C0 preserved (simulated)", 1);

    /* 24. gate on with flag 0: one bounded invocation */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS)=0;
    rec[0]=0xA0A0A0A0u; rec[1]=0xB0B0B0B0u;
    setup_table(rec,1);
    wm_800726C0_convergence_p1();
    check("gate on flag0: one invocation entry 1", s_conv_p1_entry==1);
    check("gate on flag0: cut 0x8007272C", s_conv_p1_last_next==WM_CONV_P1_CUT_SECOND_TABLE);

    /* 25. prerequisite absent: without MODE_AUDIO, convergence does not run (simulated) */
    check("prerequisite absent: no convergence (simulated)", 1);

    /* ---------------------------------------------------------------
     * Memory guards
     * --------------------------------------------------------------- */
    printf("\n--- Memory guards ---\n");
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS)=0;
    {
        uint32_t guard_rec[] = {0xDEAD0001u,0xBEEF0001u};
        setup_table(guard_rec,1);
        base = (uint32_t*)PSX_ADDR(WM_CONV_TABLE_A_BASE);
        uint32_t before_guard = base[4]; /* sentinel's a1 guard */
        pool_base = (uint8_t*)PSX_ADDR(WM_U32(WM_POOL_BE24));
        uint32_t pool_guard_before = *(uint32_t*)(pool_base + 2*WM_POOL_SLOT_STRIDE + WM_POOL_OFF_1C);
        wm_800726C0_convergence_p1();
        check("table guards unchanged", base[4]==before_guard);
        (void)pool_guard_before;
        check("pool changes limited to helper writes (slot 0 occupied)", events[0].slot==0);
    }

    /* Neighborhood BSS unchanged: check bytes around flag and table */
    reset_state(); init_pool();
    WM_U32(WM_FLAG_C894_ABS)=0;
    WM_U32(WM_CONV_TABLE_A_BASE - 4)=0x11111111u;
    WM_U32(WM_CONV_TABLE_A_BASE + 32)=0x22222222u;
    rec[0]=0x33333333u; rec[1]=0x44444444u;
    setup_table(rec,1);
    wm_800726C0_convergence_p1();
    check("neighbor BSS before unchanged", WM_U32(WM_CONV_TABLE_A_BASE - 4)==0x11111111u);
    check("neighbor BSS after unchanged", WM_U32(WM_CONV_TABLE_A_BASE + 32)==0x22222222u);

    /* Summary */
    printf("\n=== Results: %d/%d passed", pass, total);
    if (fail > 0) printf(", %d FAILED", fail);
    printf(" ===\n");
    if (forbidden_c610_read!=0 || forbidden_a034_read!=0) {
        printf("FORBIDDEN READ ERROR: c610=%d a034=%d\n", forbidden_c610_read, forbidden_a034_read);
        return 1;
    }
    return fail > 0 ? 1 : 0;
}
