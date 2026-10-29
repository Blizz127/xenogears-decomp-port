/* xg_plat configuration loader (see include/xg_plat/config.h). */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>

#include "../../include/xg_plat/config.h"
#include "../../include/xg_plat/audio.h"
#include "../../include/xg_plat/input.h"
#include "../../include/xg_plat/timing.h"

#define CFG_MAX 128

typedef struct { char key[64]; char value[192]; } CfgEntry;

static CfgEntry s_entries[CFG_MAX];
static int s_count;
static CfgEntry s_cli[CFG_MAX];   /* --set overrides */
static int s_cli_count;
static char s_cli_file[1024];     /* --config */
static int s_loaded;
static char s_path[1024];

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

static void set_entry(const char* key, const char* value)
{
    int i;
    for (i = 0; i < s_count; i++) {
        if (strcmp(s_entries[i].key, key) == 0) {
            snprintf(s_entries[i].value, sizeof s_entries[i].value, "%s", value);
            return;
        }
    }
    if (s_count < CFG_MAX) {
        snprintf(s_entries[s_count].key, sizeof s_entries[s_count].key, "%s", key);
        snprintf(s_entries[s_count].value, sizeof s_entries[s_count].value, "%s", value);
        s_count++;
    } else {
        fprintf(stderr, "[xg_plat] config: more than %d keys, ignoring %s\n", CFG_MAX, key);
    }
}

int xg_plat_config_parse(const char* text)
{
    char section[64] = "";
    char line[512];
    const char* p = text;
    int n = 0;

    s_count = 0;
    while (*p) {
        size_t len = strcspn(p, "\n");
        char* s;
        char* eq;
        if (len >= sizeof line)
            len = sizeof line - 1;
        memcpy(line, p, len);
        line[len] = '\0';
        p += strcspn(p, "\n");
        if (*p == '\n')
            p++;
        s = trim(line);
        if (*s == '\0' || *s == '#' || *s == ';')
            continue;
        if (*s == '[') {
            char* close = strchr(s, ']');
            if (close) {
                *close = '\0';
                snprintf(section, sizeof section, "%s", trim(s + 1));
            }
            continue;
        }
        eq = strchr(s, '=');
        if (eq == NULL) {
            fprintf(stderr, "[xg_plat] config: ignoring line without '=': %s\n", s);
            continue;
        }
        *eq = '\0';
        {
            char key[64];
            char* k = trim(s);
            char* v = trim(eq + 1);
            char* hash = strchr(v, '#');
            if (hash) {
                *hash = '\0';
                v = trim(v);
            }
            if (section[0] && strchr(k, '.') == NULL)
                snprintf(key, sizeof key, "%s.%s", section, k);
            else
                snprintf(key, sizeof key, "%s", k);
            for (k = key; *k; k++)
                *k = (char)tolower((unsigned char)*k);
            set_entry(key, v);
            n++;
        }
    }
    return n;
}

static int exe_dir(char* out, size_t n)
{
    ssize_t len = readlink("/proc/self/exe", out, n - 1);
    char* slash;
    if (len <= 0)
        return 0;
    out[len] = '\0';
    slash = strrchr(out, '/');
    if (slash == NULL)
        return 0;
    *slash = '\0';
    return 1;
}

