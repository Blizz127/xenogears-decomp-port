/*
 * W34C2 — PS-X static data load path certificate.
 *
 * Asserts the retail contract, not the port's: after loading disc/SLUS_006.64
 * (PS-X EXE, image at file offset 0x800 = guest 0x80010000) the main-exe
 * rodata and sdata are byte-identical to the file in g_PsxRam, .text and the
 * ._49AC0 island stay zero, .sbss stays zero, the wm_80099708 sine table
 * at 0x800523F0 is resident, and wm_80073B04's OT shift is the host
 * D_80050100 (=2), never the guest twin (0 before load, 0x342e342b after).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common.h"
#include "psx_memory.h"
#include "world_map_helper_73b04.h"

/* Host authority, as in game_overrides.c. */
s32 D_80050100 = 2;

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
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buf = (uint8_t*)malloc((size_t)len);
    if (fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *size = (size_t)len;
    return buf;
}

static int range_is_zero(u32 start, u32 end)
{
    const uint8_t* p = (const uint8_t*)PSX_ADDR(start);
    size_t i;
    for (i = 0; i < (size_t)(end - start); i++)
        if (p[i] != 0)
            return 0;
    return 1;
}

static int range_matches_image(const uint8_t* image, u32 start, u32 end)
{
    size_t off = PSX_EXE_HEADER_SIZE + (size_t)(start - PSX_EXE_LOAD_BASE);
    return memcmp(PSX_ADDR(start), image + off, (size_t)(end - start)) == 0;
}

static u32 guest_lhu_pair(u32 address)
{
    u16 lo, hi;
    memcpy(&lo, PSX_ADDR(address), 2);
    memcpy(&hi, PSX_ADDR(address + 2u), 2);
    return ((u32)hi << 16) | lo;
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
    ASSERT_MSG(memcmp(image, "PS-X EXE", 8) == 0, "magic", "not a PS-X EXE");
    ASSERT_MSG(size == 0x4A000u, "size", "file size 0x%zx != 0x4a000", size);

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

    /* 2. sine table resident: port arithmetic base + index*4, halfword pairs. */
    ASSERT_MSG(guest_lhu_pair(0x800523F0u + 0x000u * 4u) == 0x10000000u,
               "sine_0000", "0x%08x", guest_lhu_pair(0x800523F0u));
    ASSERT_MSG(guest_lhu_pair(0x800523F0u + 0x400u * 4u) == 0x00001000u,
               "sine_0400", "0x%08x", guest_lhu_pair(0x800523F0u + 0x1000u));
    ASSERT_MSG(guest_lhu_pair(0x800523F0u + 0x600u * 4u) == 0xF4B00B50u,
               "sine_0600", "0x%08x", guest_lhu_pair(0x800523F0u + 0x1800u));
    ASSERT_MSG(guest_lhu_pair(0x800523F0u + 0x800u * 4u) == 0xF0000000u,
               "sine_0800", "0x%08x", guest_lhu_pair(0x800523F0u + 0x2000u));

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

    /* 6. WM_73B04_SHIFT reads the host authority: 2, not 0, not 11 (M3). */
    {
        s32 shift = wm_73b04_ot_shift();
        u32 guest_word;
        memcpy(&guest_word, PSX_ADDR(0x80050100u), 4);
        /* Retail's static .sdata already holds 2 here (the overlay rewrites
         * 2 at 0x800847D8); the host global is still the authority. */
        ASSERT_MSG(guest_word == 0x00000002u, "guest_twin",
                   "guest 0x80050100 = 0x%08x (expected retail value 2)",
                   guest_word);
        ASSERT_MSG(shift == 2, "ot_shift", "wm_73b04_ot_shift()=%d (0x%x); "
                   "srav amount would be %u", shift, (unsigned)shift,
                   (unsigned)shift & 31u);
        D_80050100 = 5;
        ASSERT_MSG(wm_73b04_ot_shift() == 5, "ot_shift_host",
                   "shift does not follow host D_80050100");
        D_80050100 = 2;
    }

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
