#ifndef XENO_FIELD_LINKED_GEOMETRY_H
#define XENO_FIELD_LINKED_GEOMETRY_H

#include <stdint.h>

/* Supply every required block before constructing a private Gear actor.
 * The field entry point passes NULL to retain its original allocator ABI. */
typedef struct {
    void *points, *table, *vertices, *faces, *constraints;
} PcFieldLinkedGeometryStorage;

void PcPort_FieldFillLinkedGeometry(const PcFieldLinkedGeometryStorage *storage,
    uint8_t *owner, const uint16_t *source, int32_t bias, int32_t scale,
    int32_t x, int32_t y, int32_t z, int32_t constraints, int32_t texX, int32_t texY,
    int32_t width, int32_t height, int32_t clutX, int32_t clutY,
    int32_t r, int32_t g, int32_t b, int32_t backR, int32_t backG, int32_t backB);

#endif
