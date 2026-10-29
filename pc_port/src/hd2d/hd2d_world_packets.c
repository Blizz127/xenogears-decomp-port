/* Exact positive provenance for the initial opaque field-scenery slice.
 * This module is CPU-only and never writes packets, OTs, or rendering state. */
#include "hd2d_world_packets.h"

#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#if defined(USE_EXTENDED_PRIM_POINTERS) && USE_EXTENDED_PRIM_POINTERS
#error "World provenance requires the current compact 32-bit PSX packet layout"
#endif

extern uint64_t PcPort_Hd2dSceneEpoch(void);
extern int PcPort_Hd2dOpeningActive(void);
extern int PcPort_Hd2dCurrentMap(void);

#define MAX_PACKET_BYTES 52u
#define INDEX_CAPACITY (PCPORT_HD2D_WORLD_PACKET_CAPACITY * 2u)

struct WorldPacket {
    const void *packet;
    unsigned bytes;
    unsigned seen;
    unsigned classified;
    unsigned char snapshot[MAX_PACKET_BYTES];
};

struct WorldStorage {
    struct WorldPacket pending[PCPORT_HD2D_WORLD_PACKET_CAPACITY];
    /* Entry index plus one; zero is empty. Rebuilt after retirement. */
    unsigned packet_index[INDEX_CAPACITY];
};

/* Optional host metadata must not consume the low static address space used
 * by compact 24-bit GPU links. No packet or OT is allocated in this block. */
static struct WorldStorage *storage;
static int storage_failed;
static unsigned pending_count;
static unsigned scope_depth;
static int scope_allowed;
static int poisoned;
static int overflowed;
static int observed;
static int active;
static int observed_map;
static uint64_t observed_epoch;
static uint64_t generation = UINT64_C(1);

static void advance_generation(void)
{
    ++generation;
    if (!generation) generation = UINT64_C(1);
}

static void clear_pending(void)
{
    pending_count = 0;
    scope_depth = 0;
    scope_allowed = 0;
    poisoned = 0;
    overflowed = 0;
    storage_failed = 0;
    if (storage)
        memset(storage->packet_index, 0, sizeof storage->packet_index);
    advance_generation();
}

static void poison(int overflow)
{
    if (!poisoned) advance_generation();
    poisoned = 1;
    if (overflow) overflowed = 1;
}

static int synchronize(void)
{
    const uint64_t epoch = PcPort_Hd2dSceneEpoch();
    const int map = PcPort_Hd2dCurrentMap();
    const int now_active = PcPort_Hd2dOpeningActive() &&
                           (map == 14 || map == 1 || map == 2 || map == 4);
    if (!observed || epoch != observed_epoch || map != observed_map ||
        now_active != active) {
        clear_pending();
        observed = 1;
        observed_epoch = epoch;
        observed_map = map;
        active = now_active;
    }
    if (active && !storage && !storage_failed) {
        storage = calloc(1, sizeof *storage);
        if (!storage) {
            /* A dropped registration cannot certify later packets. Retry
             * only at reset/epoch/scope change, never on every GPU hook. */
            storage_failed = 1;
            poison(1);
        }
    }
    return active && storage != NULL;
}

static unsigned index_slot(const void *packet)
{
    const uintptr_t address = (uintptr_t)packet;
    unsigned slot = (unsigned)((address >> 4) ^ (address >> 17)) &
                    (INDEX_CAPACITY - 1u);
    while (storage->packet_index[slot] &&
           storage->pending[storage->packet_index[slot] - 1u].packet != packet)
        slot = (slot + 1u) & (INDEX_CAPACITY - 1u);
    return slot;
}

static struct WorldPacket *find_packet(const void *packet)
{
    const unsigned index = storage->packet_index[index_slot(packet)];
    return index ? storage->pending + index - 1u : NULL;
}

static unsigned polygon_words(unsigned code)
{
    /* Bit0 selects raw texture colour; bit1 always means semitransparency.
     * Reject all semitransparent commands, even if a CLUT pixel is opaque. */
    if (code & 2u) return 0;
    switch (code & ~1u) {
    case 0x20: return 4;  /* F3 */
    case 0x24: return 7;  /* FT3 */
    case 0x28: return 5;  /* F4 */
    case 0x2c: return 9;  /* FT4 */
    case 0x30: return 6;  /* G3 */
    case 0x34: return 9;  /* GT3 */
    case 0x38: return 8;  /* G4 */
    case 0x3c: return 12; /* GT4 */
    default: return 0;
    }
}

