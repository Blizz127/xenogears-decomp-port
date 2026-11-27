/* Independently written against the public PsyCross GTE interface at
 * e56e4cde1c2b8a15e0d4e38b26cdd9202e0d17e6. See include/psx/gtemac.h
 * (gte_OuterProduct0/12, gte_Square0) and include/psx/inline_c.h for the
 * operand, accumulator and command interfaces. */
static void sdk_vector_load_ir(const VECTOR *vector)
{
    uint32_t xyz[3];
    memcpy(xyz, vector, sizeof(xyz));
    MTC2(xyz[0], 9);
    MTC2(xyz[1], 10);
    MTC2(xyz[2], 11);
}

static void sdk_vector_store_mac(VECTOR *result)
{
    uint32_t xyz[3] = { MFC2(25), MFC2(26), MFC2(27) };
    memcpy(result, xyz, sizeof(xyz));
}

VECTOR *Square0(VECTOR *input, VECTOR *result)
{
    sdk_vector_load_ir(input);
    gte_sqr0();
    sdk_vector_store_mac(result);
    return result;
}

static void sdk_vector_cross(const VECTOR *left, const VECTOR *right,
                             VECTOR *result, int shift12)
{
    /* OP uses the rotation diagonal as one of its input vectors. The
     * callable interface preserves these three packed matrix registers. */
    unsigned int rot0 = CFC2(0), rot2 = CFC2(2), rot4 = CFC2(4);
    uint32_t xyz[3];
    memcpy(xyz, left, sizeof(xyz));
    CTC2(xyz[0], 0);
    CTC2(xyz[1], 2);
    CTC2(xyz[2], 4);
    sdk_vector_load_ir(right);
    if (shift12) {
        gte_op12();
    } else {
        gte_op0();
    }
    sdk_vector_store_mac(result);
    CTC2(rot0, 0);
    CTC2(rot2, 2);
    CTC2(rot4, 4);
}

void OuterProduct0(VECTOR *left, VECTOR *right, VECTOR *result)
{
    sdk_vector_cross(left, right, result, 0);
}

void OuterProduct12(VECTOR *left, VECTOR *right, VECTOR *result)
{
    sdk_vector_cross(left, right, result, 1);
}
