#ifndef XENO_ROBO_MENU_FIELD_VISIT_H
#define XENO_ROBO_MENU_FIELD_VISIT_H

#include <stdint.h>

typedef struct {
    uint32_t unavailable;
    uint32_t held;
} PcRoboFieldVisitResult;

int PcPort_RoboMenuDispatch(void);
int PcPort_RoboMenuFieldVisitPending(void);
void PcPort_RoboMenuFieldVisitPoll(void);
int PcPort_RoboMenuFieldOuterAvailable(void *allocation, uint32_t bytes);
PcRoboFieldVisitResult PcPort_RoboMenuFieldVisitRetry(void);

int PcPort_FieldGearMenuClosePending(void);
void PcPort_FieldGearMenuStockFrameReturned(void);

#endif
