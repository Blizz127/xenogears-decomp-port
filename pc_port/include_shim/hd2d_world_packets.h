#ifndef PC_PORT_HD2D_WORLD_PACKETS_H
#define PC_PORT_HD2D_WORLD_PACKETS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define PCPORT_HD2D_WORLD_PACKET_CAPACITY 16384u

/* Positive provenance only: the caller brackets the real field-model draw.
 * actor_index >= scripted_count is the non-scripted field-model subset.
 * The helper also requires OpeningActive and field 14, 1, 2 or 4. Nested scopes
 * are ambiguous and invalidate the pending generation. No retail writes. */
void PcPort_Hd2dWorldBegin(int actor_index, int scripted_count);
void PcPort_Hd2dWorldEnd(void);

/* Call only AFTER successful domain-aware linkage of the real packet.
 * Uses the full pointer and a post-link snapshot, including the compact
 * 32-bit PSX tag. Only exact-length opaque polygon packets can register.
 * Pending pointer reuse/duplicate linkage poisons the generation, including
 * a subsequent link outside world scope. Never masks a pointer to 24 bits. */
void PcPort_Hd2dWorldPacketLinked(const void *packet);

/* GPU parser calls Seen once before parsing each actual packet, including
 * overridden/skipped packets. Seen never dereferences its input pointer.
 * Classify is called once after parsing that same packet, with the actual
 * unmasked command byte and whether any texture override was selected.
 * Returns exactly 2 for proven opaque scenery, otherwise protected/unknown0.
 * The renderer should rasterize 0 results as protected4; untouched coverage
 * remains uncertified0. Registered packets must remain readable until the
 * walk ends. Unknown pointers are never dereferenced. */
void PcPort_Hd2dWorldPacketSeen(const void *packet);
unsigned PcPort_Hd2dWorldPacketClassify(const void *packet,
                                     unsigned command_code, int overridden);

/* End every completed OT/single-packet walk. Retires only packets actually
 * seen, preserving registrations for independently queued OTs. No present
 * parity and no SceneBegin reset: the next OT can precede the prior present. */
void PcPort_Hd2dWorldGpuWalkDone(void);

/* CPU-only invalidation, including context teardown. Epoch/map/scope changes
 * are also observed through the bridge and invalidate pending registrations.
 * Duplicate/reuse/mutation poison may recover when all tracked packets have
 * drained; overflow stays poisoned until reset/epoch, since dropped packets
 * cannot be tracked. Every invalidation/recovery changes this generation.
 * Host metadata is allocated only in active scope; allocation failure stays
 * unavailable until reset/epoch/scope change. Reset releases that storage.
 * A renderer offer MUST require WorldValid and the same WorldGeneration as
 * scene start; a late ambiguity invalidates already emitted positive tags. */
void PcPort_Hd2dWorldReset(void);
uint64_t PcPort_Hd2dWorldGeneration(void);
int PcPort_Hd2dWorldValid(void);

#ifdef __cplusplus
}
#endif
#endif
