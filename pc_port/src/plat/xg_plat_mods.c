/* xg_plat mods: folder scan, manifests, event hooks, C plugins and
 * hash-keyed asset replacement (see include/xg_plat/mods.h). */
#include <ctype.h>
#include <dirent.h>
#include <dlfcn.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "../../include/xg_plat/mods.h"
#include "../../include/xg_plat/config.h"
#include "../retail_data.h" /* XenoSha256 */

typedef struct {
    int handle;
    XgPlatModEvent event;
    XgPlatModHook hook;
    void* user;
} Hook;

typedef struct {
    char sha256[65];
    int mod;           /* index into s_mods */
    char path[600];
} Asset;

static XgPlatModManifest s_mods[XG_PLAT_MODS_MAX];
static int s_mod_count;
static Hook s_hooks[XG_PLAT_MODS_MAX_HOOKS];
static int s_hook_count;
static int s_next_handle = 1;
static uint64_t s_sequence;
static Asset* s_assets;
static int s_asset_count;
static int s_log_assets;
static const char* s_dump_dir;

static const char* const s_event_names[XG_PLAT_EVENT_COUNT] = {
    "boot", "frame_tick", "asset_loaded", "room_enter",
};

const char* xg_plat_mods_event_name(XgPlatModEvent event)
{
    return (event >= 0 && event < XG_PLAT_EVENT_COUNT) ? s_event_names[event] : NULL;
}

int xg_plat_mods_subscribe(XgPlatModEvent event, XgPlatModHook hook, void* user)
{
    if (event < 0 || event >= XG_PLAT_EVENT_COUNT || hook == NULL ||
        s_hook_count >= XG_PLAT_MODS_MAX_HOOKS)
        return 0;
    s_hooks[s_hook_count].handle = s_next_handle;
    s_hooks[s_hook_count].event = event;
    s_hooks[s_hook_count].hook = hook;
    s_hooks[s_hook_count].user = user;
    s_hook_count++;
    return s_next_handle++;
}

int xg_plat_mods_unsubscribe(int handle)
{
    int i;
    for (i = 0; i < s_hook_count; i++) {
        if (s_hooks[i].handle == handle) {
            memmove(&s_hooks[i], &s_hooks[i + 1], (size_t)(s_hook_count - i - 1) * sizeof s_hooks[0]);
            s_hook_count--;
            return 1;
        }
    }
    return 0;
}

int xg_plat_mods_emit(XgPlatModEvent event, uint32_t arg0, uint32_t arg1)
{
    XgPlatModEventData data;
    int i, ran = 0;
    if (event < 0 || event >= XG_PLAT_EVENT_COUNT)
        return 0;
    data.event = event;
    data.arg0 = arg0;
    data.arg1 = arg1;
    data.sequence = s_sequence++;
    for (i = 0; i < s_hook_count; i++) {
        if (s_hooks[i].event == event) {
            s_hooks[i].hook(&data, s_hooks[i].user);
            ran++;
        }
    }
    return ran;
}

static char* trim(char* s)
{
    char* e;
    while (isspace((unsigned char)*s))
        s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = '\0';
    return s;
}

int xg_plat_mods_parse_manifest(const char* text, XgPlatModManifest* out)
{
    char line[256];
    const char* p = text;
    memset(out, 0, sizeof *out);
    out->abi = XG_PLAT_MODS_ABI_VERSION;
    while (*p) {
        size_t len = strcspn(p, "\n");
        char *s, *eq, *k, *v;
        if (len >= sizeof line)
            len = sizeof line - 1;
        memcpy(line, p, len);
        line[len] = '\0';
        p += strcspn(p, "\n");
        if (*p)
            p++;
        s = trim(line);
        if (*s == '#' || *s == '\0' || (eq = strchr(s, '=')) == NULL)
            continue;
        *eq = '\0';
        k = trim(s);
        v = trim(eq + 1);
        if (!strcmp(k, "name"))
            snprintf(out->name, sizeof out->name, "%s", v);
        else if (!strcmp(k, "version"))
            snprintf(out->version, sizeof out->version, "%s", v);
        else if (!strcmp(k, "load_order"))
            out->load_order = atoi(v);
        else if (!strcmp(k, "abi"))
            out->abi = atoi(v);
        else if (!strcmp(k, "script"))
            snprintf(out->script, sizeof out->script, "%s", v);
    }
    return out->name[0] != '\0' && out->abi == XG_PLAT_MODS_ABI_VERSION;
}

static void api_log(const char* fmt, ...)
{
    va_list ap;
    fputs("[mod] ", stderr);
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
    fputc('\n', stderr);
}