int xg_plat_config_cli(int argc, char** argv)
{
    int i;
    for (i = 1; i < argc; i++) {
        const char* a = argv[i];
        if (!strcmp(a, "--help") || !strcmp(a, "-h")) {
            printf("usage: %s [--config FILE] [--set key=value]...\n"
                   "  --config FILE     read FILE instead of config.ini\n"
                   "  --set key=value   override one config key (e.g. video.width=1280)\n"
                   "Keys and defaults: pc_port/config.example.ini; XENO_<KEY> environment\n"
                   "variables also override the file.\n", argv[0]);
            return 1;
        }
        if (!strcmp(a, "--config") || !strncmp(a, "--config=", 9)) {
            const char* v = a[8] == '=' ? a + 9 : (i + 1 < argc ? argv[++i] : NULL);
            if (v == NULL || !*v) {
                fprintf(stderr, "[xg_plat] --config needs a file\n");
                return -1;
            }
            snprintf(s_cli_file, sizeof s_cli_file, "%s", v);
            continue;
        }
        if (!strcmp(a, "--set") || !strncmp(a, "--set=", 6)) {
            const char* v = a[5] == '=' ? a + 6 : (i + 1 < argc ? argv[++i] : NULL);
            const char* eq = v ? strchr(v, '=') : NULL;
            size_t k;
            if (eq == NULL || eq == v || s_cli_count >= CFG_MAX) {
                fprintf(stderr, "[xg_plat] --set needs key=value\n");
                return -1;
            }
            k = (size_t)(eq - v) < sizeof s_cli[0].key ? (size_t)(eq - v) : sizeof s_cli[0].key - 1;
            memcpy(s_cli[s_cli_count].key, v, k);
            s_cli[s_cli_count].key[k] = '\0';
            snprintf(s_cli[s_cli_count].value, sizeof s_cli[0].value, "%s", eq + 1);
            s_cli_count++;
            continue;
        }
        fprintf(stderr, "[xg_plat] unknown option '%s' (try --help)\n", a);
        return -1;
    }
    return 0;
}

int xg_plat_config_load(void)
{
    const char* env = getenv("XENO_CONFIG");
    const char* xdg = getenv("XDG_CONFIG_HOME");
    const char* home = getenv("HOME");
    char cand[4][1024];
    char dir[900];
    int ncand = 0, i, n;
    FILE* f = NULL;

    if (s_loaded)
        return s_count;
    s_loaded = 1;
    if (s_cli_file[0])
        snprintf(cand[ncand++], sizeof cand[0], "%s", s_cli_file);
    else if (env && *env)
        snprintf(cand[ncand++], sizeof cand[0], "%s", env);
    else {
        if (exe_dir(dir, sizeof dir))
            snprintf(cand[ncand++], sizeof cand[0], "%s/config.ini", dir);
        if (xdg && *xdg)
            snprintf(cand[ncand++], sizeof cand[0], "%.900s/xenogears-port/config.ini", xdg);
        else if (home && *home)
            snprintf(cand[ncand++], sizeof cand[0], "%.900s/.config/xenogears-port/config.ini", home);
    }
    for (i = 0; i < ncand && f == NULL; i++) {
        f = fopen(cand[i], "rb");
        if (f) {
            strncpy(s_path, cand[i], sizeof s_path - 1);
            s_path[sizeof s_path - 1] = '\0';
        }
    }
    if (f == NULL) {
        if (s_cli_file[0] || (env && *env)) {
            fprintf(stderr, "[xg_plat] config: %s not readable\n", cand[0]);
            return -1;
        }
        return 0;
    }
    {
        char* text;
        long len;
        if (fseek(f, 0, SEEK_END) != 0 || (len = ftell(f)) < 0 || fseek(f, 0, SEEK_SET) != 0 ||
            (text = (char*)malloc((size_t)len + 1)) == NULL) {
            fclose(f);
            return -1;
        }
        len = (long)fread(text, 1, (size_t)len, f);
        text[len] = '\0';
        fclose(f);
        n = xg_plat_config_parse(text);
        free(text);
    }
    printf("[xg_plat] config: %d key(s) from %s\n", n, s_path);
    return n;
}

const char* xg_plat_config_path(void)
{
    return s_path[0] ? s_path : NULL;
}

/* ---- persisting one key (player preferences changed in a menu) ---------- */

/* Path persist writes: the file that was loaded (--config, $XENO_CONFIG, or
 * whichever default was found), else ${XDG_CONFIG_HOME:-~/.config}/
 * xenogears-port/config.ini.  Returns 0 when there is no usable location. */
