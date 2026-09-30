#ifndef _XENO_FIELD_PARTICLES_H
#define _XENO_FIELD_PARTICLES_H

#include "psyq/libgte.h"
#include "psyq/libgpu.h"

#define NUM_PARTICLE_BANKS 0x8
#define NUM_PARTICLES 0x40

#define PARTICLE_SORT_TOP 0x0
#define PARTICLE_SORT_MID 0x1
#define PARTICLE_SORT_NORMAL 0x2
#define PARTICLE_SORT_BACK 0x3

#define PARTICLE_RANGE_MODE_RANDOM 0x0
#define PARTICLE_RANGE_MODE_LINE 0x1
#define PARTICLE_RANGE_MODE_CIRCLE 0x2

#define PARTICLE_FLAG_RANDROT(flags) (flags & 0x1)
#define PARTICLE_FLAG_SORT(flags) (flags >> 1) & 3
#define PARTICLE_FLAG_TYPE(flags) (flags >> 4) & 3
#define PARTICLE_FLAG_RANGE_MODE(flags) (flags >> 6) & 3

// 0: 1.0*Bk + 1.0*Fw
// 1: 1.0*Bk - 1.0*Fw
// 2: 1.0*Bk + 0.25*Fw
// 3: 0.5*Bk + 0.5*Fw
#define PARTICLE_FLAG_COLOR_MODE(flags) (flags >> 8) & 3

typedef struct {
    /* 0x0  */ u_short x0;
    /* 0x2  */ u_short y0;
    /* 0x4  */ u_short x1;
    /* 0x6  */ u_short y1;
    /* 0x8  */ u_short u0;
    /* 0xA  */ u_short v0;
    /* 0xC  */ u_short u1;
    /* 0xE  */ u_short v1;
    /* 0x10 */ u_short u2;
    /* 0x12 */ u_short v2;
    /* 0x14 */ u_short u3;
    /* 0x16 */ u_short v3;
} ParticleShape; // Size: 0x18

typedef struct {
    /* 0x0 */ s_char r;
    /* 0x1 */ s_char g;
    /* 0x2 */ s_char b;
    /* 0x3 */ s_char a;
} ParticleColorDelta; // Size: 0x4

typedef struct {
    /* 0x0  */ short active;
    /* 0x2  */ u_short swait; // Delay before particle starts
    /* 0x4  */ u_short ewait; // Duration of particle
    /* 0x6  */ short zRotation;
    /* 0x8  */ VECTOR position;
    /* 0x18 */ VECTOR velocity;
    /* 0x28 */ VECTOR gravity;
    /* 0x38 */ SVECTOR scale;
    /* 0x40 */ SVECTOR scaleDelta;
    /* 0x48 */ CVECTOR color;
    /* 0x4C */ ParticleColorDelta colorDelta;
    /* 0x50 */ POLY_FT4 poly[2];
    /* 0xA0 */ SVECTOR vertices[4];
} ParticlePrimitive; // Size: 0xC0

typedef struct {
    /* 0x0 */ short x;
    /* 0x2 */ short z;
} ParticleBankAngleOffset;

typedef struct {
    /* 0x0   */ undefined16 unk0;
    /* 0x2   */ u_short swait;
    /* 0x4   */ u_short ewait;
    /* 0x6   */ short max;
    /* 0x8   */ int speed;
    /* 0xC   */ SVECTOR pos;
    /* 0x14  */ SVECTOR epos;
    /* 0x1C  */ SVECTOR gravity;
    /* 0x24  */ short speedMultiplier;
    /* 0x26  */ u_short srange;
    /* 0x28  */ u_short erange;
    /* 0x2A  */ u_short flags;
    /* 0x2C  */ u32 pPrimitives; // Packed 32-bit address; convert at use sites.
    /* 0x30  */ ParticleBankAngleOffset directions[8];
    /* 0x50  */ s16 unk50;
    /* 0x52  */ short targetActorID;
    /* 0x54  */ short shape;
    /* 0x56  */ u_short pswait;
    /* 0x58  */ u_short pewait;
    /* 0x5A  */ SVECTOR scale;
    /* 0x62  */ SVECTOR scaleDelta;
    /* 0x6A  */ CVECTOR color;
    /* 0x6E  */ ParticleColorDelta colorDelta;
    /* 0x72  */ s16 unk72;
    /* 0x74  */ s16 unk74;
    /* 0x76  */ short rotAngle;
} ParticleBank; // Size: 0x78

#ifdef XENO_PC_PORT
/* Scripts mix typed access with the retail 0x78-byte stride. These must stay
 * packed even though standalone host pointer tables use native pointers. */
_Static_assert(sizeof(ParticleBank) == 0x78, "retail particle bank stride");
_Static_assert(__builtin_offsetof(ParticleBank, pPrimitives) == 0x2C,
               "retail particle pointer slot");
_Static_assert(__builtin_offsetof(ParticleBank, directions) == 0x30,
               "retail particle direction offset");
_Static_assert(sizeof(ParticlePrimitive) == 0xC0, "retail particle stride");
_Static_assert(__builtin_offsetof(ParticlePrimitive, vertices) == 0xA0,
               "retail particle vertex offset");
#endif

typedef struct { 
    ParticleBank banks[8]; 
} ParticleBanks;

extern int g_FieldParticleCurActor;

extern int g_FieldParticleBankIndex;
extern ParticleBank g_FieldDefaultParticleBanks[NUM_PARTICLE_BANKS];

extern short g_FieldParticleActorIDs[NUM_PARTICLES];
extern u_char g_FieldParticleStatuses[NUM_PARTICLES];
extern ParticleBank* g_FieldParticleBanks[NUM_PARTICLES];

#endif
