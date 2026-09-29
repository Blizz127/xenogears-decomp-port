#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/sound.h"

extern void SoundSetupCdMix(s32 level);
extern void SoundTransferWdsPart(u8 *data, s32 size);

SoundSpuMemoryBlock g_SoundSpuMemoryBlocks[MAX_SPU_MEMORY_BLOCKS];
s16 g_SoundControlFlags;
SoundVolumeController g_SoundVolumeController;
CdlATV g_SoundCdRomAttenuation;
int g_SoundWdsCurSpuAddress;
int g_SoundWdsRemainingBytes;
SoundTransferCommand *g_SoundTransferQueue;
u16 g_SoundTransferQueueReadIndex;
u16 g_SoundTransferQueueWriteIndex;
s16 D_80059548;

static int s_cdMixCalls;
static CdlATV s_lastCdMix;
static void fail(const char *message);

void PsyX_SPUAL_SetCdAttenuation(u8 val0, u8 val1, u8 val2, u8 val3)
{
    s_cdMixCalls++;
    s_lastCdMix.val0 = val0;
    s_lastCdMix.val1 = val1;
    s_lastCdMix.val2 = val2;
    s_lastCdMix.val3 = val3;
}

SoundTransferCallbackToken SoundTransferCallbackStore(
    u16 queueIndex, SoundCommandCallback_t callback)
{
    (void)callback;
    return 0x51000000u | queueIndex;
}

SoundCommandCallback_t SoundTransferCallbackResolve(
    SoundTransferCallbackToken token)
{
    (void)token;
    return NULL;
}

void PsyX_Sys_SoundGateEnterCritical(void) {}
void PsyX_Sys_SoundGateExitCritical(void) {}
int EnterCriticalSection(void) { return 1; }
void ExitCriticalSection(void) {}
SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc callback)
{
    return callback;
}
long SpuSetTransferMode(long mode) { return mode; }
unsigned long SpuSetTransferStartAddr(unsigned long address) { return address; }
unsigned long SpuWrite(unsigned char *data, unsigned long size)
{
    (void)data;
    return size;
}
unsigned long SpuRead(unsigned char *data, unsigned long size)
{
    (void)data;
    return size;
}
long SpuReadDecodedData(SpuDecodedData *data, long flag)
{
    (void)data;
    return flag;
}
void SoundHandleError(s32 errorId)
{
    (void)errorId;
    fail("unexpected sound error path");
}

static void fail(const char *message)
{
    fprintf(stderr, "sound retail primitives: FAIL: %s\n", message);
    exit(1);
}

static void check(int condition, const char *message)
{
    if (!condition) {
        fail(message);
    }
}

static void reset_blocks(void)
{
    memset(g_SoundSpuMemoryBlocks, 0, sizeof(g_SoundSpuMemoryBlocks));
    g_SoundSpuMemoryBlocks[0].flags = SPU_MEMORY_RESERVED | SPU_MEMORY_IN_USE;
    g_SoundSpuMemoryBlocks[0].unk1 = 5;
    g_SoundSpuMemoryBlocks[0].spuAddress = 0;
    g_SoundSpuMemoryBlocks[0].size = 0x1010;
}

static void test_automatic_allocator(void)
{
    u32 address;
    int i;

    reset_blocks();
    address = SoundSpuMemoryAllocateBlock(0x1000, 0x55);
    check(address == 0x1010, "first-fit address after reserved block");
    check(g_SoundSpuMemoryBlocks[0].nextBlockIndex == 1,
          "reserved block links new allocation");
    check(g_SoundSpuMemoryBlocks[1].flags == SPU_MEMORY_IN_USE,
          "new allocation is in use");
    check(g_SoundSpuMemoryBlocks[1].unk1 == 0,
          "retail ignores allocator arg1 and clears block type");
    check(g_SoundSpuMemoryBlocks[1].spuAddress == 0x1010,
          "new block stores returned address");
    check(g_SoundSpuMemoryBlocks[1].size == 0x1000,
          "new block stores requested size");
    check(g_SoundSpuMemoryBlocks[1].nextBlockIndex == 0,
          "new tail terminates list");

    reset_blocks();
    g_SoundSpuMemoryBlocks[0].nextBlockIndex = 1;
    g_SoundSpuMemoryBlocks[1].flags = SPU_MEMORY_IN_USE;
    g_SoundSpuMemoryBlocks[1].spuAddress = 0x3000;
    g_SoundSpuMemoryBlocks[1].size = 0x1000;
    address = SoundSpuMemoryAllocateBlock(0x800, 9);
    check(address == 0x1010, "allocator uses first interior gap");
    check(g_SoundSpuMemoryBlocks[0].nextBlockIndex == 2,
          "interior allocation is inserted after predecessor");
    check(g_SoundSpuMemoryBlocks[2].nextBlockIndex == 1,
          "interior allocation preserves successor");

    reset_blocks();
    g_SoundSpuMemoryBlocks[0].nextBlockIndex = 1;
    g_SoundSpuMemoryBlocks[1].flags = SPU_MEMORY_IN_USE;
    g_SoundSpuMemoryBlocks[1].spuAddress = 0x1800;
    g_SoundSpuMemoryBlocks[1].size = 0x800;
    address = SoundSpuMemoryAllocateBlock(0x1000, 0);
    check(address == 0x2000, "allocator skips an undersized interior gap");
    check(g_SoundSpuMemoryBlocks[1].nextBlockIndex == 2,
          "tail allocation follows last occupied block");

    reset_blocks();
    g_SoundSpuMemoryBlocks[0].size = 0x7ff00;
    address = SoundSpuMemoryAllocateBlock(0x200, 0);
    check(address == 0, "allocator rejects an undersized final gap");

    reset_blocks();
    for (i = 1; i < MAX_SPU_MEMORY_BLOCKS; i++) {
        g_SoundSpuMemoryBlocks[i].flags = SPU_MEMORY_IN_USE;
    }
    address = SoundSpuMemoryAllocateBlock(0x100, 0);
    check(address == 0x1010,
          "allocator preserves retail descriptor-exhaustion return behavior");
    check(g_SoundSpuMemoryBlocks[0].flags == SPU_MEMORY_IN_USE &&
          g_SoundSpuMemoryBlocks[0].spuAddress == 0x1010,
          "descriptor exhaustion preserves retail slot-zero overwrite");
}

