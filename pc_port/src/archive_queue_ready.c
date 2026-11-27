#include "archive_ready.h"

enum { LAST_MSF_SECTOR = 449849, AC24_FORWARD_WINDOW = 16 };

/* US 8002AE54..8002AEC0 compares the next LBA to the cursor after the
 * completed file. Equal and short forward gaps follow other retail branches. */
int PcPort_ArchiveQueueRequiresSeek(int firstLba, int sectors, int nextLba)
{
    if (firstLba < 0 || firstLba > LAST_MSF_SECTOR || nextLba < 0 ||
        nextLba > LAST_MSF_SECTOR || sectors <= 0 || sectors - 1 > LAST_MSF_SECTOR - firstLba)
        return 0;
    int followingLba = firstLba + sectors;
    return nextLba < followingLba || nextLba - followingLba > AC24_FORWARD_WINDOW;
}
