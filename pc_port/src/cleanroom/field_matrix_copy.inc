#include <stddef.h>

__attribute__((noinline)) void FieldMatrixCopyTransform(MATRIX *destination, MATRIX *source) {
    typedef __typeof__(source->m[0]) RotationRow;
    const volatile RotationRow *input = (const volatile RotationRow *)
        ((const unsigned char *)source + __builtin_offsetof(MATRIX, m));
    volatile RotationRow *output = (volatile RotationRow *)
        ((unsigned char *)destination + __builtin_offsetof(MATRIX, m));

    for (int row = 0; row < 3; ++row) {
        for (int column = 0; column < 3; ++column) {
            output[row][column] = input[row][column];
        }
    }
}

__attribute__((noinline)) void FieldMatrixCopyTranslation(MATRIX *destination, MATRIX *source) {
    const volatile __typeof__(source->t[0]) *input = source->t;
    volatile __typeof__(destination->t[0]) *output = destination->t;

    for (int axis = 0; axis < 3; ++axis) {
        output[axis] = input[axis];
    }
}

void FieldMatrixCopy(MATRIX *destination, MATRIX *source) {
    FieldMatrixCopyTransform(destination, source);
    FieldMatrixCopyTranslation(destination, source);
}
