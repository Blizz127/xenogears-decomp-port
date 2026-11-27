#include "quick_checkpoint_cave_actors.h"
#include "retail_data.h"

#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "field/camera.h"
#include "field/text_box.h"

extern s32 g_GameSceneMapNum, g_PlayerActorIndex, D_800ADBFC;
extern u16 g_FieldScriptMemoryU16[] __asm__("g_FieldScriptMemory");
extern s16 D_800AFB54;
extern s32 D_800AFB44[];
extern u8 D_800B14F0[]; /* 32 rows of player movement history, 0x48 bytes each */
extern s32 D_800B2360, D_800B2364, D_800B2368, D_800C3910;
extern u8 D_800B21CC, D_800B21CE, D_800B21CF;
extern s16 D_800B234E;

enum { FOLLOWER = 6, ROCK = 7, HISTORY_COUNT = 32 };

/* Numeric host field ABI only. No allocation, function, script, or object
 * addresses cross the checkpoint boundary. The constructor owns the hidden
 * rock's disabled interactions and VM slots even when its final pose differs. */
typedef struct {
    s32 position[3], sprite_position[3], sprite_step[3];
    s32 move[3], move_modified[3], normal[3];
    s32 matrix_translation[2][3];
    u32 material;
    s16 matrix_basis[2][9];
    s16 rotation[3], field_rotation[3], previous[3];
    u16 triangles[4], layer, height;
    s16 ground, sprite_direction;
} CavePose;

/* These are the fields consumed by func_800815F0. Copying its latest row or
 * reseeding the trail from Fei would lose Bart's real lag after walking. */
typedef struct {
    s32 step[3], normal[3];
    s16 xyz[3], ground, animation;
    u16 facing, triangles[4], layer, moving;
} TrailSample;

typedef struct {
    CavePose poses[2];
    TrailSample trail[HISTORY_COUNT];
    s32 cursor[3], last_movement;
} CaveActors;

_Static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "native field byte order");
_Static_assert(sizeof(CavePose) == 172 && sizeof(TrailSample) == 48, "ACT1 pose layout");
_Static_assert(sizeof(CaveActors) <= PC_PORT_QUICK_CAVE_BYTES, "ACT1 capacity");

static ActorData *actor_data(unsigned slot)
{
    return (ActorData *)(uintptr_t)g_FieldActors[slot].pActorData;
}

static int cave_is_ready(void)
{
    if ((g_GameSceneMapNum & 0x3fff) != 64 || g_FieldScriptMemoryU16[0] != 48 ||
        g_FieldScriptMemoryU16[1] != 3 || (g_FieldScriptMemoryU16[0xc0 / 2] & 6) != 6 ||
        !g_FieldActors || D_800ADBFC <= ROCK || g_PlayerActorIndex != 5 ||
        D_800AFB54 != 3 || D_800AFB44[0] <= 0 || D_800AFB44[1] <= 0)
        return 0;
    return actor_data(5) && actor_data(5)->characterId == 0 &&
           actor_data(FOLLOWER) && actor_data(FOLLOWER)->characterId == 3 &&
           g_FieldActors[FOLLOWER].pSpriteData &&
           actor_data(ROCK) && actor_data(ROCK)->characterId == 255 &&
           g_FieldActors[ROCK].pSpriteData && (g_FieldActors[ROCK].status & ACTOR_STATUS_INVISIBLE);
}

static int contact_valid(u16 layer, const u16 triangles[4])
{
    if (D_800AFB54 == 2) /* The laboratory has a single walkable layer. */
        return layer == 0 && triangles[0] < D_800AFB44[0] &&
               !triangles[1] && !triangles[2] && !triangles[3];
    return layer < 2 && triangles[0] < D_800AFB44[0] && triangles[1] < D_800AFB44[1] &&
           triangles[2] == 0 && triangles[3] == 0;
}

/* The same numeric members are collected and applied. The two heights and
 * rotations have different owners: the hidden, rolled rock's model continues
 * to have a pitch and its sprite ground may still be zero when its Y is 300. */
static void transfer(void *saved, void *live, size_t size, int restore)
{
    if (restore)
        memcpy(live, saved, size);
    else
        memcpy(saved, live, size);
}

static void transfer_pose(CavePose *pose, unsigned slot, int restore)
{
    FieldActor *field = &g_FieldActors[slot];
    ActorData *actor = actor_data(slot);
    u8 *sprite = (u8 *)(uintptr_t)field->pSpriteData;
    transfer(pose->position, &actor->position.vx, sizeof pose->position, restore);
    transfer(pose->sprite_position, sprite, sizeof pose->sprite_position, restore);
    transfer(pose->sprite_step, sprite + 0xc, sizeof pose->sprite_step, restore);
    transfer(pose->move, &actor->move.vx, sizeof pose->move, restore);
    transfer(pose->move_modified, &actor->moveModified.vx, sizeof pose->move_modified, restore);
    transfer(pose->normal, &actor->curTriNormal.vx, sizeof pose->normal, restore);
    transfer(pose->triangles, actor->walkmeshTriIds, sizeof pose->triangles, restore);
    transfer(&pose->material, &actor->curWalkmeshTriMaterial, sizeof pose->material, restore);
    transfer(&pose->layer, &actor->walkmeshId, sizeof pose->layer, restore);
    transfer(pose->previous, &actor->prevPosition, sizeof pose->previous, restore);
    transfer(&pose->height, &actor->curYPos, sizeof pose->height, restore);
    transfer(&pose->ground, sprite + 0x84, sizeof pose->ground, restore);
    transfer(&pose->sprite_direction, sprite + 0x32, sizeof pose->sprite_direction, restore);
    transfer(pose->rotation, &actor->rotation.vx, sizeof pose->rotation, restore);
    transfer(pose->field_rotation, &field->rotation.x, sizeof pose->field_rotation, restore);
    MATRIX *matrices[] = { &field->transformMatrix, &field->childMatrix };
    for (unsigned i = 0; i < 2; ++i) {
        transfer(pose->matrix_basis[i], matrices[i]->m, sizeof pose->matrix_basis[i], restore);
        transfer(pose->matrix_translation[i], matrices[i]->t, sizeof pose->matrix_translation[i], restore);
    }
}

static void transfer_trail_row(TrailSample *sample, u8 *row, int restore)
{
    u32 flags;
    memcpy(&flags, row, sizeof flags);
    if (restore) {
        flags = (flags & ~0x800u) | sample->moving;
        memcpy(row, &flags, sizeof flags);
        row[0x44] = (u8)sample->layer;
    } else {
        sample->moving = flags & 0x800u;
        sample->layer = row[0x44];
    }
    transfer(sample->xyz, row + 8, sizeof sample->xyz, restore);
    transfer(&sample->ground, row + 0x10, sizeof sample->ground, restore);
    transfer(&sample->animation, row + 0x12, sizeof sample->animation, restore);
    transfer(&sample->facing, row + 0x14, sizeof sample->facing, restore);
    transfer(sample->triangles, row + 0x16, sizeof sample->triangles, restore);
    transfer(sample->step, row + 0x20, sizeof sample->step, restore);
    transfer(sample->normal, row + 0x30, sizeof sample->normal, restore);
}

static void transfer_trail(TrailSample rows[HISTORY_COUNT], s32 cursor[3], s32 *last, int restore)
{
    for (unsigned i = 0; i < HISTORY_COUNT; ++i)
        transfer_trail_row(&rows[i], D_800B14F0 + i * 0x48, restore);
    transfer(&cursor[0], &D_800B2360, sizeof D_800B2360, restore);
    transfer(&cursor[1], &D_800B2364, sizeof D_800B2364, restore);
    transfer(&cursor[2], &D_800B2368, sizeof D_800B2368, restore);
    transfer(last, &D_800C3910, sizeof D_800C3910, restore);
}

static void transfer_actors(CaveActors *saved, int restore)
{
    transfer_pose(&saved->poses[0], FOLLOWER, restore);
    transfer_pose(&saved->poses[1], ROCK, restore);
    transfer_trail(saved->trail, saved->cursor, &saved->last_movement, restore);
}

static int trail_valid(const TrailSample rows[HISTORY_COUNT], const s32 cursor[3],
                       s32 last, s16 max_animation)
{
    for (unsigned i = 0; i < 3; ++i)
        if (cursor[i] < 0 || cursor[i] >= HISTORY_COUNT)
            return 0;
    if (last != -1 && last != 0)
        return 0;
    for (unsigned i = 0; i < HISTORY_COUNT; ++i) {
        const TrailSample *row = &rows[i];
        if (!contact_valid(row->layer, row->triangles) || row->facing > 0xfff ||
            (row->moving != 0 && row->moving != 0x800) ||
            row->animation < 0 || row->animation > max_animation)
            return 0;
    }
    return 1;
}

