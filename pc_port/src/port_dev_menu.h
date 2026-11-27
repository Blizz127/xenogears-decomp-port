#ifndef XENO_PORT_DEV_MENU_H
#define XENO_PORT_DEV_MENU_H
/* Public-build interface for optional host menu callbacks. The menu stays
 * closed, so game input and the ordinary save controls remain available. */
#ifdef __cplusplus
extern "C" {
#endif
enum PcPortDevKey {
    PC_DEV_OPEN, PC_DEV_UP, PC_DEV_DOWN, PC_DEV_ACCEPT, PC_DEV_BACK
};

/* Physical controller bits for PcPort_DevMenuPad (SDL GameController
 * buttons of the active pad, independent of the PS1 mapping). */
enum {
    PC_DEV_PAD_UP = 1u << 0,    /* D-pad up or left stick up */
    PC_DEV_PAD_DOWN = 1u << 1,  /* D-pad down or left stick down */
    PC_DEV_PAD_A = 1u << 2,
    PC_DEV_PAD_B = 1u << 3,
    PC_DEV_PAD_BACK = 1u << 4,  /* Back / View / Select */
    PC_DEV_PAD_START = 1u << 5
};
/* PcPort_DevMenuPad result flags. */
enum {
    PC_DEV_PASS = 0,
    PC_DEV_BLOCK_ALL = 1u << 0,     /* game sees a neutral pad (and keyboard) */
    PC_DEV_BLOCK_SELECT = 1u << 1,  /* hold Select back from the game */
    PC_DEV_BLOCK_START = 1u << 2,
    PC_DEV_FORCE_SELECT = 1u << 3,  /* replay a Select tap that was held back */
    PC_DEV_FORCE_START = 1u << 4
};

void PcPort_DevMenuInit(void);
int PcPort_DevMenuEnabled(void);
int PcPort_DevMenuInputState(unsigned *ms);   /* 0 game, 1 menu open, 2 drain */
void PcPort_DevMenuSetStatus(const char *text);
int PcPort_DevMenuOpen(void);
int PcPort_DevMenuKey(int key, int pressed);
/* Once per pad sample, from PsyX_pad.cpp.  `bits`: PC_DEV_PAD_* of the pad
 * that feeds port 1 (0 when none); `instance`: its SDL instance id (-1 none);
 * `held_any`: nonzero while any game button (pad or keyboard) is held;
 * `now_ms`: a millisecond clock.  Returns PC_DEV_* flags. */
unsigned PcPort_DevMenuPad(unsigned bits, int instance, int held_any, unsigned now_ms);

/* Page model for the overlay. */
int PcPort_DevMenuCursor(void);
int PcPort_DevMenuRows(void);
const char *PcPort_DevMenuTitle(void);
const char *PcPort_DevMenuRow(int row);
int PcPort_DevMenuRowSelectable(int row);
int PcPort_DevMenuRowAdjustable(int row);
/* Visible row window for the current cursor. Scale drops to 1 at small
 * window sizes; title and three footer lines are reserved. A viewport too
 * short for one row or too narrow for the selection rectangle returns
 * visible_rows = 0. No game/setting mutation. */
typedef struct PcPortDevMenuLayout {
    int text_scale, row_height, top, first_row, visible_rows;
} PcPortDevMenuLayout;
void PcPort_DevMenuGetLayout(int width, int height, PcPortDevMenuLayout *layout);
const char *PcPort_DevMenuStatus(void);
void PcPort_DevMenuQuickSaveResult(const char *status);
/* Nonzero while a cheat is on (persistent CHEATS marker). */
int PcPort_DevMenuCheatsActive(void);

/* Host functions the menu drives (installed by psycross_port_dev_menu.inl;
 * NULL members are shown as unavailable). */
typedef struct PcPortDevMenuHost {
    int (*fast_forward_on)(void);
    void (*fast_forward_toggle)(void);
    int (*speed)(void);
    void (*speed_cycle)(void);
    int (*recording)(void);
    void (*record_toggle)(void);
    void (*screenshot)(void);        /* taken after the menu has closed */
    int (*god_on)(void);
    int (*random_battles_on)(void);
    int (*confirm_circle)(void);     /* 1: physical Circle confirms (input.pad_confirm) */
    void (*confirm_toggle)(void);    /* applies now and saves the config */
} PcPortDevMenuHost;
void PcPort_DevMenuSetHost(const PcPortDevMenuHost *host);

/* Render features may register controls even when the menu is disabled. */
void PcPort_DevMenuRegisterEnhancement(const char *label, int (*get)(void), void (*toggle)(void));
void PcPort_DevMenuRegisterEnhancementValue(const char *label, int (*get)(void),
    void (*set)(int), int min, int max, int step, const char *unit);
int PcPort_DevMenuEnhancementCount(void);

/* Game-state actions (port_dev_menu_actions.c), reached through the cheat
 * queue as "dev <command>".  Returns 1 done, -1 refused. */
int PcPort_DevMenuAction(const char *command);

/* Data-driven verified warp / area-finish tables (port_dev_menu_warps.inc).
 * Only entries with a verification evidence path are listed; an area folder
 * without verified spots is hidden. */
typedef struct PcPortDevWarp {
    int disc;
    const char *chapter;
    const char *area;            /* folder, player-facing name */
    const char *spot;            /* named spot, player-facing name */
    int map;                     /* destination field map */
    int entrance;                /* destination entrance / door */
    unsigned progression;        /* story value recorded on the route */
    const char *evidence;        /* verification report path (required) */
} PcPortDevWarp;
typedef struct PcPortDevFinish {
    int disc;
    const char *chapter;
    const char *area;
    const char *evidence;
} PcPortDevFinish;

/* Finish Current Step (DEV_MENU_SPEC.md 7a): a data-driven walkthrough step.
 * `detect_*` is the live-state predicate (scenario word, map, and optionally
 * one script variable); `writes` are script-variable values observed in the
 * real route, applied through FieldScriptMemoryWriteU16; `next_*` is where the
 * following step happens, reached through the quick-checkpoint warp path.
 * Only steps with an evidence path (a passing sweep) are listed. */
#define PC_DEV_NO_VALUE 0xFFFFu
#define PC_DEV_NO_SCENARIO 0xFFFFFFFFu /* warp: leave the scenario word alone */
typedef struct PcPortDevStepWrite { unsigned short addr; unsigned short value; } PcPortDevStepWrite;
typedef struct PcPortDevStep {
    const char *id;              /* stable id, also the sweep target name */
    int disc;
    const char *chapter;
    const char *area;            /* Finish Area folder */
    const char *name;            /* own-words step name */
    unsigned short detect_scen;  /* scenario word while this step is current */
    int detect_map;              /* -1 = any map */
    unsigned short detect_var_addr, detect_var_value; /* PC_DEV_NO_VALUE addr = unused */
    const PcPortDevStepWrite *writes;
    int nwrites;
    int next_map, next_entrance; /* where the next step happens */
    int area_last;               /* 1: completing this step completes the area */
    const char *evidence;
} PcPortDevStep;
extern const PcPortDevStep g_PcPortDevSteps[];
extern const int g_PcPortDevStepCount;
/* Status toast for actions that run on the game thread. */
void PcPort_DevMenuSetStatus(const char *text);
extern const PcPortDevWarp g_PcPortDevWarps[];
extern const int g_PcPortDevWarpCount;
extern const PcPortDevFinish g_PcPortDevFinishes[];
extern const int g_PcPortDevFinishCount;
#ifdef __cplusplus
}
#endif
#endif
