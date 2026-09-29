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

    if (!require(PcPort_QuickCheckpointWriteFile(path, &written) == 0,
                 "write valid checkpoint"))
        return 1;
    if (!require(PcPort_QuickCheckpointReadFile(path, &loaded) == 0,
                 "read valid checkpoint"))
        return 1;
    if (!require(memcmp(&written, &loaded, sizeof(written)) == 0,
                 "checkpoint round trip"))
        return 1;

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
