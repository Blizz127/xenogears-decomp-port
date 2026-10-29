#include "quick_checkpoint_file.h"

#include <stdio.h>
#include <string.h>

#define CHECKPOINT_VERSION 1u

static const uint8_t kCheckpointMagic[8] = {
    'X', 'G', 'Q', 'C', 'K', 'P', 'T', 0
};

static void put_u16(uint8_t* dst, uint16_t value)
{
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t* dst, uint32_t value)
{
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
    dst[2] = (uint8_t)(value >> 16);
    dst[3] = (uint8_t)(value >> 24);
}

static uint16_t get_u16(const uint8_t* src)
{
    return (uint16_t)((uint16_t)src[0] | ((uint16_t)src[1] << 8));
}

static uint32_t get_u32(const uint8_t* src)
{
    return (uint32_t)src[0] |
           ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

static uint32_t checksum_bytes(uint32_t hash, const uint8_t* bytes,
                               size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

/* Rand seed sidecar "<path>.rng": "RNGS", seed, the main file's checksum,
 * check value. A separate file keeps the main checkpoint format unchanged, so
 * older builds still load new saves; the checksum ties the sidecar to the
 * exact checkpoint it was written with (a stale one is ignored). */
#define SEED_SIDECAR_BYTES 16u
static const uint8_t kSeedTag[4] = { 'R', 'N', 'G', 'S' };

static uint32_t seed_check(const uint8_t* sidecar)
{
    return checksum_bytes(2166136261u, sidecar, 12);
}

static int sidecar_path(char* out, size_t n, const char* path)
{
    return snprintf(out, n, "%s.rng", path) < (int)n ? 0 : -1;
}

static uint32_t checkpoint_checksum(const uint8_t* header,
                                    const uint8_t* game_state)
{
    uint32_t hash = 2166136261u;
    hash = checksum_bytes(hash, header + 24,
                          PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES - 24);
    return checksum_bytes(hash, game_state,
                          PC_PORT_QUICK_CHECKPOINT_STATE_BYTES);
}

int PcPort_QuickCheckpointWriteFile(const char* path,
                                    const PcPortQuickCheckpoint* checkpoint)
{
    uint8_t header[PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES] = {0};
    uint8_t sidecar[SEED_SIDECAR_BYTES];
    char temporary[1024];
    char side[1024];
    FILE* file;
    uint32_t checksum;
    int result = -1;

    if (!path || !path[0] || !checkpoint)
        return -1;
    if (snprintf(temporary, sizeof(temporary), "%s.tmp", path) >=
        (int)sizeof(temporary))
        return -1;

    memcpy(header, kCheckpointMagic, sizeof(kCheckpointMagic));
    put_u32(header + 8, CHECKPOINT_VERSION);
    put_u32(header + 12, PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES);
    put_u32(header + 16, PC_PORT_QUICK_CHECKPOINT_STATE_BYTES);
    put_u16(header + 24, checkpoint->map);
    put_u16(header + 26, checkpoint->entrance);
    put_u32(header + 28, (uint32_t)checkpoint->position[0]);
    put_u32(header + 32, (uint32_t)checkpoint->position[1]);
    put_u32(header + 36, (uint32_t)checkpoint->position[2]);
    put_u16(header + 40, (uint16_t)checkpoint->rotation[0]);
    put_u16(header + 42, (uint16_t)checkpoint->rotation[1]);
    put_u16(header + 44, (uint16_t)checkpoint->rotation[2]);
    checksum = checkpoint_checksum(header, checkpoint->game_state);
    put_u32(header + 20, checksum);
    memcpy(sidecar, kSeedTag, sizeof(kSeedTag));
    put_u32(sidecar + 4, checkpoint->random_seed);
    put_u32(sidecar + 8, checksum);
    put_u32(sidecar + 12, seed_check(sidecar));
    if (sidecar_path(side, sizeof side, path) != 0)
        return -1;

    file = fopen(temporary, "wb");
    if (!file)
        return -1;
    if (fwrite(header, 1, sizeof(header), file) == sizeof(header) &&
        fwrite(checkpoint->game_state, 1, sizeof(checkpoint->game_state), file) ==
            sizeof(checkpoint->game_state) &&
        fflush(file) == 0)
        result = 0;
    if (fclose(file) != 0)
        result = -1;
    file = NULL;
    if (result == 0 && rename(temporary, path) != 0)
        result = -1;
    if (result != 0)
        remove(temporary);
    /* The sidecar follows the checkpoint; a failure leaves a checkpoint
     * without a seed (or with a stale sidecar the checksum rejects). */
    if (result == 0 && checkpoint->has_random_seed &&
        (file = fopen(side, "wb")) != NULL) {
        fwrite(sidecar, 1, sizeof(sidecar), file);
        fclose(file);
    }
    return result;
}

int PcPort_QuickCheckpointReadFile(const char* path,
                                   PcPortQuickCheckpoint* checkpoint)
{
    uint8_t header[PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES];
    uint8_t sidecar[SEED_SIDECAR_BYTES + 1];
    char side[1024];
    PcPortQuickCheckpoint loaded;
    FILE* file;
    int trailing;
    uint32_t expected;

    if (!path || !checkpoint)
        return -1;
    memset(&loaded, 0, sizeof(loaded));
    file = fopen(path, "rb");
    if (!file)
        return -1;
    if (fread(header, 1, sizeof(header), file) != sizeof(header) ||
        memcmp(header, kCheckpointMagic, sizeof(kCheckpointMagic)) != 0 ||
        get_u32(header + 8) != CHECKPOINT_VERSION ||
        get_u32(header + 12) != PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES ||
        get_u32(header + 16) != PC_PORT_QUICK_CHECKPOINT_STATE_BYTES ||
        fread(loaded.game_state, 1, sizeof(loaded.game_state), file) !=
            sizeof(loaded.game_state)) {
        fclose(file);
        return -1;
    }
    trailing = fgetc(file);
    fclose(file);
    if (trailing != EOF)
        return -1;

    expected = get_u32(header + 20);
    if (checkpoint_checksum(header, loaded.game_state) != expected)
        return -1;
    loaded.map = get_u16(header + 24);
    loaded.entrance = get_u16(header + 26);
    loaded.position[0] = (int32_t)get_u32(header + 28);
    loaded.position[1] = (int32_t)get_u32(header + 32);
    loaded.position[2] = (int32_t)get_u32(header + 36);
    loaded.rotation[0] = (int16_t)get_u16(header + 40);
    loaded.rotation[1] = (int16_t)get_u16(header + 42);
    loaded.rotation[2] = (int16_t)get_u16(header + 44);
    if (sidecar_path(side, sizeof side, path) == 0 && (file = fopen(side, "rb")) != NULL) {
        size_t got = fread(sidecar, 1, sizeof(sidecar), file);
        fclose(file);
        if (got == SEED_SIDECAR_BYTES && memcmp(sidecar, kSeedTag, sizeof(kSeedTag)) == 0 &&
            get_u32(sidecar + 8) == expected && get_u32(sidecar + 12) == seed_check(sidecar)) {
            loaded.has_random_seed = 1;
            loaded.random_seed = get_u32(sidecar + 4);
        }
    }
    *checkpoint = loaded;
    return 0;
}
