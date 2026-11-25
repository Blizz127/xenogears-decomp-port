#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef void (*SpuTransferCallbackProc)(void);
typedef void (*SpuIRQCallbackProc)(void);

#define RCntCNT2 0xf2000002u
#define SPU_OFF 0
#define SPU_REV_MODE_OFF 0

extern s16 g_SoundControlFlags;
extern s16 g_SoundSpuErrorId;
extern u32 g_unk_SoundEvent;

void SoundReset(void);
void SoundHandleError(s32 error_id);
int EnterCriticalSection(void);
void ExitCriticalSection(void);
int StopRCnt(int descriptor);
int CloseEvent(unsigned int event);
int SpuSetIRQ(int enabled);
SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc callback);
SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc callback);
void SoundSetVoiceAdsrReleaseShiftAndMode(s32 channel, s32 shift, s32 mode);
void SoundSetVoiceKeyOff(u32 voices);
void SpuSetReverbModeDepth(short left, short right);
int SpuSetReverbModeType(int mode);

#ifdef BUILD_CANDIDATE
#include SOUND_RESET_FRAGMENT
#else
s16 g_SoundControlFlags;
s16 g_SoundSpuErrorId;
u32 g_unk_SoundEvent;

enum event {
    READ_FLAGS, WRITE_FLAGS, READ_HANDLE, WRITE_ERROR,
    ERROR_SERVICE, CRITICAL, IRQ, TRANSFER_CALLBACK, IRQ_CALLBACK, STOP, CLOSE,
    EXIT_CRITICAL, VOICE, KEY_OFF, DEPTH, MODE
};
struct event_call {
    enum event event;
    u32 a, b, c;
};
static struct event_call plan[48];
static size_t planned, current;
static unsigned behavior;
static u32 expected_handle;
static s16 expected_flags, expected_error;
static int service_result;

static void add(enum event event, u32 a, u32 b, u32 c)
{
    assert(planned < sizeof(plan) / sizeof(plan[0]));
    plan[planned++] = (struct event_call){event, a, b, c};
}

static void record(enum event event, u32 a, u32 b, u32 c)
{
    assert(current < planned);
    const struct event_call expected = plan[current++];
    assert(expected.event == event);
    assert(a == expected.a && b == expected.b && c == expected.c);
}

static void change(enum event event)
{
    /* A successful critical entry is followed by the parent's unconditional flags reset.
       Make spy state arbitrarily different everywhere else to test preservation. */
    if (event == IRQ) {
        expected_flags = 0;
    }
    assert(g_SoundControlFlags == expected_flags);
    assert(g_SoundSpuErrorId == expected_error);
    assert(g_unk_SoundEvent == expected_handle);
    if (behavior) {
        expected_flags = (s16)(0xa000 + current);
        expected_error = (s16)(0x9000 + current);
        if (behavior == 2 && event == CRITICAL) {
            expected_flags = 0;
        }
        if (behavior == 2 && event == MODE) {
            expected_error = 0;
        }
        if (event == STOP) {
            expected_handle ^= 0x89123456u;
        } else if (event == CLOSE || event == ERROR_SERVICE) {
            expected_handle ^= 0xffffffffu;
        }
        g_SoundControlFlags = expected_flags;
        g_SoundSpuErrorId = expected_error;
        g_unk_SoundEvent = expected_handle;
    }
}

static void service(enum event event, u32 a, u32 b, u32 c)
{
    record(event, a, b, c);
    change(event);
}

