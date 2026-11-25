#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
/* Synthetic records expose only the fields used by this fixture. */
#include <stddef.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef int32_t s32;

typedef struct {
    u8 prefix[0x1a];
    u8 unk_0x1a;
    u8 remainder[0x94 - 0x1b];
} AudioManager;

typedef struct {
    u8 prefix[0x26];
    u8 unk_0x26;
    u8 before_flags[0xce - 0x27];
    u16 unk_0xCE;
    u8 before_first[0xf6 - 0xd0];
    u16 unk_0xF6;
    u8 before_second[0x116 - 0xf8];
    u16 unk_0x116;
    u8 before_third[0x136 - 0x118];
    u16 unk_0x136;
    u8 remainder[0x158 - 0x138];
} AudioElement;

_Static_assert(offsetof(AudioManager, unk_0x1a) == 0x1a, "manager parameter");
_Static_assert(offsetof(AudioElement, unk_0x26) == 0x26, "instrument");
_Static_assert(offsetof(AudioElement, unk_0xCE) == 0xce, "primary flags");
_Static_assert(offsetof(AudioElement, unk_0xF6) == 0xf6, "first flags");
_Static_assert(offsetof(AudioElement, unk_0x116) == 0x116, "second flags");
_Static_assert(offsetof(AudioElement, unk_0x136) == 0x136, "third flags");
_Static_assert(sizeof(AudioElement) == 0x158, "element extent");

#ifdef BUILD_COMMANDS
#include "../../src/cleanroom/sound_script_state_ops.inc"
#else
typedef u8 *Handler(u8 *, AudioManager *, AudioElement *);
Handler SoundScriptSetManagerUnk1a, SoundScriptAddManagerUnk1a, SoundScriptCallE5BC;
Handler SoundScriptSetUnkCEAndF6, SoundScriptClearUnkCEAndF6;
Handler SoundScriptSetUnkCEAndUnk116, SoundScriptClearUnkCEAndUnk116, SoundScriptSetUnkCEAndUnk136;

typedef struct { void *address; unsigned width, write; } Access;
static Access events[4];
static unsigned expected_count, seen, calls, instrument;
static AudioElement *service_element;

static void expect(void *p, unsigned width, unsigned write)
{
    assert(expected_count < 4);
    events[expected_count++] = (Access){p, width, write};
}

#ifdef OBSERVE_ACCESSES
static void check(void *p, unsigned width, unsigned write)
{
    assert(seen < expected_count);
    Access event = events[seen++];
    assert(event.address == p && event.width == width && event.write == write);
}
void __tsan_init(void) {}
void __tsan_func_entry(void *p) { (void)p; }
void __tsan_func_exit(void) {}
#define TRACE(W) \
void __tsan_read##W(void *p) { check(p,W,0); } \
void __tsan_write##W(void *p) { check(p,W,1); } \
void __tsan_volatile_read##W(void *p) { check(p,W,0); } \
void __tsan_volatile_write##W(void *p) { check(p,W,1); }
TRACE(1) TRACE(2) TRACE(4) TRACE(8) TRACE(16)
#endif

void func_8003E5BC(s32 selected, AudioElement *element)
{
    assert(calls++ == 0 && selected == (s32)instrument && element == service_element);
#ifdef OBSERVE_ACCESSES
    assert(seen == expected_count);
#endif
    element->unk_0x26 ^= 0xffu;
    element->unk_0x136 = 0xbabe;
}

enum Kind { SET_MANAGER, ADD_MANAGER, SERVICE, SET_BITS, CLEAR_BITS };
struct Command { Handler *run; enum Kind kind; unsigned companion, mask; };
static const struct Command commands[] = {
    {SoundScriptSetManagerUnk1a, SET_MANAGER, 0, 0},
    {SoundScriptAddManagerUnk1a, ADD_MANAGER, 0, 0},
    {SoundScriptCallE5BC, SERVICE, 0, 0},
    {SoundScriptSetUnkCEAndF6, SET_BITS, 0xf6, 1},
    {SoundScriptClearUnkCEAndF6, CLEAR_BITS, 0xf6, 1},
    {SoundScriptSetUnkCEAndUnk116, SET_BITS, 0x116, 2},
    {SoundScriptClearUnkCEAndUnk116, CLEAR_BITS, 0x116, 2},
    {SoundScriptSetUnkCEAndUnk136, SET_BITS, 0x136, 4},
};

