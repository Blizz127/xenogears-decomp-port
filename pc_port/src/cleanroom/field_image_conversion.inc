#include <stdint.h>
#include <string.h>

/* A live native allocation may have address bits beyond its compact stream
 * slot. Keep its owner available while converting a strip. */
static unsigned char *fieldImageSourceOwner;

#define FIELD_IMAGE_SLOT(cell) (*(volatile __typeof__(cell) *)&(cell))

static unsigned char *FieldImageResolveWord(uint32_t slot, unsigned char *owner,
                                            uint32_t length)
{
    if (owner != 0) {
        uint32_t offset = slot - (uint32_t)(uintptr_t)owner;
        if (offset <= length - sizeof(uint32_t))
            return owner + offset;
    }
    return (unsigned char *)(uintptr_t)slot;
}

unsigned int FieldImageConvert24BitColorTo15Bit(void)
{
    uint32_t index = (uint32_t)FIELD_IMAGE_SLOT(g_FieldPixelIndex);
    if ((index & 3u) == 0) {
        uint32_t address = FIELD_IMAGE_SLOT(g_Field24BitImageData);
        uint32_t next;
        memcpy(&next, FieldImageResolveWord(address, fieldImageSourceOwner, 43008),
               sizeof(next));
        FIELD_IMAGE_SLOT(g_Field24BitImageData) = address + 4;
        FIELD_IMAGE_SLOT(g_FieldCurPixel) = next;
    }
    uint32_t channels = FIELD_IMAGE_SLOT(g_FieldCurPixel);
    FIELD_IMAGE_SLOT(g_FieldPixelIndex) = (int32_t)(index + 1);
    FIELD_IMAGE_SLOT(g_FieldCurPixel) = channels >> 8;

    unsigned int channel = channels & 255u;
    if (channel != 0 && channel < 8)
        return 1;
    return channel >> 3;
}

void FieldImageConvert24BitTo15Bit(void)
{
    unsigned char *source = HeapAlloc(43008, 0);
    unsigned char *output = HeapAlloc(28672, 0);
    unsigned char *previousSource = fieldImageSourceOwner;
    if (source != 0 && output != 0) {
        RECT region;
        for (unsigned int strip = 0; strip < 5; ++strip) {
            region = (RECT){ (short)(96 * strip), 0, 96, 224 };
            StoreImage(&region, (u_long *)source);
            DrawSync(0);
            fieldImageSourceOwner = source;
            FIELD_IMAGE_SLOT(g_Field24BitImageData) = (uint32_t)(uintptr_t)source;
            FIELD_IMAGE_SLOT(g_Field15BitImageData) = (uint32_t)(uintptr_t)output;
            FIELD_IMAGE_SLOT(g_FieldPixelIndex) = 0;
            for (unsigned int word = 0; word < 7168; ++word) {
                static const unsigned char shifts[] = { 0, 5, 10, 16, 21, 26 };
                uint32_t packed = 0;
                for (unsigned int channel = 0; channel < 6; ++channel)
                    packed |= (uint32_t)FieldImageConvert24BitColorTo15Bit() << shifts[channel];
                unsigned char *dest = FieldImageResolveWord(FIELD_IMAGE_SLOT(g_Field15BitImageData), output, 28672);
                memcpy(dest, &packed, sizeof(packed));
                FIELD_IMAGE_SLOT(g_Field15BitImageData) += 4;
            }
            region = (RECT){ (short)(64 * strip), 256, 64, 224 };
            LoadImage(&region, (u_long *)output);
            DrawSync(0);
        }
    }
    fieldImageSourceOwner = previousSource;
    if (source != 0)
        HeapFree(source);
    if (output != 0)
        HeapFree(output);
}

#undef FIELD_IMAGE_SLOT
