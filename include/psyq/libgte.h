#ifndef XENO_GEOMETRY_H
#define XENO_GEOMETRY_H

#ifdef XENO_PC_PORT
#include "../../pc_port/include_shim/psyq/libgte.h"
#else
/*
 * Geometry types for the PS1 game build. The layouts correspond to the
 * matrices, input vectors and packed colors documented by PSX-SPX. The host
 * build uses the PsyCross types at the revision in pc_port/psycross.lock.
 */
typedef struct {
    short m[3][3];
    int t[3];
} MATRIX;

typedef struct {
    int vx, vy, vz, pad;
} VECTOR;

typedef struct {
    short vx, vy, vz, pad;
} SVECTOR;

typedef struct {
    short vx, vy;
} DVECTOR;

typedef struct {
    unsigned char r, g, b, cd;
} CVECTOR;

#ifdef __cplusplus
extern "C" {
#endif

/* Geometry entry points used by the game. */
void InitGeom(void);
void SetGeomOffset(int x, int y);
void SetGeomScreen(int distance);
long ReadGeomScreen(void);
void ReadGeomOffset(long *x, long *y);
void SetRotMatrix(MATRIX *matrix);
void SetTransMatrix(MATRIX *matrix);
void PushMatrix(void);
void PopMatrix(void);

MATRIX *RotMatrix(SVECTOR *angles, MATRIX *out);
MATRIX *RotMatrixYXZ(SVECTOR *angles, MATRIX *out);
MATRIX *RotMatrixZYX(SVECTOR *angles, MATRIX *out);
MATRIX *RotMatrixX(int angle, MATRIX *matrix);
MATRIX *RotMatrixY(int angle, MATRIX *matrix);
MATRIX *RotMatrixZ(int angle, MATRIX *matrix);
MATRIX *TransMatrix(MATRIX *matrix, VECTOR *translation);
MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale);
MATRIX *ScaleMatrixL(MATRIX *matrix, VECTOR *scale);
MATRIX *CompMatrix(MATRIX *left, MATRIX *right, MATRIX *out);
MATRIX *MulMatrix(MATRIX *left, MATRIX *right);
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right);
MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *out);

VECTOR *ApplyMatrix(MATRIX *matrix, SVECTOR *in, VECTOR *out);
SVECTOR *ApplyMatrixSV(MATRIX *matrix, SVECTOR *in, SVECTOR *out);
VECTOR *ApplyMatrixLV(MATRIX *matrix, VECTOR *in, VECTOR *out);
VECTOR *ApplyRotMatrix(SVECTOR *in, VECTOR *out);
VECTOR *ApplyRotMatrixLV(VECTOR *in, VECTOR *out);

void RotTrans(SVECTOR *in, VECTOR *out, long *flags);
void RotTransSV(SVECTOR *in, SVECTOR *out, long *flags);
int RotTransPers(SVECTOR *in, int *xy, long *depth_cue, long *flags);
int RotTransPers3(SVECTOR *a, SVECTOR *b, SVECTOR *c,
                  long *xy_a, long *xy_b, long *xy_c, long *depth_cue, long *flags);
int RotTransPers4(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
                  long *xy_a, long *xy_b, long *xy_c, long *xy_d,
                  long *depth_cue, long *flags);
long RotAverage4(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
                 long *xy_a, long *xy_b, long *xy_c, long *xy_d,
                 long *depth_cue, long *flags);
int RotAverageNclip4(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
                     long *xy_a, long *xy_b, long *xy_c, long *xy_d,
                     long *depth_cue, long *depth, long *flags);
int NormalClip(int a, int b, int c);

void SetColorMatrix(MATRIX *matrix);
void SetLightMatrix(MATRIX *matrix);
void SetBackColor(int red, int green, int blue);
void SetFarColor(int red, int green, int blue);
void SetFogNearFar(int near_z, int far_z, int distance);
void NormalColor(SVECTOR *normal, CVECTOR *out);
void NormalColor3(SVECTOR *a, SVECTOR *b, SVECTOR *c,
                  CVECTOR *color_a, CVECTOR *color_b, CVECTOR *color_c);
void NormalColorCol3(SVECTOR *a, SVECTOR *b, SVECTOR *c, CVECTOR *color,
                     CVECTOR *color_a, CVECTOR *color_b, CVECTOR *color_c);

void NormalLightCol(void *normal, void *input_color, void *output_color);

void OuterProduct0(VECTOR *left, VECTOR *right, VECTOR *out);
void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *out);
long VectorNormal(VECTOR *in, VECTOR *out);
VECTOR *Square0(VECTOR *in, VECTOR *out);
int SquareRoot0(int value);
long VectorNormalS(VECTOR *in, SVECTOR *out);
long VectorNormalSS(SVECTOR *in, SVECTOR *out);
int rsin(int angle);
int rcos(int angle);
int ratan2(int y, int x);

#ifdef __cplusplus
}
#endif
#endif
#endif
