/* walkmesh_dump.c -- TEST TOOLING: dump the loaded field's walkmesh.
 *
 * Enabled only by XENO_WALKMESH_DUMP=<dir>; inert otherwise.  Once per loaded
 * field it writes, for every walkmesh layer L of map N:
 *
 *     <dir>/mapN-trisL.bin       14-byte records: 3 vertex ids, 3 neighbour
 *                                triangle ids, 1 material id (all u16)
 *     <dir>/mapN-vertsL.bin      8-byte records: x, y, z, pad (all s16)
 *     <dir>/mapN-materials.bin   u32 material flag rows
 *
 * which is exactly the layout scratchpad/blackmoon-route-20260919/cam_walk.py
 * already plans over.  Those map23 files were produced by hand with gdb, and
 * that is the reason the route walker only ever worked on map 23: every new
 * map needed a fresh manual extraction before the planner could say anything
 * about it.  Dumping them from the loader's own tables makes the walker work
 * on any field the game can load.
 *
 * The tables are the loader's (src/field/main/misc3.c:895-950): D_800AFB54
 * layers, per-layer triangle counts at D_800AFB20[9..12], triangle bases at
 * D_800AFB20[1..4], vertex bases at D_800AFB20[5..8], and the material rows at
 * D_800AFB20[0].  Vertex and material counts are not stored, so they are taken
 * as one past the largest id any triangle actually references -- which is all
 * a planner needs and cannot read past the blocks the triangles index into.
 *
 * Read-only with respect to game state.  REMOVAL: delete this file, its
 * build_port.sh entry, and the PcPort_WalkmeshDump() call in
 * pc_port/src/psyq_compat.c's Vsync shim.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "field/main.h"

#include "quick_checkpoint.h"

extern u32 D_800AFB20[];
extern s16 D_800AFB54;
extern int g_GameSceneMapNum;
extern u8 D_800ADB04;

#define WM_MAX_LAYERS 4
#define WM_TRI_STRIDE 14
#define WM_VERT_STRIDE 8

static void wm_write(const char* dir, const char* name, const void* data,
                     size_t bytes)
{
    char path[512];
    FILE* f;

    snprintf(path, sizeof(path), "%s/%s", dir, name);
    f = fopen(path, "wb");
    if (f == NULL) {
        fprintf(stderr, "[xeno-port][walkmesh] cannot write %s\n", path);
        return;
    }
    fwrite(data, 1, bytes, f);
    fclose(f);
    fprintf(stderr, "[xeno-port][walkmesh] wrote %s (%zu bytes)\n", path, bytes);
}

void PcPort_WalkmeshDump(void)
{
    static const char* dir;
    static int inited;
    static int lastMap = -1;
    int map;
    int layers;
    int layer;

    if (!inited) {
        inited = 1;
        dir = getenv("XENO_WALKMESH_DUMP");
        if (dir != NULL && dir[0] == '\0') dir = NULL;
    }
    if (dir == NULL) return;
    if (!PcPort_QuickCheckpointFieldIsActive()) return;
    if (D_800ADB04 == 0) return;

    map = g_GameSceneMapNum & 0xFFF;
    if (map == lastMap) return;

    layers = (int)D_800AFB54;
    if (layers <= 0 || layers > WM_MAX_LAYERS) return;
    if (D_800AFB20[0] == 0) return;

    lastMap = map;

    {
        u32 maxMat = 0;
        int sawAny = 0;

        for (layer = 0; layer < layers; layer++) {
            const u8* triBase = (const u8*)(uintptr_t)D_800AFB20[1 + layer];
            const u8* vertBase = (const u8*)(uintptr_t)D_800AFB20[5 + layer];
            u32 triCount = D_800AFB20[9 + layer];
            u32 maxVert = 0;
            u32 i;
            char name[64];

            if (triBase == NULL || vertBase == NULL || triCount == 0 ||
                triCount > 0x4000) {
                continue;
            }
            for (i = 0; i < triCount; i++) {
                const u16* t = (const u16*)(triBase + (size_t)i * WM_TRI_STRIDE);
                int k;
                for (k = 0; k < 3; k++) {
                    if (t[k] > maxVert) maxVert = t[k];
                }
                if (t[6] > maxMat) maxMat = t[6];
            }
            snprintf(name, sizeof(name), "map%d-tris%d.bin", map, layer);
            wm_write(dir, name, triBase, (size_t)triCount * WM_TRI_STRIDE);
            snprintf(name, sizeof(name), "map%d-verts%d.bin", map, layer);
            wm_write(dir, name, vertBase,
                     ((size_t)maxVert + 1) * WM_VERT_STRIDE);
            sawAny = 1;
        }
        if (sawAny) {
            char name[64];
            snprintf(name, sizeof(name), "map%d-materials.bin", map);
            wm_write(dir, name, (const void*)(uintptr_t)D_800AFB20[0],
                     ((size_t)maxMat + 1) * sizeof(u32));
        }
    }
}
