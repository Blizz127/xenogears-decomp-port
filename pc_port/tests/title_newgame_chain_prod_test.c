/* Production-linked title -> New Game chain certificate.
 *
 * Links the shipped field-script handlers the title map (490) executes and
 * the shipped title-menu input/choice code, and drives them the way the retail
 * boot chain does.  Not a reimplementation: these are the functions the game
 * links (src/field/main/misc.c, src/field/main/misc11.c, src/menu/main/misc.c).
 *
 *   FE60  func_8008EC30   arms the attract-STR transition (D_800ADB70=1) and
 *                         keeps bit 0x80 of arg7 in D_800ADB80 -- the flag
 *                         func_800A7C58 needs before it polls Circle.
 *   FE61  func_8008E9F8   spins (ip-1) until func_800A7C58 publishes
 *                         D_800ADB7C, then consumes the flag and advances.
 *   FE57  func_800937E0   requests menu 2 (title) in D_800ADB64 and arms the
 *                         WAIT_MENU counter D_8004F350.
 *   func_801C7D78         decodes the pad queue: UP -> 3, DOWN -> 1,
 *                         Circle RELEASE -> 4 (a bare press is not a confirm).
 *   func_801C58EC         title loop: UP moves Continue (1) -> New Game (2);
 *                         confirm runs func_801C531C(7) -> entry 9 ->
 *                         func_8001B970 and leaves the loop with
 *                         D_800594D0 still 0 (not the idle-timeout exit).
 *
 * The retail argument encoding is mirrored for the handlers' operand reads:
 * FieldScriptVMGetArgument treats bit 0x8000 as "immediate" (misc9.c:735).
 *
 * Mutants (pc_port/tests/run_title_newgame_chain.sh) live in the production
 * sources behind TITLE_CHAIN_MUTANT_* defines; each must build and then fail
 * here with the named ASSERTION on stderr. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "system/menu.h"

/* ---- handlers under test ------------------------------------------------ */
void func_8008EC30(void);   /* FE60 */
void func_8008E9F8(void);   /* FE61 */
void func_800937E0(void);   /* FE57 */
void func_801C7D78(void);   /* menu input decode */
void func_801C58EC(void);   /* title loop */

/* ---- field-side globals the handlers touch (all extern in the TUs) ------ */
ActorData g_TestActorData;
ActorData* g_FieldScriptVMCurActor = &g_TestActorData;
s32 D_800B00C0;
s32 D_800ADBDC;
s32 D_800ADB70;
s32 D_800ADB74;
s32 D_800ADB7C;
s32 D_800ADB80;
s32 D_800ADB64 = 0xFF;
s32 D_8004F350;
s16 D_800C3A20, D_800C3A22, D_800C3A24, D_800C3A26, D_800C3A28, D_800C3A2A;
s16 D_800C3A2C, D_800C3A2E, D_800C3A30, D_800C3A32, D_800C3A34, D_800C3A36;
s16 D_800C3A38, D_800C3A3A;

/* ---- menu-side globals ---------------------------------------------------- */
SystemMenu* g_Menu;
u8 D_80059460;
u8 D_800594D0;
u8 D_801E977A;
u8 D_801E9784;
u8 D_801E96A4;
u8 D_801EA1D4[64];
u8 D_801EA530[64];
u8 D_801E9E84[64];
s32 D_80059488;
u16 g_C1ButtonStatePressedOnce;
u16 g_C1ButtonStateReleased;

static int s_failures;

static void check(int condition, const char* name)
{
    if (!condition) {
        fprintf(stderr, "ASSERTION %s\n", name);
        s_failures++;
    }
}

/* ---- script fixture and the retail operand encoding ---------------------- */
static u8 s_script[64];

static void put_imm16(int offset, unsigned value)
{
    /* Retail immediates carry bit 0x8000 (FieldScriptVMGetArgument strips it;
     * a clear bit means "script variable" and is never used by this chain). */
    unsigned encoded = 0x8000u | (value & 0x7FFFu);
    s_script[offset] = (u8)(encoded & 0xFF);
    s_script[offset + 1] = (u8)(encoded >> 8);
}

