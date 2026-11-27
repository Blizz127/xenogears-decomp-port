#ifndef XENO_PC_PORT_QUICK_CHECKPOINT_PARTICLES_H
#define XENO_PC_PORT_QUICK_CHECKPOINT_PARTICLES_H

#include <stddef.h>
#include <stdint.h>

/* The retail SDK can typedef size_t as a four-byte PSX word. */
_Static_assert(sizeof(size_t) == sizeof(sizeof(0)), "include host checkpoint API before common.h");

/* Optional, pointer-free continuation for the spent Dazil pursuit. Capture
 * returns zero with *used = 0 outside its scope. Restore runs at a field
 * boundary after actor initialization and before particle update/render. */
int PcPort_FieldParticleCapture(uint8_t *out, size_t capacity, size_t *used);
int PcPort_FieldParticleRestore(const uint8_t *data, size_t size);

#endif
