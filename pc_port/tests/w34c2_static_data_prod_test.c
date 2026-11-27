#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psx_memory.h"


static int s_failures;

#define ASSERT_MSG(cond, name, ...)                                        \
    do {                                                                  \
        if (!(cond)) {                                                    \
            s_failures++;                                                 \
            fprintf(stderr, "ASSERTION %s FAILED: ", name);               \
            fprintf(stderr, __VA_ARGS__);                                 \
            fprintf(stderr, "\n");                                        \
        }                                                                 \
    } while (0)

static uint8_t* read_file(const char* path, size_t* size)
{
    FILE* f = fopen(path, "rb");
    long len;
    uint8_t* buf;
    if (f == NULL)
        return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) <= 0 || fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buf = (uint8_t*)malloc((size_t)len);
    if (!buf || fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = (size_t)len;
    return buf;
}

static int range_is_zero(uint32_t start, uint32_t end)
{
    const uint8_t* p = (const uint8_t*)PSX_ADDR(start);
    size_t i;
    for (i = 0; i < (size_t)(end - start); i++)
        if (p[i] != 0)
            return 0;
    return 1;
}

static int range_matches_image(const uint8_t* image, uint32_t start, uint32_t end)
{
    size_t off = PSX_EXE_HEADER_SIZE + (size_t)(start - PSX_EXE_LOAD_BASE);
    return memcmp(PSX_ADDR(start), image + off, (size_t)(end - start)) == 0;
}

static uint32_t guest_lhu_pair(uint32_t address)
{
    uint16_t lo, hi;
    memcpy(&lo, PSX_ADDR(address), 2);
    memcpy(&hi, PSX_ADDR(address + 2u), 2);
    return ((uint32_t)hi << 16) | lo;
}

