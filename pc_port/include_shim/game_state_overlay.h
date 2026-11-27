#ifndef PC_PORT_GAME_STATE_OVERLAY_H
#define PC_PORT_GAME_STATE_OVERLAY_H

typedef struct {
    int file;
    int logicalSize;
    int sourceSize;
    int imageSize;
} PcPortOverlayLayout;

const PcPortOverlayLayout *PcPort_GetOverlayLayout(unsigned int state);
int PcPort_ArchiveReadGameOverlay(unsigned int state, void *destination, unsigned int capacity, unsigned int *sourceSize);
int PcPort_DecodeGameOverlay(unsigned int state, const void *src, unsigned int srcSize, void *dst, unsigned int dstSize);
int PcPort_DecodeCachedGameOverlay(unsigned int state, void *dst, unsigned int dstSize);

#endif
