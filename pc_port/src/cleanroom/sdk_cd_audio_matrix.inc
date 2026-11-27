/* The four CdlATV bytes are the drive's channel routing levels. The host
 * mixer accepts the same field order and owns the audio thread lock. */
extern void PsyX_SPUAL_SetCdAttenuation(unsigned char, unsigned char,
                                        unsigned char, unsigned char);

int CdMix(CdlATV *matrix)
{
    if (!matrix)
        return 0;

    PsyX_SPUAL_SetCdAttenuation(matrix->val0, matrix->val1,
                               matrix->val2, matrix->val3);
    return 1;
}
