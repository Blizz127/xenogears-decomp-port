#ifndef XENO_RENDERER_GEOMETRY_H
#define XENO_RENDERER_GEOMETRY_H

#include <stdint.h>

/* The renderer exchanges packed PS1 coordinates and colors with game memory.
 * The translation starts at byte 20, after the 3x3 signed rotation matrix. */
typedef struct {
    int16_t vx, vy, vz, pad;
} SVECTOR;

typedef struct {
    int32_t vx, vy, vz, pad;
} VECTOR;

typedef struct {
    uint8_t r, g, b, cd;
} CVECTOR;

typedef struct {
    int16_t m[3][3];
    int32_t t[3];
} MATRIX;

#ifdef __cplusplus
extern "C" {
#endif

void InitGeom(void);
void SetGeomOffset(int x, int y);
void SetGeomScreen(int distance);

void PushMatrix(void);
void PopMatrix(void);
void SetRotMatrix(MATRIX *matrix);
void SetTransMatrix(MATRIX *matrix);
void SetLightMatrix(MATRIX *matrix);
void SetColorMatrix(MATRIX *matrix);
void SetBackColor(int red, int green, int blue);
void SetFarColor(int red, int green, int blue);
void SetDQA(int slope);
void SetDQB(int intercept);
void SetFogNear(int near_z, int distance);
void SetFogNearFar(int near_z, int far_z, int distance);

MATRIX *RotMatrix(SVECTOR *angles, MATRIX *out);
MATRIX *RotMatrixYXZ(SVECTOR *angles, MATRIX *out);
MATRIX *RotMatrixZYX_gte(SVECTOR *angles, MATRIX *out);
MATRIX *RotMatrixX(int angle, MATRIX *matrix);
MATRIX *RotMatrixY(int angle, MATRIX *matrix);
MATRIX *RotMatrixZ(int angle, MATRIX *matrix);
MATRIX *ScaleMatrix(MATRIX *matrix, VECTOR *scale);
MATRIX *TransMatrix(MATRIX *matrix, VECTOR *translation);
MATRIX *MulRotMatrix(MATRIX *matrix);
MATRIX *MulMatrix(MATRIX *left, MATRIX *right);
MATRIX *MulMatrix2(MATRIX *left, MATRIX *right);
MATRIX *MulMatrix0(MATRIX *left, MATRIX *right, MATRIX *out);
MATRIX *CompMatrix(MATRIX *left, MATRIX *right, MATRIX *out);

VECTOR *ApplyMatrix(MATRIX *matrix, SVECTOR *in, VECTOR *out);
VECTOR *ApplyMatrixLV(MATRIX *matrix, VECTOR *in, VECTOR *out);
SVECTOR *ApplyMatrixSV(MATRIX *matrix, SVECTOR *in, SVECTOR *out);
VECTOR *ApplyRotMatrix(SVECTOR *in, VECTOR *out);
VECTOR *ApplyRotMatrixLV(VECTOR *in, VECTOR *out);

void RotTrans(SVECTOR *in, VECTOR *out, long *flags);
void RotTransSV(SVECTOR *in, SVECTOR *out, long *flags);
int RotTransPers(SVECTOR *in, int *screen, long *depth_cue, long *flags);
int RotTransPers3(SVECTOR *a, SVECTOR *b, SVECTOR *c,
                  long *xy_a, long *xy_b, long *xy_c, long *depth_cue, long *flags);
int RotTransPers4(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
                  long *xy_a, long *xy_b, long *xy_c, long *xy_d,
                  long *depth_cue, long *flags);
int RotAverageNclip4(SVECTOR *a, SVECTOR *b, SVECTOR *c, SVECTOR *d,
                     long *xy_a, long *xy_b, long *xy_c, long *xy_d,
                     long *depth_cue, long *depth, long *flags);
int NormalClip(int xy_a, int xy_b, int xy_c);

void LocalLight(SVECTOR *normal, VECTOR *light);
void NormalColor(SVECTOR *normal, CVECTOR *out);
void NormalColor3(SVECTOR *a, SVECTOR *b, SVECTOR *c,
                  CVECTOR *color_a, CVECTOR *color_b, CVECTOR *color_c);
void NormalColorCol(SVECTOR *normal, CVECTOR *color, CVECTOR *out);
void NormalColorCol3(SVECTOR *a, SVECTOR *b, SVECTOR *c, CVECTOR *color,
                     CVECTOR *color_a, CVECTOR *color_b, CVECTOR *color_c);
void NormalColorDpq(SVECTOR *normal, CVECTOR *color, int depth_cue, CVECTOR *out);
void ColorCol(VECTOR *light, CVECTOR *color, CVECTOR *out);
void ColorDpq(VECTOR *light, CVECTOR *color, int depth_cue, CVECTOR *out);

int SquareRoot0(int value);
int rsin(int angle);
int rcos(int angle);
int ratan2(int y, int x);

#ifdef __cplusplus
}
#endif

#endif
