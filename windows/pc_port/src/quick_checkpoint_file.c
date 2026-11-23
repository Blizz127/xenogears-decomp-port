#include "quick_checkpoint_file.h"
#include "save_file_io.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <io.h>
#else
#include <sys/file.h>
#endif
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define CHECKPOINT_VERSION 1u

/* The final two v1 header bytes are checksummed and were originally zero. */
#define WALKMESH_PRESENT 0x8000u
#define WALKMESH_LAYER_SHIFT 13u
#define WALKMESH_TRIANGLE_MASK 0x1FFFu

static const uint8_t kCheckpointMagic[8] = {
    'X', 'G', 'Q', 'C', 'K', 'P', 'T', 0
};

static void put_u16(uint8_t* dst, uint16_t value)
{
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
}

static void put_u32(uint8_t* dst, uint32_t value)
{
    dst[0] = (uint8_t)value;
    dst[1] = (uint8_t)(value >> 8);
    dst[2] = (uint8_t)(value >> 16);
    dst[3] = (uint8_t)(value >> 24);
}

static uint16_t get_u16(const uint8_t* src)
{
    return (uint16_t)((uint16_t)src[0] | ((uint16_t)src[1] << 8));
}

static uint32_t get_u32(const uint8_t* src)
{
    return (uint32_t)src[0] |
           ((uint32_t)src[1] << 8) |
           ((uint32_t)src[2] << 16) |
           ((uint32_t)src[3] << 24);
}

