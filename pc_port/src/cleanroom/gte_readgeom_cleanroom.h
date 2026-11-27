/* gte_readgeom_cleanroom.h */
#ifndef GTE_READGEOM_CLEANROOM_H
#define GTE_READGEOM_CLEANROOM_H

#include <stdint.h>

/*
 * Seam (provided by the integrator, NOT defined by the implementation).
 * Production: wraps the public PsyCross primitive `unsigned int CFC2(int reg)`.
 * Proof harness: returns the vector's control word for `reg` and records the read.
 */
uint32_t xeno_cleanroom_cfc2(int reg);

/* Host ABI (integration requirement). */
void ReadGeomOffset(long *ofx, long *ofy);
int32_t ReadGeomScreen(void);

#endif