static int xdg_config_path(char* out, size_t n)
{
    const char* xdg = getenv("XDG_CONFIG_HOME");
    const char* home = getenv("HOME");
    if (xdg && *xdg)
        snprintf(out, n, "%.900s/xenogears-port/config.ini", xdg);
    else if (home && *home)
        snprintf(out, n, "%.900s/.config/xenogears-port/config.ini", home);
    else
        return 0;
    return 1;
}

const char* xg_plat_config_user_path(char* out, size_t n)
{
    const char* env = getenv("XENO_CONFIG");
    if (s_cli_file[0])
        snprintf(out, n, "%s", s_cli_file);
    else if (env && *env)
        snprintf(out, n, "%s", env);
    else if (s_path[0])
        snprintf(out, n, "%s", s_path);
    else if (!xdg_config_path(out, n))
        return NULL;
    return out;
}

static void make_parent_dirs(const char* path)
{
    char dir[1024];
    char* p;
    snprintf(dir, sizeof dir, "%s", path);
    for (p = dir + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(dir, 0777);
            *p = '/';
        }
    }
}

static int ieq(const char* a, size_t alen, const char* b)
{
    size_t i;
    if (strlen(b) != alen)
        return 0;
    for (i = 0; i < alen; i++)
        if (tolower((unsigned char)a[i]) != tolower((unsigned char)b[i]))
            return 0;
    return 1;
}

/* Rewrite `text` so that section.name = value.  Other lines, comments and
 * blank lines are kept byte for byte.  An existing assignment keeps its
 * indentation and trailing comment.  Returns a malloc'd string. */
static char* rewrite_key(const char* text, const char* section, const char* name,
                         const char* value)
{
    size_t cap = strlen(text) + strlen(section) + strlen(name) + strlen(value) + 64;
    char* out = (char*)malloc(cap);
    size_t o = 0;
    const char* p = text;
    char cur[64] = "";
    int found = 0, in_target = 0;
    long insert_at = -1;   /* offset in out after the target section's last content line */
    char full[130];

    if (out == NULL)
        return NULL;
    snprintf(full, sizeof full, "%s.%s", section, name);
    while (*p) {
        size_t len = strcspn(p, "\n");
        int has_nl = p[len] == '\n';
        const char* ls = p;
        const char* t = ls;
        const char* te = ls + len;
        int handled = 0;
        while (t < te && isspace((unsigned char)*t))
            t++;
        if (t < te && *t == '[') {
            const char* close = memchr(t, ']', (size_t)(te - t));
            if (close) {
                const char* a = t + 1;
                const char* b = close;
                while (a < b && isspace((unsigned char)*a)) a++;
                while (b > a && isspace((unsigned char)b[-1])) b--;
                snprintf(cur, sizeof cur, "%.*s", (int)(b - a), a);
                in_target = ieq(cur, strlen(cur), section);
            }
        } else if (t < te && *t != '#' && *t != ';') {
            const char* eq = memchr(t, '=', (size_t)(te - t));
            if (eq) {
                const char* ke = eq;
                const char* v = eq + 1;
                const char* hash;
                while (ke > t && isspace((unsigned char)ke[-1])) ke--;
                if ((in_target && ieq(t, (size_t)(ke - t), name)) ||
                    ieq(t, (size_t)(ke - t), full)) {
                    hash = memchr(v, '#', (size_t)(te - v));
                    o += (size_t)snprintf(out + o, cap - o, "%.*s = %s", (int)(ke - ls), ls, value);
                    if (hash) {
                        const char* ce = hash;
                        while (ce > v && isspace((unsigned char)ce[-1])) ce--;
                        (void)ce;
                        o += (size_t)snprintf(out + o, cap - o, "  %.*s", (int)(te - hash), hash);
                    }
                    if (has_nl)
                        out[o++] = '\n';
                    found = 1;
                    handled = 1;
                }
            }
        }
        if (!handled) {
            memcpy(out + o, ls, len + (size_t)has_nl);
            o += len + (size_t)has_nl;
        }
        if (in_target && t < te && *t != '#' && *t != ';' &&
            (*t == '[' || memchr(t, '=', (size_t)(te - t))))
            insert_at = (long)o;   /* after the section header or its last assignment */
        p += len + (size_t)has_nl;
    }
    if (!found) {
        char line[300];
        size_t ll;
        if (insert_at >= 0) {
            ll = (size_t)snprintf(line, sizeof line, "%s = %s\n", name, value);
            /* the insertion point may follow a last line without a newline */
            if (insert_at > 0 && out[insert_at - 1] != '\n') {
                memmove(out + insert_at + 1, out + insert_at, o - (size_t)insert_at);
                out[insert_at++] = '\n';
                o++;
            }
            memmove(out + insert_at + ll, out + insert_at, o - (size_t)insert_at);
            memcpy(out + insert_at, line, ll);
            o += ll;
        } else {
            if (o > 0 && out[o - 1] != '\n')
                out[o++] = '\n';
            if (o > 0)
                out[o++] = '\n';
            o += (size_t)snprintf(out + o, cap - o, "[%s]\n%s = %s\n", section, name, value);
        }
    }
    out[o] = '\0';
    return out;
}

