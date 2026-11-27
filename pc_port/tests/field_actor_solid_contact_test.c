/* Source-only field contact checks. The floor service captures the result;
 * mesh platforms and scripts are outside this ordinary-actor fixture. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef int16_t s16;
typedef int32_t s32;
typedef struct { s32 vx, vy, vz, pad; } VECTOR;
typedef struct { s16 vx, vy, vz, pad; } SVECTOR;

enum { PLAYER = 1, NPC = 37, COUNT = 38, Q16 = 65536 };
static _Alignas(8) u8 g_FieldActors[COUNT][0x5c], actors[COUNT][0x138];
static _Alignas(8) u8 sprite[0x90], animation[0x20];
static s32 D_800ADB98, D_800ADB94, D_800ADBFC = COUNT, g_FieldSystemMode = 1;
static u8 D_800B21CC;
static char D_8006FC48[1], D_8006FC58[1];
static int floor_calls, checks, failures;

static void func_8007CD3C(s32 size) { (void)size; }
static void func_8007CD60(s32 size) { (void)size; }
static void func_800379C8(char *fmt, ...) { (void)fmt; }
static void ControllerResetState(void) { abort(); }
static void *HeapAlloc(unsigned size, unsigned mode) { abort(); }
static s32 func_800825AC(s32 a, s32 b) { abort(); }
static s32 func_8008237C(s32 x, s32 z, void *other, s32 margin) { abort(); }
static s32 func_80083288(s32 a, void *mesh, s32 x, s32 z, s32 *y, void *contact) { abort(); }
static void func_800831D0(SVECTOR *out, s32 *pos) {
    *out = (SVECTOR){pos[0] >> 16, pos[1] >> 16, pos[2] >> 16, 0};
}
static void Square0(VECTOR *in, VECTOR *out) {
    out->vx = in->vx * in->vx;
    out->vy = in->vy * in->vy;
    out->vz = in->vz * in->vz;
}
static s32 func_80084A40(s32 who, s32 y, void *field, u8 *data, s32 state) {
    if (who != PLAYER || data != actors[PLAYER] || field != g_FieldActors[PLAYER] || state != 0) abort();
    floor_calls++;
    return 0;
}
#include "contact.inc"

static void set32(u8 *p, int at, s32 value) { memcpy(p + at, &value, sizeof(value)); }
static void set16(u8 *p, int at, u16 value) { memcpy(p + at, &value, sizeof(value)); }
static void setptr(u8 *p, int at, const void *target) {
    if ((uintptr_t)target > UINT32_MAX) abort();
    set32(p, at, (s32)(uintptr_t)target);
}
static void actor(u8 *data, int x, int y, int z, int height, int flags) {
    set32(data, 0, flags);
    set32(data, 0x20, x * Q16);
    set32(data, 0x24, y * Q16);
    set32(data, 0x28, z * Q16);
    set16(data, 0x1a, height);
    set16(data, 0x1e, 16);
}
enum { NORMAL, NPC_INACTIVE, NO_ACTOR_CONTACT, TERRAIN_SKIP, GLOBAL_SKIP, OTHER_LAYER };
struct Sample {
    const char *label;
    int x, z, dx, dz, npc_y, mode, blocked;
};

static int step(struct Sample s) {
    u8 *player = actors[PLAYER], *npc = actors[NPC];
    const s32 player_step[3] = {s.dx * Q16, 4660, s.dz * Q16};
    const s32 player_carry[3] = {300,301,302}, npc_step[3] = {8,9,10}, npc_carry[3] = {100,101,102};
    const s32 zero[3] = {0};
    memset(actors, 0, sizeof(actors));
    for (int i=0; i<COUNT; ++i) {
        setptr(g_FieldActors[i], 0x4c, actors[i]);
        set32(actors[i], 0, 1); /* inactive unused actor records */
    }
    setptr(g_FieldActors[PLAYER], 4, sprite);
    setptr(sprite, 0x7c, animation);
    actor(player, s.x, 87, s.z, 64, 0x24430);
    actor(npc, 428, s.npc_y, 10, 84, 0x130);
    set32(player, 4, 0x4010400);
    set32(npc, 4, 0x10000);
    player[0x74] = 0xff;
    memcpy(player+0x30, player_step, sizeof(player_step));
    memcpy(player+0x40, player_carry, sizeof(player_carry));
    memcpy(npc+0x30, npc_step, sizeof(npc_step));
    memcpy(npc+0x40, npc_carry, sizeof(npc_carry));
    if (s.mode == NPC_INACTIVE) set32(npc, 0, 0x131);
    if (s.mode == NO_ACTOR_CONTACT) set32(npc, 0, 0x1b0);
    if (s.mode == TERRAIN_SKIP) set32(player, 0x14, 0x400000);
    D_800B21CC = s.mode == GLOBAL_SKIP;
    if (s.mode == OTHER_LAYER) { set16(player, 0x10, 1); set16(npc, 0x10, 2); }
    floor_calls = 0;
    func_80084158(PLAYER, g_FieldActors[PLAYER], player);
    checks++;
    if (floor_calls != 1 || memcmp(player+0x30, s.blocked ? zero : player_step, 12) ||
        memcmp(player+0x40, s.blocked ? zero : player_carry, 12) ||
        memcmp(npc+0x30, s.blocked ? zero : npc_step, 12) ||
        memcmp(npc+0x40, s.blocked ? zero : npc_carry, 12)) {
        fprintf(stderr, "FAIL %s at (%d,%d), step (%d,%d), expected %s\n",
            s.label,s.x,s.z,s.dx,s.dz,s.blocked?"solid contact":"free movement");
        failures++;
    }
    s32 accepted_z;
    memcpy(&accepted_z, player+0x38, sizeof(accepted_z));
    return accepted_z / Q16;
}

