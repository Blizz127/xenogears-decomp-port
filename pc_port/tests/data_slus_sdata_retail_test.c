/*
 * data_slus_sdata_retail_test.c - retail-byte certificate for
 * pc_port/src/data_slus_sdata.c.
 *
 * The test includes the generated data file directly (override the include path
 * with -DDATA_SLUS_SDATA_SRC=... to test a mutant copy), then compares every
 * defined object against disc/SLUS_006.64:
 *   - raw byte arrays must memcmp equal to the retail image window;
 *   - pointer tables must hold host pointers whose guest address equals the
 *     retail 4-byte word at that slot (the tables are dereferenced by native
 *     code, so they cannot store raw guest pointers on a 64-bit host).
 *
 * It also pins the loader's .sdata/.sbss boundary constants that the same
 * change corrects (PSX_EXE_SDATA_END == PSX_EXE_SBSS_START == 0x800592C0), so
 * a regression that moves the boundary back to 0x800576E4 fails here.
 *
 * Build: see run_data_slus_sdata_retail_test.sh (O0 / O2 / UBSan + mutants).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psx_memory.h"

/* The data file's initializers reference g_PsxRam; provide the storage. */
uint8_t g_PsxRam[PSX_RAM_SIZE];
/* ...and the guest<->host mapping the retail-data loader uses for pointer
 * tables (psx_memory.c in the port). */
void* PsxMemory_GuestToHost(uint32_t guest) { return PSX_ADDR(guest); }
uint32_t PsxMemory_HostToGuest(const void* host) { return PsxMemory_GuestAddr(host); }
uint8_t g_PsxScratchpad[4096];

#ifndef DATA_SLUS_SDATA_SRC
#define DATA_SLUS_SDATA_SRC "../src/data_slus_sdata.c"
#endif
#include DATA_SLUS_SDATA_SRC

#define EXE_HEADER_SIZE 0x800u
#define EXE_LOAD_BASE   0x80010000u

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
};

struct PtrSym {
    const char* name;
    unsigned vaddr;
    void* const* host;
    size_t count;
    size_t want_count;
};

struct SizeCheck {
    const char* name;
    size_t got;
    size_t want;
};

/* Byte-exact objects (retail window [vaddr, vaddr+size) from the image). */
static const struct RawSym RAW[] = {
    { "D_8004F0C0",                0x8004F0C0u, D_8004F0C0,                sizeof(D_8004F0C0) },
    { "g_KernelMenuCurChoice",     0x8004F2D8u, g_KernelMenuCurChoice,     sizeof(g_KernelMenuCurChoice) },
    { "D_8004F304",                0x8004F304u, D_8004F304,                sizeof(D_8004F304) },
    { "D_8004F364",                0x8004F364u, D_8004F364,                sizeof(D_8004F364) },
    { "D_8004F384",                0x8004F384u, D_8004F384,                sizeof(D_8004F384) },
    { "D_8004FBB8",                0x8004FBB8u, D_8004FBB8,                sizeof(D_8004FBB8) },
    { "D_8004FD80",                0x8004FD80u, D_8004FD80,                sizeof(D_8004FD80) },
    { "D_8004FDA0",                0x8004FDA0u, D_8004FDA0,                sizeof(D_8004FDA0) },
    { "D_8004FE44",                0x8004FE44u, D_8004FE44,                sizeof(D_8004FE44) },
    { "D_8005010C",                0x8005010Cu, D_8005010C,                sizeof(D_8005010C) },
    { "D_800501D0",                0x800501D0u, D_800501D0,                sizeof(D_800501D0) },
    { "g_ControllerStickToAnalogX", 0x8005020Cu, g_ControllerStickToAnalogX, sizeof(g_ControllerStickToAnalogX) },
    { "g_ControllerStickToAnalogY", 0x8005021Cu, g_ControllerStickToAnalogY, sizeof(g_ControllerStickToAnalogY) },
    { "g_FontClutData",            0x80050598u, g_FontClutData,            sizeof(g_FontClutData) },
    { "D_8005061C",                0x8005061Cu, D_8005061C,                sizeof(D_8005061C) },
    { "D_8005061F",                0x8005061Fu, D_8005061F,                sizeof(D_8005061F) },
    { "D_80050620",                0x80050620u, D_80050620,                sizeof(D_80050620) },
    { "g_ReverbWorkAreaSizes",     0x800508E8u, g_ReverbWorkAreaSizes,     sizeof(g_ReverbWorkAreaSizes) },
    { "D_80050910",                0x80050910u, D_80050910,                sizeof(D_80050910) },
    { "D_80050924",                0x80050924u, D_80050924,                sizeof(D_80050924) },
    { "D_80050940",                0x80050940u, D_80050940,                sizeof(D_80050940) },
    { "D_80059171",                0x80059171u, D_80059171,                sizeof(D_80059171) },
    { "g_MenuDebugEnabled",        0x80059178u, g_MenuDebugEnabled,        sizeof(g_MenuDebugEnabled) },
    { "D_80059179",                0x80059179u, D_80059179,                sizeof(D_80059179) },
    { "D_800591A8",                0x800591A8u, D_800591A8,                sizeof(D_800591A8) },
    { "D_800591B8",                0x800591B8u, D_800591B8,                sizeof(D_800591B8) },
};