static const XgPlatModApi s_api = {
    XG_PLAT_MODS_ABI_VERSION, xg_plat_mods_subscribe, xg_plat_mods_unsubscribe, api_log,
    { NULL, NULL, NULL, NULL },
};

static int is_hex64(const char* s)
{
    int i;
    for (i = 0; i < 64; i++)
        if (!isxdigit((unsigned char)s[i]))
            return 0;
    return 1;
}

static void index_assets(int mod)
{
    char dir[520];
    DIR* d;
    struct dirent* e;
    snprintf(dir, sizeof dir, "%s/assets", s_mods[mod].dir);
    if ((d = opendir(dir)) == NULL)
        return;
    while ((e = readdir(d)) != NULL) {
        const char* n = e->d_name;
        Asset* grown;
        int i;
        if (strlen(n) != 68 || strcmp(n + 64, ".bin") != 0 || !is_hex64(n))
            continue;
        /* a later mod (higher load order) overrides an earlier one */
        for (i = 0; i < s_asset_count; i++)
            if (!strncasecmp(s_assets[i].sha256, n, 64))
                break;
        if (i == s_asset_count) {
            grown = (Asset*)realloc(s_assets, (size_t)(s_asset_count + 1) * sizeof *s_assets);
            if (grown == NULL)
                break;
            s_assets = grown;
            s_asset_count++;
        }
        memcpy(s_assets[i].sha256, n, 64);
        s_assets[i].sha256[64] = '\0';
        for (char* c = s_assets[i].sha256; *c; c++)
            *c = (char)tolower((unsigned char)*c);
        s_assets[i].mod = mod;
        snprintf(s_assets[i].path, sizeof s_assets[i].path, "%s/%.68s", dir, n);
    }
    closedir(d);
}

static void load_plugin(int mod)
{
    char path[540];
    struct stat st;
    void* so;
    int (*init)(const XgPlatModApi*);
    snprintf(path, sizeof path, "%s/plugin.so", s_mods[mod].dir);
    if (stat(path, &st) != 0)
        return;
    so = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (so == NULL) {
        fprintf(stderr, "[xg_plat] mods: %s: cannot load plugin: %s\n", s_mods[mod].name, dlerror());
        return;
    }
    *(void**)&init = dlsym(so, "xg_mod_init");
    if (init == NULL || init(&s_api) != 0)
        fprintf(stderr, "[xg_plat] mods: %s: plugin has no xg_mod_init or it failed\n",
                s_mods[mod].name);
}

static void scan_root(const char* root)
{
    DIR* d = opendir(root);
    struct dirent* e;
    if (d == NULL)
        return;
    while ((e = readdir(d)) != NULL && s_mod_count < XG_PLAT_MODS_MAX) {
        char mdir[512], mpath[600];
        FILE* f;
        char text[4096];
        size_t n;
        XgPlatModManifest m;
        if (e->d_name[0] == '.')
            continue;
        snprintf(mdir, sizeof mdir, "%s/%s", root, e->d_name);
        snprintf(mpath, sizeof mpath, "%s/mod.txt", mdir);
        if ((f = fopen(mpath, "rb")) == NULL)
            continue;
        n = fread(text, 1, sizeof text - 1, f);
        fclose(f);
        text[n] = '\0';
        if (!xg_plat_mods_parse_manifest(text, &m)) {
            fprintf(stderr, "[xg_plat] mods: %s: manifest needs a name and abi = %d; skipped\n",
                    mpath, XG_PLAT_MODS_ABI_VERSION);
            continue;
        }
        snprintf(m.dir, sizeof m.dir, "%s", mdir);
        s_mods[s_mod_count++] = m;
    }
    closedir(d);
}

static int mod_order(const void* a, const void* b)
{
    const XgPlatModManifest* x = (const XgPlatModManifest*)a;
    const XgPlatModManifest* y = (const XgPlatModManifest*)b;
    if (x->load_order != y->load_order)
        return x->load_order < y->load_order ? -1 : 1;
    return strcmp(x->name, y->name);
}

