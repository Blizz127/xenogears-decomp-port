/* Psy-Q compatibility: NormalLightCol (the libgte entry the game calls for
 * NormalColorCol-style lighting).
 *
 * Documented behaviour (Psy-Q Library Reference, NormalColorCol; psx-spx
 * "GTE NCCS"): load the normal vector into V0 and the local colour into
 * RGBC, run NCCS (normal colour colour, single vector), and store the RGB
 * FIFO result -- the same register sequence as PsyCross's documented
 * gte_NormalColorCol() macro (include/psx/gtemac.h: gte_ldv0, gte_ldrgb,
 * gte_nccs, gte_strgb).  The W34N120_MUTANT_* branches are fault injections
 * for pc_port/tests/run_w34n120_normal_light_col.sh.
 */
#include <string.h>

#include "common.h"
#include "psyq_normal_light_col.h"

extern void MTC2(unsigned int value, int reg);
extern unsigned int MFC2(int reg);
extern int doCOP2(int op);

/* The operands need not be 4-byte aligned in the port (retail LWC2 would
 * require it), so load/store the words through memcpy. */
static unsigned int nlc_load(const void* p)
{
    unsigned int v;
    memcpy(&v, p, sizeof v);
    return v;
}

void NormalLightCol(void* normal, void* input_color, void* output_color)
{
    unsigned int rgb;
#if defined(W34N120_MUTANT_WRONG_VXY_REGISTER)
    MTC2(nlc_load(normal), 2);
#else
    MTC2(nlc_load(normal), 0);                              /* VXY0 */
#endif
#if !defined(W34N120_MUTANT_DROP_VZ_LOAD)
    MTC2(nlc_load((const unsigned char*)normal + 4), 1);    /* VZ0 */
#endif
#if defined(W34N120_MUTANT_WRONG_RGBC_REGISTER)
    MTC2(nlc_load(input_color), 7);
#else
    MTC2(nlc_load(input_color), 6);                         /* RGBC */
#endif
#if defined(W34N120_MUTANT_WRONG_OPCODE)
    (void)doCOP2(0x0108041A);
#else
    (void)doCOP2(0x0108041B);                               /* NCCS (gte_nccs) */
#endif
#if defined(W34N120_MUTANT_WRONG_OUTPUT_REGISTER)
    rgb = MFC2(21);
#elif defined(W34N120_MUTANT_CORRUPT_OUTPUT)
    rgb = MFC2(22) ^ 1u;
#else
    rgb = MFC2(22);                                         /* RGB2 (gte_strgb) */
#endif
    memcpy(output_color, &rgb, sizeof rgb);
}
