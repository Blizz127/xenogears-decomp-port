#ifndef _XENO_MATH_H
#define _XENO_MATH_H

// PSX Angle format denotes 360 degrees as 4096.

// 1 degree represented as a PSX degree
#define PSX_DEGREE 11.377777777777778

// Degrees to PSX Angle format
#define PSX_DEGREES(degrees) (int)(degrees * PSX_DEGREE) & 0xFFF

// Convert value to PSX Angle format
#define PSX_ANGLE(value) value & 0xFFF

// Convert PSX Angle to index of 8-directional movement
#define PSX_ANGLE_TO_DIRECTION_8(angle) (angle >> 9)



// Possible bad names, but often the game shifts vector values down by 16 bytes
// before they are used with GTE and/or GPU-related functions.
#define CONV_TO_GTE(number) (number >> 0x10)
#define CONV_FROM_GTE(number) (number << 0x10)

#endif