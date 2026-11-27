#ifndef XENO_PC_PORT_MOVIE_IDCT_H
#define XENO_PC_PORT_MOVIE_IDCT_H

#include <stdint.h>

void PcPort_MovieIdct(int32_t block[64], const int16_t scale[64]);

#endif