static int captured_values_valid(const CaveActors *saved)
{
    for (unsigned i = 0; i < 2; ++i)
        if (!contact_valid(saved->poses[i].layer, saved->poses[i].triangles))
            return 0;
    /* ACT1 covers the ordinary idle and walking Gear tracks. */
    return trail_valid(saved->trail, saved->cursor, saved->last_movement, 1);
}

int PcPort_CaveActorsCapture(uint8_t out[PC_PORT_QUICK_CAVE_BYTES])
{
    CaveActors saved = {0};
    if (!cave_is_ready() || D_800B21CC || D_800B21CE || D_800B21CF || D_800B234E)
        return 0;
    transfer_actors(&saved, 0);
    if (!captured_values_valid(&saved))
        return 0;
    memset(out, 0, PC_PORT_QUICK_CAVE_BYTES);
    memcpy(out, &saved, sizeof saved);
    return 1;
}

int PcPort_CaveActorsRestore(const uint8_t in[PC_PORT_QUICK_CAVE_BYTES])
{
    CaveActors saved;
    if (!cave_is_ready())
        return 0;
    /* ACT1 reserves the tail for a future record version; the writer zeros it. */
    for (size_t i = sizeof saved; i < PC_PORT_QUICK_CAVE_BYTES; ++i)
        if (in[i] != 0)
            return 0;
    memcpy(&saved, in, sizeof saved);
    if (!captured_values_valid(&saved))
        return 0;
    transfer_actors(&saved, 1);
    return 1;
}

enum { ESTATE_FEI = 1, CITAN = 12, LENS_ACTOR = 53, ESTATE_SCRIPT_COUNT = 59 };
/* OP1F owns the low six bits. func_80075B44 maintains bit 9 from the
 * projected sprite bounds, including a free save farther down the path. */
enum { ESTATE_MOVEMENT_AND_CULL = 0x23f };
static const unsigned estate_owners[] = {0, CITAN, LENS_ACTOR};
enum { ESTATE_OWNER_COUNT = sizeof estate_owners / sizeof estate_owners[0] };

extern s32 D_800ADB68, D_800ADB64;
extern u8 D_800B21D0;
extern void func_80021BCC(void *sprite, u32 speed);
extern void func_800821F4(void *sprite, s16 animation, void *field_actor);

typedef struct {
    ActorScriptSlot slots[ACTOR_MAX_NUM_SCRIPTS];
    u16 ip;
    u8 selected, other;
} EstateVm;

typedef struct {
    u32 script_key;
    EstateVm vm[ESTATE_OWNER_COUNT];
    CavePose pose;
    u32 movement_flags, dialog_flags;
    u16 local_40a, speed;
    s16 animation, idle_counter;
} EstateActors;

typedef struct {
    u16 begin, update, talk, end;
} EstateRoutines;

_Static_assert(sizeof(ActorScriptSlot) == 8 && sizeof(EstateVm) == 68,
               "estate field VM wire layout");
_Static_assert(sizeof(EstateActors) <= PC_PORT_QUICK_ESTATE_ACTOR_BYTES,
               "estate camera follows the actors");

/* On the morning after the campfire, the field constructor places Elly and
 * seeds Fei's trail at the entrance. F12 later moves Fei to his saved spot.
 * The original follower can stand still until ten more movement records have
 * passed, and its visible sprite need not coincide with its simulation pose. */
enum { FOREST_FEI = 3, FOREST_ELLY = 4, BAL_FEI = 1, BAL_BART = 2, BAL_SENSOR_FLAGS = 0x180 };
extern s32 D_8005A444[], g_GamePartyMembers[];
extern void func_800223B0(void *sprite, s16 direction);

typedef struct {
    CavePose pose;
    TrailSample trail[HISTORY_COUNT];
    s32 cursor[3], last_movement;
    s16 sprite_facing;
    u16 reserved;
} PartyFollower;

_Static_assert(sizeof(PartyFollower) <= PC_PORT_QUICK_CAVE_BYTES, "FLW1 capacity");

static int follower_pair_bound(int leader, int partner, int character)
{
    if (!g_FieldActors || D_800ADBFC <= partner || g_FieldNumActors <= partner ||
        g_PlayerActorIndex != leader || D_8005A444[0] != leader ||
        D_8005A444[1] != partner || D_8005A444[2] != 255 ||
        g_GamePartyMembers[0] != 0 || g_GamePartyMembers[1] != character || g_GamePartyMembers[2] != 255 ||
        D_800B21CC || D_800B21CE || D_800B21CF || D_800B234E || D_800B21D0)
        return 0;
    const ActorData *fei = actor_data(leader), *follower = actor_data(partner);
    const FieldActor *field = &g_FieldActors[partner];
    return fei && fei->characterId == 0 && !(fei->scriptFlags.flags & 0x1800u) &&
           follower && follower->characterId == character && (follower->scriptFlags.flags & 0x01000000u) &&
           !(follower->scriptFlags.flags & 0x1000u) && !(field->status & ACTOR_STATUS_INVISIBLE) &&
           (field->status & 0xf80) == 0x200 && field->pSpriteData &&
           follower->defaultAnimationId == 0;
}

static int follower_pair_ready(int leader, int partner, int character)
{
    return D_800AFB54 == 3 && D_800AFB44[0] > 0 && D_800AFB44[1] > 0 &&
           follower_pair_bound(leader, partner, character) && actor_data(partner)->curAnimationId == 0;
}

static int follower_ready(int leader, int partner, int character)
{
    return follower_pair_ready(leader, partner, character) &&
           !(actor_data(partner)->scriptFlags.flags & 0x800u);
}

static int forest_follower_ready(void)
{
    return (g_GameSceneMapNum & 0x3fff) == 23 && g_FieldScriptMemoryU16[0] == 27 &&
           g_FieldScriptMemoryU16[1] == 1 && follower_ready(FOREST_FEI, FOREST_ELLY, 1);
}

static int bal_house(void)
{
    return (g_GameSceneMapNum & 0x3fff) == 71 && g_FieldScriptMemoryU16[0] == 51 &&
           g_FieldScriptMemoryU16[1] == 0 && D_800ADBFC == 6;
}

static int bal_sensor_house(void)
{
    return bal_house() && (g_FieldScriptMemoryU16[0xc0 / 2] & BAL_SENSOR_FLAGS) == BAL_SENSOR_FLAGS;
}

static int bal_follower_ready(void)
{
    return bal_sensor_house() && follower_ready(BAL_FEI, BAL_BART, 3);
}

/* Before the sensor briefing the field constructor owns Bal. Only the
 * following party member belongs in the standalone FLW1 record. */
static int standalone_follower(void)
{
    if (forest_follower_ready())
        return FOREST_ELLY;
    if (bal_house() && g_FieldScriptMemoryU16[0xc0 / 2] == 0x4087 &&
        follower_ready(BAL_FEI, BAL_BART, 3))
        return BAL_BART;
    return -1;
}

static int valid_follower(const PartyFollower *saved)
{
    return !saved->reserved && contact_valid(saved->pose.layer, saved->pose.triangles) &&
           trail_valid(saved->trail, saved->cursor, saved->last_movement, 3);
}

static void capture_follower(PartyFollower *saved, int slot)
{
    transfer_pose(&saved->pose, slot, 0);
    transfer_trail(saved->trail, saved->cursor, &saved->last_movement, 0);
    const u8 *sprite = (const u8 *)(uintptr_t)g_FieldActors[slot].pSpriteData;
    memcpy(&saved->sprite_facing, sprite + 0x80, sizeof saved->sprite_facing);
}

static void restore_follower(PartyFollower *saved, int slot)
{
    transfer_pose(&saved->pose, slot, 1);
    transfer_trail(saved->trail, saved->cursor, &saved->last_movement, 1);
    /* Rebind the direction through the destination sprite's own animation. */
    func_800223B0((void *)(uintptr_t)g_FieldActors[slot].pSpriteData, saved->sprite_facing);
}

int PcPort_FollowerCapture(uint8_t out[PC_PORT_QUICK_CAVE_BYTES])
{
    int slot = out ? standalone_follower() : -1;
    if (slot < 0)
        return 0;
    PartyFollower saved = {0};
    capture_follower(&saved, slot);
    if (!valid_follower(&saved))
        return 0;
    memset(out, 0, PC_PORT_QUICK_CAVE_BYTES);
    memcpy(out, &saved, sizeof saved);
    return 1;
}

