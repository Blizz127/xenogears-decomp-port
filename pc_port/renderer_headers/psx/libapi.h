#ifndef XENO_RENDERER_LIBAPI_H
#define XENO_RENDERER_LIBAPI_H

#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

/* These entry points have C linkage in the renderer archive although
 * src/psx/LIBAPI.C is compiled as C++ on Linux and Windows. */
typedef long (*XenoEventCallback)(void);
int OpenEvent(unsigned int event_class, int condition, int mode, XenoEventCallback callback);
int EnableEvent(unsigned int handle);
int DisableEvent(unsigned int handle);
int CloseEvent(unsigned int handle);
void DeliverEvent(unsigned int event_class, int condition);
void UnDeliverEvent(unsigned int event_class, int condition);
int TestEvent(unsigned int handle);
int WaitEvent(unsigned int handle);

int SetRCnt(int id, unsigned short target, int mode);
int StartRCnt(int id);
int StopRCnt(int id);
int ResetRCnt(int id);
int GetRCnt(int id);

int EnterCriticalSection(void);
void ExitCriticalSection(void);
void SwEnterCriticalSection(void);
void SwExitCriticalSection(void);

int InitPAD(char *port1, int port1_bytes, char *port2, int port2_bytes);
int StartPAD(void);
void StopPAD(void);
void EnablePAD(void);
void DisablePAD(void);
void ChangeClearPAD(int enabled);

int _card_format(int channel);
int _card_read(int channel, int block, unsigned char *data);
int _card_write(int channel, int block, unsigned char *data);
int Krom2RawAdd2(unsigned short code);
int Krom2RawAdd(unsigned int code);

/* LIBAPI.C exports placeholders for these BIOS services. The executable
 * descriptor is passed by address only. */
struct EXEC;
int Load(char *name, struct EXEC *program);
int LoadTest(char *name, struct EXEC *program);
int LoadExec(char *name, unsigned int stack_base, unsigned int stack_bytes);
int Exec(struct EXEC *program, int argc, char **argv);
int OpenTh(int (*entry)(void), unsigned int stack, unsigned int data);
int ChangeTh(int thread);
int CloseTh(int thread);
unsigned long SetSp(unsigned long stack);
unsigned long GetSp(void);
unsigned long GetSysSp(void);
unsigned long GetGp(void);
unsigned long GetSr(void);
unsigned long GetCr(void);
void GetConf(unsigned int *events, unsigned int *threads, unsigned int *stack);
int SetConf(unsigned int events, unsigned int threads, unsigned int stack);
void SetMem(int bytes);
void SystemError(char code, int detail);
void Exception(void);
void ReturnFromException(void);
void FlushCache(void);
void _boot(void);
void _96_init(void);
void _96_remove(void);

#ifdef __cplusplus
}
#endif
#endif
