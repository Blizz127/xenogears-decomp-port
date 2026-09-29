#ifndef PC_PORT_BOOT_MENU_H
#define PC_PORT_BOOT_MENU_H

/* Port-side retail boot path: title -> intro STR -> New Game / Continue.
 * Shared by PcPort_BootMain and the boot certificate. */

enum {
    PC_PORT_BOOT_TITLE = 0,
    PC_PORT_BOOT_MOVIE = 1,
    PC_PORT_BOOT_MENU = 2
};

enum {
    PC_PORT_BOOT_TICK_NONE = 0,
    PC_PORT_BOOT_TICK_NEWGAME = 1,
    PC_PORT_BOOT_TICK_CONTINUE = 2
};

typedef struct PcPortBootUi {
    int phase;
    int phaseFrames;
    int menuChoice; /* 0 = New Game, 1 = Continue */
    int introStrFed;
    int strFrame;
    int titleGraphicLoaded;
} PcPortBootUi;

void PcPort_BootUiInit(PcPortBootUi* ui);
int PcPort_BootPhaseFrames(int phase);
/* Advance one boot UI frame.
 * 0 = keep looping, 1 = New Game, 2 = Continue-from-save. */
int PcPort_BootUiTick(PcPortBootUi* ui, unsigned pressedOnce, unsigned released);
int PcPort_BootIntroUsedStr(const PcPortBootUi* ui);
int PcPort_BootTitleUsedRetailGraphic(void);
const unsigned char* PcPort_BootIntroStrPayload(int* size);
const unsigned char* PcPort_BootTitleGraphicPayload(int* size);
int PcPort_BootIntroStrLba(void);
int PcPort_BootIntroStrFileSize(void);
int PcPort_BootHasSave(void);
int PcPort_BootLoadContinue(void);
/* Retail New Game: gamestate-template init, then enter Field (state 1). */
void PcPort_BootEnterField(void);
void PcPort_BootEnterContinue(void);

/* Normal-boot Field destination (opening room). Field-test / XENO_FIELD_*
 * env still override; these are the values PcPort_BootEnterField applies
 * when those are unset. */
enum {
    PC_PORT_NORMAL_BOOT_FIELD_MAP = 14,
    PC_PORT_NORMAL_BOOT_FIELD_ENTRANCE = 0,
    PC_PORT_NORMAL_BOOT_FIELD_CAMOCT = 7
};
unsigned PcPort_NormalBootFieldMap(void);
unsigned PcPort_NormalBootFieldEntrance(void);
void PcPort_ApplyNormalBootFieldDefaults(void);

#endif
