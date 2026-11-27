#ifndef PC_PORT_BATTLE_CLUT_UPLOAD_H
#define PC_PORT_BATTLE_CLUT_UPLOAD_H

#include <stdint.h>

/* The 801E6618 startup call leaves compact guest words in two SLUS globals.
 * Pointers in this context are already mapped by the active battle runtime. */
typedef struct PcPortBattleClutFrame {
    const uint32_t *gpr;
    uint8_t *data;
    uint32_t data_bytes; /* at most 2 MiB, including in PsyQ translation units */
    uint8_t *stack;  /* [entry $sp - 0x50, entry $sp) */
    uint8_t *rect_word;
    uint8_t *pixels_word;
    void *runtime;
    void (*note_store)(void *runtime, const uint8_t *host, unsigned bytes);
} PcPortBattleClutFrame;

int PcPort_GfxLoadBattleCluts(const PcPortBattleClutFrame *frame);

#endif
