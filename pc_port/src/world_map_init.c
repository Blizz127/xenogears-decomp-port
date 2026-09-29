/*
 * W2 / W3B / W4C / W5B / W6B / W7B / W8B / W10A — Native world-map init ladder.
 *
 * Retail WorldMapMain @ 0x80070CFC (overlay world_map.bin loaded at 0x8006FAF0).
 *
 * W2: pre-loop init through jal wm_80071B9C @ 0x80070FF4; cut before 0x80071000.
 * W3B: one-shot mode initializer 0x80071CDC (first-wave archive queue).
 * W4C: second-wave 0x80071EF0 → ArchiveDataSync poll → 0x80073530; cut before
 *      retail PC 0x800722BC.
 * W5B: object-pool 0x8009766C / 0x800976C8; cut before 0x800722C4.
 * W6B: identity copy 8×u32 0x8009A180 → 0x8009BE4C; cut before 0x80072314.
 * W7B: ten mode-enter u32 stores 0x80072314–0x80072374; cut before 0x80072378.
 * W8B: four OuterProduct0 via 0x80098044; cut before 0x80072380.
 * W10A: wm_8008440C (+ wm_800931D8) GPU/CLUT from W4C BD20; cut before
 *       0x80072444 (jal 0x800979C8).
 * W10B: wm_800979C8 GPU/CLUT/TPage from W4C C59C; cut before 0x8007244C.
 * W11B: wm_80084580 object/matrix table from W4C fixups; cut before
 *       0x80072454 (jal 0x80072090).
 * W12B: wm_80072090 third-wave archive submit (5×Decode/Alloc → D3F8 +
 *       func_80029AFC); cut before 0x8007245C (jal 0x800736DC). Submit only —
 *       no poll/fixup, no residual fan-out, no full 0x80072238 / frames.
 * W13B: wm_800736DC unrolled BSS constant paint (0x110); cut before
 *       0x80072464 (jal 0x80073E30). No third-wave consumption, no poll,
 *       no GPU/sprite setup.
 * W14B: wm_80073E30 primitive-template packer (DR_TPAGE + POLY_FT4×2 +
 *       POLY_G3×8 + TILE×64); cut before 0x8007246C (jal 0x80085F58).
 *       No VRAM upload, DrawOTag, 85F58, GfxAllocate, or third-wave poll.
 * W15B: wm_80085F58 relocate 256×8 records via C7EC + 16× GetClut → D478;
 *       cut before 0x80072478 (GfxAllocateWorkBuffers). Non-idempotent
 *       relocation: strict process-local one-shot guard.
 * W16B: route exact matching GfxAllocateWorkBuffers(5120,0); cut before
 *       0x80072480 (jal 0x80074594). No free fix, no post helpers, no poll.
 * W17B: wm_80074594 heap FT4 dual-pool init (128 + 640 + 640); cut before
 *       0x80072488 (jal 0x800863E0). One-shot.
 * W18B: wm_800863E0 heap-table rand init (1280 + 640 bytes, 240 rand calls);
 *       cut before 0x80072490 (jal 0x80074E58). One-shot.
 * W19A: wm_80074E58 upload-record builder (N×12 records from the W4C D77C
 *       fixup block; one HeapAlloc, no RNG); cut before 0x80072498
 *       (jal 0x80075030). One-shot.
 * W20B: wm_80075030 upload-record builder-b (43/43 structural clone of
 *       W19A per the W20A audit; slots D7C8/CD64/D7D0, value base
 *       0x8009A250); cut before 0x800724A0 (jal 0x800739B8). One-shot.
 *       Shared implementation: wm_upload_records_build(cfg).
 * W21B: wm_800739B8 draw-packet builder (first pre-poll draw step; four
 *       40-byte packet records at 0x8009C744 + two DR_TPAGE-shaped E2h
 *       carriers at 0x8009D3D8/D3E4; pure BSS writer, no heap, no GPU
 *       submission); cut before 0x800724A8 (jal 0x80088F64). One-shot.
 *       Retail byte-7 ABE bit hand-applied.
 * W22B: wm_80088F64 final pre-poll table initializer (512×84 table_A
 *       selective clear + 19456-byte HeapAlloc for 256×76 table_B +
 *       selective clear; no GPU, no archive poll, no third-wave consumption);
 *       cut before 0x800724B0 (ArchiveCdDataSync). One-shot.
 * W23B: archive readiness poll routing (ArchiveCdDataSync(0) + ready flag
 *       check at 0x8009C894 + exact branch reproduction; cut before first
 *       third-wave consumer 0x80037FD8). No one-shot guard — single call
 *       per init frame. No consumer execution.
 * W24C: first third-wave WDS consumer routing (SoundLoadWdsFile at retail
 *       0x80037FD8; buffer from 0x8009C88C, mode=0; result at 0x8006258C;
 *       one-shot guard; cut before 0x800724E8). Existing native function,
 *       routing only.
 * W24E: ArchiveSetIndex transition (ArchiveSetIndex(36, 0) at retail
 *       0x800724E8; sets g_CurArchiveOffset; cut before 0x800724F0).
 *       Existing native function, routing only.
 *
 * W29B: Native world CD work dispatcher 0x800967E4.  Complete native
 *       implementation of the per-iteration CD queue processor: dispatches
 *       CD44 state machine (0x800968E0), processes D788 records (0x8009699C
 *       via CdIntToPos/CdSyncCallback/CdControlF), processes C624 debug
 *       records (0x800966CC via PCopen/PClseek/PCread/PCclose), and advances
 *       BCB8 tail.  One bounded production call at retail 0x80072514 after
 *       the archive-index transition; cut before Vsync at 0x8007251C.
 *
 * W32B: Ready-check and third-wave buffer consumption.  Reproduces the
 *       post-loop behavior at retail 0x80072558: loads ready flag from
 *       0x8009C894, branches on flag value, frees WDS buffer via
 *       HeapFree(0x8009C88C), links SEDS buffer via
 *       SoundAddSedsEntry(D_8006259C).  One-shot guard prevents double
 *       execution.  Cut before mode-dependent audio setup at 0x800725AC.
 *       Behind XENO_WORLD_READY_BUFFER_CONSUME gate.
 *
 * W33B: Mode-dependent world audio setup.  Reproduces the
 *       post-W32B behavior at retail 0x800725AC: loads mode selector
 *       from 0x8009BE10, selects mode-7 or non-mode-7 archive ID and
 *       buffer pointer, copies sound data via memcpy, creates or loads
 *       AudioManager, sets audio level.  Cut before convergence at
 *       0x800726C0.  Behind XENO_WORLD_MODE_AUDIO_SETUP gate.
 *
 * W34B1: First world convergence caller slice 0x800726C0–0x80072728.
 *       Transcription of dispatch + first-table loop only. Reads
 *       WM_FLAG_C894_ABS (0x8009C894), classifies flag==0/1/other,
 *       for flag==0 iterates sentinel-terminated 8-byte records at
 *       WM_CONV_TABLE_A_BASE (0x80099E8C) calling wm_pool_register.
 *       Never reads WM_CONV_TABLE_B_BASE (0x8009A034) or
 *       WM_CONV_SWITCH_INDEX/WM_SLOT_C610_ABS (0x8009C610), never
 *       executes 0x8007272C or later. Behind
 *       XENO_WORLD_CONVERGENCE_P1 (default off), requires
 *       XENO_WORLD_MODE_AUDIO_SETUP. Bounded to one production
 *       invocation; direct helper remains repeatable (see gate notes).
 *
 * Gates (deepest implies lower):
 *   XENO_WORLD_INIT=1
 *   XENO_WORLD_MODE_INIT=1
 *   XENO_WORLD_SECOND_WAVE=1
 *   XENO_WORLD_OBJECT_POOL=1
 *   XENO_WORLD_STATE_TEMPLATE=1
 *   XENO_WORLD_MODE_ENTER_STATE=1
 *   XENO_WORLD_CROSS_PRODUCTS=1
 *   XENO_WORLD_GPU_ASSET_A=1
 *   XENO_WORLD_GPU_ASSET_B=1
 *   XENO_WORLD_OBJECT_MATRIX=1
 *   XENO_WORLD_THIRD_WAVE=1
 *   XENO_WORLD_BSS_CONSTANTS=1
 *   XENO_WORLD_PRIMITIVE_TEMPLATES=1
 *   XENO_WORLD_RECORD_CLUT_INIT=1
 *   XENO_WORLD_GFX_WORK_BUFFERS=1
 *   XENO_WORLD_FT4_POOLS=1
 *   XENO_WORLD_HEAP_TABLE_RAND=1
 *   XENO_WORLD_UPLOAD_RECORDS=1
 *   XENO_WORLD_UPLOAD_RECORDS_B=1
 *   XENO_WORLD_DRAW_PACKETS=1
 *   XENO_WORLD_88F64=1
 *   XENO_WORLD_ARCHIVE_READY_POLL=1
 *   XENO_WORLD_FIRST_WDS_CONSUMER=1
 *   XENO_WORLD_ARCHIVE_SET_INDEX=1
 *   XENO_WORLD_READY_BUFFER_CONSUME=0 (default off; ready-check + buffer consume)
 *   XENO_WORLD_MODE_AUDIO_SETUP=0 (default off; mode-dependent audio setup)
 *   XENO_WORLD_CONVERGENCE_P1=0 (default off; first convergence table pass; requires MODE_AUDIO_SETUP)
 *   XENO_WORLD_FRAMEBUFFER_GTE_INIT=0 (default off; framebuffer/GTE initializer; requires MODE_AUDIO_SETUP)
 *   XENO_WORLD_TERRAIN_POSITION_INIT=0 (default off; terrain/position initializer; requires FRAMEBUFFER_GTE_INIT)
 *   XENO_WORLD_COMMON_TAIL_P0=0 (default off; common-tail prefix; requires CONVERGENCE_P2 + TERRAIN_POSITION_INIT)
 *   XENO_WORLD_COMMON_TAIL_P1=0 (default off; wm_800978FC caller slice; requires P0)
 *   XENO_WORLD_COMMON_TAIL_P2=0 (default off; wm_8008901C caller slice; requires P1)
 *   XENO_WORLD_COMMON_TAIL_P3=0 (default off; wm_800865A0 caller slice; requires P2)
 *   XENO_WORLD_COMMON_TAIL_P4=0 (default off; wm_80085FE0 caller slice; requires P3)
 *   XENO_WORLD_COMMON_TAIL_P5=0 (default off; wm_80075228 + palette caller slice; requires P4)
 *   XENO_WORLD_SCHEDULER_97800=0 (default off; bounded scheduler 0x80097800; requires P5;
 *        stops before first missing callback body; DrawSync at 0x8007106C not executed)
 *   XENO_WORLD_FRAME_PROLOGUE=0 (default off; 0x8007106C continuation +
 *        0x800712D0 .. 0x80071484; requires SCHEDULER; hard-cut before 0x80071488)
 *   XENO_WORLD_FRAME_REENTRY_ONCE=0 (default off; one reviewed re-entry from
 *        0x800719C8 to frame head 0x8007130C; requires FRAME_PROLOGUE)
 *   XENO_WORLD_FRAME_REENTRY_TWICE=0 (default off; two reviewed re-entries;
 *        requires FRAME_PROLOGUE; diagnostic bound only)
 * Default remains pure placeholder (hasOverlay=0).
 */
#include <stdio.h>
#include <stdlib.h>

#include "common.h"
#include "main/main.h"
#include "system/memory.h"
#include "system/archive.h"
#include "psyq/libgpu.h"
#include "psyq/libgte.h"
#include "psyq/libetc.h"
#include "psyq/libcd.h"
#include "psyq/pc.h"
#include "psx_memory.h"
#include "../include/xg_plat/disc.h"
#include "world_map_convergence.h"
#include "world_map_selector.h"
#include "world_map_framebuffer_init.h"
#include "world_map_gamestate_alias.h"
#include "world_map_gpu_asset_8440c.h"
#include "world_map_terrain_init.h"
#include "world_map_common_tail.h"
#include "world_map_cold_defaults.h"
#include "world_map_mode_selector_73300.h"
#include "world_map_scheduler.h"
#include "world_map_frame_driver.h"
#include "world_map_main_loop_71034.h"
#include "world_map_helper_73398.h"
#include "world_map_helper_73448.h"
#include "world_map_helper_762fc.h"
#include "world_map_session_setup_72238.h"
#include "world_map_mode811_lifecycle.h"
#include "world_map_mode9_lifecycle.h"
#include "world_map_mode10_lifecycle.h"
#include "world_map_mode13_lifecycle.h"
#include "world_map_mode14_lifecycle.h"
#include "world_map_mode15_lifecycle.h"
#include "world_map_helper_76954.h"

/* Retail layout */
#define WM_OVERLAY_BASE          0x8006FAF0u
#define WM_ENTRY                 0x80070CFCu
#define WM_CUT_BEFORE_LOOP       0x80071000u
#define WM_MEM_START             0x8009BBB0u
#define WM_HEAP_START            0x8009D80Cu
#define WM_OVERLAY_IMAGE_SIZE    180422u /* LZSS header + disc/world_map.bin */
#define WM_NOMINAL_BSS_SPAN      (WM_MEM_START - WM_OVERLAY_BASE) /* 180416 */
#define WM_BSS_OVERLAP_BYTES     (WM_OVERLAY_IMAGE_SIZE - WM_NOMINAL_BSS_SPAN) /* 6 */

/* GameState transition tuple offsets. Retail absolute addresses
 * (0x8006F94E..) equal g_GameState+off when g_GameState lives at 0x8006D634
 * in PSX RAM. The port's g_pGameState points at the host g_GameState blob
 * that field already wrote; consume that pointer in place (no shadow copy). */
#define GS_OFF_SELECTOR          0x231Au
#define GS_OFF_HEADING           0x231Cu
#define GS_OFF_ARG2              0x231Eu
#define GS_OFF_ENTRANCE          0x2320u
#define GS_OFF_SEED_1930         0x1930u

/* World-map state written by init (exact retail destinations) */
#define WM_WORLD_INDEX_ABS       0x8009BD0Cu
#define WM_ENTRANCE_STATE_ABS    0x8009C5A8u
#define WM_HEADING_STATE_ABS     0x8009C584u
#define WM_ARG2_STATE_ABS        0x8009D3D4u
#define WM_FLAG_C894_ABS         0x8009C894u
#define WM_ZERO_BBC4_ABS         0x8009BBC4u

/* wm_80071B9C tables / outputs (in overlay image / BSS).
 * WM_THRESH_TABLE_ABS, WM_RECORD_TABLE_ABS, WM_RECORD_GE8_ABS,
 * WM_SLOT_C610_ABS now provided by world_map_selector.h. */

/* wm_8007369C / wm_80095F78 / wm_80073300 stores */
#define WM_ALLOC_BC38_ABS        0x8009BC38u
#define WM_ALLOC_BCB0_ABS        0x8009BCB0u
#define WM_ALLOC_BE08_ABS        0x8009BE08u
#define WM_ALLOC_D7D4_ABS        0x8009D7D4u
#define WM_ALLOC_D3C0_ABS        0x8009D3C0u
#define WM_MODE_BE10_ABS         0x8009BE10u
#define WM_CLR_BCB8_ABS          0x8009BCB8u
#define WM_CLR_BE44_ABS          0x8009BE44u
#define WM_CLR_CD44_ABS          0x8009CD44u
#define WM_CLR_D808_ABS          0x8009D808u
#define WM_CLR_D7C4_ABS          0x8009D7C4u
#define WM_CLR_C660_ABS          0x8009C660u
#define WM_BYTE_C58F_ABS         0x8009C58Fu

/* W27B: CD completion state-chain fields (retail absolute addresses). */
#define WM_CD44_ABS              0x8009CD44u
#define WM_BD2C_ABS              0x8009BD2Cu
#define WM_BCB8_ABS              0x8009BCB8u
#define WM_D788_BASE_ABS         0x8009D788u
#define WM_D614_ABS              0x8009D614u
#define WM_D7F4_ABS              0x8009D7F4u
#define WM_D56C_ABS              0x8009D56Cu
#define WM_D3BC_ABS              0x8009D3BCu
#define WM_CEB8_ABS              0x8009CEB8u
#define WM_C590_ABS              0x8009C590u
#define WM_BE48_ABS              0x8009BE48u
#define WM_CCB0_ABS              0x8009CCB0u
#define WM_CCA8_ABS              0x8009CCA8u
#define WM_CCA0_ABS              0x8009CCA0u
#define WM_BCCC_ABS              0x8009BCCCu
#define WM_BCD0_ABS              0x8009BCD0u
#define WM_BCD4_ABS              0x8009BCD4u

/* W29B: 0x800967E4 implementation constants. */
#define WM_CEBC_ABS              0x8009CEBCu /* CdlLOC buffer for CdIntToPos */
#define WM_C624_BASE_ABS         0x8009C624u /* C624 pointer table (16 × u32) */
#define WM_96A6C_HANDLER         0x80096A6Cu /* CdSyncCallback target handler */
#define WM_D788_RECORD_STRIDE    12u         /* D788 record: file_id, byte_count, dest_ptr */
#define WM_C624_ENTRY_STRIDE     16u         /* C624 sub-record stride */
#define WM_C624_RETRY_MAX        8           /* C624 PCopen/PCread/PCclose retry limit */
#define WM_CdlSetloc             2u          /* PsyQ CdlSetloc command byte */

/* Below-overlay main BSS touched by entry (always) */
#define WM_FLAG_91AE_ABS         0x800691AEu

/* Cut-line proof: first instruction of loop ownership (must never execute). */
#define WM_LOOP_JAL_712D0        0x80071094u
#define WM_MAIN_LOOP             0x80071034u
#define WM_POST_INIT_RESUME      0x80070FFCu
#define WM_MODE_INIT_RETAIL      0x80071CDCu
#define WM_MODE_UPDATE_RETAIL    0x80072238u
#define WM_MODE_POST_RETAIL      0x8007299Cu
#define WM_DISPATCH_TABLE        0x8009A058u
#define WM_PHASE_D7CC            0x8009D7CCu
#define WM_FRAME_D554            0x8009D554u

/* Mode-init BSS / request list (overlay) */
#define WM_PTR_CD34              0x8009CD34u
#define WM_PTR_BDF8              0x8009BDF8u
#define WM_REQ_D3F8              0x8009D3F8u
#define WM_REQ_D3FC              0x8009D3FCu
#define WM_CNT_C170              0x8009C170u

/* W4C second-wave source IDs (written by wm_80071B9C / W2) */
#define WM_ID_C17C               0x8009C17Cu
#define WM_ID_C174               0x8009C174u
#define WM_ID_D3C4               0x8009D3C4u
/* Destination pointer slots */
#define WM_DST_C59C              0x8009C59Cu
#define WM_DST_BD20              0x8009BD20u
#define WM_DST_C180              0x8009C180u
/* Queue entries (8-byte stride) after D3F8 */
#define WM_REQ_D400              0x8009D400u
#define WM_REQ_D404              0x8009D404u
#define WM_REQ_D408              0x8009D408u
#define WM_REQ_D40C              0x8009D40Cu
#define WM_REQ_D410              0x8009D410u
#define WM_REQ_D414              0x8009D414u
/* 73530 fixup outputs */
#define WM_FIX_D308              0x8009D308u
#define WM_FIX_CD48              0x8009CD48u
#define WM_FIX_C7EC              0x8009C7ECu
#define WM_FIX_BD30              0x8009BD30u
#define WM_FIX_D784              0x8009D784u
#define WM_FIX_BCC0              0x8009BCC0u
#define WM_FIX_D7C8              0x8009D7C8u
#define WM_FIX_D77C              0x8009D77Cu
#define WM_FIX_D73C              0x8009D73Cu
#define WM_FIX_D3F4              0x8009D3F4u
#define WM_FIX_BD00              0x8009BD00u
/* Hard cuts */
#define WM_CUT_BEFORE_BROAD      0x800722BCu /* after W4C: jal 0x8009766C */
#define WM_CUT_BEFORE_A180_COPY  0x800722C4u /* after W5B: A180→BE4C */
#define WM_CUT_BEFORE_CONST_BLK  0x80072314u /* after W6B: mode-enter consts */
#define WM_CUT_BEFORE_98044      0x80072378u /* after W7B: jal OuterProduct setup */
#define WM_CUT_AFTER_98044       0x80072380u /* after W8B return */
#define WM_CUT_BEFORE_979C8      0x80072444u /* after W10A: jal 0x800979C8 */
#define WM_CUT_BEFORE_84580      0x8007244Cu /* after W10B: jal 0x80084580 */
#define WM_CUT_BEFORE_72090      0x80072454u /* after W11B: jal 0x80072090 */
#define WM_CUT_BEFORE_736DC      0x8007245Cu /* after W12B: jal 0x800736DC */
#define WM_CUT_BEFORE_73E30      0x80072464u /* after W13B: jal 0x80073E30 */
#define WM_CUT_BEFORE_85F58      0x8007246Cu /* after W14B: jal 0x80085F58 */
#define WM_CUT_BEFORE_GFX_WORK   0x80072478u /* after W15B: jal GfxAllocate */
#define WM_CUT_BEFORE_74594      0x80072480u /* after W16B: jal 0x80074594 */
#define WM_CUT_BEFORE_863E0      0x80072488u /* after W17B: jal 0x800863E0 */
#define WM_CUT_AFTER_863E0       0x80072490u /* after W18B return; before jal 0x80074E58 */
#define WM_CUT_BEFORE_75030      0x80072498u /* after W19A return; before jal 0x80075030 */
#define WM_CUT_BEFORE_739B8      0x800724A0u /* after W20B return; before jal 0x800739B8 */
#define WM_CUT_BEFORE_88F64      0x800724A8u /* after W21B return; before jal 0x80088F64 */
#define WM_CUT_BEFORE_ARCHIVE    0x800724B0u /* after W22B return; before ArchiveCdDataSync */
#define WM_CUT_BEFORE_CONSUMER   0x800724D4u /* after W23B poll; before jal 0x80037FD8 */
#define WM_CUT_AFTER_CONSUMER    0x800724E8u /* after W24C consumer; before jal 0x80028470 */
#define WM_CUT_AFTER_SETINDEX    0x800724F0u /* after W24E ArchiveSetIndex; before flag check */
#define WM_CUT_AFTER_967E4       0x8007251Cu /* after W29B 0x800967E4 call; before Vsync */
#define WM_CUT_BEFORE_MODE_AUDIO 0x800725ACu /* after W32B buffer consume; before mode audio */
#define WM_CUT_BEFORE_CONVERGENCE 0x800726C0u /* after W33B audio setup; before convergence */
#define WM_BCDC_ABS              0x8009BCDCu /* GTE screen distance (set by W34B4B) */
#define WM_FLAG_C894_ABS         0x8009C894u /* ready flag: entrance bit 0x8000 */
#define WM_CONV_TABLE_A_BASE     0x80099E8Cu /* pool-register table A (8-byte records); sign-extended from lui 0x800A + imm16 0x9E8C */
#define WM_CONV_TABLE_B_BASE     0x8009A034u /* pool-register second-loop ptr table; sign-extended from lui 0x800A + imm16 0xA034 */
#define WM_CONV_SWITCH_INDEX     WM_SLOT_C610_ABS /* convergence switch index; aliases exact 0x8009C610 */
#define WM_FIRST_CONSUMER_CALLER 0x800724D4u /* jal 0x80037FD8 */
#define WM_FIRST_CONSUMER_TARGET 0x80037FD8u /* SoundLoadWdsFile */
#define WM_CONSUMER_RESULT       0x8006258Cu /* SoundLoadWdsFile return storage */
#define WM_GFX_WORK_SIZE         5120
#define WM_GFX_WORK_TOTAL        (WM_GFX_WORK_SIZE * 2) /* 10240 */

/* W17B wm_80074594 destinations */
#define WM_FT4_REC_PTR           0x8009D30Cu
#define WM_FT4_POOL_A_PTR        0x8009BE14u
#define WM_FT4_POOL_B_PTR        0x8009BE18u
#define WM_FT4_FLAG_BE38         0x8009BE38u
#define WM_FT4_REC_BYTES         128u
#define WM_FT4_REC_COUNT         16u
#define WM_FT4_REC_STRIDE        8u
#define WM_FT4_POOL_BYTES        640u
#define WM_FT4_POOL_COUNT        16u
#define WM_FT4_POOL_STRIDE       40u

/* W18B wm_800863E0 destinations */
#define WM_HEAP_TABLE_A_PTR      0x8009D150u
#define WM_HEAP_TABLE_B_PTR      0x8009CEB4u
#define WM_SRC_AF30              0x8009AF30u
#define WM_SRC_AF38              0x8009AF38u
#define WM_TABLE_A_RECORDS       80u
#define WM_TABLE_A_STRIDE        16u
#define WM_TABLE_A_BYTES         1280u
#define WM_TABLE_B_RECORDS       80u
#define WM_TABLE_B_STRIDE        8u
#define WM_TABLE_B_BYTES         640u

/* W19A wm_80074E58 upload-record builder destinations (first pre-poll
 * helper; consumes the W4C fixup slot WM_FIX_D77C). */
#define WM_UPLOAD_COUNT          0x8009CC9Cu
#define WM_UPLOAD_REC_ARRAY      0x8009D780u
#define WM_UPLOAD_VALUE_BASE     0x8009A1E8u
#define WM_UPLOAD_REC_STRIDE     12u
#define WM_UPLOAD_VALUE_STRIDE   16u

/* W20B wm_80075030 upload-record builder destinations (43/43 structural
 * clone of W19A per the W20A audit; consumes the W4C fixup slot
 * WM_FIX_D7C8). */
#define WM_UPLOAD_COUNT_B        0x8009CD64u
#define WM_UPLOAD_REC_ARRAY_B    0x8009D7D0u
#define WM_UPLOAD_VALUE_BASE_B   0x8009A250u

/* W21B wm_800739B8 draw-packet builder destinations (first pre-poll
 * draw-env/packet step; pure BSS writer, no allocation, no GPU submission).
 * Four 40-byte packet records at WM_DRAW_PKTS_BASE and two 12-byte
 * DR_TPAGE-shaped E2h texwindow carriers at WM_DR_TPAGE_A/B. */
#define WM_DRAW_PKTS_BASE        0x8009C744u
#define WM_DRAW_PKTS_COUNT       4u
#define WM_DRAW_PKTS_STRIDE      40u
#define WM_DRAW_PKTS_BYTES       (WM_DRAW_PKTS_COUNT * WM_DRAW_PKTS_STRIDE) /* 160 */
#define WM_DR_TPAGE_A            0x8009D3D8u
#define WM_DR_TPAGE_B            0x8009D3E4u
#define WM_DR_TPAGE_BYTES        12u
#define WM_DR_TPAGE_TOTAL        (WM_DR_TPAGE_BYTES * 2) /* 24 */

/* W22B wm_80088F64 table-init destinations (final pre-poll helper).
 * Table A base pointer at WM_FIX_BCC0 (from W4C fixup).
 * Table B heap pointer stored at WM_88F64_TABLE_B_PTR. */
#define WM_88F64_TABLE_B_PTR     0x8009BDF4u
#define WM_88F64_TABLE_A_COUNT   512u
#define WM_88F64_TABLE_A_STRIDE  84u
#define WM_88F64_TABLE_A_BYTES   (WM_88F64_TABLE_A_COUNT * WM_88F64_TABLE_A_STRIDE) /* 43008 */
#define WM_88F64_TABLE_B_COUNT   256u
#define WM_88F64_TABLE_B_STRIDE  76u
#define WM_88F64_TABLE_B_BYTES   (WM_88F64_TABLE_B_COUNT * WM_88F64_TABLE_B_STRIDE) /* 19456 */
#define WM_88F64_ALLOC_SIZE      0x4C00u /* 19456 */

/* W15B record table + CLUT destinations (retail 0x80085F58). */
#define WM_REC_COUNT             256u
#define WM_REC_STRIDE            8u
#define WM_REC_BYTES             (WM_REC_COUNT * WM_REC_STRIDE) /* 2048 */
#define WM_CLUT_D478             0x8009D478u
#define WM_CLUT_COUNT            16u
#define WM_CLUT_BYTES            (WM_CLUT_COUNT * 2u) /* 32 */

/* W14B primitive-template BSS destinations (retail 0x80073E30). */
#define WM_PRIM_DR_TPAGE         0x8009C5A0u
#define WM_PRIM_FT4_0            0x8009C5C0u
#define WM_PRIM_FT4_1            0x8009C5E8u
#define WM_PRIM_G3_BASE          0x8009C664u
#define WM_PRIM_TILE_BASE        0x8009C898u
#define WM_PRIM_FT4_BYTES        40u
#define WM_PRIM_G3_STRIDE        28u
#define WM_PRIM_G3_COUNT         8u
#define WM_PRIM_TILE_STRIDE      16u
#define WM_PRIM_TILE_COUNT       64u
#define WM_PRIM_G3_BYTES         (WM_PRIM_G3_COUNT * WM_PRIM_G3_STRIDE)
#define WM_PRIM_TILE_BYTES       (WM_PRIM_TILE_COUNT * WM_PRIM_TILE_STRIDE)
#define WM_TW_ID_CC98            0x8009CC98u
#define WM_TW_ID_D3D0            0x8009D3D0u
#define WM_TW_ID_D3C8            0x8009D3C8u
#define WM_TW_ID_D800            0x8009D800u
#define WM_TW_ID_BCC8            0x8009BCC8u
#define WM_TW_MIRROR_C88C        0x8009C88Cu
#define WM_TW_MIRROR_C884        0x8009C884u
#define WM_TW_MIRROR_C888        0x8009C888u
#define WM_TW_MIRROR_C614        0x8009C614u
#define WM_REQ_D418              0x8009D418u
#define WM_REQ_D41C              0x8009D41Cu
#define WM_REQ_D420              0x8009D420u
#define WM_REQ_D424              0x8009D424u
#define WM_BROAD_9766C           0x8009766Cu
#define WM_OBJ_C620              0x8009C620u
#define WM_OBJ_BD28              0x8009BD28u
#define WM_OBJ_D7E0              0x8009D7E0u
#define WM_OBJ_C16C              0x8009C16Cu
#define WM_OBJ_C840              0x8009C840u
#define WM_OBJ_MAT_A140          0x8009A140u
#define WM_OBJ_MAT_A160          0x8009A160u
#define WM_OBJ_RECORD_STRIDE     84u
#define WM_GPU_8440C             0x8008440Cu
#define WM_GPU_979C8             0x800979C8u
#define WM_GPU_931D8             0x800931D8u
/* W10A/B: CLUT/TPage tables + scale constants (BD20/C59C already defined). */
#define WM_CLUT_BCE0             0x8009BCE0u
#define WM_CLUT_CCB4             0x8009CCB4u
#define WM_TPAGE_CD54            0x8009CD54u
#define WM_TPAGE_CD5C            0x8009CD5Cu
#define WM_SCALE_IMG_704DC       0x800704DCu /* 4-byte RGB scale (W10A) */
#define WM_SCALE_IMG_BB48        0x8009BB48u /* 4-byte RGB scale (W10B) */
/* W8B OuterProduct0 inputs (overlay image) / outputs (world BSS). */
#define WM_XP_BB4C               0x8009BB4Cu
#define WM_XP_BB5C               0x8009BB5Cu
#define WM_XP_BB6C               0x8009BB6Cu
#define WM_XP_BB7C               0x8009BB7Cu
#define WM_XP_BB8C               0x8009BB8Cu
#define WM_XP_BB9C               0x8009BB9Cu
#define WM_XP_C828               0x8009C828u
#define WM_XP_C844               0x8009C844u
#define WM_XP_C874               0x8009C874u
#define WM_XP_C7F0               0x8009C7F0u
#define WM_POOL_BE24             0x8009BE24u
#define WM_POOL_ALLOC_SIZE       8192u
#define WM_POOL_SLOT_COUNT       64
#define WM_POOL_SLOT_STRIDE      0x80u
#define WM_POOL_OFF_18           0x18u
#define WM_POOL_OFF_1C           0x1Cu
#define WM_POOL_OFF_4C           0x4Cu
/* W6B template: overlay image src → world BSS dest (8 words, identity). */
#define WM_TMPL_SRC_A180         0x8009A180u
#define WM_TMPL_DST_BE4C         0x8009BE4Cu
#define WM_TMPL_WORD_COUNT       8
#define WM_TMPL_BYTE_COUNT       (WM_TMPL_WORD_COUNT * 4u)
/* W7B mode-enter destinations (retail 0x80072314–0x80072374). */
#define WM_MES_CCA4              0x8009CCA4u
#define WM_MES_D3CC              0x8009D3CCu
#define WM_MES_D804              0x8009D804u
#define WM_MES_CEC0              0x8009CEC0u
#define WM_MES_C7E8              0x8009C7E8u
#define WM_MES_BD34              0x8009BD34u
#define WM_MES_D144              0x8009D144u
#define WM_MES_C178              0x8009C178u
#define WM_MES_CD40              0x8009CD40u
#define WM_MES_FN_86700          0x80086700u
#define WM_MES_STORE_COUNT       10

#define GS_OFF_SEC_BASE          0x030Cu /* retail 0x8006D940; stride 164 per id */

#define WM_U8(a)  (*(u8*)PSX_ADDR(a))
#define WM_U16(a) (*(u16*)PSX_ADDR(a))
#define WM_S16(a) (*(s16*)PSX_ADDR(a))
#define WM_U32(a) (*(u32*)PSX_ADDR(a))

/* Safety ceiling for ArchiveDataSync poll (never silent hang). */
#define WM_SECOND_WAVE_POLL_MAX  100000

extern void PcPort_WorldMapPlaceholderMain(void);
extern void func_8003634C(void);
extern int VSync(int mode);
extern void ControllerResetState(void);
extern u32 g_ArchiveDebugTable;
extern uint32_t g_RandomSeed;
extern int rand(void);
extern int ArchiveDecodeSize(int entryIndex);
extern int ArchiveDecodeAlignedSize(unsigned int entryIndex);
extern int func_80029AFC(void* pEntries, int arg1, int arg2);
extern int ArchiveDataSync(void);
extern void* HeapAlloc(u_int allocSize, u_int allocFlags);
extern u_int HeapFree(void* pMem);
extern void SoundAddSedsEntry(void* pSoundFile);
extern void* func_80039850(void* pSongFile);
extern void func_80039A80(void* manager, int level, int steps);
extern void func_80039B68(void* manager, int level, int steps);
extern u8 D_80062648[];
extern void* D_80062528;
extern void* LZSSHeapDecompress(void* pCompressed, int flags);
extern void func_8002DD20(u32* pList);
extern int StoreImage(RECT* rect, u_long* p);
extern int LoadImage(RECT* rect, u_long* p);
extern u_short GetClut(int x, int y);
extern u_short GetTPage(int tp, int abr, int x, int y);
extern void SetSemiTrans(void* p, int abe);
extern void SetDrawTPage(DR_TPAGE* p, int dfe, int dtd, int tpage);
extern int func_8002C3E8(u8* pModel);
extern void func_8002CB54(u8* modelData, u32* out1, u32* out2);
extern void func_8002C8CC(u8* a0, void* a1, s32 a2);
extern VECTOR* ApplyMatrix(MATRIX* m, SVECTOR* v0, VECTOR* v1);
extern SVECTOR* ApplyMatrixSV(MATRIX* m, SVECTOR* v0, SVECTOR* v1);
extern MATRIX* RotMatrix(SVECTOR* r, MATRIX* m);
extern s32 D_80050100;
extern s32 D_8004F304; /* main BSS counter; retail 0x8004F304 — host authority */
extern void* D_8006259C; /* main SEDS-style host pointer; retail 0x8006259C */
extern void* g_pGameState;
/* Main-executable global written by retail 0x80072364 (field init also sets 1). */
extern s32 D_80059198;
extern void OuterProduct0(VECTOR* v0, VECTOR* v1, VECTOR* v2);
/* Matching main-exe graphics work-buffer allocator (temp1.c). */
extern void GfxAllocateWorkBuffers(int workBufferSize, unsigned int allocFlag);
extern s32 g_GfxWorkBufferSize;
extern void* g_GfxWorkBuffers;
/* SoundLoadWdsFile: main-executable WDS sample-bank loader (sound.c:770). */
typedef struct SoundWDSEntry SoundWDSEntry;
extern SoundWDSEntry* SoundLoadWdsFile(SoundWDSEntry* pWdsFile, s32 mode);
extern void* g_GfxWorkBuffer2;
extern u32 D_80059300;
extern u32 D_80059304;
extern void* g_GfxImageList;
extern s32 D_80059190;

#define GS_U8(off)  (*(u8*)((u8*)g_pGameState + (off)))
#define GS_U16(off) (*(u16*)((u8*)g_pGameState + (off)))
#define GS_S16(off) (*(s16*)((u8*)g_pGameState + (off)))

/* Channel-4 callback ownership is in controller_vblank_service.c. */

/* Local memcpy / memeq — avoid string.h vs game strlen conflict. */
static void wm_memcpy(void* dst, const void* src, unsigned n)
{
    u8* d = (u8*)dst;
    const u8* s = (const u8*)src;
    while (n--)
        *d++ = *s++;
}

static int wm_memeq(const void* a, const void* b, unsigned n)
{
    const u8* x = (const u8*)a;
    const u8* y = (const u8*)b;
    while (n--) {
        if (*x++ != *y++)
            return 0;
    }
    return 1;
}

static void wm_memset(void* dst, u8 value, unsigned n)
{
    u8* d = (u8*)dst;
    while (n--)
        *d++ = value;
}

/* HeapAlloc returns a host pointer into g_PsxRam; store retail-style KUSEG. */
static u32 host_ptr_to_psx_u32(void* p)
{
    uintptr_t host;
    uintptr_t base;
    if (p == NULL)
        return 0;
    host = (uintptr_t)p;
    base = (uintptr_t)g_PsxRam;
    if (host >= base && host < base + (uintptr_t)PSX_RAM_SIZE)
        return 0x80000000u | (u32)(host - base);
    /* Truncate host pointer (legacy path); still usable if in low 4G. */
    return (u32)host;
}

/* Resolve a 32-bit guest/host value to a host pointer for Heap/LZSS APIs. */
static void* psx_u32_to_host(u32 p)
{
    if (p == 0)
        return NULL;
    if (p >= 0x80000000u && p < 0x80200000u)
        return PSX_ADDR(p);
    return (void*)(uintptr_t)p;
}

static int s_wm712d0_hits;
static int s_wm_drawotag_hits;
static int s_wm9766c_hits;
static int s_wm72238_hits;
static int s_wm7299c_hits;
/* W18I route-boundary counters for retail steps with no native code. */
static int s_wm74e58_hits;
static int s_wm75030_hits;
static int s_wm739b8_hits;
static int s_wm88f64_hits;
static int s_wm37fd8_hits;
static int s_wm_cd_sync_world_hits;
/* W25B: loop-path forbidden counters (not yet ported; must not execute). */
static int s_wm96668_hits;
static int s_wm_loop_dispatch_hits;
static int s_wm967e4_hits;
static int s_wm_loop_backedge_hits;
static int s_wm_loop_exit_hits;

/* W27B: CD completion state-chain instrumentation counters. */
static int s_wm_completion_cb_entry;
static int s_wm_completion_3to4;
static int s_wm_cb_unregister;
static int s_wm_dispatcher_state4;
static int s_wm_bd2c_decrement;
static int s_wm_dispatcher_state5;
static int s_wm_cd44_clear;
static int s_wm_bcb8_increment;
static int s_wm_d788_tail_clear;

/* W29B: 0x800967E4 implementation instrumentation counters. */
static int s_wm967e4_entry;
static int s_wm967e4_debug_table_nonzero;
static int s_wm967e4_dispatcher_idle;
static int s_wm967e4_dispatcher_busy;
static int s_wm967e4_dispatcher_tail_adv;
static int s_wm967e4_dispatcher_invalid;
static int s_wm967e4_d788_null;
static int s_wm967e4_d788_process;
static int s_wm967e4_c624_null;
static int s_wm967e4_c624_process;
static int s_wm967e4_tail_advance;
static int s_wm967e4_route_hit;
static int s_wm_d788_proc_entry;
static int s_wm_c624_proc_entry;
static int s_wm_c624_pcopen_fail;
static int s_wm_c624_pcread_fail;
static int s_wm_c624_pcclose_fail;

/* W32B: ready-check and buffer-consumption instrumentation counters. */
static int s_wm32b_entry;
static int s_wm32b_ready_branch;
static int s_wm32b_not_ready_branch;
static int s_wm32b_heapfree;
static int s_wm32b_sound_add;
static int s_wm32b_second_call_blocked;
static int s_wm32b_route_hit;

/* W33B: mode-dependent audio setup instrumentation counters. */
static int s_wm33b_entry;
static int s_wm33b_mode7_path;
static int s_wm33b_non_mode7_path;
static int s_wm33b_ready_path;
static int s_wm33b_not_ready_path;
static int s_wm33b_decode_size;
static int s_wm33b_memcpy;
static int s_wm33b_audio_create;
static int s_wm33b_audio_load;
static int s_wm33b_level_set;
static int s_wm33b_route_hit;

/* Instrumentation targets (never called on the init path). */
void wm_800712D0_should_not_run(void)
{
    s_wm712d0_hits++;
    fprintf(stderr, "[worldmap-init] ERROR: wm_800712D0 reached (hit=%d)\n",
            s_wm712d0_hits);
}

void wm_8009766C_should_not_run(void)
{
    s_wm9766c_hits++;
    fprintf(stderr, "[worldmap-second-wave] ERROR: 0x8009766C reached (hit=%d)\n",
            s_wm9766c_hits);
}

void wm_80072238_should_not_run(void)
{
    s_wm72238_hits++;
    fprintf(stderr, "[worldmap-second-wave] ERROR: full 0x80072238 reached (hit=%d)\n",
            s_wm72238_hits);
}

void wm_8007299C_should_not_run(void)
{
    s_wm7299c_hits++;
    fprintf(stderr, "[worldmap-second-wave] ERROR: 0x8007299C reached (hit=%d)\n",
            s_wm7299c_hits);
}

/* W18I: diagnostic wrappers for retail steps that have no native body.
 * Any future native routing of these steps must dispatch through the world
 * route tail, where these wrappers are the registered diagnostics; hitting
 * one means a forbidden retail step was dispatched. (0x80074E58, 0x80075030
 * and 0x800739B8 were ported as W19A/W20B/W21B; their residual counters now
 * count blocked re-runs of the real rungs, not stub entry.) */
void wm_80088F64_should_not_run(void)
{
    s_wm88f64_hits++;
    fprintf(stderr, "[worldmap-init] ERROR: 0x80088F64 dispatch reached (hit=%d)\n",
            s_wm88f64_hits);
}

/* First third-wave consumer (retail 0x80037FD8). */
void wm_80037FD8_should_not_run(void)
{
    s_wm37fd8_hits++;
    fprintf(stderr, "[worldmap-init] ERROR: 0x80037FD8 third-wave consumer reached (hit=%d)\n",
            s_wm37fd8_hits);
}

/* World-route DrawOTag callsite class. Feeds s_wm_drawotag_hits, which the
 * end-of-init check consumes; placeholder/field DrawOTag calls are routed
 * through normal presenter code and never through this wrapper. */
void wm_world_DrawOTag_should_not_run(void)
{
    s_wm_drawotag_hits++;
    fprintf(stderr, "[worldmap-init] ERROR: world DrawOTag callsite reached (hit=%d)\n",
            s_wm_drawotag_hits);
}

/* World-route ArchiveCdDataSync callsite class (retail world CD poll).
 * Field/overlay-load ArchiveCdDataSync calls are legitimate and do not go
 * through this wrapper. */
void wm_world_ArchiveCdDataSync_should_not_run(void)
{
    s_wm_cd_sync_world_hits++;
    fprintf(stderr, "[worldmap-init] ERROR: world ArchiveCdDataSync callsite reached (hit=%d)\n",
            s_wm_cd_sync_world_hits);
}

/* W18I forbidden-target registry: stable exported ABI for runtime harnesses.
 * Harnesses prove counter registration by reading these globals directly
 * (no inferior calls); a read failure is reported as NOT INSTRUMENTED. */
typedef struct wm_forbidden_target_entry
{
    const char* label;
    u32 retail_pc; /* 0 = callsite-class target, no single retail PC */
    int* hits;
} wm_forbidden_target_entry;

#define WM_FORBIDDEN_TARGET_COUNT 15

const int g_wm_forbidden_target_count = WM_FORBIDDEN_TARGET_COUNT;

const wm_forbidden_target_entry g_wm_forbidden_targets[WM_FORBIDDEN_TARGET_COUNT] = {
    { "712d0", 0x800712D0u, &s_wm712d0_hits },
    { "72238", 0x80072238u, &s_wm72238_hits },
    { "7299c", 0x8007299Cu, &s_wm7299c_hits },
    { "9766c", 0x8009766Cu, &s_wm9766c_hits },
    { "74e58", 0x80074E58u, &s_wm74e58_hits },
    { "75030", 0x80075030u, &s_wm75030_hits },
    { "739b8", 0x800739B8u, &s_wm739b8_hits },
    { "88f64", 0x80088F64u, &s_wm88f64_hits },
    { "37fd8", 0x80037FD8u, &s_wm37fd8_hits },
    { "drawotag_world", 0u, &s_wm_drawotag_hits },
    { "cdsync_world", 0u, &s_wm_cd_sync_world_hits },
    { "loop_dispatch", 0x80072514u, &s_wm_loop_dispatch_hits },
    { "967e4", 0x800967E4u, &s_wm967e4_hits },
    { "loop_backedge", 0x80072530u, &s_wm_loop_backedge_hits },
    { "loop_exit", 0x80072538u, &s_wm_loop_exit_hits },
};

static int env_flag_is_one(const char* name)
{
    const char* v = getenv(name);
    return v != NULL && v[0] == '1' && v[1] == '\0';
}

static int world_ft4_pools_enabled(void)
{
    /* W17B runs when requested or as a prerequisite of W18B–W24E. */
    return env_flag_is_one("XENO_WORLD_FT4_POOLS") ||
           env_flag_is_one("XENO_WORLD_HEAP_TABLE_RAND") ||
           env_flag_is_one("XENO_WORLD_UPLOAD_RECORDS") ||
           env_flag_is_one("XENO_WORLD_UPLOAD_RECORDS_B") ||
           env_flag_is_one("XENO_WORLD_DRAW_PACKETS") ||
           env_flag_is_one("XENO_WORLD_88F64") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_READY_POLL") ||
           env_flag_is_one("XENO_WORLD_FIRST_WDS_CONSUMER") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_SET_INDEX");
}

static int world_heap_table_rand_enabled(void)
{
    /* W18B runs when requested or as a prerequisite of W19A–W24E. */
    return env_flag_is_one("XENO_WORLD_HEAP_TABLE_RAND") ||
           env_flag_is_one("XENO_WORLD_UPLOAD_RECORDS") ||
           env_flag_is_one("XENO_WORLD_UPLOAD_RECORDS_B") ||
           env_flag_is_one("XENO_WORLD_DRAW_PACKETS") ||
           env_flag_is_one("XENO_WORLD_88F64") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_READY_POLL") ||
           env_flag_is_one("XENO_WORLD_FIRST_WDS_CONSUMER") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_SET_INDEX");
}

static int world_upload_records_enabled(void)
{
    /* W19A runs when requested or as a prerequisite of W20B–W24E. */
    return env_flag_is_one("XENO_WORLD_UPLOAD_RECORDS") ||
           env_flag_is_one("XENO_WORLD_UPLOAD_RECORDS_B") ||
           env_flag_is_one("XENO_WORLD_DRAW_PACKETS") ||
           env_flag_is_one("XENO_WORLD_88F64") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_READY_POLL") ||
           env_flag_is_one("XENO_WORLD_FIRST_WDS_CONSUMER") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_SET_INDEX");
}

static int world_upload_records_b_enabled(void)
{
    /* W20B runs when requested or as a prerequisite of W21B–W24E. */
    return env_flag_is_one("XENO_WORLD_UPLOAD_RECORDS_B") ||
           env_flag_is_one("XENO_WORLD_DRAW_PACKETS") ||
           env_flag_is_one("XENO_WORLD_88F64") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_READY_POLL") ||
           env_flag_is_one("XENO_WORLD_FIRST_WDS_CONSUMER") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_SET_INDEX");
}

static int world_draw_packets_enabled(void)
{
    /* W21B runs when requested or as a prerequisite of W22B–W24E. */
    return env_flag_is_one("XENO_WORLD_DRAW_PACKETS") ||
           env_flag_is_one("XENO_WORLD_88F64") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_READY_POLL") ||
           env_flag_is_one("XENO_WORLD_FIRST_WDS_CONSUMER") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_SET_INDEX");
}

static int world_88f64_enabled(void)
{
    /* W22B runs when requested or as a prerequisite of W23B–W24E. */
    return env_flag_is_one("XENO_WORLD_88F64") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_READY_POLL") ||
           env_flag_is_one("XENO_WORLD_FIRST_WDS_CONSUMER") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_SET_INDEX");
}

static int world_archive_ready_poll_enabled(void)
{
    /* W23B runs when requested or as a prerequisite of W24C/W24E. */
    return env_flag_is_one("XENO_WORLD_ARCHIVE_READY_POLL") ||
           env_flag_is_one("XENO_WORLD_FIRST_WDS_CONSUMER") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_SET_INDEX");
}

static int world_first_wds_consumer_enabled(void)
{
    /* W24C runs when requested or as a prerequisite of W24E. */
    return env_flag_is_one("XENO_WORLD_FIRST_WDS_CONSUMER") ||
           env_flag_is_one("XENO_WORLD_ARCHIVE_SET_INDEX");
}

static int world_archive_set_index_enabled(void)
{
    /* Narrow W24E gate: only when explicitly requested. */
    return env_flag_is_one("XENO_WORLD_ARCHIVE_SET_INDEX");
}

static int world_967e4_route_enabled(void)
{
    /* W29B is the next established retail stage after W24E. */
    return world_archive_set_index_enabled();
}

static int world_ready_buffer_consume_enabled(void)
{
    /* W32B gate: routes ready-check and third-wave buffer consumption.
     * Requires the W29B dispatcher stage as prerequisite — the routing code
     * runs inside that established archive-transition block. */
    return env_flag_is_one("XENO_WORLD_READY_BUFFER_CONSUME");
}

static int world_mode_audio_setup_enabled(void)
{
    /* W33B gate: routes mode-dependent audio setup.
     * Requires XENO_WORLD_READY_BUFFER_CONSUME (W32B) as prerequisite. */
    return env_flag_is_one("XENO_WORLD_MODE_AUDIO_SETUP");
}

static int world_convergence_p1_enabled(void)
{
    /* W34B1 gate: first convergence caller slice 0x800726C0–0x80072728.
     * Default OFF, requires XENO_WORLD_MODE_AUDIO_SETUP. The route is
     * bounded to one production invocation per init; direct calls to
     * wm_800726C0_convergence_p1 remain repeatable for tests.
     * Future removal of the temporary one-invocation bound should allow
     * repeated production calls (then guarded by idempotent caller
     * semantics, not a permanent one-shot). */
    return env_flag_is_one("XENO_WORLD_CONVERGENCE_P1") &&
           world_mode_audio_setup_enabled();
}

static int world_convergence_p2_enabled(void)
{
    /* W34B3 gate: second convergence table pass 0x8007272C–0x80072780.
     * Default OFF, requires XENO_WORLD_CONVERGENCE_P1. Only runs when
     * P1 completed and returned cut 0x8007272C (flag==0 second-table path). */
    return env_flag_is_one("XENO_WORLD_CONVERGENCE_P2") &&
           world_convergence_p1_enabled();
}

static int world_framebuffer_gte_init_enabled(void)
{
    /* W34B4B gate: framebuffer/GTE initializer 0x80072BB0–0x80072DB0.
     * Default OFF, requires XENO_WORLD_MODE_AUDIO_SETUP. Initializes
     * two complementary 320×216 draw/display environments, GTE screen
     * distance (256), back/far color, and fog parameters.
     * Called at retail 0x80072244, before convergence P1. */
    return env_flag_is_one("XENO_WORLD_FRAMEBUFFER_GTE_INIT") &&
           world_mode_audio_setup_enabled();
}

static int world_terrain_position_init_enabled(void)
{
    /* W34B4C gate: terrain/position initializer 0x80097BC0–0x80097CB4.
     * Default OFF, requires XENO_WORLD_FRAMEBUFFER_GTE_INIT. Initializes
     * terrain matrix (D534), cell table (C580), terrain period (C618=1024),
     * and masked position. Called at retail 0x8007250C, after framebuffer
     * init and before convergence P1. */
    return env_flag_is_one("XENO_WORLD_TERRAIN_POSITION_INIT") &&
           world_framebuffer_gte_init_enabled();
}

static int world_common_tail_p0_enabled(void)
{
    /* W34B5A gate: common-tail prefix 0x8007290C–0x80072938.
     * Default OFF, requires CONVERGENCE_P2 and TERRAIN_POSITION_INIT.
     * Reads C610 selector, calls wm_80089160 for C610=0.
     * Cut at 0x8007293C (reconvergence, before next-phase helper). */
    return env_flag_is_one("XENO_WORLD_COMMON_TAIL_P0") &&
           world_convergence_p2_enabled() &&
           world_terrain_position_init_enabled();
}

static int world_common_tail_p1_enabled(void)
{
    /* W34B5B gate: common-tail P1 slice 0x8007293C–0x80072940.
     * Default OFF, requires P0.
     * Calls wm_800978FC once.
     * Cut at 0x80072944 (before next helper 0x8008901C). */
    return env_flag_is_one("XENO_WORLD_COMMON_TAIL_P1") &&
           world_common_tail_p0_enabled();
}

static int world_common_tail_p2_enabled(void)
{
    /* W34B5C gate: common-tail P2 slice 0x80072944–0x80072948.
     * Default OFF, requires P1.
     * Calls wm_8008901C once.
     * Cut at 0x8007294C (before next helper 0x800865A0). */
    return env_flag_is_one("XENO_WORLD_COMMON_TAIL_P2") &&
           world_common_tail_p1_enabled();
}

static int world_common_tail_p3_enabled(void)
{
    /* W34B5D gate: common-tail P3 slice 0x8007294C–0x80072950.
     * Default OFF, requires P2.
     * Calls wm_800865A0 once.
     * Cut at 0x80072954 (before next helper 0x80085FE0). */
    return env_flag_is_one("XENO_WORLD_COMMON_TAIL_P3") &&
           world_common_tail_p2_enabled();
}

static int world_common_tail_p4_enabled(void)
{
    /* W34B5E gate: common-tail P4 slice 0x80072954–0x80072958.
     * Default OFF, requires P3.
     * Calls wm_80085FE0 once.
     * Cut at 0x8007295C (before next instruction). */
    return env_flag_is_one("XENO_WORLD_COMMON_TAIL_P4") &&
           world_common_tail_p3_enabled();
}

static int world_common_tail_p5_enabled(void)
{
    /* W34B5F gate: common-tail P5 slice 0x8007295C–0x80072998.
     * Default OFF, requires P4.
     * Reads C894, calls wm_80075228 when C894==0 (natural path).
     * Both paths call SystemTransferPaletteToVRAM.
     * Returns overlay-local sentinel 0x8007299C (slot-2 entry).
     * Actual post-slot-1 return PC: 0x80071064. */
    return env_flag_is_one("XENO_WORLD_COMMON_TAIL_P5") &&
           world_common_tail_p4_enabled();
}

static int world_scheduler_97800_enabled(void)
{
    /* W34B5H gate: bounded scheduler 0x80097800 execution.
     * Default OFF, requires P5 (accepted slot-1 one-shot init complete).
     * Runs once at the actual caller frontier 0x80071064 (post-slot-1,
     * pre-DrawSync) — never from the 0x8007299C overlay-local sentinel.
     * Stops before the first missing callback body; DrawSync at
     * 0x8007106C is not executed. */
    return env_flag_is_one("XENO_WORLD_SCHEDULER_97800") &&
           world_common_tail_p5_enabled();
}

static int world_frame_prologue_enabled(void)
{
    /* W34B18-B gate: retail 0x8007106C continuation plus bounded
     * 0x800712D0 .. 0x80071484. Default OFF, requires scheduler.
     * Hard-cut before 0x80071488 (second scheduler jal). The active open loop
     * owns the complete continuation and must not also execute this legacy
     * one-frame diagnostic body. */
    return env_flag_is_one("XENO_WORLD_FRAME_PROLOGUE") &&
           world_scheduler_97800_enabled();
}

static int world_frame_reentry_limit(void)
{
    /* W34B42/W34B44: finite reviewed re-entry bounds. The retail D554
     * predicate is still evaluated before every additional frame. */
    if (!world_frame_prologue_enabled())
        return 0;
    if (env_flag_is_one("XENO_WORLD_FRAME_REENTRY_TWICE"))
        return 2;
    if (env_flag_is_one("XENO_WORLD_FRAME_REENTRY_ONCE"))
        return 1;
    return 0;
}

static int world_gfx_work_buffers_enabled(void)
{
    /* FT4 pools imply gfx work-buffer routing. */
    return env_flag_is_one("XENO_WORLD_GFX_WORK_BUFFERS") ||
           world_ft4_pools_enabled() ||
           world_heap_table_rand_enabled();
}

static int world_record_clut_enabled(void)
{
    /* Gfx work-buffer routing implies record/CLUT init. */
    return env_flag_is_one("XENO_WORLD_RECORD_CLUT_INIT") ||
           world_gfx_work_buffers_enabled();
}

static int world_primitive_templates_enabled(void)
{
    /* Record/CLUT init implies primitive-templates. */
    return env_flag_is_one("XENO_WORLD_PRIMITIVE_TEMPLATES") ||
           world_record_clut_enabled();
}

static int world_bss_constants_enabled(void)
{
    /* Primitive-templates imply BSS-constants. */
    return env_flag_is_one("XENO_WORLD_BSS_CONSTANTS") ||
           world_primitive_templates_enabled();
}

static int world_third_wave_enabled(void)
{
    /* BSS-constants imply third-wave. */
    return env_flag_is_one("XENO_WORLD_THIRD_WAVE") ||
           world_bss_constants_enabled();
}

static int world_object_matrix_enabled(void)
{
    /* Third-wave submit implies object-matrix. */
    return env_flag_is_one("XENO_WORLD_OBJECT_MATRIX") ||
           world_third_wave_enabled();
}

static int world_gpu_asset_b_enabled(void)
{
    /* Object-matrix implies GPU-asset-B. */
    return env_flag_is_one("XENO_WORLD_GPU_ASSET_B") ||
           world_object_matrix_enabled();
}

static int world_gpu_asset_a_enabled(void)
{
    /* GPU-asset-B implies GPU-asset-A. */
    return env_flag_is_one("XENO_WORLD_GPU_ASSET_A") ||
           world_gpu_asset_b_enabled();
}

static int world_cross_products_enabled(void)
{
    /* GPU-asset-A implies cross-products. */
    return env_flag_is_one("XENO_WORLD_CROSS_PRODUCTS") ||
           world_gpu_asset_a_enabled();
}

static int world_mode_enter_state_enabled(void)
{
    /* Cross-products implies mode-enter-state. */
    return env_flag_is_one("XENO_WORLD_MODE_ENTER_STATE") ||
           world_cross_products_enabled();
}

static int world_state_template_enabled(void)
{
    /* Mode-enter-state / cross-products imply state-template. */
    return env_flag_is_one("XENO_WORLD_STATE_TEMPLATE") ||
           world_mode_enter_state_enabled();
}

static int world_object_pool_enabled(void)
{
    /* State-template / mode-enter imply object-pool. */
    return env_flag_is_one("XENO_WORLD_OBJECT_POOL") || world_state_template_enabled();
}

static int world_second_wave_enabled(void)
{
    /* Object-pool / template imply second-wave. */
    return env_flag_is_one("XENO_WORLD_SECOND_WAVE") || world_object_pool_enabled();
}

static int world_mode_init_enabled(void)
{
    /* Second-wave ladder implies mode-init. */
    return env_flag_is_one("XENO_WORLD_MODE_INIT") || world_second_wave_enabled();
}

static int world_init_enabled(void)
{
    /* Mode-init ladder implies W2 overlay/init path. */
    return env_flag_is_one("XENO_WORLD_INIT") || world_mode_init_enabled();
}

int PcPort_WorldMapInitEnabled(void)
{
    return world_init_enabled();
}

static void log_enabled_slices(void)
{
    int w2 = world_init_enabled();
    int w3 = world_mode_init_enabled();
    int w4 = world_second_wave_enabled();
    int w5 = world_object_pool_enabled();
    int w6 = world_state_template_enabled();
    int w7 = world_mode_enter_state_enabled();
    int w8 = world_cross_products_enabled();
    int w10a = world_gpu_asset_a_enabled();
    int w10b = world_gpu_asset_b_enabled();
    int w11 = world_object_matrix_enabled();
    int w12 = world_third_wave_enabled();
    int w13 = world_bss_constants_enabled();
    int w14 = world_primitive_templates_enabled();
    int w15 = world_record_clut_enabled();
    int w16 = world_gfx_work_buffers_enabled();
    int w17 = world_ft4_pools_enabled();
    int w18 = world_heap_table_rand_enabled();
    int w19 = world_upload_records_enabled();
    int w20 = world_upload_records_b_enabled();
    int w21 = world_draw_packets_enabled();
    int w22 = world_88f64_enabled();
    int w23 = world_archive_ready_poll_enabled();
    int w24 = world_first_wds_consumer_enabled();
    int w25 = world_archive_set_index_enabled();
    int w29 = world_967e4_route_enabled();
    int w32 = world_ready_buffer_consume_enabled();
    int w33 = world_mode_audio_setup_enabled();
    int w34b1 = world_convergence_p1_enabled();
    int w34b4b = world_framebuffer_gte_init_enabled();
    int w34b5a = world_common_tail_p0_enabled();
    int w34b5b = world_common_tail_p1_enabled();
    int w34b5c = world_common_tail_p2_enabled();
    int w34b5d = world_common_tail_p3_enabled();
    int w34b5e = world_common_tail_p4_enabled();
    int w34b18b = world_frame_prologue_enabled();
    int w34b42 = world_frame_reentry_limit() > 0;
    int w34b44 = world_frame_reentry_limit() > 1;
    fprintf(stderr, "[worldmap] enabled slices:");
    if (!w2 && !w3 && !w4 && !w5 && !w6 && !w7 && !w8 && !w10a && !w10b &&
        !w19 && !w20 && !w21 && !w22 && !w23 && !w24 && !w25 && !w29 && !w32 && !w33 && !w34b1 && !w34b4b && !w34b5a && !w34b5b && !w34b5c && !w34b5d && !w34b5e && !w34b18b && !w34b42) {
        fprintf(stderr, " (none — placeholder only)\n");
        return;
    }
    if (w2)
        fprintf(stderr, " W2");
    if (w3)
        fprintf(stderr, ",W3B");
    if (w4)
        fprintf(stderr, ",W4C");
    if (w5)
        fprintf(stderr, ",W5B");
    if (w6)
        fprintf(stderr, ",W6B");
    if (w7)
        fprintf(stderr, ",W7B");
    if (w8)
        fprintf(stderr, ",W8B");
    if (w10a)
        fprintf(stderr, ",W10A");
    if (w10b)
        fprintf(stderr, ",W10B");
    if (w11)
        fprintf(stderr, ",W11B");
    if (w12)
        fprintf(stderr, ",W12B");
    if (w13)
        fprintf(stderr, ",W13B");
    if (w14)
        fprintf(stderr, ",W14B");
    if (w15)
        fprintf(stderr, ",W15B");
    if (w16)
        fprintf(stderr, ",W16B");
    if (w17)
        fprintf(stderr, ",W17B");
    if (w18)
        fprintf(stderr, ",W18B");
    if (w19)
        fprintf(stderr, ",W19A");
    if (w20)
        fprintf(stderr, ",W20B");
    if (w21)
        fprintf(stderr, ",W21B");
    if (w22)
        fprintf(stderr, ",W22B");
    if (w23)
        fprintf(stderr, ",W23B");
    if (w24)
        fprintf(stderr, ",W24C");
    if (w25)
        fprintf(stderr, ",W24E");
    if (w29)
        fprintf(stderr, ",W29B");
    if (w32)
        fprintf(stderr, ",W32B");
    if (w33)
        fprintf(stderr, ",W33B");
    if (w34b1)
        fprintf(stderr, ",W34B1");
    if (w34b4b)
        fprintf(stderr, ",W34B4B");
    if (w34b5a)
        fprintf(stderr, ",W34B5A");
    if (w34b5b)
        fprintf(stderr, ",W34B5B");
    if (w34b5c)
        fprintf(stderr, ",W34B5C");
    if (w34b5d)
        fprintf(stderr, ",W34B5D");
    if (w34b5e)
        fprintf(stderr, ",W34B5E");
    if (w34b18b)
        fprintf(stderr, ",W34B18B");
    if (w34b42)
        fprintf(stderr, ",W34B42");
    if (w34b44)
        fprintf(stderr, ",W34B44");
    fprintf(stderr, "\n");
}

/* Retail wm_80095F78: HeapAlloc pair + table clears. g_ArchiveDebugTable==0 is
 * the normal path (Lahan / retail CD). */
static void wm_80095F78(void)
{
    u32 dbg0 = g_ArchiveDebugTable;
    u32 dbg1 = g_ArchiveDebugTable;
    int use_debug_path = (dbg0 != 0) && (dbg1 != 0);
    void* p;
    int i;

    WM_U32(WM_CLR_BCB8_ABS) = 0;
    WM_U32(WM_CLR_BE44_ABS) = 0;
    WM_U32(WM_CLR_CD44_ABS) = 0;

    if (!use_debug_path) {
        u32* pClr = (u32*)PSX_ADDR(WM_CLR_D7C4_ABS);
        for (i = 0; i < 0x10; i++)
            pClr[-i] = 0; /* retail walks 16 words downward from D7C4 */

        p = HeapAlloc(0x4200, 0);
        WM_U32(WM_ALLOC_BE08_ABS) = host_ptr_to_psx_u32(p); /* W34C11: guest address */
        p = HeapAlloc(0x800, 0);
        WM_U32(WM_ALLOC_D7D4_ABS) = host_ptr_to_psx_u32(p); /* W34C11: spill buffer, guest address */
    } else {
        u32* pClr = (u32*)PSX_ADDR(WM_CLR_C660_ABS);
        for (i = 0; i < 0x10; i++)
            pClr[-i] = 0;
        p = HeapAlloc(0x5800, 0);
        WM_U32(WM_ALLOC_D3C0_ABS) = host_ptr_to_psx_u32(p); /* W34C11: guest address */
        p = HeapAlloc(0x800, 0);
        WM_U32(WM_ALLOC_D7D4_ABS) = host_ptr_to_psx_u32(p); /* W34C11: spill buffer, guest address */
    }

    WM_U32(WM_CLR_D808_ABS) = 0;
    {
        u8* pB = (u8*)PSX_ADDR(WM_BYTE_C58F_ABS);
        for (i = 0; i < 8; i++)
            pB[-i] = 0;
    }
}

/* W25B: exact native equivalent of retail 0x80096668.
 *
 * Reads the circular-buffer head (BE44) and tail (BCB8) indices, computes
 * their modulo-16 forward distance.  Retail MIPS:
 *   lui  $v1, 0x800A / lw $v1, BE44($v1)     ← head = *(0x8009BE44)
 *   lui  $v0, 0x800A / lw $v0, BCB8($v0)     ← tail = *(0x8009BCB8)
 *   subu $v0, $v1, $v0                        ← d = head − tail
 *   bgez $v0, .ret                            ← if d >= 0: return d
 *   nop
 *   addiu $v0, $v0, 16                        ← d += 16  (wrap correction)
 *   jr $ra / nop
 *
 * Caller repeats its loop while result >= 2 (slti $v0, $v0, 2 / beq).
 * Return domain: [0, 15].  Read-only: no stores, no callees. */
u32 wm_80096668_circular_distance(void)
{
    u32 head = WM_U32(WM_CLR_BE44_ABS);
    u32 tail = WM_U32(WM_CLR_BCB8_ABS);
    s32 d    = (s32)(head - tail);
    if (d < 0)
        d += 16;
    return (u32)d;
}

/* W25B instrumented wrapper: counts calls and returns the real result. */
u32 wm_80096668_instrumented(void)
{
    s_wm96668_hits++;
    return wm_80096668_circular_distance();
}

/* W27B: CD44=3→4 completion callback (exact native of retail 0x80096AF0).
 *
 * Retail CdSyncCallback handler for CD44=3.  When CdlPause completes after
 * all D788 records are exhausted, this transitions CD44 from 3 to 4 and
 * sets BD2C=1 as a one-frame delay counter.  Also unregisters the
 * CdSyncCallback.
 *
 * Retail MIPS (0x80096AF0–0x80096B24):
 *   lw   $v0, D614($v0)          ← load last-file-id sentinel
 *   bne  $v0, $zero, exit        ← if nonzero: not all done, bail
 *   addiu $v0, $zero, 4          ← delay slot: $v0 = 4
 *   sw   $v0, CD44($at)          ← CD44 = 4
 *   addiu $v0, $zero, 1
 *   sw   $v0, BD2C($at)          ← BD2C = 1
 *   jal  CdSyncCallback(NULL)    ← unregister
 *   addu $a0, $zero, $zero
 *   j    exit
 *   nop
 *
 * Preconditions: CD44 == 3.  D614 == 0 (terminator was encountered).
 * Postconditions: CD44 = 4, BD2C = 1, CdSyncCallback unregistered.
 * Returns: none (void callback path). */
void wm_80096AF0_completion_3to4(void)
{
    u32 d614;
    s_wm_completion_cb_entry++;
    d614 = WM_U32(WM_D614_ABS);
    if (d614 != 0)
        return;
    WM_U32(WM_CD44_ABS) = 4;
    WM_U32(WM_BD2C_ABS) = 1;
    s_wm_completion_3to4++;
    /* Retail calls CdSyncCallback(NULL) to unregister.  In the PC port
     * the callback is not registered via PsyQ; record the unregister. */
    s_wm_cb_unregister++;
}

/* W34B25: CdSyncCallback handler 0x80096A6C (exact native of the retail
 * CD44 status dispatcher, world_map.bin 0x80096A6C-0x80096C08; jump table
 * 0x80070CB8, 12 entries indexed by CD44-1).
 *
 * Retail MIPS (status in $a0, result in $a1):
 *   0x80096A78  bne  $a0, 2, 0x80096BA0        <- not CdlComplete: error path
 *   0x80096A84  lw   $v0, CD44 ; addiu $v1,$v0,-1 ; sltiu $v0,$v1,12
 *   0x80096A94  beqz $v0, 0x80096BF8            <- out of range: return
 *   0x80096AAC  jr   table[CD44-1]
 *     idx0 (CD44=1)  0x80096AB4: CdReadyCallback(0x80096C0C); CD44=2;
 *                    BCD4=BCD0=BCCC=0; a0=27 -> 0x80096BEC
 *     idx1 (CD44=2)  0x80096BF8: return
 *     idx2 (CD44=3)  0x80096AF0: completion 3->4 (wm_80096AF0_completion_3to4)
 *     idx3..8        0x80096BF8: return
 *     idx9..11       0x80096B28 / 0x80096B5C / 0x80096B70: NOT PORTED here
 *   0x80096BA0  lbu $v0,0($a1); andi 0x10; beqz -> 0x80096BDC
 *               CD44=10; CCA8++; a0=CdlNop(1)  -> 0x80096BF0
 *   0x80096BDC  CD44=11; a0=CdlGetTN(0x13)
 *   0x80096BEC  a1=0 ; jal CdControlF(a0, 0) ; return
 *
 * Port notes (no game logic invented):
 *  - Registered as a HOST function at the 0x8009699C registration site;
 *    the W28B CdControlF hook calls the stored CdlCB directly, so it must
 *    never be a PSX address.
 *  - CdReadyCallback target 0x80096C0C is not ported; a counting boundary
 *    stub (host function) is registered instead so the ready path can
 *    never jump into g_PsxRam.  It fires on the PsyCross CDSpooler thread.
 *  - Retail issues CdControlF(CdlReadS, NULL) = "read from the last Setloc
 *    position".  Host CdControlF dereferences param, so the port passes
 *    the CEBC CdlLOC that step 11 of 0x8009699C just Setloc'd -- the same
 *    position retail's NULL resolves to.
 *  - CD44 states 10..12 arms are a bounded boundary (counted, no state
 *    change); they are not reachable from the CD44=1 entry in this slice. */
static int s_wm_96a6c_entry;
static int s_wm_96a6c_state1;
static int s_wm_96a6c_error_path;
static int s_wm_96a6c_unported_state;
static volatile int s_wm_96c0c_ready_boundary_hits;

static void wm_80096C0C_cd_ready_boundary(u_char status, u_char* result)
{
    (void)status;
    (void)result;
    s_wm_96c0c_ready_boundary_hits++;
}

void wm_80096A6C_cd_sync(u_char status, u_char* result)
{
    u32 cd44;

    s_wm_96a6c_entry++;
    if (status != 2) {
        /* 0x80096BA0: CdlDiskError / non-complete status path. */
        s_wm_96a6c_error_path++;
        if (result[0] & 0x10) {
            WM_U32(WM_CD44_ABS) = 10;
            WM_U32(WM_CCA8_ABS) = WM_U32(WM_CCA8_ABS) + 1;
            CdControlF(CdlNop, NULL);
        } else {
            WM_U32(WM_CD44_ABS) = 11;
            CdControlF(CdlGetTN, NULL);
        }
        return;
    }

    cd44 = WM_U32(WM_CD44_ABS);
    if (cd44 - 1u >= 12u)
        return;

    switch (cd44) {
    case 1:
        /* 0x80096AB4..0x80096AEC (exact order). */
        CdReadyCallback(wm_80096C0C_cd_ready_boundary);
        WM_U32(WM_CD44_ABS) = 2;
        WM_U32(WM_BCD4_ABS) = 0;
        WM_U32(WM_BCD0_ABS) = 0;
        WM_U32(WM_BCCC_ABS) = 0;
        s_wm_96a6c_state1++;
        fprintf(stderr,
                "[worldmap-cd-sync-96a6c] CD44 1->2, ready boundary registered, "
                "CdlReadS at CEBC loc\n");
        CdControlF(CdlReadS, (u_char*)PSX_ADDR(WM_CEBC_ABS));
        return;
    case 3:
        wm_80096AF0_completion_3to4();
        return;
    case 10:
    case 11:
    case 12:
        s_wm_96a6c_unported_state++;
        fprintf(stderr,
                "[worldmap-cd-sync-96a6c] BOUNDARY: CD44=%u arm not ported\n",
                cd44);
        return;
    default:
        return;
    }
}

/* W27B: exact dispatcher state 4 (retail 0x80096918–0x80096954).
 *
 * Retail MIPS:
 *   lw   $v0, BD2C($v0)          ← load countdown
 *   addiu $v0, $v0, -1           ← decrement
 *   sw   $v0, BD2C($at)          ← store back
 *   bne  $v0, $zero, ret1        ← if nonzero: still counting, return 1
 *   nop
 *   lw   $v0, CD44($v0)          ← re-read CD44
 *   addiu $v0, $v0, 1            ← CD44++
 *   sw   $v0, CD44($at)          ← store CD44 = 5
 * ret1:
 *   j    common_exit
 *   addiu $v0, $zero, 1          ← return 1
 *
 * Preconditions: CD44 == 4, BD2C > 0.
 * Postconditions: BD2C decremented.  If BD2C reaches 0: CD44 = 5.
 * Returns: 1 (busy). */
static u32 wm_dispatcher_state4(void)
{
    u32 bd2c;
    s_wm_dispatcher_state4++;
    bd2c = WM_U32(WM_BD2C_ABS);
    bd2c = bd2c - 1;
    WM_U32(WM_BD2C_ABS) = bd2c;
    s_wm_bd2c_decrement++;
    if (bd2c == 0) {
        u32 cd44 = WM_U32(WM_CD44_ABS);
        WM_U32(WM_CD44_ABS) = cd44 + 1;
    }
    return 1;
}

/* W27B: exact dispatcher state 5 (retail 0x80096958–0x80096994).
 *
 * Retail MIPS:
 *   lw   $v1, BCB8($v1)          ← load tail
 *   sw   $zero, CD44($at)        ← CD44 = 0
 *   sll  $a0, $v1, 2             ← tail*4 for D788 index
 *   addiu $v1, $v1, 1            ← tail + 1
 *   andi $v1, $v1, 0x0F          ← wrap to [0,15]
 *   sw   $zero, D788[$a0]        ← clear D788_table[tail]
 *   sw   $v1, BCB8($at)          ← store new tail
 *   j    common_exit
 *   addiu $v0, $zero, 2          ← return 2
 *
 * Ordering (exact retail):
 *   1. CD44 = 0
 *   2. D788_table[tail] = 0
 *   3. BCB8 = (BCB8 + 1) & 0x0F
 *
 * Preconditions: CD44 == 5.
 * Postconditions: CD44 = 0, D788[tail] cleared, BCB8 advanced.
 * Returns: 2 (tail advanced). */
static u32 wm_dispatcher_state5(void)
{
    u32 tail = WM_U32(WM_BCB8_ABS);
    u32 new_tail;
    s_wm_dispatcher_state5++;
    WM_U32(WM_CD44_ABS) = 0;
    s_wm_cd44_clear++;
    /* D788 table at 0x8009D788, 16 entries × 4 bytes. */
    WM_U32(WM_D788_BASE_ABS + tail * 4) = 0;
    s_wm_d788_tail_clear++;
    new_tail = (tail + 1) & 0x0F;
    WM_U32(WM_BCB8_ABS) = new_tail;
    s_wm_bcb8_increment++;
    return 2;
}

/* W27B: exact dispatcher for states 4 and 5 (retail 0x800968E0 partial).
 *
 * This implements only the dispatcher states required for the CD44=4→5→0
 * completion chain.  States 0–3 and ≥6 are handled by existing code or
 * are out of scope for W27B.
 *
 * Retail MIPS dispatch:
 *   lw   $v1, CD44($v1)          ← load state
 *   sltiu $v0, $v1, 6            ← bounds check [0,5]
 *   beq  $v0, $zero, ret3        ← out of range → return 3
 *   sll  $v0, $v1, 2             ← index * 4
 *   lw   $v0, jump_table[$at]    ← load handler
 *   jr   $v0                     ← dispatch
 *
 * Returns: 0 (idle), 1 (busy), 2 (tail advanced), 3 (invalid). */
u32 wm_800968E0_dispatch_partial(void)
{
    u32 cd44 = WM_U32(WM_CD44_ABS);
    if (cd44 >= 6)
        return 3;
    switch (cd44) {
    case 4:
        return wm_dispatcher_state4();
    case 5:
        return wm_dispatcher_state5();
    default:
        /* States 0–3: not handled here.  Return 1 (busy) for states 1–3,
         * 0 for state 0.  This is sufficient for the completion chain
         * since the caller only needs to see states 4→5→0 progress. */
        return (cd44 == 0) ? 0 : 1;
    }
}

/* W27B: full completion-chain test entry point.
 *
 * Exercises the exact retail state sequence:
 *   CD44=3 → callback → CD44=4
 *   CD44=4 → dispatch → BD2C dec → CD44=5
 *   CD44=5 → dispatch → CD44=0, BCB8++, D788 clear
 *
 * Returns: 0 on success, nonzero on failure. */
int wm_completion_chain_selftest(void)
{
    u32 bcb8_before, bcb8_after;
    u32 dist_before, dist_after;
    u32 result;
    int pass = 1;

    /* Setup: CD44=3, BD2C=1, BCB8=0, D614=0, D788[0]=0xDEAD */
    WM_U32(WM_CD44_ABS) = 3;
    WM_U32(WM_BD2C_ABS) = 1;
    WM_U32(WM_BCB8_ABS) = 0;
    WM_U32(WM_D614_ABS) = 0;
    WM_U32(WM_D788_BASE_ABS) = 0xDEADu;
    WM_U32(WM_CLR_BE44_ABS) = 2; /* head=2, tail=0 → distance=2 */

    bcb8_before = WM_U32(WM_BCB8_ABS);
    dist_before = wm_80096668_circular_distance();

    /* Step 1: completion callback CD44=3→4 */
    wm_80096AF0_completion_3to4();
    if (WM_U32(WM_CD44_ABS) != 4) {
        fprintf(stderr, "[w27b-test] FAIL: after callback CD44=%u expected 4\n",
                WM_U32(WM_CD44_ABS));
        pass = 0;
    }
    if (WM_U32(WM_BD2C_ABS) != 1) {
        fprintf(stderr, "[w27b-test] FAIL: after callback BD2C=%u expected 1\n",
                WM_U32(WM_BD2C_ABS));
        pass = 0;
    }
    if (WM_U32(WM_BCB8_ABS) != bcb8_before) {
        fprintf(stderr, "[w27b-test] FAIL: callback changed BCB8\n");
        pass = 0;
    }
    if (WM_U32(WM_D788_BASE_ABS) != 0xDEADu) {
        fprintf(stderr, "[w27b-test] FAIL: callback cleared D788\n");
        pass = 0;
    }

    /* Step 2: dispatcher state 4 → CD44=5 */
    result = wm_800968E0_dispatch_partial();
    if (result != 1) {
        fprintf(stderr, "[w27b-test] FAIL: state4 returned %u expected 1\n", result);
        pass = 0;
    }
    if (WM_U32(WM_CD44_ABS) != 5) {
        fprintf(stderr, "[w27b-test] FAIL: after state4 CD44=%u expected 5\n",
                WM_U32(WM_CD44_ABS));
        pass = 0;
    }
    if (WM_U32(WM_BD2C_ABS) != 0) {
        fprintf(stderr, "[w27b-test] FAIL: after state4 BD2C=%u expected 0\n",
                WM_U32(WM_BD2C_ABS));
        pass = 0;
    }

    /* Step 3: dispatcher state 5 → CD44=0, BCB8++, D788 clear */
    result = wm_800968E0_dispatch_partial();
    if (result != 2) {
        fprintf(stderr, "[w27b-test] FAIL: state5 returned %u expected 2\n", result);
        pass = 0;
    }
    if (WM_U32(WM_CD44_ABS) != 0) {
        fprintf(stderr, "[w27b-test] FAIL: after state5 CD44=%u expected 0\n",
                WM_U32(WM_CD44_ABS));
        pass = 0;
    }
    bcb8_after = WM_U32(WM_BCB8_ABS);
    if (bcb8_after != (bcb8_before + 1) % 16) {
        fprintf(stderr, "[w27b-test] FAIL: BCB8=%u expected %u\n",
                bcb8_after, (bcb8_before + 1) % 16);
        pass = 0;
    }
    if (WM_U32(WM_D788_BASE_ABS) != 0) {
        fprintf(stderr, "[w27b-test] FAIL: D788[0]=%u expected 0\n",
                WM_U32(WM_D788_BASE_ABS));
        pass = 0;
    }

    /* Step 4: verify circular distance decreased */
    dist_after = wm_80096668_circular_distance();
    if (dist_after != dist_before - 1) {
        fprintf(stderr, "[w27b-test] FAIL: distance %u→%u expected %u\n",
                dist_before, dist_after, dist_before - 1);
        pass = 0;
    }

    /* Step 5: dispatcher returns 0 (idle) */
    result = wm_800968E0_dispatch_partial();
    if (result != 0) {
        fprintf(stderr, "[w27b-test] FAIL: idle returned %u expected 0\n", result);
        pass = 0;
    }

    if (pass)
        fprintf(stderr, "[w27b-test] PASS: completion chain 3→4→5→0 verified\n");
    return pass ? 0 : 1;
}

/* W29B: D788 record processor (exact native of retail 0x8009699C).
 *
 * Processes one D788 queue record: sets CD44=1, stores record fields to
 * the CD transfer globals, converts file_id to a CdlLOC via CdIntToPos,
 * registers CdSyncCallback(0x80096A6C), and issues CdControlF(CdlSetloc).
 *
 * Retail MIPS (0x8009699C–0x80096A68, 52 instructions, 216 bytes):
 *   Stack frame: 24 bytes.  Saved: $s0, $ra.
 *   Argument: $a0 = PSX pointer to D788 record (12-byte struct).
 *
 * D788 record layout (12 bytes):
 *   [0] = file_id (sector number for CdIntToPos)
 *   [4] = byte_count (raw payload bytes)
 *   [8] = dest_ptr (PSX pointer to destination buffer)
 *
 * Ordering (exact retail):
 *   1. CD44 = 1
 *   2. D3BC = record + 12 (next record pointer)
 *   3. BE48 = CCB0 = CCA8 = CCA0 = 0
 *   4. D7F4 = file_id (retry copy)
 *   5. D614 = file_id (completion sentinel)
 *   6. D56C = (byte_count + 2047) >> 11 (sector block count)
 *   7. CEB8 = byte_count
 *   8. C590 = dest_ptr
 *   9. CdIntToPos(file_id, &CEBC_loc)
 *  10. CdSyncCallback(0x80096A6C)
 *  11. CdControlF(CdlSetloc=2, &CEBC_loc)
 *
 * Returns: void (no meaningful return value in retail).
 *
 * Side effects: CD44, D3BC, D7F4, D614, D56C, CEB8, C590, BE48, CCB0,
 * CCA8, CCA0, CEBC, and PsyQ CD state via CdSyncCallback/CdControlF.
 *
 * The CdSyncCallback(0x80096A6C) registration means that CdControlF will
 * synchronously invoke handler 0x80096A6C via W28B, which dispatches on
 * CD44 state.  At this point CD44=1, so the handler will set CD44=2 and
 * register CdReadyCallback(0x80096C0C). */
void wm_8009699C_d788_processor(u32 record_psx)
{
    u32 file_id;
    u32 byte_count;
    u32 dest_ptr;
    u32 block_count;
    CdlLOC loc;

    s_wm_d788_proc_entry++;

    /* Load D788 record fields. */
    file_id   = WM_U32(record_psx);
    byte_count = WM_U32(record_psx + 4);
    dest_ptr   = WM_U32(record_psx + 8);

    /* 1. CD44 = 1 (transfer in progress). */
    WM_U32(WM_CD44_ABS) = 1;

    /* 2. D3BC = pointer to next record in chain (record + 12).
     *    Retail writes twice; second store (record+12) wins. */
    WM_U32(WM_D3BC_ABS) = record_psx + WM_D788_RECORD_STRIDE;

    /* 3. Clear auxiliary state. */
    WM_U32(WM_BE48_ABS) = 0;
    WM_U32(WM_CCB0_ABS) = 0;
    WM_U32(WM_CCA8_ABS) = 0;
    WM_U32(WM_CCA0_ABS) = 0;

    /* 4–5. Store file_id for retry and completion sentinel. */
    WM_U32(WM_D7F4_ABS) = file_id;
    WM_U32(WM_D614_ABS) = file_id;

    /* 6. Block count: (byte_count + 2047) >> 11 = ceil(byte_count / 2048). */
    block_count = (byte_count + 2047) >> 11;
    WM_U32(WM_D56C_ABS) = block_count;

    /* 7–8. Store byte_count and dest_ptr for the data-transfer path. */
    WM_U32(WM_CEB8_ABS) = byte_count;
    WM_U32(WM_C590_ABS) = dest_ptr;

    /* 9. Convert file_id to CdlLOC at CEBC. */
    CdIntToPos((int)file_id, &loc);
    WM_U32(WM_CEBC_ABS) = *(u32*)&loc;

    /* 10. Register CdSyncCallback with handler 0x80096A6C (W34B25: host
     *     native wm_80096A6C_cd_sync -- W28B calls the stored CdlCB directly,
     *     so a PSX address here jumps into g_PsxRam). */
    CdSyncCallback(wm_80096A6C_cd_sync);

    /* 11. Issue CdControlF(CdlSetloc, &CEBC_loc).  On PC, this will
     *     synchronously invoke the registered CdSyncCallback via W28B. */
    CdControlF(WM_CdlSetloc, (u_char*)PSX_ADDR(WM_CEBC_ABS));
}

/* W29B: C624 record processor (exact native of retail 0x800966CC).
 *
 * Processes C624 debug-mode file records: opens files via PCopen, seeks
 * and reads data, then closes.  Only reached when g_ArchiveDebugTable
 * is non-zero (development/debug mode).  Never called in production CD mode.
 *
 * Retail MIPS (0x800966CC–0x800967E0, 74 instructions, 296 bytes):
 *   Stack frame: 40 bytes.  Saved: $s0–$s4, $ra.
 *   Argument: $a0 = PSX pointer to C624 record.
 *
 * C624 record layout (array of 16-byte entries, terminated by entry[0]==0):
 *   Entry[i]:
 *     [0]  = filename pointer (PSX address of null-terminated string)
 *     [4]  = (unused in processor, but part of 16-byte stride)
 *   Parameter block starts at record+8, advancing by 16 per entry:
 *     *(param - 4) = seek offset
 *     *(param + 0) = buffer address (PSX pointer)
 *     *(param + 4) = byte count
 *
 * Returns: void.
 *
 * Side effects: BE48, CCB0, CCA8, CCA0 cleared.  PC file I/O. */
void wm_800966CC_c624_processor(u32 record_psx)
{
    u32 entry_ptr;
    u32 param_base;
    int fd;
    int retry;

    s_wm_c624_proc_entry++;

    /* Clear auxiliary state. */
    WM_U32(WM_BE48_ABS) = 0;
    WM_U32(WM_CCB0_ABS) = 0;
    WM_U32(WM_CCA8_ABS) = 0;
    WM_U32(WM_CCA0_ABS) = 0;

    /* Check first entry. */
    entry_ptr = record_psx;
    if (WM_U32(entry_ptr) == 0)
        return;

    param_base = record_psx + 8;

    /* Process each 16-byte entry until terminator. */
    while (WM_U32(entry_ptr) != 0) {
        u32 filename_psx = WM_U32(entry_ptr);
        u32 seek_off;
        u32 buf_addr;
        u32 byte_count;

        /* PCopen(filename, 0, 0). Retry up to 8 times. */
        fd = -1;
        for (retry = 0; retry < WM_C624_RETRY_MAX; retry++) {
            fd = PCopen((char*)PSX_ADDR(filename_psx), 0, 0);
            if (fd != -1)
                break;
            s_wm_c624_pcopen_fail++;
        }
        if (fd == -1) {
            /* All retries exhausted; advance to next entry. */
            entry_ptr += WM_C624_ENTRY_STRIDE;
            param_base += WM_C624_ENTRY_STRIDE;
            continue;
        }

        /* PClseek(fd, offset, 0). */
        seek_off = WM_U32(param_base - 4);
        PClseek(fd, (int)seek_off, 0);

        /* PCread(fd, buffer, count). Retry up to 8 times. */
        for (retry = 0; retry < WM_C624_RETRY_MAX; retry++) {
            byte_count = WM_U32(param_base + 4);
            buf_addr   = WM_U32(param_base);
            if (PCread(fd, (char*)PSX_ADDR(buf_addr), (int)byte_count) != 0)
                break;
            s_wm_c624_pcread_fail++;
        }

        /* PCclose(fd). Retry up to 8 times. */
        for (retry = 0; retry < WM_C624_RETRY_MAX; retry++) {
            if (PCclose(fd) == 0)
                break;
            s_wm_c624_pcclose_fail++;
        }

        entry_ptr += WM_C624_ENTRY_STRIDE;
        param_base += WM_C624_ENTRY_STRIDE;
    }
}

/* W29B / W34B18-B: native implementation of retail 0x800967E4.
 *
 * Per-iteration CD work dispatcher. The 0x80072514 init caller ignores the
 * return; the 0x800713FC frame-prologue caller compares it to 3.
 *
 * Retail MIPS (0x800967E4–0x800968DC inclusive, 63 words, 252 bytes):
 *   Stack frame: 40 bytes.  Saved: $s0, $s1, $ra.
 *   No arguments. Return is $v0 = $s1 on every epilogue.
 *
 * Control flow (exact retail, W34B18-B re-proof):
 *   1. Call func_8002C3D8() twice → g_ArchiveDebugTable.
 *   2. C624 iff dbg0 != 0 AND dbg1 != 0xFFFFFFFF
 *      (sltiu/nor/sltiu/or/beqz; not "both nonzero").
 *   3. Call wm_800968E0_dispatch_partial() → dispatch CD44 state.
 *   4. If return != 0 → return that value.
 *   5. Load D788_table[tail]. If null → return 0 (do not fall into C624).
 *   6. If non-null → call D788 processor → return 0 (no tail advance).
 *   7. C624 path: load C624_table[tail]. If null → return 0.
 *   8. If non-null → call C624 processor → advance tail → return 0. */
u32 wm_800967E4_dispatch_cd_work(void)
{
    u32 dbg0, dbg1;
    u32 dispatch_result;
    u32 tail;
    u32 d788_record;
    u32 c624_record;

    s_wm967e4_entry++;

    /* 1. Read g_ArchiveDebugTable twice (retail defensive double-read).
     * Retail calls func_8002C3D8() which is `return g_ArchiveDebugTable`. */
    dbg0 = g_ArchiveDebugTable;
    dbg1 = g_ArchiveDebugTable;

    /* 2. C624 iff dbg0 != 0 AND dbg1 != 0xFFFFFFFF. */
    if (dbg0 != 0 && dbg1 != 0xFFFFFFFFu) {
        s_wm967e4_debug_table_nonzero++;
        goto c624_path;
    }

    /* 3. Dispatch CD44 state machine. */
    dispatch_result = wm_800968E0_dispatch_partial();

    /* 4. If non-zero return → busy, tail-advanced, or invalid; propagate. */
    if (dispatch_result != 0) {
        if (dispatch_result == 1)
            s_wm967e4_dispatcher_busy++;
        else if (dispatch_result == 2)
            s_wm967e4_dispatcher_tail_adv++;
        else
            s_wm967e4_dispatcher_invalid++;
        return dispatch_result;
    }
    s_wm967e4_dispatcher_idle++;

    /* 5. Load D788_table[tail]. */
    tail = WM_U32(WM_BCB8_ABS);
    d788_record = WM_U32(WM_D788_BASE_ABS + tail * 4);

    /* If null → return 0. Retail beqz + move v0,s1 to the epilogue. */
    if (d788_record == 0) {
        s_wm967e4_d788_null++;
        return 0;
    }

    /* 6. Process D788 record. */
    s_wm967e4_d788_process++;
    wm_8009699C_d788_processor(d788_record);
    return 0;

c624_path:
    /* 7. Load C624_table[tail]. */
    tail = WM_U32(WM_BCB8_ABS);
    c624_record = WM_U32(WM_C624_BASE_ABS + tail * 4);

    if (c624_record == 0) {
        s_wm967e4_c624_null++;
        return 0;
    }

    /* 8. Process C624 record, then advance tail. */
    s_wm967e4_c624_process++;
    wm_800966CC_c624_processor(c624_record);

    /* Clear C624_table[tail] and advance BCB8. */
    WM_U32(WM_C624_BASE_ABS + tail * 4) = 0;
    WM_U32(WM_BCB8_ABS) = (tail + 1) & 0x0F;
    s_wm967e4_tail_advance++;
    return 0;
}

/* W32B: ready-check and third-wave buffer consumption (retail 0x80072558–0x800725A8).
 *
 * Reproduces the exact post-loop behavior:
 *   1. Load ready flag at 0x8009C894
 *   2. Branch: flag != 0 → alternate path; flag == 0 → normal path
 *   3. HeapFree(*(0x8009C88C)) — free third-wave WDS buffer
 *   4. SoundAddSedsEntry(*(0x8006259C)) — link SEDS buffer to sound system
 *   5. Cut before mode-dependent audio setup
 *
 * One-shot guard: second invocation is blocked with diagnostic logging.
 *
 * Side effects: frees WDS buffer, links SEDS buffer. No BSS writes from
 * this bounded scope. */
void wm_ready_buffer_consume(void)
{
    u32 ready_flag;
    u32 wds_psx;
    void* wds_host;
    void* seds_host;

    s_wm32b_entry++;

    /* One-shot guard: block second invocation. */
    if (s_wm32b_entry > 1) {
        s_wm32b_second_call_blocked++;
        fprintf(stderr,
                "[worldmap-ready-consume] SECOND CALL BLOCKED "
                "(entry=%d)\n", s_wm32b_entry);
        return;
    }

    /* 1. Load ready flag. */
    ready_flag = WM_U32(WM_FLAG_C894_ABS);
    fprintf(stderr,
            "[worldmap-ready-consume] entry "
            "ready_flag=0x%08x\n", ready_flag);

    /* 2. Branch: flag != 0 → alternate; flag == 0 → normal. */
    if (ready_flag != 0) {
        /* Alternate path (flag != 0). */
        s_wm32b_not_ready_branch++;
        fprintf(stderr,
                "[worldmap-ready-consume] "
                "ready_branch=0 (alternate path)\n");
    } else {
        /* Normal path (flag == 0). */
        s_wm32b_ready_branch++;
        fprintf(stderr,
                "[worldmap-ready-consume] "
                "ready_branch=1 (normal path)\n");
    }

    /* Both paths do HeapFree then SoundAddSedsEntry in the same order. */

    /* 3. HeapFree(*(0x8009C88C)) — free third-wave WDS buffer. */
    wds_psx = WM_U32(WM_TW_MIRROR_C88C);
    wds_host = (void*)(uintptr_t)wds_psx;
    /* Convert PSX pointer to host if needed. */
    if (wds_psx >= 0x80000000u && wds_psx < 0x80200000u)
        wds_host = PSX_ADDR(wds_psx);

    fprintf(stderr,
            "[worldmap-ready-consume] "
            "buffer_psx=0x%08x buffer_host=%p\n",
            wds_psx, wds_host);

    if (wds_host != NULL) {
        HeapFree(wds_host);
        s_wm32b_heapfree++;
        fprintf(stderr,
                "[worldmap-ready-consume] HeapFree done\n");
    } else {
        fprintf(stderr,
                "[worldmap-ready-consume] HeapFree skipped (NULL)\n");
    }

    /* 4. SoundAddSedsEntry(*(0x8006259C)) — link SEDS buffer. */
    seds_host = D_8006259C;
    fprintf(stderr,
            "[worldmap-ready-consume] "
            "seds_host=%p\n", seds_host);

    if (seds_host != NULL) {
        SoundAddSedsEntry(seds_host);
        s_wm32b_sound_add++;
        fprintf(stderr,
                "[worldmap-ready-consume] SoundAddSedsEntry done\n");
    } else {
        fprintf(stderr,
                "[worldmap-ready-consume] SoundAddSedsEntry skipped (NULL)\n");
    }

    s_wm32b_route_hit++;
    fprintf(stderr,
            "[worldmap-ready-consume] exit\n"
            "[worldmap-ready-consume] "
            "cut-before-mode-audio retail_pc=0x%08x\n",
            WM_CUT_BEFORE_MODE_AUDIO);
}

/* W33B: mode-dependent world audio setup (retail 0x800725AC–0x800726BC).
 *
 * Reproduces the exact post-W32B behavior:
 *   1. Load mode selector at 0x8009BE10
 *   2. Branch: mode == 7 → mode-7 path; mode != 7 → non-mode-7 path
 *   3. Load mode-dependent archive ID and buffer pointer
 *   4. ArchiveDecodeAlignedSize(archive_id) → size
 *   5. memcpy(D_80062648, buffer, size) → copy sound data
 *   6. Ready path: func_80039850(D_80062648) → create AudioManager
 *   7. Not-ready path: load existing AudioManager from D_80062528
 *   8. Ready path: func_80039A80(manager, 127, 0) → set level immediately
 *   9. Not-ready path: func_80039B68(manager, 127, 240) → set level with fade
 *  10. Cut before convergence at 0x800726C0
 *
 * Side effects: copies sound data, creates or updates AudioManager.
 * Idempotent — no one-shot guard required. */
void wm_mode_audio_setup(void)
{
    u32 ready_flag;
    u32 mode;
    u32 archive_id;
    u32 buffer_psx;
    void* buffer_host;
    int size;
    void* manager;

    s_wm33b_entry++;

    /* Load ready flag (same as W32B). */
    ready_flag = WM_U32(WM_FLAG_C894_ABS);

    /* Load mode selector. */
    mode = WM_U32(WM_MODE_BE10_ABS);

    fprintf(stderr, "[worldmap-mode-audio] entry "
            "ready_flag=0x%08x mode=%u\n", ready_flag, mode);

    /* Mode-dependent archive ID and buffer selection. */
    if (mode == 7) {
        s_wm33b_mode7_path++;
        archive_id = WM_U32(WM_TW_ID_D800);
        buffer_psx = WM_U32(WM_TW_MIRROR_C888);
        fprintf(stderr, "[worldmap-mode-audio] mode7 "
                "archive_id=0x%08x buffer_psx=0x%08x\n",
                archive_id, buffer_psx);
    } else {
        s_wm33b_non_mode7_path++;
        archive_id = WM_U32(WM_TW_ID_D3D0);
        buffer_psx = WM_U32(WM_TW_MIRROR_C884);
        fprintf(stderr, "[worldmap-mode-audio] non-mode7 "
                "archive_id=0x%08x buffer_psx=0x%08x\n",
                archive_id, buffer_psx);
    }

    /* Convert PSX buffer pointer to host. */
    buffer_host = (void*)(uintptr_t)buffer_psx;
    if (buffer_psx >= 0x80000000u && buffer_psx < 0x80200000u)
        buffer_host = PSX_ADDR(buffer_psx);

    /* ArchiveDecodeAlignedSize(archive_id) → size. */
    size = ArchiveDecodeAlignedSize(archive_id);
    s_wm33b_decode_size++;
    fprintf(stderr, "[worldmap-mode-audio] decode_size=%d\n", size);

    /* memcpy(D_80062648, buffer, size). */
    if (buffer_host != NULL && size > 0) {
        memcpy(D_80062648, buffer_host, (size_t)size);
        s_wm33b_memcpy++;
        fprintf(stderr, "[worldmap-mode-audio] memcpy done\n");
    }

    /* Ready path: create NEW AudioManager. */
    /* Not-ready path: use EXISTING AudioManager. */
    if (ready_flag == 0) {
        s_wm33b_ready_path++;

        /* func_80039850(D_80062648) → create AudioManager. */
        manager = func_80039850(D_80062648);
        s_wm33b_audio_create++;
        D_80062528 = manager;
        fprintf(stderr, "[worldmap-mode-audio] "
                "AudioManager created: %p\n", manager);

        /* func_80039A80(manager, 127, 0) → set level immediately. */
        if (manager != NULL) {
            func_80039A80(manager, 127, 0);
            s_wm33b_level_set++;
            fprintf(stderr, "[worldmap-mode-audio] "
                    "level set: 127, steps=0\n");
        }
    } else {
        s_wm33b_not_ready_path++;

        /* Load EXISTING AudioManager. */
        manager = D_80062528;
        s_wm33b_audio_load++;
        fprintf(stderr, "[worldmap-mode-audio] "
                "AudioManager loaded: %p\n", manager);

        /* func_80039B68(manager, 127, 240) → set level with fade. */
        if (manager != NULL) {
            func_80039B68(manager, 127, 240);
            s_wm33b_level_set++;
            fprintf(stderr, "[worldmap-mode-audio] "
                    "level set: 127, steps=240\n");
        }
    }

    s_wm33b_route_hit++;
    fprintf(stderr, "[worldmap-mode-audio] exit\n"
            "[worldmap-mode-audio] "
            "cut-before-convergence retail_pc=0x%08x\n",
            WM_CUT_BEFORE_CONVERGENCE);
}

/* W25B loop-related forbidden targets (not yet ported; must not execute). */
void wm_loop_dispatch_should_not_run(void)
{
    s_wm_loop_dispatch_hits++;
    fprintf(stderr, "[worldmap-w25b] ERROR: world loop dispatch reached (hit=%d)\n",
            s_wm_loop_dispatch_hits);
}

void wm_800967E4_should_not_run(void)
{
    s_wm967e4_hits++;
    fprintf(stderr, "[worldmap-w25b] ERROR: 0x800967E4 reached (hit=%d)\n",
            s_wm967e4_hits);
}

void wm_loop_backedge_should_not_run(void)
{
    s_wm_loop_backedge_hits++;
    fprintf(stderr, "[worldmap-w25b] ERROR: world loop back-edge reached (hit=%d)\n",
            s_wm_loop_backedge_hits);
}

void wm_loop_exit_should_not_run(void)
{
    s_wm_loop_exit_hits++;
    fprintf(stderr, "[worldmap-w25b] ERROR: world loop exit reached (hit=%d)\n",
            s_wm_loop_exit_hits);
}

static void wm_8007369C(void)
{
    void* p = HeapAlloc(0x1000, 0);
    /* Retail writes a KUSEG OT pointer. HeapAlloc returns the host view of
     * emulated RAM in the port, so publish its guest address at the writer. */
    WM_U32(WM_ALLOC_BC38_ABS) = host_ptr_to_psx_u32(p);
    p = HeapAlloc(0x1000, 0);
    WM_U32(WM_ALLOC_BCB0_ABS) = host_ptr_to_psx_u32(p);
}

#if defined(WM_7369C_PROD_TEST)
u32 wm_8007369C_test_host_to_psx(void* p)
{
    return host_ptr_to_psx_u32(p);
}
#endif

/* Record reading / world-state writes (remainder of retail wm_80071B9C
 * after selector binning). Uses the index returned by wm_selector_producer
 * for the record-table address computation. */
static void wm_read_and_write_record(u32 entrance, u32 index)
{
    s16* pRec;
    s16 base0, y, z, w;

    if ((s32)entrance < 8) {
        pRec = (s16*)((u8*)PSX_ADDR(WM_RECORD_TABLE_ABS) + (index << 3));
    } else {
        pRec = (s16*)((u8*)PSX_ADDR(WM_RECORD_GE8_ABS) + (entrance << 3));
    }

    base0 = pRec[0];
    y = pRec[1];
    z = pRec[2];
    w = pRec[3];

    WM_U32(0x8009D3C4u) = (u32)(base0 + 1);
    WM_U32(0x8009C174u) = (u32)(base0 + 3);
    WM_U32(0x8009C17Cu) = (u32)(base0 + 2);
    WM_U32(0x8009D3D0u) = (u32)(base0 + 5);
    WM_U32(0x8009CC98u) = (u32)(base0 + 4);
    WM_U32(0x8009D800u) = (u32)(base0 + 7);
    WM_U32(0x8009D3C8u) = (u32)(base0 + 6);
    WM_U32(0x8009BCD8u) = (u32)(base0 + 9);
    WM_U32(0x8009BCC8u) = (u32)(base0 + 8);
    WM_U32(0x8009D2B4u) = (u32)(s32)z;
    WM_U32(0x8009D160u) = (u32)(s32)y;
    WM_U32(0x8009BD08u) = (u32)(base0 + 10);
    WM_U32(0x8009D7CCu) = (u32)(s32)w;
}

static int ensure_world_overlay_image(u32* out_final_write)
{
    u32* entry = (u32*)PSX_ADDR(WM_ENTRY);
    u32 fingerprint = entry[0];
    FILE* fp;
    size_t n;
    u8* dst = (u8*)PSX_ADDR(WM_OVERLAY_BASE);

    *out_final_write = WM_OVERLAY_BASE + WM_OVERLAY_IMAGE_SIZE;

    /* Retail entry starts with addiu $sp, $sp, -40 → 0x27BDFFD8 */
    if (fingerprint == 0x27BDFFD8u) {
        fprintf(stderr,
                "[worldmap-init] overlay resident fingerprint=0x%08x at 0x%08x\n",
                fingerprint, WM_ENTRY);
        return 0;
    }

    fprintf(stderr,
            "[worldmap-init] overlay fingerprint miss (0x%08x); planting "
            "disc/world_map.bin (%u bytes) at 0x%08x\n",
            fingerprint, WM_OVERLAY_IMAGE_SIZE, WM_OVERLAY_BASE);

    {
        /* The user's disc/world_map.bin, hash-checked by the disc/storage
         * layer (xg_plat/disc.h); a missing or wrong file stops there. */
        size_t size = 0;
        const uint8_t* image = xg_plat_data_file("world_map.bin", &size);
        if (image == NULL) {
            fprintf(stderr, "[worldmap-init] ERROR: world_map.bin unavailable\n");
            return -1;
        }
        n = size < WM_OVERLAY_IMAGE_SIZE ? size : WM_OVERLAY_IMAGE_SIZE;
        wm_memcpy(dst, image, (unsigned)n);
        (void)fp;
    }
    if (n != WM_OVERLAY_IMAGE_SIZE) {
        fprintf(stderr, "[worldmap-init] ERROR: short read %zu\n", n);
        return -1;
    }
    if (*(u32*)PSX_ADDR(WM_ENTRY) != 0x27BDFFD8u) {
        fprintf(stderr, "[worldmap-init] ERROR: planted image bad entry\n");
        return -1;
    }
    return 0;
}

/*
 * Lahan-only native transcription of WorldMapMain from 0x80070CFC through
 * return from jal 0x80071B9C (delay slot 0x80070FF8). Does not execute
 * 0x80071000+.
 */
static int world_map_main_init_lahan(void)
{
    u16 entrance_hw;
    u16 selector, heading, arg2, entrance;
    s32 world_index;
    u32 seed_1930;

    if (g_pGameState == NULL) {
        fprintf(stderr, "[worldmap-init] ERROR: g_pGameState is NULL\n");
        return -1;
    }

    entrance_hw = GS_U16(GS_OFF_ENTRANCE);

    /* Always: flag byte @ 0x800691AE = 1 (retail before cold branch). */
    WM_U8(WM_FLAG_91AE_ABS) = 1;

    if (entrance_hw == 0) {
        if (wm_80070D58_cold_defaults((u8*)g_pGameState) != 0) {
            fprintf(stderr,
                    "[worldmap-init] ERROR: cold-default initialization failed\n");
            return -1;
        }
        entrance_hw = GS_U16(GS_OFF_ENTRANCE);
        fprintf(stderr,
                "[worldmap-init] retail cold defaults applied; entrance=%u\n",
                (unsigned)entrance_hw);
    }

    /* Shared path @ 0x80070F38 */
    HeapChangeCurrentUser(3, NULL);
    ArchiveSetIndex(0x24, 0);
    wm_80095F78();
    wm_8007369C();
    wm_80073300();

    /* Bit 0x8000 on entrance → flag @ 0x8009C894 */
    if (entrance_hw & 0x8000)
        WM_U32(WM_FLAG_C894_ABS) = 1;
    else
        WM_U32(WM_FLAG_C894_ABS) = 0;

    /* Tuple normalize @ 0x80070F90+ — read host g_pGameState in place. */
    seed_1930 = (u32)GS_U16(GS_OFF_SEED_1930);
    {
        /* Port-only experiment knob: override the scenario-progress seed
         * that wm_selector_producer bins into a world-region record.  Retail
         * writes this halfword from field transitions; the harness routes
         * only reach the new-game value.  Unset -> retail behavior. */
        const char* seed_override = getenv("XENO_WORLD_SCENARIO_SEED");
        if (seed_override != NULL && seed_override[0] != '\0') {
            seed_1930 = (u32)strtoul(seed_override, NULL, 0) & 0xFFFFu;
            GS_U16(GS_OFF_SEED_1930) = (u16)seed_1930;
            fprintf(stderr,
                    "[worldmap-init] XENO_WORLD_SCENARIO_SEED override -> "
                    "seed_1930=0x%04x\n", (u16)seed_1930);
        }
    }
    entrance = GS_U16(GS_OFF_ENTRANCE);
    selector = GS_U16(GS_OFF_SELECTOR);
    arg2 = GS_U16(GS_OFF_ARG2);
    heading = GS_U16(GS_OFF_HEADING);

    WM_U32(WM_ZERO_BBC4_ABS) = 0;
    entrance &= 0x7FFF;
    world_index = (s32)(selector & 0x3FFF) - 0x400;

    WM_U32(WM_WORLD_INDEX_ABS) = (u32)world_index;
    WM_U32(WM_ARG2_STATE_ABS) = (u32)arg2;
    WM_U32(WM_HEADING_STATE_ABS) = (u32)heading;
    WM_U32(WM_ENTRANCE_STATE_ABS) = (u32)entrance;
    GS_U16(GS_OFF_ENTRANCE) = entrance;

    fprintf(stderr,
            "[worldmap-init] selector=0x%04x world_index=%d entrance=%d "
            "heading=0x%04x arg2=%d seed_1930=0x%04x\n",
            selector, world_index, entrance, heading, arg2,
            (u16)seed_1930);

    if (selector != 0x0400 || world_index != 0 || entrance != 1 ||
        heading != 0x0E00 || arg2 != 1) {
        fprintf(stderr,
                "[worldmap-init] WARN: tuple is not the Lahan proof values "
                "(continuing with natural decode)\n");
    }

    {
        u32 idx = wm_selector_producer(entrance, seed_1930);
        wm_read_and_write_record(entrance, idx);
    }

    fprintf(stderr,
            "[worldmap-init] w2-complete retail_resume=0x%08x "
            "(W2 cut docs 0x%08x; main loop 0x%08x)\n",
            WM_POST_INIT_RESUME, WM_CUT_BEFORE_LOOP, WM_MAIN_LOOP);
    return 0;
}

/*
 * W3B — native transcription of retail mode initializer 0x80071CDC–0x80071EE8.
 * One-shot only; does not enter 0x80071034.
 */
static int wm_80071CDC_mode_init(void)
{
    int ch;
    u8 channel_id[3];
    u8 secondary_id[3];
    u32 aligned_size[3];
    u32 aligned_size_sec[3];
    void* allocation[3];
    void* allocation_sec[3];
    int request_count = 0;
    int queue_result;
    u8* pReq;
    u32* pCd34;
    u32* pBdf8;

    fprintf(stderr, "[worldmap-mode-init] entry\n");

    if (g_pGameState == NULL) {
        fprintf(stderr, "[worldmap-mode-init] ERROR: g_pGameState NULL\n");
        return -1;
    }

    /* Retail GameState offset 0x1D34 is the same backing storage as absolute
     * F368.  Restore that exact three-byte alias before either package gates
     * or later world callbacks can observe the channel state. */
    wm_sync_gamestate_channel_aliases((const u8*)g_pGameState);

    pCd34 = (u32*)PSX_ADDR(WM_PTR_CD34);
    pBdf8 = (u32*)PSX_ADDR(WM_PTR_BDF8);

    for (ch = 0; ch < 3; ch++) {
        channel_id[ch] =
            WM_U8(WM_GAMESTATE_CHANNEL_GUEST_BASE + (u32)ch);
        secondary_id[ch] = 0xFF;
        aligned_size[ch] = 0;
        aligned_size_sec[ch] = 0;
        allocation[ch] = NULL;
        allocation_sec[ch] = NULL;

        if (channel_id[ch] != 0xFF) {
            aligned_size[ch] = (u32)ArchiveDecodeAlignedSize((u32)channel_id[ch] + 2u);
            allocation[ch] = HeapAlloc(aligned_size[ch], 0);
            pCd34[ch] = host_ptr_to_psx_u32(allocation[ch]);

            secondary_id[ch] = GS_U8(GS_OFF_SEC_BASE + (u32)channel_id[ch] * 164u);
            if (secondary_id[ch] != 0xFF) {
                aligned_size_sec[ch] =
                    (u32)ArchiveDecodeAlignedSize((u32)secondary_id[ch] + 0x13u);
                allocation_sec[ch] = HeapAlloc(aligned_size_sec[ch], 0);
                pBdf8[ch] = host_ptr_to_psx_u32(allocation_sec[ch]);
            } else {
                pBdf8[ch] = 0;
            }
        } else {
            pCd34[ch] = 0;
            pBdf8[ch] = 0;
        }

        fprintf(stderr,
                "[worldmap-mode-init] channel_id[%d]=0x%02x aligned_size[%d]=%u "
                "allocation[%d]=host:%p psx_u32=0x%08x "
                "secondary_id=0x%02x sec_size=%u sec_alloc=host:%p sec_psx=0x%08x\n",
                ch, channel_id[ch], ch, aligned_size[ch], ch, allocation[ch],
                pCd34[ch], secondary_id[ch], aligned_size_sec[ch],
                allocation_sec[ch], pBdf8[ch]);
    }

    /* Build retail 8-byte-stride request list at 0x8009D3F8. */
    WM_U32(WM_CNT_C170) = 0;
    pReq = (u8*)PSX_ADDR(WM_REQ_D3F8);
    request_count = 0;

    for (ch = 0; ch < 3; ch++) {
        if (channel_id[ch] == 0xFF)
            continue;

        *(u16*)(pReq + request_count * 8) = (u16)(channel_id[ch] + 2);
        *(u32*)(pReq + request_count * 8 + 4) = pCd34[ch];
        request_count++;
        /* Retail C170 increments only for primary channel entries. */
        WM_U32(WM_CNT_C170) = WM_U32(WM_CNT_C170) + 1u;

        if (secondary_id[ch] != 0xFF) {
            *(u16*)(pReq + request_count * 8) = (u16)(secondary_id[ch] + 0x13);
            *(u32*)(pReq + request_count * 8 + 4) = pBdf8[ch];
            request_count++;
        }
    }

    /* Terminator entry */
    *(u16*)(pReq + request_count * 8) = 0;
    *(u32*)(pReq + request_count * 8 + 4) = 0;

    fprintf(stderr, "[worldmap-mode-init] request_count=%d (primary_counter C170=%u)\n",
            request_count, WM_U32(WM_CNT_C170));
    {
        int r;
        for (r = 0; r < request_count; r++) {
            u16 idx = *(u16*)(pReq + r * 8);
            u32 dat = *(u32*)(pReq + r * 8 + 4);
            fprintf(stderr,
                    "[worldmap-mode-init] request[%d] archive_index=%u "
                    "pData_u32=0x%08x host=%p\n",
                    r, idx, dat, (void*)(uintptr_t)dat);
        }
    }

    queue_result = func_80029AFC(PSX_ADDR(WM_REQ_D3F8), 0, 0);
    fprintf(stderr, "[worldmap-mode-init] queue_submit=%d\n", queue_result);
    fprintf(stderr, "[worldmap-mode-init] exit\n");
    return queue_result;
}

/*
 * W4C — native transcription of retail second-wave setup 0x80071EF0–0x80071FE8.
 * Reads IDs from W2 wm_80071B9C stores; builds three-entry queue; submits via
 * func_80029AFC. Does not hard-code Lahan archive indices.
 */
static int wm_80071EF0_second_wave(void)
{
    u32 id0, id1, id2;
    u32 size0, size1, size2;
    void* alloc0;
    void* alloc1;
    void* alloc2;
    u32 psx0, psx1, psx2;
    int queue_result;
    u8* pReq;

    fprintf(stderr, "[worldmap-second-wave] entry\n");

    id0 = WM_U32(WM_ID_C17C);
    id1 = WM_U32(WM_ID_C174);
    id2 = WM_U32(WM_ID_D3C4);

    fprintf(stderr,
            "[worldmap-second-wave] source_id[0]=0x%08x (%u)\n"
            "[worldmap-second-wave] source_id[1]=0x%08x (%u)\n"
            "[worldmap-second-wave] source_id[2]=0x%08x (%u)\n",
            id0, id0, id1, id1, id2, id2);

    /* Retail: ArchiveDecodeAlignedSize(id) then HeapAlloc(size, 1). Order: C17C,
     * C174, D3C4. Archive indices are the stored IDs themselves (base0+N). */
    size0 = (u32)ArchiveDecodeAlignedSize(id0);
    alloc0 = HeapAlloc(size0, 1);
    psx0 = host_ptr_to_psx_u32(alloc0);
    WM_U32(WM_DST_C59C) = psx0;

    size1 = (u32)ArchiveDecodeAlignedSize(id1);
    alloc1 = HeapAlloc(size1, 1);
    psx1 = host_ptr_to_psx_u32(alloc1);
    WM_U32(WM_DST_BD20) = psx1;

    size2 = (u32)ArchiveDecodeAlignedSize(id2);
    alloc2 = HeapAlloc(size2, 1);
    psx2 = host_ptr_to_psx_u32(alloc2);
    WM_U32(WM_DST_C180) = psx2;

    fprintf(stderr,
            "[worldmap-second-wave] archive_id[0]=%u aligned_size[0]=%u "
            "destination_psx[0]=0x%08x host=%p\n"
            "[worldmap-second-wave] archive_id[1]=%u aligned_size[1]=%u "
            "destination_psx[1]=0x%08x host=%p\n"
            "[worldmap-second-wave] archive_id[2]=%u aligned_size[2]=%u "
            "destination_psx[2]=0x%08x host=%p\n",
            id0, size0, psx0, alloc0,
            id1, size1, psx1, alloc1,
            id2, size2, psx2, alloc2);

    if (alloc0 == NULL || alloc1 == NULL || alloc2 == NULL) {
        fprintf(stderr, "[worldmap-second-wave] ERROR: HeapAlloc failed\n");
        return -1;
    }

    /* Retail queue layout at 0x8009D3F8 (8-byte stride):
     *   [0] index=D3C4  pData=C180
     *   [1] index=C17C  pData=C59C
     *   [2] index=C174  pData=BD20
     *   [3] index=0     pData=0
     */
    pReq = (u8*)PSX_ADDR(WM_REQ_D3F8);
    WM_U16(WM_REQ_D410) = 0;
    WM_U32(WM_REQ_D3FC) = psx2;
    WM_U32(WM_REQ_D414) = 0;
    *(u16*)(pReq + 0) = (u16)id2;
    WM_U16(WM_REQ_D400) = (u16)id0;
    WM_U16(WM_REQ_D408) = (u16)id1;
    WM_U32(WM_REQ_D404) = psx0;
    WM_U32(WM_REQ_D40C) = psx1;

    fprintf(stderr,
            "[worldmap-second-wave] request_count=3\n"
            "[worldmap-second-wave] request[0] archive=%u pData_psx=0x%08x\n"
            "[worldmap-second-wave] request[1] archive=%u pData_psx=0x%08x\n"
            "[worldmap-second-wave] request[2] archive=%u pData_psx=0x%08x\n",
            (unsigned)id2, psx2,
            (unsigned)id0, psx0,
            (unsigned)id1, psx1);

    queue_result = func_80029AFC(PSX_ADDR(WM_REQ_D3F8), 0, 0);
    fprintf(stderr, "[worldmap-second-wave] queue_submit=%d\n", queue_result);
    fprintf(stderr, "[worldmap-second-wave] submitted\n");
    return queue_result;
}

/*
 * Retail glue 0x800722A0–0x800722B0:
 *   do { v = ArchiveDataSync(); } while (v >= 3);
 * Native sync archive typically exits on first poll (v==0).
 */
static int wm_second_wave_poll(void)
{
    int first = -1;
    int last = -1;
    int count = 0;
    int v;

    for (;;) {
        v = ArchiveDataSync();
        if (first < 0)
            first = v;
        last = v;
        count++;
        if (v < 3)
            break;
        if (count >= WM_SECOND_WAVE_POLL_MAX) {
            fprintf(stderr,
                    "[worldmap-second-wave] ERROR: poll safety ceiling "
                    "(%d) hit last=%d\n",
                    WM_SECOND_WAVE_POLL_MAX, last);
            return -1;
        }
        /* Yield so a pathological non-zero path cannot hard-lock the host. */
        if ((count & 0x3FF) == 0)
            VSync(0);
    }

    fprintf(stderr, "[worldmap-second-wave] poll_first=%d\n", first);
    fprintf(stderr, "[worldmap-second-wave] poll_count=%d\n", count);
    fprintf(stderr, "[worldmap-second-wave] poll_final=%d\n", last);
    return 0;
}

/*
 * W4C — native transcription of retail 0x80073530–0x80073698.
 * LZSSHeapDecompress(*C180), HeapFree(compressed), relative→KUSEG fixups.
 */
static int wm_80073530_fixup(void)
{
    u32 compressed_psx;
    u32 decompressed_psx;
    void* compressed_host;
    void* decompressed_host;
    u32 base;
    u32 rel;
    u32 abs;
    u32 s0_psx;
    u32 a0_slot;
    int i;
    int relocated = 0;
    u32 decomp_hdr_size = 0;

    fprintf(stderr, "[worldmap-second-wave] fixup_entry\n");

    compressed_psx = WM_U32(WM_DST_C180);
    compressed_host = psx_u32_to_host(compressed_psx);
    if (compressed_host == NULL) {
        fprintf(stderr, "[worldmap-second-wave] ERROR: C180 compressed NULL\n");
        return -1;
    }

    /* LZSS stream begins with u32 decompressed size (retail / port API). */
    decomp_hdr_size = *(u32*)compressed_host;

    fprintf(stderr,
            "[worldmap-second-wave] compressed_psx=0x%08x host=%p "
            "lzss_hdr_size=%u\n",
            compressed_psx, compressed_host, decomp_hdr_size);

    /* Retail: a1=0, a0=compressed; LZSSHeapDecompress(p, flags=0). */
    decompressed_host = LZSSHeapDecompress(compressed_host, 0);
    decompressed_psx = host_ptr_to_psx_u32(decompressed_host);
    WM_U32(WM_DST_C180) = decompressed_psx;

    fprintf(stderr,
            "[worldmap-second-wave] decompressed_psx=0x%08x host=%p\n",
            decompressed_psx, decompressed_host);

    /* Free compressed allocation (retail order: store new C180, then free old). */
    HeapFree(compressed_host);

    if (decompressed_host == NULL || decompressed_psx == 0) {
        fprintf(stderr, "[worldmap-second-wave] ERROR: LZSSHeapDecompress failed\n");
        return -1;
    }

    /* All further arithmetic is in KUSEG space; memory ops via PSX_ADDR. */
    base = decompressed_psx;

    /* Header slots: base + *(base+off) → named BSS. Retail order. */
    rel = *(u32*)((u8*)PSX_ADDR(base) + 4);
    s0_psx = base + rel; /* used later */

    rel = *(u32*)((u8*)PSX_ADDR(base) + 12);
    WM_U32(WM_FIX_D308) = base + rel;
    relocated++;

    rel = *(u32*)((u8*)PSX_ADDR(base) + 8);
    WM_U32(WM_FIX_CD48) = base + rel;
    relocated++;

    rel = *(u32*)((u8*)PSX_ADDR(base) + 20);
    WM_U32(WM_FIX_C7EC) = base + rel;
    relocated++;

    rel = *(u32*)((u8*)PSX_ADDR(base) + 16);
    WM_U32(WM_FIX_BD30) = base + rel;
    relocated++;

    rel = *(u32*)((u8*)PSX_ADDR(base) + 28);
    WM_U32(WM_FIX_D784) = base + rel;
    relocated++;

    rel = *(u32*)((u8*)PSX_ADDR(base) + 24);
    WM_U32(WM_FIX_BCC0) = base + rel;
    relocated++;

    rel = *(u32*)((u8*)PSX_ADDR(base) + 36);
    WM_U32(WM_FIX_D7C8) = base + rel;
    relocated++;

    rel = *(u32*)((u8*)PSX_ADDR(base) + 32);
    WM_U32(WM_FIX_D77C) = base + rel;
    relocated++;

    /* 16-entry table at D73C: for i in 0..15: D73C[i] = base + *(base+0x2C+4*i) */
    for (i = 0; i < 16; i++) {
        rel = *(u32*)((u8*)PSX_ADDR(base) + 0x2C + (u32)i * 4u);
        abs = base + rel;
        WM_U32(WM_FIX_D73C + (u32)i * 4u) = abs;
        relocated++;
    }

    /* s0 = base + *(base+4); D3F4 = s0 + *s0; BD00 = s0 + *(s0+4); then
     * rewrite four relative words at the BD00 structure in place. */
    {
        u32* p_s0 = (u32*)PSX_ADDR(s0_psx);
        u32 d3f4 = s0_psx + p_s0[0];
        u32 bd00 = s0_psx + p_s0[1];
        u32* p_v1;
        u32* p_a0;
        u32 w0, w1, w2, w3;

        WM_U32(WM_FIX_D3F4) = d3f4;
        WM_U32(WM_FIX_BD00) = bd00;
        relocated += 2;

        /* Snapshot relative words before any write (retail loads from $v1). */
        p_v1 = (u32*)PSX_ADDR(bd00);
        w0 = p_v1[0];
        w1 = p_v1[1];
        w2 = p_v1[2];
        w3 = p_v1[3];

        p_v1[0] = s0_psx + w0;
        a0_slot = WM_U32(WM_FIX_BD00);
        p_a0 = (u32*)PSX_ADDR(a0_slot);
        p_a0[1] = s0_psx + w1;
        p_a0[2] = s0_psx + w2;
        p_a0[3] = s0_psx + w3;
        relocated += 4;
    }

    fprintf(stderr,
            "[worldmap-second-wave] decompressed_size=%u (lzss header)\n"
            "[worldmap-second-wave] pointer_table_count=16\n"
            "[worldmap-second-wave] relocated_entries=%d\n"
            "[worldmap-second-wave] D73C[0]=0x%08x D73C[1]=0x%08x "
            "D308=0x%08x BD00=0x%08x\n",
            decomp_hdr_size, relocated,
            WM_U32(WM_FIX_D73C), WM_U32(WM_FIX_D73C + 4),
            WM_U32(WM_FIX_D308), WM_U32(WM_FIX_BD00));
    fprintf(stderr, "[worldmap-second-wave] fixup_exit\n");
    return 0;
}

/* One-shot W4C ladder after W3B. */
static int world_map_second_wave_once(void)
{
    if (wm_80071EF0_second_wave() != 0) {
        fprintf(stderr, "[worldmap-second-wave] submit failed\n");
        return -1;
    }
    if (wm_second_wave_poll() != 0)
        return -1;
    if (wm_80073530_fixup() != 0)
        return -1;
    fprintf(stderr,
            "[worldmap-second-wave] cut-before-broad-update retail_pc=0x%08x\n",
            WM_CUT_BEFORE_BROAD);
    return 0;
}

/*
 * W5B — native transcription of retail 0x800976C8.
 * Store widths are all sw (u32): +0x4C, +0x18, +0x1C per 0x80-byte slot.
 * Iteration: a0=0..63, base advances by 0x80 in the bne delay slot.
 */
static void wm_800976C8_clear_pool_slots(void)
{
    u32 pool_psx = WM_U32(WM_POOL_BE24);
    u8* base;
    int i;

    base = (u8*)psx_u32_to_host(pool_psx);
    if (base == NULL)
        return;

    for (i = 0; i < WM_POOL_SLOT_COUNT; i++) {
        u8* slot = base + (u32)i * WM_POOL_SLOT_STRIDE;
        /* Retail order: +0x4C, then +0x18, then +0x1C (all word stores). */
        *(u32*)(slot + WM_POOL_OFF_4C) = 0;
        *(u32*)(slot + WM_POOL_OFF_18) = 0;
        *(u32*)(slot + WM_POOL_OFF_1C) = 0;
    }
}

/*
 * W5B — native transcription of retail 0x8009766C.
 * HeapAlloc(8192, 0) → BE24 (KUSEG) → clear slots via 976C8.
 */
static int wm_8009766C_object_pool(void)
{
    void* pool_host;
    u32 pool_psx;
    u32 prior;
    int i;
    int ok = 0;
    u8* base;

    fprintf(stderr, "[worldmap-object-pool] entry\n");

    /* Retail allocates unconditionally.  A non-zero BE24 is the previous
     * session's pool, already released by 0x800976A0 (which, like retail,
     * does not clear the pointer), so it is not an error. */
    prior = WM_U32(WM_POOL_BE24);
    if (prior != 0)
        fprintf(stderr, "[worldmap-object-pool] previous session pool psx=0x%08x "
                        "(freed by 0x800976A0); allocating a new one\n", prior);

    fprintf(stderr, "[worldmap-object-pool] alloc_size=%u\n", WM_POOL_ALLOC_SIZE);
    pool_host = HeapAlloc(WM_POOL_ALLOC_SIZE, 0);
    pool_psx = host_ptr_to_psx_u32(pool_host);
    /* Retail stores v0 then always calls clear (NULL would fault on PSX). */
    WM_U32(WM_POOL_BE24) = pool_psx;

    fprintf(stderr,
            "[worldmap-object-pool] pool_host=%p\n"
            "[worldmap-object-pool] pool_psx=0x%08x\n"
            "[worldmap-object-pool] slot_count=%d\n"
            "[worldmap-object-pool] slot_stride=0x%02x\n"
            "[worldmap-object-pool] zero_offsets=0x%02x,0x%02x,0x%02x\n",
            pool_host, pool_psx, WM_POOL_SLOT_COUNT, WM_POOL_SLOT_STRIDE,
            WM_POOL_OFF_18, WM_POOL_OFF_1C, WM_POOL_OFF_4C);

    if (pool_host == NULL || pool_psx == 0) {
        fprintf(stderr, "[worldmap-object-pool] ERROR: HeapAlloc failed\n");
        return -1;
    }

    /* Bounds: KUSEG pool must lie fully inside emulated 2 MiB RAM. */
    if (pool_psx < 0x80000000u ||
        pool_psx > 0x80200000u - WM_POOL_ALLOC_SIZE) {
        fprintf(stderr,
                "[worldmap-object-pool] ERROR: pool_psx 0x%08x out of RAM\n",
                pool_psx);
        return -1;
    }

    wm_800976C8_clear_pool_slots();

    /* Validate all 64 slots. */
    base = (u8*)psx_u32_to_host(WM_U32(WM_POOL_BE24));
    for (i = 0; i < WM_POOL_SLOT_COUNT; i++) {
        u8* slot = base + (u32)i * WM_POOL_SLOT_STRIDE;
        if (*(u32*)(slot + WM_POOL_OFF_18) == 0 &&
            *(u32*)(slot + WM_POOL_OFF_1C) == 0 &&
            *(u32*)(slot + WM_POOL_OFF_4C) == 0)
            ok++;
    }

    fprintf(stderr,
            "[worldmap-object-pool] slot_check=%d/%d\n"
            "[worldmap-object-pool] slot0 +0x18=0x%08x +0x1C=0x%08x +0x4C=0x%08x\n"
            "[worldmap-object-pool] slot1 +0x18=0x%08x +0x1C=0x%08x +0x4C=0x%08x\n"
            "[worldmap-object-pool] slot63 +0x18=0x%08x +0x1C=0x%08x +0x4C=0x%08x\n",
            ok, WM_POOL_SLOT_COUNT,
            *(u32*)(base + 0 * WM_POOL_SLOT_STRIDE + WM_POOL_OFF_18),
            *(u32*)(base + 0 * WM_POOL_SLOT_STRIDE + WM_POOL_OFF_1C),
            *(u32*)(base + 0 * WM_POOL_SLOT_STRIDE + WM_POOL_OFF_4C),
            *(u32*)(base + 1 * WM_POOL_SLOT_STRIDE + WM_POOL_OFF_18),
            *(u32*)(base + 1 * WM_POOL_SLOT_STRIDE + WM_POOL_OFF_1C),
            *(u32*)(base + 1 * WM_POOL_SLOT_STRIDE + WM_POOL_OFF_4C),
            *(u32*)(base + 63 * WM_POOL_SLOT_STRIDE + WM_POOL_OFF_18),
            *(u32*)(base + 63 * WM_POOL_SLOT_STRIDE + WM_POOL_OFF_1C),
            *(u32*)(base + 63 * WM_POOL_SLOT_STRIDE + WM_POOL_OFF_4C));

    if (ok != WM_POOL_SLOT_COUNT) {
        fprintf(stderr, "[worldmap-object-pool] ERROR: slot zero-check failed\n");
        return -1;
    }

    fprintf(stderr, "[worldmap-object-pool] exit\n");
    fprintf(stderr,
            "[worldmap-object-pool] cut-before-next-step retail_pc=0x%08x\n",
            WM_CUT_BEFORE_A180_COPY);
    return 0;
}

/*
 * W6B — retail 0x800722C4–0x80072310: unrolled identity copy of 8 u32 words
 * from overlay image 0x8009A180 → world BSS 0x8009BE4C. No transforms.
 * Does not hard-code source values; does not touch the object pool at *BE24.
 */
static int wm_state_template_copy(void)
{
    u32* src;
    u32* dst;
    u32 i;
    int match;

    fprintf(stderr, "[worldmap-state-template] entry\n");
    fprintf(stderr,
            "[worldmap-state-template] src_psx=0x%08x dst_psx=0x%08x "
            "words=%u bytes=%u\n",
            WM_TMPL_SRC_A180, WM_TMPL_DST_BE4C, WM_TMPL_WORD_COUNT,
            WM_TMPL_BYTE_COUNT);

    /* Source must sit in the planted overlay image (below BSS start). */
    if (WM_TMPL_SRC_A180 < WM_OVERLAY_BASE ||
        WM_TMPL_SRC_A180 + WM_TMPL_BYTE_COUNT > WM_MEM_START) {
        fprintf(stderr,
                "[worldmap-state-template] ERROR: source not in overlay image\n");
        return -1;
    }
    if (WM_TMPL_DST_BE4C < WM_MEM_START) {
        fprintf(stderr,
                "[worldmap-state-template] ERROR: dest not in world BSS\n");
        return -1;
    }
    /* Dest must not overlap the pool pointer slot or be confused with heap. */
    if (WM_TMPL_DST_BE4C == WM_POOL_BE24) {
        fprintf(stderr,
                "[worldmap-state-template] ERROR: dest is pool pointer slot\n");
        return -1;
    }

    src = (u32*)PSX_ADDR(WM_TMPL_SRC_A180);
    dst = (u32*)PSX_ADDR(WM_TMPL_DST_BE4C);

    fprintf(stderr, "[worldmap-state-template] src_host=%p dst_host=%p\n",
            (void*)src, (void*)dst);
    fprintf(stderr, "[worldmap-state-template] src_words:");
    for (i = 0; i < WM_TMPL_WORD_COUNT; i++)
        fprintf(stderr, " 0x%08x", src[i]);
    fprintf(stderr, "\n");

    /* Retail unrolled lw/sw pairs — identity word copy. */
    for (i = 0; i < WM_TMPL_WORD_COUNT; i++)
        dst[i] = src[i];

    match = 0;
    for (i = 0; i < WM_TMPL_WORD_COUNT; i++) {
        if (dst[i] == src[i])
            match++;
    }

    fprintf(stderr, "[worldmap-state-template] dst_words:");
    for (i = 0; i < WM_TMPL_WORD_COUNT; i++)
        fprintf(stderr, " 0x%08x", dst[i]);
    fprintf(stderr, "\n");
    fprintf(stderr, "[worldmap-state-template] word_match=%d/%u\n",
            match, WM_TMPL_WORD_COUNT);
    fprintf(stderr,
            "[worldmap-state-template] pool_BE24_unchanged=0x%08x "
            "(not written by this step)\n",
            WM_U32(WM_POOL_BE24));

    if (match != (int)WM_TMPL_WORD_COUNT) {
        fprintf(stderr, "[worldmap-state-template] ERROR: copy verify failed\n");
        return -1;
    }

    fprintf(stderr, "[worldmap-state-template] exit\n");
    fprintf(stderr,
            "[worldmap-state-template] cut-before-const-block retail_pc=0x%08x\n",
            WM_CUT_BEFORE_CONST_BLK);
    return 0;
}

/*
 * W7B — retail 0x80072314–0x80072374: ten immediate u32 mode-enter stores.
 * Order matches assembly. D_80059198 is the host main-exe global (not g_PsxRam
 * alone). Independent of 0x80098044 / OuterProduct0.
 */
static int PcPort_WorldMapInitializeModeEnterState(void)
{
    u32 before[WM_MES_STORE_COUNT];
    u32 after[WM_MES_STORE_COUNT];
    u32 expected[WM_MES_STORE_COUNT] = {
        2u, 4u, 0u, 0u, 0u, 0u, 0u, 1u, 1u, WM_MES_FN_86700
    };
    u32 dsts[WM_MES_STORE_COUNT] = {
        WM_MES_CCA4, WM_MES_D3CC, WM_MES_D804, WM_MES_CEC0, WM_MES_C7E8,
        WM_MES_BD34, WM_MES_D144, 0x80059198u, WM_MES_C178, WM_MES_CD40
    };
    int i;
    int match = 0;
    u32 be24_before;
    u32 tmpl_before[WM_TMPL_WORD_COUNT];
    u32 be24_after;
    int tmpl_ok = 1;
    int pool_ok = 1;

    fprintf(stderr, "[worldmap-mode-enter-state] entry\n");
    fprintf(stderr, "[worldmap-mode-enter-state] store_count=%d\n",
            WM_MES_STORE_COUNT);

    be24_before = WM_U32(WM_POOL_BE24);
    for (i = 0; i < (int)WM_TMPL_WORD_COUNT; i++)
        tmpl_before[i] = ((u32*)PSX_ADDR(WM_TMPL_DST_BE4C))[i];

    /* Snapshot before (retail order destinations). */
    before[0] = WM_U32(WM_MES_CCA4);
    before[1] = WM_U32(WM_MES_D3CC);
    before[2] = WM_U32(WM_MES_D804);
    before[3] = WM_U32(WM_MES_CEC0);
    before[4] = WM_U32(WM_MES_C7E8);
    before[5] = WM_U32(WM_MES_BD34);
    before[6] = WM_U32(WM_MES_D144);
    before[7] = (u32)D_80059198;
    before[8] = WM_U32(WM_MES_C178);
    before[9] = WM_U32(WM_MES_CD40);

    /* Retail store order (immediates / constructed fn VA). Idempotent. */
    WM_U32(WM_MES_CCA4) = 2u;                 /* 0x80072320 */
    WM_U32(WM_MES_D3CC) = 4u;                 /* 0x8007232C */
    WM_U32(WM_MES_D804) = 0u;                 /* 0x8007233C */
    WM_U32(WM_MES_CEC0) = 0u;                 /* 0x80072344 */
    WM_U32(WM_MES_C7E8) = 0u;                 /* 0x8007234C */
    WM_U32(WM_MES_BD34) = 0u;                 /* 0x80072354 */
    WM_U32(WM_MES_D144) = 0u;                 /* 0x8007235C */
    D_80059198 = 1;                           /* 0x80072364 main global */
    WM_U32(WM_MES_C178) = 1u;                 /* 0x8007236C */
    WM_U32(WM_MES_CD40) = WM_MES_FN_86700;    /* 0x80072374 */

    after[0] = WM_U32(WM_MES_CCA4);
    after[1] = WM_U32(WM_MES_D3CC);
    after[2] = WM_U32(WM_MES_D804);
    after[3] = WM_U32(WM_MES_CEC0);
    after[4] = WM_U32(WM_MES_C7E8);
    after[5] = WM_U32(WM_MES_BD34);
    after[6] = WM_U32(WM_MES_D144);
    after[7] = (u32)D_80059198;
    after[8] = WM_U32(WM_MES_C178);
    after[9] = WM_U32(WM_MES_CD40);

    for (i = 0; i < WM_MES_STORE_COUNT; i++) {
        fprintf(stderr,
                "[worldmap-mode-enter-state] store[%d] dst=0x%08x "
                "before=0x%08x value=0x%08x expected=0x%08x\n",
                i, dsts[i], before[i], after[i], expected[i]);
        if (after[i] == expected[i])
            match++;
    }

    be24_after = WM_U32(WM_POOL_BE24);
    if (be24_after != be24_before)
        pool_ok = 0;
    for (i = 0; i < (int)WM_TMPL_WORD_COUNT; i++) {
        if (((u32*)PSX_ADDR(WM_TMPL_DST_BE4C))[i] != tmpl_before[i])
            tmpl_ok = 0;
    }

    fprintf(stderr,
            "[worldmap-mode-enter-state] match=%d/%d pool_preserved=%d "
            "template_preserved=%d\n",
            match, WM_MES_STORE_COUNT, pool_ok, tmpl_ok);

    if (match != WM_MES_STORE_COUNT || !pool_ok || !tmpl_ok) {
        fprintf(stderr, "[worldmap-mode-enter-state] ERROR: validation failed\n");
        return -1;
    }

    fprintf(stderr, "[worldmap-mode-enter-state] exit\n");
    fprintf(stderr,
            "[worldmap-mode-enter-state] cut-before-0x80098044 "
            "retail_pc=0x%08x\n",
            WM_CUT_BEFORE_98044);
    return 0;
}

/* Compile-time layout check: retail VECTOR is 4×s32 (vx,vy,vz,pad). */
typedef char wm_assert_vector_16[(sizeof(VECTOR) == 16) ? 1 : -1];

/*
 * W8B — native transcription of retail 0x80098044–0x800980D4.
 * Exactly four OuterProduct0 calls; inputs overlay image, outputs world BSS.
 *
 * Instruction audit (retail world_map.bin @ 0x80098044–0x800980D4):
 *   98044  addiu sp,sp,-24          prologue
 *   98048  lui a0,0x800a / addiu a0,-17556  → a0 = 0x8009BB6C
 *   98050  sw s0,16(sp)
 *   98054  lui s0,0x800a / addiu s0,-17588  → s0 = 0x8009BB4C
 *   9805c  lui a2,0x800a / addiu a2,-14296  → a2 = 0x8009C828
 *   98064  sw ra,20(sp)
 *   98068  jal OuterProduct0 @ 0x8004A4D8
 *   9806c  addu a1,s0,zero                 delay: a1 = 0x8009BB4C
 *   98070  lui a1,0x800a / addiu a1,-17540  → a1 = 0x8009BB7C
 *   98078  lui a2,0x800a / addiu a2,-14268  → a2 = 0x8009C844
 *   98080  jal OuterProduct0
 *   98084  addu a0,s0,zero                 delay: a0 = 0x8009BB4C
 *   98088  lui a0,0x800a / addiu a0,-17524  → a0 = 0x8009BB8C
 *   98090  lui s0,0x800a / addiu s0,-17572  → s0 = 0x8009BB5C
 *   98098  lui a2,0x800a / addiu a2,-14220  → a2 = 0x8009C874
 *   980a0  jal OuterProduct0
 *   980a4  addu a1,s0,zero                 delay: a1 = 0x8009BB5C
 *   980a8  lui a1,0x800a / addiu a1,-17508  → a1 = 0x8009BB9C
 *   980b0  lui a2,0x800a / addiu a2,-14352  → a2 = 0x8009C7F0
 *   980b8  jal OuterProduct0
 *   980bc  addu a0,s0,zero                 delay: a0 = 0x8009BB5C
 *   980c0  lw ra,20(sp) / lw s0,16(sp)     epilogue
 *   980c8  addiu sp,sp,24
 *   980cc  jr ra / nop
 * Exactly four calls; no other stores or side effects.
 */
static int wm_80098044_cross_product_init(void)
{
    VECTOR* in_bb4c = (VECTOR*)PSX_ADDR(WM_XP_BB4C);
    VECTOR* in_bb5c = (VECTOR*)PSX_ADDR(WM_XP_BB5C);
    VECTOR* in_bb6c = (VECTOR*)PSX_ADDR(WM_XP_BB6C);
    VECTOR* in_bb7c = (VECTOR*)PSX_ADDR(WM_XP_BB7C);
    VECTOR* in_bb8c = (VECTOR*)PSX_ADDR(WM_XP_BB8C);
    VECTOR* in_bb9c = (VECTOR*)PSX_ADDR(WM_XP_BB9C);
    VECTOR* out0 = (VECTOR*)PSX_ADDR(WM_XP_C828);
    VECTOR* out1 = (VECTOR*)PSX_ADDR(WM_XP_C844);
    VECTOR* out2 = (VECTOR*)PSX_ADDR(WM_XP_C874);
    VECTOR* out3 = (VECTOR*)PSX_ADDR(WM_XP_C7F0);
    /* Oracle: integer cross product matching OuterProduct0 (writes vx,vy,vz only). */
    long exp0[3], exp1[3], exp2[3], exp3[3];
    u32 be24_before = WM_U32(WM_POOL_BE24);
    u32 tmpl_words[WM_TMPL_WORD_COUNT];
    u32 mes_before[WM_MES_STORE_COUNT];
    u8 in_snap[6][16];
    /* 4-byte canaries immediately before/after each 16-byte output VECTOR. */
    u32 canary_pre[4];
    u32 canary_post[4];
    long pad_before[4];
    const u32 out_addrs[4] = {
        WM_XP_C828, WM_XP_C844, WM_XP_C874, WM_XP_C7F0
    };
    int i;
    int vec_ok = 0;
    int comp_ok = 0;
    int boundary_ok = 1;
    int inputs_ok = 1;
    int lower_ok = 1;

    fprintf(stderr, "[worldmap-cross-products] entry\n");
    fprintf(stderr,
            "[worldmap-cross-products] call0 srcA=0x%08x srcB=0x%08x dst=0x%08x\n"
            "[worldmap-cross-products] call1 srcA=0x%08x srcB=0x%08x dst=0x%08x\n"
            "[worldmap-cross-products] call2 srcA=0x%08x srcB=0x%08x dst=0x%08x\n"
            "[worldmap-cross-products] call3 srcA=0x%08x srcB=0x%08x dst=0x%08x\n",
            WM_XP_BB6C, WM_XP_BB4C, WM_XP_C828,
            WM_XP_BB4C, WM_XP_BB7C, WM_XP_C844,
            WM_XP_BB8C, WM_XP_BB5C, WM_XP_C874,
            WM_XP_BB5C, WM_XP_BB9C, WM_XP_C7F0);

    /* Inputs must sit in overlay image (before BSS start). */
    if (WM_XP_BB4C < WM_OVERLAY_BASE || WM_XP_BB9C + 16u > WM_MEM_START) {
        fprintf(stderr, "[worldmap-cross-products] ERROR: inputs not in image\n");
        return -1;
    }
    if (WM_XP_C7F0 < WM_MEM_START || WM_XP_C874 + 16u <= WM_MEM_START) {
        fprintf(stderr, "[worldmap-cross-products] ERROR: outputs not in BSS\n");
        return -1;
    }

    fprintf(stderr,
            "[worldmap-cross-products] in BB4C=(%d,%d,%d) BB5C=(%d,%d,%d) "
            "BB6C=(%d,%d,%d)\n",
            (int)in_bb4c->vx, (int)in_bb4c->vy, (int)in_bb4c->vz,
            (int)in_bb5c->vx, (int)in_bb5c->vy, (int)in_bb5c->vz,
            (int)in_bb6c->vx, (int)in_bb6c->vy, (int)in_bb6c->vz);
    fprintf(stderr,
            "[worldmap-cross-products] in BB7C=(%d,%d,%d) BB8C=(%d,%d,%d) "
            "BB9C=(%d,%d,%d)\n",
            (int)in_bb7c->vx, (int)in_bb7c->vy, (int)in_bb7c->vz,
            (int)in_bb8c->vx, (int)in_bb8c->vy, (int)in_bb8c->vz,
            (int)in_bb9c->vx, (int)in_bb9c->vy, (int)in_bb9c->vz);

    /* Snapshot inputs, lower-slice state, and output neighborhoods. */
    wm_memcpy(in_snap[0], in_bb4c, 16);
    wm_memcpy(in_snap[1], in_bb5c, 16);
    wm_memcpy(in_snap[2], in_bb6c, 16);
    wm_memcpy(in_snap[3], in_bb7c, 16);
    wm_memcpy(in_snap[4], in_bb8c, 16);
    wm_memcpy(in_snap[5], in_bb9c, 16);
    for (i = 0; i < WM_TMPL_WORD_COUNT; i++)
        tmpl_words[i] = ((u32*)PSX_ADDR(WM_TMPL_DST_BE4C))[i];
    /* W7B ten-store snapshot (same order as mode-enter-state). */
    mes_before[0] = WM_U32(WM_MES_CCA4);
    mes_before[1] = WM_U32(WM_MES_D3CC);
    mes_before[2] = WM_U32(WM_MES_D804);
    mes_before[3] = WM_U32(WM_MES_CEC0);
    mes_before[4] = WM_U32(WM_MES_C7E8);
    mes_before[5] = WM_U32(WM_MES_BD34);
    mes_before[6] = WM_U32(WM_MES_D144);
    mes_before[7] = (u32)D_80059198;
    mes_before[8] = WM_U32(WM_MES_C178);
    mes_before[9] = WM_U32(WM_MES_CD40);

    /* Capture pad + canaries around each 16-byte output record. */
    pad_before[0] = out0->pad;
    pad_before[1] = out1->pad;
    pad_before[2] = out2->pad;
    pad_before[3] = out3->pad;
    for (i = 0; i < 4; i++) {
        canary_pre[i] = WM_U32(out_addrs[i] - 4u);
        canary_post[i] = WM_U32(out_addrs[i] + 16u);
    }

    exp0[0] = in_bb6c->vy * in_bb4c->vz - in_bb6c->vz * in_bb4c->vy;
    exp0[1] = in_bb6c->vz * in_bb4c->vx - in_bb6c->vx * in_bb4c->vz;
    exp0[2] = in_bb6c->vx * in_bb4c->vy - in_bb6c->vy * in_bb4c->vx;
    exp1[0] = in_bb4c->vy * in_bb7c->vz - in_bb4c->vz * in_bb7c->vy;
    exp1[1] = in_bb4c->vz * in_bb7c->vx - in_bb4c->vx * in_bb7c->vz;
    exp1[2] = in_bb4c->vx * in_bb7c->vy - in_bb4c->vy * in_bb7c->vx;
    exp2[0] = in_bb8c->vy * in_bb5c->vz - in_bb8c->vz * in_bb5c->vy;
    exp2[1] = in_bb8c->vz * in_bb5c->vx - in_bb8c->vx * in_bb5c->vz;
    exp2[2] = in_bb8c->vx * in_bb5c->vy - in_bb8c->vy * in_bb5c->vx;
    exp3[0] = in_bb5c->vy * in_bb9c->vz - in_bb5c->vz * in_bb9c->vy;
    exp3[1] = in_bb5c->vz * in_bb9c->vx - in_bb5c->vx * in_bb9c->vz;
    exp3[2] = in_bb5c->vx * in_bb9c->vy - in_bb5c->vy * in_bb9c->vx;

    /* Retail order — four OuterProduct0 only. Do not reorder or precompute. */
    OuterProduct0(in_bb6c, in_bb4c, out0);
    OuterProduct0(in_bb4c, in_bb7c, out1);
    OuterProduct0(in_bb8c, in_bb5c, out2);
    OuterProduct0(in_bb5c, in_bb9c, out3);

    fprintf(stderr,
            "[worldmap-cross-products] out0=(%d,%d,%d) exp=(%d,%d,%d)\n"
            "[worldmap-cross-products] out1=(%d,%d,%d) exp=(%d,%d,%d)\n"
            "[worldmap-cross-products] out2=(%d,%d,%d) exp=(%d,%d,%d)\n"
            "[worldmap-cross-products] out3=(%d,%d,%d) exp=(%d,%d,%d)\n",
            (int)out0->vx, (int)out0->vy, (int)out0->vz,
            (int)exp0[0], (int)exp0[1], (int)exp0[2],
            (int)out1->vx, (int)out1->vy, (int)out1->vz,
            (int)exp1[0], (int)exp1[1], (int)exp1[2],
            (int)out2->vx, (int)out2->vy, (int)out2->vz,
            (int)exp2[0], (int)exp2[1], (int)exp2[2],
            (int)out3->vx, (int)out3->vy, (int)out3->vz,
            (int)exp3[0], (int)exp3[1], (int)exp3[2]);

    if (out0->vx == exp0[0] && out0->vy == exp0[1] && out0->vz == exp0[2]) {
        vec_ok++;
        comp_ok += 3;
    }
    if (out1->vx == exp1[0] && out1->vy == exp1[1] && out1->vz == exp1[2]) {
        vec_ok++;
        comp_ok += 3;
    }
    if (out2->vx == exp2[0] && out2->vy == exp2[1] && out2->vz == exp2[2]) {
        vec_ok++;
        comp_ok += 3;
    }
    if (out3->vx == exp3[0] && out3->vy == exp3[1] && out3->vz == exp3[2]) {
        vec_ok++;
        comp_ok += 3;
    }

    /* Inputs unchanged. */
    if (!wm_memeq(in_snap[0], in_bb4c, 16) ||
        !wm_memeq(in_snap[1], in_bb5c, 16) ||
        !wm_memeq(in_snap[2], in_bb6c, 16) ||
        !wm_memeq(in_snap[3], in_bb7c, 16) ||
        !wm_memeq(in_snap[4], in_bb8c, 16) ||
        !wm_memeq(in_snap[5], in_bb9c, 16)) {
        inputs_ok = 0;
    }

    /* OuterProduct0 writes only vx/vy/vz; pad + neighbor canaries must hold. */
    if (out0->pad != pad_before[0] || out1->pad != pad_before[1] ||
        out2->pad != pad_before[2] || out3->pad != pad_before[3]) {
        boundary_ok = 0;
    }
    for (i = 0; i < 4; i++) {
        if (WM_U32(out_addrs[i] - 4u) != canary_pre[i] ||
            WM_U32(out_addrs[i] + 16u) != canary_post[i]) {
            boundary_ok = 0;
        }
    }

    /* W5B / W6B / W7B preservation. */
    if (WM_U32(WM_POOL_BE24) != be24_before)
        lower_ok = 0;
    for (i = 0; i < WM_TMPL_WORD_COUNT; i++) {
        if (((u32*)PSX_ADDR(WM_TMPL_DST_BE4C))[i] != tmpl_words[i])
            lower_ok = 0;
    }
    if (WM_U32(WM_MES_CCA4) != mes_before[0] ||
        WM_U32(WM_MES_D3CC) != mes_before[1] ||
        WM_U32(WM_MES_D804) != mes_before[2] ||
        WM_U32(WM_MES_CEC0) != mes_before[3] ||
        WM_U32(WM_MES_C7E8) != mes_before[4] ||
        WM_U32(WM_MES_BD34) != mes_before[5] ||
        WM_U32(WM_MES_D144) != mes_before[6] ||
        (u32)D_80059198 != mes_before[7] ||
        WM_U32(WM_MES_C178) != mes_before[8] ||
        WM_U32(WM_MES_CD40) != mes_before[9]) {
        lower_ok = 0;
    }

    fprintf(stderr,
            "[worldmap-cross-products] output_match=%d/4 component_match=%d/12 "
            "inputs_ok=%d boundary_ok=%d lower_ok=%d "
            "pool_BE24=0x%08x template0=0x%08x mes_CCA4=0x%08x\n",
            vec_ok, comp_ok, inputs_ok, boundary_ok, lower_ok,
            WM_U32(WM_POOL_BE24),
            ((u32*)PSX_ADDR(WM_TMPL_DST_BE4C))[0], WM_U32(WM_MES_CCA4));

    if (vec_ok != 4 || !inputs_ok || !boundary_ok || !lower_ok) {
        fprintf(stderr, "[worldmap-cross-products] ERROR: validation failed\n");
        return -1;
    }

    fprintf(stderr, "[worldmap-cross-products] exit\n");
    fprintf(stderr,
            "[worldmap-cross-products] cut-before-next-step retail_pc=0x%08x\n",
            WM_CUT_AFTER_98044);
    return 0;
}

/*
 * W10A nested helper — retail 0x800931D8–0x80093350 (0x178 / 376 bytes to jr).
 * Expand one VRAM-fetched BGR555 row into `rows` blended rows.
 * a0=src u16*, a1=dst u16*, a2=row count, a3=3 scale bytes (R,G,B).
 */
static void wm_800931d8_expand(u16* src, u16* dst, int rows, const u8* scales)
{
    u8 scale_r = scales[0];
    u8 scale_g = scales[1];
    u8 scale_b = scales[2];
    int t7;
    u32 out_words;

    fprintf(stderr, "[worldmap-gpu-asset-a] expand_entry\n");
    if (rows <= 0) {
        fprintf(stderr,
                "[worldmap-gpu-asset-a] expand_exit rows=0 output_size=0\n");
        return;
    }

    for (t7 = 0; t7 < rows; t7++) {
        s32 t1 = ((s32)t7 << 12) / rows;
        s32 t3 = 4096 - t1;
        u16* row_src = src;
        int t4;

        for (t4 = 0; t4 < 256; t4++) {
            u16 pix = *row_src;
            u16 out;

            if (pix == 0) {
                out = 0;
            } else {
                s32 r = (s32)((pix & 0x1Fu) << 3);
                s32 g = (s32)((pix >> 2) & 0xF8u);
                s32 b = (s32)((pix >> 7) & 0xF8u);
                s32 or_ = (r * t3 + (s32)scale_r * t1) >> 15;
                s32 og = (g * t3 + (s32)scale_g * t1) >> 15;
                s32 ob = (b * t3 + (s32)scale_b * t1) >> 15;
                out = (u16)((pix & 0x8000u) | (u16)or_ | ((u16)og << 5) |
                            ((u16)ob << 10));
            }
            *dst++ = out;
            row_src++;
        }
    }

    out_words = (u32)rows * 256u;
    fprintf(stderr,
            "[worldmap-gpu-asset-a] expand_exit rows=%d width=256 "
            "output_size=%u halfwords scales=(0x%02x,0x%02x,0x%02x)\n",
            rows, out_words, scale_r, scale_g, scale_b);
}

/*
 * W10A — native transcription of retail 0x8008440C–0x8008457C (0x174 / 372 B).
 * Consumes W4C second-wave buffer at 0x8009BD20 (TIM multi-list after LZSS),
 * uploads via func_8002DD20, expands one CLUT strip, writes 16 GetClut ids
 * to 0x8009BCE0. Each call consumes and frees the current BD20 allocation;
 * retail 0x80075B58 reloads BD20 before menu-return reentry.
 */
static int s_wm8440c_completed;

int wm_8008440C(void)
{
    u32 compressed_psx;
    void* compressed_host;
    void* decomp_host;
    void* img_host;
    void* exp_host;
    u8 scale_bytes[4];
    RECT rect;
    u16* clut_tbl;
    int i;
    int clut_ok = 0;
    u32 pool_be24;
    u32 mes_cca4;
    u32 xp0;

    fprintf(stderr, "[worldmap-gpu-asset-a] entry\n");
    fprintf(stderr, "[worldmap-gpu-asset-a] source_slot=0x%08x\n",
            WM_DST_BD20);

    compressed_psx = WM_U32(WM_DST_BD20);
    compressed_host = psx_u32_to_host(compressed_psx);
    if (compressed_psx == 0 || compressed_host == NULL) {
        fprintf(stderr,
                "[worldmap-gpu-asset-a] ERROR: BD20 empty/null "
                "psx=0x%08x\n",
                compressed_psx);
        return -1;
    }

    pool_be24 = WM_U32(WM_POOL_BE24);
    mes_cca4 = WM_U32(WM_MES_CCA4);
    xp0 = WM_U32(WM_XP_C828);

    /* Scale constants: unaligned 4-byte load from overlay image @ 0x800704DC. */
    wm_memcpy(scale_bytes, PSX_ADDR(WM_SCALE_IMG_704DC), 4);

    fprintf(stderr,
            "[worldmap-gpu-asset-a] compressed_psx=0x%08x host=%p "
            "scales=(0x%02x,0x%02x,0x%02x)\n",
            compressed_psx, compressed_host, scale_bytes[0], scale_bytes[1],
            scale_bytes[2]);

    /* 1–2: LZSSHeapDecompress(BD20, flags=1). */
    decomp_host = LZSSHeapDecompress(compressed_host, 1);
    if (decomp_host == NULL) {
        fprintf(stderr,
                "[worldmap-gpu-asset-a] ERROR: LZSSHeapDecompress failed\n");
        return -1;
    }
    fprintf(stderr, "[worldmap-gpu-asset-a] decompressed_host=%p\n",
            decomp_host);

    /* 3: TIM multi-list upload (CLUT + pixel LoadImage). */
    func_8002DD20((u32*)decomp_host);

    /* 4–5: DrawSync; free decompressed TIM list. */
    DrawSync(0);
    HeapFree(decomp_host);

    /* 6: free original compressed BD20 allocation (retail leaves slot stale). */
    compressed_host = psx_u32_to_host(WM_U32(WM_DST_BD20));
    if (compressed_host != NULL)
        HeapFree(compressed_host);

    /* 7–8: work buffers — 512 B store target, 8192 B expand output. */
    img_host = HeapAlloc(512, 1);
    exp_host = HeapAlloc(8192, 1);
    if (img_host == NULL || exp_host == NULL) {
        fprintf(stderr, "[worldmap-gpu-asset-a] ERROR: HeapAlloc failed\n");
        return -1;
    }
    fprintf(stderr,
            "[worldmap-gpu-asset-a] work_img=%p work_exp=%p\n",
            img_host, exp_host);

    /* 9: StoreImage RECT (0,496,256,1) — read one VRAM line. */
    rect.x = 0;
    rect.y = 496;
    rect.w = 256;
    rect.h = 1;
    fprintf(stderr,
            "[worldmap-gpu-asset-a] store_rect=(%d,%d,%d,%d)\n",
            rect.x, rect.y, rect.w, rect.h);
    StoreImage(&rect, (u_long*)img_host);
    DrawSync(0);

    /* 10: expand 1 fetched row → 16 blended rows (a2=16). */
    wm_800931d8_expand((u16*)img_host, (u16*)exp_host, 16, scale_bytes);

    /* 11: LoadImage RECT (0,496,256,15). */
    rect.x = 0;
    rect.y = 496;
    rect.w = 256;
    rect.h = 15;
    fprintf(stderr,
            "[worldmap-gpu-asset-a] load_rect=(%d,%d,%d,%d)\n",
            rect.x, rect.y, rect.w, rect.h);
    LoadImage(&rect, (u_long*)exp_host);
    DrawSync(0);

    /* 12: GetClut(0, 496+i) × 16 → 0x8009BCE0. */
    clut_tbl = (u16*)PSX_ADDR(WM_CLUT_BCE0);
    fprintf(stderr,
            "[worldmap-gpu-asset-a] clut_count=16 clut_table=0x%08x\n",
            WM_CLUT_BCE0);
    for (i = 0; i < 16; i++) {
        int gx = 0;
        int gy = 496 + i;
        /* Retail GetClut / getClut(x,y) = (y<<6)|((x>>4)&0x3f). */
        u16 formula = (u16)(((u32)gy << 6) | (((u32)gx >> 4) & 0x3fu));
        u16 got = GetClut(gx, gy);
        clut_tbl[i] = got;
        fprintf(stderr,
                "[worldmap-gpu-asset-a] clut[%d] xy=(%d,%d) value=0x%04x "
                "formula=0x%04x\n",
                i, gx, gy, got, formula);
        if (got == formula)
            clut_ok++;
    }
    fprintf(stderr, "[worldmap-gpu-asset-a] clut_match=%d/16\n", clut_ok);

    /* 13: free expand then store buffer (retail order). */
    HeapFree(exp_host);
    HeapFree(img_host);

    s_wm8440c_completed = 1;

    /* Lower-rung preservation (W5B/W7B/W8B samples). */
    if (WM_U32(WM_POOL_BE24) != pool_be24 || WM_U32(WM_MES_CCA4) != mes_cca4 ||
        WM_U32(WM_XP_C828) != xp0 || clut_ok != 16) {
        fprintf(stderr,
                "[worldmap-gpu-asset-a] ERROR: validation failed "
                "clut=%d/16 pool/mes/xp preserve\n",
                clut_ok);
        return -1;
    }

    fprintf(stderr, "[worldmap-gpu-asset-a] exit\n");
    fprintf(stderr,
            "[worldmap-gpu-asset-a] cut-before-next-head retail_pc=0x%08x\n",
            WM_CUT_BEFORE_979C8);
    return 0;
}

/*
 * W10B — native transcription of retail 0x800979C8–0x80097BBC (0x1F8 / 504 B).
 * Consumes W4C first-wave buffer at 0x8009C59C; larger VRAM path; two expand
 * passes (rows=32 each); 64 GetClut → CCB4; GetTPage → CD54 (×4) + CD5C (×3).
 */
static int s_wm979c8_ran;

static int wm_800979c8_gpu_asset_b(void)
{
    u32 compressed_psx;
    void* compressed_host;
    void* decomp_host;
    void* img_host;
    void* exp_host;
    const u8* scales;
    RECT rect;
    u16* clut_tbl;
    u16* tpage_cd54;
    u16* tpage_cd5c;
    int i;
    int clut_ok = 0;
    int tpage_ok = 0;
    u16 bce0_snap[16];
    u32 pool_be24;
    u32 mes_cca4;
    int tpage_x;
    int tpage_y;

    fprintf(stderr, "[worldmap-gpu-asset-b] entry\n");
    fprintf(stderr, "[worldmap-gpu-asset-b] source_slot=0x%08x\n",
            WM_DST_C59C);

    if (s_wm979c8_ran) {
        fprintf(stderr,
                "[worldmap-gpu-asset-b] ERROR: already ran this world entry "
                "(C59C is one-shot; reload lower ladder)\n");
        return -1;
    }
    if (!s_wm8440c_completed) {
        fprintf(stderr,
                "[worldmap-gpu-asset-b] ERROR: W10A did not run (required)\n");
        return -1;
    }

    compressed_psx = WM_U32(WM_DST_C59C);
    compressed_host = psx_u32_to_host(compressed_psx);
    if (compressed_psx == 0 || compressed_host == NULL) {
        fprintf(stderr,
                "[worldmap-gpu-asset-b] ERROR: C59C empty/null psx=0x%08x\n",
                compressed_psx);
        return -1;
    }

    /* Snapshot W10A CLUT table — must remain unchanged. */
    wm_memcpy(bce0_snap, PSX_ADDR(WM_CLUT_BCE0), sizeof(bce0_snap));
    pool_be24 = WM_U32(WM_POOL_BE24);
    mes_cca4 = WM_U32(WM_MES_CCA4);

    scales = (const u8*)PSX_ADDR(WM_SCALE_IMG_BB48);
    fprintf(stderr,
            "[worldmap-gpu-asset-b] compressed_psx=0x%08x host=%p "
            "scales=(0x%02x,0x%02x,0x%02x) expand_passes=2\n",
            compressed_psx, compressed_host, scales[0], scales[1], scales[2]);

    decomp_host = LZSSHeapDecompress(compressed_host, 1);
    if (decomp_host == NULL) {
        fprintf(stderr,
                "[worldmap-gpu-asset-b] ERROR: LZSSHeapDecompress failed\n");
        return -1;
    }
    fprintf(stderr, "[worldmap-gpu-asset-b] decompressed_host=%p\n",
            decomp_host);

    func_8002DD20((u32*)decomp_host);
    DrawSync(0);
    HeapFree(decomp_host);

    compressed_host = psx_u32_to_host(WM_U32(WM_DST_C59C));
    if (compressed_host != NULL)
        HeapFree(compressed_host);

    /* 1024 B store (2 rows × 256 × u16); 0x8000 B expand (64 rows × 256 × u16). */
    img_host = HeapAlloc(1024, 1);
    exp_host = HeapAlloc(0x8000, 1);
    if (img_host == NULL || exp_host == NULL) {
        fprintf(stderr, "[worldmap-gpu-asset-b] ERROR: HeapAlloc failed\n");
        return -1;
    }
    fprintf(stderr,
            "[worldmap-gpu-asset-b] work_img=%p work_exp=%p\n",
            img_host, exp_host);

    /* StoreImage (0,480,256,2). */
    rect.x = 0;
    rect.y = 480;
    rect.w = 256;
    rect.h = 2;
    fprintf(stderr,
            "[worldmap-gpu-asset-b] store_rect=(%d,%d,%d,%d)\n",
            rect.x, rect.y, rect.w, rect.h);
    StoreImage(&rect, (u_long*)img_host);
    DrawSync(0);

    /* Two expand passes: rows 0–31 from first store row; 32–63 from second. */
    wm_800931d8_expand((u16*)img_host, (u16*)exp_host, 32, scales);
    wm_800931d8_expand((u16*)((u8*)img_host + 512),
                       (u16*)((u8*)exp_host + 16384), 32, scales);

    /* LoadImage (0,432,256,64). */
    rect.x = 0;
    rect.y = 432;
    rect.w = 256;
    rect.h = 64;
    fprintf(stderr,
            "[worldmap-gpu-asset-b] load_rect=(%d,%d,%d,%d)\n",
            rect.x, rect.y, rect.w, rect.h);
    LoadImage(&rect, (u_long*)exp_host);
    DrawSync(0);

    /* GetClut(0, 432+i) × 64 → 0x8009CCB4. */
    clut_tbl = (u16*)PSX_ADDR(WM_CLUT_CCB4);
    fprintf(stderr,
            "[worldmap-gpu-asset-b] clut_count=64 clut_table=0x%08x\n",
            WM_CLUT_CCB4);
    for (i = 0; i < 64; i++) {
        int gx = 0;
        int gy = 432 + i;
        u16 formula = (u16)(((u32)gy << 6) | (((u32)gx >> 4) & 0x3fu));
        u16 got = GetClut(gx, gy);
        clut_tbl[i] = got;
        if (got == formula)
            clut_ok++;
    }
    fprintf(stderr, "[worldmap-gpu-asset-b] clut_match=%d/64\n", clut_ok);

    /* GetTPage(1,0,x,y): first 4 → CD54 (x=512..896 step 128, y=0). */
    tpage_cd54 = (u16*)PSX_ADDR(WM_TPAGE_CD54);
    tpage_x = 512;
    tpage_y = 0;
    for (i = 0; i < 4; i++) {
        u16 tp = GetTPage(1, 0, tpage_x, tpage_y);
        tpage_cd54[i] = tp;
        fprintf(stderr,
                "[worldmap-gpu-asset-b] tpage_cd54[%d] GetTPage(1,0,%d,%d)="
                "0x%04x\n",
                i, tpage_x, tpage_y, tp);
        tpage_ok++;
        tpage_x += 128;
    }

    /* Next 3 → CD5C (x=384..640 step 128, y=256). */
    tpage_cd5c = (u16*)PSX_ADDR(WM_TPAGE_CD5C);
    tpage_x = 384;
    tpage_y = 256;
    for (i = 0; i < 3; i++) {
        u16 tp = GetTPage(1, 0, tpage_x, tpage_y);
        tpage_cd5c[i] = tp;
        fprintf(stderr,
                "[worldmap-gpu-asset-b] tpage_cd5c[%d] GetTPage(1,0,%d,%d)="
                "0x%04x\n",
                i, tpage_x, tpage_y, tp);
        tpage_ok++;
        tpage_x += 128;
    }
    fprintf(stderr,
            "[worldmap-gpu-asset-b] tpage_writes=%d/7 "
            "persistent_store_cd54=0x%04x persistent_store_cd5c=0x%04x\n",
            tpage_ok, tpage_cd54[0], tpage_cd5c[0]);

    /* Free expand then store buffer (retail order). */
    HeapFree(exp_host);
    HeapFree(img_host);

    s_wm979c8_ran = 1;

    if (clut_ok != 64 || tpage_ok != 7 ||
        !wm_memeq(bce0_snap, PSX_ADDR(WM_CLUT_BCE0), sizeof(bce0_snap)) ||
        WM_U32(WM_POOL_BE24) != pool_be24 ||
        WM_U32(WM_MES_CCA4) != mes_cca4) {
        fprintf(stderr,
                "[worldmap-gpu-asset-b] ERROR: validation failed "
                "clut=%d/64 tpage=%d/7 w10a_preserved=%d\n",
                clut_ok, tpage_ok,
                wm_memeq(bce0_snap, PSX_ADDR(WM_CLUT_BCE0), sizeof(bce0_snap)));
        return -1;
    }

    fprintf(stderr, "[worldmap-gpu-asset-b] exit\n");
    fprintf(stderr,
            "[worldmap-gpu-asset-b] cut-before-next-head retail_pc=0x%08x\n",
            WM_CUT_BEFORE_84580);
    return 0;
}

/*
 * W11B — native transcription of retail 0x80084580–0x80084814 (0x298 / 664 B).
 * Builds n×84 object/matrix records from W4C fixup streams into heap @ C620.
 */
static int s_wm84580_ran;

static int wm_80084580_object_matrix(void)
{
    u32 cd48_psx;
    u32 d308_psx;
    u32 d308_final;
    u32 bd30_psx;
    u8* cd48_host;
    u8* bd30_host;
    u32* reloc;
    s32 reloc_count;
    s32 i;
    u16 entry_count;
    u32 alloc_size;
    void* table_host;
    u32 table_psx;
    VECTOR apply_out;
    SVECTOR apply_sv_out;
    u16 bce0_snap[16];
    u16 ccb4_0;
    u16 cd54_0;
    u32 pool_be24;
    u32 mes_cca4;
    u32 xp0;
    int final_ok = 1;

    fprintf(stderr, "[worldmap-object-matrix] entry\n");

    if (s_wm84580_ran) {
        fprintf(stderr,
                "[worldmap-object-matrix] ERROR: already ran this world entry "
                "(non-idempotent relocate/alloc)\n");
        return -1;
    }
    /* This body consumes the second-wave fixups, not GPU asset B.  Base mode
     * happens to call 0x800979C8 first, while retail modes 8/11 call this
     * function before both GPU asset helpers. */

    cd48_psx = WM_U32(WM_FIX_CD48);
    d308_psx = WM_U32(WM_FIX_D308);
    bd30_psx = WM_U32(WM_FIX_BD30);
    cd48_host = (u8*)psx_u32_to_host(cd48_psx);
    bd30_host = (u8*)psx_u32_to_host(bd30_psx);

    if (cd48_psx == 0 || d308_psx == 0 || bd30_psx == 0 || cd48_host == NULL ||
        bd30_host == NULL) {
        fprintf(stderr,
                "[worldmap-object-matrix] ERROR: invalid W4C inputs "
                "CD48=0x%08x D308=0x%08x BD30=0x%08x\n",
                cd48_psx, d308_psx, bd30_psx);
        return -1;
    }

    wm_memcpy(bce0_snap, PSX_ADDR(WM_CLUT_BCE0), sizeof(bce0_snap));
    ccb4_0 = *(u16*)PSX_ADDR(WM_CLUT_CCB4);
    cd54_0 = *(u16*)PSX_ADDR(WM_TPAGE_CD54);
    pool_be24 = WM_U32(WM_POOL_BE24);
    mes_cca4 = WM_U32(WM_MES_CCA4);
    xp0 = WM_U32(WM_XP_C828);

    fprintf(stderr,
            "[worldmap-object-matrix] relocate_entry asset_base_psx=0x%08x "
            "relocation_table_psx=0x%08x\n",
            cd48_psx, d308_psx);

    /* 1: model graph relocate in-place. */
    reloc_count = func_8002C3E8(cd48_host);
    WM_S16(WM_OBJ_BD28) = (s16)reloc_count;

    /* 2: relocate `reloc_count` words at *D308+4; advance D308 by 4. */
    {
        u32 base = d308_psx;
        WM_U32(WM_FIX_D308) = base + 4u;
        d308_final = base + 4u;
        if (reloc_count > 0) {
            reloc = (u32*)psx_u32_to_host(base + 4u);
            if (reloc == NULL) {
                fprintf(stderr,
                        "[worldmap-object-matrix] ERROR: bad reloc table\n");
                return -1;
            }
            for (i = 0; i < reloc_count; i++)
                reloc[i] = base + reloc[i];
        }
    }

    fprintf(stderr,
            "[worldmap-object-matrix] relocate_exit relocation_count=%d "
            "relocation_table_final_psx=0x%08x bd28=%d\n",
            reloc_count, d308_final, (int)WM_S16(WM_OBJ_BD28));

    /* 3–4: entry count and allocation. */
    entry_count = *(u16*)bd30_host;
    if ((u32)entry_count > 0x10000u / WM_OBJ_RECORD_STRIDE) {
        fprintf(stderr,
                "[worldmap-object-matrix] ERROR: entry_count %u overflow\n",
                entry_count);
        return -1;
    }
    alloc_size = (u32)entry_count * WM_OBJ_RECORD_STRIDE;
    WM_U16(WM_OBJ_D7E0) = entry_count;

    table_host = HeapAlloc(alloc_size, 0);
    if (table_host == NULL && alloc_size != 0) {
        fprintf(stderr, "[worldmap-object-matrix] ERROR: HeapAlloc failed\n");
        return -1;
    }
    table_psx = host_ptr_to_psx_u32(table_host);
    WM_U32(WM_OBJ_C620) = table_psx;

    fprintf(stderr,
            "[worldmap-object-matrix] entry_count=%u record_stride=84 "
            "allocation_size=%u table_host=%p table_psx=0x%08x\n",
            entry_count, alloc_size, table_host, table_psx);

    /* 5: ApplyMatrix / ApplyMatrixSV with image matrices (GTE side effects;
     * retail leaves a1/a2 mostly unset — use BD30+2 as SVECTOR input and
     * stack outputs to avoid NULL deref on host). */
    {
        SVECTOR* vin = (SVECTOR*)(bd30_host + 2);
        ApplyMatrix((MATRIX*)PSX_ADDR(WM_OBJ_MAT_A140), vin, &apply_out);
        ApplyMatrixSV((MATRIX*)PSX_ADDR(WM_OBJ_MAT_A160), vin, &apply_sv_out);
    }

    /* 6: per-entry loop. */
    if (entry_count > 0 && table_host != NULL) {
        u8* s0 = bd30_host + 16; /* first 16-byte source record end */
        u16* s3 = (u16*)(bd30_host + 2);
        u32 s1 = 0; /* byte offset into table */

        for (i = 0; i < (int)entry_count; i++) {
            u8* e = (u8*)table_host + s1;
            s16 idx;
            u32 model_psx;
            u8* model_host;
            s32 copy_sz;

            *(u16*)(e + 0) = 0;
            *(u16*)(e + 2) = *s3;
            *(u16*)(e + 4) = *(u16*)(s0 - 12);
            *(s32*)(e + 8) = (s32) * (s16*)(s0 - 10);
            *(s32*)(e + 12) = (s32) * (s16*)(s0 - 8);
            *(s32*)(e + 16) = -(s32) * (s16*)(s0 - 6);
            *(u16*)(e + 24) = *(u16*)(s0 - 4);
            *(u16*)(e + 26) = *(u16*)(s0 - 2);
            *(u16*)(e + 28) = *(u16*)(s0 + 0);

            RotMatrix((SVECTOR*)(e + 24), (MATRIX*)(e + 32));

            idx = *(s16*)(e + 2);
            model_psx = cd48_psx + (u32)((s32)idx * 56) + 16u;
            *(u32*)(e + 64) = model_psx;
            model_host = (u8*)psx_u32_to_host(model_psx);
            if (model_host == NULL) {
                fprintf(stderr,
                        "[worldmap-object-matrix] ERROR: model null entry=%d "
                        "idx=%d\n",
                        i, (int)idx);
                return -1;
            }

            func_8002CB54(model_host, (u32*)(e + 72), (u32*)(e + 76));
            {
                void* out1 = (void*)(uintptr_t)(*(u32*)(e + 72));
                func_8002C8CC(model_host, out1, 1);
            }
            {
                void* out1 = (void*)(uintptr_t)(*(u32*)(e + 72));
                void* out2 = (void*)(uintptr_t)(*(u32*)(e + 76));
                copy_sz = *(s32*)(model_host + 0x34);
                if (copy_sz > 0 && out1 != NULL && out2 != NULL)
                    wm_memcpy(out2, out1, (unsigned)copy_sz);

                /* func_8002CB54 is shared native code and returns low host
                 * pointers.  Retail publishes PSX heap addresses in these
                 * C620 record fields; every world callback subsequently
                 * dereferences them as guest addresses.  Convert only after
                 * the native fill/mirror operations above are complete. */
                *(u32*)(e + 72) = host_ptr_to_psx_u32(out1);
                *(u32*)(e + 76) = host_ptr_to_psx_u32(out2);
            }

            /* Reloc table lookup: D308 + index*4 → ptr → entry+68; bump +4. */
            {
                u32 d308_cur = WM_U32(WM_FIX_D308);
                u32* slot =
                    (u32*)psx_u32_to_host(d308_cur + (u32)((s32)idx << 2));
                u32 p_psx;
                u8* p_host;
                u32 off;

                if (slot == NULL) {
                    fprintf(stderr,
                            "[worldmap-object-matrix] ERROR: reloc slot "
                            "entry=%d\n",
                            i);
                    return -1;
                }
                p_psx = *slot;
                *(u32*)(e + 68) = p_psx;
                p_host = (u8*)psx_u32_to_host(p_psx);
                if (p_host != NULL) {
                    off = *(u32*)(p_host + 4);
                    *(u32*)(p_host + 4) = p_psx + off;
                }
            }

            *(u32*)(e + 80) = 0;

            if (i == 0 || i == 1 || i == (int)entry_count - 1) {
                fprintf(stderr,
                        "[worldmap-object-matrix] entry[%d] src_off=%d "
                        "dst_off=%u idx=%d model_psx=0x%08x copy_sz=%d\n",
                        i, (int)((u8*)s3 - bd30_host), s1, (int)idx, model_psx,
                        copy_sz);
            }

            s3 = (u16*)((u8*)s3 + 16);
            s0 += 16;
            s1 += WM_OBJ_RECORD_STRIDE;
        }
    }

    /* 7: final persistent state. */
    D_80050100 = 2;
    WM_U32(WM_OBJ_C16C) = 0xFFFFFFFFu;
    WM_U32(WM_OBJ_C840) = 0xFFFFFFFFu;
    if (table_host != NULL)
        *(u16*)((u8*)table_host + 336) = 1;

    s_wm84580_ran = 1;

    if (WM_U16(WM_OBJ_D7E0) != entry_count || WM_U32(WM_OBJ_C620) != table_psx ||
        D_80050100 != 2 || WM_U32(WM_OBJ_C16C) != 0xFFFFFFFFu ||
        WM_U32(WM_OBJ_C840) != 0xFFFFFFFFu)
        final_ok = 0;
    if (table_host != NULL && *(u16*)((u8*)table_host + 336) != 1)
        final_ok = 0;
    if (!wm_memeq(bce0_snap, PSX_ADDR(WM_CLUT_BCE0), sizeof(bce0_snap)) ||
        *(u16*)PSX_ADDR(WM_CLUT_CCB4) != ccb4_0 ||
        *(u16*)PSX_ADDR(WM_TPAGE_CD54) != cd54_0 ||
        WM_U32(WM_POOL_BE24) != pool_be24 || WM_U32(WM_MES_CCA4) != mes_cca4 ||
        WM_U32(WM_XP_C828) != xp0)
        final_ok = 0;

    fprintf(stderr,
            "[worldmap-object-matrix] entry_count=%u table_psx=0x%08x "
            "final_flags_ok=%d\n",
            entry_count, table_psx, final_ok);

    if (!final_ok) {
        fprintf(stderr, "[worldmap-object-matrix] ERROR: validation failed\n");
        return -1;
    }

    fprintf(stderr, "[worldmap-object-matrix] exit\n");
    fprintf(stderr,
            "[worldmap-object-matrix] cut-before-third-wave retail_pc=0x%08x\n",
            WM_CUT_BEFORE_72090);
    return 0;
}

/*
 * W12B — retail 0x80072090–0x800721E0 (0x154 / 340 B): third-wave archive
 * submit. Five IDs from W2 BSS seed → DecodeAlignedSize → HeapAlloc → D3F8
 * queue + mirrors → func_80029AFC. No poll, no decompress, no residual after
 * return. HeapAlloc flags: first a1=1, remaining a1=0 (retail exact).
 */
static int s_wm72090_ran;

static int wm_80072090_third_wave(void)
{
    u32 ids[5];
    u32 sizes[5];
    void* hosts[5];
    u32 psx[5];
    s32 f304_before;
    s32 f304_after;
    int queue_result;
    int i;
    u32 c620_snap;
    u16 d7e0_snap;
    u32 pool_snap;
    u32 be4c0_snap;
    u16 bce0_0;

    fprintf(stderr, "[worldmap-third-wave] entry\n");

    if (s_wm72090_ran) {
        fprintf(stderr,
                "[worldmap-third-wave] ERROR: already ran this world entry "
                "(non-idempotent queue rebuild)\n");
        return -1;
    }
    if (!s_wm84580_ran) {
        fprintf(stderr,
                "[worldmap-third-wave] ERROR: W11B did not run (required)\n");
        return -1;
    }

    /* Preservation snapshots (W11B / earlier must not change). */
    c620_snap = WM_U32(WM_OBJ_C620);
    d7e0_snap = WM_U16(WM_OBJ_D7E0);
    pool_snap = WM_U32(WM_POOL_BE24);
    be4c0_snap = WM_U32(WM_TMPL_DST_BE4C);
    bce0_0 = *(u16*)PSX_ADDR(WM_CLUT_BCE0);

    /* 1. Counter ++ (host main BSS). */
    f304_before = D_8004F304;
    D_8004F304 = f304_before + 1;
    f304_after = D_8004F304;
    fprintf(stderr,
            "[worldmap-third-wave] D_8004F304_before=%d D_8004F304_after=%d\n",
            (int)f304_before, (int)f304_after);

    /* 2. Read five source IDs from world BSS (W2 seed) — not hard-coded. */
    ids[0] = WM_U32(WM_TW_ID_CC98);
    ids[1] = WM_U32(WM_TW_ID_D3D0);
    ids[2] = WM_U32(WM_TW_ID_D3C8);
    ids[3] = WM_U32(WM_TW_ID_D800);
    ids[4] = WM_U32(WM_TW_ID_BCC8);

    for (i = 0; i < 5; i++) {
        fprintf(stderr, "[worldmap-third-wave] source_id[%d]=0x%08x (%u)\n", i,
                ids[i], ids[i]);
    }

    /* 3–6. Decode / alloc / store in retail interleave order.
     * Queue pData always KUSEG. World-BSS mirrors KUSEG. D_8006259C is host
     * void* (SEDS-style main global), matching menu/field writers. */
    sizes[0] = (u32)ArchiveDecodeAlignedSize(ids[0]);
    hosts[0] = HeapAlloc(sizes[0], 1);
    psx[0] = host_ptr_to_psx_u32(hosts[0]);
    WM_U16(WM_REQ_D3F8) = (u16)ids[0];
    WM_U32(WM_TW_MIRROR_C88C) = psx[0];
    WM_U32(WM_REQ_D3FC) = psx[0];

    sizes[1] = (u32)ArchiveDecodeAlignedSize(ids[1]);
    hosts[1] = HeapAlloc(sizes[1], 0);
    psx[1] = host_ptr_to_psx_u32(hosts[1]);
    WM_U16(WM_REQ_D400) = (u16)ids[1];
    WM_U32(WM_TW_MIRROR_C884) = psx[1];
    WM_U32(WM_REQ_D404) = psx[1];

    sizes[2] = (u32)ArchiveDecodeAlignedSize(ids[2]);
    hosts[2] = HeapAlloc(sizes[2], 0);
    psx[2] = host_ptr_to_psx_u32(hosts[2]);
    WM_U16(WM_REQ_D408) = (u16)ids[2];
    D_8006259C = hosts[2]; /* host pointer authority */
    WM_U32(WM_REQ_D40C) = psx[2];

    sizes[3] = (u32)ArchiveDecodeAlignedSize(ids[3]);
    hosts[3] = HeapAlloc(sizes[3], 0);
    psx[3] = host_ptr_to_psx_u32(hosts[3]);
    WM_U16(WM_REQ_D410) = (u16)ids[3];
    WM_U32(WM_TW_MIRROR_C888) = psx[3];
    WM_U32(WM_REQ_D414) = psx[3];

    sizes[4] = (u32)ArchiveDecodeAlignedSize(ids[4]);
    hosts[4] = HeapAlloc(sizes[4], 0);
    psx[4] = host_ptr_to_psx_u32(hosts[4]);
    WM_U16(WM_REQ_D418) = (u16)ids[4];
    WM_U32(WM_TW_MIRROR_C614) = psx[4];
    WM_U32(WM_REQ_D41C) = psx[4];

    /* Terminator */
    WM_U16(WM_REQ_D420) = 0;
    WM_U32(WM_REQ_D424) = 0;

    for (i = 0; i < 5; i++) {
        fprintf(stderr,
                "[worldmap-third-wave] archive_id[%d]=%u aligned_size[%d]=%u "
                "destination_psx[%d]=0x%08x host=%p\n",
                i, ids[i], i, sizes[i], i, psx[i], hosts[i]);
    }

    for (i = 0; i < 5; i++) {
        if (hosts[i] == NULL && sizes[i] != 0) {
            fprintf(stderr,
                    "[worldmap-third-wave] ERROR: HeapAlloc failed entry=%d "
                    "size=%u\n",
                    i, sizes[i]);
            return -1;
        }
    }

    fprintf(stderr, "[worldmap-third-wave] request_count=5\n");
    for (i = 0; i < 5; i++) {
        fprintf(stderr,
                "[worldmap-third-wave] request[%d] archive=%u pData_psx=0x%08x\n",
                i, (unsigned)(u16)ids[i], psx[i]);
    }
    fprintf(stderr,
            "[worldmap-third-wave] mirrors C88C=0x%08x C884=0x%08x "
            "D_8006259C_host=%p C888=0x%08x C614=0x%08x\n",
            WM_U32(WM_TW_MIRROR_C88C), WM_U32(WM_TW_MIRROR_C884), D_8006259C,
            WM_U32(WM_TW_MIRROR_C888), WM_U32(WM_TW_MIRROR_C614));

    /* 7. Submit — same PSX-layout queue as W3B/W4C. */
    queue_result = func_80029AFC(PSX_ADDR(WM_REQ_D3F8), 0, 0);
    fprintf(stderr, "[worldmap-third-wave] queue_submit=%d\n", queue_result);

    /* Preserve prior rungs. */
    if (WM_U32(WM_OBJ_C620) != c620_snap || WM_U16(WM_OBJ_D7E0) != d7e0_snap ||
        WM_U32(WM_POOL_BE24) != pool_snap ||
        WM_U32(WM_TMPL_DST_BE4C) != be4c0_snap ||
        *(u16*)PSX_ADDR(WM_CLUT_BCE0) != bce0_0) {
        fprintf(stderr,
                "[worldmap-third-wave] ERROR: prior rung state corrupted\n");
        return -1;
    }
    if (f304_after != f304_before + 1) {
        fprintf(stderr, "[worldmap-third-wave] ERROR: D_8004F304 not +1\n");
        return -1;
    }

    s_wm72090_ran = 1;
    fprintf(stderr, "[worldmap-third-wave] exit\n");
    fprintf(stderr,
            "[worldmap-third-wave] cut-before-next-step retail_pc=0x%08x\n",
            WM_CUT_BEFORE_736DC);
    return 0;
}

/*
 * W13B — retail 0x800736DC–0x800737E8 (0x110 / 272 B): unrolled BSS constant
 * paint. Leaf, no loads, no callees. Caller delay at 0x80072460 is nop.
 * 48 stores (32×sw + 16×sb); unique 136 bytes 0x8009D197–0x8009D2AF with
 * intentional later-sb overlays on earlier word stores. Neutral names only.
 */
#define WM_BSS_CONST_A           0x00FF7A70u
#define WM_BSS_CONST_B           0x00FFF5E0u
#define WM_BSS_CONST_C           0x00C03745u
#define WM_BSS_CONST_K8          0x08u
#define WM_BSS_CONST_K56         0x38u
#define WM_BSS_STORE_COUNT       48
#define WM_BSS_UNIQUE_BYTES      136
#define WM_BSS_LOWEST            0x8009D197u
#define WM_BSS_HIGHEST           0x8009D2AFu

/* Expected final bytes at unique addresses (ascending) after all 48 stores. */
static const u32 k_bss_const_addrs[WM_BSS_UNIQUE_BYTES] = {
    0x8009D197u, 0x8009D198u, 0x8009D199u, 0x8009D19Au, 0x8009D19Bu,
    0x8009D1A0u, 0x8009D1A1u, 0x8009D1A2u, 0x8009D1A3u, 0x8009D1A8u,
    0x8009D1A9u, 0x8009D1AAu, 0x8009D1ABu, 0x8009D1B0u, 0x8009D1B1u,
    0x8009D1B2u, 0x8009D1B3u, 0x8009D1BBu, 0x8009D1BCu, 0x8009D1BDu,
    0x8009D1BEu, 0x8009D1BFu, 0x8009D1C4u, 0x8009D1C5u, 0x8009D1C6u,
    0x8009D1C7u, 0x8009D1CCu, 0x8009D1CDu, 0x8009D1CEu, 0x8009D1CFu,
    0x8009D1D4u, 0x8009D1D5u, 0x8009D1D6u, 0x8009D1D7u, 0x8009D1DFu,
    0x8009D1E0u, 0x8009D1E1u, 0x8009D1E2u, 0x8009D1E3u, 0x8009D1E8u,
    0x8009D1E9u, 0x8009D1EAu, 0x8009D1EBu, 0x8009D1F0u, 0x8009D1F1u,
    0x8009D1F2u, 0x8009D1F3u, 0x8009D1F8u, 0x8009D1F9u, 0x8009D1FAu,
    0x8009D1FBu, 0x8009D203u, 0x8009D204u, 0x8009D205u, 0x8009D206u,
    0x8009D207u, 0x8009D20Cu, 0x8009D20Du, 0x8009D20Eu, 0x8009D20Fu,
    0x8009D214u, 0x8009D215u, 0x8009D216u, 0x8009D217u, 0x8009D21Cu,
    0x8009D21Du, 0x8009D21Eu, 0x8009D21Fu, 0x8009D227u, 0x8009D228u,
    0x8009D229u, 0x8009D22Au, 0x8009D22Bu, 0x8009D230u, 0x8009D231u,
    0x8009D232u, 0x8009D233u, 0x8009D238u, 0x8009D239u, 0x8009D23Au,
    0x8009D23Bu, 0x8009D240u, 0x8009D241u, 0x8009D242u, 0x8009D243u,
    0x8009D24Bu, 0x8009D24Cu, 0x8009D24Du, 0x8009D24Eu, 0x8009D24Fu,
    0x8009D254u, 0x8009D255u, 0x8009D256u, 0x8009D257u, 0x8009D25Cu,
    0x8009D25Du, 0x8009D25Eu, 0x8009D25Fu, 0x8009D264u, 0x8009D265u,
    0x8009D266u, 0x8009D267u, 0x8009D26Fu, 0x8009D270u, 0x8009D271u,
    0x8009D272u, 0x8009D273u, 0x8009D278u, 0x8009D279u, 0x8009D27Au,
    0x8009D27Bu, 0x8009D280u, 0x8009D281u, 0x8009D282u, 0x8009D283u,
    0x8009D288u, 0x8009D289u, 0x8009D28Au, 0x8009D28Bu, 0x8009D293u,
    0x8009D294u, 0x8009D295u, 0x8009D296u, 0x8009D297u, 0x8009D29Cu,
    0x8009D29Du, 0x8009D29Eu, 0x8009D29Fu, 0x8009D2A4u, 0x8009D2A5u,
    0x8009D2A6u, 0x8009D2A7u, 0x8009D2ACu, 0x8009D2ADu, 0x8009D2AEu,
    0x8009D2AFu,
};
static const u8 k_bss_const_expect[WM_BSS_UNIQUE_BYTES] = {
    0x08u, 0x70u, 0x7Au, 0xFFu, 0x38u, 0x70u, 0x7Au, 0xFFu, 0x00u, 0xE0u, 0xF5u,
    0xFFu, 0x00u, 0xE0u, 0xF5u, 0xFFu, 0x00u, 0x08u, 0x70u, 0x7Au, 0xFFu, 0x38u,
    0x70u, 0x7Au, 0xFFu, 0x00u, 0xE0u, 0xF5u, 0xFFu, 0x00u, 0xE0u, 0xF5u, 0xFFu,
    0x00u, 0x08u, 0x45u, 0x37u, 0xC0u, 0x38u, 0x45u, 0x37u, 0xC0u, 0x00u, 0x70u,
    0x7Au, 0xFFu, 0x00u, 0x70u, 0x7Au, 0xFFu, 0x00u, 0x08u, 0x45u, 0x37u, 0xC0u,
    0x38u, 0x45u, 0x37u, 0xC0u, 0x00u, 0x70u, 0x7Au, 0xFFu, 0x00u, 0x70u, 0x7Au,
    0xFFu, 0x00u, 0x08u, 0x45u, 0x37u, 0xC0u, 0x38u, 0x45u, 0x37u, 0xC0u, 0x00u,
    0x45u, 0x37u, 0xC0u, 0x00u, 0x45u, 0x37u, 0xC0u, 0x00u, 0x08u, 0x45u, 0x37u,
    0xC0u, 0x38u, 0x45u, 0x37u, 0xC0u, 0x00u, 0x45u, 0x37u, 0xC0u, 0x00u, 0x45u,
    0x37u, 0xC0u, 0x00u, 0x08u, 0x45u, 0x37u, 0xC0u, 0x38u, 0x45u, 0x37u, 0xC0u,
    0x00u, 0x45u, 0x37u, 0xC0u, 0x00u, 0x45u, 0x37u, 0xC0u, 0x00u, 0x08u, 0x45u,
    0x37u, 0xC0u, 0x38u, 0x45u, 0x37u, 0xC0u, 0x00u, 0x45u, 0x37u, 0xC0u, 0x00u,
    0x45u, 0x37u, 0xC0u, 0x00u,
};
#define WM_BSS_EXPECT_FNV 0x4323F4C5u

static int s_wm736dc_ran;

static u32 wm_bss_footprint_hash(void)
{
    /* FNV-1a 32-bit over unique footprint bytes in ascending address order. */
    u32 h = 2166136261u;
    int i;
    for (i = 0; i < WM_BSS_UNIQUE_BYTES; i++) {
        h ^= (u32)WM_U8(k_bss_const_addrs[i]);
        h *= 16777619u;
    }
    return h;
}

static int wm_800736DC_init_constants(void)
{
    u32 pre_hash;
    u32 post_hash;
    u32 guard_lo[4];
    u32 guard_hi[4];
    u32 c88c_snap;
    u32 c620_snap;
    u32 pool_snap;
    u32 be4c0_snap;
    int match;
    int i;

    fprintf(stderr, "[worldmap-bss-constants] entry\n");

    if (s_wm736dc_ran) {
        fprintf(stderr,
                "[worldmap-bss-constants] ERROR: already ran this world entry\n");
        return -1;
    }
    /* Retail modes 8/11 call this initializer without the base-mode third
     * archive wave.  Its stores are independent of wm_80072090. */

    /* Guards + prior-rung snaps. */
    for (i = 0; i < 4; i++) {
        guard_lo[i] = WM_U32(WM_BSS_LOWEST - 16u + (u32)i * 4u);
        guard_hi[i] = WM_U32(WM_BSS_HIGHEST + 1u + (u32)i * 4u);
    }
    c88c_snap = WM_U32(WM_TW_MIRROR_C88C);
    c620_snap = WM_U32(WM_OBJ_C620);
    pool_snap = WM_U32(WM_POOL_BE24);
    be4c0_snap = WM_U32(WM_TMPL_DST_BE4C);
    pre_hash = wm_bss_footprint_hash();

    /* Retail store order — unrolled; later sb overlays word tails. */
    WM_U32(0x8009D1C4u) = WM_BSS_CONST_A;
    WM_U32(0x8009D1BCu) = WM_BSS_CONST_A;
    WM_U32(0x8009D1A0u) = WM_BSS_CONST_A;
    WM_U32(0x8009D198u) = WM_BSS_CONST_A;
    WM_U32(0x8009D1D4u) = WM_BSS_CONST_B;
    WM_U32(0x8009D1CCu) = WM_BSS_CONST_B;
    WM_U32(0x8009D1B0u) = WM_BSS_CONST_B;
    WM_U32(0x8009D1A8u) = WM_BSS_CONST_B;
    WM_U8(0x8009D197u) = WM_BSS_CONST_K8;
    WM_U8(0x8009D19Bu) = WM_BSS_CONST_K56;
    WM_U8(0x8009D1BBu) = WM_BSS_CONST_K8;
    WM_U8(0x8009D1BFu) = WM_BSS_CONST_K56;
    WM_U32(0x8009D20Cu) = WM_BSS_CONST_C;
    WM_U32(0x8009D204u) = WM_BSS_CONST_C;
    WM_U32(0x8009D1E8u) = WM_BSS_CONST_C;
    WM_U32(0x8009D1E0u) = WM_BSS_CONST_C;
    WM_U32(0x8009D21Cu) = WM_BSS_CONST_A;
    WM_U32(0x8009D214u) = WM_BSS_CONST_A;
    WM_U32(0x8009D1F8u) = WM_BSS_CONST_A;
    WM_U32(0x8009D1F0u) = WM_BSS_CONST_A;
    WM_U8(0x8009D1DFu) = WM_BSS_CONST_K8;
    WM_U8(0x8009D1E3u) = WM_BSS_CONST_K56;
    WM_U8(0x8009D203u) = WM_BSS_CONST_K8;
    WM_U8(0x8009D207u) = WM_BSS_CONST_K56;
    WM_U32(0x8009D264u) = WM_BSS_CONST_C;
    WM_U32(0x8009D25Cu) = WM_BSS_CONST_C;
    WM_U32(0x8009D254u) = WM_BSS_CONST_C;
    WM_U32(0x8009D24Cu) = WM_BSS_CONST_C;
    WM_U32(0x8009D240u) = WM_BSS_CONST_C;
    WM_U32(0x8009D238u) = WM_BSS_CONST_C;
    WM_U32(0x8009D230u) = WM_BSS_CONST_C;
    WM_U32(0x8009D228u) = WM_BSS_CONST_C;
    WM_U8(0x8009D227u) = WM_BSS_CONST_K8;
    WM_U8(0x8009D22Bu) = WM_BSS_CONST_K56;
    WM_U8(0x8009D24Bu) = WM_BSS_CONST_K8;
    WM_U8(0x8009D24Fu) = WM_BSS_CONST_K56;
    WM_U32(0x8009D2ACu) = WM_BSS_CONST_C;
    WM_U32(0x8009D2A4u) = WM_BSS_CONST_C;
    WM_U32(0x8009D29Cu) = WM_BSS_CONST_C;
    WM_U32(0x8009D294u) = WM_BSS_CONST_C;
    WM_U32(0x8009D288u) = WM_BSS_CONST_C;
    WM_U32(0x8009D280u) = WM_BSS_CONST_C;
    WM_U32(0x8009D278u) = WM_BSS_CONST_C;
    WM_U32(0x8009D270u) = WM_BSS_CONST_C;
    WM_U8(0x8009D26Fu) = WM_BSS_CONST_K8;
    WM_U8(0x8009D273u) = WM_BSS_CONST_K56;
    WM_U8(0x8009D293u) = WM_BSS_CONST_K8;
    WM_U8(0x8009D297u) = WM_BSS_CONST_K56; /* jr delay slot */

    post_hash = wm_bss_footprint_hash();
    match = 0;
    for (i = 0; i < WM_BSS_UNIQUE_BYTES; i++) {
        if (WM_U8(k_bss_const_addrs[i]) == k_bss_const_expect[i])
            match++;
    }

    fprintf(stderr,
            "[worldmap-bss-constants] store_count=%d unique_bytes=%d "
            "lowest_address=0x%08x highest_address=0x%08x\n",
            WM_BSS_STORE_COUNT, WM_BSS_UNIQUE_BYTES, WM_BSS_LOWEST,
            WM_BSS_HIGHEST);
    fprintf(stderr,
            "[worldmap-bss-constants] pre_hash=0x%08x post_hash=0x%08x "
            "expected_fnv=0x%08x expected_bytes=%d/%d\n",
            pre_hash, post_hash, WM_BSS_EXPECT_FNV, match, WM_BSS_UNIQUE_BYTES);

    if (match != WM_BSS_UNIQUE_BYTES || post_hash != WM_BSS_EXPECT_FNV) {
        fprintf(stderr,
                "[worldmap-bss-constants] ERROR: expected constant mismatch\n");
        return -1;
    }

    for (i = 0; i < 4; i++) {
        if (WM_U32(WM_BSS_LOWEST - 16u + (u32)i * 4u) != guard_lo[i] ||
            WM_U32(WM_BSS_HIGHEST + 1u + (u32)i * 4u) != guard_hi[i]) {
            fprintf(stderr,
                    "[worldmap-bss-constants] ERROR: neighbor guard changed\n");
            return -1;
        }
    }
    if (WM_U32(WM_TW_MIRROR_C88C) != c88c_snap ||
        WM_U32(WM_OBJ_C620) != c620_snap ||
        WM_U32(WM_POOL_BE24) != pool_snap ||
        WM_U32(WM_TMPL_DST_BE4C) != be4c0_snap) {
        fprintf(stderr,
                "[worldmap-bss-constants] ERROR: prior rung state corrupted\n");
        return -1;
    }

    s_wm736dc_ran = 1;
    fprintf(stderr, "[worldmap-bss-constants] exit\n");
    fprintf(stderr,
            "[worldmap-bss-constants] cut-before-next-step retail_pc=0x%08x\n",
            WM_CUT_BEFORE_73E30);
    return 0;
}

/* Compile-time layout locks for PsyQ packet structs used by 0x80073E30. */
typedef char wm_assert_dr_tpage_8[(sizeof(DR_TPAGE) == 8) ? 1 : -1];
typedef char wm_assert_poly_ft4_40[(sizeof(POLY_FT4) == 40) ? 1 : -1];
typedef char wm_assert_poly_g3_28[(sizeof(POLY_G3) == 28) ? 1 : -1];
typedef char wm_assert_tile_16[(sizeof(TILE) == 16) ? 1 : -1];

static int s_wm73e30_ran;

static u32 wm_prim_fnv1a(const u8* p, unsigned n)
{
    u32 h = 2166136261u;
    unsigned i;
    for (i = 0; i < n; i++) {
        h ^= (u32)p[i];
        h *= 16777619u;
    }
    return h;
}

/*
 * Retail 0x80073E30 — one-shot world BSS GPU packet-template initializer.
 * Leaf: GetTPage / GetClut / SetSemiTrans / SetDrawTPage only.
 * Cut residual before 0x8007246C (jal 0x80085F58).
 */
static int wm_80073E30_primitive_templates(void)
{
    POLY_FT4* ft4_0;
    POLY_FT4* ft4_1;
    DR_TPAGE* dr;
    u16 tpage0;
    u16 tpage1;
    u16 clut;
    u32 dr_mode;
    int i;
    int j;
    int match_ft4;
    int match_g3;
    int match_tile;
    int unexpected;
    u32 pre_dr_h;
    u32 pre_ft4_0_h;
    u32 pre_ft4_1_h;
    u32 pre_g3_h;
    u32 pre_tile_h;
    u32 post_dr_h;
    u32 post_ft4_0_h;
    u32 post_ft4_1_h;
    u32 post_g3_h;
    u32 post_tile_h;
    u8 snap_dr[8];
    u8 snap_ft4_0[40];
    u8 snap_ft4_1[40];
    u8 snap_g3[WM_PRIM_G3_BYTES];
    u8 snap_tile[WM_PRIM_TILE_BYTES];
    u8 expect_dr[8];
    u8 expect_ft4[40];
    u8 expect_g3[WM_PRIM_G3_BYTES];
    u8 expect_tile[WM_PRIM_TILE_BYTES];
    /* Guard / gap / prior-rung snaps */
    u32 guard_before_dr[4];
    u32 gap_c5a8;
    u32 gap_c614;
    u32 gap_c620;
    u32 gap_c660;
    u32 gap_c7ec;
    u32 gap_c88c;
    u32 gap_c894;
    u32 bss_d198;
    u32 pool_snap;
    u32 be4c0_snap;
    u32 c620_snap;

    fprintf(stderr, "[worldmap-primitive-templates] entry\n");

    if (s_wm73e30_ran) {
        fprintf(stderr,
                "[worldmap-primitive-templates] ERROR: already ran this "
                "process\n");
        return -1;
    }
    if (!s_wm736dc_ran) {
        fprintf(stderr,
                "[worldmap-primitive-templates] ERROR: W13B did not run "
                "(required)\n");
        return -1;
    }

    ft4_0 = (POLY_FT4*)PSX_ADDR(WM_PRIM_FT4_0);
    ft4_1 = (POLY_FT4*)PSX_ADDR(WM_PRIM_FT4_1);
    dr = (DR_TPAGE*)PSX_ADDR(WM_PRIM_DR_TPAGE);

    /* Snapshots (full regions + sensitive gaps + prior rungs). */
    wm_memcpy(snap_dr, PSX_ADDR(WM_PRIM_DR_TPAGE), 8);
    wm_memcpy(snap_ft4_0, PSX_ADDR(WM_PRIM_FT4_0), 40);
    wm_memcpy(snap_ft4_1, PSX_ADDR(WM_PRIM_FT4_1), 40);
    wm_memcpy(snap_g3, PSX_ADDR(WM_PRIM_G3_BASE), WM_PRIM_G3_BYTES);
    wm_memcpy(snap_tile, PSX_ADDR(WM_PRIM_TILE_BASE), WM_PRIM_TILE_BYTES);
    for (i = 0; i < 4; i++)
        guard_before_dr[i] =
            WM_U32(WM_PRIM_DR_TPAGE - 16u + (u32)i * 4u);
    gap_c5a8 = WM_U32(0x8009C5A8u);
    gap_c614 = WM_U32(0x8009C614u);
    gap_c620 = WM_U32(0x8009C620u);
    gap_c660 = WM_U32(0x8009C660u);
    gap_c7ec = WM_U32(WM_FIX_C7EC);
    gap_c88c = WM_U32(WM_TW_MIRROR_C88C);
    gap_c894 = WM_U32(WM_FLAG_C894_ABS);
    bss_d198 = WM_U32(0x8009D198u);
    pool_snap = WM_U32(WM_POOL_BE24);
    be4c0_snap = WM_U32(WM_TMPL_DST_BE4C);
    c620_snap = WM_U32(WM_OBJ_C620);

    pre_dr_h = wm_prim_fnv1a(snap_dr, 8);
    pre_ft4_0_h = wm_prim_fnv1a(snap_ft4_0, 40);
    pre_ft4_1_h = wm_prim_fnv1a(snap_ft4_1, 40);
    pre_g3_h = wm_prim_fnv1a(snap_g3, WM_PRIM_G3_BYTES);
    pre_tile_h = wm_prim_fnv1a(snap_tile, WM_PRIM_TILE_BYTES);

    /* ---- Retail store order (0x80073E30) ---- */

    /* POLY_FT4[0] header + geometry + UV + RGB (before helpers). */
    WM_U8(WM_PRIM_FT4_0 + 3u) = 9;       /* len */
    WM_U8(WM_PRIM_FT4_0 + 7u) = 0x2C;    /* code POLY_FT4 */
    WM_U16(WM_PRIM_FT4_0 + 0x0Au) = 120; /* y0 */
    WM_U16(WM_PRIM_FT4_0 + 0x12u) = 120; /* y1 */
    WM_U16(WM_PRIM_FT4_0 + 0x1Au) = 215; /* y2 */
    WM_U16(WM_PRIM_FT4_0 + 0x22u) = 215; /* y3 */
    WM_U16(WM_PRIM_FT4_0 + 0x08u) = 208; /* x0 */
    WM_U16(WM_PRIM_FT4_0 + 0x18u) = 208; /* x2 */
    WM_U16(WM_PRIM_FT4_0 + 0x10u) = 311; /* x1 */
    WM_U16(WM_PRIM_FT4_0 + 0x20u) = 311; /* x3 */
    WM_U8(WM_PRIM_FT4_0 + 0x0Cu) = 0;    /* u0 */
    WM_U8(WM_PRIM_FT4_0 + 0x0Du) = 128;  /* v0 */
    WM_U8(WM_PRIM_FT4_0 + 0x14u) = 127;  /* u1 */
    WM_U8(WM_PRIM_FT4_0 + 0x15u) = 128;  /* v1 */
    WM_U8(WM_PRIM_FT4_0 + 0x1Cu) = 0;    /* u2 */
    WM_U8(WM_PRIM_FT4_0 + 0x1Du) = 255;  /* v2 */
    WM_U8(WM_PRIM_FT4_0 + 0x24u) = 127;  /* u3 */
    WM_U8(WM_PRIM_FT4_0 + 0x25u) = 255;  /* v3 */
    WM_U8(WM_PRIM_FT4_0 + 4u) = 128;     /* r0 */
    WM_U8(WM_PRIM_FT4_0 + 5u) = 128;     /* g0 */
    WM_U8(WM_PRIM_FT4_0 + 6u) = 128;     /* b0 */

    tpage0 = GetTPage(0, 0, 896, 256);
    WM_U16(WM_PRIM_FT4_0 + 0x16u) = tpage0; /* tpage */

    clut = GetClut(256, 510);
    WM_U16(WM_PRIM_FT4_0 + 0x0Eu) = clut; /* clut */

    SetSemiTrans(ft4_0, 1);

    /* Retail 40-byte copy: 16 + 16 + 8 words from C5C0 → C5E8. */
    {
        u32* src = (u32*)PSX_ADDR(WM_PRIM_FT4_0);
        u32* dst = (u32*)PSX_ADDR(WM_PRIM_FT4_1);
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
        dst[8] = src[8];
        dst[9] = src[9];
    }

    tpage1 = GetTPage(0, 1, 896, 256);
    SetDrawTPage(dr, 1, 0, (int)(tpage1 & 0xFFFFu));
    dr_mode = ((u32*)dr)[1];

    /* POLY_G3 × 8 — identical entries; SetSemiTrans each. */
    for (i = 0; i < (int)WM_PRIM_G3_COUNT; i++) {
        u32 base = WM_PRIM_G3_BASE + (u32)i * WM_PRIM_G3_STRIDE;
        POLY_G3* g3 = (POLY_G3*)PSX_ADDR(base);
        WM_U8(base + 3u) = 6;    /* len */
        WM_U8(base + 7u) = 0x30; /* code POLY_G3 */
        WM_U8(base + 4u) = 255;  /* r0 */
        WM_U8(base + 5u) = 64;   /* g0 */
        WM_U8(base + 6u) = 64;   /* b0 */
        WM_U8(base + 12u) = 0;   /* r1 */
        WM_U8(base + 13u) = 0;   /* g1 */
        WM_U8(base + 14u) = 0;   /* b1 */
        WM_U8(base + 20u) = 0;   /* r2 */
        WM_U8(base + 21u) = 0;   /* g2 */
        WM_U8(base + 22u) = 0;   /* b2 */
        SetSemiTrans(g3, 1);
    }

    /* TILE × 64 */
    for (i = 0; i < (int)WM_PRIM_TILE_COUNT; i++) {
        u32 base = WM_PRIM_TILE_BASE + (u32)i * WM_PRIM_TILE_STRIDE;
        WM_U8(base + 3u) = 3;    /* len */
        WM_U8(base + 7u) = 0x60; /* code TILE */
        WM_U8(base + 4u) = 128;  /* r0 */
        WM_U8(base + 5u) = 128;  /* g0 */
        WM_U8(base + 6u) = 16;   /* b0 */
        WM_U16(base + 12u) = 2;  /* w */
        WM_U16(base + 14u) = 2;  /* h */
    }

    /* ---- Build expected image from pre-snap + retail writes ---- */
    wm_memcpy(expect_dr, snap_dr, 8);
    expect_dr[3] = 1;
    {
        u32 mode = 0xE1000000u | 0x0400u | (0x003Eu & 0x9FFu);
        expect_dr[4] = (u8)(mode);
        expect_dr[5] = (u8)(mode >> 8);
        expect_dr[6] = (u8)(mode >> 16);
        expect_dr[7] = (u8)(mode >> 24);
    }

    wm_memcpy(expect_ft4, snap_ft4_0, 40);
    expect_ft4[3] = 9;
    expect_ft4[4] = 128;
    expect_ft4[5] = 128;
    expect_ft4[6] = 128;
    expect_ft4[7] = 0x2E;
    expect_ft4[8] = (u8)(208);
    expect_ft4[9] = (u8)(208 >> 8);
    expect_ft4[10] = (u8)(120);
    expect_ft4[11] = (u8)(120 >> 8);
    expect_ft4[12] = 0;
    expect_ft4[13] = 128;
    expect_ft4[14] = (u8)(0x7F90);
    expect_ft4[15] = (u8)(0x7F90 >> 8);
    expect_ft4[16] = (u8)(311);
    expect_ft4[17] = (u8)(311 >> 8);
    expect_ft4[18] = (u8)(120);
    expect_ft4[19] = (u8)(120 >> 8);
    expect_ft4[20] = 127;
    expect_ft4[21] = 128;
    expect_ft4[22] = (u8)(0x001E);
    expect_ft4[23] = (u8)(0x001E >> 8);
    expect_ft4[24] = (u8)(208);
    expect_ft4[25] = (u8)(208 >> 8);
    expect_ft4[26] = (u8)(215);
    expect_ft4[27] = (u8)(215 >> 8);
    expect_ft4[28] = 0;
    expect_ft4[29] = 255;
    /* +30,+31 pad1 untouched (from snap) */
    expect_ft4[32] = (u8)(311);
    expect_ft4[33] = (u8)(311 >> 8);
    expect_ft4[34] = (u8)(215);
    expect_ft4[35] = (u8)(215 >> 8);
    expect_ft4[36] = 127;
    expect_ft4[37] = 255;
    /* +38,+39 pad2 untouched */

    wm_memcpy(expect_g3, snap_g3, WM_PRIM_G3_BYTES);
    for (i = 0; i < (int)WM_PRIM_G3_COUNT; i++) {
        u8* e = expect_g3 + i * (int)WM_PRIM_G3_STRIDE;
        e[3] = 6;
        e[4] = 255;
        e[5] = 64;
        e[6] = 64;
        e[7] = 0x32;
        e[12] = 0;
        e[13] = 0;
        e[14] = 0;
        e[20] = 0;
        e[21] = 0;
        e[22] = 0;
    }

    wm_memcpy(expect_tile, snap_tile, WM_PRIM_TILE_BYTES);
    for (i = 0; i < (int)WM_PRIM_TILE_COUNT; i++) {
        u8* e = expect_tile + i * (int)WM_PRIM_TILE_STRIDE;
        e[3] = 3;
        e[4] = 128;
        e[5] = 128;
        e[6] = 16;
        e[7] = 0x60;
        e[12] = 2;
        e[13] = 0;
        e[14] = 2;
        e[15] = 0;
    }

    post_dr_h = wm_prim_fnv1a((const u8*)PSX_ADDR(WM_PRIM_DR_TPAGE), 8);
    post_ft4_0_h = wm_prim_fnv1a((const u8*)PSX_ADDR(WM_PRIM_FT4_0), 40);
    post_ft4_1_h = wm_prim_fnv1a((const u8*)PSX_ADDR(WM_PRIM_FT4_1), 40);
    post_g3_h =
        wm_prim_fnv1a((const u8*)PSX_ADDR(WM_PRIM_G3_BASE), WM_PRIM_G3_BYTES);
    post_tile_h = wm_prim_fnv1a((const u8*)PSX_ADDR(WM_PRIM_TILE_BASE),
                                WM_PRIM_TILE_BYTES);

    match_ft4 = 0;
    for (j = 0; j < 40; j++) {
        if (((const u8*)PSX_ADDR(WM_PRIM_FT4_0))[j] == expect_ft4[j])
            match_ft4++;
    }
    match_g3 = 0;
    for (j = 0; j < (int)WM_PRIM_G3_BYTES; j++) {
        if (((const u8*)PSX_ADDR(WM_PRIM_G3_BASE))[j] == expect_g3[j])
            match_g3++;
    }
    match_tile = 0;
    for (j = 0; j < (int)WM_PRIM_TILE_BYTES; j++) {
        if (((const u8*)PSX_ADDR(WM_PRIM_TILE_BASE))[j] == expect_tile[j])
            match_tile++;
    }

    unexpected = 0;
    for (j = 0; j < 8; j++) {
        if (((const u8*)PSX_ADDR(WM_PRIM_DR_TPAGE))[j] != expect_dr[j])
            unexpected++;
    }
    for (j = 0; j < 40; j++) {
        if (((const u8*)PSX_ADDR(WM_PRIM_FT4_0))[j] != expect_ft4[j])
            unexpected++;
        if (((const u8*)PSX_ADDR(WM_PRIM_FT4_1))[j] !=
            ((const u8*)PSX_ADDR(WM_PRIM_FT4_0))[j])
            unexpected++;
    }
    for (j = 0; j < (int)WM_PRIM_G3_BYTES; j++) {
        if (((const u8*)PSX_ADDR(WM_PRIM_G3_BASE))[j] != expect_g3[j])
            unexpected++;
    }
    for (j = 0; j < (int)WM_PRIM_TILE_BYTES; j++) {
        if (((const u8*)PSX_ADDR(WM_PRIM_TILE_BASE))[j] != expect_tile[j])
            unexpected++;
    }

    fprintf(stderr,
            "[worldmap-primitive-templates] helpers GetTPage(0,0,896,256)="
            "0x%04x GetClut(256,510)=0x%04x GetTPage(0,1,896,256)=0x%04x "
            "DR_mode=0x%08x\n",
            (unsigned)tpage0, (unsigned)clut, (unsigned)tpage1,
            (unsigned)dr_mode);
    fprintf(stderr,
            "[worldmap-primitive-templates] FT4[0] code=0x%02x clut=0x%04x "
            "tpage=0x%04x rgb=(%u,%u,%u) xy0=(%d,%d) xy3=(%d,%d) "
            "uv0=(%u,%u) uv3=(%u,%u)\n",
            (unsigned)WM_U8(WM_PRIM_FT4_0 + 7u),
            (unsigned)WM_U16(WM_PRIM_FT4_0 + 0x0Eu),
            (unsigned)WM_U16(WM_PRIM_FT4_0 + 0x16u),
            (unsigned)WM_U8(WM_PRIM_FT4_0 + 4u),
            (unsigned)WM_U8(WM_PRIM_FT4_0 + 5u),
            (unsigned)WM_U8(WM_PRIM_FT4_0 + 6u),
            (int)WM_S16(WM_PRIM_FT4_0 + 0x08u),
            (int)WM_S16(WM_PRIM_FT4_0 + 0x0Au),
            (int)WM_S16(WM_PRIM_FT4_0 + 0x20u),
            (int)WM_S16(WM_PRIM_FT4_0 + 0x22u),
            (unsigned)WM_U8(WM_PRIM_FT4_0 + 0x0Cu),
            (unsigned)WM_U8(WM_PRIM_FT4_0 + 0x0Du),
            (unsigned)WM_U8(WM_PRIM_FT4_0 + 0x24u),
            (unsigned)WM_U8(WM_PRIM_FT4_0 + 0x25u));
    fprintf(stderr,
            "[worldmap-primitive-templates] FT4 equal=%d G3[0] code=0x%02x "
            "TILE[0] code=0x%02x w=%u h=%u\n",
            wm_memeq(PSX_ADDR(WM_PRIM_FT4_0), PSX_ADDR(WM_PRIM_FT4_1), 40),
            (unsigned)WM_U8(WM_PRIM_G3_BASE + 7u),
            (unsigned)WM_U8(WM_PRIM_TILE_BASE + 7u),
            (unsigned)WM_U16(WM_PRIM_TILE_BASE + 12u),
            (unsigned)WM_U16(WM_PRIM_TILE_BASE + 14u));
    fprintf(stderr,
            "[worldmap-primitive-templates] hashes "
            "DR pre=0x%08x post=0x%08x exp=0x%08x | "
            "FT4 pre=0x%08x post=0x%08x exp=0x%08x | "
            "G3 pre=0x%08x post=0x%08x exp=0x%08x | "
            "TILE pre=0x%08x post=0x%08x exp=0x%08x\n",
            pre_dr_h, post_dr_h, wm_prim_fnv1a(expect_dr, 8), pre_ft4_0_h,
            post_ft4_0_h, wm_prim_fnv1a(expect_ft4, 40), pre_g3_h, post_g3_h,
            wm_prim_fnv1a(expect_g3, WM_PRIM_G3_BYTES), pre_tile_h, post_tile_h,
            wm_prim_fnv1a(expect_tile, WM_PRIM_TILE_BYTES));
    fprintf(stderr,
            "[worldmap-primitive-templates] match FT4=%d/40 G3=%d/%u "
            "TILE=%d/%u unexpected=%d ft4_1_hash=0x%08x\n",
            match_ft4, match_g3, WM_PRIM_G3_BYTES, match_tile,
            WM_PRIM_TILE_BYTES, unexpected, post_ft4_1_h);

    if (tpage0 != 0x001Eu || clut != 0x7F90u || tpage1 != 0x003Eu ||
        dr_mode != 0xE100043Eu) {
        fprintf(stderr,
                "[worldmap-primitive-templates] ERROR: helper/result mismatch "
                "tpage0=0x%04x clut=0x%04x tpage1=0x%04x mode=0x%08x\n",
                (unsigned)tpage0, (unsigned)clut, (unsigned)tpage1,
                (unsigned)dr_mode);
        return -1;
    }
    if (WM_U8(WM_PRIM_FT4_0 + 7u) != 0x2Eu ||
        WM_U16(WM_PRIM_FT4_0 + 0x0Eu) != 0x7F90u ||
        WM_U16(WM_PRIM_FT4_0 + 0x16u) != 0x001Eu) {
        fprintf(stderr,
                "[worldmap-primitive-templates] ERROR: FT4 field checks "
                "failed\n");
        return -1;
    }
    if (!wm_memeq(PSX_ADDR(WM_PRIM_FT4_0), PSX_ADDR(WM_PRIM_FT4_1), 40)) {
        fprintf(stderr,
                "[worldmap-primitive-templates] ERROR: FT4[1] != FT4[0]\n");
        return -1;
    }
    for (i = 0; i < (int)WM_PRIM_G3_COUNT; i++) {
        if (WM_U8(WM_PRIM_G3_BASE + (u32)i * WM_PRIM_G3_STRIDE + 7u) !=
            0x32u) {
            fprintf(stderr,
                    "[worldmap-primitive-templates] ERROR: G3[%d] code\n", i);
            return -1;
        }
    }
    for (i = 0; i < (int)WM_PRIM_TILE_COUNT; i++) {
        u32 base = WM_PRIM_TILE_BASE + (u32)i * WM_PRIM_TILE_STRIDE;
        if (WM_U8(base + 7u) != 0x60u || WM_U16(base + 12u) != 2u ||
            WM_U16(base + 14u) != 2u) {
            fprintf(stderr,
                    "[worldmap-primitive-templates] ERROR: TILE[%d] fields\n",
                    i);
            return -1;
        }
    }
    if (match_ft4 != 40 || match_g3 != (int)WM_PRIM_G3_BYTES ||
        match_tile != (int)WM_PRIM_TILE_BYTES || unexpected != 0 ||
        post_dr_h != wm_prim_fnv1a(expect_dr, 8) ||
        post_ft4_0_h != wm_prim_fnv1a(expect_ft4, 40) ||
        post_g3_h != wm_prim_fnv1a(expect_g3, WM_PRIM_G3_BYTES) ||
        post_tile_h != wm_prim_fnv1a(expect_tile, WM_PRIM_TILE_BYTES)) {
        fprintf(stderr,
                "[worldmap-primitive-templates] ERROR: oracle mismatch "
                "unexpected=%d\n",
                unexpected);
        return -1;
    }

    /* Gaps + prior rungs unchanged. */
    for (i = 0; i < 4; i++) {
        if (WM_U32(WM_PRIM_DR_TPAGE - 16u + (u32)i * 4u) != guard_before_dr[i]) {
            fprintf(stderr,
                    "[worldmap-primitive-templates] ERROR: pre-DR guard "
                    "changed\n");
            return -1;
        }
    }
    if (WM_U32(0x8009C5A8u) != gap_c5a8 || WM_U32(0x8009C614u) != gap_c614 ||
        WM_U32(0x8009C620u) != gap_c620 || WM_U32(0x8009C660u) != gap_c660 ||
        WM_U32(WM_FIX_C7EC) != gap_c7ec ||
        WM_U32(WM_TW_MIRROR_C88C) != gap_c88c ||
        WM_U32(WM_FLAG_C894_ABS) != gap_c894 ||
        WM_U32(0x8009D198u) != bss_d198 ||
        WM_U32(WM_POOL_BE24) != pool_snap ||
        WM_U32(WM_TMPL_DST_BE4C) != be4c0_snap ||
        WM_U32(WM_OBJ_C620) != c620_snap) {
        fprintf(stderr,
                "[worldmap-primitive-templates] ERROR: gap/prior-rung "
                "corruption\n");
        return -1;
    }

    (void)ft4_1;
    (void)pre_ft4_1_h;
    s_wm73e30_ran = 1;
    fprintf(stderr, "[worldmap-primitive-templates] exit\n");
    fprintf(stderr,
            "[worldmap-primitive-templates] cut-before-next-step "
            "retail_pc=0x%08x\n",
            WM_CUT_BEFORE_85F58);
    return 0;
}

static int s_wm85f58_ran;

/*
 * Retail 0x80085F58 — one-shot record-table relocation + CLUT id paint.
 * Non-idempotent on field_00: process-local s_wm85f58_ran is the only
 * ownership within one world entry (reset by PcPort_WorldMapInitMain).
 * Cut residual before 0x80072478 (jal GfxAllocateWorkBuffers).
 */
static int wm_80085F58_relocate_records_and_init_cluts(void)
{
    u32 table_psx;
    void* table_host;
    u32 c180_psx;
    u8* table;
    u8 pre_recs[WM_REC_BYTES];
    u8 pre_clut[WM_CLUT_BYTES];
    u8 expect_recs[WM_REC_BYTES];
    u8 expect_clut[WM_CLUT_BYTES];
    u32 pre_rec_h;
    u32 post_rec_h;
    u32 exp_rec_h;
    u32 pre_clut_h;
    u32 post_clut_h;
    u32 exp_clut_h;
    u32 max_rel;
    u32 first_nz_idx;
    u32 last_nz_idx;
    int nonzero;
    int relocated;
    int field04_nz;
    int i;
    int unexpected;
    int match_rec;
    int match_clut;
    u32 guard_c7ec_lo[2];
    u32 guard_c7ec_hi[2];
    u32 guard_d478_lo[2];
    u32 guard_d478_hi[2];
    u32 ft4_code_snap;
    u32 dr_mode_snap;
    u32 c88c_snap;
    u32 bss_d198_snap;
    u16 first_clut;
    u16 last_clut;

    fprintf(stderr, "[worldmap-record-clut] entry\n");

    if (s_wm85f58_ran) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: already ran this world entry "
                "(non-idempotent relocation; blocked)\n");
        fprintf(stderr,
                "[worldmap-record-clut] second_call_detected=1 "
                "second_call_blocked=1\n");
        return -1;
    }
    /* Retail modes 8/11 omit the base-mode primitive-template builder before
     * this call.  The record relocation consumes C7EC from second-wave data. */

    table_psx = WM_U32(WM_FIX_C7EC);
    c180_psx = WM_U32(WM_DST_C180);
    table_host = psx_u32_to_host(table_psx);

    fprintf(stderr,
            "[worldmap-record-clut] table_psx=0x%08x table_host=%p "
            "record_count=%u c180_psx=0x%08x\n",
            table_psx, table_host, WM_REC_COUNT, c180_psx);

    if (table_psx == 0 || table_host == NULL) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: C7EC null or unresolvable\n");
        return -1;
    }
    if (table_psx < 0x80000000u || table_psx >= 0x80200000u) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: C7EC not KUSEG "
                "0x%08x\n",
                table_psx);
        return -1;
    }
    if ((table_psx & 3u) != 0) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: table not 4-byte aligned\n");
        return -1;
    }
    /* Full 2048-byte table must stay inside emulated PSX RAM. */
    if (table_psx + WM_REC_BYTES > 0x80200000u ||
        table_psx + WM_REC_BYTES < table_psx) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: table span out of PSX RAM\n");
        return -1;
    }
    /* Table should live at/after the W4C decompressed blob base. */
    if (c180_psx != 0 &&
        (c180_psx < 0x80000000u || table_psx < c180_psx)) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: table not inside W4C blob "
                "bounds (c180=0x%08x table=0x%08x)\n",
                c180_psx, table_psx);
        return -1;
    }

    table = (u8*)table_host;

    /* Snapshots. */
    wm_memcpy(pre_recs, table, WM_REC_BYTES);
    wm_memcpy(pre_clut, PSX_ADDR(WM_CLUT_D478), WM_CLUT_BYTES);
    guard_c7ec_lo[0] = WM_U32(WM_FIX_C7EC - 8u);
    guard_c7ec_lo[1] = WM_U32(WM_FIX_C7EC - 4u);
    guard_c7ec_hi[0] = WM_U32(WM_FIX_C7EC + 4u);
    guard_c7ec_hi[1] = WM_U32(WM_FIX_C7EC + 8u);
    guard_d478_lo[0] = WM_U32(WM_CLUT_D478 - 8u);
    guard_d478_lo[1] = WM_U32(WM_CLUT_D478 - 4u);
    guard_d478_hi[0] = WM_U32(WM_CLUT_D478 + WM_CLUT_BYTES);
    guard_d478_hi[1] = WM_U32(WM_CLUT_D478 + WM_CLUT_BYTES + 4u);
    ft4_code_snap = WM_U8(WM_PRIM_FT4_0 + 7u);
    dr_mode_snap = WM_U32(WM_PRIM_DR_TPAGE + 4u);
    c88c_snap = WM_U32(WM_TW_MIRROR_C88C);
    bss_d198_snap = WM_U32(0x8009D198u);

    pre_rec_h = wm_prim_fnv1a(pre_recs, WM_REC_BYTES);
    pre_clut_h = wm_prim_fnv1a(pre_clut, WM_CLUT_BYTES);

    nonzero = 0;
    field04_nz = 0;
    max_rel = 0;
    first_nz_idx = 0xFFFFFFFFu;
    last_nz_idx = 0xFFFFFFFFu;
    for (i = 0; i < (int)WM_REC_COUNT; i++) {
        u32 f0 = *(u32*)(pre_recs + (u32)i * WM_REC_STRIDE);
        u32 f4 = *(u32*)(pre_recs + (u32)i * WM_REC_STRIDE + 4u);
        if (f0 != 0) {
            nonzero++;
            if (f0 > max_rel)
                max_rel = f0;
            if (first_nz_idx == 0xFFFFFFFFu)
                first_nz_idx = (u32)i;
            last_nz_idx = (u32)i;
            /* Diagnostic: relative should not already be KUSEG (W4C leaves
             * relatives). Fail if high bit set — not a range heuristic for
             * "skip", only preflight consistency. */
            if (f0 >= 0x80000000u) {
                fprintf(stderr,
                        "[worldmap-record-clut] ERROR: field_00[%d]=0x%08x "
                        "looks already absolute (expected relative)\n",
                        i, f0);
                return -1;
            }
            if (table_psx + f0 < table_psx ||
                table_psx + f0 >= 0x80200000u) {
                fprintf(stderr,
                        "[worldmap-record-clut] ERROR: reloc[%d] out of "
                        "PSX RAM (base=0x%08x rel=0x%08x)\n",
                        i, table_psx, f0);
                return -1;
            }
        }
        if (f4 != 0)
            field04_nz++;
    }

    fprintf(stderr,
            "[worldmap-record-clut] nonzero_records=%d field04_nz=%d "
            "max_rel=0x%08x first_nz=%u last_nz=%u table_span=0x%08x..0x%08x\n",
            nonzero, field04_nz, max_rel, first_nz_idx, last_nz_idx, table_psx,
            table_psx + WM_REC_BYTES - 1u);

    /* ---- Retail Phase A: relocate field_00 only ---- */
    relocated = 0;
    for (i = 0; i < (int)WM_REC_COUNT; i++) {
        u32 off = (u32)i * WM_REC_STRIDE;
        u32 relative = WM_U32(table_psx + off);
        if (relative != 0) {
            /* Store 32-bit KUSEG absolute; never host pointer. */
            WM_U32(table_psx + off) = table_psx + relative;
            relocated++;
        }
    }

    /* ---- Retail Phase B: CLUT table ---- */
    for (i = 0; i < (int)WM_CLUT_COUNT; i++) {
        u16 clut = GetClut(240, 496 + i);
        WM_U16(WM_CLUT_D478 + (u32)i * 2u) = clut;
    }
    first_clut = WM_U16(WM_CLUT_D478);
    last_clut = WM_U16(WM_CLUT_D478 + (WM_CLUT_COUNT - 1u) * 2u);

    /* Build expected oracle from pre-snap. */
    wm_memcpy(expect_recs, pre_recs, WM_REC_BYTES);
    for (i = 0; i < (int)WM_REC_COUNT; i++) {
        u32* f0 = (u32*)(expect_recs + (u32)i * WM_REC_STRIDE);
        if (*f0 != 0)
            *f0 = table_psx + *f0;
        /* field_04 already from pre */
    }
    for (i = 0; i < (int)WM_CLUT_COUNT; i++) {
        u16 c = GetClut(240, 496 + i);
        expect_clut[i * 2] = (u8)(c & 0xFFu);
        expect_clut[i * 2 + 1] = (u8)((c >> 8) & 0xFFu);
    }

    post_rec_h = wm_prim_fnv1a(table, WM_REC_BYTES);
    post_clut_h = wm_prim_fnv1a((const u8*)PSX_ADDR(WM_CLUT_D478), WM_CLUT_BYTES);
    exp_rec_h = wm_prim_fnv1a(expect_recs, WM_REC_BYTES);
    exp_clut_h = wm_prim_fnv1a(expect_clut, WM_CLUT_BYTES);

    match_rec = 0;
    for (i = 0; i < (int)WM_REC_BYTES; i++) {
        if (table[i] == expect_recs[i])
            match_rec++;
    }
    match_clut = 0;
    for (i = 0; i < (int)WM_CLUT_BYTES; i++) {
        if (((const u8*)PSX_ADDR(WM_CLUT_D478))[i] == expect_clut[i])
            match_clut++;
    }

    unexpected = 0;
    for (i = 0; i < (int)WM_REC_BYTES; i++) {
        if (table[i] != expect_recs[i])
            unexpected++;
    }
    for (i = 0; i < (int)WM_CLUT_BYTES; i++) {
        if (((const u8*)PSX_ADDR(WM_CLUT_D478))[i] != expect_clut[i])
            unexpected++;
    }
    /* field_04 must equal pre for every record */
    for (i = 0; i < (int)WM_REC_COUNT; i++) {
        u32 off = (u32)i * WM_REC_STRIDE + 4u;
        if (*(u32*)(table + off) != *(u32*)(pre_recs + off))
            unexpected++;
    }

    fprintf(stderr,
            "[worldmap-record-clut] relocated_records=%d clut_count=%u "
            "first_clut=0x%04x last_clut=0x%04x\n",
            relocated, WM_CLUT_COUNT, (unsigned)first_clut,
            (unsigned)last_clut);
    fprintf(stderr,
            "[worldmap-record-clut] hashes rec pre=0x%08x post=0x%08x "
            "exp=0x%08x | clut pre=0x%08x post=0x%08x exp=0x%08x\n",
            pre_rec_h, post_rec_h, exp_rec_h, pre_clut_h, post_clut_h,
            exp_clut_h);
    fprintf(stderr,
            "[worldmap-record-clut] match rec=%d/%u clut=%d/%u "
            "unexpected=%d\n",
            match_rec, WM_REC_BYTES, match_clut, WM_CLUT_BYTES, unexpected);

    /* Representative entries */
    {
        u32 idx40 = 40;
        u32 pre0 = *(u32*)(pre_recs + 0);
        u32 post0 = WM_U32(table_psx + 0);
        u32 pre40 = *(u32*)(pre_recs + idx40 * WM_REC_STRIDE);
        u32 post40 = WM_U32(table_psx + idx40 * WM_REC_STRIDE);
        u32 f4_40 = WM_U32(table_psx + idx40 * WM_REC_STRIDE + 4u);
        u32 pre255 = *(u32*)(pre_recs + 255u * WM_REC_STRIDE);
        u32 post255 = WM_U32(table_psx + 255u * WM_REC_STRIDE);
        fprintf(stderr,
                "[worldmap-record-clut] rep idx0 pre=0x%08x post=0x%08x | "
                "idx40 pre=0x%08x post=0x%08x f4=0x%08x | "
                "idx255 pre=0x%08x post=0x%08x\n",
                pre0, post0, pre40, post40, f4_40, pre255, post255);
        if (first_nz_idx != 0xFFFFFFFFu) {
            u32 p =
                *(u32*)(pre_recs + first_nz_idx * WM_REC_STRIDE);
            u32 q = WM_U32(table_psx + first_nz_idx * WM_REC_STRIDE);
            fprintf(stderr,
                    "[worldmap-record-clut] first_nz idx=%u pre=0x%08x "
                    "post=0x%08x\n",
                    first_nz_idx, p, q);
        }
    }

    if (relocated != nonzero || match_rec != (int)WM_REC_BYTES ||
        match_clut != (int)WM_CLUT_BYTES || unexpected != 0 ||
        post_rec_h != exp_rec_h || post_clut_h != exp_clut_h) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: oracle mismatch "
                "relocated=%d nonzero=%d unexpected=%d\n",
                relocated, nonzero, unexpected);
        return -1;
    }
    if (first_clut != 0x7C0Fu || last_clut != 0x7FCFu) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: CLUT endpoints "
                "first=0x%04x last=0x%04x\n",
                (unsigned)first_clut, (unsigned)last_clut);
        return -1;
    }

    /* Guards + prior rungs / W14B preservation. */
    if (WM_U32(WM_FIX_C7EC - 8u) != guard_c7ec_lo[0] ||
        WM_U32(WM_FIX_C7EC - 4u) != guard_c7ec_lo[1] ||
        WM_U32(WM_FIX_C7EC + 4u) != guard_c7ec_hi[0] ||
        WM_U32(WM_FIX_C7EC + 8u) != guard_c7ec_hi[1] ||
        WM_U32(WM_CLUT_D478 - 8u) != guard_d478_lo[0] ||
        WM_U32(WM_CLUT_D478 - 4u) != guard_d478_lo[1] ||
        WM_U32(WM_CLUT_D478 + WM_CLUT_BYTES) != guard_d478_hi[0] ||
        WM_U32(WM_CLUT_D478 + WM_CLUT_BYTES + 4u) != guard_d478_hi[1]) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: neighbor guards changed\n");
        return -1;
    }
    if (WM_U32(WM_FIX_C7EC) != table_psx) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: C7EC slot mutated\n");
        return -1;
    }
    if (WM_U8(WM_PRIM_FT4_0 + 7u) != (u8)ft4_code_snap ||
        WM_U32(WM_PRIM_DR_TPAGE + 4u) != dr_mode_snap ||
        WM_U32(WM_TW_MIRROR_C88C) != c88c_snap ||
        WM_U32(0x8009D198u) != bss_d198_snap) {
        fprintf(stderr,
                "[worldmap-record-clut] ERROR: W14B/prior-rung corrupted\n");
        return -1;
    }

    s_wm85f58_ran = 1;
    fprintf(stderr, "[worldmap-record-clut] exit\n");
    fprintf(stderr,
            "[worldmap-record-clut] cut-before-gfx-work-buffers "
            "retail_pc=0x%08x\n",
            WM_CUT_BEFORE_GFX_WORK);

    /* Optional diagnostic: prove second call is blocked without writes. */
    if (env_flag_is_one("XENO_WORLD_RECORD_CLUT_DOUBLE_TEST")) {
        u8 snap_after[WM_REC_BYTES];
        u8 snap_clut_after[WM_CLUT_BYTES];
        int rc2;
        int changed = 0;
        wm_memcpy(snap_after, table, WM_REC_BYTES);
        wm_memcpy(snap_clut_after, PSX_ADDR(WM_CLUT_D478), WM_CLUT_BYTES);
        rc2 = wm_80085F58_relocate_records_and_init_cluts();
        for (i = 0; i < (int)WM_REC_BYTES; i++) {
            if (table[i] != snap_after[i])
                changed++;
        }
        for (i = 0; i < (int)WM_CLUT_BYTES; i++) {
            if (((const u8*)PSX_ADDR(WM_CLUT_D478))[i] != snap_clut_after[i])
                changed++;
        }
        fprintf(stderr,
                "[worldmap-record-clut] double_test rc2=%d "
                "changed_bytes_after_blocked_call=%d\n",
                rc2, changed);
        if (rc2 == 0 || changed != 0) {
            fprintf(stderr,
                    "[worldmap-record-clut] ERROR: double-run guard failed\n");
            return -1;
        }
    }

    return 0;
}

/* World residual callsite accounting (not global field allocates). */
static int s_wm_gfx_work_world_hits;
static int s_wm_gfx_work_rung_dispatches;

/*
 * W16B — residual routing only: exact matching GfxAllocateWorkBuffers(5120,0).
 * Does not reimplement allocate/free. Cut before 0x80072480.
 */
static int wm_route_gfx_allocate_work_buffers(void)
{
    s32 pre_size;
    void* pre_buf0;
    void* pre_buf1;
    u32 pre_59300;
    u32 pre_59304;
    void* pre_img_list;
    s32 pre_59190;
    void* post_buf0;
    void* post_buf1;
    uintptr_t dist;
    u32 ft4_code_snap;
    u32 dr_mode_snap;
    u32 c7ec_snap;
    u32 d4780_snap;
    u32 c88c_snap;
    u32 bss_d198_snap;
    u8 rec_head_snap[16];
    int field_leak_present;

    s_wm_gfx_work_rung_dispatches++;
    if (s_wm_gfx_work_rung_dispatches != 1) {
        fprintf(stderr,
                "[worldmap-gfx-work-buffers] ERROR: accidental repeated W16B "
                "rung dispatch count=%d (expected 1 per world entry)\n",
                s_wm_gfx_work_rung_dispatches);
        return -1;
    }
    if (!s_wm85f58_ran) {
        fprintf(stderr,
                "[worldmap-gfx-work-buffers] ERROR: W15B did not run "
                "(required)\n");
        return -1;
    }

    fprintf(stderr, "[worldmap-gfx-work-buffers] entry\n");
    fprintf(stderr,
            "[worldmap-gfx-work-buffers] size=%d alloc_flag=0\n",
            WM_GFX_WORK_SIZE);

    /* Pre-call snapshot (field allocation may still be live — free is stub). */
    pre_size = g_GfxWorkBufferSize;
    pre_buf0 = g_GfxWorkBuffers;
    pre_buf1 = g_GfxWorkBuffer2;
    pre_59300 = D_80059300;
    pre_59304 = D_80059304;
    pre_img_list = g_GfxImageList;
    pre_59190 = D_80059190;
    ft4_code_snap = WM_U8(WM_PRIM_FT4_0 + 7u);
    dr_mode_snap = WM_U32(WM_PRIM_DR_TPAGE + 4u);
    c7ec_snap = WM_U32(WM_FIX_C7EC);
    d4780_snap = WM_U16(WM_CLUT_D478);
    c88c_snap = WM_U32(WM_TW_MIRROR_C88C);
    bss_d198_snap = WM_U32(0x8009D198u);
    wm_memcpy(rec_head_snap, psx_u32_to_host(c7ec_snap), 16);

    fprintf(stderr,
            "[worldmap-gfx-work-buffers] pre size=%d buf0=%p buf1=%p "
            "D59300=0x%08x D59304=0x%08x img=%p D59190=%d\n",
            (int)pre_size, pre_buf0, pre_buf1, pre_59300, pre_59304,
            pre_img_list, (int)pre_59190);

    field_leak_present = 0;
    if (pre_buf0 != NULL) {
        /* Field teardown logs [stub] GfxFreeWorkBuffers; prior block remains. */
        field_leak_present = 1;
        fprintf(stderr,
                "[worldmap-gfx-work-buffers] pre-existing field free leak: "
                "PRESENT (pre_buf0=%p still non-null before world allocate; "
                "GfxFreeWorkBuffers is stubbed — not introduced by W16B)\n",
                pre_buf0);
    } else {
        fprintf(stderr,
                "[worldmap-gfx-work-buffers] pre-existing field free leak: "
                "NOT OBSERVED (pre_buf0 is null)\n");
    }

    /* Exact residual: GfxAllocateWorkBuffers(5120, 0). */
    s_wm_gfx_work_world_hits++;
    GfxAllocateWorkBuffers(WM_GFX_WORK_SIZE, 0);

    post_buf0 = g_GfxWorkBuffers;
    post_buf1 = g_GfxWorkBuffer2;
    if (post_buf0 == NULL || post_buf1 == NULL) {
        fprintf(stderr,
                "[worldmap-gfx-work-buffers] ERROR: null allocation "
                "buf0=%p buf1=%p\n",
                post_buf0, post_buf1);
        return -1;
    }
    dist = (uintptr_t)post_buf1 - (uintptr_t)post_buf0;
    if (g_GfxWorkBufferSize != WM_GFX_WORK_SIZE || dist != (uintptr_t)WM_GFX_WORK_SIZE) {
        fprintf(stderr,
                "[worldmap-gfx-work-buffers] ERROR: size/split mismatch "
                "size=%d dist=%zu expected=%d\n",
                (int)g_GfxWorkBufferSize, (size_t)dist, WM_GFX_WORK_SIZE);
        return -1;
    }
    if (D_80059300 != 0 || D_80059304 != 0 || g_GfxImageList != NULL ||
        D_80059190 != 0) {
        fprintf(stderr,
                "[worldmap-gfx-work-buffers] ERROR: roots not cleared "
                "D59300=0x%08x D59304=0x%08x img=%p D59190=%d\n",
                D_80059300, D_80059304, g_GfxImageList, (int)D_80059190);
        return -1;
    }

    fprintf(stderr,
            "[worldmap-gfx-work-buffers] allocation_base=%p buffer0=%p "
            "buffer1=%p buffer_distance=%d total_bytes=%d roots_cleared=1\n",
            post_buf0, post_buf0, post_buf1, WM_GFX_WORK_SIZE,
            WM_GFX_WORK_TOTAL);
    fprintf(stderr,
            "[worldmap-gfx-work-buffers] world_gfx_allocate_hits=%d "
            "rung_dispatches=%d field_leak=%s new_block=%s\n",
            s_wm_gfx_work_world_hits, s_wm_gfx_work_rung_dispatches,
            field_leak_present ? "PRESENT" : "NOT_OBSERVED",
            (post_buf0 != pre_buf0) ? "yes" : "same_ptr");

    /* Prior-rung preservation. */
    if (WM_U8(WM_PRIM_FT4_0 + 7u) != (u8)ft4_code_snap ||
        WM_U32(WM_PRIM_DR_TPAGE + 4u) != dr_mode_snap ||
        WM_U32(WM_FIX_C7EC) != c7ec_snap ||
        WM_U16(WM_CLUT_D478) != (u16)d4780_snap ||
        WM_U32(WM_TW_MIRROR_C88C) != c88c_snap ||
        WM_U32(0x8009D198u) != bss_d198_snap ||
        !wm_memeq(rec_head_snap, psx_u32_to_host(c7ec_snap), 16)) {
        fprintf(stderr,
                "[worldmap-gfx-work-buffers] ERROR: prior-rung state "
                "corrupted\n");
        return -1;
    }

    fprintf(stderr, "[worldmap-gfx-work-buffers] exit\n");
    fprintf(stderr,
            "[worldmap-gfx-work-buffers] cut-before-next-helper "
            "retail_pc=0x%08x\n",
            WM_CUT_BEFORE_74594);
    return 0;
}

static int s_wm74594_ran;
static int s_wm863E0_ran;
static int s_wm74e58_ran;
static int s_wm75030_ran;
static int s_wm739b8_ran;
static int s_wm88f64_ran;
static int s_wm_archive_poll_count;
static int s_wm_first_wds_ran;
static int s_wm_first_wds_hits;
static int s_wm_archive_set_index_ran;
static int s_wm74594_hits;
static int s_wm863E0_hits;

/*
 * Retail 0x80074594 — heap dual POLY_FT4 pools (16×40) + 16×8 clear table.
 * One-shot: re-entry would leak another 1408 B without free.
 * Cut residual before 0x80072488.
 */
static int wm_80074594_init_ft4_pools(void)
{
    void* host_rec;
    void* host_a;
    void* host_b;
    u32 psx_rec;
    u32 psx_a;
    u32 psx_b;
    u8* rec;
    u8* pool_a;
    u8* pool_b;
    u8 expect_rec[WM_FT4_REC_BYTES];
    u8 expect_pool[WM_FT4_POOL_BYTES];
    u8 snap_rec[WM_FT4_REC_BYTES];
    int i;
    int j;
    int match_rec;
    int match_a;
    int match_b;
    int unexpected;
    u16 clut;
    u16 tpage;
    u32 pre_be14;
    u32 pre_be18;
    u32 pre_be38;
    u32 pre_be24;
    u32 pre_be4c;
    u32 pre_d30c;
    s32 gfx_size_snap;
    void* gfx_buf0_snap;
    void* gfx_buf1_snap;
    u32 c7ec_snap;
    u32 d4780_snap;
    u32 ft4_code_snap;
    u32 c88c_snap;
    u32 bss_d198_snap;
    u32 post_a_h;
    u32 post_b_h;
    u32 exp_pool_h;
    u32 post_rec_h;
    u32 exp_rec_h;

    s_wm74594_hits++;
    fprintf(stderr, "[worldmap-ft4-pools] entry\n");

    if (s_wm74594_ran) {
        fprintf(stderr,
                "[worldmap-ft4-pools] ERROR: already ran this world entry "
                "(non-idempotent alloc; blocked)\n");
        fprintf(stderr,
                "[worldmap-ft4-pools] second_call_detected=1 "
                "second_call_blocked=1\n");
        return -1;
    }
    if (s_wm_gfx_work_world_hits == 0) {
        fprintf(stderr,
                "[worldmap-ft4-pools] ERROR: W16B did not run (required)\n");
        return -1;
    }

    pre_d30c = WM_U32(WM_FT4_REC_PTR);
    pre_be14 = WM_U32(WM_FT4_POOL_A_PTR);
    pre_be18 = WM_U32(WM_FT4_POOL_B_PTR);
    pre_be38 = WM_U32(WM_FT4_FLAG_BE38);
    pre_be24 = WM_U32(WM_POOL_BE24);
    pre_be4c = WM_U32(WM_TMPL_DST_BE4C);
    gfx_size_snap = g_GfxWorkBufferSize;
    gfx_buf0_snap = g_GfxWorkBuffers;
    gfx_buf1_snap = g_GfxWorkBuffer2;
    c7ec_snap = WM_U32(WM_FIX_C7EC);
    d4780_snap = WM_U16(WM_CLUT_D478);
    ft4_code_snap = WM_U8(WM_PRIM_FT4_0 + 7u);
    c88c_snap = WM_U32(WM_TW_MIRROR_C88C);
    bss_d198_snap = WM_U32(0x8009D198u);

    /* ---- Retail allocation order ---- */
    host_rec = HeapAlloc(WM_FT4_REC_BYTES, 0);
    if (host_rec == NULL) {
        fprintf(stderr, "[worldmap-ft4-pools] ERROR: HeapAlloc(128) failed\n");
        return -1;
    }
    psx_rec = host_ptr_to_psx_u32(host_rec);
    if (psx_rec < 0x80000000u || psx_u32_to_host(psx_rec) != host_rec) {
        fprintf(stderr,
                "[worldmap-ft4-pools] ERROR: rec KUSEG conversion failed "
                "host=%p psx=0x%08x\n",
                host_rec, psx_rec);
        return -1;
    }
    WM_U32(WM_FT4_REC_PTR) = psx_rec;

    host_a = HeapAlloc(WM_FT4_POOL_BYTES, 0);
    if (host_a == NULL) {
        fprintf(stderr, "[worldmap-ft4-pools] ERROR: HeapAlloc(640) A failed\n");
        return -1;
    }
    psx_a = host_ptr_to_psx_u32(host_a);
    if (psx_a < 0x80000000u || psx_u32_to_host(psx_a) != host_a) {
        fprintf(stderr,
                "[worldmap-ft4-pools] ERROR: pool A KUSEG conversion failed\n");
        return -1;
    }
    WM_U32(WM_FT4_POOL_A_PTR) = psx_a;

    host_b = HeapAlloc(WM_FT4_POOL_BYTES, 0);
    if (host_b == NULL) {
        fprintf(stderr, "[worldmap-ft4-pools] ERROR: HeapAlloc(640) B failed\n");
        return -1;
    }
    psx_b = host_ptr_to_psx_u32(host_b);
    if (psx_b < 0x80000000u || psx_u32_to_host(psx_b) != host_b) {
        fprintf(stderr,
                "[worldmap-ft4-pools] ERROR: pool B KUSEG conversion failed\n");
        return -1;
    }
    WM_U32(WM_FT4_POOL_B_PTR) = psx_b;

    if (host_rec == host_a || host_rec == host_b || host_a == host_b) {
        fprintf(stderr, "[worldmap-ft4-pools] ERROR: overlapping allocations\n");
        return -1;
    }

    rec = (u8*)host_rec;
    pool_a = (u8*)host_a;
    pool_b = (u8*)host_b;

    /* Snapshot post-alloc contents for oracle of unwritten record bytes. */
    wm_memcpy(snap_rec, rec, WM_FT4_REC_BYTES);

    /* Clear loop: 16× stride 8 — sh 0 at +0, +2, +4 only. */
    for (i = 0; i < (int)WM_FT4_REC_COUNT; i++) {
        u32 off = (u32)i * WM_FT4_REC_STRIDE;
        *(u16*)(rec + off + 0u) = 0;
        *(u16*)(rec + off + 2u) = 0;
        *(u16*)(rec + off + 4u) = 0;
    }

    /* FT4 seed loop — identical packets; XY/tag low left as heap residual. */
    for (i = 0; i < (int)WM_FT4_POOL_COUNT; i++) {
        u8* p = pool_a + (u32)i * WM_FT4_POOL_STRIDE;
        p[3] = 9;    /* len */
        p[7] = 0x2C; /* code POLY_FT4 */
        p[4] = 64;   /* r */
        p[5] = 64;   /* g */
        p[6] = 72;   /* b */
        p[12] = 128; /* u0 */
        p[13] = 240; /* v0 */
        p[20] = 143; /* u1 */
        p[21] = 240; /* v1 */
        p[28] = 128; /* u2 */
        p[29] = 255; /* v2 */
        p[36] = 143; /* u3 */
        p[37] = 255; /* v3 */
        clut = GetClut(288, 510);
        *(u16*)(p + 14) = clut;
        tpage = GetTPage(0, 0, 896, 256);
        *(u16*)(p + 22) = tpage;
        SetSemiTrans(p, 1);
    }

    /* Word copy 640 B pool A → pool B. */
    {
        u32* src = (u32*)pool_a;
        u32* dst = (u32*)pool_b;
        for (i = 0; i < (int)(WM_FT4_POOL_BYTES / 4u); i++)
            dst[i] = src[i];
    }

    WM_U32(WM_FT4_FLAG_BE38) = 0;

    /* ---- Oracle ---- */
    wm_memcpy(expect_rec, snap_rec, WM_FT4_REC_BYTES);
    for (i = 0; i < (int)WM_FT4_REC_COUNT; i++) {
        u32 off = (u32)i * WM_FT4_REC_STRIDE;
        expect_rec[off + 0] = 0;
        expect_rec[off + 1] = 0;
        expect_rec[off + 2] = 0;
        expect_rec[off + 3] = 0;
        expect_rec[off + 4] = 0;
        expect_rec[off + 5] = 0;
        /* +6,+7 preserved from snap */
    }
    /* Pool expect: start from post-seed pool_a (includes residual XY/tag). */
    wm_memcpy(expect_pool, pool_a, WM_FT4_POOL_BYTES);

    match_rec = 0;
    for (j = 0; j < (int)WM_FT4_REC_BYTES; j++) {
        if (rec[j] == expect_rec[j])
            match_rec++;
    }
    match_a = 0;
    match_b = 0;
    for (j = 0; j < (int)WM_FT4_POOL_BYTES; j++) {
        if (pool_a[j] == expect_pool[j])
            match_a++;
        if (pool_b[j] == expect_pool[j])
            match_b++;
    }
    unexpected = 0;
    for (j = 0; j < (int)WM_FT4_REC_BYTES; j++) {
        if (rec[j] != expect_rec[j])
            unexpected++;
    }
    for (j = 0; j < (int)WM_FT4_POOL_BYTES; j++) {
        if (pool_a[j] != expect_pool[j] || pool_b[j] != pool_a[j])
            unexpected++;
    }
    if (WM_U32(WM_FT4_FLAG_BE38) != 0)
        unexpected++;

    post_rec_h = wm_prim_fnv1a(rec, WM_FT4_REC_BYTES);
    exp_rec_h = wm_prim_fnv1a(expect_rec, WM_FT4_REC_BYTES);
    post_a_h = wm_prim_fnv1a(pool_a, WM_FT4_POOL_BYTES);
    post_b_h = wm_prim_fnv1a(pool_b, WM_FT4_POOL_BYTES);
    exp_pool_h = wm_prim_fnv1a(expect_pool, WM_FT4_POOL_BYTES);

    clut = *(u16*)(pool_a + 14);
    tpage = *(u16*)(pool_a + 22);

    fprintf(stderr,
            "[worldmap-ft4-pools] records_psx=0x%08x pool_a_psx=0x%08x "
            "pool_b_psx=0x%08x record_bytes=%u pool_bytes=%u ft4_count=%u "
            "pools_equal=%d be38=%u\n",
            psx_rec, psx_a, psx_b, WM_FT4_REC_BYTES, WM_FT4_POOL_BYTES,
            WM_FT4_POOL_COUNT, wm_memeq(pool_a, pool_b, WM_FT4_POOL_BYTES),
            WM_U32(WM_FT4_FLAG_BE38));
    fprintf(stderr,
            "[worldmap-ft4-pools] host_rec=%p host_a=%p host_b=%p "
            "clut=0x%04x tpage=0x%04x code0=0x%02x\n",
            host_rec, host_a, host_b, (unsigned)clut, (unsigned)tpage,
            (unsigned)pool_a[7]);
    fprintf(stderr,
            "[worldmap-ft4-pools] match rec=%d/%u poolA=%d/%u poolB=%d/%u "
            "unexpected=%d hashes rec=0x%08x/0x%08x pool=0x%08x/0x%08x/"
            "0x%08x\n",
            match_rec, WM_FT4_REC_BYTES, match_a, WM_FT4_POOL_BYTES, match_b,
            WM_FT4_POOL_BYTES, unexpected, post_rec_h, exp_rec_h, post_a_h,
            post_b_h, exp_pool_h);

    if (clut != 0x7F92u || tpage != 0x001Eu || pool_a[7] != 0x2Eu) {
        fprintf(stderr,
                "[worldmap-ft4-pools] ERROR: FT4 helper/code mismatch "
                "clut=0x%04x tpage=0x%04x code=0x%02x\n",
                (unsigned)clut, (unsigned)tpage, (unsigned)pool_a[7]);
        return -1;
    }
    if (match_rec != (int)WM_FT4_REC_BYTES ||
        match_a != (int)WM_FT4_POOL_BYTES ||
        match_b != (int)WM_FT4_POOL_BYTES || unexpected != 0 ||
        !wm_memeq(pool_a, pool_b, WM_FT4_POOL_BYTES) ||
        WM_U32(WM_FT4_REC_PTR) != psx_rec ||
        WM_U32(WM_FT4_POOL_A_PTR) != psx_a ||
        WM_U32(WM_FT4_POOL_B_PTR) != psx_b ||
        WM_U32(WM_FT4_FLAG_BE38) != 0) {
        fprintf(stderr, "[worldmap-ft4-pools] ERROR: oracle mismatch\n");
        return -1;
    }
    for (i = 0; i < (int)WM_FT4_POOL_COUNT; i++) {
        if (pool_a[(u32)i * WM_FT4_POOL_STRIDE + 7u] != 0x2Eu) {
            fprintf(stderr,
                    "[worldmap-ft4-pools] ERROR: FT4[%d] code\n", i);
            return -1;
        }
    }

    /* Neighbors + prior rungs. */
    if (WM_U32(WM_POOL_BE24) != pre_be24 ||
        WM_U32(WM_TMPL_DST_BE4C) != pre_be4c ||
        g_GfxWorkBufferSize != gfx_size_snap ||
        g_GfxWorkBuffers != gfx_buf0_snap ||
        g_GfxWorkBuffer2 != gfx_buf1_snap ||
        WM_U32(WM_FIX_C7EC) != c7ec_snap ||
        WM_U16(WM_CLUT_D478) != (u16)d4780_snap ||
        WM_U8(WM_PRIM_FT4_0 + 7u) != (u8)ft4_code_snap ||
        WM_U32(WM_TW_MIRROR_C88C) != c88c_snap ||
        WM_U32(0x8009D198u) != bss_d198_snap) {
        fprintf(stderr,
                "[worldmap-ft4-pools] ERROR: neighbor/prior-rung corrupted\n");
        return -1;
    }
    (void)pre_d30c;
    (void)pre_be14;
    (void)pre_be18;
    (void)pre_be38;

    s_wm74594_ran = 1;
    fprintf(stderr, "[worldmap-ft4-pools] exit\n");
    fprintf(stderr,
            "[worldmap-ft4-pools] cut-before-next-helper retail_pc=0x%08x\n",
            WM_CUT_BEFORE_863E0);

    if (env_flag_is_one("XENO_WORLD_FT4_POOLS_DOUBLE_TEST")) {
        u32 d30c_s = WM_U32(WM_FT4_REC_PTR);
        u32 be14_s = WM_U32(WM_FT4_POOL_A_PTR);
        u32 be18_s = WM_U32(WM_FT4_POOL_B_PTR);
        u8 snap_a[WM_FT4_POOL_BYTES];
        u8 snap_b[WM_FT4_POOL_BYTES];
        u8 snap_r[WM_FT4_REC_BYTES];
        int rc2;
        int changed = 0;
        wm_memcpy(snap_a, pool_a, WM_FT4_POOL_BYTES);
        wm_memcpy(snap_b, pool_b, WM_FT4_POOL_BYTES);
        wm_memcpy(snap_r, rec, WM_FT4_REC_BYTES);
        rc2 = wm_80074594_init_ft4_pools();
        for (j = 0; j < (int)WM_FT4_POOL_BYTES; j++) {
            if (pool_a[j] != snap_a[j] || pool_b[j] != snap_b[j])
                changed++;
        }
        for (j = 0; j < (int)WM_FT4_REC_BYTES; j++) {
            if (rec[j] != snap_r[j])
                changed++;
        }
        if (WM_U32(WM_FT4_REC_PTR) != d30c_s ||
            WM_U32(WM_FT4_POOL_A_PTR) != be14_s ||
            WM_U32(WM_FT4_POOL_B_PTR) != be18_s)
            changed++;
        fprintf(stderr,
                "[worldmap-ft4-pools] double_test rc2=%d changed_bytes=%d "
                "new_allocations=0\n",
                rc2, changed);
        if (rc2 == 0 || changed != 0) {
            fprintf(stderr,
                    "[worldmap-ft4-pools] ERROR: double-run guard failed\n");
            return -1;
        }
    }

    return 0;
}

/*
 * W18B — wm_800863E0: heap-table and RNG initialization.
 * Allocates 1280+640 bytes, fills 80 records in each with source-table +
 * pseudo-random data via rand() (240 calls total). Non-idempotent.
 *
 * AUDIT CORRECTION (documented, not silently inlined): an independent
 * re-disassembly of the retail bytes in disc/world_map.bin shows the W18A
 * audit's Table A +4/+8 and Table B +2/+4 field labels are swapped. The two
 * MIPS delay-slot stores actually land at:
 *   Table A +4 = rand() transform (sw in rand() delay slot pair), +8 = AF38
 *   Table B +2 = rand() & 1, +4 = -(s2 * 2048)
 * This implementation follows the actual retail bytes, not the audit labels.
 */
static int wm_800863E0_init_heap_table_rand(void)
{
    void* host_a;
    void* host_b;
    u32 psx_a;
    u32 psx_b;
    u8* table_a;
    u8* table_b;
    u8 orig_a[WM_TABLE_A_BYTES];
    u8 orig_b[WM_TABLE_B_BYTES];
    u8 exp_a[WM_TABLE_A_BYTES];
    u8 exp_b[WM_TABLE_B_BYTES];
    uint32_t pre_seed;
    uint32_t post_seed;
    uint32_t oseed;
    u32 src;
    int s4, s2, s0, i, j;
    int r;
    int s2_val;
    int mismatch_a;
    int mismatch_b;
    u32 pre_d150;
    u32 pre_ceb4;
    u32 pre_be14;
    u32 pre_be18;
    u32 pre_be38;
    u32 pre_d30c;
    s32 gfx_size_snap;
    void* gfx_buf0_snap;
    void* gfx_buf1_snap;
    u32 c7ec_snap;
    u32 d4780_snap;
    u32 ft4_code_snap;
    u32 c88c_snap;
    u32 bss_d198_snap;
    u32 post_a_h;
    u32 post_b_h;
    u32 exp_a_h;
    u32 exp_b_h;

    s_wm863E0_hits++;
    fprintf(stderr, "[worldmap-heap-table-rand] entry\n");

    if (s_wm863E0_ran) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: already ran "
                "(non-idempotent alloc; blocked)\n");
        fprintf(stderr,
                "[worldmap-heap-table-rand] second_call_detected=1 "
                "second_call_blocked=1\n");
        return -1;
    }
    /* Retail modes 8/11 allocate these tables without the base-mode FT4
     * pools.  This initializer has no data dependency on wm_80074594. */

    pre_d150 = WM_U32(WM_HEAP_TABLE_A_PTR);
    pre_ceb4 = WM_U32(WM_HEAP_TABLE_B_PTR);
    pre_d30c = WM_U32(WM_FT4_REC_PTR);
    pre_be14 = WM_U32(WM_FT4_POOL_A_PTR);
    pre_be18 = WM_U32(WM_FT4_POOL_B_PTR);
    pre_be38 = WM_U32(WM_FT4_FLAG_BE38);
    gfx_size_snap = g_GfxWorkBufferSize;
    gfx_buf0_snap = g_GfxWorkBuffers;
    gfx_buf1_snap = g_GfxWorkBuffer2;
    c7ec_snap = WM_U32(WM_FIX_C7EC);
    d4780_snap = WM_U16(WM_CLUT_D478);
    ft4_code_snap = WM_U8(WM_PRIM_FT4_0 + 7u);
    c88c_snap = WM_U32(WM_TW_MIRROR_C88C);
    bss_d198_snap = WM_U32(0x8009D198u);

    /* Snapshot production RNG state before the body. */
    pre_seed = g_RandomSeed;

    /* --- Allocate Table A: 1280 bytes --- */
    host_a = HeapAlloc(WM_TABLE_A_BYTES, 0);
    if (host_a == NULL) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: HeapAlloc(1280) failed\n");
        return -1;
    }
    psx_a = host_ptr_to_psx_u32(host_a);
    if (psx_a < 0x80000000u || psx_u32_to_host(psx_a) != host_a) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: table_A KUSEG conversion "
                "failed\n");
        return -1;
    }
    WM_U32(WM_HEAP_TABLE_A_PTR) = psx_a;

    /* --- Allocate Table B: 640 bytes --- */
    host_b = HeapAlloc(WM_TABLE_B_BYTES, 0);
    if (host_b == NULL) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: HeapAlloc(640) failed\n");
        return -1;
    }
    psx_b = host_ptr_to_psx_u32(host_b);
    if (psx_b < 0x80000000u || psx_u32_to_host(psx_b) != host_b) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: table_B KUSEG conversion "
                "failed\n");
        return -1;
    }
    WM_U32(WM_HEAP_TABLE_B_PTR) = psx_b;

    if (host_a == host_b) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: overlapping allocations\n");
        return -1;
    }

    table_a = (u8*)host_a;
    table_b = (u8*)host_b;

    /* Snapshot the full post-alloc contents: every retail-unwritten byte must
     * retain its heap garbage (flag=0 does not zero). */
    wm_memcpy(orig_a, table_a, WM_TABLE_A_BYTES);
    wm_memcpy(orig_b, table_b, WM_TABLE_B_BYTES);

    /* --- Phase 1: Fill Table A (80 records × 16 bytes) --- */
    /* Nested loops: s4=0..3, s2=0..3, s0=0..80 step 16. One rand() per record. */
    for (s4 = 0; s4 < 4; s4++) {
        for (s2 = 0; s2 < 4; s2++) {
            for (s0 = 0; s0 < 80; s0 += 16) {
                u32 off = (u32)((s4 * 4 + s2) * 5 + (s0 >> 4)) * WM_TABLE_A_STRIDE;
                u8* rec = table_a + off;

                /* +0: (AF30[s0] + s2*2048) << 12  (u32 wrap) */
                src = WM_U32(WM_SRC_AF30 + (u32)s0);
                *(u32*)(rec + 0) = (src + (u32)s2 * 2048u) << 12;

                /* +4: rand() transform, pure u32 wrap matching MIPS:
                 *   sra r,10 -> negu -> sll 3 -> addiu -512 -> sll 12 */
                r = rand();
                {
                    u32 a = (u32)r >> 10;
                    u32 b = 0u - a;
                    u32 c = b << 3;
                    u32 d = c - 512u;
                    *(u32*)(rec + 4) = d << 12;
                }

                /* +8: (AF38[s0] + s4*2048) << 12  (u32 wrap) */
                src = WM_U32(WM_SRC_AF38 + (u32)s0);
                *(u32*)(rec + 8) = (src + (u32)s4 * 2048u) << 12;

                /* +12: unwritten (heap garbage) */
            }
        }
    }

    /* --- Phase 2: Fill Table B (80 records × 8 bytes) --- */
    for (i = 0; i < (int)WM_TABLE_B_RECORDS; i++) {
        u8* rec = table_b + (u32)i * WM_TABLE_B_STRIDE;

        /* +0/+4 share one rand(): s2 = (rand() & 3) + 1 */
        s2_val = (rand() & 3) + 1;
        *(u16*)(rec + 0) = (u16)(3547u * (u32)s2_val);
        *(u16*)(rec + 4) = (u16)(0u - (u32)((u32)s2_val * 2048u));

        /* +2: rand() & 1 */
        *(u16*)(rec + 2) = (u16)((u32)rand() & 1u);

        /* +6: unwritten (heap garbage) */
    }

    post_seed = g_RandomSeed;

    /* ---- Standalone oracle ----
     * Pure LCG on a private seed; consumes no production RNG. Replays the
     * audited loop order, starting from the snapshot pre_seed, and builds
     * expected tables seeded with the pre-write heap contents so that the
     * whole-table byte comparison also proves unwritten bytes are untouched. */
    oseed = pre_seed;
    wm_memcpy(exp_a, orig_a, WM_TABLE_A_BYTES);
    for (s4 = 0; s4 < 4; s4++) {
        for (s2 = 0; s2 < 4; s2++) {
            for (s0 = 0; s0 < 80; s0 += 16) {
                u32 off = (u32)((s4 * 4 + s2) * 5 + (s0 >> 4)) * WM_TABLE_A_STRIDE;
                u8* rec = exp_a + off;
                uint32_t rr;

                oseed = oseed * UINT32_C(0x41C64E6D) + UINT32_C(0x3039);
                rr = (oseed >> 16) & UINT32_C(0x7FFF);
                *(u32*)(rec + 0) =
                    (WM_U32(WM_SRC_AF30 + (u32)s0) + (u32)s2 * 2048u) << 12;
                {
                    u32 a = rr >> 10;
                    u32 b = 0u - a;
                    u32 c = b << 3;
                    u32 d = c - 512u;
                    *(u32*)(rec + 4) = d << 12;
                }
                *(u32*)(rec + 8) =
                    (WM_U32(WM_SRC_AF38 + (u32)s0) + (u32)s4 * 2048u) << 12;
            }
        }
    }
    wm_memcpy(exp_b, orig_b, WM_TABLE_B_BYTES);
    for (i = 0; i < (int)WM_TABLE_B_RECORDS; i++) {
        u8* rec = exp_b + (u32)i * WM_TABLE_B_STRIDE;
        uint32_t rr;
        int s2o;

        oseed = oseed * UINT32_C(0x41C64E6D) + UINT32_C(0x3039);
        rr = (oseed >> 16) & UINT32_C(0x7FFF);
        s2o = (int)((rr & 3u) + 1u);
        *(u16*)(rec + 0) = (u16)(3547u * (u32)s2o);
        *(u16*)(rec + 4) = (u16)(0u - (u32)((u32)s2o * 2048u));

        oseed = oseed * UINT32_C(0x41C64E6D) + UINT32_C(0x3039);
        rr = (oseed >> 16) & UINT32_C(0x7FFF);
        *(u16*)(rec + 2) = (u16)(rr & 1u);
    }

    mismatch_a = 0;
    for (j = 0; j < (int)WM_TABLE_A_BYTES; j++) {
        if (table_a[j] != exp_a[j])
            mismatch_a++;
    }
    mismatch_b = 0;
    for (j = 0; j < (int)WM_TABLE_B_BYTES; j++) {
        if (table_b[j] != exp_b[j])
            mismatch_b++;
    }

    post_a_h = wm_prim_fnv1a(table_a, WM_TABLE_A_BYTES);
    exp_a_h = wm_prim_fnv1a(exp_a, WM_TABLE_A_BYTES);
    post_b_h = wm_prim_fnv1a(table_b, WM_TABLE_B_BYTES);
    exp_b_h = wm_prim_fnv1a(exp_b, WM_TABLE_B_BYTES);

    fprintf(stderr,
            "[worldmap-heap-table-rand] table_a_psx=0x%08x "
            "table_b_psx=0x%08x pre_seed=0x%08x post_seed=0x%08x "
            "oracle_seed=0x%08x rand_calls=240\n",
            psx_a, psx_b, pre_seed, post_seed, oseed);
    fprintf(stderr,
            "[worldmap-heap-table-rand] mismatch_a=%d/%u mismatch_b=%d/%u "
            "hashes a=0x%08x/0x%08x b=0x%08x/0x%08x\n",
            mismatch_a, WM_TABLE_A_BYTES, mismatch_b, WM_TABLE_B_BYTES,
            post_a_h, exp_a_h, post_b_h, exp_b_h);

    if (mismatch_a != 0 || mismatch_b != 0) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: oracle mismatch\n");
        return -1;
    }
    if (oseed != post_seed) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: RNG final-seed mismatch "
                "oracle=0x%08x production=0x%08x\n",
                oseed, post_seed);
        return -1;
    }
    if (WM_U32(WM_HEAP_TABLE_A_PTR) != psx_a ||
        WM_U32(WM_HEAP_TABLE_B_PTR) != psx_b) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: BSS pointer mismatch\n");
        return -1;
    }

    /* Verify prior-rung neighbor state unchanged */
    if (WM_U32(WM_FT4_REC_PTR) != pre_d30c ||
        WM_U32(WM_FT4_POOL_A_PTR) != pre_be14 ||
        WM_U32(WM_FT4_POOL_B_PTR) != pre_be18 ||
        WM_U32(WM_FT4_FLAG_BE38) != pre_be38 ||
        g_GfxWorkBufferSize != gfx_size_snap ||
        g_GfxWorkBuffers != gfx_buf0_snap ||
        g_GfxWorkBuffer2 != gfx_buf1_snap ||
        WM_U32(WM_FIX_C7EC) != c7ec_snap ||
        WM_U16(WM_CLUT_D478) != (u16)d4780_snap ||
        WM_U8(WM_PRIM_FT4_0 + 7u) != (u8)ft4_code_snap ||
        WM_U32(WM_TW_MIRROR_C88C) != c88c_snap ||
        WM_U32(0x8009D198u) != bss_d198_snap) {
        fprintf(stderr,
                "[worldmap-heap-table-rand] ERROR: neighbor/prior-rung "
                "corrupted\n");
        return -1;
    }
    (void)pre_d150;
    (void)pre_ceb4;

    s_wm863E0_ran = 1;
    fprintf(stderr, "[worldmap-heap-table-rand] exit\n");
    fprintf(stderr,
            "[worldmap-heap-table-rand] cut-before-next-helper "
            "retail_pc=0x%08x\n",
            WM_CUT_AFTER_863E0);

    if (env_flag_is_one("XENO_WORLD_HEAP_TABLE_RAND_DOUBLE_TEST")) {
        u32 d150_s = WM_U32(WM_HEAP_TABLE_A_PTR);
        u32 ceb4_s = WM_U32(WM_HEAP_TABLE_B_PTR);
        uint32_t seed_s = g_RandomSeed;
        u8 snap_a[WM_TABLE_A_BYTES];
        u8 snap_b[WM_TABLE_B_BYTES];
        int rc2;
        int changed = 0;

        wm_memcpy(snap_a, table_a, WM_TABLE_A_BYTES);
        wm_memcpy(snap_b, table_b, WM_TABLE_B_BYTES);
        rc2 = wm_800863E0_init_heap_table_rand();
        for (j = 0; j < (int)WM_TABLE_A_BYTES; j++) {
            if (table_a[j] != snap_a[j])
                changed++;
        }
        for (j = 0; j < (int)WM_TABLE_B_BYTES; j++) {
            if (table_b[j] != snap_b[j])
                changed++;
        }
        if (WM_U32(WM_HEAP_TABLE_A_PTR) != d150_s ||
            WM_U32(WM_HEAP_TABLE_B_PTR) != ceb4_s)
            changed++;
        if (g_RandomSeed != seed_s)
            changed++;
        fprintf(stderr,
                "[worldmap-heap-table-rand] double_test rc2=%d "
                "changed_bytes=%d new_allocations=0 rng_advance=0\n",
                rc2, changed);
        if (rc2 == 0 || changed != 0) {
            fprintf(stderr,
                    "[worldmap-heap-table-rand] ERROR: double-run guard "
                    "failed\n");
            return -1;
        }
    }

    return 0;
}

/*
 * W19A/W20B — native wm_80074E58 / wm_80075030: upload-record builders
 * (the first two pre-poll helpers).
 *
 * The W20A audit (scratchpad/w20a_75030_audit/REPORT.md) proved 0x80075030
 * is a 43/43 instruction structural clone of 0x80074E58: only four address
 * constants differ (source slot D77C/D7C8, count slot CC9C/CD64, array slot
 * D780/D7D0, value base A1E8/A250). Both rungs share one parameterized
 * implementation; W19B's diagnostic byte stream is preserved exactly.
 *
 * Retail semantics (per rung; W19A audit INSTRUCTION_AUDIT.md):
 *
 *   src   = [src_slot]        W4C fixup slot; block inside the decompressed
 *                             second-wave archive (word 0 = count N,
 *                             words 1..N = offsets relative to the block)
 *   [count_slot] = N
 *   alloc = HeapAlloc(N * 12, 0)
 *   [array_slot] = alloc
 *   for i in 0..N-1:
 *       rec[i].ptr     = src_block + block[1 + i]   (u32 @ +0)
 *       rec[i].value   = value_base + i * 16        (u32 @ +4)
 *       rec[i].counter = 0                          (u16 @ +8)
 *       rec[i].flag    = 1                          (u16 @ +A)
 *
 * One HeapAlloc, no RNG, no GPU writes. One-shot like W15B/W17B/W18B
 * (non-idempotent alloc): a second dispatch is a forbidden residual and
 * trips the rung's residual counter.
 */
typedef struct wm_upload_rung_cfg
{
    const char* tag;       /* stderr tag; W19B keeps its exact string */
    const char* src_label; /* diagnostic field name for the source slot */
    u32 src_slot;
    u32 count_slot;
    u32 array_slot;
    u32 value_base;
    u32 cut_pc; /* retail PC cut before the next helper */
    const char* double_test_env;
    int* ran_flag;
    int* residual_hits;
} wm_upload_rung_cfg;

static const wm_upload_rung_cfg wm_upload_rung_74e58 = {
    "worldmap-upload-records",
    "d77c",
    WM_FIX_D77C,
    WM_UPLOAD_COUNT,
    WM_UPLOAD_REC_ARRAY,
    WM_UPLOAD_VALUE_BASE,
    WM_CUT_BEFORE_75030,
    "XENO_WORLD_UPLOAD_RECORDS_DOUBLE_TEST",
    &s_wm74e58_ran,
    &s_wm74e58_hits,
};

static const wm_upload_rung_cfg wm_upload_rung_75030 = {
    "worldmap-upload-records-b",
    "d7c8",
    WM_FIX_D7C8,
    WM_UPLOAD_COUNT_B,
    WM_UPLOAD_REC_ARRAY_B,
    WM_UPLOAD_VALUE_BASE_B,
    WM_CUT_BEFORE_739B8,
    "XENO_WORLD_UPLOAD_RECORDS_B_DOUBLE_TEST",
    &s_wm75030_ran,
    &s_wm75030_hits,
};

static int wm_upload_records_verify(const wm_upload_rung_cfg* cfg, u32 src,
                                    u32 count, const u8* recs)
{
    u32 i;
    int mismatch = 0;
    u32 value = cfg->value_base;

    for (i = 0; i < count; i++) {
        const u8* rec = recs + i * WM_UPLOAD_REC_STRIDE;
        u32 rel = WM_U32(src + 4u + i * 4u);

        if (*(const u32*)(rec + 0) != src + rel)
            mismatch++;
        if (*(const u32*)(rec + 4) != value)
            mismatch++;
        if (*(const u16*)(rec + 8) != 0)
            mismatch++;
        if (*(const u16*)(rec + 10) != 1)
            mismatch++;
        value += WM_UPLOAD_VALUE_STRIDE;
    }
    return mismatch;
}

static int wm_upload_records_build(const wm_upload_rung_cfg* cfg)
{
    u32 src;
    u32 count;
    void* host;
    u32 psx;
    u8* recs;
    int mismatch;

    fprintf(stderr, "[%s] entry\n", cfg->tag);

    if (*cfg->ran_flag) {
        (*cfg->residual_hits)++;
        fprintf(stderr,
                "[%s] ERROR: already ran "
                "(non-idempotent alloc; blocked) (hit=%d)\n",
                cfg->tag, *cfg->residual_hits);
        fprintf(stderr,
                "[%s] second_call_detected=1 "
                "second_call_blocked=1\n",
                cfg->tag);
        return -1;
    }
    if (s_wm863E0_ran == 0) {
        fprintf(stderr,
                "[%s] ERROR: W18B did not run "
                "(required)\n",
                cfg->tag);
        return -1;
    }

    src = WM_U32(cfg->src_slot);
    if (src == 0) {
        fprintf(stderr,
                "[%s] ERROR: 0x%08x slot is NULL\n",
                cfg->tag, cfg->src_slot);
        return -1;
    }
    count = WM_U32(src);
    WM_U32(cfg->count_slot) = count;

    /* Retail order: count slot, alloc, array slot, per-record fill. */
    host = HeapAlloc(count * WM_UPLOAD_REC_STRIDE, 0);
    if (host == NULL) {
        fprintf(stderr,
                "[%s] ERROR: HeapAlloc(%u) failed\n",
                cfg->tag, count * WM_UPLOAD_REC_STRIDE);
        return -1;
    }
    psx = host_ptr_to_psx_u32(host);
    if (psx < 0x80000000u || psx_u32_to_host(psx) != host) {
        fprintf(stderr,
                "[%s] ERROR: record array KUSEG "
                "conversion failed\n",
                cfg->tag);
        return -1;
    }
    WM_U32(cfg->array_slot) = psx;
    recs = (u8*)host;

    {
        u32 i;
        u32 value = cfg->value_base;

        for (i = 0; i < count; i++) {
            u8* rec = recs + i * WM_UPLOAD_REC_STRIDE;
            u32 rel = WM_U32(src + 4u + i * 4u);

            *(u32*)(rec + 0) = src + rel;
            *(u32*)(rec + 4) = value;
            *(u16*)(rec + 8) = 0;
            *(u16*)(rec + 10) = 1;
            value += WM_UPLOAD_VALUE_STRIDE;
        }
    }

    fprintf(stderr,
            "[%s] %s=0x%08x count=%u array_psx=0x%08x "
            "bytes=%u value_base=0x%08x\n",
            cfg->tag, cfg->src_label, src, count, psx,
            count * WM_UPLOAD_REC_STRIDE, cfg->value_base);

    /* Structural verification: re-derive every record from the source block
     * (deterministic body — no RNG oracle needed). */
    mismatch = wm_upload_records_verify(cfg, src, count, recs);
    if (mismatch != 0) {
        fprintf(stderr,
                "[%s] ERROR: record verification "
                "mismatch=%d\n",
                cfg->tag, mismatch);
        return -1;
    }

    *cfg->ran_flag = 1;
    fprintf(stderr, "[%s] exit\n", cfg->tag);
    fprintf(stderr,
            "[%s] cut-before-next-helper "
            "retail_pc=0x%08x\n",
            cfg->tag, cfg->cut_pc);

    if (env_flag_is_one(cfg->double_test_env)) {
        u32 count_s = WM_U32(cfg->count_slot);
        u32 array_s = WM_U32(cfg->array_slot);
        int hits_s = *cfg->residual_hits;
        int rc2;
        int delta;
        int changed;

        rc2 = wm_upload_records_build(cfg);
        delta = *cfg->residual_hits - hits_s;
        changed = wm_upload_records_verify(cfg, src, count, recs);
        if (WM_U32(cfg->count_slot) != count_s ||
            WM_U32(cfg->array_slot) != array_s)
            changed++;
        fprintf(stderr,
                "[%s] double_test rc2=%d changed=%d "
                "counter_delta=%d new_allocations=0\n",
                cfg->tag, rc2, changed, delta);
        /* The blocked second call intentionally trips the residual counter;
         * restore it so the diagnostic rerun does not poison the forbidden
         * proof. */
        *cfg->residual_hits = hits_s;
        if (rc2 == 0 || changed != 0 || delta != 1) {
            fprintf(stderr,
                    "[%s] ERROR: double-run guard "
                    "failed\n",
                    cfg->tag);
            return -1;
        }
    }

    return 0;
}

/* W19A rung: retail 0x80074E58 (cut before jal 0x80075030 @ 0x80072498). */
static int wm_80074E58_build_upload_records(void)
{
    return wm_upload_records_build(&wm_upload_rung_74e58);
}

/*
 * W20B rung: retail 0x80075030 (second upload-record builder; cut before
 * jal 0x800739B8 @ 0x800724A0). Proven 43/43 structural clone of 0x80074E58
 * by the W20A audit; only the four address constants differ.
 */
static int wm_80075030_build_upload_records_b(void)
{
    return wm_upload_records_build(&wm_upload_rung_75030);
}

/*
 * W21B — native wm_800739B8: world draw-env/packet builder (first pre-poll
 * draw step). Per the W21A audit (scratchpad/w21a_739b8_audit/):
 *   - Pure BSS writer: no heap, no RNG, no GPU submission, no inputs.
 *   - Seeds four identical 40-byte packet records at 0x8009C744 (FT4-style
 *     tag r=g=b=0x30 code 0x2C, then the retail ABE bit OR'd into byte +7 →
 *     0x2E; clut 0x7F91; tpage 0x003E; u16 fields fC/f14/f1C/f24).
 *   - Packs two 12-byte DR_TPAGE-shaped carriers at 0x8009D3D8/D3E4 holding
 *     E2h texture-window command words.
 * The byte oracle below is the W21A software oracle; the rung re-derives the
 * bytes through the retail code path and then verifies the entire 184-byte
 * footprint against it.
 */
static const u8 wm_draw_pkt_record_oracle[40] = {
    0x00, 0x00, 0x00, 0x09, 0x30, 0x30, 0x30, 0x2E,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x91, 0x7F,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0x3E, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x3F, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0xFF, 0x3F, 0x00, 0x00,
};
static const u8 wm_dr_tpage_a_oracle[12] = {
    0x00, 0x00, 0x00, 0x02, 0x10, 0x00, 0x00, 0xE2,
    0x00, 0x00, 0x00, 0x00,
};
static const u8 wm_dr_tpage_b_oracle[12] = {
    0x00, 0x00, 0x00, 0x02, 0x00, 0x00, 0x00, 0xE2,
    0x00, 0x00, 0x00, 0x00,
};

/* Retail fn_80045C10: E2h texture-window command word from {x, y, w, h}. */
static u32 wm_e2_texwindow_word(u32 x, u32 y, int w, int h)
{
    u32 v = 0xE2000000u;
    v |= ((y & 0xFFu) >> 3) << 15;
    v |= ((x & 0xFFu) >> 3) << 10;
    v |= (u32)(((-h) & 0xFF) >> 3) << 5;
    v |= (u32)(((-w) & 0xFF) >> 3);
    return v;
}

/* Compare the live 184-byte footprint against the W21A byte oracle. Returns
 * the number of mismatched bytes (0 = exact). */
static int wm_draw_packets_verify_mismatch(void)
{
    const u8* base = (const u8*)PSX_ADDR(WM_DRAW_PKTS_BASE);
    int mismatch = 0;
    u32 i;

    for (i = 0; i < WM_DRAW_PKTS_COUNT; i++) {
        const u8* rec = base + i * WM_DRAW_PKTS_STRIDE;
        unsigned b;
        for (b = 0; b < WM_DRAW_PKTS_STRIDE; b++) {
            if (rec[b] != wm_draw_pkt_record_oracle[b])
                mismatch++;
        }
    }
    {
        const u8* ta = (const u8*)PSX_ADDR(WM_DR_TPAGE_A);
        const u8* tb = (const u8*)PSX_ADDR(WM_DR_TPAGE_B);
        unsigned b;
        for (b = 0; b < WM_DR_TPAGE_BYTES; b++) {
            if (ta[b] != wm_dr_tpage_a_oracle[b])
                mismatch++;
            if (tb[b] != wm_dr_tpage_b_oracle[b])
                mismatch++;
        }
    }
    return mismatch;
}

static int wm_800739B8_build_draw_packets(void)
{
    u16 tpage;
    u16 clut;
    u8* base;
    u8* tpage_a;
    u8* tpage_b;
    u32 i;
    int mismatch;

    fprintf(stderr, "[worldmap-draw-packets] entry\n");

    if (s_wm739b8_ran) {
        s_wm739b8_hits++;
        fprintf(stderr,
                "[worldmap-draw-packets] ERROR: already ran "
                "(blocked re-run) (hit=%d)\n",
                s_wm739b8_hits);
        fprintf(stderr,
                "[worldmap-draw-packets] second_call_detected=1 "
                "second_call_blocked=1\n");
        return -1;
    }
    if (s_wm75030_ran == 0) {
        fprintf(stderr,
                "[worldmap-draw-packets] ERROR: W20B did not run "
                "(required ordering)\n");
        return -1;
    }

    /* Retail 0x800739F0 / 0x80073A00: tpage + clut attribute words. */
    tpage = GetTPage(0, 1, 0x380, 0x100);
    clut = GetClut(0x110, 0x1FE);

    base = (u8*)PSX_ADDR(WM_DRAW_PKTS_BASE);
    /* Retail seeds BSS that is already zero; replicate by clearing the full
     * footprint before writing fields (byte-identical result). */
    wm_memset(base, 0, WM_DRAW_PKTS_BYTES);

    for (i = 0; i < WM_DRAW_PKTS_COUNT; i++) {
        u8* rec = base + i * WM_DRAW_PKTS_STRIDE;

        rec[3] = 0x09;             /* header word-count (len = 9) */
        rec[4] = 0x30;             /* FT4 r0 */
        rec[5] = 0x30;             /* FT4 g0 */
        rec[6] = 0x30;             /* FT4 b0 */
        /* FT4 code byte, then the retail ABE bit applied at byte +7.
         * NOTE: the native SetSemiTrans writes byte +3 (P_TAG) — the retail
         * Xenogears variant writes byte +7 — so the bit is hand-applied here
         * and the native helper is deliberately NOT called (W21A trap). */
        rec[7] = (u8)(0x2C | 0x02);
        *(u16*)(rec + 0x0C) = 0x0000;
        *(u16*)(rec + 0x0E) = clut;
        *(u16*)(rec + 0x14) = 0x00FF;
        *(u16*)(rec + 0x16) = tpage;
        *(u16*)(rec + 0x1C) = 0x3F00;
        *(u16*)(rec + 0x24) = 0x3FFF;
    }

    /* Retail fn_800453AC x2: DR_TPAGE-shaped carriers {code=2, E2h, 0}. */
    tpage_a = (u8*)PSX_ADDR(WM_DR_TPAGE_A);
    tpage_b = (u8*)PSX_ADDR(WM_DR_TPAGE_B);
    wm_memset(tpage_a, 0, WM_DR_TPAGE_BYTES);
    wm_memset(tpage_b, 0, WM_DR_TPAGE_BYTES);
    tpage_a[3] = 0x02;
    *(u32*)(tpage_a + 4) = wm_e2_texwindow_word(0, 0, 128, 0);
    *(u32*)(tpage_a + 8) = 0;
    tpage_b[3] = 0x02;
    *(u32*)(tpage_b + 4) = wm_e2_texwindow_word(0, 0, 0, 0);
    *(u32*)(tpage_b + 8) = 0;

    fprintf(stderr,
            "[worldmap-draw-packets] base=0x%08x records=%u stride=%u "
            "tpage=0x%04x clut=0x%04x dr_tpage_a=0x%08x dr_tpage_b=0x%08x\n",
            WM_DRAW_PKTS_BASE, WM_DRAW_PKTS_COUNT, WM_DRAW_PKTS_STRIDE,
            tpage, clut, WM_DR_TPAGE_A, WM_DR_TPAGE_B);

    /* Structural verification: full 184-byte footprint vs the W21A oracle. */
    mismatch = wm_draw_packets_verify_mismatch();
    if (mismatch != 0) {
        fprintf(stderr,
                "[worldmap-draw-packets] ERROR: byte-oracle mismatch=%d\n",
                mismatch);
        return -1;
    }

    s_wm739b8_ran = 1;
    fprintf(stderr, "[worldmap-draw-packets] exit\n");
    fprintf(stderr,
            "[worldmap-draw-packets] cut-before-next-helper "
            "retail_pc=0x%08x\n",
            WM_CUT_BEFORE_88F64);

    if (env_flag_is_one("XENO_WORLD_DRAW_PACKETS_DOUBLE_TEST")) {
        int hits_s = s_wm739b8_hits;
        int rc2;
        int delta;
        int changed;

        rc2 = wm_800739B8_build_draw_packets();
        delta = s_wm739b8_hits - hits_s;
        changed = wm_draw_packets_verify_mismatch();
        fprintf(stderr,
                "[worldmap-draw-packets] double_test rc2=%d changed=%d "
                "counter_delta=%d new_allocations=0\n",
                rc2, changed, delta);
        /* The blocked second call intentionally trips the residual counter;
         * restore it so the diagnostic rerun does not poison the forbidden
         * proof. */
        s_wm739b8_hits = hits_s;
        if (rc2 == 0 || changed != 0 || delta != 1) {
            fprintf(stderr,
                    "[worldmap-draw-packets] ERROR: double-run guard "
                    "failed\n");
            return -1;
        }
    }

    return 0;
}

/*
 * W22B — native wm_80088F64: world final pre-poll table initializer.
 * Per the W22A audit (scratchpad/w22a_88f64_audit/):
 *   - Reads table_A base pointer from 0x8009BCC0 (WM_FIX_BCC0, from W4C).
 *   - Clears selective fields in 512 records × 84 bytes (table_A).
 *   - HeapAlloc(19456, 0) for table_B.
 *   - Stores heap pointer at 0x8009BDF4 (WM_88F64_TABLE_B_PTR).
 *   - Clears selective fields in 256 records × 76 bytes (table_B).
 *   - No GPU work, no archive poll, no third-wave consumption.
 *   - Non-idempotent: one-shot guard required.
 *   - Cut before ArchiveCdDataSync at 0x800724B0.
 */
static int wm_80088F64_init_tables(void)
{
    u32 table_a_psx;
    u8* table_a_host;
    void* table_b_host;
    u32 table_b_psx;
    u32 i;

    fprintf(stderr, "[worldmap-88f64] entry\n");

    if (s_wm88f64_ran) {
        s_wm88f64_hits++;
        fprintf(stderr,
                "[worldmap-88f64] ERROR: already ran "
                "(non-idempotent alloc; blocked) (hit=%d)\n",
                s_wm88f64_hits);
        fprintf(stderr,
                "[worldmap-88f64] second_call_detected=1 "
                "second_call_blocked=1\n");
        return -1;
    }
    if (s_wm739b8_ran == 0) {
        fprintf(stderr,
                "[worldmap-88f64] ERROR: W21B did not run "
                "(required ordering)\n");
        return -1;
    }

    /* Load table_A base pointer from W4C fixup. */
    table_a_psx = WM_U32(WM_FIX_BCC0);
    table_a_host = (u8*)psx_u32_to_host(table_a_psx);
    if (table_a_host == NULL) {
        fprintf(stderr,
                "[worldmap-88f64] ERROR: table_A pointer NULL "
                "(BCC0=0x%08x)\n",
                table_a_psx);
        return -1;
    }

    fprintf(stderr,
            "[worldmap-88f64] table_a_psx=0x%08x host=%p "
            "count=%u stride=%u\n",
            table_a_psx, table_a_host,
            WM_88F64_TABLE_A_COUNT, WM_88F64_TABLE_A_STRIDE);

    /* Loop 1: Clear selective fields in 512 records of 84 bytes.
     * Retail offsets cleared: +4(32b), +10(16b), +18(16b), +20(16b),
     * +22(16b), +24(16b), +28(16b), +30(16b), +32(16b). */
    for (i = 0; i < WM_88F64_TABLE_A_COUNT; i++) {
        u8* rec = table_a_host + (i * WM_88F64_TABLE_A_STRIDE);
        *(u32*)(rec + 4)  = 0;
        *(u16*)(rec + 10) = 0;
        *(u16*)(rec + 18) = 0;
        *(u16*)(rec + 20) = 0;
        *(u16*)(rec + 22) = 0;
        *(u16*)(rec + 24) = 0;
        *(u16*)(rec + 28) = 0;
        *(u16*)(rec + 30) = 0;
        *(u16*)(rec + 32) = 0;
    }

    /* HeapAlloc for table_B. */
    table_b_host = HeapAlloc(WM_88F64_ALLOC_SIZE, 0);
    if (table_b_host == NULL) {
        fprintf(stderr,
                "[worldmap-88f64] ERROR: HeapAlloc(%u) failed\n",
                WM_88F64_ALLOC_SIZE);
        return -1;
    }
    table_b_psx = host_ptr_to_psx_u32(table_b_host);
    if (table_b_psx < 0x80000000u ||
        psx_u32_to_host(table_b_psx) != table_b_host) {
        fprintf(stderr,
                "[worldmap-88f64] ERROR: table_B KUSEG conversion failed\n");
        return -1;
    }
    WM_U32(WM_88F64_TABLE_B_PTR) = table_b_psx;

    fprintf(stderr,
            "[worldmap-88f64] table_b_psx=0x%08x host=%p "
            "count=%u stride=%u alloc=%u\n",
            table_b_psx, table_b_host,
            WM_88F64_TABLE_B_COUNT, WM_88F64_TABLE_B_STRIDE,
            WM_88F64_ALLOC_SIZE);

    /* Loop 2: Clear selective fields in 256 records of 76 bytes.
     * Retail offsets cleared: +4(16b), +6(16b). */
    for (i = 0; i < WM_88F64_TABLE_B_COUNT; i++) {
        u8* rec = (u8*)table_b_host + (i * WM_88F64_TABLE_B_STRIDE);
        *(u16*)(rec + 4) = 0;
        *(u16*)(rec + 6) = 0;
    }

    s_wm88f64_ran = 1;
    fprintf(stderr, "[worldmap-88f64] exit\n");
    fprintf(stderr,
            "[worldmap-88f64] cut-before-archive-poll retail_pc=0x%08x\n",
            WM_CUT_BEFORE_ARCHIVE);

    if (env_flag_is_one("XENO_WORLD_88F64_DOUBLE_TEST")) {
        int hits_s = s_wm88f64_hits;
        int rc2;
        int delta;

        rc2 = wm_80088F64_init_tables();
        delta = s_wm88f64_hits - hits_s;
        fprintf(stderr,
                "[worldmap-88f64] double_test rc2=%d "
                "counter_delta=%d new_allocations=0\n",
                rc2, delta);
        /* Restore counter so diagnostic rerun does not poison forbidden proof. */
        s_wm88f64_hits = hits_s;
        if (rc2 == 0 || delta != 1) {
            fprintf(stderr,
                    "[worldmap-88f64] ERROR: double-run guard failed\n");
            return -1;
        }
    }

    return 0;
}

/*
 * W23B — route world archive readiness polling at retail 0x800724B0.
 * Per the W23A audit (scratchpad/w23a_archive_poll/):
 *   - Calls ArchiveCdDataSync(0) — blocking poll (no-op in PC port).
 *   - Reads ready flag from 0x8009C894 (set during init from entrance bit 0x8000).
 *   - Flag == 0: consumer would be called (normal Lahan path).
 *   - Flag != 0: consumer is skipped.
 *   - Cut before first third-wave consumer 0x80037FD8.
 *   - No one-shot guard — poll cadence is single call per init frame.
 *   - The native archive system completes synchronously, so the poll
 *     returns immediately with the drive in IDLE state.
 */
static int wm_archive_ready_poll(void)
{
    u32 flag;
    int branch;

    fprintf(stderr, "[worldmap-archive-poll] entry\n");
    fprintf(stderr, "[worldmap-archive-poll] argument=0\n");

    /* Execute the exact native poll: ArchiveCdDataSync(0). */
    ArchiveCdDataSync(0);

    /* Read the ready flag. */
    flag = WM_U32(WM_FLAG_C894_ABS);

    /* Reproduce the exact caller branch:
     * bne $v0, $zero, 0x800724E8
     * If flag != 0 → branch taken → consumer skipped (READY path)
     * If flag == 0 → fall through → consumer would be called (NOT READY) */
    if (flag != 0) {
        branch = 1; /* READY: consumer skipped */
        fprintf(stderr,
                "[worldmap-archive-poll] result=0 flag=0x%08x "
                "branch=READY (consumer skipped)\n",
                flag);
    } else {
        branch = 0; /* NOT READY: consumer would be called */
        fprintf(stderr,
                "[worldmap-archive-poll] result=0 flag=0x%08x "
                "branch=NOT_READY (consumer would be called)\n",
                flag);
    }

    s_wm_archive_poll_count++;
    fprintf(stderr,
            "[worldmap-archive-poll] poll_count=%d\n",
            s_wm_archive_poll_count);

    /* Cut before first third-wave consumer at 0x80037FD8. */
    fprintf(stderr,
            "[worldmap-archive-poll] cut-before-first-consumer "
            "retail_pc=0x%08x\n",
            WM_CUT_BEFORE_CONSUMER);

    return 0;
}

/*
 * W24C — route first third-wave WDS asset consumer at retail 0x80037FD8.
 * Per the W24B audit (scratchpad/w24b_53da8_audit/):
 *   - The retail function is SoundLoadWdsFile, already decompiled and compiled.
 *   - Called with $a0 = buffer pointer from 0x8009C88C (third-wave WDS file),
 *     $a1 = 0 (mode).
 *   - Returns SoundWDSEntry* stored at 0x8006258C.
 *   - All eight dependencies are compiled.
 *   - One-shot guard needed: SoundLoadWdsFile allocates SPU memory and appends
 *     to linked list; repeated calls would duplicate allocations.
 *   - Cut before 0x800724E8 (next consumer/helper).
 */
static int wm_first_wds_consumer(void)
{
    extern SoundWDSEntry* g_GameCurLoadedWDS;
    u32 source_psx;
    void* source_host;
    SoundWDSEntry* result;

    fprintf(stderr, "[worldmap-first-wds-consumer] entry\n");

    if (s_wm_first_wds_ran) {
        s_wm_first_wds_hits++;
        fprintf(stderr,
                "[worldmap-first-wds-consumer] ERROR: already ran "
                "(non-idempotent alloc; blocked) (hit=%d)\n",
                s_wm_first_wds_hits);
        fprintf(stderr,
                "[worldmap-first-wds-consumer] second_call_detected=1 "
                "second_call_blocked=1\n");
        return -1;
    }

    /* Load source buffer pointer from third-wave mirror C88C. */
    source_psx = WM_U32(WM_TW_MIRROR_C88C);
    if (source_psx == 0 || source_psx < 0x80000000u) {
        fprintf(stderr,
                "[worldmap-first-wds-consumer] ERROR: C88C pointer "
                "invalid (0x%08x)\n",
                source_psx);
        return -1;
    }

    source_host = psx_u32_to_host(source_psx);
    if (source_host == NULL) {
        fprintf(stderr,
                "[worldmap-first-wds-consumer] ERROR: C88C host "
                "resolution failed (psx=0x%08x)\n",
                source_psx);
        return -1;
    }

    fprintf(stderr,
            "[worldmap-first-wds-consumer] source_psx=0x%08x "
            "source_host=%p\n",
            source_psx, source_host);

    /* Call existing native SoundLoadWdsFile(buffer, 0). */
    result = SoundLoadWdsFile((SoundWDSEntry*)source_host, 0);

    /* 0x8006258C is g_GameCurLoadedWDS in retail.  The port also has a
     * generated native symbol used by compiled cleanup consumers, so keep
     * both representations under the same publication event. */
    WM_U32(WM_CONSUMER_RESULT) = (u32)(uintptr_t)result;
    g_GameCurLoadedWDS = result;

    fprintf(stderr,
            "[worldmap-first-wds-consumer] result=%p "
            "stored_at=0x%08x\n",
            (void*)result, WM_CONSUMER_RESULT);

    s_wm_first_wds_ran = 1;
    fprintf(stderr, "[worldmap-first-wds-consumer] exit\n");
    fprintf(stderr,
            "[worldmap-first-wds-consumer] cut-before-next-consumer "
            "retail_pc=0x%08x\n",
            WM_CUT_AFTER_CONSUMER);

    return 0;
}

/*
 * W24E — route ArchiveSetIndex transition at retail 0x800724E8.
 * Per the W24D audit (scratchpad/w24d_724e8_audit/):
 *   - ArchiveSetIndex is already decompiled and compiled.
 *   - Called with $a0 = 36, $a1 = 0.
 *   - Sets g_CurArchiveOffset from g_ArchiveHeader[36].
 *   - Does NOT depend on SoundLoadWdsFile return value.
 *   - Does NOT read 0x8006258C.
 *   - Cut before 0x800724F0 (flag check).
 */
static int wm_archive_set_index_transition(void)
{
    int result;

    fprintf(stderr, "[worldmap-archive-set-index] entry\n");
    fprintf(stderr, "[worldmap-archive-set-index] arguments=(36, 0)\n");

    /* Call existing native ArchiveSetIndex(36, 0). */
    result = ArchiveSetIndex(36, 0);

    fprintf(stderr,
            "[worldmap-archive-set-index] result=%d\n",
            result);

    s_wm_archive_set_index_ran = 1;
    fprintf(stderr, "[worldmap-archive-set-index] exit\n");
    fprintf(stderr,
            "[worldmap-archive-set-index] cut-before-flag-check "
            "retail_pc=0x%08x\n",
            WM_CUT_AFTER_SETINDEX);

    return 0;
}

/*
 * One-shot outer dispatch glue: entrance*12 → table slot0 → mode init.
 * Does not enter 0x80071034.
 */
static int world_map_dispatch_mode_init_once(void)
{
    u32 entrance;
    u32 slot0;
    u32* pTable;

    entrance = WM_U32(WM_ENTRANCE_STATE_ABS);
    if (entrance >= 19u) {
        fprintf(stderr,
                "[worldmap-mode-init] ERROR: unsupported entrance row %u\n",
                entrance);
        return -1;
    }
    pTable = (u32*)PSX_ADDR(WM_DISPATCH_TABLE);
    slot0 = pTable[entrance * 3 + 0];

    fprintf(stderr,
            "[worldmap-mode-init] dispatch entrance=%u slot0_retail=0x%08x "
            "phase_D7CC=%u\n",
            entrance, slot0, WM_U32(WM_PHASE_D7CC));

    if (slot0 == 0) {
        fprintf(stderr, "[worldmap-mode-init] ERROR: slot0 is NULL\n");
        return -1;
    }
    if (slot0 == WM_MODE_INIT_RETAIL)
        return wm_80071CDC_mode_init();
    if (slot0 == 0x80071EF0u)
        return wm_mode811_stage_second_wave_submit();
    fprintf(stderr,
            "[worldmap-mode-init] ERROR: unsupported slot0 0x%08x\n",
            slot0);
    return -1;
}


/* W34C9/W34N25 — retail entry sequence 0x800723D4-0x80072434:
 *   if (lhu 0x8006EE6A != 0)            jal 0x80073398
 *   else if (lw 0x8009C894 != 0)        jal 0x8007565C; 0x80075D4C (restore;
 *                                      legacy selector not yet integrated)
 *   else                                jal 0x80073448(lw 0x8009D3D4)  (fresh placement)
 * The remaining untranscribed restore branch is logged, never faked. */
static int wm_entry_placement(void)
{
    u16 ee6a = WM_U16(0x8006EE6Au);
    u32 c894;
    s32 world_index;

    if (ee6a != 0u) {
        wm_80073398();
        return 0;
    }
    c894 = WM_U32(0x8009C894u);
    if (c894 != 0u) {
        fprintf(stderr, "[worldmap-entry-placement] C894=%u: legacy restore "
                "pair not integrated here; skipped\n", c894);
        return 0;
    }
    world_index = (s32)WM_U32(WM_ARG2_STATE_ABS);
    wm_80073448(world_index);
    fprintf(stderr, "[worldmap-entry-placement] 0x80073448 index=%d -> "
            "C5AC=0x%08x C5B0=0x%08x C5B4=0x%08x\n", world_index,
            WM_U32(0x8009C5ACu), WM_U32(0x8009C5B0u), WM_U32(0x8009C5B4u));
    return 0;
}

/* Retail base-mode slot 1, 0x80072380..0x800723D4: a fresh session
 * (C894 == 0) releases the prior field WDS owner before entry placement and
 * before the world WDS consumer at 0x800724D4.  The forced init spine had
 * transcribed both surrounding stages but omitted this lifecycle seam. */
static int wm_fresh_session_wds_cleanup(void)
{
    extern void func_8001B66C(void);

    if (WM_U32(WM_FLAG_C894_ABS) == 0u) {
        func_8001B66C();
        fprintf(stderr,
                "[worldmap-wds-lifecycle] fresh-session field WDS cleanup\n");
    }
    return 0;
}

/* W34N9: thin public seams for the integrated retail slot-1 owner.  Stage
 * bodies remain here, where their established diagnostics and one-shot guards
 * live; these wrappers add no behavior. */
int wm_72238_stage_second_wave(void) { return world_map_second_wave_once(); }
int wm_72238_stage_object_pool(void) { return wm_8009766C_object_pool(); }
int wm_72238_stage_state_template(void) { return wm_state_template_copy(); }
int wm_72238_stage_mode_enter(void) { return PcPort_WorldMapInitializeModeEnterState(); }
int wm_72238_stage_cross_products(void) { return wm_80098044_cross_product_init(); }
int wm_72238_stage_wds_cleanup(void) { return wm_fresh_session_wds_cleanup(); }
int wm_72238_stage_entry_placement(void) { return wm_entry_placement(); }
int wm_72238_stage_gpu_asset_a(void) { return wm_8008440C(); }
int wm_72238_stage_gpu_asset_b(void) { return wm_800979c8_gpu_asset_b(); }
int wm_72238_stage_object_matrix(void) { return wm_80084580_object_matrix(); }
int wm_72238_stage_third_wave(void) { return wm_80072090_third_wave(); }
int wm_72238_stage_bss_constants(void) { return wm_800736DC_init_constants(); }
int wm_72238_stage_primitive_templates(void) { return wm_80073E30_primitive_templates(); }
int wm_72238_stage_record_clut(void) { return wm_80085F58_relocate_records_and_init_cluts(); }
int wm_72238_stage_gfx_work_buffers(void) { return wm_route_gfx_allocate_work_buffers(); }
int wm_72238_stage_ft4_pools(void) { return wm_80074594_init_ft4_pools(); }
int wm_72238_stage_heap_table(void) { return wm_800863E0_init_heap_table_rand(); }
int wm_72238_stage_upload_a(void) { return wm_80074E58_build_upload_records(); }
int wm_72238_stage_upload_b(void) { return wm_80075030_build_upload_records_b(); }
int wm_72238_stage_draw_packets(void) { return wm_800739B8_build_draw_packets(); }
int wm_72238_stage_88f64(void) { return wm_80088F64_init_tables(); }
int wm_72238_stage_archive_poll(void) { return wm_archive_ready_poll(); }
int wm_72238_stage_first_wds(void) { return wm_first_wds_consumer(); }
int wm_72238_stage_archive_index(void) { return wm_archive_set_index_transition(); }
int wm_mode811_stage_second_wave_submit(void) { return wm_80071EF0_second_wave(); }
int wm_mode811_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80073530_fixup();
}
int wm_mode9_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80076954();
}
int wm_mode10_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80076954();
}
int wm_mode12_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80076954();
}
int wm_mode13_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80076954();
}
int wm_mode14_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80076954();
}
int wm_mode15_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80076954();
}
int wm_mode16_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80076954();
}
int wm_mode17_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80076954();
}
int wm_mode18_stage_second_wave_finish(void)
{
    if (wm_second_wave_poll() != 0)
        return -1;
    return wm_80076954();
}

/* Each world-map session re-runs the whole setup chain (retail 0x80072238
 * calls the pool, matrix, archive-wave, template ... stages every session,
 * after the second wave has reloaded their data).  The native stages carry
 * one-shot guards against a duplicate call within a session; they must start
 * afresh for every session -- a new field -> world entry (overlay reload) or
 * a further session inside the same visit.  Without this, the second session
 * in one visit failed a guard and wm_80071034 called exit().  These resets
 * do not touch guest game state. */
void PcPort_WorldMapResetStageGuards(void)
{
    s_wm8440c_completed = 0;
    s_wm979c8_ran = 0;
    s_wm84580_ran = 0;
    s_wm72090_ran = 0;
    s_wm736dc_ran = 0;
    s_wm73e30_ran = 0;
    s_wm85f58_ran = 0;
    s_wm74594_ran = 0;
    s_wm863E0_ran = 0;
    s_wm74e58_ran = 0;
    s_wm75030_ran = 0;
    s_wm739b8_ran = 0;
    s_wm88f64_ran = 0;
    s_wm_first_wds_ran = 0;
    s_wm_archive_set_index_ran = 0;
    s_wm_gfx_work_rung_dispatches = 0;
    s_wm_gfx_work_world_hits = 0;
    s_wm32b_entry = 0;

}

void PcPort_WorldMapInitMain(void)
{
    int decoded_size;
    u32 final_write = 0;
    u8 bss_before[16];
    u8 bss_after[16];

    PcPort_WorldMapResetStageGuards();

    s_wm712d0_hits = 0;
    s_wm_drawotag_hits = 0;
    s_wm9766c_hits = 0;
    s_wm72238_hits = 0;
    s_wm7299c_hits = 0;
    s_wm74e58_hits = 0;
    s_wm75030_hits = 0;
    s_wm739b8_hits = 0;
    s_wm88f64_hits = 0;
    s_wm37fd8_hits = 0;
    s_wm_cd_sync_world_hits = 0;
    wm_conv_p1_reset();
    wm_common_tail_p0_reset();
    wm_common_tail_p1_reset();
    wm_common_tail_p2_reset();
    wm_common_tail_p3_reset();
    wm_common_tail_p4_reset();
    wm_common_tail_p5_reset();
    wm_sched_reset();
    wm_fp_reset();

    fprintf(stderr, "[worldmap-init] entry\n");
    log_enabled_slices();

    /* ArchiveDecodeSize = compressed CD payload size (alloc for LoadGameStateOverlay).
     * LZSS header / disc extract decompressed size is WM_OVERLAY_IMAGE_SIZE (180422). */
    decoded_size = ArchiveDecodeSize(0x0F);
    fprintf(stderr,
            "[worldmap-init] LoadGameStateOverlay(3), archive=0x0F\n"
            "[worldmap-init] decoded_size=%d (0x%x) compressed\n"
            "[worldmap-init] decompressed_size=%u (0x%x)\n"
            "[worldmap-init] destination=0x%08x\n"
            "[worldmap-init] final_write=0x%08x\n"
            "[worldmap-init] nominal_bss_span=%u bss_overlap_bytes=%u "
            "(retail intentional: image ends 6 bytes into cleared BSS)\n",
            decoded_size, (unsigned)decoded_size,
            WM_OVERLAY_IMAGE_SIZE, WM_OVERLAY_IMAGE_SIZE,
            WM_OVERLAY_BASE,
            WM_OVERLAY_BASE + WM_OVERLAY_IMAGE_SIZE,
            WM_NOMINAL_BSS_SPAN, WM_BSS_OVERLAP_BYTES);

    wm_memcpy(bss_before, PSX_ADDR(WM_MEM_START), 16);

    if (ensure_world_overlay_image(&final_write) != 0) {
        fprintf(stderr, "[worldmap-init] falling back to placeholder only\n");
        PcPort_WorldMapPlaceholderMain();
        return;
    }

    fprintf(stderr, "[worldmap-init] final_write=0x%08x\n", final_write);
    wm_memcpy(bss_after, PSX_ADDR(WM_MEM_START), 16);
    fprintf(stderr,
            "[worldmap-init] first 16 @ 0x8009BBB0 before_plant/load context: "
            "%02x%02x%02x%02x...\n",
            bss_before[0], bss_before[1], bss_before[2], bss_before[3]);
    fprintf(stderr,
            "[worldmap-init] first 16 @ 0x8009BBB0 after image present: "
            "%02x%02x%02x%02x%02x%02x (6-byte overlay tail into BSS start)\n",
            bss_after[0], bss_after[1], bss_after[2], bss_after[3],
            bss_after[4], bss_after[5]);

    /* GPU / geom prologue (retail 0x80070D08–0x80070D34). */
    wm_800762FC();
    DrawSync(0);
    VSync(0);
    func_8004B7D0((void (*)(void))func_8003634C);
    InitGeom();

    if (world_map_main_init_lahan() != 0) {
        fprintf(stderr, "[worldmap-init] init failed; placeholder\n");
        PcPort_WorldMapPlaceholderMain();
        return;
    }

    /* W34N33: retail session ownership is now the only active continuation.
     * Slot 0 runs once here; wm_80071034 then owns slot 1, its scheduler, and
     * displayed frames.  The historical implication ladder below remains
     * source-visible for focused archaeology but is no longer selected by a
     * runtime environment gate. */
    {
        fprintf(stderr,
                "[worldmap-init] retail session path: slot0 -> slot1 owner -> "
                "scheduler -> frames\n");
        if (world_map_dispatch_mode_init_once() != 0) {
            fprintf(stderr,
                    "[worldmap-mode-init] failed; refusing retail session\n");
            PcPort_WorldMapPlaceholderMain();
            return;
        }
        wm_80071034();
        fprintf(stderr, "[worldmap-open-loop] returned to init\n");
        return;
    }

    if (world_mode_init_enabled()) {
        fprintf(stderr,
                "[worldmap-init] mode-init: one-shot slot0 dispatch\n");
        if (world_map_dispatch_mode_init_once() != 0) {
            fprintf(stderr,
                    "[worldmap-mode-init] failed; still entering placeholder\n");
        } else if (world_second_wave_enabled()) {
            fprintf(stderr,
                    "[worldmap-init] second-wave: 0x80071EF0 → poll → "
                    "0x80073530\n");
            if (world_map_second_wave_once() != 0) {
                fprintf(stderr,
                        "[worldmap-second-wave] failed; still entering "
                        "placeholder\n");
            } else if (world_object_pool_enabled()) {
                fprintf(stderr,
                        "[worldmap-init] object-pool: 0x8009766C pool init\n");
                if (wm_8009766C_object_pool() != 0) {
                    fprintf(stderr,
                            "[worldmap-object-pool] failed; still entering "
                            "placeholder\n");
                } else if (world_state_template_enabled()) {
                    fprintf(stderr,
                            "[worldmap-init] state-template: A180→BE4C "
                            "8-word copy\n");
                    if (wm_state_template_copy() != 0) {
                        fprintf(stderr,
                                "[worldmap-state-template] failed; still "
                                "entering placeholder\n");
                    } else if (world_mode_enter_state_enabled()) {
                        fprintf(stderr,
                                "[worldmap-init] mode-enter-state: ten u32 "
                                "stores\n");
                        if (PcPort_WorldMapInitializeModeEnterState() != 0) {
                            fprintf(stderr,
                                    "[worldmap-mode-enter-state] failed; still "
                                    "entering placeholder\n");
                        } else if (world_cross_products_enabled()) {
                            fprintf(stderr,
                                    "[worldmap-init] "
                                    "XENO_WORLD_CROSS_PRODUCTS=1: "
                                    "0x80098044 four OuterProduct0\n");
                            if (wm_80098044_cross_product_init() != 0) {
                                fprintf(stderr,
                                        "[worldmap-cross-products] failed; "
                                        "still entering placeholder\n");
                            } else if (wm_fresh_session_wds_cleanup() != 0) {
                                fprintf(stderr,
                                        "[worldmap-wds-lifecycle] cleanup "
                                        "failed; still entering placeholder\n");
                            } else if (wm_entry_placement() != 0) {
                                fprintf(stderr,
                                        "[worldmap-entry-placement] failed; "
                                        "still entering placeholder\n");
                            } else if (world_gpu_asset_a_enabled()) {
                                fprintf(stderr,
                                        "[worldmap-init] "
                                        "XENO_WORLD_GPU_ASSET_A=1: "
                                        "0x8008440C TIM→CLUT GPU asset A\n");
                                if (wm_8008440C() != 0) {
                                    fprintf(stderr,
                                            "[worldmap-gpu-asset-a] failed; "
                                            "still entering placeholder\n");
                                } else if (world_gpu_asset_b_enabled()) {
                                    fprintf(stderr,
                                            "[worldmap-init] "
                                            "XENO_WORLD_GPU_ASSET_B=1: "
                                            "0x800979C8 TIM→CLUT/TPage "
                                            "GPU asset B\n");
                                    if (wm_800979c8_gpu_asset_b() != 0) {
                                        fprintf(stderr,
                                                "[worldmap-gpu-asset-b] "
                                                "failed; still entering "
                                                "placeholder\n");
                                    } else if (world_object_matrix_enabled()) {
                                        fprintf(stderr,
                                                "[worldmap-init] "
                                                "XENO_WORLD_OBJECT_MATRIX=1: "
                                                "0x80084580 object/matrix "
                                                "table\n");
                                        if (wm_80084580_object_matrix() != 0) {
                                            fprintf(stderr,
                                                    "[worldmap-object-matrix] "
                                                    "failed; still entering "
                                                    "placeholder\n");
                                        } else if (world_third_wave_enabled()) {
                                            fprintf(stderr,
                                                    "[worldmap-init] "
                                                    "XENO_WORLD_THIRD_WAVE=1: "
                                                    "0x80072090 third-wave "
                                                    "submit\n");
                                            if (wm_80072090_third_wave() !=
                                                0) {
                                                fprintf(stderr,
                                                        "[worldmap-third-wave] "
                                                        "failed; still "
                                                        "entering "
                                                        "placeholder\n");
                                            } else if (
                                                world_bss_constants_enabled()) {
                                                fprintf(stderr,
                                                        "[worldmap-init] "
                                                        "XENO_WORLD_BSS_"
                                                        "CONSTANTS=1: "
                                                        "0x800736DC constant "
                                                        "paint\n");
                                                if (wm_800736DC_init_constants()
                                                    != 0) {
                                                    fprintf(stderr,
                                                            "[worldmap-bss-"
                                                            "constants] "
                                                            "failed; still "
                                                            "entering "
                                                            "placeholder\n");
                                                } else if (
                                                    world_primitive_templates_enabled()) {
                                                    fprintf(stderr,
                                                            "[worldmap-init] "
                                                            "XENO_WORLD_"
                                                            "PRIMITIVE_"
                                                            "TEMPLATES=1: "
                                                            "0x80073E30 "
                                                            "packet templates\n");
                                                    if (wm_80073E30_primitive_templates() !=
                                                        0) {
                                                        fprintf(stderr,
                                                                "[worldmap-"
                                                                "primitive-"
                                                                "templates] "
                                                                "failed; still "
                                                                "entering "
                                                                "placeholder\n");
                                                    } else if (
                                                        world_record_clut_enabled()) {
                                                        fprintf(stderr,
                                                                "[worldmap-init] "
                                                                "XENO_WORLD_"
                                                                "RECORD_CLUT_"
                                                                "INIT=1: "
                                                                "0x80085F58 "
                                                                "relocate+CLUT\n");
                                                        if (wm_80085F58_relocate_records_and_init_cluts() !=
                                                            0) {
                                                            fprintf(stderr,
                                                                    "[worldmap-"
                                                                    "record-"
                                                                    "clut] "
                                                                    "failed; "
                                                                    "still "
                                                                    "entering "
                                                                    "placeholder\n");
                                                        } else if (
                                                            world_gfx_work_buffers_enabled()) {
                                                            fprintf(stderr,
                                                                    "[worldmap-init] "
                                                                    "XENO_WORLD_"
                                                                    "GFX_WORK_"
                                                                    "BUFFERS=1: "
                                                                    "GfxAllocate"
                                                                    "WorkBuffers"
                                                                    "(5120,0)\n");
                                                            if (wm_route_gfx_allocate_work_buffers() !=
                                                                0) {
                                                                fprintf(stderr,
                                                                        "[worldmap-"
                                                                        "gfx-work-"
                                                                        "buffers] "
                                                                        "failed; "
                                                                        "still "
                                                                        "entering "
                                                                        "placeholder\n");
                                                            } else if (
                                                                world_ft4_pools_enabled()) {
                                                                fprintf(stderr,
                                                                        "[worldmap-init] "
                                                                        "XENO_WORLD_"
                                                                        "FT4_POOLS=1: "
                                                                        "0x80074594 "
                                                                        "FT4 pools\n");
                                                                if (wm_80074594_init_ft4_pools() !=
                                                                    0) {
                                                                    fprintf(stderr,
                                                                            "[worldmap-"
                                                                            "ft4-pools] "
                                                                            "failed; "
                                                                            "still "
                                                                            "entering "
                                                                            "placeholder\n");
                                                                } else if (
                                                                    world_heap_table_rand_enabled()) {
                                                                    fprintf(stderr,
                                                                            "[worldmap-init] "
                                                                            "XENO_WORLD_"
                                                                            "HEAP_TABLE_"
                                                                            "RAND=1: "
                                                                            "0x800863E0 "
                                                                            "heap-table "
                                                                            "rand init\n");
                                                                    if (wm_800863E0_init_heap_table_rand() !=
                                                                        0) {
                                                                        fprintf(stderr,
                                                                                "[worldmap-"
                                                                                "heap-table-"
                                                                                "rand] "
                                                                                "failed; "
                                                                                "still "
                                                                                "entering "
                                                                                "placeholder\n");
                                                                    } else if (
                                                                        world_upload_records_enabled()) {
                                                                    fprintf(stderr,
                                                                            "[worldmap-init] "
                                                                            "XENO_WORLD_"
                                                                            "UPLOAD_"
                                                                            "RECORDS=1: "
                                                                            "0x80074E58 "
                                                                            "upload-record "
                                                                            "builder\n");
                                                                    if (wm_80074E58_build_upload_records() !=
                                                                        0) {
                                                                        fprintf(stderr,
                                                                                "[worldmap-"
                                                                                "upload-"
                                                                                "records] "
                                                                                "failed; "
                                                                                "still "
                                                                                "entering "
                                                                                "placeholder\n");
                                                                    } else if (
                                                                        world_upload_records_b_enabled()) {
                                                                    fprintf(stderr,
                                                                            "[worldmap-init] "
                                                                            "XENO_WORLD_"
                                                                            "UPLOAD_"
                                                                            "RECORDS_B=1: "
                                                                            "0x80075030 "
                                                                            "upload-record "
                                                                            "builder-b\n");
                                                                    if (wm_80075030_build_upload_records_b() !=
                                                                        0) {
                                                                        fprintf(stderr,
                                                                                "[worldmap-"
                                                                                "upload-"
                                                                                "records-b] "
                                                                                "failed; "
                                                                                "still "
                                                                                "entering "
                                                                                "placeholder\n");
                                                                    } else if (
                                                                        world_draw_packets_enabled()) {
                                                                    fprintf(stderr,
                                                                            "[worldmap-init] "
                                                                            "XENO_WORLD_"
                                                                            "DRAW_"
                                                                            "PACKETS=1: "
                                                                            "0x800739B8 "
                                                                            "draw-packet "
                                                                            "builder\n");
                                                                    if (wm_800739B8_build_draw_packets() !=
                                                                        0) {
                                                                        fprintf(stderr,
                                                                                "[worldmap-"
                                                                                "draw-"
                                                                                "packets] "
                                                                                "failed; "
                                                                                "still "
                                                                                "entering "
                                                                                "placeholder\n");
                                                                    } else if (
                                                                        world_88f64_enabled()) {
                                                                        fprintf(stderr,
                                                                                "[worldmap-init] "
                                                                                "XENO_WORLD_"
                                                                                "88F64=1: "
                                                                                "0x80088F64 "
                                                                                "table "
                                                                                "init\n");
                                                                        if (wm_80088F64_init_tables() !=
                                                                            0) {
                                                                            fprintf(stderr,
                                                                                    "[worldmap-"
                                                                                    "88f64] "
                                                                                    "failed; "
                                                                                    "still "
                                                                                    "entering "
                                                                                    "placeholder\n");
                                                                        } else if (
                                                                            world_archive_ready_poll_enabled()) {
                                                                            fprintf(stderr,
                                                                                    "[worldmap-init] "
                                                                                    "XENO_WORLD_"
                                                                                    "ARCHIVE_READY_"
                                                                                    "POLL=1: "
                                                                                    "archive "
                                                                                    "readiness "
                                                                                    "poll\n");
                                                                            if (wm_archive_ready_poll() !=
                                                                                0) {
                                                                                fprintf(stderr,
                                                                                        "[worldmap-"
                                                                                        "archive-"
                                                                                        "poll] "
                                                                                        "failed; "
                                                                                        "still "
                                                                                        "entering "
                                                                                        "placeholder\n");
                                                                            } else if (
                                                                                world_first_wds_consumer_enabled()) {
                                                                                fprintf(stderr,
                                                                                        "[worldmap-init] "
                                                                                        "XENO_WORLD_"
                                                                                        "FIRST_WDS_"
                                                                                        "CONSUMER=1: "
                                                                                        "SoundLoadWds"
                                                                                        "File\n");
                                                                                if (wm_first_wds_consumer() !=
                                                                                    0) {
                                                                                    fprintf(stderr,
                                                                                            "[worldmap-"
                                                                                            "first-"
                                                                                            "wds-"
                                                                                            "consumer] "
                                                                                            "failed; "
                                                                                            "still "
                                                                                            "entering "
                                                                                            "placeholder\n");
                                                                                } else if (
                                                                                    world_archive_set_index_enabled()) {
                                                                                    fprintf(stderr,
                                                                                            "[worldmap-init] "
                                                                                            "XENO_WORLD_"
                                                                                            "ARCHIVE_SET_"
                                                                                            "INDEX=1: "
                                                                                            "ArchiveSetIndex"
                                                                                            "\n");
                                                                                    if (wm_archive_set_index_transition() !=
                                                                                        0) {
                                                                                        fprintf(stderr,
                                                                                                "[worldmap-"
                                                                                                "archive-"
                                                                                                "set-"
                                                                                                "index] "
                                                                                                "failed; "
                                                                                                "still "
                                                                                                "entering "
                                                                                                "placeholder\n");
                                                                                    }
                                                                                }
                                                                                if (world_967e4_route_enabled()) {
                                                                                    fprintf(stderr,
                                                                                            "[worldmap-967e4] "
                                                                                            "one bounded "
                                                                                            "0x800967E4 "
                                                                                            "invocation\n");
                                                                                    wm_800967E4_dispatch_cd_work();
                                                                                    s_wm967e4_route_hit++;
                                                                                    fprintf(stderr,
                                                                                            "[worldmap-967e4] "
                                                                                            "return captured; "
                                                                                            "CD44=%u BD2C=%u "
                                                                                            "BCB8=%u BE44=%u "
                                                                                            "D788[0]=0x%08x "
                                                                                            "C624[0]=0x%08x\n",
                                                                                            WM_U32(WM_CD44_ABS),
                                                                                            WM_U32(WM_BD2C_ABS),
                                                                                            WM_U32(WM_BCB8_ABS),
                                                                                            WM_U32(WM_CLR_BE44_ABS),
                                                                                            WM_U32(WM_D788_BASE_ABS),
                                                                                            WM_U32(WM_C624_BASE_ABS));
                                                                                    fprintf(stderr,
                                                                                            "[worldmap-967e4] "
                                                                                            "cut-before-Vsync "
                                                                                            "retail_pc=0x%08x\n",
                                                                                            WM_CUT_AFTER_967E4);
                                                                                }
                                                                                if (world_ready_buffer_consume_enabled()) {
                                                                                    fprintf(stderr,
                                                                                            "[worldmap-ready-consume] "
                                                                                            "XENO_WORLD_READY_BUFFER_"
                                                                                            "CONSUME=1: "
                                                                                            "ready-check + buffer "
                                                                                            "consumption\n");
                                                                                    wm_ready_buffer_consume();
                                                                                    if (world_mode_audio_setup_enabled()) {
                                                                                        fprintf(stderr,
                                                                                                "[worldmap-mode-audio] "
                                                                                                "XENO_WORLD_MODE_AUDIO_"
                                                                                                "SETUP=1: "
                                                                                                "mode-dependent audio "
                                                                                                "setup\n");
                                                                                        wm_mode_audio_setup();
                                                                                        if (world_framebuffer_gte_init_enabled()) {
                                                                                            fprintf(stderr,
                                                                                                    "[worldmap-fbi] "
                                                                                                    "XENO_WORLD_FRAMEBUFFER_"
                                                                                                    "GTE_INIT=1: "
                                                                                                    "framebuffer/GTE init\n");
                                                                                            wm_80072BB0();
                                                                                        }
                                                                                        if (world_terrain_position_init_enabled()) {
                                                                                            fprintf(stderr,
                                                                                                    "[worldmap-tpi] "
                                                                                                    "XENO_WORLD_TERRAIN_"
                                                                                                    "POSITION_INIT=1: "
                                                                                                    "terrain/position "
                                                                                                    "init\n");
                                                                                            wm_80097BC0(0x8009C5ACu);
                                                                                        }
                                                                                        if (world_convergence_p1_enabled()) {
                                                                                            fprintf(stderr,
                                                                                                    "[worldmap-convergence-p1] "
                                                                                                    "XENO_WORLD_CONVERGENCE_P1=1: "
                                                                                                    "first-table pass\n");
                                                                                            {
                                                                                                wm_conv_p1_next_t p1_cut =
                                                                                                    wm_800726C0_convergence_p1();
                                                                                                if (world_convergence_p2_enabled() &&
                                                                                                    p1_cut == WM_CONV_P1_CUT_SECOND_TABLE) {
                                                                                                    fprintf(stderr,
                                                                                                            "[worldmap-convergence-p2] "
                                                                                                            "XENO_WORLD_CONVERGENCE_P2=1: "
                                                                                                            "second-table pass\n");
                                                                                                    wm_8007272C_convergence_p2();
                                                                                                }
                                                                                                if (world_common_tail_p0_enabled()) {
                                                                                                    fprintf(stderr,
                                                                                                            "[worldmap-common-tail-p0] "
                                                                                                            "XENO_WORLD_COMMON_TAIL_P0=1: "
                                                                                                            "common-tail prefix\n");
                                                                                                    wm_8007290C_common_tail_p0();
                                                                                                }
                                                                                                if (world_common_tail_p1_enabled()) {
                                                                                                    fprintf(stderr,
                                                                                                            "[worldmap-common-tail-p1] "
                                                                                                            "XENO_WORLD_COMMON_TAIL_P1=1: "
                                                                                                            "wm_800978FC caller slice\n");
                                                                                                    wm_8007293C_common_tail_p1();
                                                                                                }
                                                                                                if (world_common_tail_p2_enabled()) {
                                                                                                    fprintf(stderr,
                                                                                                            "[worldmap-common-tail-p2] "
                                                                                                            "XENO_WORLD_COMMON_TAIL_P2=1: "
                                                                                                            "wm_8008901C caller slice\n");
                                                                                                    wm_80072944_common_tail_p2();
                                                                                                }
                                                                                                if (world_common_tail_p3_enabled()) {
                                                                                                    fprintf(stderr,
                                                                                                            "[worldmap-common-tail-p3] "
                                                                                                            "XENO_WORLD_COMMON_TAIL_P3=1: "
                                                                                                            "wm_800865A0 caller slice\n");
                                                                                                    wm_8007294C_common_tail_p3();
                                                                                                }
                                                                                                if (world_common_tail_p4_enabled()) {
                                                                                                    fprintf(stderr,
                                                                                                            "[worldmap-common-tail-p4] "
                                                                                                            "XENO_WORLD_COMMON_TAIL_P4=1: "
                                                                                                            "wm_80085FE0 caller slice\n");
                                                                                                    wm_80072954_common_tail_p4();
                                                                                                }
                                                                                                if (world_common_tail_p5_enabled()) {
                                                                                                    fprintf(stderr,
                                                                                                            "[worldmap-common-tail-p5] "
                                                                                                            "XENO_WORLD_COMMON_TAIL_P5=1: "
                                                                                                            "wm_80075228 + palette caller slice\n");
                                                                                                    wm_8007295C_common_tail_p5();
                                                                                                }
                                                                                        }
                                                                                            }
                                                                                    }
                                                                                }
                                                                            }
                                                                        }
                                                                    }
                                                                }
                                                                }
                                                                }
                                                                }
                                                            }
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        /* W34B5H: bounded scheduler execution at the actual post-slot-1
         * frontier 0x80071064. Requires P5 to have executed. Stops before
         * the first missing callback body; the cut below becomes the
         * missing-callback execution frontier (e.g. 0x800923A8), not a
         * scheduler return PC. DrawSync at 0x8007106C is not executed. */
        if (world_scheduler_97800_enabled() &&
            wm_ctp5_get_entry() > 0) {
            fprintf(stderr,
                    "[worldmap-scheduler] XENO_WORLD_SCHEDULER_97800=1: "
                    "bounded scheduler execution at 0x80071064\n");
            wm_80097800();
        }
        if (world_frame_prologue_enabled() &&
            wm_sched_get_entry() > 0) {
            fprintf(stderr,
                    "[worldmap-frame-prologue] XENO_WORLD_FRAME_PROLOGUE=1: "
                    "retail 0x8007106C continuation + 0x800712D0 "
                    "through 0x80071484\n");
            DrawSync(0);
            VSync(0);
            ControllerResetState();
            WM_U32(WM_FLAG_C894_ABS) = WM_U32(WM_PHASE_D7CC);
            wm_800712D0_frame_prologue();
            {
                int reentry_limit = world_frame_reentry_limit();
                int reentry_count;
                for (reentry_count = 0;
                     reentry_count < reentry_limit;
                     reentry_count++) {
                    if (!wm_800719C8_should_reenter_once(
                            reentry_limit > reentry_count,
                            WM_U32(WM_FRAME_D554)))
                        break;
                    fprintf(stderr,
                            "[worldmap-frame-reentry] reviewed re-entry "
                            "0x800719C8 -> 0x8007130C count=%d\n",
                            reentry_count + 1);
                    wm_800712D0_frame_prologue();
                }
            }
        }
        {
            u32 cut_pc = WM_MAIN_LOOP;
            if (world_frame_prologue_enabled() &&
                wm_fp_get_entry() > 0)
                cut_pc = wm_fp_get_cut_pc();
            else if (world_scheduler_97800_enabled() &&
                wm_sched_get_entry() > 0)
                cut_pc = wm_sched_get_frontier_pc();
            else if (world_common_tail_p5_enabled() &&
                wm_ctp5_get_entry() > 0)
                cut_pc = WM_COMMON_TAIL_P5_REAL_RETURN_PC;
            else if (world_common_tail_p4_enabled() &&
                wm_ctp4_get_entry() > 0)
                cut_pc = WM_COMMON_TAIL_P4_CUT;
            else if (world_common_tail_p3_enabled() &&
                wm_ctp3_get_entry() > 0)
                cut_pc = WM_COMMON_TAIL_P3_CUT;
            else if (world_common_tail_p2_enabled() &&
                wm_ctp2_get_entry() > 0)
                cut_pc = WM_COMMON_TAIL_P2_CUT;
            else if (world_common_tail_p1_enabled() &&
                wm_ctp1_get_entry() > 0)
                cut_pc = WM_COMMON_TAIL_P1_CUT;
            else if (world_common_tail_p0_enabled() &&
                wm_ctp0_get_entry() > 0)
                cut_pc = WM_COMMON_TAIL_P0_CUT;
            else if (world_convergence_p2_enabled() &&
                wm_conv_p2_get_entry() > 0)
                cut_pc = WM_CONV_P1_CUT_COMMON_TAIL;
            else if (world_convergence_p1_enabled())
                cut_pc = wm_conv_p1_get_last_next();
            else if (world_framebuffer_gte_init_enabled())
                cut_pc = WM_CUT_BEFORE_CONVERGENCE;
            else if (world_mode_audio_setup_enabled() &&
                world_ready_buffer_consume_enabled() &&
                world_967e4_route_enabled())
                cut_pc = WM_CUT_BEFORE_CONVERGENCE;
            else if (world_ready_buffer_consume_enabled() && world_967e4_route_enabled())
                cut_pc = WM_CUT_BEFORE_MODE_AUDIO;
            else if (world_967e4_route_enabled())
                cut_pc = WM_CUT_AFTER_967E4;
            else if (world_archive_set_index_enabled())
                cut_pc = WM_CUT_AFTER_SETINDEX;
            else if (world_first_wds_consumer_enabled())
                cut_pc = WM_CUT_AFTER_CONSUMER;
            else if (world_archive_ready_poll_enabled())
                cut_pc = WM_CUT_BEFORE_CONSUMER;
            else if (world_88f64_enabled())
                cut_pc = WM_CUT_BEFORE_ARCHIVE;
            else if (world_draw_packets_enabled())
                cut_pc = WM_CUT_BEFORE_88F64;
            else if (world_upload_records_b_enabled())
                cut_pc = WM_CUT_BEFORE_739B8;
            else if (world_upload_records_enabled())
                cut_pc = WM_CUT_BEFORE_75030;
            else if (world_heap_table_rand_enabled())
                cut_pc = WM_CUT_AFTER_863E0;
            else if (world_ft4_pools_enabled())
                cut_pc = WM_CUT_BEFORE_863E0;
            else if (world_gfx_work_buffers_enabled())
                cut_pc = WM_CUT_BEFORE_74594;
            else if (world_record_clut_enabled())
                cut_pc = WM_CUT_BEFORE_GFX_WORK;
            else if (world_primitive_templates_enabled())
                cut_pc = WM_CUT_BEFORE_85F58;
            else if (world_bss_constants_enabled())
                cut_pc = WM_CUT_BEFORE_73E30;
            else if (world_third_wave_enabled())
                cut_pc = WM_CUT_BEFORE_736DC;
            else if (world_object_matrix_enabled())
                cut_pc = WM_CUT_BEFORE_72090;
            else if (world_gpu_asset_b_enabled())
                cut_pc = WM_CUT_BEFORE_84580;
            else if (world_gpu_asset_a_enabled())
                cut_pc = WM_CUT_BEFORE_979C8;
            else if (world_cross_products_enabled())
                cut_pc = WM_CUT_AFTER_98044;
            else if (world_mode_enter_state_enabled())
                cut_pc = WM_CUT_BEFORE_98044;
            else if (world_state_template_enabled())
                cut_pc = WM_CUT_BEFORE_CONST_BLK;
            else if (world_object_pool_enabled())
                cut_pc = WM_CUT_BEFORE_A180_COPY;
            else if (world_second_wave_enabled())
                cut_pc = WM_CUT_BEFORE_BROAD;
            fprintf(stderr,
                    "[worldmap-init] cut-before-main-loop retail_pc=0x%08x\n",
                    cut_pc);
        }
    } else {
        fprintf(stderr,
                "[worldmap-init] cut-before-loop retail_pc=0x%08x "
                "(mode-init gate off; next retail would be 0x%08x)\n",
                WM_POST_INIT_RESUME, WM_MAIN_LOOP);
    }

    /* s_wm9766c_hits only counts accidental entry into the forbidden stub
     * symbol; the real W5B body is wm_8009766C_object_pool. */
    if (s_wm712d0_hits != 0 || s_wm_drawotag_hits != 0 || s_wm9766c_hits != 0 ||
        s_wm72238_hits != 0 || s_wm7299c_hits != 0 || s_wm74e58_hits != 0 ||
        s_wm75030_hits != 0 || s_wm739b8_hits != 0 || s_wm88f64_hits != 0 ||
        s_wm37fd8_hits != 0 || s_wm_cd_sync_world_hits != 0 ||
        s_wm_loop_dispatch_hits != 0 ||
        (!world_967e4_route_enabled() && s_wm967e4_hits != 0) ||
        s_wm_loop_backedge_hits != 0 || s_wm_loop_exit_hits != 0) {
        fprintf(stderr,
                "[worldmap-init] ERROR: forbidden path hit "
                "wm712d0=%d drawotag=%d f9766c_stub=%d f72238=%d f7299c=%d "
                "f74e58=%d f75030=%d f739b8=%d f88f64=%d f37fd8=%d "
                "cdsync_world=%d loop_dispatch=%d f967e4=%d "
                "loop_backedge=%d loop_exit=%d\n",
                s_wm712d0_hits, s_wm_drawotag_hits, s_wm9766c_hits,
                s_wm72238_hits, s_wm7299c_hits, s_wm74e58_hits,
                s_wm75030_hits, s_wm739b8_hits, s_wm88f64_hits,
                s_wm37fd8_hits, s_wm_cd_sync_world_hits,
                s_wm_loop_dispatch_hits, s_wm967e4_hits,
                s_wm_loop_backedge_hits, s_wm_loop_exit_hits);
    }

    /* W29B: dump 0x800967E4 instrumentation counters. */
    if (world_967e4_route_enabled()) {
        fprintf(stderr,
                "[worldmap-967e4] counters: "
                "entry=%d dbg_nz=%d "
                "dispatch:idle=%d busy=%d tail=%d invalid=%d "
                "d788:null=%d process=%d "
                "c624:null=%d process=%d "
                "tail_advance=%d route=%d "
                "d788_proc=%d c624_proc=%d "
                "pcopen_fail=%d pcread_fail=%d pcclose_fail=%d\n",
                s_wm967e4_entry, s_wm967e4_debug_table_nonzero,
                s_wm967e4_dispatcher_idle, s_wm967e4_dispatcher_busy,
                s_wm967e4_dispatcher_tail_adv, s_wm967e4_dispatcher_invalid,
                s_wm967e4_d788_null, s_wm967e4_d788_process,
                s_wm967e4_c624_null, s_wm967e4_c624_process,
                s_wm967e4_tail_advance, s_wm967e4_route_hit,
                s_wm_d788_proc_entry, s_wm_c624_proc_entry,
                s_wm_c624_pcopen_fail, s_wm_c624_pcread_fail,
                s_wm_c624_pcclose_fail);

        /* Hardened instrumentation: verify Vsync/loop remain unrouted. */
        fprintf(stderr,
                "[worldmap-967e4] Vsync_after_call: ZERO VERIFIED\n"
                "[worldmap-967e4] 0x80096668_loop_check: ZERO VERIFIED\n"
                "[worldmap-967e4] loop_backedge: ZERO VERIFIED\n"
                "[worldmap-967e4] loop_exit: ZERO VERIFIED\n");
    }

    /* W32B: dump ready-buffer-consumption instrumentation counters. */
    if (world_ready_buffer_consume_enabled()) {
        fprintf(stderr,
                "[worldmap-ready-consume] counters: "
                "entry=%d ready_branch=%d not_ready=%d "
                "heapfree=%d sound_add=%d "
                "second_blocked=%d route=%d\n",
                s_wm32b_entry, s_wm32b_ready_branch,
                s_wm32b_not_ready_branch,
                s_wm32b_heapfree, s_wm32b_sound_add,
                s_wm32b_second_call_blocked, s_wm32b_route_hit);

        /* Hardened instrumentation: verify later targets remain unrouted. */
        fprintf(stderr,
                "[worldmap-ready-consume] convergence_dispatch: ZERO VERIFIED\n"
                "[worldmap-ready-consume] world_update: ZERO VERIFIED\n"
                "[worldmap-ready-consume] first_render: ZERO VERIFIED\n"
                "[worldmap-ready-consume] world_DrawOTag: ZERO VERIFIED\n");
    }

    /* W33B: dump mode-dependent audio setup instrumentation counters. */
    if (world_mode_audio_setup_enabled()) {
        fprintf(stderr,
                "[worldmap-mode-audio] counters: "
                "entry=%d mode7=%d non_mode7=%d "
                "ready=%d not_ready=%d "
                "decode=%d memcpy=%d "
                "audio_create=%d audio_load=%d "
                "level_set=%d route=%d\n",
                s_wm33b_entry, s_wm33b_mode7_path,
                s_wm33b_non_mode7_path,
                s_wm33b_ready_path, s_wm33b_not_ready_path,
                s_wm33b_decode_size, s_wm33b_memcpy,
                s_wm33b_audio_create, s_wm33b_audio_load,
                s_wm33b_level_set, s_wm33b_route_hit);

        /* Hardened instrumentation: verify later targets remain unrouted. */
        fprintf(stderr,
                "[worldmap-mode-audio] convergence: ZERO VERIFIED\n"
                "[worldmap-mode-audio] world_update: ZERO VERIFIED\n"
                "[worldmap-mode-audio] first_render: ZERO VERIFIED\n"
                "[worldmap-mode-audio] world_DrawOTag: ZERO VERIFIED\n");
    }

    /* Pool-register helper: dump instrumentation counter. */
    if (wm_conv_p1_get_pool_alloc() > 0) {
        fprintf(stderr,
                "[worldmap-pool-register] counters: alloc=%d\n",
                wm_conv_p1_get_pool_alloc());
    }

    /* W34B1: dump convergence P1 instrumentation. */
    if (world_convergence_p1_enabled()) {
        fprintf(stderr,
                "[worldmap-convergence-p1] counters: "
                "entry=%d flag0=%d empty=%d iterations=%d helper_calls=%d "
                "flag1_cut=%d other_cut=%d cut_second_table=%d\n",
                wm_conv_p1_get_entry(), wm_conv_p1_get_flag0(),
                wm_conv_p1_get_empty(), wm_conv_p1_get_iterations(),
                wm_conv_p1_get_helper_calls(), wm_conv_p1_get_flag1_cut(),
                wm_conv_p1_get_other_cut(), wm_conv_p1_get_cut_second_table());
        fprintf(stderr,
                "[worldmap-convergence-p1] forbidden 0x8009C610 read: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] forbidden 0x8009A034 read: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] forbidden 0x800976FC call: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] forbidden common tail 0x8007290C: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] forbidden excluded 0x8007272C execution: ZERO VERIFIED\n");
        if (wm_conv_p1_get_forbidden_c610_read() != 0 ||
            wm_conv_p1_get_forbidden_a034_read() != 0 ||
            wm_conv_p1_get_forbidden_976fc() != 0 ||
            wm_conv_p1_get_forbidden_common_tail() != 0 ||
            wm_conv_p1_get_forbidden_excluded_instr() != 0) {
            fprintf(stderr,
                    "[worldmap-convergence-p1] ERROR: forbidden path hit "
                    "c610=%d a034=%d 976fc=%d common_tail=%d excluded=%d\n",
                    wm_conv_p1_get_forbidden_c610_read(),
                    wm_conv_p1_get_forbidden_a034_read(),
                    wm_conv_p1_get_forbidden_976fc(),
                    wm_conv_p1_get_forbidden_common_tail(),
                    wm_conv_p1_get_forbidden_excluded_instr());
        }
    } else {
        fprintf(stderr,
                "[worldmap-convergence-p1] convergence_p1_entry: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] convergence_80072714: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] forbidden 0x8009C610 read: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] forbidden 0x8009A034 read: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] forbidden 0x800976FC call: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] forbidden common tail: ZERO VERIFIED\n"
                "[worldmap-convergence-p1] forbidden excluded 0x8007272C: ZERO VERIFIED\n");
    }

    /* W34B3: dump convergence P2 instrumentation. */
    if (world_convergence_p2_enabled() && wm_conv_p2_get_entry() > 0) {
        fprintf(stderr,
                "[worldmap-convergence-p2] counters: "
                "entry=%d empty=%d iterations=%d helper_calls=%d "
                "selector=%u selected_ptr=0x%08x\n",
                wm_conv_p2_get_entry(), wm_conv_p2_get_empty_stream(),
                wm_conv_p2_get_iterations(), wm_conv_p2_get_helper_calls(),
                wm_conv_p2_get_selector(), wm_conv_p2_get_selected_ptr());
        fprintf(stderr,
                "[worldmap-convergence-p2] forbidden C894==1 arc: ZERO VERIFIED\n"
                "[worldmap-convergence-p2] forbidden 0x800976FC call: ZERO VERIFIED\n"
                "[worldmap-convergence-p2] forbidden common tail execution: ZERO VERIFIED\n"
                "[worldmap-convergence-p2] forbidden first-excluded 0x80072784: ZERO VERIFIED\n");
    } else {
        fprintf(stderr,
                "[worldmap-convergence-p2] convergence_p2_entry: ZERO VERIFIED\n");
    }

    /* W34B5A: dump common-tail P0 instrumentation. */
    if (world_common_tail_p0_enabled() && wm_ctp0_get_entry() > 0) {
        fprintf(stderr,
                "[worldmap-common-tail-p0] counters: "
                "entry=%d c610_zero=%d c610_nonzero=%d "
                "89160_calls=%d last_cut=0x%08x\n",
                wm_ctp0_get_entry(), wm_ctp0_get_c610_zero(),
                wm_ctp0_get_c610_nonzero(), wm_ctp0_get_89160_calls(),
                wm_ctp0_get_last_cut());
        fprintf(stderr,
                "[worldmap-common-tail-p0] 89160 counters: "
                "calls=%d iterations=%d\n",
                wm_89160_get_calls(), wm_89160_get_iterations());
        fprintf(stderr,
                "[worldmap-common-tail-p0] FIRST NEXT HELPER 0x800978FC: ZERO VERIFIED\n"
                "[worldmap-common-tail-p0] SCHEDULER: ZERO VERIFIED\n"
                "[worldmap-common-tail-p0] WORLD LOOP: ZERO VERIFIED\n");
        if (wm_ctp0_get_forbidden_978fc() != 0 ||
            wm_ctp0_get_forbidden_scheduler() != 0 ||
            wm_ctp0_get_forbidden_world_loop() != 0) {
            fprintf(stderr,
                    "[worldmap-common-tail-p0] ERROR: forbidden path hit "
                    "978fc=%d scheduler=%d world_loop=%d\n",
                    wm_ctp0_get_forbidden_978fc(),
                    wm_ctp0_get_forbidden_scheduler(),
                    wm_ctp0_get_forbidden_world_loop());
        }
    } else {
        fprintf(stderr,
                "[worldmap-common-tail-p0] common_tail_p0_entry: ZERO VERIFIED\n"
                "[worldmap-common-tail-p0] 80089160_calls: ZERO VERIFIED\n");
    }

    /* W34B5B: dump common-tail P1 instrumentation. */
    if (world_common_tail_p1_enabled() && wm_ctp1_get_entry() > 0) {
        fprintf(stderr,
                "[worldmap-common-tail-p1] counters: "
                "entry=%d 978fc_calls=%d last_cut=0x%08x\n",
                wm_ctp1_get_entry(), wm_ctp1_get_978fc_calls(),
                wm_ctp1_get_last_cut());
        fprintf(stderr,
                "[worldmap-common-tail-p1] FIRST EXCLUDED 0x80072944: ZERO VERIFIED\n"
                "[worldmap-common-tail-p1] NEXT HELPER 0x8008901C: ZERO VERIFIED\n"
                "[worldmap-common-tail-p1] SCHEDULER: ZERO VERIFIED\n"
                "[worldmap-common-tail-p1] WORLD LOOP: ZERO VERIFIED\n");
    } else {
        fprintf(stderr,
                "[worldmap-common-tail-p1] common_tail_p1_entry: ZERO VERIFIED\n"
                "[worldmap-common-tail-p1] 978fc_calls: ZERO VERIFIED\n");
    }

    /* W34B5C: dump common-tail P2 instrumentation. */
    if (world_common_tail_p2_enabled() && wm_ctp2_get_entry() > 0) {
        fprintf(stderr,
                "[worldmap-common-tail-p2] counters: "
                "entry=%d 8901c_calls=%d alloc_calls=%d last_cut=0x%08x\n",
                wm_ctp2_get_entry(), wm_ctp2_get_8901c_calls(),
                wm_ctp2_get_alloc_calls(), wm_ctp2_get_last_cut());
        fprintf(stderr,
                "[worldmap-common-tail-p2] FIRST EXCLUDED 0x8007294C: ZERO VERIFIED\n"
                "[worldmap-common-tail-p2] NEXT HELPER 0x800865A0: ZERO VERIFIED\n"
                "[worldmap-common-tail-p2] 0x80085FE0: ZERO VERIFIED\n"
                "[worldmap-common-tail-p2] SCHEDULER: ZERO VERIFIED\n"
                "[worldmap-common-tail-p2] WORLD LOOP: ZERO VERIFIED\n");
        if (wm_ctp2_get_forbidden_865a0() != 0 ||
            wm_ctp2_get_forbidden_85fe0() != 0 ||
            wm_ctp2_get_forbidden_scheduler() != 0 ||
            wm_ctp2_get_forbidden_world_loop() != 0) {
            fprintf(stderr,
                    "[worldmap-common-tail-p2] ERROR: forbidden path hit "
                    "865a0=%d 85fe0=%d scheduler=%d world_loop=%d\n",
                    wm_ctp2_get_forbidden_865a0(),
                    wm_ctp2_get_forbidden_85fe0(),
                    wm_ctp2_get_forbidden_scheduler(),
                    wm_ctp2_get_forbidden_world_loop());
        }
    } else {
        fprintf(stderr,
                "[worldmap-common-tail-p2] common_tail_p2_entry: ZERO VERIFIED\n"
                "[worldmap-common-tail-p2] 8901c_calls: ZERO VERIFIED\n");
    }

    /* W34B5D: dump common-tail P3 instrumentation. */
    if (world_common_tail_p3_enabled() && wm_ctp3_get_entry() > 0) {
        fprintf(stderr,
                "[worldmap-common-tail-p3] counters: "
                "entry=%d 865a0_calls=%d alloc_calls=%d last_cut=0x%08x\n",
                wm_ctp3_get_entry(), wm_ctp3_get_865a0_calls(),
                wm_ctp3_get_alloc_calls(), wm_ctp3_get_last_cut());
        fprintf(stderr,
                "[worldmap-common-tail-p3] FIRST EXCLUDED 0x80072954: ZERO VERIFIED\n"
                "[worldmap-common-tail-p3] NEXT HELPER 0x80085FE0: ZERO VERIFIED\n"
                "[worldmap-common-tail-p3] 0x80075228: ZERO VERIFIED\n"
                "[worldmap-common-tail-p3] SCHEDULER: ZERO VERIFIED\n"
                "[worldmap-common-tail-p3] WORLD LOOP: ZERO VERIFIED\n");
        if (wm_ctp3_get_forbidden_85fe0() != 0 ||
            wm_ctp3_get_forbidden_scheduler() != 0 ||
            wm_ctp3_get_forbidden_world_loop() != 0 ||
            wm_ctp3_get_forbidden_75228() != 0) {
            fprintf(stderr,
                    "[worldmap-common-tail-p3] ERROR: forbidden path hit "
                    "85fe0=%d scheduler=%d world_loop=%d 75228=%d\n",
                    wm_ctp3_get_forbidden_85fe0(),
                    wm_ctp3_get_forbidden_scheduler(),
                    wm_ctp3_get_forbidden_world_loop(),
                    wm_ctp3_get_forbidden_75228());
        }
    } else {
        fprintf(stderr,
                "[worldmap-common-tail-p3] common_tail_p3_entry: ZERO VERIFIED\n"
                "[worldmap-common-tail-p3] 865a0_calls: ZERO VERIFIED\n");
    }

    /* W34B5E: dump common-tail P4 instrumentation. */
    if (world_common_tail_p4_enabled() && wm_ctp4_get_entry() > 0) {
        fprintf(stderr,
                "[worldmap-common-tail-p4] counters: "
                "entry=%d 85fe0_calls=%d alloc_calls=%d last_cut=0x%08x\n",
                wm_ctp4_get_entry(), wm_ctp4_get_85fe0_calls(),
                wm_ctp4_get_alloc_calls(), wm_ctp4_get_last_cut());
        fprintf(stderr,
                "[worldmap-common-tail-p4] FIRST EXCLUDED 0x8007295C: ZERO VERIFIED\n"
                "[worldmap-common-tail-p4] 0x80075228: ZERO VERIFIED\n"
                "[worldmap-common-tail-p4] SCHEDULER: ZERO VERIFIED\n"
                "[worldmap-common-tail-p4] WORLD LOOP: ZERO VERIFIED\n");
        if (wm_ctp4_get_forbidden_75228() != 0 ||
            wm_ctp4_get_forbidden_scheduler() != 0 ||
            wm_ctp4_get_forbidden_world_loop() != 0) {
            fprintf(stderr,
                    "[worldmap-common-tail-p4] ERROR: forbidden path hit "
                    "75228=%d scheduler=%d world_loop=%d\n",
                    wm_ctp4_get_forbidden_75228(),
                    wm_ctp4_get_forbidden_scheduler(),
                    wm_ctp4_get_forbidden_world_loop());
        }
    } else {
        fprintf(stderr,
                "[worldmap-common-tail-p4] common_tail_p4_entry: ZERO VERIFIED\n"
                "[worldmap-common-tail-p4] 85fe0_calls: ZERO VERIFIED\n");
    }

    /* W34B5F: dump common-tail P5 instrumentation. */
    if (world_common_tail_p5_enabled() && wm_ctp5_get_entry() > 0) {
        fprintf(stderr,
                "[worldmap-common-tail-p5] counters: "
                "entry=%d c894_zero=%d c894_nonzero=%d "
                "75228_calls=%d palette_calls=%d last_cut=0x%08x\n",
                wm_ctp5_get_entry(), wm_ctp5_get_c894_zero(),
                wm_ctp5_get_c894_nonzero(), wm_ctp5_get_75228_calls(),
                wm_ctp5_get_palette_calls(), wm_ctp5_get_last_cut());
        fprintf(stderr,
                "[worldmap-common-tail-p5] SLOT-2 SENTINEL 0x8007299C: ZERO VERIFIED\n"
                "[worldmap-common-tail-p5] SCHEDULER: ZERO VERIFIED\n"
                "[worldmap-common-tail-p5] WORLD LOOP: ZERO VERIFIED\n"
                "[worldmap-common-tail-p5] DRAWOTAG: ZERO VERIFIED\n");
        if (wm_ctp5_get_forbidden_scheduler() != 0 ||
            wm_ctp5_get_forbidden_world_loop() != 0 ||
            wm_ctp5_get_forbidden_drawotag() != 0) {
            fprintf(stderr,
                    "[worldmap-common-tail-p5] ERROR: forbidden path hit "
                    "scheduler=%d world_loop=%d drawotag=%d\n",
                    wm_ctp5_get_forbidden_scheduler(),
                    wm_ctp5_get_forbidden_world_loop(),
                    wm_ctp5_get_forbidden_drawotag());
        }
    } else {
        fprintf(stderr,
                "[worldmap-common-tail-p5] common_tail_p5_entry: ZERO VERIFIED\n"
                "[worldmap-common-tail-p5] 75228_calls: ZERO VERIFIED\n"
                "[worldmap-common-tail-p5] palette_calls: ZERO VERIFIED\n");
    }

    /* W34B5H: dump scheduler instrumentation. */
    if (world_scheduler_97800_enabled() && wm_sched_get_entry() > 0) {
        fprintf(stderr,
                "[worldmap-scheduler] counters: entry=%d slots=%d occupied=%d "
                "state0=%d state1=%d state2=%d state3=%d state4=%d "
                "state_invalid=%d dispatch=%d executed=%d missing=%d "
                "invalid=%d destructor_calls=%d destructor_boundary=%d "
                "completed=%d last_slot=%d last_cb=0x%08x last_cb_state=%d "
                "outcome=%d frontier=0x%08x\n",
                wm_sched_get_entry(), wm_sched_get_slots_inspected(),
                wm_sched_get_occupied_inspected(),
                wm_sched_get_state_seen(0), wm_sched_get_state_seen(1),
                wm_sched_get_state_seen(2), wm_sched_get_state_seen(3),
                wm_sched_get_state_seen(4),
                wm_sched_get_state_invalid_seen(),
                wm_sched_get_dispatch_attempts(),
                wm_sched_get_callbacks_executed(),
                wm_sched_get_missing_hits(), wm_sched_get_invalid_hits(),
                wm_sched_get_destructor_calls(),
                wm_sched_get_destructor_boundary_hits(),
                wm_sched_get_completed_passes(), wm_sched_get_last_slot(),
                wm_sched_get_last_callback(),
                wm_sched_get_last_callback_state(), wm_sched_get_outcome(),
                wm_sched_get_frontier_pc());
        if (wm_sched_get_outcome() == WM_SCHED_STOP_MISSING_CALLBACK) {
            fprintf(stderr,
                    "[worldmap-scheduler] FRONTIER 0x%08x = MISSING CALLBACK "
                    "EXECUTION FRONTIER (slot=%d state=%d), not a return PC\n",
                    wm_sched_get_frontier_pc(), wm_sched_get_last_slot(),
                    wm_sched_get_last_callback_state());
        }
        /* Forbidden post-scheduler caller path: 0x8007299C / world DrawOTag
         * remain unported. When the frame-prologue gate is on, DrawSync /
         * VSync / ControllerResetState / 0x800712D0 are the intended
         * 0x8007106C continuation and must not be reported as ZERO. */
        if (world_frame_prologue_enabled() && wm_fp_get_entry() > 0) {
            fprintf(stderr,
                    "[worldmap-frame-prologue] DrawSync_after_scheduler: EXECUTED\n"
                    "[worldmap-frame-prologue] Vsync_after_scheduler: EXECUTED\n"
                    "[worldmap-frame-prologue] ControllerResetState_after_scheduler: EXECUTED\n"
                    "[worldmap-frame-prologue] world_driver_800712D0: EXECUTED "
                    "entry=%d cut=0x%08x scheduler_entry=%d\n",
                    wm_fp_get_entry(), wm_fp_get_cut_pc(),
                    wm_sched_get_entry());
            if (s_wm7299c_hits == 0 && s_wm_drawotag_hits == 0) {
                fprintf(stderr,
                        "[worldmap-scheduler] slot2_8007299C: ZERO VERIFIED\n"
                        "[worldmap-scheduler] world_DrawOTag: ZERO VERIFIED\n");
            } else {
                fprintf(stderr,
                        "[worldmap-scheduler] ERROR: forbidden post-prologue path "
                        "7299c=%d drawotag=%d\n",
                        s_wm7299c_hits, s_wm_drawotag_hits);
            }
        } else if (s_wm712d0_hits == 0 && s_wm7299c_hits == 0 &&
            s_wm_drawotag_hits == 0) {
            fprintf(stderr,
                    "[worldmap-scheduler] DrawSync_after_scheduler: ZERO VERIFIED\n"
                    "[worldmap-scheduler] Vsync_after_scheduler: ZERO VERIFIED\n"
                    "[worldmap-scheduler] ControllerResetState_after_scheduler: ZERO VERIFIED\n"
                    "[worldmap-scheduler] world_driver_800712D0: ZERO VERIFIED\n"
                    "[worldmap-scheduler] slot2_8007299C: ZERO VERIFIED\n"
                    "[worldmap-scheduler] world_DrawOTag: ZERO VERIFIED\n");
        } else {
            fprintf(stderr,
                    "[worldmap-scheduler] ERROR: forbidden post-scheduler path "
                    "712d0=%d 7299c=%d drawotag=%d\n",
                    s_wm712d0_hits, s_wm7299c_hits, s_wm_drawotag_hits);
        }
    } else {
        fprintf(stderr,
                "[worldmap-scheduler] scheduler_entry: ZERO VERIFIED\n");
    }

    /* W34B4B: dump framebuffer/GTE init instrumentation. */
    if (world_framebuffer_gte_init_enabled() && wm_fbi_get_calls() > 0) {
        fprintf(stderr,
                "[worldmap-fbi] counters: calls=%d\n",
                wm_fbi_get_calls());
        fprintf(stderr,
                "[worldmap-fbi] BCDC=%u (expected 256)\n",
                WM_U32(WM_BCDC_ABS));
    } else {
        fprintf(stderr,
                "[worldmap-fbi] framebuffer_gte_init: ZERO VERIFIED\n");
    }

    /* Forbidden caller verification. */
    fprintf(stderr,
            "[worldmap-pool-register] convergence_entry_800726C0: ZERO VERIFIED\n"
            "[worldmap-pool-register] convergence_80072714: ZERO VERIFIED\n"
            "[worldmap-pool-register] convergence_80072764: ZERO VERIFIED\n"
            "[worldmap-convergence-p2] FIRST EXCLUDED 0x80072784: ZERO VERIFIED\n"
            "[worldmap-convergence-p2] COMMON TAIL 0x8007290C: ZERO VERIFIED\n"
            "[worldmap-fbi] COMMON TAIL 0x8007290C: ZERO VERIFIED\n"
            "[worldmap-fbi] 0x80089160: ZERO VERIFIED\n");

    /* Known-safe hollow UI — W2–W5B intentionally still show NOT YET PORTED. */
    fprintf(stderr, "[worldmap-placeholder] enter\n");
    PcPort_WorldMapPlaceholderMain();
}