int main(void)
{
    static const char* const paths[] = {
        "disc/SLUS_006.64", "../disc/SLUS_006.64", "../../disc/SLUS_006.64",
    };
    uint8_t* image = NULL;
    size_t size = 0;
    size_t i;
    int rc;

    for (i = 0; image == NULL && i < 3; i++)
        image = read_file(paths[i], &size);
    if (image == NULL) {
        fprintf(stderr, "ASSERTION image FAILED: disc/SLUS_006.64 missing\n");
        return 1;
    }
    if (size != 0x4A000u || memcmp(image, "PS-X EXE", 8) != 0) {
        fputs("ASSERTION image FAILED: unexpected PS-X executable revision or size\n", stderr);
        free(image);
        return 1;
    }

    PsxMemory_Init();
    /* Canary in the not-copied ranges must survive as zero, so pre-check. */
    ASSERT_MSG(range_is_zero(PSX_EXE_TEXT_START, PSX_EXE_TEXT_END), "pre_text",
               ".text not zero after init");

    /* Canary in the island range: the loader must not write there at all,
     * whatever the file holds at 0x49AC0 (M2). */
    memset(PSX_ADDR(PSX_EXE_ISLAND_START), 0xA5,
           PSX_EXE_ISLAND_END - PSX_EXE_ISLAND_START);
    /* Canary in .sbss [0x800592C0,0x80059800): the loader must stop exactly at
     * the .sdata/.sbss boundary and never copy guest bytes there (M4). A plain
     * "still zero" check cannot see M4 any more now that the production range
     * correctly ends at 0x800592C0, because retail .sbss bytes are zero too;
     * a canary makes the illegal copy observable. */
    memset(PSX_ADDR(PSX_EXE_SBSS_START), 0xA5,
           PSX_EXE_IMAGE_END - PSX_EXE_SBSS_START);

    rc = PsxMemory_LoadStaticDataFromImage(image, size);
    ASSERT_MSG(rc == 0, "load_rc", "loader returned %d", rc);

    /* 1. rodata + sdata byte-identical to the file image. */
    ASSERT_MSG(range_matches_image(image, PSX_EXE_RODATA_START, PSX_EXE_RODATA_END),
               "rodata_bytes", "rodata [0x%08x,0x%08x) differs from image",
               PSX_EXE_RODATA_START, PSX_EXE_RODATA_END);
    ASSERT_MSG(range_matches_image(image, PSX_EXE_SDATA_START, PSX_EXE_SDATA_END),
               "sdata_bytes", "sdata [0x%08x,0x%08x) differs from image",
               PSX_EXE_SDATA_START, PSX_EXE_SDATA_END);
    /* Last rodata page must be present (M5: range short by one page). */
    ASSERT_MSG(!range_is_zero(PSX_EXE_RODATA_END - 0x1000u, PSX_EXE_RODATA_END),
               "rodata_tail", "last rodata page is zero");

    /* Confirm the loader's paired halfwords at representative angle-table indices. */
    const unsigned indices[] = {0u, 0x400u, 0x600u, 0x800u};
    for (unsigned index = 0; index < sizeof(indices) / sizeof(indices[0]); ++index) {
        uint32_t address = 0x800523f0u + 4u * indices[index];
        const uint8_t *word = image + PSX_EXE_HEADER_SIZE + (address - PSX_EXE_LOAD_BASE);
        uint32_t expected = (uint32_t)word[0] | (uint32_t)word[1] << 8 |
                            (uint32_t)word[2] << 16 | (uint32_t)word[3] << 24;
        ASSERT_MSG(guest_lhu_pair(address) == expected, "angle_table",
                   "paired halfwords at index 0x%x differ from the supplied executable", indices[index]);
    }

    /* 3. .text guest bytes remain zero (M1). */
    ASSERT_MSG(range_is_zero(PSX_EXE_TEXT_START, PSX_EXE_TEXT_END), "text_zero",
               ".text [0x%08x,0x%08x) was populated", PSX_EXE_TEXT_START,
               PSX_EXE_TEXT_END);
    /* 4. ._49AC0 island untouched (M2). */
    {
        const uint8_t* p = (const uint8_t*)PSX_ADDR(PSX_EXE_ISLAND_START);
        size_t k, intact = 1;
        for (k = 0; k < (size_t)(PSX_EXE_ISLAND_END - PSX_EXE_ISLAND_START); k++)
            if (p[k] != 0xA5) { intact = 0; break; }
        ASSERT_MSG(intact, "island_untouched",
                   "island [0x%08x,0x%08x) canary overwritten at +0x%zx",
                   PSX_EXE_ISLAND_START, PSX_EXE_ISLAND_END, k);
        memset(PSX_ADDR(PSX_EXE_ISLAND_START), 0,
               PSX_EXE_ISLAND_END - PSX_EXE_ISLAND_START);
    }
    /* 5. .sbss stays untouched by the loader (M4: sdata range extended into
     *    .sbss copies the guest bytes there, zeroing the 0xA5 canary). */
    {
        const uint8_t* p = (const uint8_t*)PSX_ADDR(PSX_EXE_SBSS_START);
        size_t k, intact = 1;
        for (k = 0; k < (size_t)(PSX_EXE_IMAGE_END - PSX_EXE_SBSS_START); k++)
            if (p[k] != 0xA5) { intact = 0; break; }
        ASSERT_MSG(intact, "sbss_untouched",
                   ".sbss [0x%08x,0x%08x) canary overwritten at +0x%zx (loader "
                   "copied past 0x%08x)", PSX_EXE_SBSS_START, PSX_EXE_IMAGE_END,
                   k, PSX_EXE_SDATA_END);
    }
    /* Nothing above the image end either. */
    ASSERT_MSG(range_is_zero(PSX_EXE_IMAGE_END, PSX_EXE_IMAGE_END + 0x1000u),
               "above_image_zero", "bytes above 0x80059800 were populated");

    /* The loader's former world data address must agree with its source image. */
    ASSERT_MSG(range_matches_image(image, 0x80050100u, 0x80050104u),
               "guest_word", "loaded 32-bit word differs from the supplied executable");

    /* 7. Malformed image is rejected and leaves RAM untouched. */
    {
        uint8_t bogus[0x800];
        PsxMemory_Init();
        memset(bogus, 0x5A, sizeof(bogus));
        ASSERT_MSG(PsxMemory_LoadStaticDataFromImage(bogus, sizeof(bogus)) != 0,
                   "reject_bogus", "non PS-X image accepted");
        ASSERT_MSG(range_is_zero(PSX_EXE_RODATA_START, PSX_EXE_SDATA_END),
                   "reject_untouched", "RAM modified by rejected image");
    }

    free(image);
    if (s_failures != 0) {
        fprintf(stderr, "W34C2 static data certificate: %d failure(s)\n",
                s_failures);
        return 1;
    }
    printf("W34C2 static data certificate PASS\n");
    return 0;
}