static void test_cd_mix(void)
{
    memset(&g_SoundVolumeController, 0, sizeof(g_SoundVolumeController));
    memset(&g_SoundCdRomAttenuation, 0, sizeof(g_SoundCdRomAttenuation));
    memset(&s_lastCdMix, 0, sizeof(s_lastCdMix));
    s_cdMixCalls = 0;
    g_SoundControlFlags = 0;

    SoundSetupCdMix(0x80);
    check(g_SoundVolumeController.unk_field2 == 0x80,
          "CD mix stores the unscaled requested level");
    check(s_cdMixCalls == 1, "CD mix calls the hardware adapter once");
    check(s_lastCdMix.val0 == 0x40 && s_lastCdMix.val1 == 0x40 &&
          s_lastCdMix.val2 == 0x40 && s_lastCdMix.val3 == 0x40,
          "CD mix applies the retail half-volume matrix");

    g_SoundControlFlags = 0x100;
    SoundSetupCdMix(0x7f);
    check(g_SoundVolumeController.unk_field2 == 0x7f,
          "muted CD mix still retains the request");
    check(s_lastCdMix.val0 == 0 && s_lastCdMix.val1 == 0 &&
          s_lastCdMix.val2 == 0 && s_lastCdMix.val3 == 0,
          "sound mode bits mute the CD matrix");
}

static void test_streamed_wds_transfer(void)
{
    SoundTransferCommand queue[SOUND_TRANSFER_QUEUE_SIZE];
    u8 data[0x1000];

    memset(queue, 0, sizeof(queue));
    memset(data, 0xa5, sizeof(data));
    g_SoundTransferQueue = queue;
    g_SoundTransferQueueReadIndex = 0;
    g_SoundTransferQueueWriteIndex = 0;
    g_SoundControlFlags = SOUND_CTL_FLAG_IRQ_HANDLER | 0x10;
    g_SoundWdsCurSpuAddress = 0x38000;
    g_SoundWdsRemainingBytes = 0x1000;

    SoundTransferWdsPart(data, 0x800);
    check(g_SoundTransferQueueWriteIndex == 1,
          "first WDS chunk is queued");
    check(queue[1].commandType == SOUND_SPU_COMMAND_WRITE,
          "WDS chunk queues an SPU write");
    check(queue[1].pTransferAddress == 0x38000,
          "WDS chunk uses the current SPU destination");
    check(queue[1].dataSize == 0x800,
          "WDS chunk preserves an in-range byte count");
    check(g_SoundWdsCurSpuAddress == 0x38800 &&
          g_SoundWdsRemainingBytes == 0x800,
          "WDS cursor advances after the queued write");

    SoundTransferWdsPart(data, 0x1000);
    check(g_SoundTransferQueueWriteIndex == 2,
          "final WDS chunk is queued");
    check(queue[2].pTransferAddress == 0x38800 &&
          queue[2].dataSize == 0x800,
          "final WDS chunk clips to remaining bytes");
    check(g_SoundWdsCurSpuAddress == 0x39000 &&
          g_SoundWdsRemainingBytes == 0,
          "final WDS chunk exhausts the transfer");

    SoundTransferWdsPart(data, 0x800);
    check(g_SoundTransferQueueWriteIndex == 2,
          "zero remaining bytes enqueue nothing");
}

int main(void)
{
    test_automatic_allocator();
    test_cd_mix();
    test_streamed_wds_transfer();
    puts("sound retail primitives: PASS");
    return 0;
}
