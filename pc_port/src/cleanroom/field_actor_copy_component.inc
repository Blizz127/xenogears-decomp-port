#include <stdint.h>
#include <string.h>

static inline void CopyActorFieldS32(FieldActor *actors, int destination, int source, int offset)
{
    if (source < 0 || destination < 0 || (unsigned)offset > sizeof(FieldActor) - sizeof(uint32_t))
        return;
    uint32_t component;
    memcpy(&component, (const uint8_t *)&actors[source] + offset, sizeof component);
    memcpy((uint8_t *)&actors[destination] + offset, &component, sizeof component);
}
