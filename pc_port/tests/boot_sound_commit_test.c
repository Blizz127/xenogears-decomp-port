#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "system/sound.h"
#include "boot_sound_commit.h"

SoundVolumeController g_SoundVolumeController;
u_long g_unk_SoundEvent;

static char s_trace[8];
static unsigned int s_traceLength;
static unsigned int s_disabledEvent;
static unsigned int s_enabledEvent;
static unsigned int s_submitCount;
static SpuCommonAttr *s_submittedPointer;
static SpuCommonAttr s_submittedValue;

static void fail(const char *message)
{
    fprintf(stderr, "boot sound common-attribute commit: FAIL: %s\n", message);
    exit(1);
}

static void check(int condition, const char *message)
{
    if (!condition) {
        fail(message);
    }
}

static void trace(char operation)
{
    if (s_traceLength >= sizeof(s_trace)) {
        fail("operation trace overflow");
    }
    s_trace[s_traceLength++] = operation;
}

int DisableEvent(unsigned int event)
{
    s_disabledEvent = event;
    trace('D');
    return 1;
}

int EnableEvent(unsigned int event)
{
    s_enabledEvent = event;
    trace('E');
    return 1;
}

void SpuSetCommonAttr(SpuCommonAttr *attr)
{
    check(attr == &g_SoundVolumeController.commonAttr,
          "backend did not receive the exact pending retail attribute pointer");
    check(attr->mask == 0x3CF,
          "pending mask was cleared before the backend submission");
    s_submitCount++;
    s_submittedPointer = attr;
    s_submittedValue = *attr;
    trace('S');
}

static void reset_trace(void)
{
    memset(s_trace, 0, sizeof(s_trace));
    s_traceLength = 0;
    s_disabledEvent = 0;
    s_enabledEvent = 0;
    s_submitCount = 0;
    s_submittedPointer = NULL;
    memset(&s_submittedValue, 0, sizeof(s_submittedValue));
}

static void test_pending_commit(void)
{
    memset(&g_SoundVolumeController, 0, sizeof(g_SoundVolumeController));
    reset_trace();
    g_unk_SoundEvent = 0x12345678UL;
    g_SoundVolumeController.commonAttr.mask = 0x3CF;
    g_SoundVolumeController.commonAttr.mvol.left = 0x3FFF;
    g_SoundVolumeController.commonAttr.mvol.right = 0x3FFF;
    g_SoundVolumeController.commonAttr.cd.volume.left = 0x7FFF;
    g_SoundVolumeController.commonAttr.cd.volume.right = 0x7FFF;
    g_SoundVolumeController.commonAttr.cd.reverb = 1;
    g_SoundVolumeController.commonAttr.cd.mix = 1;

    PcPort_CommitPendingBootSoundCommonAttr();

    check(s_traceLength == 3 && memcmp(s_trace, "DSE", 3) == 0,
          "commit order must be DisableEvent, SpuSetCommonAttr, EnableEvent");
    check(s_disabledEvent == 0x12345678U && s_enabledEvent == 0x12345678U,
          "sound event gate did not receive the retail event handle");
    check(s_submitCount == 1 &&
          s_submittedPointer == &g_SoundVolumeController.commonAttr,
          "pending attributes were not submitted exactly once");
    check(s_submittedValue.mask == 0x3CF &&
          s_submittedValue.mvol.left == 0x3FFF &&
          s_submittedValue.mvol.right == 0x3FFF &&
          s_submittedValue.cd.volume.left == 0x7FFF &&
          s_submittedValue.cd.volume.right == 0x7FFF &&
          s_submittedValue.cd.reverb == 1 &&
          s_submittedValue.cd.mix == 1,
          "backend did not receive the pending retail master/CD payload");
    check(g_SoundVolumeController.commonAttr.mask == 0,
          "pending mask was not cleared after submission");
}

static void test_empty_commit(void)
{
    memset(&g_SoundVolumeController, 0, sizeof(g_SoundVolumeController));
    reset_trace();
    g_unk_SoundEvent = 0x89ABU;

    PcPort_CommitPendingBootSoundCommonAttr();

    check(s_traceLength == 2 && memcmp(s_trace, "DE", 2) == 0,
          "empty commit must still bracket the retail event gate");
    check(s_submitCount == 0 && s_submittedPointer == NULL,
          "empty commit unexpectedly wrote hardware state");
    check(g_SoundVolumeController.commonAttr.mask == 0,
          "empty commit changed the pending mask");
}

int main(void)
{
    test_pending_commit();
    test_empty_commit();
    puts("boot sound pending common-attribute commit: PASS");
    return 0;
}