static uint32_t checksum_bytes(uint32_t hash, const uint8_t* bytes,
                               size_t size)
{
    size_t i;
    for (i = 0; i < size; ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

/* Rand seed sidecar "<path>.rng": "RNGS", seed, the main file's checksum,
 * check value. A separate file keeps the main checkpoint format unchanged, so
 * older builds still load new saves; the checksum ties the sidecar to the
 * exact checkpoint it was written with (a stale one is ignored). */
#define COMPANION_BYTES 16u
/* GTL2, primary checksum, C8..C20, checksum bound to this slot tuple. */
#define LIGHT_BYTES (12u + 4u * PC_PORT_QUICK_LIGHTING_WORDS)
/* CAM1, primary checksum, heading, Q16 eye/at, bound checksum. */
#define CAMERA_BYTES 40u
#define CAMERA_MAX_HEADING 0xfffu
/* PRT1, primary checksum, payload length, payload, tuple checksum. */
#define PARTICLE_OVERHEAD 16u
/* Field record tag, primary checksum, field data, tuple checksum. */
#define ACTOR_BYTES (12u + PC_PORT_QUICK_CAVE_BYTES)
#define BART_BYTES (12u + PC_PORT_QUICK_BART_BYTES)
static const char* const actor_tags[] = {
    [PC_PORT_QUICK_ACTORS_CAVE] = "ACT1",
    [PC_PORT_QUICK_ACTORS_ESTATE] = "EST1",
    [PC_PORT_QUICK_ACTORS_BAL] = "BAL1",
    [PC_PORT_QUICK_ACTORS_FOLLOWER] = "FLW1"
};
/* MUS1, primary checksum, wait byte, reserved byte, ticks, fade value/step, tuple checksum. */
#define MUSIC_BYTES 24u
static const uint8_t kSeedTag[4] = { 'R', 'N', 'G', 'S' };

static uint32_t record_check(const uint8_t* sidecar)
{
    return checksum_bytes(2166136261u, sidecar, 12);
}

/* Older writers can change companions while writing byte-identical primary
 * data. Bind each newer record to the validated records its predecessors
 * knew. Each absent record contributes its tag and a zero presence byte. */
static uint32_t include_companion(uint32_t hash, const char tag[4],
                                  const uint8_t* record, size_t size)
{
    uint8_t present = record != NULL;
    hash = checksum_bytes(hash, (const uint8_t*)tag, 4);
    hash = checksum_bytes(hash, &present, 1);
    return record ? checksum_bytes(hash, record, size) : hash;
}

static uint32_t field_record_checksum(const uint8_t* data, size_t size,
                                      const uint8_t* rng, const uint8_t* timer)
{
    uint32_t hash = checksum_bytes(2166136261u, data, size - 4);
    hash = include_companion(hash, "RNGS", rng, COMPANION_BYTES);
    return include_companion(hash, "TIME", timer, COMPANION_BYTES);
}

static uint32_t camera_checksum(const uint8_t* camera, const uint8_t* rng,
                                const uint8_t* timer, const uint8_t* light)
{
    uint32_t hash = field_record_checksum(camera, CAMERA_BYTES, rng, timer);
    return include_companion(hash, "GTL2", light, LIGHT_BYTES);
}

static uint32_t camera_tuple_checksum(const uint8_t* record, size_t size, const uint8_t* rng,
                                      const uint8_t* timer, const uint8_t* light, const uint8_t* camera)
{
    uint32_t hash = field_record_checksum(record, size, rng, timer);
    hash = include_companion(hash, "GTL2", light, LIGHT_BYTES);
    return include_companion(hash, "CAM1", camera, CAMERA_BYTES);
}

/* All participants in a slot use one permanent lock inode. Saves copied from
 * prior versions can be loaded without creating anything in their directory. */
enum { RNG_FILE, TIME_FILE, LIGHT_FILE, CAMERA_FILE, PARTICLE_FILE, ACTOR_FILE, MUSIC_FILE, BART_FILE, COMPANION_COUNT };
static const size_t fixed_companion_sizes[COMPANION_COUNT] = {
    COMPANION_BYTES, COMPANION_BYTES, LIGHT_BYTES, CAMERA_BYTES, 0, ACTOR_BYTES, MUSIC_BYTES, BART_BYTES
};

static uint32_t actor_tuple_checksum(const uint8_t* record, size_t size,
                               const uint8_t* const records[COMPANION_COUNT],
                               size_t particle_size)
{
    uint32_t hash = camera_tuple_checksum(record, size, records[RNG_FILE],
            records[TIME_FILE], records[LIGHT_FILE], records[CAMERA_FILE]);
    hash = include_companion(hash, "PRT1", records[PARTICLE_FILE], particle_size + PARTICLE_OVERHEAD);
    const uint8_t* actors = records[ACTOR_FILE];
    return include_companion(hash, actors ? (const char*)actors : "ACT1", actors, ACTOR_BYTES);
}

/* Older writers zero the final two alignment bytes of the merchant pose.
 * A completed new save records its Bart witness there. The old reader
 * copies numeric fields and ignores these bytes, including after rollback. */
static uint16_t bart_witness(const uint8_t* payload)
{
    uint32_t hash = checksum_bytes(2166136261u, payload, PC_PORT_QUICK_BART_BYTES);
    return (uint16_t)(1u + hash % 65535u);
}

static uint32_t bart_checksum(const uint8_t* bart, const uint8_t* const records[COMPANION_COUNT],
                              size_t particle_size)
{
    uint32_t hash = actor_tuple_checksum(bart, BART_BYTES, records, particle_size);
    return include_companion(hash, "MUS1", records[MUSIC_FILE], MUSIC_BYTES);
}

typedef struct {
    const char* primary;
    char side[COMPANION_COUNT][PATH_MAX];
    char lock[PATH_MAX];
    char journal[PATH_MAX];
} CheckpointSlot;

typedef struct {
    char primary[PATH_MAX];
    char next[COMPANION_COUNT][PATH_MAX];
    char previous[COMPANION_COUNT][PATH_MAX];
    char undo[PATH_MAX];
    char prepared[PATH_MAX];
} CheckpointJournal;

static int append_path(char out[PATH_MAX], const char* base, const char* suffix)
{
    int n = snprintf(out, PATH_MAX, "%s%s", base, suffix);
    return n >= 0 && n < PATH_MAX ? 0 : -1;
}

static int slot_paths(CheckpointSlot* slot, const char* path)
{
    slot->primary = path;
    return path && *path &&
           !append_path(slot->side[RNG_FILE], path, ".rng") &&
           !append_path(slot->side[TIME_FILE], path, ".time") &&
           !append_path(slot->side[LIGHT_FILE], path, ".light") &&
           !append_path(slot->side[CAMERA_FILE], path, ".camera") &&
           !append_path(slot->side[PARTICLE_FILE], path, ".particles") &&
           !append_path(slot->side[ACTOR_FILE], path, ".actors") &&
           !append_path(slot->side[MUSIC_FILE], path, ".music") &&
           !append_path(slot->side[BART_FILE], path, ".bart") &&
           !append_path(slot->lock, path, ".lock") &&
           !append_path(slot->journal, path, ".txn") ? 0 : -1;
}

static int journal_paths(CheckpointJournal* j, const char* dir)
{
    return !append_path(j->primary, dir, "/primary.tmp") &&
           !append_path(j->next[RNG_FILE], dir, "/seed.rng.tmp") &&
           !append_path(j->next[TIME_FILE], dir, "/clock.time.tmp") &&
           !append_path(j->previous[RNG_FILE], dir, "/prev.rng") &&
           !append_path(j->previous[TIME_FILE], dir, "/prev.time") &&
           !append_path(j->next[LIGHT_FILE], dir, "/field.light.tmp") &&
           !append_path(j->previous[LIGHT_FILE], dir, "/prev.light") &&
           !append_path(j->next[CAMERA_FILE], dir, "/field.camera.tmp") &&
           !append_path(j->previous[CAMERA_FILE], dir, "/prev.camera") &&
           !append_path(j->next[PARTICLE_FILE], dir, "/field.particles.tmp") &&
           !append_path(j->previous[PARTICLE_FILE], dir, "/prev.particles") &&
           !append_path(j->next[ACTOR_FILE], dir, "/field.actors.tmp") &&
           !append_path(j->previous[ACTOR_FILE], dir, "/prev.actors") &&
           !append_path(j->next[MUSIC_FILE], dir, "/field.music.tmp") &&
           !append_path(j->previous[MUSIC_FILE], dir, "/prev.music") &&
           !append_path(j->next[BART_FILE], dir, "/field.bart.tmp") &&
           !append_path(j->previous[BART_FILE], dir, "/prev.bart") &&
           !append_path(j->undo, dir, "/undo.tmp") &&
           !append_path(j->prepared, dir, "/prepared") ? 0 : -1;
}

/* A missing entry is distinct from a failed stat or an unexpected file type. */
static int entry_exists(const char* path, mode_t type)
{
#ifdef _WIN32
    DWORD attrs = GetFileAttributesA(path);
    if (attrs == INVALID_FILE_ATTRIBUTES) {
        DWORD error = GetLastError();
        return error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND ? 0 : -1;
    }
    if (attrs & (FILE_ATTRIBUTE_REPARSE_POINT | FILE_ATTRIBUTE_DEVICE))
        return -1;
    return ((attrs & FILE_ATTRIBUTE_DIRECTORY) ? S_IFDIR : S_IFREG) == type ? 1 : -1;
#else
    struct stat info;
    if (lstat(path, &info) == 0)
        return (info.st_mode & S_IFMT) == type ? 1 : -1;
    return errno == ENOENT ? 0 : -1;
#endif
}

static int lock_slot(const char* path, int writer)
{
#ifdef _WIN32
    HANDLE handle = CreateFileA(path, writer ? GENERIC_READ | GENERIC_WRITE : GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, writer ? OPEN_ALWAYS : OPEN_EXISTING,
        FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        return !writer && (error == ERROR_FILE_NOT_FOUND || error == ERROR_PATH_NOT_FOUND) ? -2 : -1;
    }
    BY_HANDLE_FILE_INFORMATION info;
    OVERLAPPED position = {0};
    if (GetFileType(handle) == FILE_TYPE_DISK && GetFileInformationByHandle(handle, &info) &&
        !(info.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT)) &&
        LockFileEx(handle, writer ? LOCKFILE_EXCLUSIVE_LOCK : 0, 0, 1, 0, &position)) {
        /* Closing this descriptor releases the byte-range lock and handle. */
        int fd = _open_osfhandle((intptr_t)handle, _O_BINARY | _O_NOINHERIT |
                                                  (writer ? _O_RDWR : _O_RDONLY));
        if (fd >= 0)
            return fd;
    }
    CloseHandle(handle);
    return -1;
#else
    int flags = writer ? O_CREAT | O_RDWR : O_RDONLY;
    int fd = open(path, flags | O_NOFOLLOW | O_CLOEXEC | O_NONBLOCK, 0666);
    if (fd < 0)
        return !writer && errno == ENOENT ? -2 : -1;
    struct stat info;
    if (fstat(fd, &info) == 0 && S_ISREG(info.st_mode)) {
        int result;
        do {
            result = flock(fd, writer ? LOCK_EX : LOCK_SH);
        } while (result < 0 && errno == EINTR);
        if (result == 0)
            return fd;
    }
    close(fd);
    return -1;
#endif
}

/* A legacy file may be a readable alias. A pipe or device cannot supply a
 * finite checkpoint and must never block the game while it holds the lock. */
static FILE* open_regular_input(const char* path)
{
#ifdef _WIN32
    HANDLE handle = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE |
        FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE)
        return NULL;
    if (GetFileType(handle) != FILE_TYPE_DISK) {
        CloseHandle(handle);
        return NULL;
    }
    int fd = _open_osfhandle((intptr_t)handle, _O_RDONLY | _O_BINARY | _O_NOINHERIT);
    if (fd < 0) {
        CloseHandle(handle);
        return NULL;
    }
    FILE* file = _fdopen(fd, "rb");
    if (file)
        return file;
#else
    int fd = open(path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0)
        return NULL;
    struct stat info;
    if (fstat(fd, &info) == 0 && S_ISREG(info.st_mode)) {
        FILE* file = fdopen(fd, "rb");
        if (file)
            return file;
    }
#endif
    close(fd);
    return NULL;
}

