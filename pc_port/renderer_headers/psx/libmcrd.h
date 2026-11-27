#ifndef XENO_RENDERER_LIBMCRD_H
#define XENO_RENDERER_LIBMCRD_H

#include "psx/kernel.h"

typedef void (*MemCB)(int command, int result);
enum {
    McFuncExist = 1, McFuncAccept = 2,
    McFuncReadFile = 3, McFuncWriteFile = 4,
    McFuncReadData = 5, McFuncWriteData = 6
};
enum { McErrNone = 0, McErrCardNotExist = 1, McErrNewCard = 3 };

#ifdef __cplusplus
extern "C" {
#endif

void MemCardInit(int mode);
void MemCardStart(void);
void MemCardStop(void);
void MemCardEnd(void);
MemCB MemCardCallback(MemCB callback);
int MemCardSync(int mode, int *command, int *result);

int MemCardExist(int slot);
int MemCardAccept(int slot);
int MemCardFormat(int slot);
int MemCardUnformat(int slot);
int MemCardCreateFile(int slot, char *name, int blocks);
int MemCardDeleteFile(int slot, char *name);
int MemCardGetDirentry(int slot, char *pattern, struct DIRENTRY *entries,
                       int *count, int offset, int capacity);
int MemCardOpen(int slot, char *name, int mode);
void MemCardClose(void);
int MemCardReadData(unsigned int *buffer, int offset, int length);
int MemCardReadFile(int slot, char *name, unsigned int *buffer, int offset, int length);
int MemCardWriteData(unsigned int *buffer, int offset, int length);
int MemCardWriteFile(int slot, char *name, unsigned int *buffer, int offset, int length);

#ifdef __cplusplus
}
#endif
#endif
