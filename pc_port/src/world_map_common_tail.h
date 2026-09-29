/*
 * World-map common-tail prefix + 0x80089160 initializer (W34B5A).
 *
 * Common-tail prefix: 0x8007290C–0x80072938.
 * Selector-dependent logic: C610=0 calls wm_80089160(14,0,0).
 * Reconvergence at 0x8007293C (first excluded = next-phase helper).
 *
 * 0x80089160: bounded table/state initializer [0x80089160,0x800893E0).
 * Leaf function, no direct calls. Record stride 672 bytes.
 */
#ifndef WORLD_MAP_COMMON_TAIL_H
#define WORLD_MAP_COMMON_TAIL_H

#include "common.h"

/* Retail cut PC constants. */
#define WM_COMMON_TAIL_P0_START      0x8007290Cu
#define WM_COMMON_TAIL_P0_CUT        0x8007293Cu  /* reconvergence / first excluded */
#define WM_COMMON_TAIL_P1_START      0x8007293Cu
#define WM_COMMON_TAIL_P1_CUT        0x80072944u  /* first excluded after wm_800978FC */
#define WM_COMMON_TAIL_P2_START      0x80072944u
#define WM_COMMON_TAIL_P2_CUT        0x8007294Cu  /* first excluded after wm_8008901C */
#define WM_COMMON_TAIL_P3_START      0x8007294Cu
#define WM_COMMON_TAIL_P3_CUT        0x80072954u  /* first excluded after wm_800865A0 */
#define WM_800865A0_START            0x800865A0u
#define WM_800865A0_END_EXCLUSIVE    0x800866C8u
#define WM_8008901C_START            0x8008901Cu
#define WM_8008901C_END_EXCLUSIVE    0x80089128u
#define WM_80089160_START            0x80089160u
/* 0x280 bytes / 160 instructions.  Retail slice SHA-256:
 * 497f19a8b36ee1df78b5bfb66301a0a5efd97a1ed54c906285cd2b2053521f77 */
#define WM_80089160_END_EXCLUSIVE    0x800893E0u
/* Retail [0x800894C8, 0x80089514): 76 bytes / 19 instructions.
 * Slice SHA-256:
 * 3cd5b7a87503c92b5647e9eb2ff784fe6298fc80acc8e955bd935fc43f80e77b */
#define WM_800894C8_START            0x800894C8u
#define WM_800894C8_END_EXCLUSIVE    0x80089514u
#define WM_800978FC_START            0x800978FCu
#define WM_800978FC_END_EXCLUSIVE    0x800979C8u

/* Next-phase forbidden targets. */
#define WM_NEXT_HELPER_978FC         0x800978FCu
#define WM_NEXT_HELPER_8901C         0x8008901Cu
#define WM_NEXT_HELPER_865A0         0x800865A0u
#define WM_NEXT_HELPER_85FE0         0x80085FE0u
#define WM_NEXT_HELPER_75228         0x80075228u
#define WM_SCHEDULER_97800           0x80097800u
#define WM_WORLD_LOOP_712D0          0x800712D0u

/* Retail address of D_80059179. In the native port the compiled host symbol
 * is authoritative; this constant remains for exact-address certificates. */
#define WM_D_80059179_ABS            0x80059179u

/* 0x80089160 constants. */
#define WM_89160_TABLE_BASE_PTR      0x8009BCC0u
#define WM_89160_RECORD_STRIDE       672u    /* 0x2A0 */
#define WM_89160_SUBRECORD_STRIDE    0x54u   /* 84 bytes */
#define WM_89160_SUBRECORD_COUNT     8
#define WM_89160_FLAG_BYTE_OFFSET    0x4Fu
#define WM_89160_FLAG_BIT            0x80u

/* Test-only exact retail access trace kinds.  The production build does not
 * define WM_89160_TEST_TRACE, so the observer and every call compile away. */
#define WM_89160_TRACE_LBU           1u
#define WM_89160_TRACE_LHU           2u
#define WM_89160_TRACE_LW            3u
#define WM_89160_TRACE_SB            4u
#define WM_89160_TRACE_SH            5u
#define WM_89160_TRACE_SW            6u
#define WM_89160_TRACE_LWL           7u
#define WM_89160_TRACE_LWR           8u
#define WM_89160_TRACE_SWL           9u
#define WM_89160_TRACE_SWR           10u

#if defined(WM_89160_TEST_TRACE)
void wm_89160_test_trace(u32 pc, u32 kind, u32 address,
                         u32 width, u32 value);
#endif

/* Production functions. */

