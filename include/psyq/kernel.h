#ifndef XENO_PSX_KERNEL_H
#define XENO_PSX_KERNEL_H

/* Counter identifiers and mode bits used by system/sound.c and by the
 * matching SetRCnt implementation in slus_006.64/psyq/libapi.c. */
#define RCntCNT2    0xf2000002U
#define RCntMdSC    0x0001
#define RCntMdGATE  0x0010
#define RCntMdINTR  0x1000

#ifdef __cplusplus
extern "C" {
#endif

int EnterCriticalSection(void);
void ExitCriticalSection(void);

int OpenEvent(unsigned int event, int spec, int mode, long (*callback)(void));
int CloseEvent(unsigned int handle);
int EnableEvent(unsigned int handle);
int DisableEvent(unsigned int handle);
void DeliverEvent(unsigned int event, int spec);

/* The console implementation uses 32-bit longs. The native timer backend
 * exports int on both LP64 and Windows. */
#ifdef XENO_PC_PORT
int SetRCnt(int counter, unsigned short target, int mode);
int GetRCnt(int counter);
int StartRCnt(int counter);
int StopRCnt(int counter);
#else
long SetRCnt(long counter, short target, long mode);
long GetRCnt(long counter);
long StartRCnt(long counter);
long StopRCnt(long counter);
#endif

#ifdef __cplusplus
}
#endif
#endif
