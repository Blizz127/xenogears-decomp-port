/* cheat_console.c -- cheat console on the xg_plat hook layer.
 *
 * Cheats are named commands executed on the game thread from the frame_tick
 * hook (xg_plat/mods.h), never from the thread that typed them:
 *
 *   help                        list commands
 *   god [on|off]                party protection / one-hit enemies (god_mode.c)
 *   encounters [on|off]         random battles (god_mode.c)
 *   save | load                 field quick checkpoint (quick_checkpoint.c)
 *   warp <map> <x> <z>          teleport the player once map <map> is active
 *   battle <id>                 start battle <id> from an idle field
 *   speed <1..5>                wall-clock speed (xg_plat/timing.h)
 *   bind [<button> <key>]       list or change a key binding (xg_plat/input.h)
 *   mods                        list loaded mods
 *
 * Sources of commands: the host toolbar / hotkeys (PcPort_CheatQueue), stdin
 * when cheats.console = on (XENO_CHEATS_CONSOLE=1), and PcPort_CheatExec for
 * code already on the game thread.  The per-frame cheat services (field
 * warp, battle warp) are frame_tick subscribers too, so no cheat is called
 * directly from the VSync shim any more.  Test harness switches
 * (XENO_GOD_MODE, XENO_FIELD_WARP, XENO_BATTLE_WARP_FILE) keep working and
 * stay off by default.
 */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cheat_console.h"
#include "god_mode.h"
#include "port_dev_menu.h"
#include "quick_checkpoint.h"
#include "../include/xg_plat/config.h"
#include "../include/xg_plat/mods.h"
#include "../include/xg_plat/timing.h"
#include "../include/xg_plat/input.h"

extern void PcPort_FieldWarpDiag(void);
extern void PcPort_FieldWarpArm(int map, int x, int z);
extern void PcPort_DebugBattleWarp(void);
extern void PcPort_DebugBattleWarpQueue(int id);

#define QUEUE_MAX 16
static char s_queue[QUEUE_MAX][128];
static int s_queue_count;
static pthread_mutex_t s_queue_lock = PTHREAD_MUTEX_INITIALIZER;

static int parse_onoff(const char* arg, int current)
{
    if (arg == NULL || !strcmp(arg, "toggle"))
        return !current;
    return !strcmp(arg, "on") || !strcmp(arg, "1");
}

