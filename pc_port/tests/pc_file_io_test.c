#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "psyq/pc.h"

int main(void)
{
    char path[] = "/tmp/xeno-pc-io.XXXXXX";
    char input[0x10001], output[sizeof(input) + 8];
    int seed = mkstemp(path), fd, other;
    assert(seed >= 0 && close(seed) == 0);
    for (size_t i = 0; i < sizeof(input); i++) input[i] = (char)(i * 37);
    assert(PCinit() == 0);
    fd = PCcreate(path, 0);
    assert(fd >= 0);
    assert(PCwrite(fd, input, sizeof(input)) == (int)sizeof(input));
    assert(PClseek(fd, 0, SEEK_CUR) == (int)sizeof(input));
    other = PCopen(path, O_RDONLY, 0);
    assert(other >= 0 && other != fd);
    memset(output, 0x5A, sizeof(output));
    assert(PCread(other, output, sizeof(output)) == (int)sizeof(input));
    assert(memcmp(input, output, sizeof(input)) == 0);
    assert(output[sizeof(input)] == 0x5A);
    assert(PCread(other, output, 1) == 0);
    assert(PCwrite(other, input, 1) == -1);
    assert(PClseek(fd, -3, SEEK_END) == (int)sizeof(input) - 3);
    assert(PCread(fd, output, 3) == 3);
    assert(memcmp(output, input + sizeof(input) - 3, 3) == 0);
    assert(PCclose(other) == 0 && PCclose(fd) == 0);
    fd = PCopen(path, O_WRONLY, 0);
    assert(fd >= 0 && PClseek(fd, 0, SEEK_END) == (int)sizeof(input));
    assert(PCclose(fd) == 0);
    fd = PCopen(path, O_RDWR, 0);
    assert(fd >= 0 && PCread(fd, output, 1) == 1);
    assert(PCclose(fd) == 0);
    assert(PCopen(path, 3, 0) == -1);
    assert(PCread(-1, output, 1) == -1);
    assert(PCwrite(-1, input, 1) == -1);
    assert(PCread(-1, output, 0) == 0);
    assert(PCread(fd, output, -1) == -1);
    assert(PClseek(-1, 0, SEEK_SET) == -1);
    assert(PCclose(-1) == -1);
    fd = PCcreate(path, 0);
    assert(fd >= 0 && PClseek(fd, 0, SEEK_END) == 0);
    assert(PCclose(fd) == 0);
    assert(unlink(path) == 0);
    assert(PCopen(path, O_RDONLY, 0) == -1);
    puts("PC FILE IO PASS: lifecycle, byte counts, multi-chunk, EOF, modes, errors");
    return 0;
}
