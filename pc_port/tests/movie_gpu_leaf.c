/* The two libgpu leaves the movie overlay calls that are real functions on retail
 * (SetPolyG4 0x80043CC4, SetSemiTrans 0x80043BFC).  Defined here from the port's own
 * libgpu macros (PsyCross, the production implementation) so the native side of the
 * differential uses the production semantics; the retail side runs the SLUS code. */
#include <psx/libgte.h>
#include <psx/libgpu.h>
void SetPolyG4(POLY_G4* p) { setPolyG4(p); }
void SetSemiTrans(void* p, int abe) { setSemiTrans(p, abe); }
