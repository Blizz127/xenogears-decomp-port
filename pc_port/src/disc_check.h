#ifndef XENO_DISC_CHECK_H
#define XENO_DISC_CHECK_H

/* First-run check of the user's disc image against the Redump Xenogears
 * (USA) Disc 1 dump (disc_check.c).  0 = OK (or overridden), -1 = wrong. */
int PcPort_VerifyDiscImage(const char* path);

/* Error message box for a user without a terminal (no-op when headless). */
void PcPort_UserNotice(const char* title, const char* text);

#endif