int main(void) {
    const struct Sample cases[] = {
        {"first inward step",405,-13,0,1,87,NORMAL,1},
        {"outside on approach",405,-14,0,1,87,NORMAL,0},
        {"exact tangent",396,9,0,1,87,NORMAL,0},
        {"one unit inside",397,9,0,1,87,NORMAL,1},
        {"retreat still inside",398,10,-1,0,87,NORMAL,1},
        {"retreat reaches tangent",397,10,-1,0,87,NORMAL,0},
        {"lateral still inside",397,10,0,1,87,NORMAL,1},
        {"lateral exits",397,10,0,8,87,NORMAL,0},
        {"NPC above",405,-13,0,1,10,NORMAL,0},
        {"NPC below",405,-13,0,1,176,NORMAL,0},
        {"different layers at same height",405,-13,0,1,87,OTHER_LAYER,1},
        {"inactive NPC",405,-13,0,1,87,NPC_INACTIVE,0},
        {"non-solid actor flag",405,-13,0,1,87,NO_ACTOR_CONTACT,0},
        {"terrain bypass",405,-13,0,1,87,TERRAIN_SKIP,0},
        {"global bypass",405,-13,0,1,87,GLOBAL_SKIP,0}
    };
    for (size_t i=0; i<sizeof(cases)/sizeof(cases[0]); ++i) step(cases[i]);
    /* Whole-unit samples of the story-eight approach. Keep trying to walk
     * after contact; the parallel lane at X=300 continues through. */
    for (int lane=0; lane<2; ++lane) {
        int z=-73;
        for (int i=0; i<20; ++i)
            z += step((struct Sample){"repeated approach",lane?300:405,z,0,5,87,NORMAL,!lane && i>=12});
        if (z != (lane?27:-13)) { fprintf(stderr,"FAIL lane %d ends at Z=%d\n",lane,z); failures++; }
    }
    printf("field actor solid contact: %d cases, %d failures\n",checks,failures);
    return failures != 0;
}
