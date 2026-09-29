/*
 * D_8004FE50 model-prim dispatch table vs the retail table.
 *
 * The port's D_8004FE50[17] is a hand-maintained mirror of a real retail
 * .sdata table at vaddr 0x8004FE50 (17 rows of 0x28: six variant walker
 * pointers, a build proc, then cmd/packet/output strides).  Two failure modes
 * have already cost real bugs:
 *
 *   1. A row present but with a NULL variant slot that retail fills: the BUILD
 *      side still links that group's packets into ot3 with tag lengths set,
 *      so the walker parses uninitialised packets ("ptag length is not valid")
 *      and runs off the chain -- the Map 2 SEGV, row 0x04.
 *   2. A row ABSENT from the designated initialiser entirely: it is
 *      zero-filled, so buildProc is NULL and all three strides are 0.  Same
 *      corruption class, strictly worse.
 *
 * This test links the real production pc_port/src/game_overrides.c and
 * compares its live table against the retail bytes read straight out of
 * disc/SLUS_006.64, so drift in either direction is caught.
 *
 * It deliberately does NOT require every row to be populated -- five rows are
 * known-absent because their walkers are not ported yet.  Instead it pins the
 * exact populated set, so filling a row (good) or losing one (bad) both show
 * up as an explicit, reviewable diff rather than silently changing behaviour.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef int32_t s32;
typedef uint32_t u32;
typedef uint8_t u8;

typedef s32 (*ModelPrimProc)(u8* pCmd, s32 count);
typedef s32 (*ModelPrimBuildProc)(u32 pSrc, u32 pCmd, s32 shade);

typedef struct ModelPrimDesc {
    ModelPrimProc proc[6];
    ModelPrimBuildProc buildProc;
    u32 cmdStride;
    u32 packetStride;
    u32 outputStride;
} ModelPrimDesc;

extern ModelPrimDesc D_8004FE50[17];

#define ROWS 17
#define ROW_STRIDE 0x28
#define TABLE_VADDR 0x8004FE50u

/* Retail .sdata mapping used throughout this project. */
#define FILE_OFF(vaddr) (0x800u + (vaddr) - 0x80010000u)

/* Rows the port currently populates.  Rows 0x0B, 0x0F and 0x10 are still
 * absent: each needs a walker (retail 0x8002E22C, 0x80030750) that has no port
 * implementation under any name yet.
 * See docs/evidence/prim-table-rows-20260906/README.md. */
static const int kExpectedPopulated[] = {
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0C,
    0x0D, 0x0E
};
static const int kExpectedPopulatedCount =
    (int)(sizeof(kExpectedPopulated) / sizeof(kExpectedPopulated[0]));

static int g_assertions;
static int g_failures;

static void check(int cond, const char* fmt, ...) {
    __builtin_va_list ap;
    g_assertions++;
    if (cond) {
        return;
    }
    g_failures++;
    fputs("FAIL ", stdout);
    __builtin_va_start(ap, fmt);
    vprintf(fmt, ap);
    __builtin_va_end(ap);
    fputc('\n', stdout);
}

static u32 rd32(const u8* p) {
    return (u32)p[0] | ((u32)p[1] << 8) | ((u32)p[2] << 16) | ((u32)p[3] << 24);
}

typedef struct RetailRow {
    u32 proc[6];
    u32 buildProc;
    u32 cmdStride;
    u32 packetStride;
    u32 outputStride;
} RetailRow;

