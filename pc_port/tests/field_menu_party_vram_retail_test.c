/* Regression test for the field menu's party-sprite VRAM save/restore tables.
 *
 * func_800799D4 (src/field/main/misc4.c) blits six 0x40 x 0x20 VRAM rects from
 * D_800ADCB0 out to D_800ADCC8 before running MenuMain, and back afterwards,
 * because the menu overwrites the party sprites' texture pages.  Both tables
 * are retail .data (asm/field/data/3DF78.data.s, RAM 0x800ADCB0 / 0x800ADCC8,
 * field.bin file offsets 0x3E1C0 / 0x3E1D8; field overlay base 0x8006FAF0).
 *
 * They sat in the gap between the already-migrated D_800ADC44 and
 * g_FieldData_800ADCE0, so the port's stub generator zero-filled them and every
 * blit degenerated to MoveImage({0,0,64,32}, 0, 0) -- a no-op self-copy of the
 * top-left corner of VRAM.  Nothing was saved and nothing was restored, so a
 * party member came back from the menu with the menu's leftovers (or 0x0000
 * texels, which the PSX blender treats as fully transparent) in its texture
 * page: the character renders its quads and shows nothing.
 *
 * The oracle is disc/field.bin itself, not a second copy of the transcription.
 * On top of that byte comparison the test replays the opener's blit sequence
 * over a VRAM model, with the menu's observed effect (the field pages come back
 * wiped -- measured on the Map 1 repro) in between, and requires the party
 * pages to survive.
 */
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

/* Retail geometry, from func_800799D4's rect.w / rect.h (misc4.c).  The runner
 * greps misc4.c for these literals so a change there cannot silently drift.
 * This is what the OPENER blits. */
#define PARTY_RECT_W 0x40
#define PARTY_RECT_H 0x20
#define PARTY_RECT_COUNT 6

/* What the MENU clobbers, and therefore what has to come back intact: the full
 * party texture page, independent of whatever geometry the opener chose to
 * blit.  Kept separate on purpose -- if the two are the same symbol, an opener
 * that saves and restores too little a rect looks self-consistent and passes. */
#define PARTY_PAGE_W 0x40
#define PARTY_PAGE_H 0x20

/* PSX VRAM: 1024 x 512 16-bit pixels. */
#define VRAM_W 1024
#define VRAM_H 512

/* Retail field.bin offsets of the two tables. */
#define FIELD_BIN_PATH "disc/field.bin"
#define D_800ADCB0_FILE_OFFSET 0x3E1C0
#define D_800ADCC8_FILE_OFFSET 0x3E1D8
#define TABLE_BYTES (PARTY_RECT_COUNT * 4)

/* Supplied by the tables extracted from pc_port/src/data_field.c. */
extern unsigned short D_800ADCB0[12];
extern unsigned short D_800ADCC8[12];

static unsigned short s_vram[VRAM_H][VRAM_W];

static int s_failures;

#define CHECK(cond, ...)                                                       \
    do {                                                                       \
        if (!(cond)) {                                                         \
            fprintf(stderr, "MENU PARTY VRAM FAIL ");                          \
            fprintf(stderr, __VA_ARGS__);                                      \
            fputc('\n', stderr);                                               \
            s_failures++;                                                      \
        }                                                                      \
    } while (0)

/* PsyQ MoveImage semantics: copy rect (sx, sy, w, h) to (dx, dy) in VRAM. */
static void vram_move_image(int sx, int sy, int w, int h, int dx, int dy)
{
    int y;

    for (y = 0; y < h; y++) {
        memmove(&s_vram[dy + y][dx], &s_vram[sy + y][sx],
                (size_t)w * sizeof(unsigned short));
    }
}

/* func_800799D4's backup loop (misc4.c: field rects -> backup rects). */
static void party_vram_backup(void)
{
    int i;

    for (i = 0; i < PARTY_RECT_COUNT; i++) {
        vram_move_image(D_800ADCB0[i * 2], D_800ADCB0[i * 2 + 1],
                        PARTY_RECT_W, PARTY_RECT_H,
                        D_800ADCC8[i * 2], D_800ADCC8[i * 2 + 1]);
    }
}