/* Bad record lengths are valid legacy inputs. An I/O failure when reading
 * a required journal snapshot must instead fail the whole operation. */
static int read_exact(const char* path, uint8_t* bytes, size_t size)
{
    FILE* file = open_regular_input(path);
    if (!file)
        return -1;
    int complete = fread(bytes, 1, size, file) == size && fgetc(file) == EOF;
    int failed = ferror(file);
    return fclose(file) || failed ? -1 : complete;
}

/* Variable-length optional record. A truncation, oversized record or trailing
 * bytes invalidates only this companion. Preserve I/O error semantics for a
 * journal entry that the transaction reader is required to read. */
static int read_particles(const char* path, uint8_t* bytes, size_t* size)
{
    FILE* file = open_regular_input(path);
    if (!file)
        return -1;
    size_t count = fread(bytes, 1, 12, file);
    uint32_t payload = count == 12 ? get_u32(bytes + 8) : 0;
    int valid = payload > 0 && payload <= PC_PORT_QUICK_EFFECT_BYTES &&
                fread(bytes + 12, 1, payload + 4, file) == payload + 4 &&
                fgetc(file) == EOF;
    int error = ferror(file);
    if (fclose(file) || error)
        return -1;
    if (valid)
        *size = payload;
    return valid;
}

static int finish_write(FILE* file, int complete)
{
    int flushed = fflush(file) == 0;
    return fclose(file) == 0 && flushed && complete ? 0 : -1;
}

static int write_record(const char* path, const uint8_t* data, size_t size)
{
    FILE* file = fopen(path, "wb");
    return file ? finish_write(file, fwrite(data, 1, size, file) == size) : -1;
}

/* Exact snapshots also preserve malformed optional records on a failed save.
 * Copying instead of hard-linking allows quicksaves on removable filesystems. */
static int copy_record(const char* from, const char* to)
{
    FILE* input = open_regular_input(from);
    if (!input)
        return -1;
    FILE* output = fopen(to, "wb");
    if (!output) {
        fclose(input);
        return -1;
    }
    uint8_t buffer[4096];
    size_t n;
    int complete = 1;
    while ((n = fread(buffer, 1, sizeof buffer, input)) != 0) {
        if (fwrite(buffer, 1, n, output) != n) {
            complete = 0;
            break;
        }
    }
    complete = complete && !ferror(input);
#ifndef _WIN32
    struct stat info;
    complete = complete && fstat(fileno(input), &info) == 0 &&
               fchmod(fileno(output), info.st_mode & 0777) == 0;
#endif
    int closed = fclose(input) == 0;
    return finish_write(output, complete && closed);
}

#ifdef _WIN32
/* Moving a directory onto an existing empty directory fails on Windows. A
 * retirement instead claims an unused name with the move itself. */
