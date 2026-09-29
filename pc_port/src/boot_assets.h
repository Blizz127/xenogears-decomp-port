#ifndef PC_PORT_BOOT_ASSETS_H
#define PC_PORT_BOOT_ASSETS_H

/* Retail boot blobs from disc1:
 *   intro STR = archive file 15 (Opening / str0014, LBA 54133)
 *   title graphic = last high-detail picture of that Opening STR (not the
 *   fade-to-black hold at the tail)
 * Same loader BootUiTick / BootStrLoad / the boot certificate call.
 * File 2 is the short capsule-launch STR, not the opening. */

#define PC_PORT_BOOT_INTRO_STR_ENTRY 15
#define PC_PORT_BOOT_INTRO_STR_PREFIX_SECTORS 32
#define PC_PORT_BOOT_STR_INDEX_MAX 128
/* Last Opening pictures drop to a ~40-frame solid hold (flen 2936). Freeze
 * the last picture whose bitstream is still a real scene, not that hold. */
#define PC_PORT_BOOT_TITLE_MIN_FLEN 12000
#define PC_PORT_BOOT_TITLE_SCAN_SECTORS 2500

int PcPort_BootAssetsLoad(void);
void PcPort_BootAssetsUnload(void);
int PcPort_BootAssetsLoaded(void);
const unsigned char* PcPort_BootIntroStrPayload(int* size);
const unsigned char* PcPort_BootTitleGraphicPayload(int* size);
int PcPort_BootTitleUsedRetailGraphic(void);
int PcPort_BootIntroStrLba(void);
int PcPort_BootIntroStrFileSize(void);
int PcPort_BootIntroStrSectorCount(void);
int PcPort_BootIntroStrEntry(void);
int PcPort_BootTitleFrameSectorOff(void);
int PcPort_BootTitleFrameSectors(void);
int PcPort_BootTitleFrameLen(void);
int PcPort_BootStrIndexCount(void);
int PcPort_BootStrIndexOff(int i);
int PcPort_BootStrIndexNs(int i);
int PcPort_BootStrIndexLen(int i);
int PcPort_BootDiscReadSectors(int lba, int count, void* dst);

#endif
