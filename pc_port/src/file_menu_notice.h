/* Host notice shown when the player picks the unimplemented File (memory card)
 * entry in the field menu.
 *
 * The policy lives here, free of SDL, so it can be unit tested: the notice must
 * be dispatched through the platform hooks and MUST NOT run on the calling
 * thread, because the native dialog is modal.  When the dialog cannot be
 * surfaced (no window manager to map the zenity window onto the game's display)
 * a synchronous call parks the game thread forever - the frame stops being
 * redrawn and no input is ever read again, which is precisely how the Blackmoon
 * Forest route hung at (-496,0,-1276) with `owner=0x80`.
 */
#ifndef PC_PORT_FILE_MENU_NOTICE_H
#define PC_PORT_FILE_MENU_NOTICE_H

#ifdef __cplusplus
extern "C" {
#endif

/* Platform hooks, installed by the PsyCross host renderer (where SDL lives) and
 * overridable by tests.  A NULL hook degrades to logging, never to a crash. */
extern void *(*PcPort_FileMenuNoticeAlloc)(unsigned long size);
extern void (*PcPort_FileMenuNoticeFree)(void *memory);
extern void (*PcPort_FileMenuNoticeLog)(const char *message);
/* Must start `entry(context)` on another thread and return nonzero on success
 * WITHOUT waiting for it.  Zero means "could not dispatch". */
extern int (*PcPort_FileMenuNoticeStartThread)(void (*entry)(void *),
                                               void *context);
/* The dialog itself - modal on every platform, so it only ever runs off the
 * game thread. */
extern void (*PcPort_FileMenuNoticeDialog)(const char *title, const char *text);

/* Entry point called from the field menu.  Never blocks on the dialog. */
void PcPort_NotifyUnsupportedFileMenu(void);

/* Exposed for the regression test: the thread body the platform hook runs. */
void PcPort_FileMenuNoticeThreadMain(void *context);

extern const char *const PcPort_FileMenuNoticeTitle;
extern const char *const PcPort_FileMenuNoticeText;

#ifdef __cplusplus
}
#endif

#endif /* PC_PORT_FILE_MENU_NOTICE_H */