int FieldScriptVMGetInstructionArgument(int argumentIndex)
{
    const u8* p = s_script + g_FieldScriptVMCurActor->scriptInstructionPointer +
                  argumentIndex;
    return p[0] | (p[1] << 8);
}

int FieldScriptVMGetArgument(int index)
{
    int argument = FieldScriptVMGetInstructionArgument(index);
    if (!(argument & 0x8000)) {
        check(0, "fixture.only.immediate.operands");
        return 0;
    }
    return argument & 0x7FFF;
}

/* ---- controller / sound / menu leaf stubs -------------------------------- */
typedef struct { u16 pressedOnce; u16 released; } PadState;
static PadState s_padQueue[8];
static int s_padQueueCount;
static int s_padQueueRead;

static void pad_queue_reset(void)
{
    s_padQueueCount = 0;
    s_padQueueRead = 0;
}

static void pad_queue_push(u16 pressedOnce, u16 released)
{
    s_padQueue[s_padQueueCount].pressedOnce = pressedOnce;
    s_padQueue[s_padQueueCount].released = released;
    s_padQueueCount++;
}

int ControllerPopState(void)
{
    if (s_padQueueRead >= s_padQueueCount) {
        return 0;
    }
    g_C1ButtonStatePressedOnce = s_padQueue[s_padQueueRead].pressedOnce;
    g_C1ButtonStateReleased = s_padQueue[s_padQueueRead].released;
    s_padQueueRead++;
    return 1;
}

int ControllerGetType(int port) { (void)port; return 1; }
s32 func_80036410(void) { return 0; }
void ControllerResetState(void) {}
void SoundMuteAllSpuChannels(void) {}
void SoundEnableAllSpuChannels(void) {}
void func_80039DB8(s32 id) { (void)id; }
int ArchiveGetDiscNumber(void) { return 1; }

/* Title-screen resource / draw leaves (weakened in the linked object). */
void func_801E8474(s32 a, void* b) { (void)a; (void)b; }
void func_801E8018(s32 a, void* b, void* c, void* d) { (void)a; (void)b; (void)c; (void)d; }
void func_801D22C4(void) {}
static int s_titleCleanup;
void func_801E8044(s32 a, void* b) { (void)a; (void)b; s_titleCleanup++; }
void func_801E8978(s32 a, s32 b, void* c) { (void)a; (void)b; (void)c; }
void func_801E8070(s32 a, void* b, void* c, void* d, void* e, s32 f, s32 g, s32 h)
{
    (void)a; (void)b; (void)c; (void)d; (void)e; (void)f; (void)g; (void)h;
}
void func_801D3674(void) {}
void func_801E3088(s32 a) { (void)a; }
void func_801D29A8(u8 a, u8 b) { (void)a; (void)b; }
void func_801D1EB0(void) {}

/* func_801C531C entries other than New Game. */
static int s_continueCalls;
static int s_newGameCalls;
static int s_optionsCalls;
s32 func_801D9F98(s32 a, s32 b) { (void)a; (void)b; s_continueCalls++; return 0; }
s32 func_801E23CC(void) { return 0; }
s32 func_801DE29C(s32 a, s32 b) { (void)a; (void)b; return 0; }
s32 func_801DBE54(void) { return 0; }
s32 func_801E0F78(u8 a, u8 b) { (void)a; (void)b; return 0; }
s32 func_801E2BE4(void) { return 0; }
s32 func_801D9808(void) { s_optionsCalls++; return 0; }
void func_8001B970(void) { s_newGameCalls++; }

/* The title loop's per-frame pump (weakened in the linked object): feed one
 * scripted MENU_INPUT_* code per call, note the cursor position when the
 * confirm code is delivered, and drive retail's disc-1 idle timeout once the
 * script is exhausted so a loop that never leaves still terminates. */
