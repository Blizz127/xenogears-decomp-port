#include "boot_menu.h"
#include "boot_assets.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/controller.h"

#ifndef BOOT_CERT_NO_STR
#include "boot_str.h"
#endif

extern void func_8001B970(void);
extern void ChangeGameState(unsigned int state);
extern unsigned short D_8006F94E;
extern unsigned short D_8006F954;
extern unsigned short D_8006F950;
extern unsigned char g_GameState[];

#define BOOT_SAVE_MAGIC 0x4F4E4558u /* 'XENO' */
#define BOOT_SAVE_STATE 0x2358

typedef struct PcPortBootSave {
    unsigned int magic;
    unsigned short map;
    unsigned short entrance;
    unsigned short camoct;
    unsigned short pad;
    unsigned char gameState[BOOT_SAVE_STATE];
} PcPortBootSave;

static const char* BootSavePath(void)
{
    const char* e = getenv("XENO_BOOT_SAVE");
    if (e && e[0])
        return e;
    return "pc_port/tests/fixtures/continue_save.bin";
}

int PcPort_BootHasSave(void)
{
    FILE* f;
    unsigned int magic = 0;
#ifdef BOOT_MUTANT_CONTINUE_SNAP
    return 0;
#else
    f = fopen(BootSavePath(), "rb");
    if (!f)
        return 0;
    if (fread(&magic, 1, 4, f) != 4) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return magic == BOOT_SAVE_MAGIC;
#endif
}

int PcPort_BootLoadContinue(void)
{
    FILE* f;
    PcPortBootSave save;

    memset(&save, 0, sizeof(save));
    f = fopen(BootSavePath(), "rb");
    if (!f)
        return 0;
    if (fread(&save, 1, sizeof(save), f) < 12) {
        fclose(f);
        return 0;
    }
    fclose(f);
    if (save.magic != BOOT_SAVE_MAGIC)
        return 0;
    D_8006F94E = save.map;
    D_8006F954 = save.entrance;
    D_8006F950 = (unsigned short)(save.camoct << 9);
    memcpy(g_GameState, save.gameState, BOOT_SAVE_STATE);
    printf("[xeno-port][boot] Continue restore map=%u entrance=%u camoct=%u\n",
           (unsigned)save.map, (unsigned)save.entrance, (unsigned)save.camoct);
    fflush(stdout);
    return 1;
}

void PcPort_BootUiInit(PcPortBootUi* ui)
{
#ifdef BOOT_MUTANT_SKIP_TITLE
    ui->phase = PC_PORT_BOOT_MOVIE;
#else
    ui->phase = PC_PORT_BOOT_TITLE;
#endif
    ui->phaseFrames = 0;
    ui->menuChoice = 0;
    ui->introStrFed = 0;
    ui->strFrame = 0;
    ui->titleGraphicLoaded = 0;
}

int PcPort_BootIntroUsedStr(const PcPortBootUi* ui)
{
    return ui->introStrFed;
}

int PcPort_BootPhaseFrames(int phase)
{
    const char* e = getenv("XENO_BOOT_DELAY");
    int base = (e && *e) ? atoi(e) : 60;

    if (base < 1)
        base = 1;
    if (phase == PC_PORT_BOOT_MOVIE) {
        const char* mf = getenv("XENO_BOOT_MOVIE_FRAMES");
        if (mf && *mf) {
            int n = atoi(mf);
            return n > 0 ? n : 1;
        }
        return base * 2 > 0 ? base * 2 : 1;
    }
    return base;
}