int xg_plat_mods_init(void)
{
    static int done;
    const char* dir;
    const char* xdg = getenv("XDG_DATA_HOME");
    const char* home = getenv("HOME");
    char user_root[600];
    int i;

    if (done)
        return s_mod_count;
    done = 1;
    s_log_assets = getenv("XENO_MODS_LOG_ASSETS") && getenv("XENO_MODS_LOG_ASSETS")[0] == '1';
    s_dump_dir = getenv("XENO_MODS_DUMP_DIR");
    if (s_dump_dir && !*s_dump_dir)
        s_dump_dir = NULL;
    if (!xg_plat_config_int("mods.enabled", 1))
        return 0;
    dir = xg_plat_config_get("mods.dir");
    scan_root(dir && *dir ? dir : "mods");
    if (xdg && *xdg)
        snprintf(user_root, sizeof user_root, "%s/xenogears-port/mods", xdg);
    else if (home && *home)
        snprintf(user_root, sizeof user_root, "%s/.local/share/xenogears-port/mods", home);
    else
        user_root[0] = '\0';
    if (user_root[0])
        scan_root(user_root);
    qsort(s_mods, (size_t)s_mod_count, sizeof s_mods[0], mod_order);
    for (i = 0; i < s_mod_count; i++) {
        index_assets(i);
        load_plugin(i);
        if (s_mods[i].script[0])
            fprintf(stderr, "[xg_plat] mods: %s: script '%s' not run: Lua scripting is reserved "
                            "but not implemented yet (assets and plugin still load)\n",
                    s_mods[i].name, s_mods[i].script);
        printf("[xg_plat] mod %d: %s %s (load order %d) from %s\n", i, s_mods[i].name,
               s_mods[i].version, s_mods[i].load_order, s_mods[i].dir);
    }
    if (s_mod_count)
        printf("[xg_plat] mods: %d loaded, %d asset replacement(s)\n", s_mod_count, s_asset_count);
    return s_mod_count;
}

int xg_plat_mods_count(void)
{
    return s_mod_count;
}

const XgPlatModManifest* xg_plat_mods_get(int index)
{
    return (index >= 0 && index < s_mod_count) ? &s_mods[index] : NULL;
}

int xg_plat_mods_assets_active(void)
{
    return s_asset_count > 0 || s_log_assets || s_dump_dir != NULL;
}

static int find_replacement(const void* data, size_t size, char digest[65])
{
    XenoSha256 sha;
    int i;
    FILE* f;
    XenoSha256_Init(&sha);
    XenoSha256_Update(&sha, data, size);
    XenoSha256_HexFinal(&sha, digest);
    if (s_log_assets)
        printf("[xg_plat] asset %zu bytes sha256 %s\n", size, digest);
    if (s_dump_dir) {
        /* Modder aid: the original, as read from the user's own disc, under
         * the name a replacement must use.  Local files only. */
        char path[1024];
        struct stat st;
        snprintf(path, sizeof path, "%s/%s.bin", s_dump_dir, digest);
        if (stat(path, &st) != 0 && (f = fopen(path, "wb")) != NULL) {
            fwrite(data, 1, size, f);
            fclose(f);
        }
    }
    for (i = 0; i < s_asset_count; i++)
        if (strcmp(s_assets[i].sha256, digest) == 0)
            return i;
    return -1;
}

static void* read_whole(const char* path, size_t* out_size)
{
    FILE* f = fopen(path, "rb");
    long len;
    void* buf = NULL;
    if (f == NULL)
        return NULL;
    if (fseek(f, 0, SEEK_END) == 0 && (len = ftell(f)) >= 0 && fseek(f, 0, SEEK_SET) == 0 &&
        (buf = malloc((size_t)len ? (size_t)len : 1)) != NULL &&
        fread(buf, 1, (size_t)len, f) != (size_t)len) {
        free(buf);
        buf = NULL;
    }
    if (buf)
        *out_size = (size_t)len;
    fclose(f);
    return buf;
}

void* xg_plat_mods_lookup_asset(const void* data, size_t size, size_t* out_size)
{
    char digest[65];
    int i;
    void* repl;
    if (!xg_plat_mods_assets_active() || data == NULL || size == 0)
        return NULL;
    if ((i = find_replacement(data, size, digest)) < 0)
        return NULL;
    repl = read_whole(s_assets[i].path, out_size);
    if (repl)
        printf("[xg_plat] mods: %s replaced asset %s (%zu -> %zu bytes)\n",
               s_mods[s_assets[i].mod].name, digest, size, *out_size);
    return repl;
}

size_t xg_plat_mods_filter_asset(void* buffer, size_t size, size_t capacity)
{
    size_t repl_size = 0;
    void* repl;

    xg_plat_mods_emit(XG_PLAT_EVENT_ASSET_LOADED, (uint32_t)size, 0);
    repl = xg_plat_mods_lookup_asset(buffer, size, &repl_size);
    if (repl == NULL)
        return size;
    if (repl_size > capacity) {
        fprintf(stderr, "[xg_plat] mods: replacement is %zu bytes, larger than the game's "
                        "%zu-byte buffer; not applied\n", repl_size, capacity);
        free(repl);
        return size;
    }
    memcpy(buffer, repl, repl_size);
    if (repl_size < capacity)
        memset((uint8_t*)buffer + repl_size, 0, capacity - repl_size);
    free(repl);
    return repl_size;
}