int PcPort_FollowerRestore(const uint8_t in[PC_PORT_QUICK_CAVE_BYTES])
{
    int slot = in ? standalone_follower() : -1;
    if (slot < 0)
        return 0;
    PartyFollower saved;
    memcpy(&saved, in, sizeof saved);
    if (!valid_follower(&saved))
        return 0;
    for (size_t i = sizeof saved; i < PC_PORT_QUICK_CAVE_BYTES; ++i)
        if (in[i])
            return 0;
    restore_follower(&saved, slot);
    return 1;
}

/* The Disc 2 forest and workshop have one follower. The constructor owns the other
 * actor flags, VM slots and animation resources. The follower update changes
 * moving and use_trail independently on each step. */
typedef struct {
    u8 tag[4];
    u16 moving, use_trail;
    PartyFollower elly;
    PcPortForestCameraTiming timing;
    u8 animation, slope_contact; /* F616 only; F614 and W615 leave both zero. */
} EllyParty;
enum EllyRoom { ELLY_FOREST, ELLY_WORKSHOP, ELLY_LAB };
static const char elly_tags[][5] = { "F614", "W615", "F616" };
enum { ACTOR_SLOPE_CONTACT = 0x04000000 };

_Static_assert(offsetof(EllyParty, elly) == 8 &&
               offsetof(EllyParty, timing) == 8 + PC_PORT_QUICK_BART_BYTES &&
               sizeof(PcPortForestCameraTiming) == 2 &&
               sizeof(EllyParty) == 12 + PC_PORT_QUICK_BART_BYTES &&
               sizeof(EllyParty) <= PC_PORT_QUICK_PARTY_BYTES, "forest party layout");

enum { WORKSHOP_FEI = 1, WORKSHOP_ELLY = 2, WORKSHOP_SCRIPT_ACTORS = 17 };

static int workshop_party_ready(void)
{
    return (g_GameSceneMapNum & 0x3fff) == 614 && g_FieldScriptMemoryU16[0] == 249 &&
           g_FieldScriptMemoryU16[1] == 2 && g_FieldScriptMemoryU16[2] == 615 &&
           D_800ADBFC == WORKSHOP_SCRIPT_ACTORS && g_FieldNumActors >= WORKSHOP_SCRIPT_ACTORS &&
           follower_pair_ready(WORKSHOP_FEI, WORKSHOP_ELLY, 1) &&
           g_FieldActors[WORKSHOP_FEI].pSpriteData;
}

static int forest_camera_timing_valid(PcPortForestCameraTiming timing)
{
    return timing.step >= 8 && timing.step <= 20 && !(timing.step & 1u) && timing.phase < 4;
}

/* Actor 14 is the cinematic Elly. The visible party member is actor 2,
 * running her ordinary update. Actor 17 starts the later workshop story. */
static int taura_workshop_party_ready(void)
{
    if ((g_GameSceneMapNum & 0x3fff) != 615 || g_FieldScriptMemoryU16[0] != 249 ||
        g_FieldScriptMemoryU16[1] != 0 || g_FieldScriptMemoryU16[2] != 614 ||
        g_FieldScriptMemoryU16[0x1c0 / 2] != 0x400 || D_800ADBFC != 24 ||
        g_FieldNumActors < 24 || !follower_pair_ready(WORKSHOP_FEI, WORKSHOP_ELLY, 1))
        return 0;
    const ActorData *fei = actor_data(WORKSHOP_FEI), *elly = actor_data(WORKSHOP_ELLY);
    const ActorData *event = actor_data(17);
    if (!event || !(event->scriptFlags.flags & 1) || !(g_FieldActors[14].status & ACTOR_STATUS_INVISIBLE) ||
        fei->scriptInstructionPointer != 0x55 || fei->scripts[0].currentIP != 0x55 ||
        elly->scriptInstructionPointer != 0xbe || elly->scripts[0].currentIP != 0xbe)
        return 0;
    for (unsigned i = 0; i < 4; ++i)
        if (g_FieldTextBoxes[i].visibility != -1)
            return 0;
    return 1;
}

typedef struct {
    s16 rotation[3], sprite_direction, sprite_facing;
    u16 culled;
} LabResidentFacing;

enum { LAB_CITAN = 14, LAB_TAURA = 16, LAB_RESIDENTS = 2, LAB_CULLED = 0x200 };
/* Earlier F616 records left this byte zero. Their ground contact is unknown. */
enum { LAB_CONTACT_UNSAVED, LAB_CONTACT_CLEAR, LAB_CONTACT_SET };
enum { LAB_FEI_CONTACT_OFFSET = sizeof(EllyParty) + LAB_RESIDENTS * sizeof(LabResidentFacing) };
static const u8 lab_residents[LAB_RESIDENTS] = { LAB_CITAN, LAB_TAURA };
_Static_assert(sizeof(LabResidentFacing) == 12 && LAB_FEI_CONTACT_OFFSET == 1764 &&
               LAB_FEI_CONTACT_OFFSET < PC_PORT_QUICK_PARTY_BYTES, "F616 party layout");

static int lab_raw_heading_valid(u16 angle)
{
    /* Script turns retain the high direction flag and can sign-extend a
     * negative 12-bit angle; do not strip those bits from the loaded pose. */
    u16 high = angle & 0xf000u;
    return high == 0 || high == 0x8000 || high == 0xf000;
}

static int lab_facings_valid(const LabResidentFacing facings[LAB_RESIDENTS])
{
    for (unsigned i = 0; i < LAB_RESIDENTS; ++i) {
        const LabResidentFacing *f = &facings[i];
        if (!lab_raw_heading_valid((u16)f->rotation[0]) || !lab_raw_heading_valid((u16)f->rotation[1]) ||
            (u16)f->rotation[2] > 0xfff || f->sprite_direction < -0x1000 || f->sprite_direction > 0xfff ||
            f->sprite_facing < -0x1000 || f->sprite_facing > 0x1ffe || (f->culled & ~LAB_CULLED))
            return 0;
    }
    return 1;
}

static void transfer_lab_facings(LabResidentFacing facings[LAB_RESIDENTS], int restore)
{
    for (unsigned i = 0; i < LAB_RESIDENTS; ++i) {
        const unsigned slot = lab_residents[i];
        ActorData *npc = actor_data(slot);
        u8 *sprite = (u8 *)(uintptr_t)g_FieldActors[slot].pSpriteData;
        LabResidentFacing *f = &facings[i];
        transfer(f->rotation, &npc->rotation.vx, sizeof f->rotation, restore);
        transfer(&f->sprite_direction, sprite + 0x32, sizeof f->sprite_direction, restore);
        if (restore) {
            npc->flags = (npc->flags & ~LAB_CULLED) | f->culled;
            func_800223B0(sprite, f->sprite_facing);
        } else {
            memcpy(&f->sprite_facing, sprite + 0x80, sizeof f->sprite_facing);
            f->culled = npc->flags & LAB_CULLED;
        }
    }
}

/* The field loader separately authenticates the Disc 2 script and actor 19's
 * completed meeting. This is Elly's ordinary postmeeting party binding. */
static int taura_lab_party_ready(void)
{
    if ((g_GameSceneMapNum & 0x3fff) != 616 || g_FieldScriptMemoryU16[0] != 249 ||
        g_FieldScriptMemoryU16[1] != 0 || g_FieldScriptMemoryU16[2] != 615 ||
        (g_FieldScriptMemoryU16[0x1c0 / 2] & 0x1c00) != 0x1400 || D_800ADBFC != 34 ||
        g_FieldNumActors < 34 || D_800AFB54 != 2 || D_800AFB44[0] <= 0 ||
        !follower_pair_bound(WORKSHOP_FEI, WORKSHOP_ELLY, 1) || !g_FieldActors[WORKSHOP_FEI].pSpriteData)
        return 0;
    for (unsigned i = 0; i < LAB_RESIDENTS; ++i) {
        const unsigned slot = lab_residents[i];
        const FieldActor *field = &g_FieldActors[slot];
        const ActorData *npc = actor_data(slot);
        if (!npc || !field->pSpriteData || (field->status & (0xf80u | ACTOR_STATUS_INVISIBLE)) != 0x200 ||
            npc->characterId != 255 || npc->scriptFlags.flags & 1)
            return 0;
    }
    const ActorData *elly = actor_data(WORKSHOP_ELLY);
    return elly->curAnimationId >= 0 && elly->curAnimationId <= 3;
}

