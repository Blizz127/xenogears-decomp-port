#ifndef XENO_PC_PORT_QUICK_CHECKPOINT_FILE_H
#define XENO_PC_PORT_QUICK_CHECKPOINT_FILE_H

#include <stdint.h>

#define PC_PORT_QUICK_CHECKPOINT_STATE_BYTES 0x2358u
#define PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES 48u

typedef struct PcPortQuickCheckpoint {
    uint8_t game_state[PC_PORT_QUICK_CHECKPOINT_STATE_BYTES];
    uint16_t map;
    uint16_t entrance;
    int32_t position[3];
    int16_t rotation[3];
} PcPortQuickCheckpoint;

/* Return 0 on success; all failures are reported as nonzero. */
int PcPort_QuickCheckpointWriteFile(const char* path,
                                    const PcPortQuickCheckpoint* checkpoint);
int PcPort_QuickCheckpointReadFile(const char* path,
                                   PcPortQuickCheckpoint* checkpoint);

#endif
