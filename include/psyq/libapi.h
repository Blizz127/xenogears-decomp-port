#ifndef XENO_PSX_LIBAPI_H
#define XENO_PSX_LIBAPI_H

#include "kernel.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Boot, controller and memory-card entry points used by main/main.c and
 * system/controller2.c. The port owns its file-backed card lifecycle. */
#ifdef XENO_PC_PORT
void InitCARD(int pad_enable);
int StartCARD(void);
#else
void InitCARD(long pad_enable);
void StartCARD(void);
void InitCARD2(long pad_enable);
void StartCARD2(void);
void StopCARD2(void);
long InitPAD(char *first, long first_size, char *second, long second_size);
void StartPAD(void);
void ChangeClearPAD(int enable);
#endif
void _bu_init(void);

#ifdef __cplusplus
}
#endif
#endif
