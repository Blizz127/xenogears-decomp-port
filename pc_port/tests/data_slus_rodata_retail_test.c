/*
 * data_slus_rodata_retail_test.c - retail-byte certificate for
 * pc_port/src/data_slus_rodata.c.
 *
 * The test includes the generated data file directly (override the include path
 * with -DDATA_SLUS_RODATA_SRC=... to test a mutant copy), then compares every
 * defined object against disc/SLUS_006.64:
 *   - every object must memcmp equal to the retail image window
 *     [vaddr, vaddr + sizeof);
 *   - the three head objects (D_80010000 / D_80010004 / D_80018004) must tile
 *     [0x80010000, 0x80018084) exactly, with D_80010000 == 0xFFFFFFFF;
 *   - every symbol must be non-zero in retail (the audit's mismatch criterion).
 *
 * It also pins the loader's .rodata boundary constants that the same change
 * depends on, so a regression that moves PSX_EXE_RODATA_END fails here.
 *
 * Build: see run_data_slus_rodata_retail_test.sh (O0 / O2 / clang-UBSan +
 * mutants).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psx_memory.h"

/* The data file's headers only need the PSX_RAM_SIZE constant; provide the
 * storage anyway so the test also links if a future object grows a PSX_ADDR. */
uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

#ifndef DATA_SLUS_RODATA_SRC
#define DATA_SLUS_RODATA_SRC "../src/data_slus_rodata.c"
#endif
#include DATA_SLUS_RODATA_SRC

#define EXE_HEADER_SIZE 0x800u
#define EXE_LOAD_BASE   0x80010000u
#define RODATA_END      0x80019524u

static unsigned s_checks;
static unsigned s_failures;

#define CHECK(cond, what, ...)                                             \
    do {                                                                   \
        s_checks++;                                                        \
        if (!(cond)) {                                                     \
            s_failures++;                                                  \
            fprintf(stderr, "FAIL %s: ", (what));                          \
            fprintf(stderr, __VA_ARGS__);                                  \
            fprintf(stderr, "\n");                                         \
        }                                                                  \
    } while (0)

struct RawSym {
    const char* name;
    unsigned vaddr;
    const unsigned char* host;
    size_t size;
    size_t want_size;
};

/* Every .rodata mismatch the data-stub parity audit lists as
 * "retail non-zero / host all-zero" that is not already handled elsewhere.
 * Sizes are the splat dlabel .size from asm/slus_006.64/data/<name>.rodata.s. */
static const struct RawSym RAW[] = {
    { "D_80010000", 0x80010000u, D_80010000, sizeof(D_80010000), 0x0004 },
    { "D_80010004", 0x80010004u, D_80010004, sizeof(D_80010004), 0x8000 },
    { "D_80018004", 0x80018004u, D_80018004, sizeof(D_80018004), 0x0080 },
    { "D_800180FC", 0x800180FCu, D_800180FC, sizeof(D_800180FC), 0x00B1 },
    { "D_800181B8", 0x800181B8u, D_800181B8, sizeof(D_800181B8), 0x0010 },
    { "D_800181C8", 0x800181C8u, D_800181C8, sizeof(D_800181C8), 0x0014 },
    { "D_800181DC", 0x800181DCu, D_800181DC, sizeof(D_800181DC), 0x000C },
    { "D_800181E8", 0x800181E8u, D_800181E8, sizeof(D_800181E8), 0x000A },
    { "D_800181F4", 0x800181F4u, D_800181F4, sizeof(D_800181F4), 0x000A },
    { "D_80018200", 0x80018200u, D_80018200, sizeof(D_80018200), 0x001B },
    { "D_8001821C", 0x8001821Cu, D_8001821C, sizeof(D_8001821C), 0x0002 },
    { "D_80018220", 0x80018220u, D_80018220, sizeof(D_80018220), 0x0004 },
    { "D_80018224", 0x80018224u, D_80018224, sizeof(D_80018224), 0x0013 },
    { "D_80018238", 0x80018238u, D_80018238, sizeof(D_80018238), 0x001D },
    { "D_80018258", 0x80018258u, D_80018258, sizeof(D_80018258), 0x0052 },
    { "D_8001833C", 0x8001833Cu, D_8001833C, sizeof(D_8001833C), 0x0014 },
    { "D_80018350", 0x80018350u, D_80018350, sizeof(D_80018350), 0x0012 },
    { "D_80018364", 0x80018364u, D_80018364, sizeof(D_80018364), 0x0013 },
    { "D_80018378", 0x80018378u, D_80018378, sizeof(D_80018378), 0x0015 },
    { "D_80018390", 0x80018390u, D_80018390, sizeof(D_80018390), 0x0013 },
    { "D_80018644", 0x80018644u, D_80018644, sizeof(D_80018644), 0x0020 },
    { "D_800188EC", 0x800188ECu, D_800188EC, sizeof(D_800188EC), 0x0004 },
    { "D_800188F0", 0x800188F0u, D_800188F0, sizeof(D_800188F0), 0x0004 },
    { "D_80018944", 0x80018944u, D_80018944, sizeof(D_80018944), 0x0010 },
    { "D_80018954", 0x80018954u, D_80018954, sizeof(D_80018954), 0x000D },
    { "D_80018964", 0x80018964u, D_80018964, sizeof(D_80018964), 0x000E },
    { "D_80018974", 0x80018974u, D_80018974, sizeof(D_80018974), 0x000A },
};

static size_t file_offset(unsigned vaddr)
{
    return (size_t)EXE_HEADER_SIZE + ((size_t)vaddr - (size_t)EXE_LOAD_BASE);
}