/* wm_80089160: full native implementation of retail 0x80089160.
 * Leaf function. Arguments: a0=record index, a1=src ptr, a2=src ptr.
 * For natural C610=0 path: (14, 0, 0). */
void wm_80089160(u32 a0, u32 a1, u32 a2);

/* wm_800894C8: clear bit 0x80 on all 8 subrecord flag bytes of record a0.
 * Leaf. Void. No bounds check. Uses the same table/stride/flag constants
 * as wm_80089160. */
void wm_800894C8(u32 record_index);

/* wm_8007290C_common_tail_p0: bounded common-tail prefix.
 * Reads C610, dispatches to wm_80089160 for C610=0.
 * Returns the exact new cut PC (0x8007293C). */
u32 wm_8007290C_common_tail_p0(void);

/* Per-world-init reset. Call at each new world init boundary. */
void wm_common_tail_p0_reset(void);

/* Counter accessors. */
int  wm_ctp0_get_entry(void);
int  wm_ctp0_get_c610_zero(void);
int  wm_ctp0_get_c610_nonzero(void);
int  wm_ctp0_get_89160_calls(void);
u32  wm_ctp0_get_last_cut(void);
int  wm_ctp0_get_forbidden_978fc(void);
int  wm_ctp0_get_forbidden_scheduler(void);
int  wm_ctp0_get_forbidden_world_loop(void);
int  wm_ctp0_get_forbidden_8901c(void);
int  wm_ctp0_get_forbidden_865a0(void);
int  wm_ctp0_get_forbidden_85fe0(void);
int  wm_ctp0_get_forbidden_75228(void);

/* wm_800978FC: world-map graphics buffer allocator.
 * Allocates two 64 KB buffers, initializes first with repeating
 * byte pattern (2048 records × 32 bytes), copies to second.
 * Self-contained leaf — only calls HeapAlloc. */
void wm_800978FC(void);

/* wm_8007293C_common_tail_p1: caller slice from accepted P0 frontier.
 * Calls wm_800978FC exactly once.
 * Returns exact new cut PC (0x80072944).
 * Requires P0 to have executed and returned 0x8007293C. */
u32 wm_8007293C_common_tail_p1(void);

/* P1 per-world-init reset. */
void wm_common_tail_p1_reset(void);

/* P1 counter accessors. */
int  wm_ctp1_get_entry(void);
int  wm_ctp1_get_978fc_calls(void);
u32  wm_ctp1_get_last_cut(void);

/* 0x80089160 instrumentation. */
int  wm_89160_get_calls(void);
int  wm_89160_get_iterations(void);
void wm_89160_reset(void);

/* 0x8008901C constants. */
#define WM_8901C_ALLOC_SIZE          0x2800u   /* 10240 bytes */
#define WM_8901C_ALLOC_COUNT         2
#define WM_8901C_RECORD_COUNT        256
#define WM_8901C_RECORD_STRIDE       40u       /* 0x28 */
#define WM_8901C_RECORD_BASE_OFFSET  7         /* ptr + 7 */
#define WM_8901C_COPY_CHUNK          16

/* Global addresses written by wm_8008901C. */
#define WM_D_8009BE1C_ABS            0x8009BE1Cu
#define WM_D_8009BE20_ABS            0x8009BE20u

/* 0x800865A0 constants. */
#define WM_865A0_ALLOC_SIZE          0x2D00u   /* 11520 bytes */
#define WM_865A0_ALLOC_COUNT         2
#define WM_865A0_RECORD_COUNT        288
#define WM_865A0_RECORD_STRIDE       40u       /* 0x28 */
#define WM_865A0_CLUT_OFFSET         14u
#define WM_865A0_COPY_CHUNK          16

/* Global addresses written by wm_800865A0. */
#define WM_D_8009D7F8_ABS            0x8009D7F8u
#define WM_D_8009D7FC_ABS            0x8009D7FCu

/* wm_8008901C: world-map secondary buffer allocator (retail 0x8008901C).
 * Allocates two 10240-byte buffers, initializes 256 × 40-byte records
 * in the first (with GetTPage/GetClut halfwords), copies to second.
 * Self-contained — only calls HeapAlloc + PsyQ GetTPage/GetClut. */
void wm_8008901C(void);

/* wm_80072944_common_tail_p2: caller slice from accepted P1 frontier.
 * Calls wm_8008901C exactly once.
 * Returns exact new cut PC (0x8007294C).
 * Requires P1 to have executed and returned 0x80072944. */
u32 wm_80072944_common_tail_p2(void);

/* P2 per-world-init reset. */
void wm_common_tail_p2_reset(void);

