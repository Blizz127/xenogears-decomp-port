#ifndef XENO_FIELD_OBJECT_POOL_SCOPE_H
#define XENO_FIELD_OBJECT_POOL_SCOPE_H

#include <stdint.h>

/* Single game-thread owner for the Robo companion's 801E738C(64) lifetime.
 * Zero-initialize the handle and keep its address stable until restoration. */
typedef struct {
    unsigned active;
} PcFieldObjectPoolScope;

/* Install the two initialized pools and suspend the existing field registry.
 * Returns 0 on success or -1 with the prior field state retained. */
int PcPort_FieldObjectPoolBegin(PcFieldObjectPoolScope *scope);

/* The scoped caller must first release its models, effects and pool entries.
 * CanRestore is a non-mutating ownership check. Restore returns 0 on success
 * or -1 with the current owners and handle still live. */
int PcPort_FieldObjectPoolCanRestore(const PcFieldObjectPoolScope *scope);
int PcPort_FieldObjectPoolRestore(PcFieldObjectPoolScope *scope);

/* Read-only view used by the Gear owner after CFB44. Slot 1 and exactly
 * one packed registry row must agree with the owner's actor and compact
 * table. The caller separately verifies the actor, table, and model blocks. */
int PcPort_FieldObjectPoolOwnsGear(const PcFieldObjectPoolScope *scope, const void *actor,
                                  unsigned registryIndex, const uint32_t *groups, uint32_t count);

#endif
