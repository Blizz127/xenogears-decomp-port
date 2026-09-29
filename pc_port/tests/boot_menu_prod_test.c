/* Production-linked boot-menu certificate.
 *
 * Drives the shipped PcPort_BootUiTick / PcPort_BootEnterField from their
 * real start state (title phase, empty pad). Not a reimplementation of the
 * boot loop: the same functions PcPort_BootMain calls. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "common.h"
#include "boot_menu.h"
#include "system/controller.h"

static int s_failures;
static int s_initCalled;
static unsigned s_fieldState = 0xFFFFFFFFu;

unsigned short D_8006F94E;
unsigned short D_8006F954;
unsigned short D_8006F950;
unsigned char g_GameState[0x2400];

void func_8001B970(void)
{
    s_initCalled = 1;
}

void ChangeGameState(unsigned int state)
{
    s_fieldState = state;
}

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

static char s_savePath[256];

static void write_save_fixture(unsigned short map, unsigned short entrance,
                               unsigned short camoct)
{
    FILE* f;
    unsigned char blob[8 + 4 + 0x2358];
    unsigned int magic = 0x4F4E4558u;

    memset(blob, 0, sizeof(blob));
    memcpy(blob, &magic, 4);
    memcpy(blob + 4, &map, 2);
    memcpy(blob + 6, &entrance, 2);
    memcpy(blob + 8, &camoct, 2);
    snprintf(s_savePath, sizeof(s_savePath),
             "%s/continue_save.bin",
             getenv("BOOT_MENU_FIXTURE_DIR") ? getenv("BOOT_MENU_FIXTURE_DIR")
                                             : "/tmp");
    f = fopen(s_savePath, "wb");
    check(f != NULL, "continue.save.fixture.write");
    if (f) {
        fwrite(blob, 1, sizeof(blob), f);
        fclose(f);
    }
    setenv("XENO_BOOT_SAVE", s_savePath, 1);
}

static void test_phase_order(void)
{
    PcPortBootUi ui;
    int ticks;
    int sawMovie = 0;
    int sawMenu = 0;
    int confirmed = 0;

    unsetenv("XENO_BOOT_SAVE");
    setenv("XENO_BOOT_DELAY", "2", 1);
    PcPort_BootUiInit(&ui);
    check(ui.phase == PC_PORT_BOOT_TITLE, "boot.title.then.movie.then.menu");

    for (ticks = 0; ticks < 16; ticks++) {
        int rc = PcPort_BootUiTick(&ui, 0, 0);

        if (ui.phase == PC_PORT_BOOT_MOVIE)
            sawMovie = 1;
        if (ui.phase == PC_PORT_BOOT_MENU)
            sawMenu = 1;
        if (rc == PC_PORT_BOOT_TICK_NEWGAME) {
            confirmed = 1;
            break;
        }
    }

    check(sawMovie, "boot.title.then.movie.then.menu");
    check(sawMenu, "boot.title.then.movie.then.menu");
    check(confirmed, "boot.title.then.movie.then.menu");
    check(ui.phase == PC_PORT_BOOT_MENU, "boot.title.then.movie.then.menu");
}

static void test_start_advances_title(void)
{
    PcPortBootUi ui;

    setenv("XENO_BOOT_DELAY", "60", 1);
    PcPort_BootUiInit(&ui);
    check(ui.phase == PC_PORT_BOOT_TITLE, "boot.start.advances.title");
    check(PcPort_BootUiTick(&ui, CTRL_BTN_START, 0) == 0,
          "boot.start.advances.title");
    check(ui.phase == PC_PORT_BOOT_MOVIE, "boot.start.advances.title");
}

static void test_intro_is_str_feed(void)
{
    PcPortBootUi ui;

    setenv("XENO_BOOT_DELAY", "60", 1);
    PcPort_BootUiInit(&ui);
    PcPort_BootUiTick(&ui, CTRL_BTN_START, 0);
    check(ui.phase == PC_PORT_BOOT_MOVIE, "boot.intro.str.feed");
    PcPort_BootUiTick(&ui, 0, 0);
    check(PcPort_BootIntroUsedStr(&ui), "boot.intro.str.feed");
}

static const char* oracle_disc_path(void)
{
    static const char* defaults[] = {
        "disc/disc1.bin", "../disc/disc1.bin", "../../disc/disc1.bin",
    };
    const char* env = getenv("XENO_DISC");
    unsigned i;
    FILE* f;
    if (env && env[0]) {
        f = fopen(env, "rb");
        if (f) {
            fclose(f);
            return env;
        }
    }
    for (i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {
        f = fopen(defaults[i], "rb");
        if (f) {
            fclose(f);
            return defaults[i];
        }
    }
    return NULL;
}

static int oracle_read_sectors(int lba, int count, unsigned char* dst)
{
    const char* path = oracle_disc_path();
    FILE* f;
    struct stat st;
    int sec_size = 2048;
    int data_off = 0;
    int i;

    if (!path)
        return -1;
    if (stat(path, &st) != 0)
        return -1;
    if (st.st_size % 2352 == 0 && st.st_size / 2352 > 1000) {
        sec_size = 2352;
        data_off = 24;
    }
    f = fopen(path, "rb");
    if (!f)
        return -1;
    for (i = 0; i < count; i++) {
        if (fseek(f, (long)(lba + i) * sec_size + data_off, SEEK_SET) != 0) {
            fclose(f);
            return -1;
        }
        if (fread(dst + i * 0x800, 1, 0x800, f) != 0x800) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}

/* Independent Opening-STR oracle: archive file 15 / LBA 54133, not file 2.
 * Last-picture span is next-entry LBA 98917 (file 16), not table size. */
