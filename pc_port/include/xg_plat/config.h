/* xg_plat/config.h -- port configuration (moddable port layer).
 *
 * config.ini: one `key = value` per line, `#` or `;` comments, optional
 * `[section]` headers that prefix keys ("[video]" + "width = 1280" ==
 * "video.width").  The first file found is used:
 *   --config FILE, $XENO_CONFIG, config.ini next to the executable,
 *   ${XDG_CONFIG_HOME:-~/.config}/xenogears-port/config.ini
 * Precedence per key: --set key=value  >  environment  >  file.  The
 * environment name is XENO_<KEY> with dots as underscores, upper-case
 * ("video.width" -> XENO_VIDEO_WIDTH); historical names stay aliases
 * (XENO_SPEED == game.speed).  Keys: pc_port/config.example.ini.
 */
#ifndef XG_PLAT_CONFIG_H
#define XG_PLAT_CONFIG_H

#include "renderer.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Command line: --config FILE, --set key=value (repeatable), --help.
 * Recognised options are consumed; returns 0 to continue, 1 when --help
 * was printed, -1 on a usage error (message printed). */
int xg_plat_config_cli(int argc, char** argv);

/* Load the config file (once).  Returns the number of keys read, 0 when no
 * file exists, -1 on a read error. */
int xg_plat_config_load(void);
/* Path of the loaded file, or NULL. */
const char* xg_plat_config_path(void);
/* Value for `key` (environment override first), or NULL. */
const char* xg_plat_config_get(const char* key);
int xg_plat_config_int(const char* key, int fallback);
/* Parse text directly (tests; replaces any loaded values). */
int xg_plat_config_parse(const char* text);

/* Apply the [video] keys to a renderer config before xg_plat_renderer_init. */
void xg_plat_config_video(XgPlatVideoConfig* video);
/* Apply game.speed, audio.volume and input.* after the backend is up. */
void xg_plat_config_apply_runtime(void);

#ifdef __cplusplus
}
#endif

#endif