static int private_staging_path(char dir[PATH_MAX], const char* slot, const char* retire)
{
    static LONG sequence;
    const char* slash = strrchr(slot, '/');
    const char* backslash = strrchr(slot, '\\');
    if (backslash && (!slash || backslash > slash))
        slash = backslash;
    size_t parent = slash ? (size_t)(slash - slot + 1) : (slot[1] == ':' ? 2 : 0);
    memcpy(dir, slot, parent);
    for (unsigned tries = 0; tries < 4096; ++tries) {
        int n = snprintf(dir + parent, PATH_MAX - parent, ".xeno-quicksave-%08lx-%08lx",
                         (unsigned long)GetCurrentProcessId(),
                         (unsigned long)InterlockedIncrement(&sequence));
        if (n < 0 || (size_t)n >= PATH_MAX - parent)
            return -1;
        if (retire ? MoveFileExA(retire, dir, MOVEFILE_WRITE_THROUGH)
                   : CreateDirectoryA(dir, NULL))
            return 0;
        DWORD error = GetLastError();
        if (error != ERROR_ALREADY_EXISTS && error != ERROR_FILE_EXISTS)
            return -1;
    }
    return -1;
}
#endif

static int staging_dir(char dir[PATH_MAX], const char* slot)
{
#ifdef _WIN32
    return private_staging_path(dir, slot, NULL);
#else
    const char* slash = strrchr(slot, '/');
    size_t parent = slash ? (size_t)(slash - slot + 1) : 0;
    static const char name[] = ".xeno-quicksave-XXXXXX";
    if (parent + sizeof name > PATH_MAX)
        return -1;
    memcpy(dir, slot, parent);
    memcpy(dir + parent, name, sizeof name);
    return mkdtemp(dir) ? 0 : -1;
#endif
}

static void discard_staging(const char* dir, const CheckpointJournal* j)
{
    unlink(j->primary);
    for (int i = 0; i < COMPANION_COUNT; ++i) {
        unlink(j->next[i]);
        unlink(j->previous[i]);
    }
    unlink(j->undo);
    unlink(j->prepared);
    rmdir(dir);
}

static int remove_if_present(const char* path)
{
    return unlink(path) == 0 || errno == ENOENT ? 0 : -1;
}

/* A prepared journal has a complete, immutable set of previous companions.
 * While primary.tmp exists, the previous primary is still at its public
 * path. Renaming primary.tmp onto it commits even a byte-identical primary. */
static int journal_state(const CheckpointJournal* j, unsigned* previous, int* count)
{
    uint8_t marker[5];
    if (entry_exists(j->prepared, S_IFREG) != 1 ||
        read_exact(j->prepared, marker, sizeof marker) != 1)
        return -1;
    /* Older journal versions only staged the companions they understood. */
    if (memcmp(marker, "XGQ1", 4) == 0)
        *count = TIME_FILE + 1;
    else if (memcmp(marker, "XGQ2", 4) == 0)
        *count = LIGHT_FILE + 1;
    else if (memcmp(marker, "XGQ3", 4) == 0)
        *count = CAMERA_FILE + 1;
    else if (memcmp(marker, "XGQ4", 4) == 0)
        *count = PARTICLE_FILE + 1;
    else if (memcmp(marker, "XGQ5", 4) == 0)
        *count = ACTOR_FILE + 1;
    else if (memcmp(marker, "XGQ6", 4) == 0)
        *count = MUSIC_FILE + 1;
    else if (memcmp(marker, "XGQ7", 4) == 0)
        *count = BART_FILE + 1;
    else
        return -1;
    if (marker[4] >= (1u << *count))
        return -1;
    *previous = marker[4];
    for (int i = 0; i < *count; ++i)
        if (entry_exists(j->previous[i], S_IFREG) != (int)((*previous >> i) & 1u))
            return -1;
    return entry_exists(j->primary, S_IFREG);
}

static int restore_companions(const CheckpointSlot* slot, const CheckpointJournal* j,
                              unsigned previous, int count)
{
    for (int i = 0; i < count; ++i) {
        if (previous & (1u << i)) {
            if (remove_if_present(j->undo) || copy_record(j->previous[i], j->undo) ||
                replace_save_file(j->undo, slot->side[i]))
                return -1;
        } else if (remove_if_present(slot->side[i])) {
            return -1;
        }
    }
    return 0;
}

/* Retire by rename before deleting journal contents. Interruption during
 * cleanup can only leave an unused private staging directory. */
static int retire_journal(const CheckpointSlot* slot, CheckpointJournal* j)
{
    char dir[PATH_MAX];
#ifdef _WIN32
    if (private_staging_path(dir, slot->primary, slot->journal) || journal_paths(j, dir))
        return -1;
#else
    if (staging_dir(dir, slot->primary) != 0)
        return -1;
    if (journal_paths(j, dir) || rename(slot->journal, dir)) {
        rmdir(dir);
        return -1;
    }
#endif
    discard_staging(dir, j);
    return 0;
}

static int recover_journal(const CheckpointSlot* slot, CheckpointJournal* j)
{
    int present = entry_exists(slot->journal, S_IFDIR);
    if (present <= 0)
        return present;
    unsigned previous;
    int count;
    int pending = journal_state(j, &previous, &count);
    if (pending < 0 || (pending && restore_companions(slot, j, previous, count)))
        return -1;
    return retire_journal(slot, j);
}

