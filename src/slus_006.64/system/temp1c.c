#include "common.h"
#ifdef XENO_PC_PORT
#include <stdio.h>
#include "psx_memory.h"
#include "guest_prim_link.h"
#endif
#include "field/actor.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "system/memory.h"

/* Retail TU 0x80024FF4..0x80025044 (D_8004FBB8 matrix setter), one of the TUs that were merged into
 * system/temp1.c (the groups reach the shared Gfx globals differently:
 * %gp_rel for their own .sbss, absolute for other TUs'). This is the only
 * copy of these bodies; the port builds it too. */

extern MATRIX D_8004FBB8;

/* Sets the D_8004FBB8 matrix. */
void func_80024FF4(MATRIX* matrix) {
    D_8004FBB8 = *matrix;
}
