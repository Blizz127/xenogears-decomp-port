#ifndef XENO_PC_PORT_QUICK_CHECKPOINT_FILE_H
#define XENO_PC_PORT_QUICK_CHECKPOINT_FILE_H

#include <stddef.h>
#include <stdint.h>

/* The PSX headers can typedef size_t to a 32-bit retail word. Every user
 * of this host-only structure must see the same native particle/actor offsets. */
_Static_assert(sizeof(size_t) == sizeof(sizeof(0)), "include host checkpoint API before common.h");

#define PC_PORT_QUICK_CHECKPOINT_STATE_BYTES 0x2358u
#define PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES 48u
#define PC_PORT_QUICK_LIGHTING_WORDS 13u
#define PC_PORT_QUICK_EFFECT_BYTES 32768u
#define PC_PORT_QUICK_CAVE_BYTES 2048u
#define PC_PORT_QUICK_BAL_POSE_BYTES 172u
#define PC_PORT_QUICK_BART_BYTES 1728u
#define PC_PORT_QUICK_FOLLOW_CAMERA_BYTES 128u
#define PC_PORT_QUICK_PARTY_BYTES 1920u
/* At divisor 256 the step calculator multiplies radius >> 4 by a signed
 * trig coefficient of magnitude <= 1024 before its final shift. */
#define PC_PORT_QUICK_MAX_IDLE_RADIUS (INT32_MAX / 64)
#define PC_PORT_QUICK_ESTATE_CAMERA_BYTES PC_PORT_QUICK_FOLLOW_CAMERA_BYTES
#define PC_PORT_QUICK_ESTATE_ACTOR_BYTES (PC_PORT_QUICK_CAVE_BYTES - PC_PORT_QUICK_ESTATE_CAMERA_BYTES)

enum {
    PC_PORT_QUICK_ACTORS_NONE,
    PC_PORT_QUICK_ACTORS_CAVE,
    PC_PORT_QUICK_ACTORS_ESTATE,
    PC_PORT_QUICK_ACTORS_BAL,
    PC_PORT_QUICK_ACTORS_FOLLOWER,
    PC_PORT_QUICK_ACTORS_DAZIL,
    PC_PORT_QUICK_ACTORS_TANK_ROOM,
    PC_PORT_QUICK_ACTORS_GONDOLA,
    PC_PORT_QUICK_ACTORS_FOLLOW_CAMERA
};

typedef struct PcPortQuickCheckpoint {
    uint8_t game_state[PC_PORT_QUICK_CHECKPOINT_STATE_BYTES];
    uint16_t map;
    uint16_t entrance;
    int32_t position[3];
    int16_t rotation[3];
    /* The retail rand seed (g_RandomSeed, a host global in the port, so not
     * part of g_GameState), kept in a sidecar "<path>.rng" tied to this
     * checkpoint by its checksum. The main file format is unchanged; a save
     * without a valid matching sidecar loads with has_random_seed = 0. */
    uint8_t has_random_seed;
    uint32_t random_seed;
    /* Optional v1 contact hint. UINT16_MAX means only the layer is known. */
    uint8_t has_walkmesh;
    uint8_t walkmesh_layer;
    uint16_t walkmesh_triangle;
    /* Raw unsigned File-menu counter from retail's card payload. Old
     * checkpoints omit the optional checksum-bound "<path>.time" record. */
    uint8_t has_file_ticks;
    uint32_t file_ticks;
    /* GTE lighting control registers 8..20 at the time of the quicksave. */
    uint8_t has_field_lighting;
    uint32_t field_lighting[PC_PORT_QUICK_LIGHTING_WORDS];
    /* Settled follow camera. Heading is a 12-bit PSX angle; both vectors are Q16. */
    uint8_t has_field_camera;
    uint16_t camera_heading;
    int32_t camera_eye[3];
    int32_t camera_at[3];
    /* A length of zero leaves the field particle companion absent. */
    size_t field_particles_size;
    uint8_t field_particles[PC_PORT_QUICK_EFFECT_BYTES];
    /* Checksum-bound field continuation from this F11, in the .actors companion. */
    uint8_t field_actor_kind;
    uint8_t field_actors[PC_PORT_QUICK_CAVE_BYTES];
    /* Optional delayed music and fade state, stored in the .music companion. */
    uint8_t has_delayed_music;
    uint8_t music_wait;
    int32_t music_fade_value;
    int32_t music_fade_step;
    uint16_t music_fade_ticks;
    uint8_t on_disc2;
    /* Read-side provenance. An absent .actors file can identify older saves;
     * an ignored corrupt or unsupported actor record cannot. */
    uint8_t field_actor_file_rejected;
    /* Optional party poses and movement trail paired with the saved FCV1 camera. */
    uint8_t has_party_followers;
    uint8_t party_followers[PC_PORT_QUICK_PARTY_BYTES];
    /* Optional FRD1 associated with the forest party and its camera. */
    uint8_t has_forest_idle_radius;
    int32_t forest_idle_radius;
} PcPortQuickCheckpoint;

/* Return 0 on success; all failures are reported as nonzero. */
int PcPort_QuickCheckpointWriteFile(const char* path,
                                    const PcPortQuickCheckpoint* checkpoint);
int PcPort_QuickCheckpointReadFile(const char* path,
                                   PcPortQuickCheckpoint* checkpoint);

#endif