int PcPort_BootUiTick(PcPortBootUi* ui, unsigned pressedOnce, unsigned released)
{
    int advance = 0;
    int delay;

    ui->phaseFrames++;
    delay = PcPort_BootPhaseFrames(ui->phase);

    switch (ui->phase) {
    case PC_PORT_BOOT_TITLE:
        if ((pressedOnce & CTRL_BTN_START) || (released & CTRL_BTN_CIRCLE) ||
            ui->phaseFrames >= delay)
            advance = 1;
        break;

    case PC_PORT_BOOT_MOVIE:
#ifndef BOOT_MUTANT_SKIP_INTRO
        if (!PcPort_BootAssetsLoaded())
            PcPort_BootAssetsLoad();
        ui->introStrFed = PcPort_BootAssetsLoaded();
        ui->titleGraphicLoaded = PcPort_BootTitleUsedRetailGraphic();
#ifndef BOOT_CERT_NO_STR
        PcPort_BootStrPlayFrame(ui->strFrame);
        if ((ui->phaseFrames & 1) == 0)
            ui->strFrame++;
#endif
#else
        ui->introStrFed = 0;
#endif
        if ((pressedOnce & (CTRL_BTN_START | CTRL_BTN_CROSS)) ||
            (released & CTRL_BTN_CIRCLE) || ui->phaseFrames >= delay)
            advance = 1;
        break;

    case PC_PORT_BOOT_MENU:
        if (pressedOnce & CTRL_BTN_UP) {
            ui->menuChoice = 0;
        } else if (pressedOnce & CTRL_BTN_DOWN) {
            ui->menuChoice = 1;
        }
        {
            const char* cont = getenv("XENO_BOOT_CONTINUE");
            if (cont && cont[0] == '1' && PcPort_BootHasSave()) {
                ui->menuChoice = 1;
                if (ui->phaseFrames >= 2) {
                    printf("[xeno-port][boot] Continue -> Field\n");
                    fflush(stdout);
                    return PC_PORT_BOOT_TICK_CONTINUE;
                }
                break;
            }
        }

        if ((released & CTRL_BTN_CIRCLE) ||
            (pressedOnce & (CTRL_BTN_START | CTRL_BTN_CROSS)) ||
            ui->phaseFrames >= delay) {
            int timedOut = ui->phaseFrames >= delay;
            int confirm = !timedOut ||
                          (released & CTRL_BTN_CIRCLE) ||
                          (pressedOnce & (CTRL_BTN_START | CTRL_BTN_CROSS));
            if (ui->menuChoice != 0) {
#ifdef BOOT_MUTANT_CONTINUE_SNAP
                printf("[xeno-port][boot] Continue unavailable "
                       "(save system not ported) — use New Game\n");
                ui->menuChoice = 0;
                return 0;
#else
                if (PcPort_BootHasSave()) {
                    printf("[xeno-port][boot] Continue -> Field\n");
                    return PC_PORT_BOOT_TICK_CONTINUE;
                }
                if (!timedOut || confirm) {
                    printf("[xeno-port][boot] Continue: no save — stay on menu\n");
                    fflush(stdout);
                }
                return 0;
#endif
            }
            if (timedOut || confirm) {
                printf("[xeno-port][boot] New Game -> Field\n");
                return PC_PORT_BOOT_TICK_NEWGAME;
            }
        }
        break;
    }

    if (advance) {
        ui->phaseFrames = 0;
        if (ui->phase == PC_PORT_BOOT_TITLE) {
            ui->phase = PC_PORT_BOOT_MOVIE;
            /* Skip the opening fade-from-black so the first captured STR
             * frames already have picture. */
            ui->strFrame = 90;
#ifndef BOOT_MUTANT_SKIP_INTRO
            printf("[xeno-port][boot] intro STR\n");
            if (PcPort_BootAssetsLoad()) {
                ui->introStrFed = 1;
                ui->titleGraphicLoaded = PcPort_BootTitleUsedRetailGraphic();
            }
#ifndef BOOT_CERT_NO_STR
            PcPort_BootStrLoad();
#endif
#else
            printf("[xeno-port][boot] intro movie (skipped)\n");
#endif
        } else if (ui->phase == PC_PORT_BOOT_MOVIE) {
            ui->phase = PC_PORT_BOOT_MENU;
            ui->menuChoice = 0;
            printf("[xeno-port][boot] new game menu\n");
#ifndef BOOT_CERT_NO_STR
            /* Opening STR last picture frame is the title screen. */
            PcPort_BootStrPlayTitleFrame();
#endif
            if (PcPort_BootTitleUsedRetailGraphic())
                printf("[xeno-port][boot] title/menu graphic Opening STR last frame\n");
            fflush(stdout);
        }
    }

    return 0;
}

unsigned PcPort_NormalBootFieldMap(void)
{
#ifdef BOOT_MUTANT_LAHAN_DEST
    return 1;
#else
    return PC_PORT_NORMAL_BOOT_FIELD_MAP;
#endif
}

unsigned PcPort_NormalBootFieldEntrance(void)
{
#ifdef BOOT_MUTANT_LAHAN_DEST
    return 6;
#else
    return PC_PORT_NORMAL_BOOT_FIELD_ENTRANCE;
#endif
}

void PcPort_ApplyNormalBootFieldDefaults(void)
{
    extern unsigned short D_8006F94E;
    extern unsigned short D_8006F954;
    extern unsigned short D_8006F950;
    const char* fieldTest = getenv("XENO_FIELD_TEST");

    if (fieldTest != NULL && fieldTest[0] == '1') {
        return;
    }
    if (getenv("XENO_FIELD_MAP") == NULL) {
        D_8006F94E = (unsigned short)PcPort_NormalBootFieldMap();
    }
    if (getenv("XENO_FIELD_ENTRANCE") == NULL) {
        D_8006F954 = (unsigned short)PcPort_NormalBootFieldEntrance();
    }
    if (getenv("XENO_FIELD_CAMDIR") == NULL) {
        D_8006F950 = (unsigned short)(PC_PORT_NORMAL_BOOT_FIELD_CAMOCT << 9);
    }
}

void PcPort_BootEnterField(void)
{
    PcPort_ApplyNormalBootFieldDefaults();
#ifndef BOOT_MUTANT_SKIP_INIT
    func_8001B970();
#endif
    ChangeGameState(1);
}

void PcPort_BootEnterContinue(void)
{
    if (!PcPort_BootLoadContinue()) {
        PcPort_BootEnterField();
        return;
    }
    ChangeGameState(1);
}