/* P2 counter accessors. */
int  wm_ctp2_get_entry(void);
int  wm_ctp2_get_8901c_calls(void);
u32  wm_ctp2_get_last_cut(void);
int  wm_ctp2_get_alloc_calls(void);
int  wm_ctp2_get_forbidden_865a0(void);
int  wm_ctp2_get_forbidden_85fe0(void);
int  wm_ctp2_get_forbidden_scheduler(void);
int  wm_ctp2_get_forbidden_world_loop(void);

/* wm_800865A0: world-map tertiary buffer allocator (retail 0x800865A0).
 * Allocates two 11520-byte (0x2D00) buffers via HeapAlloc.
 * Stores pointers at D_8009D7F8 and D_8009D7FC.
 *
 * Initializes 288 records (40 bytes each) in the first buffer.
 * Record base = alloc_ptr + 14.  Per-record writes:
 *   byte[+3]  = 9    (type marker)
 *   byte[+4]  = 38 (0x26)
 *   byte[+5]  = 38 (0x26)
 *   byte[+6]  = 38 (0x26)
 *   byte[+7]  = 44 (0x2C), then OR'd with 0x02 → 0x2E (SetSemiTrans)
 *   hw[+14]   = GetTPage(0, 1, 960, 256)
 *   hw[+22]   = GetClut(304, 510)
 *
 * Then copies first buffer → second buffer (11520 bytes, 16-byte chunks).
 *
 * Calls: HeapAlloc, PsyQ GetTPage, PsyQ GetClut, PsyQ SetSemiTrans. */
void wm_800865A0(void);

/* wm_8007294C_common_tail_p3: caller slice from accepted P2 frontier.
 * Calls wm_800865A0 exactly once.
 * Returns exact new cut PC (0x80072954).
 * Requires P2 to have executed and returned 0x8007294C. */
u32 wm_8007294C_common_tail_p3(void);

/* P3 per-world-init reset. */
void wm_common_tail_p3_reset(void);

/* P3 counter accessors. */
int  wm_ctp3_get_entry(void);
int  wm_ctp3_get_865a0_calls(void);
u32  wm_ctp3_get_last_cut(void);
int  wm_ctp3_get_alloc_calls(void);
int  wm_ctp3_get_forbidden_85fe0(void);
int  wm_ctp3_get_forbidden_scheduler(void);
int  wm_ctp3_get_forbidden_world_loop(void);
int  wm_ctp3_get_forbidden_75228(void);

/* 0x80085FE0 constants. */
#define WM_85FE0_ALLOC_SIZE          0x5000u   /* 20480 bytes */
#define WM_85FE0_ALLOC_COUNT         2
#define WM_85FE0_RECORD_COUNT        512
#define WM_85FE0_RECORD_STRIDE       40u       /* 0x28 */
#define WM_85FE0_RECORD_BASE_OFFSET  22        /* ptr + 22 */
#define WM_85FE0_COPY_CHUNK          16

/* Global addresses written by wm_80085FE0. */
#define WM_D_8009D7E8_ABS            0x8009D7E8u
#define WM_D_8009D7EC_ABS            0x8009D7ECu

/* Common-tail P4 constants. */
#define WM_COMMON_TAIL_P4_START      0x80072954u
#define WM_COMMON_TAIL_P4_CUT        0x8007295Cu  /* first excluded after wm_80085FE0 */

/* wm_80085FE0: world-map quaternary buffer allocator (retail 0x80085FE0).
 * Allocates two 20480-byte (0x5000) buffers via HeapAlloc.
 * Stores pointers at D_8009D7E8 and D_8009D7EC.
 *
 * Initializes 512 records (40 bytes each) in the first buffer.
 * Record base = alloc_ptr + 22.  Per-record writes:
 *   byte[-19] = 9    (type marker)
 *   byte[-18] = 128  (R)
 *   byte[-17] = 128  (G)
 *   byte[-16] = 128  (B)
 *   byte[-15] = 44   (code byte)
 *   byte[-10] = 0
 *   byte[-9]  = 64   (0x40)
 *   byte[-2]  = 31   (0x1F)
 *   byte[-1]  = 64   (0x40)
 *   byte[+6]  = 0
 *   byte[+7]  = 111  (0x6F)
 *   byte[+14] = 31   (0x1F)
 *   byte[+15] = 111  (0x6F)
 *   hw[-8]    = GetClut(240, 511)
 *   hw[+0]    = GetTPage(0, 0, 896, 256)
 *
 * Then copies first buffer → second buffer (20480 bytes, 16-byte chunks).
 *
 * Calls: HeapAlloc, PsyQ GetTPage, PsyQ GetClut. */
void wm_80085FE0(void);