static int capture_elly_party(uint8_t *out, enum EllyRoom room, PcPortForestCameraTiming timing)
{
    EllyParty saved;
    memset(&saved, 0, sizeof saved);
    memcpy(saved.tag, elly_tags[room], sizeof saved.tag);
    saved.timing = timing;
    const ActorData *elly = actor_data(WORKSHOP_ELLY);
    saved.moving = elly->scriptFlags.flags & 0x800u;
    saved.use_trail = elly->flags & 0x1000u;
    if (room == ELLY_LAB) {
        saved.animation = (u8)elly->curAnimationId;
        saved.slope_contact = !!(elly->scriptFlags.flags & ACTOR_SLOPE_CONTACT);
    }
    capture_follower(&saved.elly, WORKSHOP_ELLY);
    if (!valid_follower(&saved.elly))
        return 0;
    LabResidentFacing facings[LAB_RESIDENTS];
    if (room == ELLY_LAB) {
        transfer_lab_facings(facings, 0);
        if (!lab_facings_valid(facings))
            return 0;
    }
    memset(out, 0, PC_PORT_QUICK_PARTY_BYTES);
    memcpy(out, &saved, sizeof saved);
    if (room == ELLY_LAB) {
        memcpy(out + sizeof saved, facings, sizeof facings);
        const ActorData *fei = actor_data(WORKSHOP_FEI);
        out[LAB_FEI_CONTACT_OFFSET] = (fei->scriptFlags.flags & ACTOR_SLOPE_CONTACT)
                                    ? LAB_CONTACT_SET : LAB_CONTACT_CLEAR;
    }
    return 1;
}

static int restore_elly_party(const uint8_t *in, enum EllyRoom room, PcPortForestCameraTiming *timing)
{
    EllyParty saved;
    memcpy(&saved, in, sizeof saved);
    if (memcmp(saved.tag, elly_tags[room], sizeof saved.tag) ||
        (room == ELLY_LAB ? saved.animation > 3 || saved.slope_contact > 1 :
                            saved.animation || saved.slope_contact) ||
        (timing ? !forest_camera_timing_valid(saved.timing) : saved.timing.step || saved.timing.phase) ||
        (saved.moving & ~0x800u) || (saved.use_trail & ~0x1000u) ||
        !valid_follower(&saved.elly))
        return 0;
    /* CavePose ends in two bytes of alignment padding. The writer leaves
     * these and the rest of the fixed-size payload zero. */
    enum { pose_end = __builtin_offsetof(EllyParty, elly.pose) + sizeof(CavePose) };
    if (in[pose_end - 2] || in[pose_end - 1])
        return 0;
    size_t used = sizeof saved;
    LabResidentFacing facings[LAB_RESIDENTS];
    u8 fei_contact = LAB_CONTACT_UNSAVED;
    if (room == ELLY_LAB) {
        memcpy(facings, in + used, sizeof facings);
        fei_contact = in[LAB_FEI_CONTACT_OFFSET];
        if (!lab_facings_valid(facings) || fei_contact > LAB_CONTACT_SET)
            return 0;
        used = LAB_FEI_CONTACT_OFFSET + 1;
    }
    for (size_t i = used; i < PC_PORT_QUICK_PARTY_BYTES; ++i)
        if (in[i])
            return 0;
    ActorData *elly = actor_data(WORKSHOP_ELLY);
    if (room == ELLY_LAB) {
        if (fei_contact != LAB_CONTACT_UNSAVED) {
            ActorData *fei = actor_data(WORKSHOP_FEI);
            fei->scriptFlags.flags = (fei->scriptFlags.flags & ~ACTOR_SLOPE_CONTACT) |
                                     (fei_contact == LAB_CONTACT_SET ? ACTOR_SLOPE_CONTACT : 0);
        }
        transfer_lab_facings(facings, 1);
        /* Rebinding can clear the moving flag. Restore the saved movement and
         * sprite-facing values after the live animation owns its resources. */
        if (elly->curAnimationId != saved.animation) {
            elly->curAnimationId = saved.animation;
            func_800821F4((void *)(uintptr_t)g_FieldActors[WORKSHOP_ELLY].pSpriteData,
                          saved.animation, &g_FieldActors[WORKSHOP_ELLY]);
        }
        elly->scriptFlags.flags = (elly->scriptFlags.flags & ~ACTOR_SLOPE_CONTACT) |
                                  (saved.slope_contact ? ACTOR_SLOPE_CONTACT : 0);
    }
    restore_follower(&saved.elly, WORKSHOP_ELLY);
    elly->scriptFlags.flags = (elly->scriptFlags.flags & ~0x800u) | saved.moving;
    elly->flags = (elly->flags & ~0x1000u) | saved.use_trail;
    if (timing)
        *timing = saved.timing;
    return 1;
}

int PcPort_ForestPartyCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES], PcPortForestCameraTiming timing)
{
    return out && forest_camera_timing_valid(timing) && workshop_party_ready() &&
           capture_elly_party(out, ELLY_FOREST, timing);
}

int PcPort_ForestPartyRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES], PcPortForestCameraTiming *timing)
{
    return in && timing && workshop_party_ready() && restore_elly_party(in, ELLY_FOREST, timing);
}

int PcPort_TauraWorkshopPartyCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES])
{
    return out && taura_workshop_party_ready() &&
           capture_elly_party(out, ELLY_WORKSHOP, (PcPortForestCameraTiming){0});
}

int PcPort_TauraWorkshopPartyRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES])
{
    return in && taura_workshop_party_ready() && restore_elly_party(in, ELLY_WORKSHOP, NULL);
}

int PcPort_TauraPartyCapture(uint8_t out[PC_PORT_QUICK_PARTY_BYTES])
{
    return out && taura_lab_party_ready() &&
           capture_elly_party(out, ELLY_LAB, (PcPortForestCameraTiming){0});
}

int PcPort_TauraPartyRestore(const uint8_t in[PC_PORT_QUICK_PARTY_BYTES])
{
    return in && taura_lab_party_ready() && restore_elly_party(in, ELLY_LAB, NULL);
}

enum { DAZIL_FEI = 1, DAZIL_CITAN = 3, DAZIL_SCRIPT_COUNT = 56,
       DAZIL_REPOSITIONED = 0x00400000 };

typedef struct {
    PartyFollower follower;
    u32 repositioned, reserved_motion;
    s32 sprite_speed, gravity;
    u16 move_speed, divisor;
    u8 reserved[52];
} DazilParty;

_Static_assert(offsetof(DazilParty, repositioned) == 1728 &&
               offsetof(DazilParty, reserved) == 1748 &&
               sizeof(DazilParty) == PC_PORT_DAZIL_PARTY_BYTES, "DAZ1 party layout");

/* Identify the script locations this save continuation supports from the
 * directory and opcodes in the player's loaded field script. */
static int script_bytes_match(const u8 *bytes, size_t length, const char *expected)
{
    XenoSha256 hash;
    char digest[65];

    XenoSha256_Init(&hash);
    XenoSha256_Update(&hash, bytes, length);
    XenoSha256_HexFinal(&hash, digest);
    return strcmp(digest, expected) == 0;
}

static int dazil_follower_ready(void)
{
    const u8 *file = (const u8 *)g_FieldCurScriptFile;
    const u8 *code = g_FieldScriptVMCurScriptData;
    if ((g_GameSceneMapNum & 0x3fff) != 35 || g_FieldScriptMemoryU16[0] != 38 ||
        g_FieldScriptMemoryU16[1] != 14 || !(g_FieldScriptMemoryU16[0xe4 / 2] & 1) ||
        D_800ADBFC != DAZIL_SCRIPT_COUNT || !follower_ready(DAZIL_FEI, DAZIL_CITAN, 2) ||
        !file || code != file + 0x84 + DAZIL_SCRIPT_COUNT * SCRIPT_OFFSET_TABLE_SIZE)
        return 0;
    const u8 *row = file + 0x84 + DAZIL_CITAN * SCRIPT_OFFSET_TABLE_SIZE;
    /* The locked dialogue branch stops at 0337. The earned handback and
     * the reconstructed free field both hold UPDATE_CHARACTER at 0338. */
    return actor_data(DAZIL_CITAN)->scriptInstructionPointer == 0x338 &&
           script_bytes_match(row, 4 * sizeof(u16),
                              "b633f54629681a4bf56237b147f29f7cbc5add881187f5ab0bfd8038619d95c3") &&
           script_bytes_match(code + 0x338, 2,
                              "bf60e4349cace6bce0d552e8d783428db66d0d649bd9e430a3627e2ee14ac839");
}

static u32 dazil_word(const void *p)
{
    u32 word;
    memcpy(&word, p, sizeof word);
    return word;
}

