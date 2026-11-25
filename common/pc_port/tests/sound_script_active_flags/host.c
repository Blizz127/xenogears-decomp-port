#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef struct { u8 guard[19]; } AudioManager;
typedef struct {
    u16 active_flag;
    u8 guard[342];
} AudioElement;

#include "../../src/cleanroom/sound_script_active_flag800.inc"

typedef u8 *(*Command)(u8 *, AudioManager *, AudioElement *);

static void test(Command command, int set)
{
    AudioManager manager;
    u8 script[13], previous_script[sizeof script];
    memset(&manager, 0x8b, sizeof manager);
    memset(script, 0xa7, sizeof script);
    memcpy(previous_script, script, sizeof script);
    for (unsigned initial = 0; initial < 65536; ++initial) {
        struct { u16 first_guard; AudioElement object; u16 last_guard; } guarded, expected;
        memset(&guarded, initial & 255, sizeof guarded);
        guarded.object.active_flag = (u16)initial;
        memcpy(&expected, &guarded, sizeof guarded);
        expected.object.active_flag = (u16)((initial & 0xf7ffu) + (set ? 0x800u : 0u));
        u8 *cursor = NULL;
        AudioManager *context = NULL;
        switch (initial % 4) {
        case 0: cursor = script + 7; context = &manager; break;
        case 1: cursor = (u8 *)&guarded.object; break;
        case 2: cursor = (u8 *)&guarded.object + 1; break;
        case 3: break;
        }
        assert(command(cursor, context, &guarded.object) == cursor);
        assert(!memcmp(&guarded, &expected, sizeof guarded));
    }
    for (size_t i = 0; i < sizeof manager.guard; ++i) assert(manager.guard[i] == 0x8b);
    assert(!memcmp(script, previous_script, sizeof script));
}

int main(void)
{
    test(SoundScriptSetActiveFlag800, 1);
    test(SoundScriptClearActiveFlag800, 0);
    puts("131072 flag cases passed");
    return 0;
}
