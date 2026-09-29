#ifndef PC_PORT_FEI_HD2D_H
#define PC_PORT_FEI_HD2D_H

typedef struct PcPortFeiHd2dBatch {
    void *sprite;
    void *prims[64];
    void *ots[64];
    int active, count, walking;
} PcPortFeiHd2dBatch;

void PcPort_FeiHd2dBegin(PcPortFeiHd2dBatch *batch, void *sprite, int parts);
int PcPort_FeiHd2dCapture(PcPortFeiHd2dBatch *batch, void *poly, void *ot);
void PcPort_FeiHd2dEnd(PcPortFeiHd2dBatch *batch);
int PcPort_FeiHd2dEnabled(void);
void PcPort_FeiHd2dToggle(void);
#endif