/* func_800799D4's restore loop (backup rects -> field rects). */
static void party_vram_restore(void)
{
    int i;

    for (i = 0; i < PARTY_RECT_COUNT; i++) {
        vram_move_image(D_800ADCC8[i * 2], D_800ADCC8[i * 2 + 1],
                        PARTY_RECT_W, PARTY_RECT_H,
                        D_800ADCB0[i * 2], D_800ADCB0[i * 2 + 1]);
    }
}

static void fill_vram_pattern(void)
{
    int y;
    int x;

    for (y = 0; y < VRAM_H; y++) {
        for (x = 0; x < VRAM_W; x++) {
            /* Deterministic, never 0x0000 anywhere inside the party pages, so
             * "restored" cannot be satisfied by a page that stayed blank. */
            s_vram[y][x] = (unsigned short)(((y * 7919u) ^ (x * 104729u)) | 1u);
        }
    }
}

/* What MenuMain does to the party pages, as measured on the Map 1 repro:
 * the field rects come back zeroed. */
static void wipe_party_pages(void)
{
    int i;
    int y;

    for (i = 0; i < PARTY_RECT_COUNT; i++) {
        int x0 = D_800ADCB0[i * 2];
        int y0 = D_800ADCB0[i * 2 + 1];

        for (y = 0; y < PARTY_PAGE_H; y++) {
            memset(&s_vram[y0 + y][x0], 0,
                   (size_t)PARTY_PAGE_W * sizeof(unsigned short));
        }
    }
}

static void snapshot_party_pages(unsigned short out[PARTY_RECT_COUNT]
                                                   [PARTY_PAGE_H]
                                                   [PARTY_PAGE_W])
{
    int i;
    int y;

    for (i = 0; i < PARTY_RECT_COUNT; i++) {
        int x0 = D_800ADCB0[i * 2];
        int y0 = D_800ADCB0[i * 2 + 1];

        for (y = 0; y < PARTY_PAGE_H; y++) {
            memcpy(out[i][y], &s_vram[y0 + y][x0],
                   (size_t)PARTY_PAGE_W * sizeof(unsigned short));
        }
    }
}

static int rects_overlap(int ax, int ay, int bx, int by)
{
    return ax < bx + PARTY_RECT_W && bx < ax + PARTY_RECT_W &&
           ay < by + PARTY_RECT_H && by < ay + PARTY_RECT_H;
}

/* 1. The migrated tables must be byte-identical to retail field.bin. */
static void check_against_disc(void)
{
    unsigned char expected[TABLE_BYTES];
    FILE *f = fopen(FIELD_BIN_PATH, "rb");

    if (f == NULL) {
        fprintf(stderr, "MENU PARTY VRAM FAIL cannot open %s\n",
                FIELD_BIN_PATH);
        s_failures++;
        return;
    }

    CHECK(fseek(f, D_800ADCB0_FILE_OFFSET, SEEK_SET) == 0, "seek D_800ADCB0");
    CHECK(fread(expected, 1, TABLE_BYTES, f) == TABLE_BYTES, "read D_800ADCB0");
    CHECK(memcmp(expected, D_800ADCB0, TABLE_BYTES) == 0,
          "D_800ADCB0 differs from retail field.bin@0x%X",
          D_800ADCB0_FILE_OFFSET);

    CHECK(fseek(f, D_800ADCC8_FILE_OFFSET, SEEK_SET) == 0, "seek D_800ADCC8");
    CHECK(fread(expected, 1, TABLE_BYTES, f) == TABLE_BYTES, "read D_800ADCC8");
    CHECK(memcmp(expected, D_800ADCC8, TABLE_BYTES) == 0,
          "D_800ADCC8 differs from retail field.bin@0x%X",
          D_800ADCC8_FILE_OFFSET);

    fclose(f);
}

/* 2. Geometry the opener depends on: every rect inside VRAM, the backup rects
 *    mutually disjoint, and no backup rect landing on a field page (which would
 *    let the menu's wipe destroy the saved copy). */
