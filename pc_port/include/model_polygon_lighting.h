#ifndef XENO_MODEL_POLYGON_LIGHTING_H
#define XENO_MODEL_POLYGON_LIGHTING_H

enum PcPortModelLightingMode {
    PCPORT_MODEL_LIT_F3,
    PCPORT_MODEL_LIT_FT3,
    PCPORT_MODEL_LIT_FT4,
    PCPORT_MODEL_LIT_F4,
    PCPORT_MODEL_LIT_G3
};

int PcPort_DrawLitModelPolygons(const unsigned char *commands, int count, int lighting_mode);

#endif
