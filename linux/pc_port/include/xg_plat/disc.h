/* xg_plat/disc.h -- disc/storage interface of the moddable port layer.
 *
 * Meaning-level access to "the user's copy of the game": the Disc 1 image
 * (raw MODE2/2352 BIN, found via XENO_DISC or disc/), the retail files the
 * matching build extracts from it (SLUS_006.64, field.bin, ..., found via
 * XENO_DATA_DIR / XENO_SLUS or disc/). A Sony BIOS is optional and is used
 * only when explicitly selected with XENO_BIOS. Nothing from the game is
 * shipped; retail data is read from the user's files and checked against hashes.
 *
 * Layers (see pc_port/README.md "Port layers"): game/port code calls these
 * functions; the default backend (pc_port/src/plat/xg_plat_disc_default.h,
 * weak symbols over retail_data.h, disc_check.c and plain stdio) is pulled
 * in by this header so every TU and test links it without extra inputs.  A
 * different backend replaces it by defining the same functions (strong
 * symbols win at link time).  The PsyCross CD layer (libcd emulation used
 * by the game's archive code) is opened on xg_plat_disc_image_path().
 */
#ifndef XG_PLAT_DISC_H
#define XG_PLAT_DISC_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define XG_PLAT_SECTOR_USER 2048u
#define XG_PLAT_SECTOR_RAW  2352u

/* Path of the Disc 1 image, or NULL when none is found.  Resolved once. */
const char* xg_plat_disc_image_path(void);

/* First-run check of the image against the Redump dump (cached after the
 * first success).  0 = OK (or overridden / no checker linked), -1 = wrong
 * image (already explained on stderr). */
int xg_plat_disc_verify(void);

/* Read `count` 2048-byte user sectors starting at `lba` from the image.
 * 0 on success, -1 on failure. */
int xg_plat_disc_read_sectors(uint32_t lba, uint32_t count, void* out);

/* Resolve an extracted retail file or the BIOS ("SLUS_006.64", "field.bin",
 * "world_map.bin", "scph5500.bin", ...) to a readable path.  Returns 1 and
 * fills `out`, or 0 when no candidate exists. */
int xg_plat_data_file_path(const char* name, char* out, size_t out_size);

/* The verified contents of a known retail file (hash-checked against
 * config/checksum.sha, loaded once, owned by the layer).  A missing or
 * wrong file stops the program with an explanation (see retail_data.h). */
const uint8_t* xg_plat_data_file(const char* name, size_t* size);

/* The game's CD drive commands and data reads (libcd CdControl / CdControlB /
 * CdControlF / CdRead calls arrive here by link-time routing,
 * build_port.sh XG_PLAT_WRAP_SYMS).  `com` / `param` / `result` are the CD
 * drive's command byte, parameter bytes and result bytes; `mode` is the read
 * mode.  Implemented by the drive backend (PsyCross: src/plat/xg_plat_psycross.c),
 * not by the header default. */
typedef enum {
    XG_PLAT_CD_ASYNC,     /* CdControl: issue, completion by callback/sync */
    XG_PLAT_CD_BLOCKING,  /* CdControlB: wait for the command to complete */
    XG_PLAT_CD_NO_RESULT  /* CdControlF: issue, no result bytes */
} XgPlatCdWait;
int xg_plat_disc_cd_control(unsigned char com, unsigned char* param, unsigned char* result,
                            XgPlatCdWait wait);
int xg_plat_disc_cd_read(int sectors, void* buf, int mode);

#ifdef __cplusplus
}
#endif

#include "../../src/plat/xg_plat_disc_default.h"

#endif /* XG_PLAT_DISC_H */
