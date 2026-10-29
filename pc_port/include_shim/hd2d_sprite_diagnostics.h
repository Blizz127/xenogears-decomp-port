#ifndef PC_PORT_HD2D_SPRITE_DIAGNOSTICS_H
#define PC_PORT_HD2D_SPRITE_DIAGNOSTICS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint64_t registered, queries, accepted, no_scope, no_texture, no_identity;
    uint64_t snapshot_miss, metadata_miss, uv_miss, premature_presents;
    uint64_t gpu_walks, expired_unconsumed;
    unsigned pending;
} PcPortHd2dSpriteDiagnostics;
/* XENO_HD2D_SPRITE_DIAG=1 and master ON; OFF never touches out. */
int PcPort_Hd2dSpriteDiagnosticsGet(PcPortHd2dSpriteDiagnostics *out);
/* CPU-only explicit invalidation for enable transitions. */
void PcPort_Hd2dSpriteReset(void);
/* Renderer observes exact address before the DR override early return. */
void PcPort_Hd2dSpritePacketSeen(const void *packet);
/* Native OT parser calls AFTER completing an actual linked-list walk. */
void PcPort_Hd2dSpriteGpuWalkDone(void);
#ifdef __cplusplus
}
#endif
#endif