#ifdef OBSERVE_ACCESSES
static void access_event(void *address, unsigned width, int write)
{
    if (address == &g_SoundControlFlags) {
        assert(width == 2);
        record(write ? WRITE_FLAGS : READ_FLAGS, 0, 0, 0);
    } else if (address == &g_unk_SoundEvent) {
        assert(width == 4 && !write);
        record(READ_HANDLE, 0, 0, 0);
    } else if (address == &g_SoundSpuErrorId) {
        assert(width == 2 && write);
        record(WRITE_ERROR, 0, 0, 0);
    }
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define ACCESS(n) \
    void __tsan_read##n(void *p) { access_event(p,n,0); } \
    void __tsan_write##n(void *p) { access_event(p,n,1); } \
    void __tsan_volatile_read##n(void *p) { access_event(p,n,0); } \
    void __tsan_volatile_write##n(void *p) { access_event(p,n,1); }
ACCESS(1)
ACCESS(2)
ACCESS(4)
ACCESS(8)
#undef ACCESS
#define PLAN_ACCESS(e) add(e, 0, 0, 0)
#else
#define PLAN_ACCESS(e) ((void)0)
#endif

static void prior_callback(void) {}
void SoundHandleError(s32 code) { service(ERROR_SERVICE, (u32)code, 0, 0); }
int EnterCriticalSection(void) { service(CRITICAL, 0, 0, 0); return service_result; }
void ExitCriticalSection(void) { service(EXIT_CRITICAL, 0, 0, 0); }
int SpuSetIRQ(int x) { service(IRQ, (u32)x, 0, 0); return service_result; }
SpuTransferCallbackProc SpuSetTransferCallback(SpuTransferCallbackProc f)
{
    assert(f == NULL);
    service(TRANSFER_CALLBACK, 0, 0, 0);
    return behavior ? prior_callback : NULL;
}
SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc f)
{
    assert(f == NULL);
    service(IRQ_CALLBACK, 0, 0, 0);
    return behavior ? prior_callback : NULL;
}
int StopRCnt(int id) { service(STOP, (u32)id, 0, 0); return service_result; }
int CloseEvent(unsigned int id) { service(CLOSE, id, 0, 0); return service_result; }
void SoundSetVoiceAdsrReleaseShiftAndMode(s32 channel, s32 shift, s32 mode)
{
    service(VOICE, (u32)channel, (u32)shift, (u32)mode);
}
void SoundSetVoiceKeyOff(u32 voices) { service(KEY_OFF, voices, 0, 0); }
void SpuSetReverbModeDepth(short left, short right)
{
    service(DEPTH, (u32)left, (u32)right, 0);
}
int SpuSetReverbModeType(int mode) { service(MODE, (u32)mode, 0, 0); return service_result; }

static void check(u16 flags, u32 handle, s16 last_error, unsigned mutation, int outcome)
{
    current = planned = 0;
    behavior = mutation;
    service_result = outcome;
    g_unk_SoundEvent = expected_handle = handle;
    g_SoundControlFlags = expected_flags = (s16)flags;
    g_SoundSpuErrorId = expected_error = last_error;
    PLAN_ACCESS(READ_FLAGS);
    if (!flags) {
        add(ERROR_SERVICE, 41, 0, 0);
    } else {
        add(CRITICAL, 0, 0, 0);
        PLAN_ACCESS(WRITE_FLAGS);
        add(IRQ, 0, 0, 0);
        add(TRANSFER_CALLBACK, 0, 0, 0);
        add(IRQ_CALLBACK, 0, 0, 0);
        add(STOP, RCntCNT2, 0, 0);
        PLAN_ACCESS(READ_HANDLE);
        add(CLOSE, mutation ? (handle ^ 0x89123456u) : handle, 0, 0);
        add(EXIT_CRITICAL, 0, 0, 0);
        for (unsigned voice = 0; voice < 24; ++voice) {
            add(VOICE, voice, 6, 3);
        }
        add(KEY_OFF, 0xffffffu, 0, 0);
        add(DEPTH, 0, 0, 0);
        add(MODE, 0, 0, 0);
        PLAN_ACCESS(WRITE_ERROR);
    }

    SoundReset();
    assert(current == planned);
    assert(g_unk_SoundEvent == expected_handle);
    assert(g_SoundControlFlags == expected_flags);
    assert(g_SoundSpuErrorId == (flags ? 0 : expected_error));
}

int main(void)
{
    const u32 handles[] = {0, 1, 0x7fffffffu, 0x80000000u, 0xffffffffu};
    unsigned cases = 0;
    for (unsigned bit = 0; bit < 21; ++bit) {
        u16 flags;
        if (bit < 16) {
            flags = (u16)(1u << bit);
        } else {
            static const u16 extras[] = {0, 0xb901, 0xffff, 0x0088, 0xfffe};
            flags = extras[bit - 16];
        }
        for (unsigned i = 0; i < sizeof(handles) / sizeof(handles[0]); ++i) {
            for (unsigned mutation = 0; mutation < 3; ++mutation) {
                check(flags, handles[i], i ? (s16)(0x7000 + i) : 0, mutation, (int)handles[i]);
                ++cases;
            }
        }
    }
    printf("passed %u reset calls\n", cases);
    return 0;
}
#endif
