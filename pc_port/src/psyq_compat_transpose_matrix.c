/* Psy-Q compatibility: TransposeMatrix (libgte).
 *
 * Clean-room body, written from the public interface alone (the specification
 * handed to the implementer, 2026-10-25):
 *
 *     MATRIX *TransposeMatrix(MATRIX *m0, MATRIX *m1);
 *
 * It stores the transpose of m0's 3x3 rotation part (m[3][3], short) in m1 and
 * returns m1.  It does not touch the translation t[3].  The game calls it in
 * place (m0 == m1) and with disjoint matrices, and the result must be the true
 * transpose in both cases.  MATRIX is PsyCross's
 * `typedef struct { short m[3][3]; int t[3]; } MATRIX;` (retail layout).
 *
 * Design: the nine source halfwords are read into a local copy before the first
 * store, so the same body gives the true transpose when m0 == m1 and when the
 * matrices are disjoint.  Only the nine rotation halfwords of m1 are written;
 * the pad halfword and t[] are never stored to.  For matrices that overlap
 * without being identical the interface says nothing; this body then yields
 * the transpose of m0's rotation as it was on entry.
 *
 * The TRANSPOSE_MATRIX_MUTANT_* branches are fault injections for
 * pc_port/tests/run_psyq_transpose_matrix_retail_test.sh; none of them is
 * defined in a normal build.
 */
#include <libgte.h>

MATRIX* TransposeMatrix(MATRIX* m0, MATRIX* m1);

MATRIX* TransposeMatrix(MATRIX* m0, MATRIX* m1)
{
#if defined(TRANSPOSE_MATRIX_MUTANT_NAIVE_INPLACE)
    /* Fault: transpose straight from m0 into m1 with no copy of the source.
     * Right for disjoint matrices, wrong in place: later reads see elements
     * that were already overwritten. */
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            m1->m[i][j] = m0->m[j][i];
#else
    short r[3][3];
    int i, j;

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            r[i][j] = m0->m[i][j];

    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++) {
#if defined(TRANSPOSE_MATRIX_MUTANT_DROP_M22)
            /* Fault: the last rotation element is never stored. */
            if (i == 2 && j == 2) continue;
#endif
            m1->m[i][j] = r[j][i];
        }
#endif
#if defined(TRANSPOSE_MATRIX_MUTANT_TOUCH_T)
    /* Fault: the translation is clobbered although the function must leave it
     * alone. */
    m1->t[0] = 0;
    m1->t[1] = 0;
    m1->t[2] = 0;
#endif
#if defined(TRANSPOSE_MATRIX_MUTANT_RETURN_INPUT)
    /* Fault: returns the source instead of the destination. */
    return m0;
#else
    return m1;
#endif
}