static int stage_checkpoint(const CheckpointSlot* slot, const CheckpointJournal* staged,
                            const uint8_t* header, const PcPortQuickCheckpoint* save,
                            const uint8_t* const next[COMPANION_COUNT])
{
    if (entry_exists(slot->primary, S_IFREG) < 0)
        return -1;
    FILE* file = fopen(staged->primary, "wb");
    if (!file)
        return -1;
    int complete = fwrite(header, 1, PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES, file) ==
                   PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES &&
                   fwrite(save->game_state, 1, sizeof save->game_state, file) ==
                   sizeof save->game_state;
    if (finish_write(file, complete))
        return -1;
    uint8_t marker[5] = {'X', 'G', 'Q', '3', 0};
    for (int i = 0; i < COMPANION_COUNT; ++i) {
        int present = entry_exists(slot->side[i], S_IFREG);
        if (present < 0 || (present && copy_record(slot->side[i], staged->previous[i])))
            return -1;
        marker[4] |= (uint8_t)(present << i);
        if (i == PARTICLE_FILE && (present || next[i]))
            marker[3] = '4';
        if (i == ACTOR_FILE && (present || next[i]))
            marker[3] = '5';
        if (i == MUSIC_FILE && (present || next[i]))
            marker[3] = '6';
        if (i == BART_FILE && (present || next[i]))
            marker[3] = '7';
        size_t bytes = i == PARTICLE_FILE ? save->field_particles_size + PARTICLE_OVERHEAD
                                          : fixed_companion_sizes[i];
        if (next[i] && write_record(staged->next[i], next[i], bytes))
            return -1;
    }
    return write_record(staged->prepared, marker, sizeof marker);
}

static int commit_checkpoint(const CheckpointSlot* slot, const uint8_t* header,
                             const PcPortQuickCheckpoint* save,
                             const uint8_t* const next[COMPANION_COUNT])
{
    char dir[PATH_MAX];
    CheckpointJournal journal;
    if (journal_paths(&journal, slot->journal) || recover_journal(slot, &journal) ||
        staging_dir(dir, slot->primary))
        return -1;
    if (journal_paths(&journal, dir)) {
        rmdir(dir);
        return -1;
    }
    if (stage_checkpoint(slot, &journal, header, save, next) || rename(dir, slot->journal)) {
        discard_staging(dir, &journal);
        return -1;
    }
    /* The same strings were bounds-checked before preparing the transaction. */
    journal_paths(&journal, slot->journal);
    for (int i = 0; i < COMPANION_COUNT; ++i) {
        int failed = next[i] ? replace_save_file(journal.next[i], slot->side[i])
                             : remove_if_present(slot->side[i]);
        if (failed)
            goto rollback;
    }
    if (replace_save_file(journal.primary, slot->primary))
        goto rollback;
    (void)retire_journal(slot, &journal); /* Committed. A later writer can finish cleanup. */
    return 0;

rollback:
    /* The prior tuple remains readable from the journal even if the same
     * filesystem error prevents immediate rollback. A writer retries first. */
    (void)recover_journal(slot, &journal);
    return -1;
}

static uint32_t checkpoint_checksum(const uint8_t* header,
                                    const uint8_t* game_state)
{
    uint32_t hash = 2166136261u;
    hash = checksum_bytes(hash, header + 24,
                          PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES - 24);
    return checksum_bytes(hash, game_state,
                          PC_PORT_QUICK_CHECKPOINT_STATE_BYTES);
}

