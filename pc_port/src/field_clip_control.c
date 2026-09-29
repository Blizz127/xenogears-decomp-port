#include "field_clip_control.h"

/* Dispatch and control paths in pinned E39F0. The continuation list comes
 * from retail table [801DC040,801DC204), not from unimplemented native cases. */
int PcPort_FieldClipControlStep(PcPortFieldClipControl* state)
{
    uint32_t start = state->stream;
    uint16_t instruction = *(uint16_t*)(uintptr_t)start;
    unsigned opcode = instruction & 255;
    switch (opcode) {
    case 0: case 1: case 2: case 3:
    case 0x20: case 0x21: case 0x22:
    case 4: case 5: case 6: case 7: case 9: case 15: case 18:
    case 27: case 28: case 44: case 45: case 47: case 58: case 62:
    case 63: case 81: case 82: case 83: case 88: case 89: case 90:
    case 96: case 97: case 101: case 102: case 103: case 104:
    case 105: case 106:
        break;
    default:
        if (opcode < 113) return 0; /* A different real handler is required. */
        break;
    }
    state->operand = instruction;
    state->stream = start + 2u;
    if (opcode == 0 || opcode >= 113) {
        state->stream = start;
        state->running = 0;
    } else if (opcode == 1) {
        if (state->limit != -1) {
            uint16_t duration = *(uint16_t*)(uintptr_t)state->stream;
            uint16_t* elapsed = (uint16_t*)(state->object + 0x40);
            *elapsed = (uint16_t)((uint32_t)*elapsed + (uint32_t)state->ticks);
            state->operand = duration;
            state->stream += 2u;
            if ((int16_t)*elapsed >= (int16_t)duration) {
                *elapsed = 0;
                state->ticks = 0;
                return 1;
            }
        }
        state->stream = start;
        state->running = 0;
    } else if (opcode == 2 || opcode == 3) {
        state->postprocess = 1;
    } else if (opcode == 0x20 || opcode == 0x21) {
        uint32_t bit = opcode == 0x20 ? 0x100 : 1;
        if (opcode == 0x21)
            *(uint16_t*)(state->object + 0x3c) = instruction >> 8;
        if (state->limit == -1 || ((uint32_t)state->limit & bit)) {
            state->stream = start;
            state->running = 0;
        }
    } else if (opcode == 0x22) {
        if (state->limit != -1) {
            uint16_t duration = *(uint16_t*)(uintptr_t)state->stream;
            unsigned parameter = instruction >> 8;
            uint32_t bit = parameter == 255 ? 0x400 : 4;
            state->operand = duration;
            state->stream += 2u;
            if (parameter != 255)
                *(uint16_t*)(state->object + 0x3c) = (uint16_t)parameter;
            if ((uint32_t)state->limit & bit) {
                uint16_t* count = (uint16_t*)(state->object + 0x42);
                *count = (uint16_t)(*count + 1u);
                if ((int32_t)*count >= (int16_t)duration) {
                    *count = 0;
                    return 1;
                }
            }
        }
        state->stream = start;
        state->running = 0;
    }
    return 1;
}