static void dazil_capture_motion(DazilParty *saved)
{
    const ActorData *actor = actor_data(DAZIL_CITAN);
    const u8 *sprite = (const u8 *)(uintptr_t)g_FieldActors[DAZIL_CITAN].pSpriteData;
    saved->repositioned = actor->scriptFlags.flags & DAZIL_REPOSITIONED;
    memcpy(&saved->sprite_speed, sprite + 0x18, sizeof saved->sprite_speed);
    memcpy(&saved->gravity, sprite + 0x1c, sizeof saved->gravity);
    saved->move_speed = actor->moveSpeed;
    saved->divisor = (dazil_word(sprite + 0xac) >> 7) & 0xfff;
}

/* The ordinary Dazil reunion sets Citan's movement divisor to 192. The
 * field rebuilt with E4 already set skips that script and retains 256. */
static int dazil_motion_valid(const DazilParty *saved, int allow_constructor)
{
    if (saved->sprite_speed != 0 || saved->move_speed != saved->divisor)
        return 0;
    if (saved->repositioned == DAZIL_REPOSITIONED)
        return saved->move_speed == 192 && saved->gravity == 232448;
    return allow_constructor && !saved->repositioned &&
           saved->move_speed == 256 && saved->gravity == 131072;
}

int PcPort_DazilPartyCapture(uint8_t out[PC_PORT_DAZIL_PARTY_BYTES])
{
    DazilParty saved = {0};
    if (!out || !dazil_follower_ready())
        return 0;
    dazil_capture_motion(&saved);
    if (!dazil_motion_valid(&saved, 0))
        return 0;
    capture_follower(&saved.follower, DAZIL_CITAN);
    if (!valid_follower(&saved.follower))
        return 0;
    memcpy(out, &saved, sizeof saved);
    return 1;
}

int PcPort_DazilPartyRestore(const uint8_t in[PC_PORT_DAZIL_PARTY_BYTES], int apply)
{
    DazilParty saved, current;
    if (!in || !dazil_follower_ready())
        return 0;
    memcpy(&saved, in, sizeof saved);
    dazil_capture_motion(&current);
    if (saved.reserved_motion || !dazil_motion_valid(&current, 1) || !dazil_motion_valid(&saved, 0) ||
        !valid_follower(&saved.follower))
        return 0;
    for (size_t i = 0; i < sizeof saved.reserved; i++)
        if (saved.reserved[i])
            return 0;
    if (apply) {
        ActorData *actor = actor_data(DAZIL_CITAN);
        u8 *sprite = (u8 *)(uintptr_t)g_FieldActors[DAZIL_CITAN].pSpriteData;
        /* The dialogue changed the portrait-cache slot at actor +12c.
         * Its index belongs to the newly constructed field cache. */
        actor->scriptFlags.flags = (actor->scriptFlags.flags & ~(u32)DAZIL_REPOSITIONED) |
                                   saved.repositioned;
        actor->moveSpeed = saved.move_speed;
        func_80021BCC(sprite, saved.divisor);
        restore_follower(&saved.follower, DAZIL_CITAN);
        memcpy(sprite + 0x18, &saved.sprite_speed, sizeof saved.sprite_speed);
        memcpy(sprite + 0x1c, &saved.gravity, sizeof saved.gravity);
    }
    return 1;
}

extern void FieldActorCopyPlacement(s32 destination, s32 source);
extern void func_8009B338(void);

static void copy_follower_placement(int leader, int partner)
{
    /* Older saves have only the leader's placement. Use the field's own
     * placement and trail initialization; the original lag is unknown. */
    FieldActorCopyPlacement(partner, leader);
    FieldActor *field = &g_FieldActors[partner];
    memcpy(field->childMatrix.t, field->transformMatrix.t, sizeof field->childMatrix.t);
    ActorData *follower = actor_data(partner);
    follower->prevPosition = (SVEC){follower->position.vx >> 16, follower->position.vy >> 16,
                                    follower->position.vz >> 16};
}

static int group_follower(int leader, int partner)
{
    ActorData *fei = actor_data(leader);
    if (!g_FieldActors[leader].pSpriteData || !contact_valid(fei->walkmeshId, fei->walkmeshTriIds))
        return 0;
    copy_follower_placement(leader, partner);
    func_8009B338();
    return 1;
}

int PcPort_TauraPartyRegroup(void)
{
    if (!taura_lab_party_ready() || !group_follower(WORKSHOP_FEI, WORKSHOP_ELLY))
        return 0;
    /* Legacy FCV1 has no follower offset or trail. At the meeting handback
     * Elly is standing on Fei facing him, independently of his own yaw.
     * For later saved positions retain the field's normal regroup. */
    const ActorData *fei = actor_data(WORKSHOP_FEI);
    if (fei->position.vx == -393 * 65536 && fei->position.vy == 30 * 65536 &&
        fei->position.vz == -177 * 65536 && (u16)fei->rotation.vy == 0xfffe) {
        ActorData *elly = actor_data(WORKSHOP_ELLY);
        elly->rotation.vx = elly->rotation.vy = (s16)0x8400;
        elly->rotation.vz = 0x400;
        elly->scriptFlags.flags |= ACTOR_SLOPE_CONTACT;
        u8 *sprite = (u8 *)(uintptr_t)g_FieldActors[WORKSHOP_ELLY].pSpriteData;
        s16 direction = (u16)fei->rotation.vy & 0xfff;
        memcpy(sprite + 0x32, &direction, sizeof direction);
        func_800223B0(sprite, (s16)(g_CamInterpolation.curAngleY + elly->rotation.vz));
    }
    return 1;
}

int PcPort_ForestRegroupAtPlayer(void)
{
    if ((g_GameSceneMapNum & 0x3fff) != 614 || !g_FieldActors || D_800ADBFC != 17 ||
        g_PlayerActorIndex != 1 || g_GamePartyMembers[0] != 0 ||
        g_GamePartyMembers[1] != 1 || g_GamePartyMembers[2] != 255 ||
        D_8005A444[0] != 1 || D_8005A444[1] != 2 || D_8005A444[2] != 255 ||
        !g_FieldActors[1].pActorData || !g_FieldActors[2].pActorData ||
        !g_FieldActors[2].pSpriteData || (g_FieldActors[2].status & ACTOR_STATUS_INVISIBLE))
        return 0;
    return group_follower(1, 2);
}

int PcPort_FollowerGroupAtPlayer(void)
{
    int slot = standalone_follower();
    if (slot >= 0)
        return group_follower(g_PlayerActorIndex, slot);
    return bal_follower_ready() && group_follower(BAL_FEI, BAL_BART);
}


#include "quick_checkpoint_street_followers.inc"

static int estate_is_ready(void)
{
    return (g_GameSceneMapNum & 0x3fff) == 17 && g_FieldScriptMemoryU16[0] == 15 &&
           g_FieldScriptMemoryU16[1] == 2 && g_FieldScriptMemoryU16[2] == 19 &&
           g_FieldActors && g_FieldNumActors > LENS_ACTOR && D_800ADBFC == ESTATE_SCRIPT_COUNT && g_PlayerActorIndex == ESTATE_FEI &&
           D_800AFB54 == 4 && D_800AFB44[0] > 0 && D_800AFB44[1] > 0 && D_800AFB44[2] > 0 &&
           D_800ADB68 == 1 && D_800ADB64 == 0xff && !D_800B21D0 &&
           actor_data(ESTATE_FEI) && actor_data(ESTATE_FEI)->characterId == 0 &&
           !(actor_data(ESTATE_FEI)->scriptFlags.flags & 0x1800) &&
           actor_data(0) && actor_data(LENS_ACTOR) && actor_data(CITAN) &&
           g_FieldActors[CITAN].status == 0x240 && g_FieldActors[CITAN].pSpriteData &&
           actor_data(CITAN)->characterId == 0xff && actor_data(CITAN)->faceId == 2 &&
           actor_data(CITAN)->unkAnimationId == 0xff &&
           (actor_data(CITAN)->flags & ~(u32)ESTATE_MOVEMENT_AND_CULL) == 0x10000 &&
           !(actor_data(CITAN)->scriptFlags.flags & 1);
}

/* FieldLoad owns these offsets in the resident file, independent of a
 * checkpoint's contents. Bind all three script tables and bodies to this
 * record before any saved IP can reach the live scheduler. */