/* Pointer tables. Stored as host pointers into g_PsxRam; the guest address
 * recoverable from each entry must equal the retail word in the image. */
static const struct PtrSym PTR[] = {
    { "D_8004FA9C",             0x8004FA9Cu, D_8004FA9C,             sizeof(D_8004FA9C) / sizeof(D_8004FA9C[0]),             7 },
    { "D_80050110",             0x80050110u, D_80050110,             sizeof(D_80050110) / sizeof(D_80050110[0]),             12 },
    { "g_HeapContentTypeNames", 0x80050140u, g_HeapContentTypeNames, sizeof(g_HeapContentTypeNames) / sizeof(g_HeapContentTypeNames[0]), 20 },
};

/* Retail sizes (splat .size, except D_80050910 which consumers read as a
 * 0x20-byte SoundFile header). A truncation mutant fails here. */
static const struct SizeCheck SIZES[] = {
    { "D_8004F0C0",                sizeof(D_8004F0C0),                0x1FC },
    { "g_KernelMenuCurChoice",     sizeof(g_KernelMenuCurChoice),     0x01C },
    { "D_8004F304",                sizeof(D_8004F304),                0x004 },
    { "D_8004F364",                sizeof(D_8004F364),                0x004 },
    { "D_8004F384",                sizeof(D_8004F384),                0x004 },
    { "D_8004FBB8",                sizeof(D_8004FBB8),                0x020 },
    { "D_8004FD80",                sizeof(D_8004FD80),                0x020 },
    { "D_8004FDA0",                sizeof(D_8004FDA0),                0x020 },
    { "D_8004FE44",                sizeof(D_8004FE44),                0x001 },
    { "D_8005010C",                sizeof(D_8005010C),                0x004 },
    { "D_800501D0",                sizeof(D_800501D0),                0x018 },
    { "g_ControllerStickToAnalogX", sizeof(g_ControllerStickToAnalogX), 0x010 },
    { "g_ControllerStickToAnalogY", sizeof(g_ControllerStickToAnalogY), 0x010 },
    { "g_FontClutData",            sizeof(g_FontClutData),            0x080 },
    { "D_8005061C",                sizeof(D_8005061C),                0x001 },
    { "D_8005061F",                sizeof(D_8005061F),                0x001 },
    { "D_80050620",                sizeof(D_80050620),                0x001 },
    { "g_ReverbWorkAreaSizes",     sizeof(g_ReverbWorkAreaSizes),     0x028 },
    { "D_80050910",                sizeof(D_80050910),                0x020 },
    { "D_80050924",                sizeof(D_80050924),                0x01C },
    { "D_80050940",                sizeof(D_80050940),                0x070 },
    { "D_80059171",                sizeof(D_80059171),                0x007 },
    { "g_MenuDebugEnabled",        sizeof(g_MenuDebugEnabled),        0x004 },
    { "D_80059179",                sizeof(D_80059179),                0x003 },
    { "D_800591A8",                sizeof(D_800591A8),                0x004 },
    { "D_800591B8",                sizeof(D_800591B8),                0x00C },
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
    unsigned raw_bytes = 0;
    unsigned ptr_entries = 0;

    /* 1. Loader boundary constants: the whole point of the fix. */
    CHECK(PSX_EXE_LOAD_BASE == EXE_LOAD_BASE, "load_base", "0x%08x", EXE_LOAD_BASE);
    CHECK(PSX_EXE_SDATA_START == 0x8004EA90u, "sdata_start", "0x%08x", PSX_EXE_SDATA_START);
    CHECK(PSX_EXE_SDATA_END == 0x800592C0u, "sdata_end", "0x%08x (want 0x800592c0)",
          PSX_EXE_SDATA_END);
    CHECK(PSX_EXE_SBSS_START == 0x800592C0u, "sbss_start", "0x%08x (want 0x800592c0)",
          PSX_EXE_SBSS_START);
    CHECK(PSX_EXE_IMAGE_END == 0x80059800u, "image_end", "0x%08x", PSX_EXE_IMAGE_END);
    CHECK(PSX_EXE_SDATA_END - PSX_EXE_SDATA_START == 0xA830u, "sdata_len",
          "0x%x (want 0xa830)", PSX_EXE_SDATA_END - PSX_EXE_SDATA_START);
    /* The old 0x800576E4 end dropped the tail containing 0x8005919C..0x800592BB. */
    CHECK(PSX_EXE_SDATA_END > 0x800592BBu, "covers_last_nonzero",
          "sdata end 0x%08x excludes last non-zero byte 0x800592bb", PSX_EXE_SDATA_END);

    image = read_disc(&size);
    if (image == NULL) {
        fprintf(stderr, "FAIL disc: disc/SLUS_006.64 not readable\n");
        return 1;
    }
    CHECK(size == 0x4A000u, "disc_size", "0x%zx", size);
    CHECK(memcmp(image, "PS-X EXE", 8) == 0, "disc_magic", "not a PS-X EXE");
    /* t_addr/t_size from the PS-X EXE header. */
    CHECK(read_le32(image + 0x18) == EXE_LOAD_BASE, "disc_t_addr", "0x%08x",
          read_le32(image + 0x18));
    CHECK(read_le32(image + 0x1C) == 0x49800u, "disc_t_size", "0x%08x",
          read_le32(image + 0x1C));

    /* 2. Section-boundary corroboration from retail bytes. */
    CHECK(image[file_offset(0x800592BBu)] != 0, "last_nonzero",
          "0x800592bb is zero in retail");
    {
        unsigned k;
        int tail_zero = 1;
        for (k = 0x800592BCu; k < PSX_EXE_IMAGE_END; k++)
            if (image[file_offset(k)] != 0) { tail_zero = 0; break; }
        CHECK(tail_zero, "sbss_zero_in_image", "retail byte 0x%08x is non-zero", k);
    }

    /* 3. Every defined symbol against the disc. */
    for (i = 0; i < sizeof(RAW) / sizeof(RAW[0]); i++) {
        const struct RawSym* s = &RAW[i];
        const uint8_t* retail = image + file_offset(s->vaddr);
        CHECK(memcmp(s->host, retail, s->size) == 0, "raw", "%s @0x%08x (%zu bytes)",
              s->name, s->vaddr, s->size);
        raw_bytes += (unsigned)s->size;
    }
    for (i = 0; i < sizeof(PTR) / sizeof(PTR[0]); i++) {
        const struct PtrSym* s = &PTR[i];
        const uint8_t* retail = image + file_offset(s->vaddr);
        CHECK(s->count == s->want_count, "ptr_count", "%s has %zu entries, want %zu",
              s->name, s->count, s->want_count);
        for (j = 0; j < s->count; j++) {
            uint32_t want = read_le32(retail + j * 4u);
            uint32_t got = PsxMemory_GuestAddr(s->host[j]);
            CHECK(got == want, "ptr", "%s[%zu] guest 0x%08x want 0x%08x",
                  s->name, j, got, want);
            ptr_entries++;
        }
    }
    for (i = 0; i < sizeof(SIZES) / sizeof(SIZES[0]); i++) {
        CHECK(SIZES[i].got == SIZES[i].want, "size", "%s is %zu, want 0x%zx",
              SIZES[i].name, SIZES[i].got, SIZES[i].want);
    }

    free(image);

    if (s_failures != 0) {
        fprintf(stderr, "data_slus_sdata retail certificate: %u/%u checks failed\n",
                s_failures, s_checks);
        return 1;
    }
    printf("data_slus_sdata retail certificate PASS: %u raw symbols (%u bytes), "
           "%u pointer tables (%u entries), %u size checks, %u total assertions\n",
           (unsigned)(sizeof(RAW) / sizeof(RAW[0])), raw_bytes,
           (unsigned)(sizeof(PTR) / sizeof(PTR[0])), ptr_entries,
           (unsigned)(sizeof(SIZES) / sizeof(SIZES[0])), s_checks);
    return 0;
}
