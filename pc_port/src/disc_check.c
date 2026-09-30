/* disc_check.c -- first-run check of the user's disc image.
 *
 * The port ships no game data (pc_port/THIRD_PARTY.md); it reads the user's
 * own Xenogears (USA) Disc 1 image.  Before the archive is opened the image
 * is compared with the Redump dump of that disc (redump.org/disc/177:
 * single MODE2/2352 track, 718738272 bytes, SHA-1 below).  Hashing ~700 MB
 * takes a few seconds, so a successful check is remembered in
 * ${XDG_CACHE_HOME:-~/.cache}/xenogears-port/disc-verified, keyed by the
 * image's path, size and modification time; later runs skip the hash.
 *
 * The retail files the native tables are loaded from (SLUS_006.64 and the
 * overlay .bin files) are checked separately against config/checksum.sha
 * by retail_data.h before main() runs.
 *
 * A wrong image stops the port with an explanation; XENO_ALLOW_UNVERIFIED_DISC=1
 * continues anyway (e.g. for a differently-ripped but equivalent image).
 */
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <openssl/evp.h>

#include "disc_check.h"

#define DISC1_SIZE 718738272LL
#define DISC1_SHA1 "12db8ccb93516c391630f046a143762337cc21f4"

static int stamp_path(char* out, size_t n)
{
    const char* xdg = getenv("XDG_CACHE_HOME");
    const char* home = getenv("HOME");
    if (xdg && *xdg)
        return snprintf(out, n, "%s/xenogears-port/disc-verified", xdg) < (int)n;
    if (home && *home)
        return snprintf(out, n, "%s/.cache/xenogears-port/disc-verified", home) < (int)n;
    return 0;
}

static void stamp_key(char* out, size_t n, const char* path, const struct stat* st)
{
    char real[PATH_MAX];
    if (realpath(path, real) == NULL)
        snprintf(real, sizeof real, "%s", path);
    snprintf(out, n, "%s\t%lld\t%lld\t%s", real, (long long)st->st_size,
             (long long)st->st_mtime, DISC1_SHA1);
}

static int stamp_has(const char* key)
{
    char sp[PATH_MAX + 64], line[PATH_MAX + 256];
    FILE* f;
    int found = 0;
    if (!stamp_path(sp, sizeof sp) || (f = fopen(sp, "r")) == NULL)
        return 0;
    while (!found && fgets(line, sizeof line, f)) {
        line[strcspn(line, "\n")] = '\0';
        found = strcmp(line, key) == 0;
    }
    fclose(f);
    return found;
}

static void stamp_add(const char* key)
{
    char sp[PATH_MAX + 64], dir[PATH_MAX + 64];
    char* slash;
    FILE* f;
    if (!stamp_path(sp, sizeof sp))
        return;
    snprintf(dir, sizeof dir, "%s", sp);
    slash = strrchr(dir, '/');
    if (slash) {
        *slash = '\0';
        (void)mkdir(dir, 0755); /* parent ~/.cache normally exists */
    }
    if ((f = fopen(sp, "a")) != NULL) {
        fprintf(f, "%s\n", key);
        fclose(f);
    }
}

static int sha1_file(const char* path, char hex[41])
{
    static unsigned char buf[1 << 20];
    unsigned char md[EVP_MAX_MD_SIZE];
    unsigned int mdlen = 0, i;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    FILE* f = fopen(path, "rb");
    size_t got;
    int ok = ctx != NULL && f != NULL && EVP_DigestInit_ex(ctx, EVP_sha1(), NULL) == 1;
    while (ok && (got = fread(buf, 1, sizeof buf, f)) > 0)
        ok = EVP_DigestUpdate(ctx, buf, got) == 1;
    if (ok)
        ok = !ferror(f) && EVP_DigestFinal_ex(ctx, md, &mdlen) == 1;
    if (f)
        fclose(f);
    EVP_MD_CTX_free(ctx);
    if (!ok)
        return 0;
    for (i = 0; i < mdlen && i < 20; i++)
        sprintf(hex + 2 * i, "%02x", md[i]);
    hex[40] = '\0';
    return 1;
}

static int allow_unverified(void)
{
    const char* v = getenv("XENO_ALLOW_UNVERIFIED_DISC");
    return v && v[0] == '1';
}

/* Port sources build without SDL's include path; these two SDL2 entry points
 * are declared here as in SDL_messagebox.h / SDL_error.h (the binary links
 * libSDL2).  SDL_MESSAGEBOX_ERROR is 0x10; the parent window is SDL_Window*. */
