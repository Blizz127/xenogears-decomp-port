#include "robo_menu_field_visit.h"

int PcPort_RoboMenuDispatch(void)
{
    return 0;
}

int PcPort_RoboMenuFieldVisitPending(void)
{
    return 0;
}

void PcPort_RoboMenuFieldVisitPoll(void)
{
    /* A build without the shop never suspends field updates for a visit. */
}

int PcPort_RoboMenuFieldOuterAvailable(void *allocation, uint32_t bytes)
{
    (void)allocation;
    (void)bytes;
    /* This provider cannot verify ownership of field heap allocations. */
    return 0;
}

PcRoboFieldVisitResult PcPort_RoboMenuFieldVisitRetry(void)
{
    /* No shop acquired any field resources, so there is nothing to release. */
    return (PcRoboFieldVisitResult){0, 0};
}
