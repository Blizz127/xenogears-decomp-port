/* Retail TU 0x8001B970..0x8001BDDC, one of the three TUs merged into system/temp3.c
 * (see the note above its .sbss definitions). Its source stays in temp3.c,
 * selected with TEMP3_PART_C; the port builds it from temp3.c, so this TU is empty
 * there. */
#ifndef XENO_PC_PORT
#define TEMP3_PART_C
#include "temp3.c"
#endif
