#ifndef XENO_DISC_CHECK_H
#define XENO_DISC_CHECK_H

/* Full identity check of the user-supplied US Disc 1. Zero means accepted. */
int PcPort_VerifyDiscImage(const char* path);
int PcPort_VerifyMountedDisc1(int fd);
int PcPort_VerifyMountedDisc2(int fd);

/* Record only a successful SDK insertion or initial lid authorization using
 * the actual mounted descriptor and the corresponding full-disc verifier. */
void PcPort_NotifyVerifiedDiscInsertion(int disc);
/* Authenticate the actual boot drive once its archive reads have drained. */
void PcPort_VerifyBattleBootDisc(void);
/* 1/2 for an authenticated US image whose mounted archive agrees with the
 * initialized game directory, 0 when that identity is unavailable. */
int PcPort_BattleArchiveDisc(void);

/* Error message box for a user without a terminal (no-op when headless). */
void PcPort_UserNotice(const char* title, const char* text);

#endif