static const u8* s_pumpScript;
static int s_pumpLength;
static int s_pumpCalls;
static int s_choiceAtConfirm = -1;
static int s_checkpointReady;
int PcPort_QuickCheckpointPoll(void) { return s_checkpointReady; }

void func_801C7BF4(void)
{
    s_pumpCalls++;
    if (s_pumpCalls <= s_pumpLength) {
        g_Menu->input = s_pumpScript[s_pumpCalls - 1];
        if (g_Menu->input == 4) {
            s_choiceAtConfirm = g_Menu->menu1Choice;
        }
    } else {
        g_Menu->input = 8;
        g_Menu->unk2D8 = 0x259;
    }
}

/* ---- fixtures -------------------------------------------------------------- */
static void reset_field_fixture(void)
{
    memset(&g_TestActorData, 0, sizeof(g_TestActorData));
    memset(s_script, 0, sizeof(s_script));
    D_800B00C0 = 0;
    D_800ADBDC = 1;
    D_800ADB70 = 0;
    D_800ADB74 = -1;
    D_800ADB7C = 0;
    D_800ADB80 = -1;
    D_800ADB64 = 0xFF;
    D_8004F350 = 0;
    D_800C3A20 = D_800C3A2A = D_800C3A2E = -1;
    D_800C3A36 = -1;
    D_800C3A3A = -1;
}

static void reset_menu_fixture(void)
{
    free(g_Menu ? g_Menu->unk32C : NULL);
    free(g_Menu ? g_Menu->pManager : NULL);
    free(g_Menu ? g_Menu->pSelectionMenu : NULL);
    free(g_Menu ? g_Menu->unk348 : NULL);
    free(g_Menu);
    g_Menu = calloc(1, sizeof(SystemMenu));
    g_Menu->unk32C = calloc(1, sizeof(MenuUnk2));
    g_Menu->pManager = calloc(1, sizeof(MenuManager));
    g_Menu->pSelectionMenu = calloc(1, sizeof(MenuSelectionMenu));
    g_Menu->unk348 = calloc(1, sizeof(MenuUnk1));
    g_Menu->unk32A = 0;          /* no nav blips */
    g_Menu->input = 8;
    D_80059460 = 2;              /* MenuExecute slot 2: title */
    D_800594D0 = 0;
    D_801E9784 = 0;
    pad_queue_reset();
    s_continueCalls = 0;
    s_newGameCalls = 0;
    s_optionsCalls = 0;
    s_pumpCalls = 0;
    s_checkpointReady = 0;
    s_titleCleanup = 0;
    s_choiceAtConfirm = -1;
}

/* ---- FE60 ------------------------------------------------------------------ */
static void run_fe60(unsigned strFile, unsigned start, unsigned end, unsigned arg7,
                     int ip)
{
    s_script[ip - 1] = 0xFE;
    s_script[ip] = 0x60;
    put_imm16(ip + 1, strFile);
    put_imm16(ip + 3, start);
    put_imm16(ip + 5, end);
    put_imm16(ip + 7, arg7);
    g_FieldScriptVMCurActor->scriptInstructionPointer = (u_short)ip;
    func_8008EC30();
}

static void test_fe60_arms_transition(void)
{
    reset_field_fixture();
    /* Title map 490's call: STR file index, frame range, type 1 | 0x80. */
    run_fe60(0x0F, 1, 0x12C, 0x81, 16);

    check(D_800C3A20 == 0x0F, "fe60.reads.str.file");
    check(D_800C3A2A == 1, "fe60.reads.start.frame");
    check(D_800C3A2E == 0x12C, "fe60.reads.end.frame");
    check(D_800ADB80 == 0x80, "fe60.keeps.circle.interrupt.bit");
    check(D_800ADB74 == 0 && D_800C3A36 == 0, "fe60.type1.selects.mode0");
    check(D_800C3A3A == 0, "fe60.not.looping");
    check(D_800C3A38 == (s16)0xFF, "fe60.fade.sentinel");
    check(D_800ADB70 == 1, "fe60.arms.transition");
    check(D_800B00C0 == 1, "fe60.yields.vm");
    check(g_FieldScriptVMCurActor->scriptInstructionPointer == 16 + 9,
          "fe60.advances.9");
}

