/*
 * World-map convergence P1/P2 module (W34B1 + W34B3).
 *
 * Extracted from world_map_init.c so that both the production game build
 * and the production-linked test link the same authoritative object.
 *
 * P1 retail slice: 0x800726C0–0x80072728.
 * P2 retail slice: 0x8007272C–0x80072780.
 */
#ifndef WORLD_MAP_CONVERGENCE_H
#define WORLD_MAP_CONVERGENCE_H

#include "common.h"

typedef u32 wm_conv_p1_next_t;

/* Retail cut PC constants. */
#define WM_CONV_P1_CUT_SECOND_TABLE 0x8007272Cu
#define WM_CONV_P1_CUT_FLAG1_ARC    0x80072784u
#define WM_CONV_P1_CUT_COMMON_TAIL  0x8007290Cu

/* Production P1 function. */
wm_conv_p1_next_t wm_800726C0_convergence_p1(void);

/* Production P2 function — second convergence table pass. */
u32 wm_8007272C_convergence_p2(void);

/* Retail restored-session callback initialization branch. */
void wm_80072784_convergence_resume(void);

/* Pool registration helper (called by P1 and P2). */
void wm_pool_register(u32 a0, u32 a1);

/* Reset all per-world-init counters. Call at each new world init boundary. */
void wm_conv_p1_reset(void);

/* P1 counter accessors. */
int wm_conv_p1_get_entry(void);
int wm_conv_p1_get_flag0(void);
int wm_conv_p1_get_flag1_cut(void);
int wm_conv_p1_get_other_cut(void);
int wm_conv_p1_get_empty(void);
int wm_conv_p1_get_iterations(void);
int wm_conv_p1_get_helper_calls(void);
int wm_conv_p1_get_cut_second_table(void);
u32 wm_conv_p1_get_last_next(void);
int wm_conv_p1_get_forbidden_c610_read(void);
int wm_conv_p1_get_forbidden_a034_read(void);
int wm_conv_p1_get_forbidden_976fc(void);
int wm_conv_p1_get_forbidden_common_tail(void);
int wm_conv_p1_get_forbidden_excluded_instr(void);
int wm_conv_p1_get_pool_alloc(void);

/* P2 counter accessors. */
int wm_conv_p2_get_entry(void);
int wm_conv_p2_get_empty_stream(void);
int wm_conv_p2_get_iterations(void);
int wm_conv_p2_get_helper_calls(void);
int wm_conv_p2_get_forbidden_c894(void);
int wm_conv_p2_get_forbidden_976fc(void);
int wm_conv_p2_get_forbidden_common_tail(void);
int wm_conv_p2_get_forbidden_excluded_instr(void);
u32 wm_conv_p2_get_selector(void);
u32 wm_conv_p2_get_selected_ptr(void);

#endif /* WORLD_MAP_CONVERGENCE_H */
