/* world_map_retail_trig.h -- the retail trig entry points, by retail name.
 *
 * The decomp symbol map names SLUS 0x8003F8B0 `rcos` and 0x8003F8CC `rsin`,
 * but their bodies are the other way round:
 *
 *   0x8003F8B0  andi a0,0xFFF; lh v0, D_800523F0(a0*4)  -> the SINE halfword
 *   0x8003F8CC  andi a0,0xFFF; lh v0, D_800523F2(a0*4)  -> the COSINE halfword
 *
 * (rcossin_tbl at 0x800523F0 holds {sin, cos} pairs: entry 0x400 is
 * {4096, 0}.)  The world-map transcriptions call these entries by their decomp
 * names, as the retail `jal rcos` / `jal rsin` do, so they must get the retail
 * bodies, not PsyCross's conventional rcos/rsin.  Calling the conventional
 * ones mirrors every heading about the 45-degree diagonal: at the Lahan
 * camera heading 0xE00 the d-pad Up/Down walk went backwards.
 *
 * pc_port/include_shim/psyq/libgte.h makes the same mapping for the game
 * translation units that include it; this header serves the world-map
 * transcriptions, which address guest memory and do not include the Psy-Q
 * headers. */
#ifndef WORLD_MAP_RETAIL_TRIG_H
#define WORLD_MAP_RETAIL_TRIG_H

/* PsyCross (conventional): rsin(a) is sine, rcos(a) is cosine. */
extern int rsin(int a);
extern int rcos(int a);

/* Retail 0x8003F8B0 (decomp `rcos`): sine of (a & 0xFFF). */
static inline int wm_retail_rcos(int a)
{
    return rsin(a & 0xFFF);
}

/* Retail 0x8003F8CC (decomp `rsin`): cosine of (a & 0xFFF). */
static inline int wm_retail_rsin(int a)
{
    return rcos(a & 0xFFF);
}

#endif
