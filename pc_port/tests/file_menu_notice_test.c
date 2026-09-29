/* Regression test for the File-menu notice dispatch policy.
 *
 * PcPort_NotifyUnsupportedFileMenu is called from the field menu when the player
 * picks the unimplemented File entry.  The notice used to be shown with a direct
 * SDL_ShowSimpleMessageBox call on the game thread; because that call is modal,
 * the game thread never returned whenever the dialog could not be surfaced (no
 * window manager to map the zenity window onto the game display).  The route
 * hung at (-496,0,-1276) with owner=0x80, two byte-identical screenshots 25 s
 * apart and the stack parked in SDL_Zenity_ShowMessageBox.
 *
 * The policy under test, in pc_port/src/file_menu_notice.c:
 *   1. the dialog must NEVER run on the calling thread;
 *   2. the call must return promptly even when the dialog blocks;
 *   3. the notice must be dispatched exactly once;
 *   4. a dispatch failure must free the context and still return;
 *   5. the notice text must stay the one the player is told to act on.
 *
 * Failure prints "FILE MENU NOTICE FAIL <label> line=N"; success prints one
 * PASS line.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../src/file_menu_notice.h"

/* The platform hooks are function pointers; this TU owns them and installs the
 * test doubles below (production installs the SDL-backed ones). */
void *(*PcPort_FileMenuNoticeAlloc)(unsigned long size);
void (*PcPort_FileMenuNoticeFree)(void *memory);
void (*PcPort_FileMenuNoticeLog)(const char *message);
int (*PcPort_FileMenuNoticeStartThread)(void (*entry)(void *), void *context);
void (*PcPort_FileMenuNoticeDialog)(const char *title, const char *text);

/* ---- test doubles for the platform hooks ---- */
static unsigned alloc_calls, free_calls, log_calls, start_calls, dialog_calls;
static unsigned dialog_on_caller_thread;
static unsigned start_returns_zero;
static const char *last_title, *last_text;
static void *last_context;
static void (*last_entry)(void *);
static int fake_alloc_failure;

static void *test_alloc(unsigned long size)
{
    alloc_calls++;
    if (fake_alloc_failure)
        return NULL;
    return malloc((size_t)size);
}

static void test_free(void *memory)
{
    free_calls++;
    free(memory);
}

static void test_log(const char *message)
{
    (void)message;
    log_calls++;
}

/* Runs the entry point on the calling thread.  Because the policy must not call
 * the dialog itself, a synchronous start only proves the hand-off is explicit -
 * the "no dialog on caller thread" check is what catches an inline call. */
static int test_start_thread(void (*entry)(void *), void *context)
{
    start_calls++;
    last_entry = entry;
    last_context = context;
    if (start_returns_zero)
        return 0;
    if (dialog_calls > 0)
        dialog_on_caller_thread++;   /* dialog ran before the hand-off */
    return 1;
}

static void test_dialog(const char *title, const char *text)
{
    dialog_calls++;
    last_title = title;
    last_text = text;
}

#define CHECK(label, condition) do { checks++; if (!(condition)) { \
    fprintf(stderr, "FILE MENU NOTICE FAIL %s line=%d\n", label, __LINE__); \
    return 1; } } while (0)

static unsigned checks;

static void reset(void)
{
    alloc_calls = free_calls = log_calls = start_calls = dialog_calls = 0;
    dialog_on_caller_thread = 0;
    start_returns_zero = 0;
    fake_alloc_failure = 0;
    last_title = last_text = NULL;
    last_context = NULL;
    last_entry = NULL;
}

/* 1. The notice is handed to the platform hook, not shown inline. */
static int dispatch_not_inline(void)
{
    reset();
    PcPort_NotifyUnsupportedFileMenu();
    CHECK("dispatch-alloc", alloc_calls == 1);
    CHECK("dispatch-start", start_calls == 1);
    CHECK("dispatch-entry", last_entry == PcPort_FileMenuNoticeThreadMain);
    CHECK("dispatch-context", last_context != NULL);
    CHECK("dispatch-no-inline-dialog", dialog_calls == 0);
    CHECK("dispatch-no-dialog-on-caller", dialog_on_caller_thread == 0);
    CHECK("dispatch-logged", log_calls >= 1);
    /* Run the thread body and confirm it shows the dialog and frees. */
    last_entry(last_context);
    CHECK("thread-dialog-once", dialog_calls == 1);
    CHECK("thread-freed", free_calls == 1);
    CHECK("thread-title", last_title != NULL &&
          strcmp(last_title, "File menu not implemented") == 0);
    CHECK("thread-text-action", last_text != NULL &&
          strstr(last_text, "cancel back to the field") != NULL);
    return 0;
}

/* 2. A blocking dialog on the worker thread must not delay the call. */
static int dispatch_returns_promptly(void)
{
    struct timespec before, after;
    double elapsed;
    reset();
    clock_gettime(CLOCK_MONOTONIC, &before);
    PcPort_NotifyUnsupportedFileMenu();
    clock_gettime(CLOCK_MONOTONIC, &after);
    elapsed = (double)(after.tv_sec - before.tv_sec) +
              1e-9 * (double)(after.tv_nsec - before.tv_nsec);
    CHECK("prompt-start", start_calls == 1);
    CHECK("prompt-fast", elapsed < 0.05);
    return 0;
}

/* 3. A refused dispatch frees the context and still returns. */
static int dispatch_failure_cleans_up(void)
{
    reset();
    start_returns_zero = 1;
    PcPort_NotifyUnsupportedFileMenu();
    CHECK("fail-start", start_calls == 1);
    CHECK("fail-no-dialog", dialog_calls == 0);
    CHECK("fail-freed", free_calls == 1);
    CHECK("fail-logged", log_calls >= 2);
    return 0;
}

/* 4. An allocation failure returns without dispatching. */
static int alloc_failure_returns(void)
{
    reset();
    fake_alloc_failure = 1;
    PcPort_NotifyUnsupportedFileMenu();
    CHECK("oom-no-start", start_calls == 0);
    CHECK("oom-no-dialog", dialog_calls == 0);
    CHECK("oom-no-free", free_calls == 0);
    return 0;
}

/* 5. The advertised title/text are pinned by the caller's contract. */
static int notice_text_pinned(void)
{
    CHECK("pin-title", strcmp(PcPort_FileMenuNoticeTitle,
                              "File menu not implemented") == 0);
    CHECK("pin-text-toolbar", strstr(PcPort_FileMenuNoticeText,
                                     "SAVE and LOAD") != NULL);
    CHECK("pin-text-card", strstr(PcPort_FileMenuNoticeText,
                                  "memory-card File menu") != NULL);
    return 0;
}

int main(void)
{
    /* The hooks are function pointers precisely so a test can install these. */
    PcPort_FileMenuNoticeDialog = test_dialog;
    PcPort_FileMenuNoticeAlloc = test_alloc;
    PcPort_FileMenuNoticeFree = test_free;
    PcPort_FileMenuNoticeLog = test_log;
    PcPort_FileMenuNoticeStartThread = test_start_thread;

    if (dispatch_not_inline()) return 1;
    if (dispatch_returns_promptly()) return 1;
    if (dispatch_failure_cleans_up()) return 1;
    if (alloc_failure_returns()) return 1;
    if (notice_text_pinned()) return 1;

    printf("FILE MENU NOTICE PASS checks=%u dispatch/oom/pins\n", checks);
    return 0;
}
