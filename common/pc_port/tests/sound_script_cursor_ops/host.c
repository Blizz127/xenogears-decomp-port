#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef struct AudioManager AudioManager;
typedef struct AudioElement AudioElement;
typedef u8 *CursorCommand(u8 *, AudioManager *, AudioElement *);

CursorCommand SoundScriptNop;
CursorCommand SoundScriptNop2;
CursorCommand SoundScriptNop3;
CursorCommand SoundScriptNop4;
CursorCommand SoundScriptNop5;
CursorCommand SoundScriptDefaultHandler;
CursorCommand SoundScriptDefaultSkip3;
CursorCommand SoundScriptSkip3;

#ifdef BUILD_COMMANDS
#ifndef CURSOR_FRAGMENT
#define CURSOR_FRAGMENT "../../src/cleanroom/sound_script_cursor_ops.inc"
#endif
#include CURSOR_FRAGMENT
#else

static const struct {
    const char *name;
    CursorCommand *call;
    unsigned consumed;
} commands[] = {
    {"Nop", SoundScriptNop, 0}, {"Nop2", SoundScriptNop2, 0},
    {"Nop3", SoundScriptNop3, 0}, {"Nop4", SoundScriptNop4, 0},
    {"Nop5", SoundScriptNop5, 0}, {"DefaultHandler", SoundScriptDefaultHandler, 0},
    {"DefaultSkip3", SoundScriptDefaultSkip3, 3}, {"Skip3", SoundScriptSkip3, 3}
};

#ifdef OBSERVE_ACCESSES
/* Only the candidate translation unit is instrumented. These operations must
 * emit zero data accesses, including when the addresses are mapped. */
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define FORBID_ACCESS(kind, width) \
    void __tsan_##kind##width(void *p) { (void)p; abort(); }
#define FORBID_WIDTH(width) \
    FORBID_ACCESS(read, width) FORBID_ACCESS(write, width) \
    FORBID_ACCESS(volatile_read, width) FORBID_ACCESS(volatile_write, width)
FORBID_WIDTH(1)
FORBID_WIDTH(2)
FORBID_WIDTH(4)
FORBID_WIDTH(8)
FORBID_WIDTH(16)
#endif

static unsigned calls;
static void exercise(uintptr_t cursor, uintptr_t manager, uintptr_t element)
{
    for (size_t i = 0; i < sizeof commands / sizeof commands[0]; ++i) {
        uintptr_t result = (uintptr_t)commands[i].call((u8 *)cursor,
                    (AudioManager *)manager, (AudioElement *)element);
        if (result != cursor + commands[i].consumed) {
            fprintf(stderr, "%s returned the wrong cursor\n", commands[i].name);
            abort();
        }
        ++calls;
    }
}

int main(void)
{
    unsigned char region[512], prior[sizeof region];
    for (size_t i = 0; i < sizeof region; ++i)
        region[i] = (unsigned char)(i * 149u + 17u);
    memcpy(prior, region, sizeof region);
    const uintptr_t buf = (uintptr_t)region;
    const uintptr_t offsets[] = {0, 1, 2, 3, 255, 508, 509, 510, 511, 512};
    for (size_t i = 0; i < sizeof offsets / sizeof offsets[0]; ++i) {
        uintptr_t cursor = buf + offsets[i];
        exercise(cursor, buf + 64, buf + 128);
        exercise(cursor, cursor, cursor);
        exercise(cursor, 0, 0);
    }
    assert(memcmp(region, prior, sizeof region) == 0);

    const uintptr_t words[] = {
        0, 1, 2, 3, 0x7fffffffu, 0x80000000u, 0xfffffffcu, 0xfffffffdu,
        0xfffffffeu, 0xffffffffu, UINTPTR_MAX-3, UINTPTR_MAX-2, UINTPTR_MAX-1,
        UINTPTR_MAX
#if UINTPTR_MAX > UINT32_MAX
        , UINT64_C(0x100000000), UINT64_C(0x1fffffffe), UINT64_C(0x7fff12345678),
        UINT64_C(0xffff800000000003)
#endif
    };
    for (size_t i = 0; i < sizeof words / sizeof words[0]; ++i) {
        uintptr_t cursor = words[i];
        exercise(cursor, 0, UINTPTR_MAX);
        exercise(cursor, cursor, cursor);
        exercise(cursor, buf + 3, buf + 3);
    }
    uint32_t state = 0x645ac23u;
    for (unsigned i = 0; i < 512; ++i) {
        state = state * UINT32_C(1664525) + UINT32_C(1013904223);
        uintptr_t cursor = state;
#if UINTPTR_MAX > UINT32_MAX
        cursor |= ((uintptr_t)(state ^ UINT32_C(0xc3508e7d))) << 32;
#endif
        exercise(cursor, ~cursor, cursor ^ 3u);
    }
    assert(memcmp(region, prior, sizeof region) == 0);
    printf("%u cursor calls passed (%zu-bit native address)\n", calls, sizeof(uintptr_t)*8);
    return 0;
}
#endif
