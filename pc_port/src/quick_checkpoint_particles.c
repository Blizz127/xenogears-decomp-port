#include "quick_checkpoint_particles.h"
#include "retail_data.h"

#include <string.h>

#include "common.h"
#include "field/actor.h"
#include "field/particles.h"
#include "system/memory.h"

extern s32 g_GameSceneMapNum, g_PlayerActorIndex, D_800ADBFC;
extern u16 g_FieldScriptMemoryU16[] __asm__("g_FieldScriptMemory");
extern u16 g_HeapCurContentType;
extern void FieldInitializeParticlePrimitive(void *, s32, s32);
extern void FieldParticlesFree(int);

enum {
    FIRST_EMITTER = 3,
    EMITTERS = 2,
    SETS_PER_EMITTER = 3,                /* actor routines 3/5 and 4/5 */
    MAX_SETS = EMITTERS * SETS_PER_EMITTER,
    BANKS_PER_SET = 4,
    PRIMITIVES_PER_SET = 6,
    MAX_BYTES = 32768
};

/* These scalar spans are little endian in the host game's retail ABI.
 * Never copy a whole vector, color, bank, primitive, or GPU packet to disk. */
_Static_assert(__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__, "native field byte order");
_Static_assert(sizeof(ParticleBankAngleOffset) == 4, "two packed s16 offsets");

typedef struct {
    const u8 *input;
    u8 *output;
    size_t size, at;
    int failed;
} Stream;

typedef struct {
    s32 position[3];
    s16 rotation[3];
} Origin;

typedef struct {
    u8 slot, actor;
    ParticleBank banks[BANKS_PER_SET];
    ParticlePrimitive primitives[PRIMITIVES_PER_SET];
} SavedSet;

static void transfer(Stream *stream, void *field, size_t bytes)
{
    if (bytes > stream->size - stream->at) {
        stream->failed = 1;
        return;
    }
    if (stream->input)
        memcpy(field, stream->input + stream->at, bytes);
    else
        memcpy(stream->output + stream->at, field, bytes);
    stream->at += bytes;
}

static int in_scene(void)
{
    return (g_GameSceneMapNum & 0x3fff) == 53 &&
           g_FieldScriptMemoryU16[0] == 39 && g_FieldScriptMemoryU16[1] == 0 &&
           (g_FieldScriptMemoryU16[0x182 / 2] & 3) == 3;
}

static int emitter_index(int actor)
{
    return actor >= FIRST_EMITTER && actor < FIRST_EMITTER + EMITTERS;
}

static ActorData *actor_data(int actor)
{
    if (!emitter_index(actor) || actor >= D_800ADBFC || actor == g_PlayerActorIndex ||
        !g_FieldActors)
        return NULL;
    return (ActorData *)(uintptr_t)g_FieldActors[actor].pActorData;
}

static int within(s32 value, s32 limit)
{
    return value >= -limit && value <= limit;
}

static int origin_valid(const Origin *origin)
{
    for (int i = 0; i < 3; ++i) {
        if (!within(origin->position[i], 8192 * 65536) ||
            ((u16)origin->rotation[i] & 0x7000))
            return 0;
    }
    return 1;
}

static void transfer_origins(Stream *stream, Origin origins[EMITTERS])
{
    for (int i = 0; i < EMITTERS; ++i) {
        transfer(stream, origins[i].position, sizeof origins[i].position);
        transfer(stream, origins[i].rotation, sizeof origins[i].rotation);
    }
}

static void transfer_bank(Stream *stream, ParticleBank *bank)
{
    transfer(stream, &bank->swait, 6);               /* swait, ewait, max */
    transfer(stream, &bank->speed, 4);
    transfer(stream, &bank->pos, 6);
    transfer(stream, &bank->epos, 6);
    transfer(stream, &bank->gravity, 6);
    transfer(stream, &bank->speedMultiplier, 8);     /* multiplier, ranges, flags */
    transfer(stream, bank->directions, sizeof bank->directions);
    transfer(stream, &bank->unk50, 10);              /* scale, actor, shape, waits */
    transfer(stream, &bank->scale, 6);
    transfer(stream, &bank->scaleDelta, 6);
    transfer(stream, &bank->color, 3);
    transfer(stream, &bank->colorDelta, 3);
    transfer(stream, &bank->rotAngle, 2);
}

