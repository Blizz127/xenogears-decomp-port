#ifndef WORLD_MAP_SESSION_SETUP_72238_H
#define WORLD_MAP_SESSION_SETUP_72238_H

#include "common.h"

/* Retail base-world mode slot 1, [0x80072238,0x8007299C).
 * Returns 0 on the transcribed fresh-session path, -1 on a stage failure,
 * and -2 when an as-yet untranscribed restore-entry branch is selected. */
int wm_80072238(void);

/* Thin integration seams implemented by world_map_init.c.  They preserve the
 * already-certified stage bodies while allowing the owner ordering to be
 * focused-tested without linking the whole initialization translation unit. */
int wm_72238_stage_second_wave(void);
int wm_72238_stage_object_pool(void);
int wm_72238_stage_state_template(void);
int wm_72238_stage_mode_enter(void);
int wm_72238_stage_cross_products(void);
int wm_72238_stage_wds_cleanup(void);
int wm_72238_stage_entry_placement(void);
int wm_72238_stage_gpu_asset_a(void);
int wm_72238_stage_gpu_asset_b(void);
int wm_72238_stage_object_matrix(void);
int wm_72238_stage_third_wave(void);
int wm_72238_stage_bss_constants(void);
int wm_72238_stage_primitive_templates(void);
int wm_72238_stage_record_clut(void);
int wm_72238_stage_gfx_work_buffers(void);
int wm_72238_stage_ft4_pools(void);
int wm_72238_stage_heap_table(void);
int wm_72238_stage_upload_a(void);
int wm_72238_stage_upload_b(void);
int wm_72238_stage_draw_packets(void);
int wm_72238_stage_88f64(void);
int wm_72238_stage_archive_poll(void);
int wm_72238_stage_first_wds(void);
int wm_72238_stage_archive_index(void);

#endif /* WORLD_MAP_SESSION_SETUP_72238_H */
