s32 SoundCalculateAudioManagerSize(s32 elementCount)
{
    const u32 headerBytes = 148U;
    const u32 recordBytes = 344U;
    return (s32)(headerBytes + (u32)elementCount * recordBytes);
}
