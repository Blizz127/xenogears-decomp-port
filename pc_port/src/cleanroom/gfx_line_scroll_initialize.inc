#ifndef _WIN32
#define uid_t line_scroll_init_system_uid_t
#endif
#include <signal.h>
#ifndef _WIN32
#undef uid_t
#endif

LineScroll *GfxLineScrollInitialize(LineScroll *descriptor, u16 x, u16 y,
                                    u16 modulus, s16 width, s16 numLines,
                                    u16 destX, u16 destY, s8 *pData)
{
    typedef unsigned short LineScrollInitRow __attribute__((__may_alias__));
    volatile LineScroll *const state = descriptor;
    HeapChangeCurrentUser(4, NULL);
    state->x = x;
    state->y = y;
    state->modulus = modulus;
    state->width = width;
    state->numLines = numLines;
    state->destX = destX;
    state->destY = destY;
    state->pData = PsxMemory_GuestAddr(pData);
    if (!numLines) {
        raise(SIGFPE);
        return NULL;
    }
    *(volatile u16 *)&state->height = (u16)((int)width / (int)numLines);

    void *const allocation = HeapAlloc((u_int)((int)numLines * 2), 0);
    state->unk14 = PsxMemory_GuestAddr(allocation);
    if (!allocation) {
        return NULL;
    }
    for (int index = 0; index < (int)numLines; ++index) {
        volatile LineScrollInitRow *const rows = LineScrollSlotPtr(state->unk14);
        rows[index] = 0;
    }
    return descriptor;
}
