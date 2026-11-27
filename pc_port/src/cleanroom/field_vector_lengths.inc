#ifdef XENO_PC_PORT
extern VECTOR *Square0(VECTOR *input, VECTOR *output);
#endif

static s32 FieldMagnitudeSignedWord(u32 word) {
    if (word <= 0x7fffffffu)
        return (s32)word;
    return -1 - (s32)~word;
}

long FieldGetVec1Magnitude(long x) {
    /* Inactive components have no bearing on the result. Initializing them
     * replaces incidental original stack contents passed to the GTE. */
    VECTOR input = {FieldMagnitudeSignedWord((u32)x), 0, 0, 0};
    VECTOR squared;
    Square0(&input, &squared);
    return SquareRoot0(squared.vx);
}

long FieldGetVec2Magnitude(long x, long y) {
    VECTOR input = {
        FieldMagnitudeSignedWord((u32)x), FieldMagnitudeSignedWord((u32)y), 0, 0
    };
    VECTOR squared;
    u32 sum;
    Square0(&input, &squared);
    sum = (u32)squared.vx + (u32)squared.vy;
    return SquareRoot0(FieldMagnitudeSignedWord(sum));
}

long FieldGetVec3Magnitude(long x, long y, long z) {
    VECTOR input = {
        FieldMagnitudeSignedWord((u32)x), FieldMagnitudeSignedWord((u32)y),
        FieldMagnitudeSignedWord((u32)z), 0
    };
    VECTOR squared;
    u32 sum;
    Square0(&input, &squared);
    sum = (u32)squared.vx + (u32)squared.vy + (u32)squared.vz;
    return SquareRoot0(FieldMagnitudeSignedWord(sum));
}
