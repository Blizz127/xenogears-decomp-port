#ifndef XENO_MODEL_POLYGON_PROJECTION_H
#define XENO_MODEL_POLYGON_PROJECTION_H

enum PcPortModelDepthMode {
    PCPORT_MODEL_DEPTH_AVERAGE,
    PCPORT_MODEL_DEPTH_MINIMUM,
    PCPORT_MODEL_DEPTH_MAXIMUM
};

int PcPort_DrawModelPolygons(const unsigned char *commands, int count, int nvertices,
                             int packet_bytes, int xy_stride, int depth_mode);

#endif