/* wm_80072954_common_tail_p4: caller slice from accepted P3 frontier.
 * Calls wm_80085FE0 exactly once.
 * Returns exact new cut PC (0x8007295C).
 * Requires P3 to have executed and returned 0x80072954. */
u32 wm_80072954_common_tail_p4(void);

/* P4 per-world-init reset. */
void wm_common_tail_p4_reset(void);

/* P4 counter accessors. */
int  wm_ctp4_get_entry(void);
int  wm_ctp4_get_85fe0_calls(void);
u32  wm_ctp4_get_last_cut(void);
int  wm_ctp4_get_alloc_calls(void);
int  wm_ctp4_get_forbidden_75228(void);
int  wm_ctp4_get_forbidden_scheduler(void);
int  wm_ctp4_get_forbidden_world_loop(void);

/* 0x80075228 constants. */
#define WM_80075228_START            0x80075228u
#define WM_80075228_END_EXCLUSIVE    0x8007528Cu
#define WM_75228_HALFWORD_COUNT      16
#define WM_75228_FLAG_CROSS          0x4000u
#define WM_75228_VALUE_CROSS         0x0300u   /* 768 */
#define WM_75228_VALUE_NO_CROSS      0x0180u   /* 384 */

/* Global addresses read/written by P5. */
#define WM_FLAG_C894_ABS             0x8009C894u  /* ready flag: entrance bit 0x8000 */
#define WM_D_8009C872_ABS            0x8009C872u
#define WM_D_8009D64C_ABS            0x8009D64Cu
#define WM_D_8009BE40_ABS            0x8009BE40u
#define WM_D_8009BCC4_ABS            0x8009BCC4u
#define WM_D_8009D80C_ABS            0x8009D80Cu
#define WM_BUTTONS_ABS               0x8006EE68u

/* SystemTransferPaletteToVRAM constants (0x80033698). */
#define WM_PALETTE_33698             0x80033698u
#define WM_PALETTE_ARCHIVE_IDX       0x130u    /* 304 */
#define WM_PALETTE_Y_POS             0x1E0u    /* 480 */

/* Common-tail P5 constants.
 *
 * P5 is the final overlay-local slice. After P5 completes, slot-1 returns
 * to its caller (WorldMapMain dispatch at 0x8007105C→0x80071064).
 *
 * WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL: overlay-local first excluded /
 * slot-2 callback entry. NOT the actual execution frontier.
 * WM_COMMON_TAIL_P5_REAL_RETURN_PC: actual retail post-slot-1 return
 * point where WorldMapMain resumes (before scheduler call). */
#define WM_COMMON_TAIL_P5_START               0x8007295Cu
#define WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL  0x8007299Cu  /* slot-2 entry, overlay-local boundary */
#define WM_COMMON_TAIL_P5_REAL_RETURN_PC      0x80071064u  /* actual post-slot-1 return to caller */
#define WM_COMMON_TAIL_P5_CUT                 WM_COMMON_TAIL_P5_SLOT1_END_SENTINEL

/* wm_80075228: world-map palette state initializer (retail 0x80075228).
 * Zeroes 16 halfwords at D_8009C872, sets D_8009D64C=1,
 * writes 768/384 to D_8009BE40 based on button 0x4000,
 * sets D_8009BCC4=1 and D_8009D80C=0.
 * Leaf function — no external calls. */
void wm_80075228(void);

/* wm_8007295C_common_tail_p5: caller slice from accepted P4 frontier.
 * Reads C894 ready flag. Two paths:
 *   C894 == 0: calls wm_80075228 (natural Lahan), then falls through.
 *   C894 != 0: skips wm_80075228, falls through.
 * Both paths call SystemTransferPaletteToVRAM(0x130, 0x1E0) then epilogue.
 * Returns overlay-local sentinel (0x8007299C = slot-2 entry).
 * Actual post-slot-1 return PC is 0x80071064 (WorldMapMain resumes before scheduler).
 * Requires P4 to have executed and returned 0x8007295C. */
u32 wm_8007295C_common_tail_p5(void);

/* P5 per-world-init reset. */
void wm_common_tail_p5_reset(void);

/* P5 counter accessors. */
int  wm_ctp5_get_entry(void);
int  wm_ctp5_get_c894_zero(void);
int  wm_ctp5_get_c894_nonzero(void);
int  wm_ctp5_get_75228_calls(void);
int  wm_ctp5_get_palette_calls(void);
u32  wm_ctp5_get_last_cut(void);
int  wm_ctp5_get_forbidden_scheduler(void);
int  wm_ctp5_get_forbidden_world_loop(void);
int  wm_ctp5_get_forbidden_drawotag(void);

#endif /* WORLD_MAP_COMMON_TAIL_H */
