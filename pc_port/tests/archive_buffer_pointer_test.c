/* Actual archive wrapper: verify host pointers survive its retail call order.
 * Decode/CD/read boundaries are spies; no filesystem or CD transfer is made. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>

extern int ArchiveReadFileToBuffer(int, void *, unsigned, unsigned);
unsigned g_CurArchiveOffset;
int D_8004FE18, g_ArchiveCurFileSize;

int g_ArchiveCurFileSector;
static int index_arg, decoded_size, calls, cases;
static void *destination;
static unsigned channel_arg, flags_arg;

static void check(int condition, const char *message) {
    if (!condition) {
        fprintf(stderr, "ARCHIVE BUFFER FAIL case=%d calls=%d %s\n",
                cases, calls, message);
        exit(1);
    }
}

int ArchiveDecodeSize(int index) {
    check(calls++ == 0 && index == index_arg, "decode size order/argument");
    return decoded_size;
}
void ArchiveCdDataSync(int mode) {
    check(calls++ == 1 && mode == 0, "sync order/argument");
    g_CurArchiveOffset = 0x345;
}
int ArchiveDecodeSector(int index) {
    check(calls++ == 2 && index == index_arg, "decode sector order/argument");
    check(D_8004FE18 == 0x345, "offset reloaded after sync");
    return 0x678;
}
int ArchiveDecodeAlignedSize(unsigned index) {
    check(calls++ == 3 && index == (unsigned)index_arg, "aligned size order/argument");
    check(g_ArchiveCurFileSector == 0x678, "sector stored before size decode");
    return 0x1800;
}
int ArchiveReadFile(int index, void *buffer, unsigned channel, unsigned flags) {
    check(calls++ == 4 && index == index_arg, "read order/index");
    check(buffer == destination, "native destination pointer truncated");
    check(channel == channel_arg && flags == flags_arg, "read arguments");
    check(g_ArchiveCurFileSize == 0x1800, "size stored before read");
    return -123;
}

static void run(int index, int size, void *buffer, unsigned channel, unsigned flags) {
    index_arg = index; decoded_size = size; destination = buffer;
    channel_arg = channel; flags_arg = flags; calls = 0;
    g_CurArchiveOffset = 0xABC; D_8004FE18 = 0x111;
    g_ArchiveCurFileSector = 0x222; g_ArchiveCurFileSize = 0x333;
    int result = ArchiveReadFileToBuffer(index, buffer, channel, flags);
    if (index <= 0 || size <= 0 || !buffer) {
        check(result == -3, "invalid request result");
        check(calls == (index > 0), "invalid request short circuit");
        check(D_8004FE18 == 0x111 && g_ArchiveCurFileSector == 0x222 &&
              g_ArchiveCurFileSize == 0x333, "invalid request changed state");
    } else {
        check(result == -123 && calls == 5, "read result/call count");
    }
    ++cases;
}

int main(void) {
    void *buffer = mmap(NULL, 4096, PROT_READ | PROT_WRITE,
                        MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    check(buffer != MAP_FAILED && (uintptr_t)buffer > UINT32_MAX,
          "high host mapping required");
    run(0, 1, buffer, 0, 0);
    run(-1, 1, buffer, 0, 0);
    run(1, 0, buffer, 0, 0);
    run(1, -1, buffer, 0, 0);
    run(1, 1, NULL, 0, 0);
    run(1, 1, buffer, 0, 0x80);
    run(32767, 2048, buffer, 0x81234567u, 0xFEDCBA98u);
    check(munmap(buffer, 4096) == 0, "unmap");
    printf("ARCHIVE BUFFER PASS cases=%d\n", cases);
    return 0;
}
