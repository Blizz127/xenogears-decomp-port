/* xg_plat/timing.h -- timing interface: game speed, fast-forward and the
 * vertical-blank clock.  Backend: PsyCross (src/plat/xg_plat_psycross.c).
 * Speed is a host wall-clock multiplier only; no emulated register or game
 * state changes. */
#ifndef XG_PLAT_TIMING_H
#define XG_PLAT_TIMING_H

#ifdef __cplusplus
extern "C" {
#endif

#define XG_PLAT_SPEED_MIN 1
#define XG_PLAT_SPEED_MAX 5

int  xg_plat_timing_speed(void);              /* current multiplier, 1..5 */
void xg_plat_timing_set_speed(int multiplier); /* clamped to 1..5 */
void xg_plat_timing_fast_forward(int held);    /* held: run at the fast-forward speed */
void xg_plat_timing_set_fast_forward_speed(int multiplier); /* 1..5, default 5 */
int  xg_plat_timing_vblank_count(void);        /* host vblanks since start */

/* The game's VSync and root-counter calls (link-time routing,
 * build_port.sh XG_PLAT_WRAP_SYMS).  vsync: mode 0 waits for the next
 * vblank, n > 0 waits n vblanks after the previous one, 1 / negative query
 * the time / count without waiting.  Counters: `spec` names a root counter
 * (PSX RCntCNT0..3); target / mode as the PSX counter registers. */
int xg_plat_timing_vsync(int mode);
int xg_plat_timing_counter_set(int spec, unsigned short target, int mode);
int xg_plat_timing_counter_get(int spec);
int xg_plat_timing_counter_start(int spec);

#ifdef __cplusplus
}
#endif

#endif