int PcPort_CheatExec(const char* line)
{
    char buf[128];
    char* argv[6];
    int argc = 0;
    char* tok;
    char* save = NULL;

    snprintf(buf, sizeof buf, "%s", line);
    for (tok = strtok_r(buf, " \t\r\n", &save); tok && argc < 6; tok = strtok_r(NULL, " \t\r\n", &save))
        argv[argc++] = tok;
    if (argc == 0)
        return 0;
    if (!strcmp(argv[0], "help")) {
        printf("[cheat] commands: god [on|off], encounters [on|off], save, load, "
               "warp <map> <x> <z>, battle <id>, speed <1..5>, bind [<button> <key>], mods\n");
    } else if (!strcmp(argv[0], "dev") && argc >= 2) {
        char command[80];
        snprintf(command, sizeof command, "%s%s%s", argv[1], argc > 2 ? " " : "", argc > 2 ? argv[2] : "");
        return PcPort_DevMenuAction(command);
    } else if (!strcmp(argv[0], "god")) {
        if (parse_onoff(argc > 1 ? argv[1] : NULL, PcPort_GodModeEnabled()) != PcPort_GodModeEnabled())
            PcPort_GodModeToggle();
    } else if (!strcmp(argv[0], "encounters")) {
        if (parse_onoff(argc > 1 ? argv[1] : NULL, PcPort_RandomBattlesEnabled()) !=
            PcPort_RandomBattlesEnabled())
            PcPort_RandomBattlesToggle();
    } else if (!strcmp(argv[0], "save")) {
        PcPort_QuickCheckpointRequestSave();
    } else if (!strcmp(argv[0], "load")) {
        PcPort_QuickCheckpointRequestLoad();
    } else if (!strcmp(argv[0], "warp") && argc == 4) {
        PcPort_FieldWarpArm(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    } else if (!strcmp(argv[0], "battle") && argc == 2) {
        PcPort_DebugBattleWarpQueue(atoi(argv[1]));
    } else if (!strcmp(argv[0], "speed") && argc == 2) {
        xg_plat_timing_set_speed(atoi(argv[1]));
    } else if (!strcmp(argv[0], "bind") && (argc == 1 || argc == 3)) {
        int b;
        if (argc == 3) {
            for (b = 0; b < XG_PLAT_BTN_COUNT; b++)
                if (!strcmp(argv[1], xg_plat_input_button_name((XgPlatButton)b)))
                    break;
            if (b == XG_PLAT_BTN_COUNT || !xg_plat_input_bind_key((XgPlatButton)b, argv[2])) {
                printf("[cheat] bind: unknown button '%s' or key '%s'\n", argv[1], argv[2]);
                return -1;
            }
        }
        for (b = 0; b < XG_PLAT_BTN_COUNT; b++)
            printf("[cheat] %s = %s\n", xg_plat_input_button_name((XgPlatButton)b),
                   xg_plat_input_key_name((XgPlatButton)b));
    } else if (!strcmp(argv[0], "mods")) {
        int i;
        for (i = 0; i < xg_plat_mods_count(); i++)
            printf("[cheat] mod %d: %s %s\n", i, xg_plat_mods_get(i)->name,
                   xg_plat_mods_get(i)->version);
    } else {
        printf("[cheat] unknown or malformed command '%s' (try help)\n", line);
        return -1;
    }
    return 1;
}

void PcPort_CheatQueue(const char* line)
{
    pthread_mutex_lock(&s_queue_lock);
    if (s_queue_count < QUEUE_MAX) {
        char* q = s_queue[s_queue_count++];
        snprintf(q, sizeof s_queue[0], "%s", line);
        q[strcspn(q, "\r\n")] = '\0';
    }
    pthread_mutex_unlock(&s_queue_lock);
}

static void run_queue(void)
{
    char local[QUEUE_MAX][128];
    int n, i;
    pthread_mutex_lock(&s_queue_lock);
    n = s_queue_count;
    memcpy(local, s_queue, sizeof s_queue[0] * (size_t)n);
    s_queue_count = 0;
    pthread_mutex_unlock(&s_queue_lock);
    for (i = 0; i < n; i++)
        (void)PcPort_CheatExec(local[i]);
}

/* frame_tick subscribers, in the order the VSync shim used to call them. */
static void tick_field_warp(const XgPlatModEventData* e, void* u) { (void)e; (void)u; PcPort_FieldWarpDiag(); }
static void tick_battle_warp(const XgPlatModEventData* e, void* u) { (void)e; (void)u; PcPort_DebugBattleWarp(); }
static void tick_console(const XgPlatModEventData* e, void* u) { (void)e; (void)u; run_queue(); }

static void* stdin_reader(void* arg)
{
    char line[128];
    (void)arg;
    while (fgets(line, sizeof line, stdin) != NULL)
        PcPort_CheatQueue(line);
    return NULL;
}

void PcPort_CheatConsoleInit(void)
{
    static int done;
    if (done)
        return;
    done = 1;
    xg_plat_mods_subscribe(XG_PLAT_EVENT_FRAME_TICK, tick_field_warp, NULL);
    xg_plat_mods_subscribe(XG_PLAT_EVENT_FRAME_TICK, tick_battle_warp, NULL);
    xg_plat_mods_subscribe(XG_PLAT_EVENT_FRAME_TICK, tick_console, NULL);
    if (xg_plat_config_int("cheats.console", 0)) {
        pthread_t t;
        if (pthread_create(&t, NULL, stdin_reader, NULL) == 0) {
            pthread_detach(t);
            printf("[cheat] console reading commands from stdin (type 'help')\n");
        }
    }
}