/* PRT1 stores variable-sized primitive pools. Authenticate the fixed fields
 * that identify each bank before accepting the set or allocating its pools.
 * Values come from the loaded field or the checkpoint being validated. */
static int recipe_matches(const ParticleBank banks[BANKS_PER_SET])
{
    uint16_t fields[BANKS_PER_SET][8];
    XenoSha256 hash;
    char digest[65];

    for (int i = 0; i < BANKS_PER_SET; ++i) {
        fields[i][0] = (uint16_t)banks[i].max;
        fields[i][1] = (uint16_t)banks[i].shape;
        fields[i][2] = banks[i].pswait;
        fields[i][3] = banks[i].pewait;
        fields[i][4] = banks[i].flags;
        fields[i][5] = (uint16_t)banks[i].unk50;
        fields[i][6] = banks[i].swait;
        fields[i][7] = (uint16_t)banks[i].speedMultiplier;
    }
    XenoSha256_Init(&hash);
    XenoSha256_Update(&hash, fields, sizeof fields);
    XenoSha256_HexFinal(&hash, digest);
    return strcmp(digest, "8d0d82985beedcecee941ab0fa2d31171ad7bba6bd34c225e0b3a4c27b532b8c") == 0;
}

static int bank_valid(const ParticleBank *bank, int j, int actor)
{
    return bank->targetActorID == actor && bank->ewait <= (j ? 2000 : 0) &&
           within(bank->speed, 32767) &&
           bank->srange <= 4096 && bank->erange <= 4096 &&
           within(bank->gravity.vx, 4096) && within(bank->gravity.vy, 4096) &&
           within(bank->gravity.vz, 4096);
}

static void transfer_primitive(Stream *stream, ParticlePrimitive *p)
{
    transfer(stream, &p->active, 2);
    if (p->active != 1)
        return;                                     /* no expired heap contents */
    transfer(stream, &p->swait, 6);                 /* swait, ewait, rotation */
    transfer(stream, &p->position, 12);
    transfer(stream, &p->velocity, 12);
    transfer(stream, &p->gravity, 12);
    transfer(stream, &p->scale, 6);
    transfer(stream, &p->scaleDelta, 6);
    transfer(stream, &p->color, 3);
    transfer(stream, &p->colorDelta, 3);
}

static int primitive_valid(const ParticlePrimitive *p, const ParticleBank *bank)
{
    if (!p->active)
        return 1;
    return p->active == 1 && p->swait <= bank->pswait * bank->max &&
           p->ewait > 0 && p->ewait <= bank->pewait &&
           within(p->position.vx, 1 << 27) && within(p->position.vy, 1 << 27) &&
           within(p->position.vz, 1 << 27) && within(p->velocity.vx, 1 << 20) &&
           within(p->velocity.vy, 1 << 20) && within(p->velocity.vz, 1 << 20) &&
           p->gravity.vx == bank->gravity.vx && p->gravity.vy == bank->gravity.vy &&
           p->gravity.vz == bank->gravity.vz;
}

static int live_recipe(int slot)
{
    const ParticleBank *banks = g_FieldParticleBanks[slot];
    if (!banks || !recipe_matches(banks))
        return 0;
    for (int j = 0; j < NUM_PARTICLE_BANKS; ++j) {
        if (j < BANKS_PER_SET) {
            if (!bank_valid(&banks[j], j, g_FieldParticleActorIDs[slot]) || !banks[j].pPrimitives)
                return 0;
        } else if (banks[j].max != 0) {
            return 0;
        }
    }
    return 1;
}

