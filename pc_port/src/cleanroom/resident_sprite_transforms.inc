#include "packed_matrix_column_scale.inc"

extern MATRIX D_80018644;
extern MATRIX *ScaleMatrixL(MATRIX *, VECTOR *);
void Retail8003F738RotMatrix(const SVECTOR *, MATRIX *);
void SpriteComputeTransformMatrix(SpriteData *);

typedef u32 SpriteRecordWord __attribute__((__may_alias__, __aligned__(1)));
typedef u16 SpriteRecordHalf __attribute__((__may_alias__, __aligned__(1)));

static volatile SpriteRecordWord *spriteRecordWord(void *record, int offset)
{
    return (volatile void *)((u8 *)record + offset);
}

static volatile SpriteRecordHalf *spriteRecordHalf(void *record, int offset)
{
    return (volatile void *)((u8 *)record + offset);
}

static u8 *spriteTransformRecord(SpriteData *sprite)
{
    return AnimWordPtr(*spriteRecordWord(sprite, 0x20));
}

void SpriteSetScale(SpriteData *sprite, short scale)
{
    u8 *transform = (void *)(uintptr_t)*spriteRecordWord(sprite, 0x20);
    if (transform == NULL)
        return;
    *spriteRecordHalf(sprite, 0x2c) = (u16)scale;
    *spriteRecordHalf(transform, 10) = (u16)scale;
    *spriteRecordHalf(transform, 8) = (u16)scale;
    *spriteRecordHalf(transform, 6) = (u16)scale;
    *spriteRecordWord(sprite, 0x3c) |= 0x10000000u;
}

void func_80022038(SpriteData *sprite)
{
    volatile SpriteRecordWord *flags = spriteRecordWord(sprite, 0x3c);
    if (*flags & 0x10000000u) {
        SpriteComputeTransformMatrix(sprite);
        *flags &= ~0x10000000u;
    }
}

static void spriteCopyBasis(MATRIX *target)
{
    const volatile PackedMatrixWord *source = (const void *)&D_80018644;
    volatile PackedMatrixWord *dest = (void *)target;
    for (int i = 0; i < 6; i += 3) {
        u32 a = source[i];
        u32 b = source[i + 1];
        u32 c = source[i + 2];
        dest[i] = a;
        dest[i + 1] = b;
        dest[i + 2] = c;
    }
    u32 a = source[6];
    u32 b = source[7];
    dest[6] = a;
    dest[7] = b;
}

void SpriteComputeTransformMatrix(SpriteData *sprite)
{
    u32 alternate = *spriteRecordWord(sprite, 0x40) & 1u;
    MATRIX basis, rotation;
    VECTOR factors;
    if (alternate)
        spriteCopyBasis(&basis);
    factors.vx = (s16)*spriteRecordHalf(spriteTransformRecord(sprite), 6);
    factors.vy = (s16)*spriteRecordHalf(spriteTransformRecord(sprite), 8);
    factors.vz = (s16)*spriteRecordHalf(spriteTransformRecord(sprite), 10);
    if (alternate) {
        ScaleMatrixL(&basis, &factors);
        Retail8003F738RotMatrix((const void *)spriteTransformRecord(sprite), &rotation);
        MulMatrix0(&rotation, &basis, (void *)(spriteTransformRecord(sprite) + 12));
    } else {
        u8 *transform = spriteTransformRecord(sprite);
        Retail8003F738RotMatrix((const void *)transform, (void *)(transform + 12));
        ScaleMatrixL((void *)(spriteTransformRecord(sprite) + 12), &factors);
    }
    unsigned int extra = *spriteRecordHalf(sprite, 0x3a);
    if (extra) {
        factors.vx = (int)(extra >> 1);
        factors.vy = (int)(*spriteRecordHalf(sprite, 0x3a) >> 1);
        factors.vz = (int)(*spriteRecordHalf(sprite, 0x3a) >> 1);
        packedMatrixScaleColumns((void *)(spriteTransformRecord(sprite) + 12), &factors);
    }
}