int PcPort_QuickCheckpointWriteFile(const char* path,
                                    const PcPortQuickCheckpoint* checkpoint)
{
    uint8_t header[PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES] = {0};
    uint8_t sidecar[COMPANION_BYTES];
    uint8_t timer[COMPANION_BYTES];
    uint8_t lighting[LIGHT_BYTES];
    uint8_t camera[CAMERA_BYTES];
    uint8_t particles[PC_PORT_QUICK_EFFECT_BYTES + PARTICLE_OVERHEAD];
    uint8_t actors[ACTOR_BYTES];
    uint8_t music[MUSIC_BYTES] = {0};
    uint8_t bart[BART_BYTES];
    CheckpointSlot slot;
    uint32_t checksum;
    if (!checkpoint || checkpoint->field_actor_kind >= sizeof actor_tags / sizeof actor_tags[0] ||
        checkpoint->field_particles_size > PC_PORT_QUICK_EFFECT_BYTES ||
        slot_paths(&slot, path))
        return -1;

    memcpy(header, kCheckpointMagic, sizeof(kCheckpointMagic));
    put_u32(header + 8, CHECKPOINT_VERSION);
    put_u32(header + 12, PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES);
    put_u32(header + 16, PC_PORT_QUICK_CHECKPOINT_STATE_BYTES);
    put_u16(header + 24, checkpoint->map);
    put_u16(header + 26, checkpoint->entrance);
    put_u32(header + 28, (uint32_t)checkpoint->position[0]);
    put_u32(header + 32, (uint32_t)checkpoint->position[1]);
    put_u32(header + 36, (uint32_t)checkpoint->position[2]);
    put_u16(header + 40, (uint16_t)checkpoint->rotation[0]);
    put_u16(header + 42, (uint16_t)checkpoint->rotation[1]);
    put_u16(header + 44, (uint16_t)checkpoint->rotation[2]);
    if (checkpoint->has_walkmesh && checkpoint->walkmesh_layer < 4) {
        uint16_t triangle = checkpoint->walkmesh_triangle;
        if (triangle > WALKMESH_TRIANGLE_MASK)
            triangle = WALKMESH_TRIANGLE_MASK;
        put_u16(header + 46, WALKMESH_PRESENT |
                (checkpoint->walkmesh_layer << WALKMESH_LAYER_SHIFT) | triangle);
    }
    checksum = checkpoint_checksum(header, checkpoint->game_state);
    put_u32(header + 20, checksum);
    memcpy(sidecar, kSeedTag, sizeof(kSeedTag));
    put_u32(sidecar + 4, checkpoint->random_seed);
    put_u32(sidecar + 8, checksum);
    put_u32(sidecar + 12, record_check(sidecar));
    memcpy(timer, "TIME", 4);
    put_u32(timer + 4, checkpoint->file_ticks);
    put_u32(timer + 8, checksum);
    put_u32(timer + 12, record_check(timer));
    const uint8_t* extension = checkpoint->field_actors + PC_PORT_QUICK_BAL_POSE_BYTES;
    int has_bart = checkpoint->field_actor_kind == PC_PORT_QUICK_ACTORS_BAL && !memcmp(extension, "BRT1", 4);
    const uint8_t* next[COMPANION_COUNT] = {
        checkpoint->has_random_seed ? sidecar : NULL,
        checkpoint->has_file_ticks ? timer : NULL,
        checkpoint->has_field_lighting ? lighting : NULL,
        checkpoint->has_field_camera && checkpoint->camera_heading <= CAMERA_MAX_HEADING ? camera : NULL,
        checkpoint->field_particles_size ? particles : NULL,
        checkpoint->field_actor_kind ? actors : NULL,
        checkpoint->has_delayed_music ? music : NULL,
        has_bart ? bart : NULL
    };
    if (checkpoint->has_field_lighting) {
        memcpy(lighting, "GTL2", 4);
        put_u32(lighting + 4, checksum);
        for (unsigned i = 0; i < PC_PORT_QUICK_LIGHTING_WORDS; ++i)
            put_u32(lighting + 8 + i * 4, checkpoint->field_lighting[i]);
        put_u32(lighting + LIGHT_BYTES - 4,
                field_record_checksum(lighting, LIGHT_BYTES, next[RNG_FILE], next[TIME_FILE]));
    }
    if (next[CAMERA_FILE]) {
        memcpy(camera, "CAM1", 4);
        put_u32(camera + 4, checksum);
        put_u32(camera + 8, checkpoint->camera_heading);
        for (unsigned i = 0; i < 3; ++i) {
            put_u32(camera + 12 + i * 4, (uint32_t)checkpoint->camera_eye[i]);
            put_u32(camera + 24 + i * 4, (uint32_t)checkpoint->camera_at[i]);
        }
        put_u32(camera + CAMERA_BYTES - 4,
                camera_checksum(camera, next[RNG_FILE], next[TIME_FILE], next[LIGHT_FILE]));
    }
    if (next[PARTICLE_FILE]) {
        size_t size = checkpoint->field_particles_size;
        memcpy(particles, "PRT1", 4);
        put_u32(particles + 4, checksum);
        put_u32(particles + 8, (uint32_t)size);
        memcpy(particles + 12, checkpoint->field_particles, size);
        put_u32(particles + 12 + size, camera_tuple_checksum(particles, size + PARTICLE_OVERHEAD,
                next[RNG_FILE], next[TIME_FILE], next[LIGHT_FILE], next[CAMERA_FILE]));
    }
    if (next[ACTOR_FILE]) {
        memcpy(actors, actor_tags[checkpoint->field_actor_kind], 4);
        put_u32(actors + 4, checksum);
        memcpy(actors + 8, checkpoint->field_actors, sizeof checkpoint->field_actors);
        if (has_bart) {
            /* Keep the serialized BAL1 tail zero for older merchant readers. */
            memset(actors + 8 + PC_PORT_QUICK_BAL_POSE_BYTES, 0,
                   PC_PORT_QUICK_CAVE_BYTES - PC_PORT_QUICK_BAL_POSE_BYTES);
            put_u16(actors + 8 + PC_PORT_QUICK_BAL_POSE_BYTES - 2, bart_witness(extension + 4));
        }
        uint32_t hash = camera_tuple_checksum(actors, sizeof actors, next[RNG_FILE],
                next[TIME_FILE], next[LIGHT_FILE], next[CAMERA_FILE]);
        put_u32(actors + sizeof actors - 4, include_companion(hash, "PRT1", next[PARTICLE_FILE],
                checkpoint->field_particles_size + PARTICLE_OVERHEAD));
    }
    if (next[MUSIC_FILE]) {
        memcpy(music, "MUS1", 4);
        put_u32(music + 4, checksum);
        music[8] = checkpoint->music_wait;
        put_u16(music + 10, checkpoint->music_fade_ticks);
        put_u32(music + 12, (uint32_t)checkpoint->music_fade_value);
        put_u32(music + 16, (uint32_t)checkpoint->music_fade_step);
        put_u32(music + 20, actor_tuple_checksum(music, MUSIC_BYTES, next, checkpoint->field_particles_size));
    }
    if (has_bart) {
        memcpy(bart, "BRT1", 4);
        put_u32(bart + 4, checksum);
        memcpy(bart + 8, extension + 4, PC_PORT_QUICK_BART_BYTES);
        put_u32(bart + BART_BYTES - 4, bart_checksum(bart, next, checkpoint->field_particles_size));
    }
    int fd = lock_slot(slot.lock, 1);
    if (fd < 0)
        return -1;
    int result = commit_checkpoint(&slot, header, checkpoint, next);
    close(fd);
    return result;
}

