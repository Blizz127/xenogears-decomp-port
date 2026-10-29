/* The battle highlight frame block in the main executable, immediately before
 * WorkListsFreeAllEntries. The owner supplies the verified retail executable;
 * the bytes are loaded at startup and are never compiled into the port. */
unsigned char D_8001C76C[0x170];
#include "retail_data.h"
_Static_assert(sizeof(D_8001C76C)==0x170, "popup frame block size");
XENO_RETAIL_DATA_BEGIN(battle_popup_frame)
    XENO_RD(D_8001C76C, XENO_RD_SLUS, XENO_RD_SLUS_OFF(0x8001c76cu), 0x170),
XENO_RETAIL_DATA_END(battle_popup_frame)
