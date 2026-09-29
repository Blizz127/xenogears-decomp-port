/* Behavioral regression for the no-scene VRAM display-page selection.
 *
 * PsyCross deliberately keeps activeDispEnv one PutDispEnv behind when the
 * display page alternates.  That lag is useful to its primitive renderer, but
 * a vblank scanout must show the page just submitted in currentDispEnv. */
#include <stdio.h>
#include <string.h>

typedef struct {
    short x;
    short y;
    short w;
    short h;
} TestRect;

typedef struct {
    TestRect disp;
    int isinter;
    int isrgb24;
} TestDispEnv;

static TestDispEnv activeDispEnv;
static TestDispEnv currentDispEnv;
static int failures;

enum TestScanoutKind {
    TEST_SCANOUT_BLACK,
    TEST_SCANOUT_VRAM
};

static void check(const char* label, int condition)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", label);
        failures++;
    }
}

/* Exact state transition used by PsyCross PutDispEnv. */
static void submit_display_env(const TestDispEnv* env)
{
    if (env->isinter != currentDispEnv.isinter ||
        env->disp.y == currentDispEnv.disp.y) {
        activeDispEnv = currentDispEnv = *env;
    } else {
        activeDispEnv = currentDispEnv;
        currentDispEnv = *env;
    }
}

static const TestDispEnv* no_scene_vblank_scanout(void)
{
#ifdef XENO_ACTIVE_DISPENV_MUTANT
    return &activeDispEnv;
#else
    return &currentDispEnv;
#endif
}

/* SetDispMask controls the PS1 display output, not VRAM contents.  A disabled
 * display therefore scans out black while leaving a stale page untouched for
 * a later re-enable. */
static enum TestScanoutKind no_scene_vblank_kind(int displayEnabled)
{
#ifdef XENO_IGNORE_DISP_MASK_MUTANT
    (void)displayEnabled;
    return TEST_SCANOUT_VRAM;
#else
    return displayEnabled ? TEST_SCANOUT_VRAM : TEST_SCANOUT_BLACK;
#endif
}

static TestDispEnv page(int y)
{
    TestDispEnv env;

    memset(&env, 0, sizeof(env));
    env.disp.y = (short)y;
    env.disp.w = 320;
    env.disp.h = 240;
    env.isrgb24 = 1;
    return env;
}

int main(void)
{
    TestDispEnv page0 = page(0);
    TestDispEnv page240 = page(240);
    const TestDispEnv* scanout;
    int pageStamp[2] = { 0x434c454e, 0x55564f52 }; /* CLEN / UVOR */

    memset(&activeDispEnv, 0, sizeof(activeDispEnv));
    memset(&currentDispEnv, 0, sizeof(currentDispEnv));

    submit_display_env(&page0);
    submit_display_env(&page240);
    check("alternating PutDispEnv deliberately lags active page",
          activeDispEnv.disp.y == 0 && currentDispEnv.disp.y == 240);
    scanout = no_scene_vblank_scanout();
    check("first movie flip scans out submitted y=240 page",
          scanout->disp.y == 240 && scanout->isrgb24 == 1);
    check("repeated vblank keeps the completed submitted page",
          no_scene_vblank_scanout()->disp.y == 240);

    submit_display_env(&page0);
    check("second movie flip keeps one-page active lag",
          activeDispEnv.disp.y == 240 && currentDispEnv.disp.y == 0);
    scanout = no_scene_vblank_scanout();
    check("second movie flip scans out submitted y=0 page",
          scanout->disp.y == 0 && scanout->isrgb24 == 1);

    /* Exact movie-to-field transition observed in the port: field UI uploads
     * have repurposed the lagged y=240 page, while current y=0 still contains
     * the final completed movie frame. */
    pageStamp[0] = 0x434c454e;
    pageStamp[1] = 0x434f5252; /* CORR: overwritten/repurposed VRAM */
    check("transition vblank selects clean final movie page",
          pageStamp[scanout->disp.y / 240] == 0x434c454e);

    check("SetDispMask(0) blanks a no-scene scanout",
          no_scene_vblank_kind(0) == TEST_SCANOUT_BLACK);
    check("blank scanout does not rewrite the selected VRAM page",
          pageStamp[scanout->disp.y / 240] == 0x434c454e);
    check("SetDispMask(1) resumes VRAM scanout",
          no_scene_vblank_kind(1) == TEST_SCANOUT_VRAM);

    if (failures != 0) {
        fprintf(stderr, "Display-env page regression: FAIL (%d check(s))\n",
                failures);
        return 1;
    }

    puts("Display-env page regression: PASS");
    return 0;
}
