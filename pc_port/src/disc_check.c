/* disc_check.c -- content check of the user's disc image.
 *
 * The port ships no game data (pc_port/THIRD_PARTY.md); it reads the user's
 * own Xenogears (USA) Disc 1 image.  Before the archive is opened the image
 * is compared with the Redump dump of that disc (redump.org/disc/177:
 * single MODE2/2352 track, 718738272 bytes, SHA-1 below).
 *
 * The retail files the native tables are loaded from (SLUS_006.64 and the
 * overlay .bin files) are checked separately against config/checksum.sha
 * by retail_data.h before main() runs.
 *
 * Unknown images stop startup before the archive opens.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifdef _WIN32
#include <io.h>
#include "krom_disc_win_hash.h"
#else
#include <unistd.h>
#include <openssl/evp.h>
#endif

#include "disc_check.h"
#include "quick_disc_index.h"
#include "host_thread.h"

#define DISC1_SIZE 718738272LL
#define DISC1_SHA1 "12db8ccb93516c391630f046a143762337cc21f4"

/* The SDK owns the open file and its seek position. Identify its regular
 * read-only descriptor before and after reading exactly the expected bytes. */
typedef struct DiscFingerprint {
    uint64_t device, file, size, writeTime, writeFraction, changeTime, changeFraction;
} DiscFingerprint;

