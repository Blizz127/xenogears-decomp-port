#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "quick_checkpoint_file.h"

static int require(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    char directory[] = "/tmp/xeno-quick-checkpoint-XXXXXX";
    char path[512];
    PcPortQuickCheckpoint written;
    PcPortQuickCheckpoint loaded;
    FILE* file;
    int byte;
    size_t i;

    if (!mkdtemp(directory))
        return 2;
    snprintf(path, sizeof(path), "%s/quick.xgqs", directory);

    memset(&written, 0, sizeof(written));
    written.map = 14;
    written.entrance = 3;
    written.position[0] = 123 << 16;
    written.position[1] = -7 * 65536;
    written.position[2] = 456 << 16;
    written.rotation[0] = 0x120;
    written.rotation[1] = 0x920;
    written.rotation[2] = 0x120;
    for (i = 0; i < sizeof(written.game_state); ++i)
        written.game_state[i] = (uint8_t)(i * 37u + 11u);
    written.has_random_seed = 1;
    written.random_seed = 0x62264E95u;

    if (!require(PcPort_QuickCheckpointWriteFile(path, &written) == 0,
                 "write valid checkpoint"))
        return 1;
    if (!require(PcPort_QuickCheckpointReadFile(path, &loaded) == 0,
                 "read valid checkpoint"))
        return 1;
    if (!require(memcmp(&written, &loaded, sizeof(written)) == 0,
                 "checkpoint round trip (with rand seed sidecar)"))
        return 1;

    /* The main file keeps the original format (older builds load it); the
     * seed lives in the "<path>.rng" sidecar. */
    {
        char side[600];
        FILE* f;
        long size;
        snprintf(side, sizeof(side), "%s.rng", path);
        f = fopen(path, "rb");
        if (!f)
            return 2;
        fseek(f, 0, SEEK_END);
        size = ftell(f);
        fclose(f);
        if (!require(size == (long)(PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES +
                                    PC_PORT_QUICK_CHECKPOINT_STATE_BYTES),
                     "main checkpoint size unchanged by the seed"))
            return 1;
        if (!require(access(side, F_OK) == 0, "seed sidecar written"))
            return 1;

        /* A checkpoint without a sidecar (all older saves) loads, seedless. */
        unlink(side);
        if (!require(PcPort_QuickCheckpointReadFile(path, &loaded) == 0 &&
                     !loaded.has_random_seed &&
                     memcmp(written.game_state, loaded.game_state, sizeof(loaded.game_state)) == 0,
                     "checkpoint without sidecar loads without a seed"))
            return 1;

        /* A stale sidecar (from another checkpoint) is ignored. */
        if (!require(PcPort_QuickCheckpointWriteFile(path, &written) == 0, "rewrite"))
            return 1;
        {
            PcPortQuickCheckpoint other = written;
            char keep[sizeof(side) + 8];
            snprintf(keep, sizeof(keep), "%s.keep", side);
            rename(side, keep);
            other.game_state[0] ^= 0xFF;
            other.has_random_seed = 0;
            if (!require(PcPort_QuickCheckpointWriteFile(path, &other) == 0, "write other"))
                return 1;
            rename(keep, side);
            if (!require(PcPort_QuickCheckpointReadFile(path, &loaded) == 0 &&
                         !loaded.has_random_seed,
                         "stale sidecar from another checkpoint is ignored"))
                return 1;
        }

        /* A corrupt sidecar is ignored, the checkpoint still loads. */
        if (!require(PcPort_QuickCheckpointWriteFile(path, &written) == 0, "rewrite"))
            return 1;
        f = fopen(side, "r+b");
        if (!f)
            return 2;
        fseek(f, 5, SEEK_SET);
        fputc(0xEE, f);
        fclose(f);
        if (!require(PcPort_QuickCheckpointReadFile(path, &loaded) == 0 &&
                     !loaded.has_random_seed,
                     "corrupt sidecar is ignored"))
            return 1;

        /* Trailing bytes in the main file are still rejected. */
        if (!require(PcPort_QuickCheckpointWriteFile(path, &written) == 0, "rewrite"))
            return 1;
        f = fopen(path, "ab");
        if (!f)
            return 2;
        fputs("junk", f);
        fclose(f);
        if (!require(PcPort_QuickCheckpointReadFile(path, &loaded) != 0,
                     "reject trailing bytes in the main file"))
            return 1;
        if (!require(PcPort_QuickCheckpointWriteFile(path, &written) == 0, "rewrite"))
            return 1;
        unlink(side);
        written.has_random_seed = 0;
        written.random_seed = 0;
        if (!require(PcPort_QuickCheckpointWriteFile(path, &written) == 0, "rewrite seedless"))
            return 1;
    }
    file = fopen(path, "r+b");
    if (!file)
        return 2;
    fseek(file, PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES + 100, SEEK_SET);
    byte = fgetc(file);
    fseek(file, -1, SEEK_CUR);
    fputc(byte ^ 0x80, file);
    fclose(file);
    if (!require(PcPort_QuickCheckpointReadFile(path, &loaded) != 0,
                 "reject corrupt payload"))
        return 1;

    unlink(path);
    rmdir(directory);
    puts("quick checkpoint file regression: PASS");
    return 0;
}