static void test_fe60_types_and_busy_spin(void)
{
    reset_field_fixture();
    run_fe60(3, 0, 10, 0x42, 16);          /* type 2, bit 0x40 only */
    check(D_800ADB74 == 0 && D_800C3A36 == 1, "fe60.type2.selects.rgb24");
    check(D_800ADB80 == 0x40, "fe60.masks.arg7.to.c0");

    reset_field_fixture();
    run_fe60(3, 0, 10, 0x00, 16);          /* type 0 */
    check(D_800ADB74 == 1 && D_800C3A36 == 0, "fe60.type0.selects.mode1");
    check(D_800C3A22 == 0x140 && D_800C3A28 == 0x100, "fe60.type0.rects");

    reset_field_fixture();
    D_800ADBDC = 0;                        /* previous transition still busy */
    run_fe60(3, 0, 10, 0x81, 16);
    check(g_FieldScriptVMCurActor->scriptInstructionPointer == 15,
          "fe60.spins.while.transition.busy");
    check(D_800ADB70 == 0, "fe60.busy.does.not.arm");
    check(D_800B00C0 == 1, "fe60.busy.yields.vm");
}

/* ---- FE61 ------------------------------------------------------------------ */
static void test_fe61_waits_then_releases(void)
{
    reset_field_fixture();
    s_script[24] = 0xFE;
    s_script[25] = 0x61;
    g_FieldScriptVMCurActor->scriptInstructionPointer = 25;

    D_800ADB7C = 0;
    func_8008E9F8();
    check(g_FieldScriptVMCurActor->scriptInstructionPointer == 24,
          "fe61.spins.until.flag");
    check(D_800B00C0 == 1, "fe61.yields.vm");

    /* func_800A7C58 (FE60 body) publishes the flag before its attract loop. */
    g_FieldScriptVMCurActor->scriptInstructionPointer = 25;
    D_800ADB7C = 1;
    func_8008E9F8();
    check(g_FieldScriptVMCurActor->scriptInstructionPointer == 26 && D_800ADB7C == 0,
          "fe61.releases.on.flag");
}

/* ---- FE57 ------------------------------------------------------------------ */
static void test_fe57_requests_title_menu(void)
{
    reset_field_fixture();
    s_script[30] = 0xFE;
    s_script[31] = 0x57;
    g_FieldScriptVMCurActor->scriptInstructionPointer = 31;

    func_800937E0();
    check(D_800ADB64 == 2, "fe57.requests.title.menu.2");
    check(D_8004F350 == 1, "fe57.arms.wait.menu");
    check(D_800B00C0 == 1, "fe57.yields.vm");
    check(g_FieldScriptVMCurActor->scriptInstructionPointer == 32, "fe57.advances.1");
}