static int transfer_set(Stream *stream, SavedSet *set)
{
    transfer(stream, &set->slot, 1);
    transfer(stream, &set->actor, 1);
    if (set->slot >= NUM_PARTICLES || !emitter_index(set->actor))
        return 0;
    int offset = 0;
    for (int j = 0; j < BANKS_PER_SET; ++j) {
        ParticleBank *b = &set->banks[j];
        transfer_bank(stream, b);
        if (stream->failed || !bank_valid(b, j, set->actor) ||
            b->max <= 0 || b->max > PRIMITIVES_PER_SET - offset)
            return 0;
        for (int k = 0; k < b->max; ++k) {
            ParticlePrimitive *p = &set->primitives[offset++];
            transfer_primitive(stream, p);
            if (!primitive_valid(p, b))
                return 0;
        }
    }
    return !stream->failed && recipe_matches(set->banks);
}

int PcPort_FieldParticleCapture(uint8_t *out, size_t capacity, size_t *used)
{
    if (!used)
        return 0;
    *used = 0;
    if (!out || capacity < 6 || !in_scene())
        return 0;
    Stream stream = {.output = out, .size = capacity < MAX_BYTES ? capacity : MAX_BYTES};
    int per_actor[EMITTERS] = {0};
    u8 count = 0;
    memcpy(out, "F53P\1", 5);
    stream.at = 6;
    Origin origins[EMITTERS] = {0};
    for (int i = 0; i < EMITTERS; ++i) {
        ActorData *a = actor_data(i + FIRST_EMITTER);
        if (!a)
            return 0;
        memcpy(origins[i].position, &a->position, sizeof origins[i].position);
        memcpy(origins[i].rotation, &a->rotation, sizeof origins[i].rotation);
        if (!origin_valid(&origins[i]))
            return 0;
    }
    transfer_origins(&stream, origins);
    for (int slot = 0; slot < NUM_PARTICLES; ++slot) {
        int actor = g_FieldParticleActorIDs[slot];
        if (!g_FieldParticleStatuses[slot] || !emitter_index(actor))
            continue;
        if (g_FieldParticleStatuses[slot] != 1 || !live_recipe(slot) ||
            ++per_actor[actor - FIRST_EMITTER] > SETS_PER_EMITTER)
            return 0;
        SavedSet set = {.slot = slot, .actor = actor};
        memcpy(set.banks, g_FieldParticleBanks[slot], sizeof set.banks);
        int offset = 0;
        for (int j = 0; j < BANKS_PER_SET; ++j) {
            const ParticlePrimitive *pool = (const ParticlePrimitive *)(uintptr_t)set.banks[j].pPrimitives;
            int count = set.banks[j].max;
            if (!pool || count <= 0 || count > PRIMITIVES_PER_SET - offset)
                return 0;
            memcpy(&set.primitives[offset], pool, (size_t)count * sizeof *pool);
            offset += count;
        }
        if (!transfer_set(&stream, &set))
            return 0;
        ++count;
    }
    if (!count || stream.failed)
        return 0;
    out[5] = count;
    *used = stream.at;
    return 1;
}

/* Stage the same five independent native heap allocations that the ordinary
 * bank initializer makes. FieldParticlesFree will later reclaim them. */
static void discard(ParticleBank *banks)
{
    if (!banks)
        return;
    for (int j = 0; j < BANKS_PER_SET; ++j)
        if (banks[j].pPrimitives)
            HeapFree((void *)(uintptr_t)banks[j].pPrimitives);
    HeapFree(banks);
}