extern int SDL_ShowSimpleMessageBox(unsigned int flags, const char* title,
                                    const char* message, void* window);
extern const char* SDL_GetError(void);
#define XENO_SDL_MESSAGEBOX_ERROR 0x00000010u

/* A launcher or gamescope session has no terminal, so a check that stops
 * the port also says why in a message box.  SDL_ShowSimpleMessageBox works
 * before SDL_Init.  Skipped for headless runs (no display, or the dummy and
 * offscreen video drivers) and with XENO_NO_DIALOG=1. */
void PcPort_UserNotice(const char* title, const char* text)
{
    const char* no = getenv("XENO_NO_DIALOG");
    const char* drv = getenv("SDL_VIDEODRIVER");
    if (no != NULL && no[0] == '1')
        return;
    if (drv != NULL && (strcmp(drv, "dummy") == 0 || strcmp(drv, "offscreen") == 0))
        return;
    if (getenv("DISPLAY") == NULL && getenv("WAYLAND_DISPLAY") == NULL)
        return;
    if (SDL_ShowSimpleMessageBox(XENO_SDL_MESSAGEBOX_ERROR, title, text, NULL) != 0)
        fprintf(stderr, "[xeno-port] message box failed: %s\n", SDL_GetError());
}

/* Strong override of retail_data.h's silent weak default. */
void XenoRetailData_Notice(const char* title, const char* text)
{
    PcPort_UserNotice(title, text);
}

static void explain(const char* path, const char* problem)
{
    fprintf(stderr,
            "\n[xeno-port] ===== disc image check failed =====\n"
            "[xeno-port] %s: %s\n"
            "[xeno-port] The port contains no game data and needs YOUR Xenogears (USA)\n"
            "[xeno-port] Disc 1 (SLUS-00664) as a raw BIN image (MODE2/2352, one track),\n"
            "[xeno-port] e.g. disc/disc1.bin + disc/disc1.cue or XENO_DISC=/path/to.bin.\n"
            "[xeno-port] Expected (Redump, redump.org/disc/177): %lld bytes, sha1 %s.\n",
            path, problem, DISC1_SIZE, DISC1_SHA1);
    if (allow_unverified()) {
        fprintf(stderr, "[xeno-port] XENO_ALLOW_UNVERIFIED_DISC=1: continuing anyway.\n\n");
    } else {
        char text[1024];
        fprintf(stderr, "[xeno-port] (XENO_ALLOW_UNVERIFIED_DISC=1 continues anyway)\n\n");
        snprintf(text, sizeof text,
                 "%s: %s\n\nThe port contains no game data and needs your own "
                 "Xenogears (USA) Disc 1 as a raw BIN image (MODE2/2352, one track).\n"
                 "Expected (Redump, redump.org/disc/177): %lld bytes, sha1 %s.",
                 path, problem, DISC1_SIZE, DISC1_SHA1);
        PcPort_UserNotice("Xenogears: disc image not usable", text);
    }
}

int PcPort_VerifyDiscImage(const char* path)
{
    struct stat st;
    char key[PATH_MAX + 128];
    char hex[41];
    char problem[160];

    if (stat(path, &st) != 0) {
        snprintf(problem, sizeof problem, "cannot stat (%s)", strerror(errno));
        explain(path, problem);
        return allow_unverified() ? 0 : -1;
    }
    stamp_key(key, sizeof key, path, &st);
    if (stamp_has(key)) {
        printf("[xeno-port] disc image verified earlier (Xenogears USA Disc 1): %s\n", path);
        return 0;
    }
    if ((long long)st.st_size != DISC1_SIZE) {
        snprintf(problem, sizeof problem,
                 "%lld bytes -- not a raw Xenogears (USA) Disc 1 image "
                 "(wrong disc/region, ISO/2048, or compressed)", (long long)st.st_size);
        explain(path, problem);
        return allow_unverified() ? 0 : -1;
    }
    printf("[xeno-port] first run: verifying disc image %s (sha1, once)...\n", path);
    fflush(stdout);
    if (!sha1_file(path, hex)) {
        explain(path, "read error while hashing");
        return allow_unverified() ? 0 : -1;
    }
    if (strcmp(hex, DISC1_SHA1) != 0) {
        snprintf(problem, sizeof problem, "sha1 %s -- not the Redump Xenogears (USA) Disc 1 "
                 "image (modified, patched or damaged)", hex);
        explain(path, problem);
        return allow_unverified() ? 0 : -1;
    }
    stamp_add(key);
    printf("[xeno-port] disc image OK: Xenogears (USA) Disc 1 (Redump sha1 %s)\n", hex);
    return 0;
}
