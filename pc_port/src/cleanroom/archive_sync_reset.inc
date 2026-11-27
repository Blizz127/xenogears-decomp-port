#include <stdint.h>

extern uint64_t g_PcPortArchiveFileRegistration;
extern int (*g_PcPortArchiveDeferredFileSync)(int, int *) __attribute__((weak));
extern int PcPort_ArchiveResetFieldWds(void) __attribute__((weak));
extern void PcPort_ServiceVblank(void) __attribute__((weak));
extern void func_8002A498(int);
extern int ArchiveCdDataSync(int);
extern void ArchiveCdSetMode(u_char);
extern int Vsync(int);

void ArchiveReset(void)
{
    if (PcPort_ArchiveResetFieldWds && PcPort_ArchiveResetFieldWds())
        return;
    if (g_PcPortArchiveFileRegistration) {
        func_8002A498(0);
        return;
    }

    func_8002A498(0);
    ArchiveCdDataSync(0);
    if (g_ArchiveDebugTable == 0) {
        while (!CdControlB(9, 0, (u_char *)&D_80059F1C)) {
        }
        ArchiveCdSetMode(0xa0);
        ArchiveCdDataSync(0);
        Vsync(3);
    }
    CdDataCallback(0);
    CdSyncCallback(0);
    CdReadyCallback(0);
    *(volatile s32 *)&D_8004FDFC = 0;
    *(volatile int *)&g_ArchiveCurFileSize = 0;
    *(volatile u_int *)&g_ArchiveCdDriveState = 0;
}

int ArchiveDataSync(void)
{
    if (g_PcPortArchiveFileRegistration) {
        if (&g_PcPortArchiveDeferredFileSync == 0 || g_PcPortArchiveDeferredFileSync == 0)
            return 1;
        int pending = 0;
        if (!g_PcPortArchiveDeferredFileSync(1, &pending) || pending > 0)
            return 1;
        if (g_PcPortArchiveQueueService || g_PcPortArchiveModeService)
            return 1;
    }

    if (g_PcPortArchiveQueueService) {
        if (g_PcPortArchiveCancelFilteredQueue) {
            int status = D_8004FDFC;
            return status ? status : g_ArchiveCdDriveState != 0;
        }
        if (PcPort_ServiceVblank)
            PcPort_ServiceVblank();
        if (g_PcPortArchiveQueueService)
            g_PcPortArchiveQueueService();
    } else if (g_PcPortArchiveModeService) {
        if (g_PcPortArchiveCancelFilteredQueue && PcPort_ServiceVblank)
            PcPort_ServiceVblank();
        if (g_PcPortArchiveModeService)
            g_PcPortArchiveModeService();
    }

    int status = D_8004FDFC;
    if (status)
        return status;
    if (g_ArchiveDebugTable == 0 && CdDataSync(1))
        return 1;
    return g_ArchiveCdDriveState != 0;
}