int xg_plat_config_persist(const char* key, const char* value)
{
    char path[1024], tmp[1100], section[64];
    const char* dot = strchr(key, '.');
    char* text = NULL;
    char* out;
    long len = 0;
    FILE* f;
    int pass;

    if (dot == NULL || dot == key || value == NULL || strpbrk(value, "\r\n#"))
        return 0;
    snprintf(section, sizeof section, "%.*s", (int)(dot - key), key);
    for (pass = 0; pass < 2; pass++) {
        if (pass == 0) {
            if (xg_plat_config_user_path(path, sizeof path) == NULL)
                continue;
        } else if (!xdg_config_path(path, sizeof path)) {
            break;
        }
        free(text);
        text = NULL;
        f = fopen(path, "rb");
        if (f) {
            if (fseek(f, 0, SEEK_END) == 0 && (len = ftell(f)) >= 0 && fseek(f, 0, SEEK_SET) == 0 &&
                (text = (char*)malloc((size_t)len + 1)) != NULL) {
                len = (long)fread(text, 1, (size_t)len, f);
                text[len] = '\0';
            }
            fclose(f);
            if (text == NULL)
                continue;
        } else if (errno != ENOENT) {
            continue;
        } else {
            text = (char*)calloc(1, 1);
        }
        out = rewrite_key(text ? text : "", section, dot + 1, value);
        if (out == NULL)
            continue;
        make_parent_dirs(path);
        snprintf(tmp, sizeof tmp, "%s.tmp", path);
        f = fopen(tmp, "wb");
        if (f && fwrite(out, 1, strlen(out), f) == strlen(out) && fclose(f) == 0 &&
            rename(tmp, path) == 0) {
            free(out);
            free(text);
            {
                char lk[64];
                size_t i;
                snprintf(lk, sizeof lk, "%s", key);
                for (i = 0; lk[i]; i++)
                    lk[i] = (char)tolower((unsigned char)lk[i]);
                set_entry(lk, value);
            }
            printf("[xg_plat] config: saved %s = %s to %s\n", key, value, path);
            return 1;
        }
        if (f && tmp[0])
            remove(tmp);
        free(out);
        fprintf(stderr, "[xg_plat] config: cannot write %s: %s\n", path, strerror(errno));
    }
    free(text);
    return 0;
}

static const char* env_override(const char* key)
{
    static const struct { const char* key; const char* env; } aliases[] = {
        { "game.speed", "XENO_SPEED" },
    };
    char name[96] = "XENO_";
    size_t i, j = 5;
    const char* v;
    for (i = 0; key[i] && j + 1 < sizeof name; i++)
        name[j++] = key[i] == '.' ? '_' : (char)toupper((unsigned char)key[i]);
    name[j] = '\0';
    v = getenv(name);
    if (v && *v)
        return v;
    for (i = 0; i < sizeof aliases / sizeof aliases[0]; i++) {
        if (strcmp(aliases[i].key, key) == 0 && (v = getenv(aliases[i].env)) && *v)
            return v;
    }
    return NULL;
}