static int read_checkpoint(const char* path, const char* const files[COMPANION_COUNT],
                           unsigned required, PcPortQuickCheckpoint* checkpoint)
{
    uint8_t header[PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES];
    uint8_t sidecar[COMPANION_BYTES];
    PcPortQuickCheckpoint loaded;
    FILE* file;
    int trailing;
    uint32_t expected;
    uint16_t hint;

    if (!path || !checkpoint)
        return -1;
    memset(&loaded, 0, sizeof(loaded));
    file = open_regular_input(path);
    if (!file)
        return -1;
    if (fread(header, 1, sizeof(header), file) != sizeof(header) ||
        memcmp(header, kCheckpointMagic, sizeof(kCheckpointMagic)) != 0 ||
        get_u32(header + 8) != CHECKPOINT_VERSION ||
        get_u32(header + 12) != PC_PORT_QUICK_CHECKPOINT_HEADER_BYTES ||
        get_u32(header + 16) != PC_PORT_QUICK_CHECKPOINT_STATE_BYTES ||
        fread(loaded.game_state, 1, sizeof(loaded.game_state), file) !=
            sizeof(loaded.game_state)) {
        fclose(file);
        return -1;
    }
    trailing = fgetc(file);
    int complete = trailing == EOF && !ferror(file);
    if (fclose(file) != 0 || !complete)
        return -1;

    expected = get_u32(header + 20);
    if (checkpoint_checksum(header, loaded.game_state) != expected)
        return -1;
    loaded.map = get_u16(header + 24);
    loaded.entrance = get_u16(header + 26);
    loaded.position[0] = (int32_t)get_u32(header + 28);
    loaded.position[1] = (int32_t)get_u32(header + 32);
    loaded.position[2] = (int32_t)get_u32(header + 36);
    loaded.rotation[0] = (int16_t)get_u16(header + 40);
    loaded.rotation[1] = (int16_t)get_u16(header + 42);
    loaded.rotation[2] = (int16_t)get_u16(header + 44);
    hint = get_u16(header + 46);
    if (hint & WALKMESH_PRESENT) {
        uint16_t triangle = hint & WALKMESH_TRIANGLE_MASK;
        loaded.has_walkmesh = 1;
        loaded.walkmesh_layer = (hint >> WALKMESH_LAYER_SHIFT) & 3u;
        loaded.walkmesh_triangle = triangle == WALKMESH_TRIANGLE_MASK
            ? UINT16_MAX : triangle;
    }
    int rng_read = read_exact(files[RNG_FILE], sidecar, sizeof sidecar);
    if (rng_read < 0 && (required & (1u << RNG_FILE)))
        return -1;
    if (rng_read == 1 &&
        memcmp(sidecar, kSeedTag, 4) == 0 && get_u32(sidecar + 8) == expected &&
        get_u32(sidecar + 12) == record_check(sidecar)) {
        loaded.has_random_seed = 1;
        loaded.random_seed = get_u32(sidecar + 4);
    }
    uint8_t timer[COMPANION_BYTES];
    int time_read = read_exact(files[TIME_FILE], timer, sizeof timer);
    if (time_read < 0 && (required & (1u << TIME_FILE)))
        return -1;
    if (time_read == 1 &&
        memcmp(timer, "TIME", 4) == 0 && get_u32(timer + 8) == expected &&
        get_u32(timer + 12) == record_check(timer)) {
        loaded.has_file_ticks = 1;
        loaded.file_ticks = get_u32(timer + 4);
    }
    const uint8_t* valid_rng = loaded.has_random_seed ? sidecar : NULL;
    const uint8_t* valid_time = loaded.has_file_ticks ? timer : NULL;
    uint8_t lighting[LIGHT_BYTES];
    int light_read = read_exact(files[LIGHT_FILE], lighting, sizeof lighting);
    if (light_read < 0 && (required & (1u << LIGHT_FILE)))
        return -1;
    if (light_read == 1 && memcmp(lighting, "GTL2", 4) == 0 &&
        get_u32(lighting + 4) == expected &&
        get_u32(lighting + LIGHT_BYTES - 4) ==
            field_record_checksum(lighting, LIGHT_BYTES, valid_rng, valid_time)) {
        for (unsigned i = 0; i < PC_PORT_QUICK_LIGHTING_WORDS; ++i)
            loaded.field_lighting[i] = get_u32(lighting + 8 + i * 4);
        loaded.has_field_lighting = 1;
    }
    uint8_t camera[CAMERA_BYTES];
    int camera_read = read_exact(files[CAMERA_FILE], camera, sizeof camera);
    if (camera_read < 0 && (required & (1u << CAMERA_FILE)))
        return -1;
    if (camera_read == 1 && memcmp(camera, "CAM1", 4) == 0 &&
        get_u32(camera + 4) == expected && get_u32(camera + 8) <= CAMERA_MAX_HEADING &&
        get_u32(camera + CAMERA_BYTES - 4) ==
            camera_checksum(camera, valid_rng, valid_time, loaded.has_field_lighting ? lighting : NULL)) {
        loaded.camera_heading = (uint16_t)get_u32(camera + 8);
        for (unsigned i = 0; i < 3; ++i) {
            loaded.camera_eye[i] = (int32_t)get_u32(camera + 12 + i * 4);
            loaded.camera_at[i] = (int32_t)get_u32(camera + 24 + i * 4);
        }
        loaded.has_field_camera = 1;
    }
    uint8_t particles[PC_PORT_QUICK_EFFECT_BYTES + PARTICLE_OVERHEAD];
    size_t particle_size = 0;
    int particle_read = read_particles(files[PARTICLE_FILE], particles, &particle_size);
    if (particle_read < 0 && (required & (1u << PARTICLE_FILE)))
        return -1;
    const uint8_t* valid_light = loaded.has_field_lighting ? lighting : NULL;
    const uint8_t* valid_camera = loaded.has_field_camera ? camera : NULL;
    if (particle_read == 1 && memcmp(particles, "PRT1", 4) == 0 &&
        get_u32(particles + 4) == expected &&
        get_u32(particles + 12 + particle_size) ==
            camera_tuple_checksum(particles, particle_size + PARTICLE_OVERHEAD,
                                  valid_rng, valid_time, valid_light, valid_camera)) {
        loaded.field_particles_size = particle_size;
        memcpy(loaded.field_particles, particles + 12, particle_size);
    }
    uint8_t actors[ACTOR_BYTES];
    int actor_read = read_exact(files[ACTOR_FILE], actors, sizeof actors);
    if (actor_read < 0 && (required & (1u << ACTOR_FILE)))
        return -1;
    if (actor_read == 1 && get_u32(actors + 4) == expected) {
        uint32_t hash = camera_tuple_checksum(actors, sizeof actors,
                valid_rng, valid_time, valid_light, valid_camera);
        const uint8_t* valid_particles = loaded.field_particles_size ? particles : NULL;
        if (get_u32(actors + sizeof actors - 4) ==
            include_companion(hash, "PRT1", valid_particles, particle_size + PARTICLE_OVERHEAD)) {
            for (unsigned kind = 1; kind < sizeof actor_tags / sizeof actor_tags[0]; ++kind) {
                if (memcmp(actors, actor_tags[kind], 4) == 0) {
                    /* The combined BRT1 tag exists only in memory after its
                     * independent file and the complete tuple verify. */
                    if (kind == PC_PORT_QUICK_ACTORS_BAL &&
                        !memcmp(actors + 8 + PC_PORT_QUICK_BAL_POSE_BYTES, "BRT1", 4))
                        break;
                    loaded.field_actor_kind = (uint8_t)kind;
                    memcpy(loaded.field_actors, actors + 8, sizeof loaded.field_actors);
                    break;
                }
            }
        }
    }
    const uint8_t* valid[COMPANION_COUNT] = {
        valid_rng, valid_time, valid_light, valid_camera,
        loaded.field_particles_size ? particles : NULL,
        loaded.field_actor_kind ? actors : NULL
    };
    uint8_t music[MUSIC_BYTES];
    int music_read = read_exact(files[MUSIC_FILE], music, sizeof music);
    if (music_read < 0 && (required & (1u << MUSIC_FILE)))
        return -1;
    if (music_read == 1 && memcmp(music, "MUS1", 4) == 0 &&
        get_u32(music + 4) == expected && music[9] == 0) {
        if (get_u32(music + 20) == actor_tuple_checksum(music, MUSIC_BYTES, valid, loaded.field_particles_size)) {
            loaded.music_wait = music[8];
            loaded.music_fade_ticks = get_u16(music + 10);
            loaded.music_fade_value = (int32_t)get_u32(music + 12);
            loaded.music_fade_step = (int32_t)get_u32(music + 16);
            loaded.has_delayed_music = 1;
            valid[MUSIC_FILE] = music;
        }
    }
    uint8_t bart[BART_BYTES];
    int bart_read = read_exact(files[BART_FILE], bart, sizeof bart);
    if (bart_read < 0 && (required & (1u << BART_FILE)))
        return -1;
    if (loaded.field_actor_kind == PC_PORT_QUICK_ACTORS_BAL && bart_read == 1 &&
        memcmp(bart, "BRT1", 4) == 0 && get_u32(bart + 4) == expected &&
        get_u16(loaded.field_actors + PC_PORT_QUICK_BAL_POSE_BYTES - 2) == bart_witness(bart + 8) &&
        get_u32(bart + BART_BYTES - 4) == bart_checksum(bart, valid, loaded.field_particles_size)) {
        uint8_t* extension = loaded.field_actors + PC_PORT_QUICK_BAL_POSE_BYTES;
        /* The witness is file metadata, not part of the live merchant pose. */
        put_u16(extension - 2, 0);
        memcpy(extension, "BRT1", 4);
        memcpy(extension + 4, bart + 8, PC_PORT_QUICK_BART_BYTES);
    }
    *checkpoint = loaded;
    return 0;
}

