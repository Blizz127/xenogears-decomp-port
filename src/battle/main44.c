#include "common.h"


extern int ArchiveSetIndex(int directoryIndex, int entryIndex);


/* func_8008AB4C.s */
void func_8008AB4C(void) {
    ArchiveSetIndex(0x20, 0);
}
/* func_8008AB70.s */
void func_8008AB70(void) {
    ArchiveSetIndex(0x20, 2);
}
/* func_8008AB94.s */
void func_8008AB94(void) {
    ArchiveSetIndex(0x20, 3);
}
/* func_8008ABB8.s */
void* func_8008ABB8(s32 size, s32 mode) {
    HeapChangeCurrentUser(2, 0);
    return HeapAlloc(size, mode);
}
/* func_8008AC00.s */
void* func_8008AC00(s32 size) {
    HeapChangeCurrentUser(2, 0);
    return HeapAlloc((size + 3) * 26, 0);
}
/* func_8008AC50.s */
/* Not yet byte-matching: the matching build assembles retail bytes for
 * this function from its own asm segment (see config/battle.yaml).  The
 * body below is kept for the port and the differential harnesses. */
#ifdef XENO_PC_PORT
void func_8008AC50(void) {
    while (ArchiveDataSync() != 0) {
        func_800716D8();
    }
}
#endif /* XENO_PC_PORT */
