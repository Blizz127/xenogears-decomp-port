#ifndef PC_PORT_BOOT_STR_H
#define PC_PORT_BOOT_STR_H

/* Retail Opening STR (archive file 15 / str0014): 320x224 MDEC bitstream.
 * Boot feeds those sectors, VLC-expands, software-MDECs, and blits frames.
 * Skip-card font is not this path. */

int PcPort_BootStrLoad(void);
int PcPort_BootStrFrameCount(void);
int PcPort_BootStrLastSectors(void);
int PcPort_BootStrFinished(void);
int PcPort_BootStrPlayFrame(int frame_index);
int PcPort_BootStrPlayTitleFrame(void);
/* Queue 16-bit POLY_FT4s that sample the last PlayFrame tpage. When ot is
 * non-NULL they are AddPrim'd (same path as the boot cursor); otherwise
 * DrawPrim. */
void PcPort_BootStrDrawPrims(void* ot);
int PcPort_BootStrWriteBmp(const char* path);
void PcPort_BootStrUnload(void);

#endif
