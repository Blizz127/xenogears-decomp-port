#ifndef XENO_FIELD_CLIP_PRELUDE_H
#define XENO_FIELD_CLIP_PRELUDE_H
#include <stdint.h>
/* Translation of E39F0 entry through E3D34, not an interpreter substitute.
 * Returns 0 on retail's early-exit gate (stream output untouched), otherwise
 * 1 with the stream selected by the motion/redirect/target preparation phase.
 * The interpreter must still initialize and preserve its own local state. */
int PcPort_FieldClipPrelude(uint8_t* object, int32_t ticks, uint32_t* stream);
#endif