#ifdef _WIN32
static int fingerprint(int fd, DiscFingerprint* identity)
{
    if (fd < 0) return 0;
    intptr_t raw = _get_osfhandle(fd);
    BY_HANDLE_FILE_INFORMATION info;
    FILE_BASIC_INFO times;
    if (raw == -1 || GetFileType((HANDLE)raw) != FILE_TYPE_DISK ||
        !GetFileInformationByHandle((HANDLE)raw, &info) ||
        (info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
        !GetFileInformationByHandleEx((HANDLE)raw, FileBasicInfo, &times, sizeof times))
        return 0;
    *identity = (DiscFingerprint){
        info.dwVolumeSerialNumber, (uint64_t)info.nFileIndexHigh << 32 | info.nFileIndexLow,
        (uint64_t)info.nFileSizeHigh << 32 | info.nFileSizeLow, times.LastWriteTime.QuadPart,
        0, times.ChangeTime.QuadPart, 0
    };
    return 1;
}
#else
static int fingerprint(int fd, DiscFingerprint* identity)
{
    struct stat st;
    int flags = fcntl(fd, F_GETFL);
    if (flags == -1 || (flags & O_ACCMODE) != O_RDONLY ||
        fstat(fd, &st) != 0 || !S_ISREG(st.st_mode))
        return 0;
    *identity = (DiscFingerprint){st.st_dev, st.st_ino, st.st_size, st.st_mtim.tv_sec,
                                 st.st_mtim.tv_nsec, st.st_ctim.tv_sec, st.st_ctim.tv_nsec};
    return 1;
}
#endif

static int verify_disc1_fd(int fd, PcPortDiscIndex* index, DiscFingerprint* identity)
{
    DiscFingerprint before, after;
    if (!fingerprint(fd, &before) || before.size != DISC1_SIZE) return 0;
    unsigned char buffer[64 * 1024], digest[20];
    uint64_t offset = 0;
#ifdef _WIN32
    /* A reopened handle has a private file position. Sharing read alone
     * excludes a writable original and prevents changes during the check. */
    HANDLE input = ReOpenFile((HANDLE)_get_osfhandle(fd), GENERIC_READ, FILE_SHARE_READ,
                              FILE_FLAG_SEQUENTIAL_SCAN);
    if (input == INVALID_HANDLE_VALUE) return 0;
    XenoWinHash ctx = {0};
    int ok = xeno_win_hash_init(&ctx, BCRYPT_SHA1_ALGORITHM);
#else
    unsigned int length = 0;
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    int ok = ctx && EVP_DigestInit_ex(ctx, EVP_sha1(), NULL) == 1;
#endif
    while (ok && offset < before.size) {
#ifdef _WIN32
        DWORD got = 0;
        if (!ReadFile(input, buffer, sizeof buffer, &got, NULL) || !got) {
            ok = 0;
            break;
        }
        ok = BCRYPT_SUCCESS(BCryptHashData(ctx.hash, buffer, got, 0));
#else
        ssize_t got = pread(fd, buffer, sizeof buffer, (off_t)offset);
        if (got < 0 && errno == EINTR) continue;
        if (got <= 0) {
            ok = 0;
            break;
        }
        ok = EVP_DigestUpdate(ctx, buffer, (size_t)got) == 1;
#endif
        PcPort_CaptureDiscIndex(index, offset, buffer, (size_t)got);
        offset += (size_t)got;
    }
    ok = ok && offset == before.size && fingerprint(fd, &after) &&
         !memcmp(&before, &after, sizeof before);
#ifdef _WIN32
    ok = ok && BCRYPT_SUCCESS(BCryptFinishHash(ctx.hash, digest, sizeof digest, 0));
    xeno_win_hash_close(&ctx);
    CloseHandle(input);
#else
    ok = ok && EVP_DigestFinal_ex(ctx, digest, &length) == 1 && length == sizeof digest;
    EVP_MD_CTX_free(ctx);
#endif
    if (!ok) return 0;
    char hex[41];
    for (unsigned i = 0; i < sizeof digest; ++i)
        snprintf(hex + 2 * i, 3, "%02x", (unsigned)digest[i]);
    if (strcmp(hex, DISC1_SHA1) != 0) return 0;
    if (identity) *identity = before;
    return 1;
}

int PcPort_VerifyMountedDisc1(int fd)
{
    return verify_disc1_fd(fd, NULL, NULL);
}

int PcPort_VerifyMountedDisc2(int fd)
{
    (void)fd;
    return 0;  /* This preview supports US Disc 1. */
}

int PcPort_VerifyDiscIndex(int fd, int disc, PcPortDiscIndex* out)
{
    PcPortDiscIndex index;
    if (disc != 1 || !out || !verify_disc1_fd(fd, &index, NULL) ||
        !PcPort_DiscIndexMatches(&index, 1))
        return 0;
    *out = index;
    return 1;
}

extern int PsyX_CdAuthorizeLid(int (*verify)(int));
extern uint32_t g_ArchiveHeader, g_ArchiveDebugTable;
extern unsigned char D_80018004[];

static HostMutex archiveProofLock = HOST_MUTEX_INITIALIZER;
static DiscFingerprint archiveFile;
static unsigned char archiveDirectory[122];
static int haveArchiveProof, currentFileMatches;

/* Invoked synchronously by PsyX_CdAuthorizeLid under its own cursor lock.
 * Returning zero leaves an ordinary game mount in its original mode. */
static int inspect_archive_file(int fd)
{
    PcPortDiscIndex index;
    if (verify_disc1_fd(fd, &index, &archiveFile) && PcPort_DiscIndexMatches(&index, 1)) {
        memcpy(archiveDirectory, index.sectors[17] + 24, sizeof archiveDirectory);
        haveArchiveProof = 1;
    }
    return 0;
}

static int inspect_current_identity(int fd)
{
    DiscFingerprint before, after;
    unsigned char directory[sizeof archiveDirectory];
    const uint64_t offset = 40u * 2352u + 24u;
    if (!fingerprint(fd, &before) || memcmp(&before, &archiveFile, sizeof before)) return 0;
#ifdef _WIN32
    HANDLE input = ReOpenFile((HANDLE)_get_osfhandle(fd), GENERIC_READ, FILE_SHARE_READ, 0);
    if (input == INVALID_HANDLE_VALUE) return 0;
    LARGE_INTEGER pos;
    DWORD got = 0;
    pos.QuadPart = offset;
    int ok = SetFilePointerEx(input, pos, NULL, FILE_BEGIN) &&
             ReadFile(input, directory, sizeof directory, &got, NULL) && got == sizeof directory;
#else
    ssize_t got;
    do {
        got = pread(fd, directory, sizeof directory, (off_t)offset);
    } while (got < 0 && errno == EINTR);
    int ok = got == sizeof directory;
#endif
    currentFileMatches = ok && fingerprint(fd, &after) && !memcmp(&before, &after, sizeof after) &&
                         !memcmp(directory, archiveDirectory, sizeof directory) &&
                         !memcmp(D_80018004, directory, sizeof directory);
#ifdef _WIN32
    CloseHandle(input);
#endif
    return 0;
}

void PcPort_VerifyBattleBootDisc(void)
{
    host_mutex_lock(&archiveProofLock);
    haveArchiveProof = 0;
    (void)PsyX_CdAuthorizeLid(inspect_archive_file);
    host_mutex_unlock(&archiveProofLock);
}

void PcPort_NotifyVerifiedDiscInsertion(int disc)
{
    /* The insertion argument alone cannot establish a new mount's identity.
     * Any later proof must observe the SDK's actual descriptor. */
    host_mutex_lock(&archiveProofLock);
    haveArchiveProof = 0;
    if (disc == 1)
        (void)PsyX_CdAuthorizeLid(inspect_archive_file);
    host_mutex_unlock(&archiveProofLock);
}

int PcPort_BattleArchiveDisc(void)
{
    int disc = 0;
    host_mutex_lock(&archiveProofLock);
    if (haveArchiveProof && !g_ArchiveDebugTable &&
        (uintptr_t)g_ArchiveHeader == (uintptr_t)D_80018004) {
        currentFileMatches = 0;
        (void)PsyX_CdAuthorizeLid(inspect_current_identity);
        if (currentFileMatches)
            disc = 1;
    }
    host_mutex_unlock(&archiveProofLock);
    return disc;
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
#ifndef _WIN32
    if (getenv("DISPLAY") == NULL && getenv("WAYLAND_DISPLAY") == NULL)
        return;
#endif
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
    char text[1024];
    snprintf(text, sizeof text,
             "%s: %s\n\nThe port contains no game data and needs your own "
             "Xenogears (USA) Disc 1 as a raw BIN image (MODE2/2352, one track).\n"
             "Expected (Redump, redump.org/disc/177): %lld bytes, sha1 %s.",
             path, problem, DISC1_SIZE, DISC1_SHA1);
    PcPort_UserNotice("Xenogears: disc image not usable", text);
}

int PcPort_VerifyDiscImage(const char* path)
{
    if (!path || !*path) {
        explain("(no image)", "missing image path");
        return -1;
    }
#ifdef _WIN32
    int fd = _open(path, _O_RDONLY | _O_BINARY | _O_NOINHERIT);
#else
    int fd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
#endif
    if (fd < 0) {
        explain(path, strerror(errno));
        return -1;
    }
    printf("[xeno-port] verifying disc image %s (sha1)...\n", path);
    fflush(stdout);
    int verified = PcPort_VerifyMountedDisc1(fd);
#ifdef _WIN32
    _close(fd);
#else
    close(fd);
#endif
    if (!verified) {
        explain(path, "image size, contents or read-only identity did not verify");
        return -1;
    }
    printf("[xeno-port] disc image OK: Xenogears (USA) Disc 1 (Redump sha1 %s)\n", DISC1_SHA1);
    return 0;
}
