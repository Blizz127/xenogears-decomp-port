/*
 * W34B38 — world-map-only ordering-table adapter.
 *
 * Retail world OTs are 0x400 four-byte words with 24-bit guest links
 * (retail ClearOTagR 0x80044AD8; tail DrawOTag(root+0xFFC) 0x800719B4).
 * PsyCross's ClearOTagR/DrawOTag use 8-byte padded OT_TAG slots and
 * host-pointer links, so the world OT must be cleared and walked in the
 * guest representation and only cross into the host at DrawPrim.
 * See scratchpad/w34_overnight_20260823/AUDIT_W34B38_ADAPTER_DESIGN.md.
 */
#ifndef WORLD_MAP_OT_ADAPTER_H
#define WORLD_MAP_OT_ADAPTER_H

#include "common.h"

/* Retail ClearOTagR semantics on a guest OT: entry i (i>=1) links to entry
 * i-1 (len 0); entry 0 terminates the chain. */
void wm_ot_clear_r_guest(u32 ot_guest, u32 count);

/* Retail DrawOTag semantics on a guest OT entry (the tail passes
 * root + 0xFFC): walk 24-bit guest links, submit each len>0 packet via
 * DrawPrim(PSX_ADDR(pkt)), stop at link 0xFFFFFF. Returns 1 on a complete
 * walk, 0 when aborted by a guard (abort kind counted, never raw-deref). */
int wm_ot_draw_otag_guest(u32 entry_guest);

void wm_ot_reset(void);
int  wm_ot_get_packets_submitted(void);
int  wm_ot_get_steps(void);
int  wm_ot_get_abort_range(void);
int  wm_ot_get_abort_align(void);
int  wm_ot_get_abort_len(void);
int  wm_ot_get_abort_steps(void);
int  wm_ot_get_multi_prim_packets(void);
u32  wm_ot_get_last_tag(void);
u32  wm_ot_get_last_addr(void);

#endif /* WORLD_MAP_OT_ADAPTER_H */