enum { ORACLE_OPENING_STR_ENTRY = 15 };
enum { ORACLE_OPENING_STR_LBA = 54133 };
enum { ORACLE_OPENING_NEXT_LBA = 98917 };
enum { ORACLE_PREFIX_SECTORS = 32 };
enum { ORACLE_TITLE_MIN_FLEN = 12000 };
enum { ORACLE_TITLE_SCAN_SECTORS = 2500 };

static int oracle_opening_entry(int* lba_out, int* size_out)
{
    unsigned char table[0x10 * 0x800];
    unsigned char* e;

    if (oracle_read_sectors(0x18, 0x10, table) != 0)
        return 0;
    e = table + (ORACLE_OPENING_STR_ENTRY - 1) * 7;
    *lba_out = e[0] | (e[1] << 8) | (e[2] << 16);
    *size_out = e[3] | (e[4] << 8) | (e[5] << 16) | (e[6] << 24);
    return *lba_out > 0 && *size_out >= 0x800;
}

static int oracle_opening_prefix(unsigned char** str_out, int* str_size,
                                 int* lba_out, int* file_size_out)
{
    int lba, size, prefix_ns;
    unsigned char* buf;

    *str_out = NULL;
    *str_size = 0;
    if (!oracle_opening_entry(&lba, &size))
        return 0;
    prefix_ns = ORACLE_PREFIX_SECTORS;
    if (prefix_ns > size / 0x800)
        prefix_ns = size / 0x800;
    buf = (unsigned char*)malloc((size_t)prefix_ns * 0x800);
    if (!buf)
        return 0;
    if (oracle_read_sectors(lba, prefix_ns, buf) != 0) {
        free(buf);
        return 0;
    }
    *str_out = buf;
    *str_size = prefix_ns * 0x800;
    *lba_out = lba;
    *file_size_out = size;
    return 1;
}

