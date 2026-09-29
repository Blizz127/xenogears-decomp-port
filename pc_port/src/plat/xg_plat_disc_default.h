/* Default (PsyCross-era, host stdio) backend of xg_plat/disc.h.  Weak
 * definitions: a stronger backend overrides any of them.  Retail-file
 * resolution and hash checks are retail_data.h's; the Redump image check is
 * disc_check.c's (optional at link time). */
#ifndef XG_PLAT_DISC_DEFAULT_H
#define XG_PLAT_DISC_DEFAULT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../retail_data.h"

#define XG_PLAT_WEAK __attribute__((weak))

int PcPort_VerifyDiscImage(const char* path) __attribute__((weak));

XG_PLAT_WEAK const char* xg_plat_disc_image_path(void)
{
    static const char* const defaults[] = {
        "disc/disc1.bin", "../disc/disc1.bin", "../../disc/disc1.bin",
    };
    static int resolved;
    static const char* path;
    const char* env;
    unsigned i;
    FILE* f;

    if (resolved)
        return path;
    resolved = 1;
    env = getenv("XENO_DISC");
    if (env && env[0] && (f = fopen(env, "rb")) != NULL) {
        fclose(f);
        return path = env;
    }
    for (i = 0; i < sizeof(defaults) / sizeof(defaults[0]); i++) {
        if ((f = fopen(defaults[i], "rb")) != NULL) {
            fclose(f);
            return path = defaults[i];
        }
    }
    /* An installed port started from another directory: disc/ beside the
     * executable, or the image next to it. */
    {
        static char beside[1100];
        char dir[1024];
        if (XenoRetailData_ExeDir(dir, sizeof dir)) {
            const char* const forms[] = { "%s/disc/disc1.bin", "%s/disc1.bin" };
            for (i = 0; i < 2; i++) {
                snprintf(beside, sizeof beside, forms[i], dir);
                if ((f = fopen(beside, "rb")) != NULL) {
                    fclose(f);
                    return path = beside;
                }
            }
        }
    }
    return path = NULL;
}

XG_PLAT_WEAK int xg_plat_disc_verify(void)
{
    const char* path = xg_plat_disc_image_path();
    if (path == NULL || PcPort_VerifyDiscImage == NULL)
        return 0;
    return PcPort_VerifyDiscImage(path);
}

XG_PLAT_WEAK int xg_plat_disc_read_sectors(uint32_t lba, uint32_t count, void* out)
{
    const char* path = xg_plat_disc_image_path();
    struct stat st;
    long sec_size = XG_PLAT_SECTOR_USER, data_off = 0;
    unsigned char* dst = (unsigned char*)out;
    uint32_t i;
    FILE* f;

    if (path == NULL || stat(path, &st) != 0)
        return -1;
    /* raw BIN (2352-byte sectors, MODE2 form 1 user data at +24) or ISO */
    if (st.st_size % XG_PLAT_SECTOR_RAW == 0 && st.st_size / XG_PLAT_SECTOR_RAW > 1000) {
        sec_size = XG_PLAT_SECTOR_RAW;
        data_off = 24;
    }
    if ((f = fopen(path, "rb")) == NULL)
        return -1;
    for (i = 0; i < count; i++) {
        if (fseek(f, (long)(lba + i) * sec_size + data_off, SEEK_SET) != 0 ||
            fread(dst + (size_t)i * XG_PLAT_SECTOR_USER, 1, XG_PLAT_SECTOR_USER, f) !=
                XG_PLAT_SECTOR_USER) {
            fclose(f);
            return -1;
        }
    }
    fclose(f);
    return 0;
}

XG_PLAT_WEAK int xg_plat_data_file_path(const char* name, char* out, size_t out_size)
{
    return XenoRetailData_Resolve(name, out, out_size, NULL, 0);
}

XG_PLAT_WEAK const uint8_t* xg_plat_data_file(const char* name, size_t* size)
{
    int file;
    for (file = 0; file < XENO_RD_FILE_COUNT; file++) {
        const XenoRetailFileInfo* fi = XenoRetailData_FileInfo(file);
        if (fi && strcmp(fi->name, name) == 0) {
            XenoRetailFileSlot* slot = XenoRetailData_File(file);
            if (slot == NULL)
                return NULL;
            if (size)
                *size = slot->size;
            return slot->bytes;
        }
    }
    fprintf(stderr, "[xg_plat] unknown retail file '%s'\n", name);
    return NULL;
}

#endif /* XG_PLAT_DISC_DEFAULT_H */