static u32 estate_script_key(EstateRoutines bounds[ESTATE_OWNER_COUNT])
{
    const u8 *file = (const u8 *)g_FieldCurScriptFile;
    const u8 *code = g_FieldScriptVMCurScriptData;
    if (!file)
        return 0;
    u32 count;
    memcpy(&count, file + 0x80, sizeof count);
    if (count != ESTATE_SCRIPT_COUNT ||
        code != file + 0x84 + ESTATE_SCRIPT_COUNT * SCRIPT_OFFSET_TABLE_SIZE)
        return 0;
    u32 key = 2166136261u;
    for (unsigned i = 0; i < ESTATE_OWNER_COUNT; ++i) {
        const u8 *row = file + 0x84 + estate_owners[i] * SCRIPT_OFFSET_TABLE_SIZE;
        EstateRoutines *r = &bounds[i];
        memcpy(&r->begin, row, 2);
        memcpy(&r->update, row + 2, 2);
        memcpy(&r->talk, row + 4, 2);
        memcpy(&r->end, row + SCRIPT_OFFSET_TABLE_SIZE, 2);
        if (!(r->begin < r->update && r->update < r->talk && r->talk <= r->end))
            return 0;
        for (unsigned j = 0; j < SCRIPT_OFFSET_TABLE_SIZE + 2; ++j)
            key = (key ^ row[j]) * 16777619u;
        for (unsigned j = r->begin; j < r->end; ++j)
            key = (key ^ code[j]) * 16777619u;
    }
    return key;
}

static void transfer_estate_vm(EstateVm *saved, unsigned index, int restore)
{
    ActorData *actor = actor_data(index);
    transfer(saved->slots, actor->scripts, sizeof saved->slots, restore);
    transfer(&saved->ip, &actor->scriptInstructionPointer, sizeof saved->ip, restore);
    transfer(&saved->selected, &actor->curScriptIndex, 1, restore);
    transfer(&saved->other, &actor->unkCF, 1, restore);
}

static int estate_vm_valid(const EstateVm *vm, unsigned index, const EstateRoutines *range)
{
    const u8 *code = g_FieldScriptVMCurScriptData;
    if (vm->selected != 0 || vm->other >= ACTOR_MAX_NUM_SCRIPTS ||
        vm->ip != vm->slots[0].currentIP || vm->ip < range->update || vm->ip >= range->talk)
        return 0;
    for (unsigned i = 0; i < ACTOR_MAX_NUM_SCRIPTS; ++i) {
        const ActorScriptSlot *s = &vm->slots[i];
        if (s->scriptId != 0xff || s->state != SCRIPT_STATE_IDLE || s->flags_0 != 0xffff ||
            s->flags_0x17 || s->flags_0x19)
            return 0;
        if (i && (s->waitTimer || s->flags_0x12 != 15 || s->isInUse ||
                  (s->currentIP != 0xffff &&
                   (s->currentIP < range->begin || s->currentIP >= range->end ||
                    code[s->currentIP] != 0))))
            return 0;
    }
    const ActorScriptSlot *s = &vm->slots[0];
    u8 op = code[vm->ip];
    if (index == LENS_ACTOR) {
        /* Sleep yields at the instruction while counting down, then at the
         * following JMP/STOP for one turn. Keep the actual remaining timer. */
        if (op == 0x26 && vm->ip + 3 <= range->talk && code[vm->ip + 2] == 0x80)
            return s->flags_0x12 == 7 && s->waitTimer <= code[vm->ip + 1];
        if (op == 1 && vm->ip + 3 <= range->talk && s->flags_0x12 == 7 && !s->waitTimer) {
            u16 jump;
            memcpy(&jump, code + vm->ip + 1, sizeof jump);
            return jump >= range->update && jump < range->talk && code[jump] == 0;
        }
    }
    return !s->waitTimer && ((op == 0x5b && s->flags_0x12 == 7) ||
           (op == 0 && (s->flags_0x12 == 15 || (index == LENS_ACTOR && s->flags_0x12 == 7))));
}

static int estate_values_valid(const EstateActors *saved, const EstateRoutines ranges[ESTATE_OWNER_COUNT])
{
    for (unsigned i = 0; i < ESTATE_OWNER_COUNT; ++i)
        if (!estate_vm_valid(&saved->vm[i], estate_owners[i], &ranges[i]))
            return 0;
    const CavePose *p = &saved->pose;
    if (p->layer >= 3 || p->triangles[3] || saved->local_40a > 1 ||
        (saved->movement_flags & ~(u32)ESTATE_MOVEMENT_AND_CULL) || saved->speed == 0 || saved->speed > 0xfff ||
        (saved->animation != 0 && saved->animation != 4) ||
        (saved->dialog_flags & ~0x00040000u))
        return 0;
    for (unsigned i = 0; i < 3; ++i)
        if (p->triangles[i] >= D_800AFB44[i] || p->move[i] || p->move_modified[i] ||
            p->sprite_step[i] || p->matrix_translation[0][i] != (p->position[i] >> 16) ||
            p->matrix_translation[1][i] != (p->position[i] >> 16) ||
            p->sprite_position[i] != p->position[i])
            return 0;
    return 1;
}

static int capture_estate(EstateActors *saved, EstateRoutines ranges[ESTATE_OWNER_COUNT])
{
    if (!estate_is_ready() || !(saved->script_key = estate_script_key(ranges)))
        return 0;
    ActorData *citan = actor_data(CITAN);
    if (citan->defaultAnimationId != citan->curAnimationId)
        return 0;
    for (unsigned i = 0; i < ESTATE_OWNER_COUNT; ++i)
        transfer_estate_vm(&saved->vm[i], estate_owners[i], 0);
    transfer_pose(&saved->pose, CITAN, 0);
    saved->local_40a = g_FieldScriptMemoryU16[0x40a / 2];
    saved->movement_flags = citan->flags & ESTATE_MOVEMENT_AND_CULL;
    saved->dialog_flags = citan->dialogFlags;
    saved->speed = citan->moveSpeed;
    saved->animation = citan->curAnimationId;
    saved->idle_counter = citan->unk102;
    return estate_values_valid(saved, ranges);
}

/* Optional CSP1 extension in EST1's formerly zero-filled actor area. The only
 * cursor on disk is a little-endian offset from the saved clip descriptor. */
typedef struct {
    u32 tag, package_key;
    u16 pc, frame, wait;
    s16 facing;
    u8 steps, state;
    u16 reserved;
} EstateSpritePhase;

enum { ESTATE_PHASE_V1 = 0x31505343 }; /* "CSP1" */
_Static_assert(sizeof(EstateSpritePhase) == 20 &&
               sizeof(EstateActors) + sizeof(EstateSpritePhase) <= PC_PORT_QUICK_ESTATE_ACTOR_BYTES,
               "optional estate sprite phase layout");

typedef struct {
    const u8 *descriptor, *commands, *terminal, *frames, *palette;
    u32 key;
} EstateClip;

static u16 estate_half(const u8 *p)
{
    u16 word;
    memcpy(&word, p, sizeof word);
    return word;
}

static u32 estate_word(const u8 *p)
{
    u32 word;
    memcpy(&word, p, sizeof word);
    return word;
}

static int estate_pointer_is(const u8 *slot, const void *owner)
{
    return estate_word(slot) == (uintptr_t)owner;
}

/* FieldLoad's skin directory bounds the whole package. Clip descriptors,
 * direction lists, and command streams are separate arrays inside its first
 * section. In particular the next descriptor is not a command-stream end. */
static int estate_clip(s16 animation, EstateClip *clip)
{
    const u8 *directory = g_FieldSpriteData;
    const u8 *sprite = (const u8 *)(uintptr_t)g_FieldActors[CITAN].pSpriteData;
    unsigned skin = actor_data(CITAN)->unk126;
    if (!directory || !(skin & 0x80) || (estate_word(sprite + 0x3c) & 3) != 1 ||
        !estate_pointer_is(sprite + 0x20, sprite + 0xb4) ||
        !estate_pointer_is(sprite + 0x24, sprite + 0x110))
        return 0;
    skin &= 0x7f;
    u32 count = estate_word(directory);
    if (count > 128 || skin + 1 >= count)
        return 0;
    u32 begin = estate_word(directory + 4 + skin * 4);
    u32 end = estate_word(directory + 8 + skin * 4);
    if (begin < (count + 1) * 4 || end <= begin || end - begin < 20)
        return 0;
    const u8 *file = directory + begin;
    if (!estate_pointer_is(sprite + 0x44, file) || !estate_pointer_is(sprite + 0x48, file) ||
        estate_word(file) != 3)
        return 0;
    u32 animations = estate_word(file + 4), frames = estate_word(file + 8);
    u32 palette = estate_word(file + 12), size = estate_word(file + 16);
    if (animations < 20 || frames <= animations || frames - animations < 4 ||
        palette <= frames || palette - frames < 4 || size < palette || size > end - begin ||
        !estate_pointer_is(sprite + 0x110, file + frames) ||
        !estate_pointer_is(sprite + 0x120, file + animations))
        return 0;
    const u8 *table = file + animations;
    u32 span = frames - animations;
    count = estate_half(table) & 0x3f;
    if ((unsigned)animation >= count || 2 + count * 2 > span)
        return 0;
    u32 offset = estate_half(table + 2 + (unsigned)animation * 2);
    if (offset < 2 + count * 2 || offset + 14 > span)
        return 0;
    clip->descriptor = table + offset;
    if ((estate_half(clip->descriptor) & 3) != 2)
        return 0;
    u32 start = offset + 2 + estate_half(clip->descriptor + 2);
    u32 limit = span;
    for (unsigned i = 0; i < count; ++i) {
        /* Siblings can share code or place it beyond their descriptor. */
        u32 sibling = estate_half(table + 2 + i * 2);
        if (sibling < 2 + count * 2 || sibling + 14 > span)
            return 0;
        u32 command = sibling + 2 + estate_half(table + sibling + 2);
        if (command < sibling + 14 || command >= span)
            return 0;
        if (command > start && command < limit)
            limit = command;
    }
    clip->commands = table + start;
    /* These Citan clips select one directional frame, step locally, then
     * park at END. No VM jumps, callbacks, or command replay are necessary. */
    if ((table[start] >> 4) != 1)
        return 0;
    u32 cursor = start + 1;
    while (cursor < limit && cursor - start <= 63 &&
           (table[cursor] < 0x10 || (table[cursor] >> 4) == 2))
        ++cursor;
    if (cursor >= limit || table[cursor] != 0x81 || cursor - start > 63)
        return 0;
    clip->terminal = table + cursor;
    clip->frames = file + frames;
    clip->palette = file + palette;
    u32 key = 2166136261u;
    for (u32 i = 0; i < size; ++i)
        key = (key ^ file[i]) * 16777619u;
    clip->key = key;
    return 1;
}

