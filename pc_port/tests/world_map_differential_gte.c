/* Size of the PsyCross GTE register file, for the world-map differential. */
#include <stddef.h>
#include "psx/libgte.h"
#include "psx/gtereg.h"
const unsigned wmd_gte_size = (unsigned)sizeof(gteRegs);

/* PsyX_GTE.cpp's two links into the rest of PsyCross. PGXP stays off: the
 * differential compares integer GTE results only. */
int g_cfg_pgxpTextureCorrection = 0;
void PsyX_Log_Warning(const char *fmt, ...) { (void)fmt; }
