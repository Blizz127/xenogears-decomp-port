/*
 * psx_memory.c - PSX main-RAM emulation for the Xenogears PC port.
 * See psx_memory.h. Mirrors the Silent Hill port's approach.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "psx_memory.h"

uint8_t g_PsxRam[PSX_RAM_SIZE];
uint8_t g_PsxScratchpad[4096];

void PsxMemory_Init(void)
{
    memset(g_PsxRam, 0, sizeof(g_PsxRam));
    memset(g_PsxScratchpad, 0, sizeof(g_PsxScratchpad));
    printf("[xeno-port] PSX RAM emulation: %d KB at %p\n",
           (int)(PSX_RAM_SIZE / 1024), (void*)g_PsxRam);
}

static uint32_t psx_exe_u32(const uint8_t* p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

/* Copy guest [start,end) from the image (file offset = header + vaddr - base). */
static int psx_exe_copy_range(const uint8_t* image, size_t size,
                              uint32_t start, uint32_t end)
{
    size_t off = PSX_EXE_HEADER_SIZE + (size_t)(start - PSX_EXE_LOAD_BASE);
    size_t len = (size_t)(end - start);
    if (off + len > size)
        return -1;
    memcpy(PSX_ADDR(start), image + off, len);
    return 0;
}

int PsxMemory_LoadStaticDataFromImage(const uint8_t* image, size_t size)
{
    uint32_t t_addr, t_size;
    uint32_t rodata_end = PSX_EXE_RODATA_END;
    uint32_t sdata_end = PSX_EXE_SDATA_END;

    if (image == NULL || size < PSX_EXE_HEADER_SIZE ||
            memcmp(image, "PS-X EXE", 8) != 0)
        return -1;
    t_addr = psx_exe_u32(image + 0x18);
    t_size = psx_exe_u32(image + 0x1C);
    if (t_addr != PSX_EXE_LOAD_BASE ||
            PSX_EXE_HEADER_SIZE + (size_t)t_size > size ||
            PSX_EXE_LOAD_BASE + t_size < PSX_EXE_SDATA_END)
        return -1;

#if defined(PSX_EXE_MUTANT_M5)          /* rodata short by one page */
    rodata_end -= 0x1000u;
#endif
#if defined(PSX_EXE_MUTANT_M4)          /* sdata extended into .sbss */
    sdata_end = PSX_EXE_IMAGE_END;
#endif
    if (psx_exe_copy_range(image, size, PSX_EXE_RODATA_START, rodata_end) != 0)
        return -1;
    if (psx_exe_copy_range(image, size, PSX_EXE_SDATA_START, sdata_end) != 0)
        return -1;
#if defined(PSX_EXE_MUTANT_M1)          /* .text copied as well */
    if (psx_exe_copy_range(image, size, PSX_EXE_TEXT_START, PSX_EXE_TEXT_END) != 0)
        return -1;
#endif
#if defined(PSX_EXE_MUTANT_M2)          /* ._49AC0 island copied as well */
    if (PSX_EXE_ISLAND_FILE + (PSX_EXE_ISLAND_END - PSX_EXE_ISLAND_START) <= size)
        memcpy(PSX_ADDR(PSX_EXE_ISLAND_START), image + PSX_EXE_ISLAND_FILE,
               PSX_EXE_ISLAND_END - PSX_EXE_ISLAND_START);
#endif
    return 0;
}

static uint8_t* psx_exe_read_file(const char* path, size_t* out_size)
{
    FILE* f = fopen(path, "rb");
    uint8_t* buf;
    long len;
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

/* Weak: the default backend is header-pulled by the port's disc.h users;
 * standalone tests that link this file alone skip it. */
extern int xg_plat_data_file_path(const char* name, char* out, size_t out_size)
    __attribute__((weak));

int PsxMemory_LoadStaticData(void)
{
    /* Same resolution order as the disc image in port_main.c (run from
     * pc_port/build_native, repo root, or pc_port). */
    static const char* const defaults[] = {
        "../../disc/SLUS_006.64", "disc/SLUS_006.64", "../disc/SLUS_006.64",
    };
    const char* env = getenv("XENO_SLUS");
    static char resolved[1024];
    const char* used = NULL;
    uint8_t* image = NULL;
    size_t size = 0;
    size_t i;
    int rc;

    if (env != NULL && env[0] != '\0') {
        image = psx_exe_read_file(env, &size);
        used = env;
    }
    /* The disc/storage layer's single search order (xg_plat/disc.h:
     * XENO_DATA_DIR, XENO_SLUS's directory, disc/ ../disc/ ../../disc/), the
     * same one that finds and verifies every other retail file.  Before this,
     * a run whose working directory was not the repo root or build_native
     * (XENO_DATA_DIR set) left main-exe static data zero-filled; battle's OT
     * sentinel 0x8005698C then read 0 and DrawOTag stopped (divergence D5). */
    if (image == NULL && xg_plat_data_file_path != NULL &&
        xg_plat_data_file_path("SLUS_006.64", resolved, sizeof resolved)) {
        image = psx_exe_read_file(resolved, &size);
        used = resolved;
    }
    for (i = 0; image == NULL && i < sizeof(defaults) / sizeof(defaults[0]); i++) {
        image = psx_exe_read_file(defaults[i], &size);
        used = defaults[i];
    }
    if (image == NULL) {
        fprintf(stderr,
                "[xeno-port] SLUS_006.64 not found (set XENO_SLUS or place "
                "disc/SLUS_006.64); main-exe static data stays zero-filled\n");
        return -1;
    }
    rc = PsxMemory_LoadStaticDataFromImage(image, size);
    if (rc != 0) {
        fprintf(stderr, "[xeno-port] %s is not a PS-X EXE for 0x%08x; "
                "main-exe static data stays zero-filled\n",
                used, (unsigned)PSX_EXE_LOAD_BASE);
    } else {
        printf("[xeno-port] main-exe static data loaded from %s: rodata "
               "[0x%08x,0x%08x) sdata [0x%08x,0x%08x) (file offset 0x800 + "
               "vaddr - 0x%08x); .text and ._49AC0 island not copied\n",
               used, (unsigned)PSX_EXE_RODATA_START, (unsigned)PSX_EXE_RODATA_END,
               (unsigned)PSX_EXE_SDATA_START, (unsigned)PSX_EXE_SDATA_END,
               (unsigned)PSX_EXE_LOAD_BASE);
    }
    free(image);
    return rc;
}

/* Guest <-> host mapping for the retail-data loader's pointer tables
 * (retail_data.h references these weakly). */
void* PsxMemory_GuestToHost(uint32_t guest)
{
    return PSX_ADDR(guest);
}

uint32_t PsxMemory_HostToGuest(const void* host)
{
    return PsxMemory_GuestAddr(host);
}