static ParticleBank *make_set(const SavedSet *set)
{
    ParticleBanks *group = HeapAlloc(sizeof *group, 0);
    if (!group)
        return NULL;
    memset(group, 0, sizeof *group);
    int offset = 0;
    for (int j = 0; j < BANKS_PER_SET; ++j) {
        ParticleBank *bank = &group->banks[j];
        *bank = set->banks[j];
        ParticlePrimitive *pool = HeapAlloc(bank->max * sizeof *pool, 0);
        if (!pool) {
            discard(group->banks);
            return NULL;
        }
        bank->pPrimitives = (u32)(uintptr_t)pool;
        for (int k = 0; k < bank->max; ++k) {
            pool[k] = set->primitives[offset++];
            FieldInitializeParticlePrimitive(&pool[k], bank->shape, ((bank->flags >> 8) + 1) & 3);
        }
    }
    return group->banks;
}

int PcPort_FieldParticleRestore(const uint8_t *data, size_t size)
{
    if (!data || size < 6 || size > MAX_BYTES || memcmp(data, "F53P\1", 5) ||
        !data[5] || data[5] > MAX_SETS || !in_scene())
        return 0;
    int count = data[5];
    Stream stream = {.input = data, .size = size, .at = 6};
    Origin origins[EMITTERS] = {0};
    ActorData *actors[EMITTERS];
    transfer_origins(&stream, origins);
    for (int i = 0; i < EMITTERS; ++i) {
        actors[i] = actor_data(i + FIRST_EMITTER);
        if (!actors[i] || !origin_valid(&origins[i]))
            return 0;
    }
    SavedSet sets[MAX_SETS] = {0};
    int per_actor[EMITTERS] = {0};
    u8 wanted[NUM_PARTICLES] = {0}, replace[NUM_PARTICLES] = {0};
    for (int i = 0; i < count; ++i) {
        if (!transfer_set(&stream, &sets[i]) || (i && sets[i].slot <= sets[i - 1].slot) ||
            ++per_actor[sets[i].actor - FIRST_EMITTER] > SETS_PER_EMITTER)
            return 0;
        wanted[sets[i].slot] = 1;
    }
    if (stream.failed || stream.at != size)
        return 0;
    for (int slot = 0; slot < NUM_PARTICLES; ++slot) {
        if (g_FieldParticleStatuses[slot] == 0)
            continue;
        replace[slot] = g_FieldParticleStatuses[slot] == 1 &&
                        emitter_index(g_FieldParticleActorIDs[slot]) && live_recipe(slot);
        if (wanted[slot] && !replace[slot])
            return 0;
    }

    ParticleBank *staged[MAX_SETS] = {0};
    u_int old_user = HeapGetCurrentUser();
    u_int old_handler = HeapToggleErrorHandler(HEAP_ERROR_HANDLER_OFF);
    u16 old_content = g_HeapCurContentType;
    HeapSetCurrentUser(HEAP_USER_YOSI);
    HeapSetCurrentContentType(HEAP_CONTENT_NONE);
    int made = 0;
    while (made < count && (staged[made] = make_set(&sets[made])) != NULL)
        ++made;
    HeapSetCurrentContentType(old_content);
    HeapSetCurrentUser(old_user);
    HeapToggleErrorHandler(old_handler);
    if (made != count) {
        for (int i = 0; i < made; ++i)
            discard(staged[i]);
        return 0;
    }
    for (int slot = 0; slot < NUM_PARTICLES; ++slot)
        if (replace[slot])
            FieldParticlesFree(slot);
    for (int i = 0; i < count; ++i) {
        int slot = sets[i].slot;
        g_FieldParticleBanks[slot] = staged[i];
        g_FieldParticleActorIDs[slot] = sets[i].actor;
        g_FieldParticleStatuses[slot] = 1;
    }
    for (int i = 0; i < EMITTERS; ++i) {
        if (!per_actor[i])
            continue;
        /* Type 0 samples this origin again whenever a delayed primitive
         * starts. The field smooths rotation.vz toward rotation.vy. */
        memcpy(&actors[i]->position, origins[i].position, sizeof origins[i].position);
        memcpy(&actors[i]->rotation, origins[i].rotation, sizeof origins[i].rotation);
    }
    return 1;
}
