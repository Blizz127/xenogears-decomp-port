#ifndef XENO_WORLD_MAP_UNAVAILABLE_H
#define XENO_WORLD_MAP_UNAVAILABLE_H

#include <stdio.h>
#include <stdlib.h>

_Noreturn static inline void PcPort_WorldMapUnavailable(const char *entry)
{
    fprintf(stderr, "[xeno-port] Unavailable world-map routine reached: %s\n", entry);
    exit(EXIT_FAILURE);
}

#endif
