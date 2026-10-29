#ifndef PC_PORT_HD2D_HEAD_CROP_VARIANTS_H
#define PC_PORT_HD2D_HEAD_CROP_VARIANTS_H
/* Test-only, predetermined source UV rectangles. All sample unchanged pixels
 * inside original first pose tile; padded variants include its existing clear
 * left margin. No raster transforms or production profile mutation. */
typedef struct { const char *name; unsigned short x,y,width,height; } Hd2dHeadCropVariant;
static const Hd2dHeadCropVariant hd2d_head_crop_variants[] = {
    {"baseline",100,37,78,120},
    {"padding_200",0,37,200,120},
    {"padding_184",0,37,184,120},
    {"padding_216",0,37,216,120},
    {"padding_200_down20",0,57,200,120},
    {"padding_200_down40",0,77,200,120},
    {"padding_184_down40",0,77,184,120},
    {"padding_216_down40",0,77,216,120}
};
#define HD2D_HEAD_CROP_VARIANT_COUNT (sizeof hd2d_head_crop_variants / sizeof hd2d_head_crop_variants[0])
/* Existing harness uses eight-present retail/HD/hidden blocks; /3 associates
 * every HD block with one retail/hidden triple, regardless of starting frame. */
static unsigned Hd2dHeadCropVariantIndex(unsigned next_present)
{
    return (next_present / 8 / 3) % HD2D_HEAD_CROP_VARIANT_COUNT;
}
#endif