static void check_geometry(void)
{
    int i;
    int j;
    int nonZeroTables = 0;

    for (i = 0; i < PARTY_RECT_COUNT; i++) {
        int fx = D_800ADCB0[i * 2];
        int fy = D_800ADCB0[i * 2 + 1];
        int bx = D_800ADCC8[i * 2];
        int by = D_800ADCC8[i * 2 + 1];

        if (fx != 0 || fy != 0 || bx != 0 || by != 0) {
            nonZeroTables = 1;
        }
        CHECK(fx >= 0 && fx + PARTY_PAGE_W <= VRAM_W && fy >= 0 &&
                  fy + PARTY_PAGE_H <= VRAM_H,
              "field rect %d (%d,%d) outside VRAM", i, fx, fy);
        CHECK(bx >= 0 && bx + PARTY_RECT_W <= VRAM_W && by >= 0 &&
                  by + PARTY_RECT_H <= VRAM_H,
              "backup rect %d (%d,%d) outside VRAM", i, bx, by);
        CHECK(!rects_overlap(fx, fy, bx, by),
              "rect %d backs itself up onto its own page (%d,%d)", i, fx, fy);

        for (j = 0; j < PARTY_RECT_COUNT; j++) {
            int ox = D_800ADCB0[j * 2];
            int oy = D_800ADCB0[j * 2 + 1];

            CHECK(!rects_overlap(bx, by, ox, oy),
                  "backup rect %d (%d,%d) overlaps field rect %d (%d,%d)",
                  i, bx, by, j, ox, oy);
            if (j <= i) {
                continue;
            }
            CHECK(!rects_overlap(bx, by, D_800ADCC8[j * 2],
                                 D_800ADCC8[j * 2 + 1]),
                  "backup rects %d and %d overlap", i, j);
            CHECK(!rects_overlap(fx, fy, ox, oy),
                  "field rects %d and %d overlap", i, j);
        }
    }

    /* The pre-fix state was an all-zero table: six identical (0,0) rects.  That
     * trips the overlap checks above, but state it explicitly so the failure
     * names the actual bug. */
    CHECK(nonZeroTables,
          "both tables are all zero -- the stub-generated placeholders, not the "
          "retail .data (this is the pre-fix bug: nothing is saved or restored)");
}

/* 3. Replay the opener's blit sequence with the menu's wipe in between. */
static void check_round_trip(void)
{
    static unsigned short before[PARTY_RECT_COUNT][PARTY_PAGE_H][PARTY_PAGE_W];
    static unsigned short after[PARTY_RECT_COUNT][PARTY_PAGE_H][PARTY_PAGE_W];
    int i;
    int y;
    int x;
    int nonZeroBefore = 0;

    fill_vram_pattern();
    snapshot_party_pages(before);

    for (i = 0; i < PARTY_RECT_COUNT; i++) {
        for (y = 0; y < PARTY_PAGE_H; y++) {
            for (x = 0; x < PARTY_PAGE_W; x++) {
                if (before[i][y][x] != 0) {
                    nonZeroBefore++;
                }
            }
        }
    }
    CHECK(nonZeroBefore ==
              PARTY_RECT_COUNT * PARTY_PAGE_H * PARTY_PAGE_W,
          "fixture is not fully non-zero (%d texels) -- a restore that leaves "
          "blanks could pass by accident", nonZeroBefore);

    party_vram_backup();
    wipe_party_pages();
    party_vram_restore();
    snapshot_party_pages(after);

    for (i = 0; i < PARTY_RECT_COUNT; i++) {
        CHECK(memcmp(before[i], after[i], sizeof(before[i])) == 0,
              "party page %d at (%u,%u) did not survive the menu round trip",
              i, (unsigned)D_800ADCB0[i * 2], (unsigned)D_800ADCB0[i * 2 + 1]);
    }
}

int main(void)
{
    check_against_disc();
    check_geometry();
    check_round_trip();

    if (s_failures != 0) {
        fprintf(stderr, "MENU PARTY VRAM FAIL failures=%d\n", s_failures);
        return 1;
    }
    printf("MENU PARTY VRAM PASS rects=%d %dx%d disc-oracle=%s\n",
           PARTY_RECT_COUNT, PARTY_RECT_W, PARTY_RECT_H, FIELD_BIN_PATH);
    return 0;
}
