/* SPDX-License-Identifier: MIT */
void GameSoftReset(void)
{
    void (*const clear_callback)(void) = 0;

    /* Let the devices finish their reset work outside the critical section. */
    SwExitCriticalSection();
    ResetGraph(0);
    ArchiveReset();
    SoundReset();
    SpuQuit();

    /* Detach the old callbacks before flushing the disc service. */
    func_800363F0(clear_callback);
    DrawSyncCallback(clear_callback);
    func_8004B7D0(clear_callback);
    CdFlush();
    func_800408F4();

    /* The startup service expects to take over inside the critical section. */
    SwEnterCriticalSection();
    start();
}