static int oracle_opening_title(unsigned char** title_out, int* title_size)
{
    unsigned char table[0x10 * 0x800];
    unsigned char* e;
    int lba, size, next_lba, nsect, back, last_off, last_ns;
    unsigned char sec[0x800];
    unsigned char* title;

    *title_out = NULL;
    *title_size = 0;
    if (oracle_read_sectors(0x18, 0x10, table) != 0)
        return 0;
    e = table + (ORACLE_OPENING_STR_ENTRY - 1) * 7;
    lba = e[0] | (e[1] << 8) | (e[2] << 16);
    size = e[3] | (e[4] << 8) | (e[5] << 16) | (e[6] << 24);
    e = table + (ORACLE_OPENING_STR_ENTRY) * 7;
    next_lba = e[0] | (e[1] << 8) | (e[2] << 16);
    if (lba != ORACLE_OPENING_STR_LBA || next_lba != ORACLE_OPENING_NEXT_LBA)
        return 0;
    if (next_lba <= lba)
        return 0;
    nsect = next_lba - lba;
    (void)size;
    last_off = 0;
    last_ns = 0;
    {
        int fb_off = 0, fb_ns = 0, have_fb = 0;
        for (back = nsect - 1;
             back >= 0 && back >= nsect - ORACLE_TITLE_SCAN_SECTORS;
             back--) {
            unsigned mag, typ, sin, ns, flen;
            if (oracle_read_sectors(lba + back, 1, sec) != 0)
                break;
            mag = (unsigned)(sec[0] | (sec[1] << 8));
            typ = (unsigned)(sec[2] | (sec[3] << 8));
            sin = (unsigned)(sec[4] | (sec[5] << 8));
            ns = (unsigned)(sec[6] | (sec[7] << 8));
            flen = (unsigned)(sec[12] | (sec[13] << 8) | (sec[14] << 16) |
                             (sec[15] << 24));
            if (mag == 0x0160 && sin == 0 && typ == 0x8001) {
                if (ns <= 0)
                    ns = 1;
                if (back + (int)ns > nsect)
                    continue;
                if (!have_fb) {
                    fb_off = back;
                    fb_ns = (int)ns;
                    have_fb = 1;
                }
                if ((int)flen >= ORACLE_TITLE_MIN_FLEN) {
                    last_off = back;
                    last_ns = (int)ns;
                    have_fb = 0;
                    break;
                }
            }
        }
        if (have_fb) {
            last_off = fb_off;
            last_ns = fb_ns;
        }
    }
    if (last_ns < 1)
        return 0;
    title = (unsigned char*)malloc((size_t)last_ns * 0x800);
    if (!title)
        return 0;
    if (oracle_read_sectors(lba + last_off, last_ns, title) != 0) {
        free(title);
        return 0;
    }
    *title_out = title;
    *title_size = last_ns * 0x800;
    return 1;
}

static void drive_to_movie(PcPortBootUi* ui)
{
    setenv("XENO_BOOT_DELAY", "60", 1);
    PcPort_BootUiInit(ui);
    PcPort_BootUiTick(ui, CTRL_BTN_START, 0);
    PcPort_BootUiTick(ui, 0, 0);
}

static void test_intro_str_memcmp(void)
{
    PcPortBootUi ui;
    unsigned char* oracle = NULL;
    int oracle_size = 0, oracle_lba = 0, oracle_file = 0;
    int got_size = 0;
    const unsigned char* got;
    int ncmp;

    drive_to_movie(&ui);
    check(oracle_opening_prefix(&oracle, &oracle_size, &oracle_lba, &oracle_file),
          "boot.intro.str.memcmp");
    check(oracle_lba == ORACLE_OPENING_STR_LBA, "boot.intro.str.memcmp");
    check(oracle_file > 50 * 1024 * 1024, "boot.intro.str.memcmp");
    check(PcPort_BootIntroStrLba() == oracle_lba, "boot.intro.str.memcmp");
    check(PcPort_BootIntroStrFileSize() == oracle_file, "boot.intro.str.memcmp");
    got = PcPort_BootIntroStrPayload(&got_size);
    check(got != NULL && oracle != NULL, "boot.intro.str.memcmp");
    check(got_size >= 0x8000 && oracle_size >= 0x8000, "boot.intro.str.memcmp");
    ncmp = got_size < oracle_size ? got_size : oracle_size;
    check(got && oracle && memcmp(got, oracle, (size_t)ncmp) == 0,
          "boot.intro.str.memcmp");
    check(got && (got[0] | (got[1] << 8)) == 0x0160, "boot.intro.str.memcmp");
    free(oracle);
}

static void test_title_graphic_memcmp(void)
{
    PcPortBootUi ui;
    unsigned char* title = NULL;
    int title_size = 0;
    int got_size = 0;
    const unsigned char* got;
    int ticks;

    drive_to_movie(&ui);
    for (ticks = 0; ticks < 8 && ui.phase != PC_PORT_BOOT_MENU; ticks++)
        PcPort_BootUiTick(&ui, CTRL_BTN_CROSS, 0);
    check(ui.phase == PC_PORT_BOOT_MENU, "boot.menu.retail.graphic");
    check(PcPort_BootTitleUsedRetailGraphic(), "boot.menu.retail.graphic");
    check(ui.titleGraphicLoaded, "boot.menu.retail.graphic");
    check(oracle_opening_title(&title, &title_size), "boot.menu.retail.graphic");
    got = PcPort_BootTitleGraphicPayload(&got_size);
    check(got != NULL && title != NULL, "boot.menu.retail.graphic");
    check(got_size == title_size && title_size >= 0x800, "boot.menu.retail.graphic");
    check(got && title && got_size == title_size &&
              memcmp(got, title, (size_t)got_size) == 0,
          "boot.menu.retail.graphic");
    check(got && (got[0] | (got[1] << 8)) == 0x0160, "boot.menu.retail.graphic");
    free(title);
}

