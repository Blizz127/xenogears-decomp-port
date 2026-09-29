#ifndef XENO_SHIM_PSYQ_LIBGPU_H
#define XENO_SHIM_PSYQ_LIBGPU_H
/*
 * Port build: redirect the game's PsyQ GPU header to PsyCross's implementation,
 * smoothing two API divergences:
 *   - PsyCross's libgpu.h does not include libgte.h, but uses SVECTOR; pull it in first.
 *   - PsyCross renamed Sony's RECT to RECT16; alias it back for the game code.
 */
#include <libgte.h>
#include <libgpu.h>

#ifndef XENO_COMPAT_RECT
#define XENO_COMPAT_RECT
typedef RECT16 RECT;
/* PsyCross bulk-renamed Sony's RECT to RECT16, including TIM_IMAGE members
 * crect/prect -> cRECT16/pRECT16. Map the game's field names back. */
#define crect cRECT16
#define prect pRECT16
#endif

#endif