void PcPort_Hd2dWorldReset(void)
{
    if (storage) free(storage);
    storage = NULL;
    clear_pending();
    observed = 0;
    active = 0;
}

uint64_t PcPort_Hd2dWorldGeneration(void)
{
    (void)synchronize();
    return generation;
}

int PcPort_Hd2dWorldValid(void)
{
    return synchronize() && !poisoned;
}

void PcPort_Hd2dWorldBegin(int actor_index, int scripted_count)
{
    const int available = synchronize();
    if (scope_depth) poison(0);
    if (scope_depth == UINT_MAX) {
        poison(1);
        scope_allowed = 0;
        return;
    }
    ++scope_depth;
    scope_allowed = available && scope_depth == 1u && scripted_count >= 0 &&
                    actor_index >= scripted_count;
}

void PcPort_Hd2dWorldEnd(void)
{
    (void)synchronize();
    if (scope_depth) --scope_depth;
    scope_allowed = 0;
}

void PcPort_Hd2dWorldPacketLinked(const void *packet)
{
    const unsigned char *bytes;
    unsigned words;
    unsigned slot;
    struct WorldPacket *entry;
    if (!synchronize() || !packet) return;
    /* Check reuse even when the new link is not in an allowed world scope.
     * A scripted draw must not inherit an earlier map packet's permission. */
    slot = index_slot(packet);
    if (storage->packet_index[slot]) {
        poison(0);
        return;
    }
    if (!scope_allowed || scope_depth != 1u) return;
    bytes = (const unsigned char *)packet;
    /* The real link hook supplies a valid tag. Reject impossible lengths
     * before reading the command or taking the bounded full snapshot. */
    if (bytes[3] < 4u || bytes[3] > 12u) return;
    words = polygon_words(bytes[7]);
    if (!words || bytes[3] != words) return;
    if (pending_count == PCPORT_HD2D_WORLD_PACKET_CAPACITY) {
        poison(1);
        return;
    }
    entry = storage->pending + pending_count;
    entry->packet = packet;
    entry->bytes = (words + 1u) * 4u;
    entry->seen = 0;
    entry->classified = 0;
    memcpy(entry->snapshot, packet, entry->bytes);
    storage->packet_index[slot] = ++pending_count;
}

void PcPort_Hd2dWorldPacketSeen(const void *packet)
{
    struct WorldPacket *entry;
    if (!synchronize() || !packet) return;
    entry = find_packet(packet);
    if (!entry) return;
    if (entry->seen) poison(0);
    entry->seen = 1;
}

unsigned PcPort_Hd2dWorldPacketClassify(const void *packet,
                                     unsigned command_code, int overridden)
{
    struct WorldPacket *entry;
    if (!synchronize() || !packet) return 0;
    entry = find_packet(packet);
    if (!entry) return 0; /* Unknown pointers must not be dereferenced. */
    if (!entry->seen || entry->classified) {
        poison(0);
        return 0;
    }
    entry->classified = 1;
    if (memcmp(packet, entry->snapshot, entry->bytes) ||
        command_code != entry->snapshot[7]) {
        poison(0);
        return 0;
    }
    if (poisoned || overridden || !polygon_words(command_code)) return 0;
    return 2;
}

void PcPort_Hd2dWorldGpuWalkDone(void)
{
    unsigned read_index;
    unsigned write_index = 0;
    if (!synchronize()) return;
    for (read_index = 0; read_index < pending_count; ++read_index) {
        struct WorldPacket *entry = storage->pending + read_index;
        if (entry->seen) {
            /* Catch a late mutation after classification but before retirement
             * so previously emitted positive tags cannot certify the offer. */
            if (memcmp(entry->packet, entry->snapshot, entry->bytes)) poison(0);
        } else {
            if (read_index != write_index) storage->pending[write_index] = *entry;
            ++write_index;
        }
    }
    if (write_index != pending_count) {
        pending_count = write_index;
        memset(storage->packet_index, 0, sizeof storage->packet_index);
        for (read_index = 0; read_index < pending_count; ++read_index)
            storage->packet_index[index_slot(storage->pending[read_index].packet)] =
                read_index + 1u;
    }
    if (poisoned && !overflowed && !pending_count && !scope_depth) {
        poisoned = 0;
        advance_generation();
    }
}
