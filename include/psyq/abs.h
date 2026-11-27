#ifndef XENO_PSX_ABS_H
#define XENO_PSX_ABS_H

/* The battle steering code takes the magnitude of a sign-extended angle. */
#define abs(angle) __builtin_abs(angle)

#endif