static unsigned estate_direction(s16 angle)
{
    unsigned direction = (((u16)angle + 0x500u) >> 9) & 7;
    return direction < 5 ? direction : (direction - 5) ^ 3;
}

static const u8 *estate_frame_list(const EstateClip *clip, s16 angle)
{
    const u8 *entry = clip->descriptor + 4 + 2 * estate_direction(angle);
    u32 offset = estate_half(entry);
    if (entry >= clip->commands || (size_t)(clip->commands - entry) < offset + 2)
        return NULL;
    return entry + offset;
}

static int estate_phase_valid(const EstateSpritePhase *phase, const EstateClip *clip, u16 speed)
{
    const u8 *list = estate_frame_list(clip, phase->facing);
    unsigned steps = phase->steps;
    if (!list || phase->tag != ESTATE_PHASE_V1 || phase->reserved || phase->package_key != clip->key ||
        !steps || steps > (unsigned)(clip->terminal - clip->commands) ||
        phase->pc != (unsigned)(clip->commands - clip->descriptor) + steps)
        return 0;
    int frame = estate_half(list) & 0x1ff;
    for (unsigned i = 1; i < steps; ++i)
        frame += clip->commands[i] < 0x10 ? 1 : -1;
    unsigned max_frame = estate_half(clip->frames) & 0x1ff;
    size_t frame_bytes = (size_t)(clip->palette - clip->frames);
    if (frame != phase->frame || !phase->frame || phase->frame > max_frame ||
        (max_frame + 1) * 2 > frame_bytes)
        return 0;
    unsigned body = estate_half(clip->frames + phase->frame * 2);
    if (body < (max_frame + 1) * 2 || body + 4 > frame_bytes)
        return 0;
    if (phase->state == 1)
        return clip->commands + steps == clip->terminal && phase->wait == 0;
    unsigned delay = ((clip->commands[steps - 1] & 15) + 1) * speed >> 8;
    return phase->state == 2 && phase->wait <= (delay ? delay : 1);
}

static void estate_capture_phase(u8 *out, const EstateActors *actors)
{
    EstateClip clip;
    if (!estate_clip(actors->animation, &clip))
        return;
    const u8 *sprite = (const u8 *)(uintptr_t)g_FieldActors[CITAN].pSpriteData;
    u32 flags = estate_word(sprite + 0xa8);
    uintptr_t pc = estate_word(sprite + 0x64);
    if (!estate_pointer_is(sprite + 0x58, clip.descriptor) ||
        pc <= (uintptr_t)clip.commands || pc > (uintptr_t)clip.terminal ||
        pc - (uintptr_t)clip.descriptor > UINT16_MAX || sprite[0x8c] != 0x10 ||
        sprite[0xaf] != actors->animation || ((flags >> 11) & 0x3f) ||
        ((flags >> 20) & 3) != 2)
        return;
    EstateSpritePhase phase = {
        .tag = ESTATE_PHASE_V1, .package_key = clip.key, .pc = (u16)(pc - (uintptr_t)clip.descriptor),
        .frame = estate_half(sprite + 0x34), .wait = estate_half(sprite + 0x9e),
        .facing = (s16)estate_half(sprite + 0x80), .steps = (flags >> 22) & 0x3f, .state = (flags >> 28) & 3
    };
    if (!estate_phase_valid(&phase, &clip, actors->speed) ||
        ((flags >> 17) & 7) != estate_direction(phase.facing) ||
        !estate_pointer_is(sprite + 0x54, estate_frame_list(&clip, phase.facing)))
        return;
    memcpy(out, &phase, sizeof phase);
}

extern void func_8001D2B0(void *sprite, s16 frame);

static void estate_apply_phase(u8 *sprite, const EstateSpritePhase *phase, const EstateClip *clip)
{
    /* Bind the captured facing without invoking a direction-change replay.
     * The clip pointer itself was resolved on the destination field. Bits 2
     * and 3 of AC own directional mirror and frame-list mirror, respectively. */
    u32 flags = estate_word(sprite + 0xa8);
    flags = (flags & ~0x3ffff800u) | (estate_direction(phase->facing) << 17) |
            (2u << 20) | ((u32)phase->steps << 22) | ((u32)phase->state << 28);
    memcpy(sprite + 0xa8, &flags, sizeof flags);
    flags = estate_word(sprite + 0xac);
    if (estate_half(estate_frame_list(clip, phase->facing)) & 0x200)
        flags |= 8;
    else
        flags &= ~8u;
    memcpy(sprite + 0xac, &flags, sizeof flags);
    func_800223B0(sprite, phase->facing);
    u32 pc = (u32)(uintptr_t)(clip->descriptor + phase->pc);
    memcpy(sprite + 0x64, &pc, sizeof pc);
    memcpy(sprite + 0x9e, &phase->wait, sizeof phase->wait);
    /* Queuing even an equal cell lets the ordinary pre-draw drain rebuild
     * the new sprite's texture, palette, cell geometry and scaled extents. */
    func_8001D2B0(sprite, (s16)phase->frame);
}

int PcPort_EstateActorsCapture(uint8_t out[PC_PORT_QUICK_CAVE_BYTES])
{
    EstateActors saved = {0};
    EstateRoutines ranges[ESTATE_OWNER_COUNT];
    if (!out || !capture_estate(&saved, ranges))
        return 0;
    memset(out, 0, PC_PORT_QUICK_ESTATE_ACTOR_BYTES);
    memcpy(out, &saved, sizeof saved);
    estate_capture_phase(out + sizeof saved, &saved);
    return 1;
}

/* The post-dinner director parks after asking Fei to run his departure
 * script. A cold entrance parks on a different branch. EST1 preserves the
 * director, but entering the map constructs Fei with the entrance speed.
 * Recover just the speed operand from the mounted field, without repeating
 * the scripted turn and walk. This also works for existing EST1 saves. */
static int estate_player_speed(const EstateActors *saved, const EstateRoutines *ranges, u16 *speed)
{
    enum { HAND_BACK = 0x20c, CALL_FEI = 0x1f3, DEPARTURE = 12 };
    *speed = 0;
    if (saved->vm[0].ip != HAND_BACK)
        return 1;

    const u8 *code = g_FieldScriptVMCurScriptData;
    if (CALL_FEI < ranges[0].update || HAND_BACK >= ranges[0].talk ||
        code[CALL_FEI] != 0x08 || code[CALL_FEI + 1] != ESTATE_FEI ||
        (code[CALL_FEI + 2] & 0x1f) != DEPARTURE || !g_FieldActors[ESTATE_FEI].pSpriteData)
        return 0;

    const u8 *row = (const u8 *)g_FieldCurScriptFile + 0x84 + ESTATE_FEI * SCRIPT_OFFSET_TABLE_SIZE;
    u16 begin = estate_half(row + 2 * DEPARTURE);
    u16 end = estate_half(row + 2 * (DEPARTURE + 1));
    u16 actor_end = estate_half(row + SCRIPT_OFFSET_TABLE_SIZE);
    if (begin < ranges[0].end || begin + 3 > end || end > actor_end ||
        actor_end > ranges[1].begin || code[begin] != 0x21)
        return 0;
    u16 operand = estate_half(code + begin + 1);
    if (operand <= 0x8000 || operand > 0x8fff)
        return 0;
    *speed = operand & 0xfff;
    return 1;
}