/* ---- menu input decode --------------------------------------------------- */
static void test_menu_input_decode(void)
{
    reset_menu_fixture();

    pad_queue_reset();
    func_801C7D78();
    check(g_Menu->input == 8, "menu.idle.is.8");

    pad_queue_reset();
    pad_queue_push(0x1000, 0);             /* UP pressed */
    func_801C7D78();
    check(g_Menu->input == 3, "menu.up.decodes.to.3");

    pad_queue_reset();
    pad_queue_push(0x4000, 0);             /* DOWN pressed */
    func_801C7D78();
    check(g_Menu->input == 1, "menu.down.decodes.to.1");

    pad_queue_reset();
    pad_queue_push(0x0020, 0);             /* Circle pressed, still held */
    func_801C7D78();
    check(g_Menu->input == 8, "menu.confirm.not.on.circle.press");

    pad_queue_reset();
    pad_queue_push(0, 0x0020);             /* Circle released */
    func_801C7D78();
    check(g_Menu->input == 4, "menu.confirm.on.circle.release");

    pad_queue_reset();
    pad_queue_push(0, 0x0040);             /* Cross released = cancel */
    func_801C7D78();
    check(g_Menu->input == 5, "menu.cancel.on.cross.release");

    /* The queue can hold several vblanks of state: the first event wins. */
    pad_queue_reset();
    pad_queue_push(0, 0);
    pad_queue_push(0, 0);
    pad_queue_push(0x1000, 0);
    func_801C7D78();
    check(g_Menu->input == 3, "menu.drains.queue.to.first.event");
}

/* ---- title loop: Continue -> UP -> New Game -> confirm ------------------ */
static void test_title_loop_new_game(void)
{
    static const u8 script[] = { 8, 3, 8, 4 };

    reset_menu_fixture();
    g_Menu->menu1Choice = 1;               /* retail default: Continue */
    g_Menu->unk337 = 0xFF;
    s_pumpScript = script;
    s_pumpLength = (int)sizeof(script);

    func_801C58EC();

    check(s_choiceAtConfirm == 2, "title.up.moves.continue.to.newgame");
    check(s_newGameCalls == 1, "title.confirm.choice2.starts.new.game");
    check(s_continueCalls == 0 && s_optionsCalls == 0,
          "title.confirm.does.not.open.other.entries");
    check(D_800594D0 == 0, "title.exits.by.new.game.not.timeout");
    check(s_pumpCalls == (int)sizeof(script), "title.loop.exits.after.new.game");
    check(g_Menu->menu1Choice == 2, "title.cursor.left.on.new.game");
}

/* The default cursor (Continue) confirmed as-is must NOT start a new game --
 * that is the func_801D9F98 save/load path -- and the loop must keep running
 * (the port's screen stub returns 0, retail returns to the title loop too). */
static void test_title_loop_default_is_continue(void)
{
    static const u8 script[] = { 8, 4 };

    reset_menu_fixture();
    g_Menu->menu1Choice = 1;
    g_Menu->unk337 = 0xFF;
    s_pumpScript = script;
    s_pumpLength = (int)sizeof(script);

    func_801C58EC();

    check(s_choiceAtConfirm == 1, "title.default.cursor.is.continue");
    check(s_continueCalls == 1, "title.confirm.default.opens.continue");
    check(s_newGameCalls == 0, "title.confirm.default.does.not.start.new.game");
    check(D_800594D0 == 1 && s_pumpCalls > (int)sizeof(script),
          "title.idle.timeout.exit");
}

static void test_title_checkpoint_handoff(void)
{
    reset_menu_fixture();
    g_Menu->menu1Choice = 1;
    s_checkpointReady = 1;
    func_801C58EC();
    check(s_pumpCalls == 1, "title.checkpoint.exits.without.idle.timeout");
    check(s_titleCleanup == 1, "title.checkpoint.cleans.up");
    check(s_newGameCalls == 0 && s_continueCalls == 0,
          "title.checkpoint.does.not.execute.menu.choice");
    check(D_800594D0 == 0, "title.checkpoint.does.not.select.attract.mode");
}

int main(void)
{
    test_fe60_arms_transition();
    test_fe60_types_and_busy_spin();
    test_fe61_waits_then_releases();
    test_fe57_requests_title_menu();
    test_menu_input_decode();
    test_title_loop_new_game();
    test_title_loop_default_is_continue();
    test_title_checkpoint_handoff();

    if (s_failures != 0) {
        fprintf(stderr, "TITLE NEWGAME CHAIN certificate FAIL (%d)\n", s_failures);
        return 1;
    }
    printf("TITLE NEWGAME CHAIN certificate PASS\n");
    return 0;
}