const char* xg_plat_config_get(const char* key)
{
    const char* env;
    int i;
    for (i = s_cli_count - 1; i >= 0; i--)
        if (strcmp(s_cli[i].key, key) == 0)
            return s_cli[i].value;
    env = env_override(key);
    if (env)
        return env;
    for (i = 0; i < s_count; i++)
        if (strcmp(s_entries[i].key, key) == 0)
            return s_entries[i].value;
    return NULL;
}

int xg_plat_config_int(const char* key, int fallback)
{
    const char* v = xg_plat_config_get(key);
    char* end;
    long n;
    if (v == NULL)
        return fallback;
    if (!strcmp(v, "on") || !strcmp(v, "true") || !strcmp(v, "yes"))
        return 1;
    if (!strcmp(v, "off") || !strcmp(v, "false") || !strcmp(v, "no"))
        return 0;
    n = strtol(v, &end, 0);
    if (end == v || *end != '\0') {
        fprintf(stderr, "[xg_plat] config: %s = '%s' is not a number; using %d\n", key, v, fallback);
        return fallback;
    }
    return (int)n;
}

static void check_backend(const char* key)
{
    const char* v = xg_plat_config_get(key);
    if (v && strcmp(v, "psycross") != 0)
        fprintf(stderr, "[xg_plat] config: %s = %s is not available in this build "
                        "(only 'psycross'); using psycross\n", key, v);
}

void xg_plat_config_video(XgPlatVideoConfig* video)
{
    int fps = xg_plat_config_int("video.fps", 30);
    int scale = xg_plat_config_int("video.internal_resolution", 0);
    check_backend("video.backend");
    check_backend("audio.backend");
    check_backend("input.backend");
    /* PsyCross renders at the output size, so the internal resolution is
     * the window size: N x the PSX 320x240 unless width/height are given. */
    if (scale > 0 && scale <= 8) {
        video->window_width = 320 * scale;
        video->window_height = 240 * scale;
    } else if (scale != 0) {
        fprintf(stderr, "[xg_plat] config: video.internal_resolution must be 1..8\n");
    }
    video->window_width = xg_plat_config_int("video.width", video->window_width);
    video->window_height = xg_plat_config_int("video.height", video->window_height);
    video->fullscreen = xg_plat_config_int("video.fullscreen", video->fullscreen);
    video->vsync = xg_plat_config_int("video.vsync", video->vsync);
    video->bilinear = xg_plat_config_int("video.bilinear", video->bilinear);
    video->widescreen = xg_plat_config_int("video.widescreen", video->widescreen);
    if (fps != 30)
        fprintf(stderr, "[xg_plat] config: video.fps = %d needs frame interpolation, which no "
                        "renderer backend has yet; running at the game's native 30 fps\n", fps);
}

void xg_plat_config_apply_runtime(void)
{
    const char* v;
    int b;
    if (xg_plat_config_get("game.speed"))
        xg_plat_timing_set_speed_from(xg_plat_config_int("game.speed", 1), "config");
    if (xg_plat_config_get("game.fast_forward_speed"))
        xg_plat_timing_set_fast_forward_speed(xg_plat_config_int("game.fast_forward_speed", 5));
    if (xg_plat_config_get("audio.volume") &&
        !xg_plat_audio_set_user_volume(xg_plat_config_int("audio.volume", 100)))
        fprintf(stderr, "[xg_plat] config: audio.volume is not supported by this audio "
                        "backend yet; ignored\n");
    for (b = 0; b < XG_PLAT_BTN_COUNT; b++) {
        char key[32];
        snprintf(key, sizeof key, "input.%s", xg_plat_input_button_name((XgPlatButton)b));
        if ((v = xg_plat_config_get(key)) != NULL && !xg_plat_input_bind_key((XgPlatButton)b, v))
            fprintf(stderr, "[xg_plat] config: %s = '%s' is not a key name\n", key, v);
    }
}
