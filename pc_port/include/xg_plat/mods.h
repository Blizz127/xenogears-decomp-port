/* xg_plat/mods.h -- mods interface of the moddable port layer.
 *
 * Mods live in folders under mods/ (working directory, or mods.dir in the
 * config) and ${XDG_DATA_HOME:-~/.local/share}/xenogears-port/mods/.  Each
 * mod folder holds:
 *   mod.txt        manifest, `key = value` lines:
 *                    name = Example Mod     (required)
 *                    version = 1.0
 *                    load_order = 10        (lower loads first; ties by name)
 *                    abi = 1                (XG_PLAT_MODS_ABI_VERSION)
 *   assets/<sha256>.bin
 *                  replacement for the game file whose ORIGINAL bytes (as
 *                  read from the user's disc) hash to <sha256>.  Run with
 *                  XENO_MODS_LOG_ASSETS=1 to print the hash of every file
 *                  the game loads, or XENO_MODS_DUMP_DIR=<dir> to save each
 *                  original (from your own disc) as <dir>/<sha256>.bin to
 *                  edit.  Replacement files come from modders; nothing from
 *                  the disc is shipped.
 *   script = <file>.lua   (optional manifest key) RESERVED for Lua
 *                  scripting: recognised and reported, not run yet; the
 *                  mod's assets and plugin still load.  See
 *                  docs/port/MODDING.md.
 *   plugin.so      optional C plugin exporting
 *                    int xg_mod_init(const XgPlatModApi* api);
 *                  which may subscribe to events through the api.
 * Mods are disabled with mods.enabled = off (XENO_MODS_ENABLED=0).  With no
 * mods present every call here is a no-op, so default play is unchanged.
 */
#ifndef XG_PLAT_MODS_H
#define XG_PLAT_MODS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XG_PLAT_MODS_ABI_VERSION 1
#define XG_PLAT_MODS_MAX         32
#define XG_PLAT_MODS_MAX_HOOKS   64

typedef enum {
    XG_PLAT_EVENT_BOOT = 0,      /* once, before the game's main loop */
    XG_PLAT_EVENT_FRAME_TICK,    /* every presented frame */
    XG_PLAT_EVENT_ASSET_LOADED,  /* arg0 = size; a game file was read */
    XG_PLAT_EVENT_ROOM_ENTER,    /* arg0 = field map number (g_GameSceneMapNum & 0xFFFF) */
    XG_PLAT_EVENT_COUNT
} XgPlatModEvent;

typedef struct {
    XgPlatModEvent event;
    uint32_t arg0, arg1;
    uint64_t sequence;           /* emits so far, all events */
} XgPlatModEventData;

typedef void (*XgPlatModHook)(const XgPlatModEventData* event, void* user);

typedef struct {
    char name[64];
    char version[32];
    int  load_order;
    int  abi;
    char dir[512];
    char script[128];            /* reserved: Lua script named by the manifest */
} XgPlatModManifest;

/* What a plugin gets from xg_mod_init. */
typedef struct {
    int abi;
    int (*subscribe)(XgPlatModEvent event, XgPlatModHook hook, void* user);
    int (*unsubscribe)(int handle);
    void (*log)(const char* fmt, ...);
    /* Reserved for later ABI 1 additions (first: the Lua scripting bridge).
     * Always NULL today; plugins must not use them.  New members are only
     * appended, so plugins built against ABI 1 keep working. */
    void* reserved[4];
} XgPlatModApi;

/* Scan the mod folders, load manifests and plugins, index asset
 * replacements; emits nothing.  Returns the number of mods loaded. */
int  xg_plat_mods_init(void);
int  xg_plat_mods_count(void);
const XgPlatModManifest* xg_plat_mods_get(int index);

/* Hook table. */
int  xg_plat_mods_subscribe(XgPlatModEvent event, XgPlatModHook hook, void* user);
int  xg_plat_mods_unsubscribe(int handle);
int  xg_plat_mods_emit(XgPlatModEvent event, uint32_t arg0, uint32_t arg1);
const char* xg_plat_mods_event_name(XgPlatModEvent event);

/* Parse manifest text; 1 when it names the mod and the ABI is compatible. */
int  xg_plat_mods_parse_manifest(const char* text, XgPlatModManifest* out);

/* Asset replacement: called with a game file just read into `buffer`
 * (`size` bytes, `capacity` usable).  When a mod replaces the file (keyed
 * by the SHA-256 of these original bytes) the replacement is copied in and
 * its size returned; otherwise `size` is returned unchanged.  Replacements
 * larger than `capacity` are refused with a message. */
size_t xg_plat_mods_filter_asset(void* buffer, size_t size, size_t capacity);

/* 1 when any mod replaces assets or asset logging/dumping is on, i.e. when a
 * loader should hash what it reads (streamed data reads the whole file up
 * front only in that case). */
int xg_plat_mods_assets_active(void);

/* Replacement for a game file given its ORIGINAL bytes: a malloc'd copy of
 * the mod's file (caller frees) with *out_size set, or NULL when no mod
 * replaces it.  Also performs XENO_MODS_LOG_ASSETS / XENO_MODS_DUMP_DIR. */
void* xg_plat_mods_lookup_asset(const void* data, size_t size, size_t* out_size);

#ifdef __cplusplus
}
#endif

#endif
