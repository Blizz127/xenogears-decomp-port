#include "file_menu_notice.h"

const char *const PcPort_FileMenuNoticeTitle = "File menu not implemented";
const char *const PcPort_FileMenuNoticeText =
    "The in-game memory-card File menu is not implemented in this port yet.\n\n"
    "Close this message and cancel back to the field. Use the toolbar's "
    "SAVE and LOAD buttons for field checkpoints.";

typedef struct PcPortFileMenuNoticeContext {
    const char *title;
    const char *text;
} PcPortFileMenuNoticeContext;

void PcPort_FileMenuNoticeThreadMain(void *context)
{
    PcPortFileMenuNoticeContext *notice =
        (PcPortFileMenuNoticeContext *)context;

    if (PcPort_FileMenuNoticeDialog != 0)
        PcPort_FileMenuNoticeDialog(notice->title, notice->text);
    if (PcPort_FileMenuNoticeFree != 0)
        PcPort_FileMenuNoticeFree(notice);
}

void PcPort_NotifyUnsupportedFileMenu(void)
{
    PcPortFileMenuNoticeContext *notice;

    if (PcPort_FileMenuNoticeLog != 0)
        PcPort_FileMenuNoticeLog(
            "File menu unavailable: native memory-card screen is not "
            "implemented\n");

    if (PcPort_FileMenuNoticeAlloc == 0 ||
        PcPort_FileMenuNoticeStartThread == 0) {
        if (PcPort_FileMenuNoticeLog != 0)
            PcPort_FileMenuNoticeLog("File menu notice skipped: no host hooks\n");
        return;
    }

    notice = (PcPortFileMenuNoticeContext *)
        PcPort_FileMenuNoticeAlloc(sizeof(*notice));
    if (notice == 0) {
        if (PcPort_FileMenuNoticeLog != 0)
            PcPort_FileMenuNoticeLog("File menu notice skipped: out of memory\n");
        return;
    }
    notice->title = PcPort_FileMenuNoticeTitle;
    notice->text = PcPort_FileMenuNoticeText;

    /* Dispatch only.  The dialog is modal, so running it here would stop the
     * game thread whenever the dialog cannot be surfaced. */
    if (!PcPort_FileMenuNoticeStartThread(PcPort_FileMenuNoticeThreadMain,
                                          notice)) {
        if (PcPort_FileMenuNoticeLog != 0)
            PcPort_FileMenuNoticeLog("File menu notice skipped: no thread\n");
        if (PcPort_FileMenuNoticeFree != 0)
            PcPort_FileMenuNoticeFree(notice);
    }
}
