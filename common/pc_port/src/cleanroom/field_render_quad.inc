#include <stddef.h>
#include <stdint.h>

#if defined(__GNUC__) || defined(__clang__)
#define FIELD_RENDER_QUAD_OFFSETOF(type, member) __builtin_offsetof(type, member)
#else
#define FIELD_RENDER_QUAD_OFFSETOF(type, member) offsetof(type, member)
#endif

void FieldRenderQuad(u_long* ot, Quad* quad, MATRIX* matrix, int context)
{
    enum {
        packetArrayOffset = 32,
        x0Offset = 8,
        x1Offset = 16,
        x2Offset = 24,
        x3Offset = 32
    };
    _Static_assert(FIELD_RENDER_QUAD_OFFSETOF(Quad, polys) == packetArrayOffset,
                   "Quad packet array must start at byte 32");
    _Static_assert(sizeof(POLY_FT4) == 40, "POLY_FT4 must occupy 40 bytes");
    _Static_assert(FIELD_RENDER_QUAD_OFFSETOF(POLY_FT4, x0) == x0Offset,
                   "POLY_FT4 x0 must start at byte 8");
    _Static_assert(FIELD_RENDER_QUAD_OFFSETOF(POLY_FT4, x1) == x1Offset,
                   "POLY_FT4 x1 must start at byte 16");
    _Static_assert(FIELD_RENDER_QUAD_OFFSETOF(POLY_FT4, x2) == x2Offset,
                   "POLY_FT4 x2 must start at byte 24");
    _Static_assert(FIELD_RENDER_QUAD_OFFSETOF(POLY_FT4, x3) == x3Offset,
                   "POLY_FT4 x3 must start at byte 32");
#if defined(__GNUC__) || defined(__clang__)
    typedef uint32_t TagWord __attribute__((__may_alias__));
#else
    typedef uint32_t TagWord;
#endif
    uint32_t offset = (uint32_t)packetArrayOffset +
                      (uint32_t)context * (uint32_t)sizeof(POLY_FT4);
    uintptr_t packetAddress = (uintptr_t)quad + (uintptr_t)(intptr_t)(int32_t)offset;
    unsigned char* packet = (unsigned char*)packetAddress;
    volatile TagWord* packetTag = (volatile TagWord*)packet;
    volatile TagWord* otTag = (volatile TagWord*)((unsigned char*)ot + sizeof(uint32_t));
    uint32_t packetWord;
    uint32_t otWord;
    long p;
    long flag;

    PushMatrix();
    SetRotMatrix(matrix);
    SetTransMatrix(matrix);
    RotAverage4(&quad->vertices[0], &quad->vertices[1],
                &quad->vertices[2], &quad->vertices[3],
                (long*)(packet + x0Offset), (long*)(packet + x1Offset),
                (long*)(packet + x2Offset), (long*)(packet + x3Offset), &p, &flag);

    packetWord = *packetTag;
    otWord = *otTag;
    *packetTag = (packetWord & 0xff000000u) | (otWord & 0x00ffffffu);
    otWord = *otTag;
    *otTag = (otWord & 0xff000000u) | ((uint32_t)packetAddress & 0x00ffffffu);
    PopMatrix();
}

#undef FIELD_RENDER_QUAD_OFFSETOF