static int read_locked(const CheckpointSlot* slot, PcPortQuickCheckpoint* checkpoint)
{
    int present = entry_exists(slot->journal, S_IFDIR);
    if (present < 0)
        return -1;
    CheckpointJournal active;
    const char* files[COMPANION_COUNT];
    for (int i = 0; i < COMPANION_COUNT; ++i)
        files[i] = slot->side[i];
    unsigned required = 0;
    if (present) {
        unsigned previous;
        int count;
        if (journal_paths(&active, slot->journal))
            return -1;
        int pending = journal_state(&active, &previous, &count);
        if (pending < 0)
            return -1;
        if (pending) {
            required = previous;
            for (int i = 0; i < count; ++i)
                files[i] = active.previous[i];
        }
    }
    return read_checkpoint(slot->primary, files, required, checkpoint);
}

int PcPort_QuickCheckpointReadFile(const char* path, PcPortQuickCheckpoint* checkpoint)
{
    CheckpointSlot slot;
    if (!checkpoint || slot_paths(&slot, path))
        return -1;
    int fd = lock_slot(slot.lock, 0);
    if (fd == -2) {
        /* A first writer could create the lock during our import. In that
         * case throw away the speculative result and reread every file. */
        PcPortQuickCheckpoint legacy;
        int result = read_locked(&slot, &legacy);
        fd = lock_slot(slot.lock, 0);
        if (fd == -2) {
            if (result == 0)
                *checkpoint = legacy;
            return result;
        }
    }
    if (fd < 0)
        return -1;
    int result = read_locked(&slot, checkpoint);
    close(fd);
    return result;
}