static void test_continue_without_save_does_not_freeze(void)
{
    PcPortBootUi ui;
    int rc;

    unsetenv("XENO_BOOT_SAVE");
    setenv("XENO_BOOT_DELAY", "60", 1);
    setenv("XENO_BOOT_SAVE", "/no/such/continue.save", 1);
    PcPort_BootUiInit(&ui);
    PcPort_BootUiTick(&ui, CTRL_BTN_START, 0);
    PcPort_BootUiTick(&ui, CTRL_BTN_CROSS, 0);
    check(ui.phase == PC_PORT_BOOT_MENU, "boot.continue.does.not.freeze");

    ui.menuChoice = 1;
    rc = PcPort_BootUiTick(&ui, 0, CTRL_BTN_CIRCLE);
    check(rc == 0, "boot.continue.does.not.freeze");
    check(ui.menuChoice == 1, "boot.continue.does.not.freeze");
    check(ui.phase == PC_PORT_BOOT_MENU, "boot.continue.does.not.freeze");
}

static void test_continue_with_save_enters_saved_field(void)
{
    PcPortBootUi ui;
    int rc;

    write_save_fixture(5, 0, 7);
    s_initCalled = 0;
    s_fieldState = 0xFFFFFFFFu;
    D_8006F94E = 14;
    D_8006F954 = 0;
    D_8006F950 = 0;
    setenv("XENO_BOOT_DELAY", "60", 1);
    PcPort_BootUiInit(&ui);
    PcPort_BootUiTick(&ui, CTRL_BTN_START, 0);
    PcPort_BootUiTick(&ui, CTRL_BTN_CROSS, 0);
    check(ui.phase == PC_PORT_BOOT_MENU, "boot.continue.restores.save");
    ui.menuChoice = 1;
    rc = PcPort_BootUiTick(&ui, 0, CTRL_BTN_CIRCLE);
    check(rc == PC_PORT_BOOT_TICK_CONTINUE, "boot.continue.restores.save");
    PcPort_BootEnterContinue();
    check(s_initCalled == 0, "boot.continue.restores.save");
    check(s_fieldState == 1, "boot.continue.restores.save");
    check(D_8006F94E == 5, "boot.continue.restores.save");
}

static void test_new_game_init_then_field(void)
{
    PcPortBootUi ui;
    int ticks;
    int confirmed = 0;

    unsetenv("XENO_BOOT_SAVE");
    s_initCalled = 0;
    s_fieldState = 0xFFFFFFFFu;
    setenv("XENO_BOOT_DELAY", "2", 1);
    PcPort_BootUiInit(&ui);
    for (ticks = 0; ticks < 16; ticks++) {
        if (PcPort_BootUiTick(&ui, 0, 0) == PC_PORT_BOOT_TICK_NEWGAME) {
            confirmed = 1;
            break;
        }
    }
    check(confirmed, "newgame.init.then.field");
    unsetenv("XENO_FIELD_TEST");
    unsetenv("XENO_FIELD_MAP");
    unsetenv("XENO_FIELD_ENTRANCE");
    unsetenv("XENO_FIELD_CAMDIR");
    D_8006F94E = 0;
    D_8006F954 = 0xFFFF;
    D_8006F950 = 0;
    PcPort_BootEnterField();
    check(s_initCalled == 1, "newgame.init.then.field");
    check(s_fieldState == 1, "newgame.init.then.field");
    check(D_8006F94E == 14, "newgame.field.dest.map14");
    check(D_8006F954 == 0, "newgame.field.dest.map14");
    check(PcPort_NormalBootFieldMap() == 14, "newgame.field.dest.map14");
}

int main(void)
{
    test_phase_order();
    test_start_advances_title();
    test_intro_is_str_feed();
    test_intro_str_memcmp();
    test_title_graphic_memcmp();
    test_continue_without_save_does_not_freeze();
    test_continue_with_save_enters_saved_field();
    test_new_game_init_then_field();
    if (s_failures != 0)
        return 1;
    printf("BOOT MENU certificate PASS\n");
    return 0;
}
