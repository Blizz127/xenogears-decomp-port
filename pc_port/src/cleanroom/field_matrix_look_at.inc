#include <stdint.h>
#include <string.h>

static void field_look_read_xyz(VECTOR *copy, const VECTOR *source)
{
    memcpy(copy, source, 3 * sizeof(copy->vx));
}

static int16_t field_look_half(uint32_t bits)
{
    return (int16_t)((int32_t)(bits & 0x7fffU) - (int32_t)(bits & 0x8000U));
}

static int16_t field_look_direction(int32_t start, int32_t end)
{
    uint32_t difference = (uint32_t)end - (uint32_t)start;
    return field_look_half(difference >> 16);
}

static int16_t field_look_scaled_eye(int32_t coordinate)
{
    return field_look_half(3U * ((uint32_t)coordinate >> 16));
}

static void field_look_write_row(volatile MATRIX *matrix, int row, const VECTOR *axis)
{
    matrix->m[row][0] = field_look_half((uint32_t)axis->vx);
    matrix->m[row][1] = field_look_half((uint32_t)axis->vy);
    matrix->m[row][2] = field_look_half((uint32_t)axis->vz);
}

static int32_t field_look_negate(int32_t value)
{
    return value == INT32_MIN ? INT32_MIN : -value;
}

void FieldMatrixLookAt(MATRIX *out, VECTOR *eye, VECTOR *at, VECTOR *up)
{
    VECTOR origin = {0}, direction = {0}, up_axis = {0};
    VECTOR forward = {0}, right = {0}, corrected_up = {0}, cross = {0};
    SVECTOR scaled_eye = {0};
    VECTOR rotated = {0};
    volatile MATRIX *ordered_out = out;

    field_look_read_xyz(&origin, eye);
    field_look_read_xyz(&direction, at);
    field_look_read_xyz(&up_axis, up);
    direction.vx = field_look_direction(origin.vx, direction.vx);
    direction.vy = field_look_direction(origin.vy, direction.vy);
    direction.vz = field_look_direction(origin.vz, direction.vz);
    up_axis.vx = field_look_half((uint32_t)up_axis.vx >> 16);
    up_axis.vy = field_look_half((uint32_t)up_axis.vy >> 16);
    up_axis.vz = field_look_half((uint32_t)up_axis.vz >> 16);

    VectorNormal(&direction, &forward);
    OuterProduct12(&up_axis, &forward, &cross);
    VectorNormal(&cross, &right);
    OuterProduct12(&forward, &right, &cross);
    VectorNormal(&cross, &corrected_up);

    field_look_write_row(ordered_out, 0, &right);
    field_look_write_row(ordered_out, 1, &corrected_up);
    field_look_write_row(ordered_out, 2, &forward);

    /* The new matrix rows can occupy the same bytes as the eye coordinates. */
    field_look_read_xyz(&origin, eye);
    scaled_eye.vx = field_look_scaled_eye(origin.vx);
    scaled_eye.vy = field_look_scaled_eye(origin.vy);
    scaled_eye.vz = field_look_scaled_eye(origin.vz);
    ApplyMatrix(out, &scaled_eye, &rotated);
    ordered_out->t[0] = field_look_negate(rotated.vx);
    ordered_out->t[1] = field_look_negate(rotated.vy);
    ordered_out->t[2] = field_look_negate(rotated.vz);
}