static uint32_t read_le32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint8_t* read_disc(size_t* out_size)
{
    static const char* const paths[] = {
        "disc/SLUS_006.64", "../disc/SLUS_006.64", "../../disc/SLUS_006.64",
    };
    FILE* f = NULL;
    long len;
    uint8_t* buf;
    size_t i;

    for (i = 0; i < sizeof(paths) / sizeof(paths[0]) && f == NULL; i++)
        f = fopen(paths[i], "rb");
    if (f == NULL)
        return NULL;
    if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) <= 0 ||
            fseek(f, 0, SEEK_SET) != 0) {
        fclose(f);
        return NULL;
    }
    buf = (uint8_t*)malloc((size_t)len);
    if (buf == NULL || fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf);
        fclose(f);
        return NULL;
    }
    fclose(f);
    *out_size = (size_t)len;
    return buf;
}

int main(void)
{
    uint8_t* image;
    size_t size = 0;
    size_t i, j;
    unsigned bytes = 0;

    /* 1. Loader .rodata boundary constants: the fix depends on these. */
    CHECK(PSX_EXE_LOAD_BASE == EXE_LOAD_BASE, "load_base", "0x%08x", EXE_LOAD_BASE);
    CHECK(PSX_EXE_RODATA_START == 0x80010000u, "rodata_start", "0x%08x",
          PSX_EXE_RODATA_START);
    CHECK(PSX_EXE_RODATA_END == RODATA_END, "rodata_end", "0x%08x (want 0x80019524)",
          PSX_EXE_RODATA_END);
    CHECK(PSX_EXE_TEXT_START == PSX_EXE_RODATA_END, "text_start", "0x%08x",
          PSX_EXE_TEXT_START);
    CHECK(PSX_EXE_SDATA_END == 0x800592C0u, "sdata_end", "0x%08x", PSX_EXE_SDATA_END);
    CHECK(PSX_EXE_SBSS_START == 0x800592C0u, "sbss_start", "0x%08x", PSX_EXE_SBSS_START);
    CHECK(PSX_EXE_IMAGE_END == 0x80059800u, "image_end", "0x%08x", PSX_EXE_IMAGE_END);
    CHECK(PSX_EXE_RODATA_END == 0x80019524u, "rodata_end_len",
          "rodata span 0x%x", PSX_EXE_RODATA_END - PSX_EXE_RODATA_START);

    /* 2. The three head objects tile [0x80010000, 0x80018084). */
    CHECK((unsigned)(0x80010000u + sizeof(D_80010000)) == 0x80010004u,
          "tile_10000", "0x%x", 0x80010000u + (unsigned)sizeof(D_80010000));
    CHECK(0x80010004u + (unsigned)sizeof(D_80010004) == 0x80018004u,
          "tile_10004", "0x%x", 0x80010004u + (unsigned)sizeof(D_80010004));
    CHECK(0x80018004u + (unsigned)sizeof(D_80018004) == 0x80018084u,
          "tile_18004", "0x%x", 0x80018004u + (unsigned)sizeof(D_80018004));

    image = read_disc(&size);
    if (image == NULL) {
        fprintf(stderr, "FAIL disc: disc/SLUS_006.64 not readable\n");
        return 1;
    }
    CHECK(size == 0x4A000u, "disc_size", "0x%zx", size);
    CHECK(memcmp(image, "PS-X EXE", 8) == 0, "disc_magic", "not a PS-X EXE");
    CHECK(read_le32(image + 0x18) == EXE_LOAD_BASE, "disc_t_addr", "0x%08x",
          read_le32(image + 0x18));
    CHECK(read_le32(image + 0x1C) == 0x49800u, "disc_t_size", "0x%08x",
          read_le32(image + 0x1C));

    /* 3. The mode word the whole ad-hoc patch was about. */
    CHECK(read_le32(image + file_offset(0x80010000u)) == 0xFFFFFFFFu, "disc_mode",
          "retail D_80010000 is 0x%08x", read_le32(image + file_offset(0x80010000u)));
    CHECK(*(const uint32_t*)(const void*)D_80010000 == 0xFFFFFFFFu, "host_mode",
          "0x%08x (want 0xffffffff)", *(const uint32_t*)(const void*)D_80010000);

    /* 4. Every defined symbol against the disc, plus size and non-zero. */
    for (i = 0; i < sizeof(RAW) / sizeof(RAW[0]); i++) {
        const struct RawSym* s = &RAW[i];
        const uint8_t* retail = image + file_offset(s->vaddr);
        int nonzero = 0;

        CHECK(s->size == s->want_size, "size", "%s is %zu bytes, want 0x%zx",
              s->name, s->size, s->want_size);
        CHECK(memcmp(s->host, retail, s->size) == 0, "raw", "%s @0x%08x (%zu bytes)",
              s->name, s->vaddr, s->size);
        for (j = 0; j < s->size; j++)
            if (s->host[j] != 0) { nonzero = 1; break; }
        CHECK(nonzero, "nonzero", "%s is all-zero in retail", s->name);
        CHECK(s->vaddr >= PSX_EXE_RODATA_START && s->vaddr + s->size <= RODATA_END,
              "in_rodata", "%s @0x%08x+0x%zx escapes rodata", s->name, s->vaddr,
              s->size);
        bytes += (unsigned)s->size;
    }

    free(image);

    if (s_failures != 0) {
        fprintf(stderr, "data_slus_rodata retail certificate: %u/%u checks failed\n",
                s_failures, s_checks);
        return 1;
    }
    printf("data_slus_rodata retail certificate PASS: %u symbols (%u bytes), "
           "%u checks\n",
           (unsigned)(sizeof(RAW) / sizeof(RAW[0])), bytes, s_checks);
    return 0;
}
