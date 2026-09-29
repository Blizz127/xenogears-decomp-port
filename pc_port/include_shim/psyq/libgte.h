#ifndef XENO_SHIM_PSYQ_LIBGTE_H
#define XENO_SHIM_PSYQ_LIBGTE_H
/* Port build: redirect the game's PsyQ GTE header to PsyCross's implementation. */
#include <libgte.h>
extern long VectorNormalSS(SVECTOR* v0, SVECTOR* v1);

/* Xenogears' decomp symbol map names the retail executable's two trig entry
 * points opposite to PsyCross's conventional implementations.  At 0x8003F8B0
 * the decomp `rcos` entry reads the sine halfwords rooted at D_800523F0,
 * while 0x8003F8CC (`rsin`) reads the cosine halfwords at D_800523F2.
 *
 * Keep PsyCross itself conventional, but preserve the retail entry-point
 * semantics for game translation units.  In particular, the field camera
 * calls retail `rsin` for its vertical component; using mathematical sine
 * puts an unclamped camera below the Lahan well instead of above it. */
/* PsyCross omits the geom-offset read-back; implemented in
 * pc_port/src/psyq_compat.c. */
extern void ReadGeomOffset(long* ofx, long* ofy);
extern long RotAverage4(SVECTOR* v0, SVECTOR* v1, SVECTOR* v2, SVECTOR* v3,
                        long* sxy0, long* sxy1, long* sxy2, long* sxy3,
                        long* p, long* flag);
void gte_OuterProduct12(VECTOR* v0, VECTOR* v1, VECTOR* v2);
SVECTOR* gte_ApplyMatrixSV(MATRIX* m, SVECTOR* v0, SVECTOR* v1);
int gte_RotTransPers(SVECTOR* v0, int* sxy, long* p, long* flag, long* otz);

static inline int xeno_retail_rsin(int angle) {
    return rcos(angle);
}

static inline int xeno_retail_rcos(int angle) {
    return rsin(angle);
}

#define rsin xeno_retail_rsin
#define rcos xeno_retail_rcos
#endif
