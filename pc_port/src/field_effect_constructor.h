#ifndef XENO_FIELD_EFFECT_CONSTRUCTOR_H
#define XENO_FIELD_EFFECT_CONSTRUCTOR_H
#include <stdint.h>
/* Native translation of retail 801E0A00. The last argument is the actual
 * incoming MIPS s1, which retail consumes on negative pattern remainders.
 * It is mandatory: never supply a guessed default. E5D44 supplies its parent
 * object's retail address. This is not a replacement 19-argument guest ABI. */
uint8_t* PcPort_FieldEffectConstructWithS1(uint8_t* effect, uint8_t* parent,
    int32_t type, int32_t flags, uint8_t* source,
    int32_t ax, int32_t ay, int32_t az, int32_t bx, int32_t by, int32_t bz,
    int32_t x, int32_t y, int32_t width, int32_t height,
    int32_t period, int32_t parameter0, int32_t parameter1,
    uint32_t callback, uint32_t incomingS1);
#endif