int PcPort_EstateActorsRestore(const uint8_t in[PC_PORT_QUICK_CAVE_BYTES], int apply)
{
    EstateActors saved, current = {0};
    EstateRoutines ranges[ESTATE_OWNER_COUNT];
    if (!in || !capture_estate(&current, ranges))
        return 0;
    memcpy(&saved, in, sizeof saved);
    if (current.script_key != saved.script_key || !estate_values_valid(&saved, ranges))
        return 0;
    u16 player_speed;
    if (!estate_player_speed(&saved, ranges, &player_speed))
        return 0;
    EstateSpritePhase phase;
    EstateClip clip;
    memcpy(&phase, in + sizeof saved, sizeof phase);
    if (phase.tag) {
        const u8 *sprite = (const u8 *)(uintptr_t)g_FieldActors[CITAN].pSpriteData;
        EstateClip bound;
        if (!estate_clip(current.animation, &bound) || sprite[0xaf] != current.animation ||
            !estate_pointer_is(sprite + 0x58, bound.descriptor) || sprite[0x8c] != 0x10 ||
            !estate_clip(saved.animation, &clip) || !estate_phase_valid(&phase, &clip, saved.speed))
            return 0;
    }
    size_t used = sizeof saved + (phase.tag ? sizeof phase : 0);
    for (size_t i = used; i < PC_PORT_QUICK_ESTATE_ACTOR_BYTES; ++i)
        if (in[i])
            return 0;
    if (apply) {
        if (player_speed) {
            actor_data(ESTATE_FEI)->moveSpeed = player_speed;
            func_80021BCC((void *)(uintptr_t)g_FieldActors[ESTATE_FEI].pSpriteData, player_speed);
        }
        ActorData *citan = actor_data(CITAN);
        FieldActor *field = &g_FieldActors[CITAN];
        void *sprite = (void *)(uintptr_t)field->pSpriteData;
        citan->flags = (citan->flags & ~(u32)ESTATE_MOVEMENT_AND_CULL) | saved.movement_flags;
        citan->dialogFlags = saved.dialog_flags;
        citan->moveSpeed = saved.speed;
        func_80021BCC(sprite, saved.speed);
        citan->defaultAnimationId = saved.animation;
        if (citan->curAnimationId != saved.animation) {
            citan->curAnimationId = saved.animation;
            func_800821F4(sprite, saved.animation, field);
        }
        /* The new sprite owns its allocation, animation pointers and portrait
         * cache. Apply its saved numeric pose after the clip has been bound. */
        transfer_pose(&saved.pose, CITAN, 1);
        if (phase.tag)
            estate_apply_phase(sprite, &phase, &clip);
        citan->unk102 = saved.idle_counter;
        for (unsigned i = 0; i < ESTATE_OWNER_COUNT; ++i)
            transfer_estate_vm(&saved.vm[i], estate_owners[i], 1);
        g_FieldScriptMemoryU16[0x40a / 2] = saved.local_40a;
    }
    return 1;
}


enum { BAL_SLOT = 3, BAL_IDLE_IP = 0x721, BAL_TOUCH_IP = 0x92e };

/* Field 71 puts Bal beside the bed when both flags are set. His briefing moves
 * him beside the repair station and sets the second flag without reloading the
 * room. Both locations are authentic for the same saved scenario. */
static int bal_merchant_idle(int restoring)
{
    const u8 *file = (const u8 *)g_FieldCurScriptFile;
    const u8 *code = g_FieldScriptVMCurScriptData;
    if (!bal_sensor_house() || !g_FieldActors || g_FieldNumActors <= BAL_SLOT ||
        g_PlayerActorIndex != 1 || D_800AFB54 != 3 ||
        !file || code != file + 0x84 + 6 * SCRIPT_OFFSET_TABLE_SIZE)
        return 0;
    u16 routines[4];
    memcpy(routines, file + 0x84 + BAL_SLOT * SCRIPT_OFFSET_TABLE_SIZE, sizeof routines);
    if (!script_bytes_match((const u8 *)routines, 3 * sizeof(u16),
                            "a2b7e5dd351eb990db6623d5e584c7d13177d79bd4b42bd9d69d3fc9b6e27ef8") ||
        !script_bytes_match(code + BAL_IDLE_IP - 1, 3,
                            "a808b7e1fc8291206b3d6a125c5a68f84ec708a4da935e95ee9dd7cef4d26c99"))
        return 0;
    const ActorData *actor = actor_data(BAL_SLOT);
    if (!actor || !g_FieldActors[BAL_SLOT].pSpriteData || actor->characterId != 0xff)
        return 0;
    /* RestorePlayer runs immediately after FieldLoad. Routine 0 has stopped on
     * its final END here, before the first field tick selects routine 1. */
    int allow_touch = !restoring && routines[3] == BAL_TOUCH_IP && code[BAL_TOUCH_IP] == 0;
    if (actor->scriptInstructionPointer != BAL_IDLE_IP &&
        !(allow_touch && actor->scriptInstructionPointer == BAL_TOUCH_IP) &&
        (!restoring || actor->scriptInstructionPointer != BAL_IDLE_IP - 1))
        return 0;
    /* Field contact enqueues routine 3 at priority 4 after the VM runs.
     * Bal's entire routine is END. Standing beside him can requeue it on
     * every tick. His pose survives both sides of END without saving a task. */
    for (unsigned i = 0; i < ACTOR_MAX_NUM_SCRIPTS; ++i) {
        const ActorScriptSlot *slot = &actor->scripts[i];
        if (slot->isInUse)
            return 0;
        if (slot->flags_0x12 == 15)
            continue;
        if (!allow_touch || slot->flags_0x12 != 4 ||
            slot->scriptId != 3 || slot->currentIP != BAL_TOUCH_IP)
            return 0;
        allow_touch = 0;
    }
    return 1;
}

/* In memory the two continuations share the actor buffer. The file layer
 * stores Bart separately so that older builds still accept the full BAL1. */
typedef struct {
    CavePose merchant;
    u8 tag[4];
    PartyFollower bart;
} BalActors;

_Static_assert(sizeof(CavePose) == PC_PORT_QUICK_BAL_POSE_BYTES &&
               offsetof(CavePose, sprite_direction) + sizeof(s16) == PC_PORT_QUICK_BAL_POSE_BYTES - 2,
               "BAL1 ends in two bytes of unused alignment padding");
_Static_assert(offsetof(BalActors, tag) == PC_PORT_QUICK_BAL_POSE_BYTES &&
               offsetof(BalActors, bart) == PC_PORT_QUICK_BAL_POSE_BYTES + 4 &&
               sizeof(PartyFollower) == PC_PORT_QUICK_BART_BYTES &&
               sizeof(BalActors) <= PC_PORT_QUICK_CAVE_BYTES, "combined actor buffer");

int PcPort_BalActorCapture(uint8_t out[PC_PORT_QUICK_CAVE_BYTES])
{
    BalActors saved = {0};
    if (!bal_merchant_idle(0))
        return 0;
    transfer_pose(&saved.merchant, BAL_SLOT, 0);
    if (!contact_valid(saved.merchant.layer, saved.merchant.triangles))
        return 0;
    memset(out, 0, PC_PORT_QUICK_CAVE_BYTES);
    size_t used = sizeof saved.merchant;
    if (bal_follower_ready()) {
        capture_follower(&saved.bart, BAL_BART);
        if (valid_follower(&saved.bart)) {
            memcpy(saved.tag, "BRT1", 4);
            used = sizeof saved;
        }
    }
    memcpy(out, &saved, used);
    return 1;
}

int PcPort_BalActorRestore(const uint8_t in[PC_PORT_QUICK_CAVE_BYTES])
{
    BalActors saved;
    if (!bal_merchant_idle(1))
        return 0;
    memcpy(&saved, in, sizeof saved);
    if (!contact_valid(saved.merchant.layer, saved.merchant.triangles))
        return 0;
    int has_bart = !memcmp(saved.tag, "BRT1", 4);
    size_t used = has_bart ? sizeof saved : sizeof saved.merchant;
    for (size_t i = used; i < PC_PORT_QUICK_CAVE_BYTES; ++i)
        if (in[i])
            return 0;
    transfer_pose(&saved.merchant, BAL_SLOT, 1);
    if (bal_follower_ready()) {
        if (has_bart && valid_follower(&saved.bart))
            restore_follower(&saved.bart, BAL_BART);
        else
            group_follower(BAL_FEI, BAL_BART);
    }
    return 1;
}
