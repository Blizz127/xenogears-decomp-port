/* SPDX-License-Identifier: MIT
 * Native pitch lookup derived from observed note-to-frequency responses.
 * Both tuning tables are hydrated from the player's executable. */
s16 func_8003EEA0(u32 note)
{
    const unsigned key = (note >> 8) & 0x7f;
    const unsigned fraction = note & 0xff;
    unsigned tuning;
    if (key < sizeof(D_80050B78)) {
        tuning = D_80050B78[key];
    } else {
        /* The final eight key numbers reach the adjacent tuning data on disc.
         * Select the little-endian byte explicitly across host allocations. */
        const unsigned offset = key - sizeof(D_80050B78);
        const u16 packed = (u16)D_80050BF0[offset / 2];
        tuning = (packed >> (8 * (offset % 2))) & 0xff;
    }
    const unsigned semitone = tuning & 0x0f;
    const unsigned octave = tuning >> 4;
    if (semitone >= 12)
        return 0;

    const s16 sample = D_80050BF0[256 * semitone + fraction];
    if (octave < 6) {
        const int divisor = 1 << (6 - octave);
        /* Quantize toward minus infinity for negative tuning offsets. */
        return (s16)(sample < 0 ? -((-sample + divisor - 1) / divisor)
                                : sample / divisor);
    }
    return (s16)((u32)(u16)sample << (octave - 6));
}