static u16 sample(const void *address)
{
    u16 value;
    memcpy(&value,address,2);
    return value;
}

static void store(void *address, u16 value) { memcpy(address,&value,2); }

static void run(const struct Command *cmd, unsigned n, unsigned layout)
{
    struct { unsigned guard; AudioManager manager; unsigned tail; } m, wanted_m;
    struct { unsigned guard; AudioElement el; unsigned tail; } rec, wanted_rec;
    u8 buffer[3] = {0x5a,(u8)n,0xa5}, copy[3];
    memset(&m,(int)(n^layout),sizeof m);
    memset(&rec,(int)(n+layout),sizeof rec);
    AudioManager *manager=&m.manager;
    AudioElement *element=&rec.el;
    u8 *record=(u8 *)element, *script=buffer+1;
    int uses_script = cmd->kind == SET_MANAGER || cmd->kind == ADD_MANAGER;
    manager->unk_0x1a=(u8)(layout&1 ? 255-n : 37+n);
    element->unk_0x26=(u8)n;
    if (cmd->kind==SET_BITS || cmd->kind==CLEAR_BITS) {
        u16 a=(u16)(n*257u ^ (1u<<(layout%16u))), b=(u16)(~a ^ (0x1021u*layout));
        store(record+0xce,a);
        store(record+cmd->companion,b);
    }
    switch (layout) {
        case 0: break;
        case 1: script=(u8 *)manager+0x1a; break;
        case 2: script=record+0x26; break;
        case 3: script=record+0xce; break;
        case 4: script=(u8 *)manager+0x19; break;
        case 5: script=record+0x116; break;
        case 6: script=uses_script ? buffer : NULL; break;
        case 7: script=uses_script ? record+0xf6 : (u8 *)(uintptr_t)UINTPTR_MAX; break;
    }
    if (uses_script) *script=(u8)n;
    memcpy(&wanted_m,&m,sizeof m); memcpy(&wanted_rec,&rec,sizeof rec); memcpy(copy,buffer,sizeof copy);
    expected_count=seen=calls=0;
    u8 *expected_return=script;
    if (uses_script) {
        if (cmd->kind==ADD_MANAGER) expect(&manager->unk_0x1a,1,0);
        expect(script,1,0); expect(&manager->unk_0x1a,1,1);
        wanted_m.manager.unk_0x1a=(u8)(*script+(cmd->kind==ADD_MANAGER ? manager->unk_0x1a : 0));
        expected_return=script+1;
    } else if (cmd->kind==SERVICE) {
        expect(&element->unk_0x26,1,0);
        instrument=element->unk_0x26; service_element=element;
        wanted_rec.el.unk_0x26 ^= 0xffu; wanted_rec.el.unk_0x136=0xbabe;
    } else {
        u8 *bytes=(u8 *)&wanted_rec.el;
        u16 a=sample(bytes+0xce), b=sample(bytes+cmd->companion);
        expect(record+0xce,2,0); expect(record+cmd->companion,2,0);
        expect(record+0xce,2,1); expect(record+cmd->companion,2,1);
        store(bytes+0xce,cmd->kind==SET_BITS ? (u16)(a|cmd->mask) : (u16)(a&~cmd->mask));
        store(bytes+cmd->companion,cmd->kind==SET_BITS ? (u16)(b|1u) : (u16)(b&~1u));
    }
    u8 *actual=cmd->run(script,uses_script || layout&1u ? manager : NULL,
                         !uses_script || layout&1u ? element : NULL);
    assert(actual==expected_return && calls==(cmd->kind==SERVICE));
#ifdef OBSERVE_ACCESSES
    assert(seen==expected_count);
#endif
    assert(!memcmp(&m,&wanted_m,sizeof m) && !memcmp(&rec,&wanted_rec,sizeof rec));
    assert(!memcmp(copy,buffer,sizeof buffer));
}

int main(void)
{
    for (unsigned k=0;k<sizeof commands/sizeof *commands;k++)
        for (unsigned n=0;n<256;n++)
            for (unsigned layout=0;layout<8;layout++)
                run(&commands[k],n,layout);
    puts("16384 state command invocations matched");
    return 0;
}
#endif
