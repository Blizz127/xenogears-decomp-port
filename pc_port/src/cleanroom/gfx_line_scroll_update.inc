/* The PSYQ headers reserve uid_t for their 16-bit guest type. */
#if defined(__linux__)
#define uid_t FieldScrollSignalUid
#include <signal.h>
#undef uid_t
#else
#include <signal.h>
#endif

void GfxLineScrollUpdate(LineScroll *descriptor)
{
    const volatile LineScroll *state = descriptor;
    volatile unsigned short *phase = LineScrollSlotPtr(state->unk14);
    const volatile signed char *increment = LineScrollSlotPtr(state->pData);
    if (phase == 0)
        return;

    RECT part;
    part.y = (short)state->y;
    part.h = (short)state->height;
    unsigned short outputY = state->destY;

    for (unsigned int row = 0; row < state->numLines; ++row) {
        int step = increment[row];
        phase[row] = (unsigned short)(phase[row] + step);
        short fixed = (short)phase[row];
        unsigned short position = (unsigned short)(fixed >> 4);
        unsigned short modulus = state->modulus;
        if (modulus == 0) {
            raise(SIGFPE);
            return;
        }
        unsigned short split = position % modulus;
        unsigned short remaining = (unsigned short)(state->modulus - split);

        part.x = (short)state->x;
        part.w = (short)split;
        MoveImage(&part, (int)state->destX + remaining, outputY);

        part.x = (short)(unsigned short)(state->x + split);
        part.w = (short)remaining;
        MoveImage(&part, state->destX, outputY);

        unsigned short rowHeight = state->height;
        part.y = (short)(unsigned short)((unsigned short)part.y + rowHeight);
        outputY = (unsigned short)(outputY + rowHeight);
    }
}
