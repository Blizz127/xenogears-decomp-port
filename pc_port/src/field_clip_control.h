#ifndef XENO_FIELD_CLIP_CONTROL_H
#define XENO_FIELD_CLIP_CONTROL_H
#include <stdint.h>
/* Native interpreter working state, corresponding to s4/s3 and stack
 * D0/D8/E8/F0/6C. Not persistent game state or a substitute for E39F0. */
typedef struct PcPortFieldClipControl {
    uint8_t* object;
    uint8_t* pool;
    uint32_t stream;
    int32_t limit, ticks, running, postprocess;
    uint16_t operand;
    /* E39F0 stack +E0: entry object, retained when opcode 1F changes s4. */
    uint8_t* origin;
} PcPortFieldClipControl;
/* Executes one proven control instruction through retail E5974.
 * Returns 1 for a handled instruction; 0 leaves state untouched and requires
 * another opcode handler. No caller may interpret 0 as a successful skip. */
int PcPort_FieldClipControlStep(PcPortFieldClipControl* state);
/* Same handled/unhandled contract, for verified object/data instructions. */
int PcPort_FieldClipDataStep(PcPortFieldClipControl* state);
#endif
