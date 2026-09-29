/*
 * World-map helpers 0x800966CC and 0x8009699C — file I/O and CD operations.
 *
 * 0x800966CC [0x800966CC, 0x800967E4): 70 insns. Batch file loader.
 * Iterates over file table entries (stride 0x10), opens/reads/closes
 * via PsyQ PCopen/PClseek/PCread/PCclose with retry loops.
 *
 * 0x8009699C [0x8009699C, 0x80096ACC): 52 insns. CD sector loader.
 * Uses CdIntToPos/CdControlF/CdSyncCallback to read CD sectors.
 */
#ifndef WORLD_MAP_HELPER_966CC_H
#define WORLD_MAP_HELPER_966CC_H

#include "common.h"

void wm_800966CC(u32 file_table);
void wm_8009699C(u32 list);

#endif
