#include <assert.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint16_t u16;
typedef uint32_t u32;
typedef unsigned long u_long;
typedef void (*SoundCommandCallback_t)(void);
_Static_assert(sizeof(int) == 4, "WDS transfer parameter size");

extern int g_SoundWdsCurSpuAddress, g_SoundWdsRemainingBytes;
extern u16 g_SoundTransferQueueWriteIndex, g_SoundTransferQueueReadIndex;
void SoundQueueTransferCommand(u32, void *, u_long, SoundCommandCallback_t, u16);

#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_transfer_controls.inc"
#else
void SoundWdsSetTransferParamters(int, int);
int SoundTransferQueueSync(void);
void SoundQueueSpuWriteCommand(u32, void *, u_long, SoundCommandCallback_t);

int g_SoundWdsCurSpuAddress, g_SoundWdsRemainingBytes;
u16 g_SoundTransferQueueWriteIndex, g_SoundTransferQueueReadIndex;

struct Access { void *address; unsigned bytes, write; };
static struct Access sequence[2];
static unsigned seen, expected;
static unsigned setupCases, queueCases, poolCases;

static void beginSequence(void) { seen = expected = 0; }
static void expectAccess(void *address, unsigned bytes, unsigned write)
{
    assert(expected < sizeof sequence / sizeof *sequence);
    sequence[expected++] = (struct Access){address, bytes, write};
}
static void endSequence(void)
{
#ifdef OBSERVE_ACCESSES
    assert(seen == expected);
#endif
}

#ifdef OBSERVE_ACCESSES
static void observe(void *address, unsigned bytes, unsigned write)
{
    assert(seen < expected);
    const struct Access *next = &sequence[seen++];
    assert(next->address == address && next->bytes == bytes && next->write == write);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define ACCESS(n) \
    void __tsan_read##n(void *p) { observe(p, n, 0); } \
    void __tsan_write##n(void *p) { observe(p, n, 1); } \
    void __tsan_volatile_read##n(void *p) { observe(p, n, 0); } \
    void __tsan_volatile_write##n(void *p) { observe(p, n, 1); }
ACCESS(1)
ACCESS(2)
ACCESS(4)
ACCESS(8)
#undef ACCESS
#endif

static unsigned providerCalls;
static struct {
    u32 address;
    void *buffer;
    u_long size;
    SoundCommandCallback_t callback;
} arguments;

void SoundQueueTransferCommand(u32 address, void *buffer, u_long size,
                               SoundCommandCallback_t callback, u16 command)
{
    assert(providerCalls++ == 0);
    assert(address == arguments.address && buffer == arguments.buffer);
    assert(size == arguments.size && callback == arguments.callback && command == 1);
    /* The real provider owns the queue. Detect wrappers that restore old state. */
    g_SoundTransferQueueWriteIndex = 3;
}

static void unusedCallback(void) { assert(0 && "The adapter must only forward its callback"); }
static void otherCallback(void) { assert(0 && "The adapter must only forward its callback"); }

static void setup(int address, int length, unsigned equality)
{
    g_SoundWdsCurSpuAddress = (equality & 1) ? address : ~address;
    g_SoundWdsRemainingBytes = (equality & 2) ? length : ~length;
    g_SoundTransferQueueWriteIndex = 0x23;
    g_SoundTransferQueueReadIndex = 0x45;
    providerCalls = 0;
    beginSequence();
    expectAccess(&g_SoundWdsCurSpuAddress, 4, 1);
    expectAccess(&g_SoundWdsRemainingBytes, 4, 1);
    SoundWdsSetTransferParamters(address, length);
    endSequence();
    assert(providerCalls == 0 && g_SoundWdsCurSpuAddress == address && g_SoundWdsRemainingBytes == length);
    assert(g_SoundTransferQueueWriteIndex == 0x23 && g_SoundTransferQueueReadIndex == 0x45);
    ++setupCases;
}

static void testWrapper(void)
{
    unsigned char data[] = {0, 127, 255, 193};
    const unsigned char before[] = {0, 127, 255, 193};
    void *buffers[] = {NULL, data + 1, &g_SoundTransferQueueReadIndex, &g_SoundWdsRemainingBytes};
    SoundCommandCallback_t callbacks[] = {NULL, unusedCallback, otherCallback};
    const u32 addresses[] = {0, 1, UINT32_C(0x80000000), UINT32_MAX};
    const u_long sizes[] = {0, 1, 7, UINT32_MAX, ULONG_MAX, ULONG_MAX - 17};
    for (unsigned i = 0; i < sizeof addresses / sizeof *addresses; ++i)
        for (unsigned j = 0; j < sizeof sizes / sizeof *sizes; ++j)
            for (unsigned b = 0; b < sizeof buffers / sizeof *buffers; ++b)
                for (unsigned c = 0; c < sizeof callbacks / sizeof *callbacks; ++c) {
                    g_SoundWdsCurSpuAddress = INT_MIN;
                    g_SoundWdsRemainingBytes = INT_MAX;
                    g_SoundTransferQueueWriteIndex = 4;
                    g_SoundTransferQueueReadIndex = 7;
                    providerCalls = 0;
                    arguments.address = addresses[i];
                    arguments.buffer = buffers[b];
                    arguments.size = sizes[j];
                    arguments.callback = callbacks[c];
                    beginSequence();
                    SoundQueueSpuWriteCommand(addresses[i], buffers[b], sizes[j], callbacks[c]);
                    endSequence();
                    assert(providerCalls == 1 && !memcmp(data, before, sizeof before));
                    assert(g_SoundWdsCurSpuAddress == INT_MIN && g_SoundWdsRemainingBytes == INT_MAX);
                    assert(g_SoundTransferQueueWriteIndex == 3 && g_SoundTransferQueueReadIndex == 7);
                    ++queueCases;
                }
}

static void sample(u16 written, u16 read)
{
    /* A writer behind the reader can pass only at distances one and two,
     * and only when adding eight fits in the 16-bit stored index. */
    const int want = written >= read ? written - read >= 6 :
                     written <= UINT16_MAX - 8 && read - written <= 2;
    g_SoundTransferQueueWriteIndex = written;
    g_SoundTransferQueueReadIndex = read;
    providerCalls = 0;
    beginSequence();
    expectAccess(&g_SoundTransferQueueWriteIndex, 2, 0);
    expectAccess(&g_SoundTransferQueueReadIndex, 2, 0);
    assert(SoundTransferQueueSync() == want);
    endSequence();
    assert(providerCalls == 0);
    assert(g_SoundTransferQueueWriteIndex == written && g_SoundTransferQueueReadIndex == read);
    ++poolCases;
}

int main(void)
{
    const int inputs[] = {0, 1, -1, INT_MIN, INT_MAX, 229376, 195600};
    for (unsigned i = 0; i < sizeof inputs / sizeof *inputs; ++i)
        for (unsigned j = 0; j < sizeof inputs / sizeof *inputs; ++j)
            for (unsigned k = 0; k < 4; ++k)
                setup(inputs[i], inputs[j], k);
    testWrapper();
    for (unsigned written = 0; written <= UINT16_MAX; ++written) {
        for (int delta = -8; delta <= 8; ++delta)
            sample((u16)written, (u16)(written + delta));
        sample((u16)written, 0);
        sample((u16)written, UINT16_MAX);
    }
    printf("%u WDS setups, %u write submissions and %u queue snapshots passed\n",
           setupCases, queueCases, poolCases);
    return 0;
}
#endif