int main(int argc, char** argv) {
    const char* path = (argc > 1) ? argv[1] : "disc/SLUS_006.64";
    RetailRow retail[ROWS];
    long size;
    u8* blob;
    FILE* f;
    int i;
    int populated[ROWS];
    int cases = 0;

    f = fopen(path, "rb");
    if (f == NULL) {
        printf("FAIL cannot open retail binary %s\n", path);
        return 1;
    }
    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);
    blob = (u8*)malloc((size_t)size);
    if (blob == NULL || fread(blob, 1, (size_t)size, f) != (size_t)size) {
        printf("FAIL cannot read retail binary %s\n", path);
        fclose(f);
        free(blob);
        return 1;
    }
    fclose(f);

    /* Parse the retail table. */
    for (i = 0; i < ROWS; i++) {
        u32 va = TABLE_VADDR + (u32)i * ROW_STRIDE;
        u32 off = FILE_OFF(va);
        const u8* p;
        int k;

        if ((long)(off + ROW_STRIDE) > size) {
            printf("FAIL retail row 0x%02X at file offset 0x%X is past EOF\n",
                   i, off);
            free(blob);
            return 1;
        }
        p = blob + off;
        for (k = 0; k < 6; k++) {
            retail[i].proc[k] = rd32(p + k * 4);
        }
        retail[i].buildProc = rd32(p + 24);
        retail[i].cmdStride = rd32(p + 28);
        retail[i].packetStride = rd32(p + 32);
        retail[i].outputStride = rd32(p + 36);
    }

    /* Sanity-pin the parse itself against a row whose retail contents are
     * independently recorded in the 2026-09-02 grind-log entry.  If the
     * offset math or row layout ever drifts, this fails first and loudly. */
    cases++;
    check(retail[0x04].proc[0] == 0x8002E038u,
          "retail parse: row 0x04 proc[0] expected 8002E038, got %08X",
          retail[0x04].proc[0]);
    check(retail[0x04].proc[3] == 0x8002E8DCu,
          "retail parse: row 0x04 proc[3] expected 8002E8DC, got %08X",
          retail[0x04].proc[3]);
    check(retail[0x04].buildProc == 0x8002CF34u,
          "retail parse: row 0x04 buildProc expected 8002CF34, got %08X",
          retail[0x04].buildProc);
    check(retail[0x04].cmdStride == 0x8u && retail[0x04].packetStride == 0x4u &&
              retail[0x04].outputStride == 0x14u,
          "retail parse: row 0x04 strides expected 0x8/0x4/0x14, got %X/%X/%X",
          retail[0x04].cmdStride, retail[0x04].packetStride,
          retail[0x04].outputStride);

    /* Which rows does the port actually populate?
     *
     * Judged on the variant procs ONLY, deliberately.  The buildProcs are
     * not-yet-ported externs (func_8002CDCC and friends), and this test links
     * with --unresolved-symbols=ignore-all, so every buildProc address reads
     * back as 0 here regardless of what the source says.  buildProc presence
     * is therefore pinned at the SOURCE level by the runner script instead of
     * being checked at run time. */
    for (i = 0; i < ROWS; i++) {
        int k;
        int any = 0;
        for (k = 0; k < 6; k++) {
            if (D_8004FE50[i].proc[k] != NULL) {
                any = 1;
            }
        }
        populated[i] = any;
    }

    /* Case 1: the populated set is exactly the expected set. */
    cases++;
    for (i = 0; i < ROWS; i++) {
        int expected = 0;
        int j;
        for (j = 0; j < kExpectedPopulatedCount; j++) {
            if (kExpectedPopulated[j] == i) {
                expected = 1;
            }
        }
        check(populated[i] == expected,
              "row 0x%02X: populated=%d but expected %d "
              "(filling or dropping a row must be an explicit change)",
              i, populated[i], expected);
    }

    /* Case 2: every populated row's three strides equal retail exactly.
     * A zero-filled row would fail here too, which is the Map-2 bug class. */
    cases++;
    for (i = 0; i < ROWS; i++) {
        if (!populated[i]) {
            continue;
        }
        check(D_8004FE50[i].cmdStride == retail[i].cmdStride,
              "row 0x%02X cmdStride: port 0x%X != retail 0x%X",
              i, D_8004FE50[i].cmdStride, retail[i].cmdStride);
        check(D_8004FE50[i].packetStride == retail[i].packetStride,
              "row 0x%02X packetStride: port 0x%X != retail 0x%X",
              i, D_8004FE50[i].packetStride, retail[i].packetStride);
        check(D_8004FE50[i].outputStride == retail[i].outputStride,
              "row 0x%02X outputStride: port 0x%X != retail 0x%X",
              i, D_8004FE50[i].outputStride, retail[i].outputStride);
    }

    /* Case 3: retail defines a buildProc for every row.  The port-side
     * counterpart cannot be observed here (see the populated[] note above); it
     * is pinned in the runner against the source text. */
    cases++;
    for (i = 0; i < ROWS; i++) {
        check(retail[i].buildProc != 0u,
              "retail row 0x%02X unexpectedly has a NULL buildProc", i);
    }

    /* Case 4: variant 0 must never be NULL in a populated row.  Retail always
     * fills proc[0]; a NULL there means the row's primary walker is missing
     * while its packets are still being built. */
    cases++;
    for (i = 0; i < ROWS; i++) {
        if (!populated[i]) {
            continue;
        }
        check(retail[i].proc[0] != 0u,
              "retail row 0x%02X unexpectedly has a NULL proc[0]", i);
        check(D_8004FE50[i].proc[0] != NULL,
              "row 0x%02X is populated but proc[0] is NULL (retail %08X)",
              i, retail[i].proc[0]);
    }

    /* Case 5: rows retail defines as identical must be identical in the port.
     * Retail row 0x0E is byte-for-byte row 0x0A, and this is exactly why 0x0E
     * could be filled with no new walker code -- pin that reasoning so it
     * cannot silently stop being true. */
    cases++;
    check(memcmp(&retail[0x0E], &retail[0x0A], sizeof(RetailRow)) == 0,
          "retail rows 0x0A and 0x0E are no longer identical -- row 0x0E was "
          "filled on the assumption that they are");
    {
        int k;
        for (k = 0; k < 6; k++) {
            check(D_8004FE50[0x0E].proc[k] == D_8004FE50[0x0A].proc[k],
                  "port row 0x0E proc[%d] does not match row 0x0A", k);
        }
        check(D_8004FE50[0x0E].buildProc == D_8004FE50[0x0A].buildProc,
              "port row 0x0E buildProc does not match row 0x0A");
    }

    /* Case 6: row 0x07 differs from row 0x03 in retail only at proc[1]; that
     * is what let 0x07 reuse row 0x03's already-ported walkers. */
    cases++;
    check(retail[0x07].buildProc == retail[0x03].buildProc,
          "retail rows 0x03/0x07 buildProc diverged (%08X vs %08X)",
          retail[0x03].buildProc, retail[0x07].buildProc);
    check(retail[0x07].cmdStride == retail[0x03].cmdStride &&
              retail[0x07].packetStride == retail[0x03].packetStride &&
              retail[0x07].outputStride == retail[0x03].outputStride,
          "retail rows 0x03/0x07 strides diverged");
    {
        int k;
        int diffs = 0;
        for (k = 0; k < 6; k++) {
            if (retail[0x07].proc[k] != retail[0x03].proc[k]) {
                diffs++;
                check(k == 1,
                      "retail rows 0x03/0x07 differ at proc[%d]; only proc[1] "
                      "was expected to differ", k);
            }
        }
        check(diffs == 1, "retail rows 0x03/0x07 differ in %d slots, expected 1",
              diffs);
        /* Row 0x07 proc[1] reuses the shared body that is also its proc[0]. */
        check(retail[0x07].proc[1] == retail[0x07].proc[0],
              "retail row 0x07 proc[1] (%08X) was expected to equal proc[0] "
              "(%08X)", retail[0x07].proc[1], retail[0x07].proc[0]);
        check(D_8004FE50[0x07].proc[1] == D_8004FE50[0x07].proc[0],
              "port row 0x07 proc[1] must be the same walker as proc[0]");
    }

    free(blob);

    printf("PRIM TABLE CASES %d ASSERTIONS %d FAILURES %d\n",
           cases, g_assertions, g_failures);
    if (g_failures != 0) {
        printf("PRIM TABLE RESULT FAIL\n");
        return 1;
    }
    printf("PRIM TABLE RESULT PASS\n");
    return 0;
}
