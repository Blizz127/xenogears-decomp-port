#ifndef XENO_TEST_INPUT_H
#define XENO_TEST_INPUT_H

#include "common.h"

/* Parse XENO_TEST_INPUT before game startup. Returns zero when unset/valid. */
int PcPort_TestInputInit(void);

/* Parse an independent, world-frame-relative held-button schedule from
 * XENO_WORLD_TEST_INPUT.  It is merged at retail's post-controller-drain
 * accumulator seam and derives first-press edges from schedule transitions. */
int PcPort_WorldTestInputInit(void);

/* DIAGNOSTIC / TEST TOOLING (remove with its FieldPollControllers call site).
 * Field-frame-relative held-button schedule from XENO_FIELD_TEST_INPUT, merged
 * at retail's post-drain accumulator seam in FieldPollControllers.  Separate
 * from XENO_FIELD_TEST on purpose: that variable also selects the developer
 * KernelMenu boot path, which bypasses the retail movie/title chain. */
int PcPort_FieldTestInputInit(void);
void PcPort_FieldTestInputMerge(u16 *held_buttons, u16 *pressed_edges,
                                u16 *released_edges);

/* Advance the shared field/world frame clock, then inject at an input seam. */
void PcPort_TestInputAdvanceFrame(void);
void PcPort_TestInputInject(u16 *held_buttons);
void PcPort_WorldTestInputMerge(u16 *held_buttons, u16 *pressed_edges,
                                u16 *repeat_edges);

#if defined(XENO_TEST_INPUT_CERTIFICATE)
void PcPort_TestInputResetForCertificate(void);
void PcPort_WorldTestInputResetForCertificate(void);
#endif

#endif
