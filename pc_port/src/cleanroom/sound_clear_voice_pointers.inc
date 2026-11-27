void SoundClearVoiceDataPointers(void)
{
    SoundVoiceData **slot = g_SoundChannels;
    SoundVoiceData **end = slot + NUM_VOICES;
    while (slot != end) {
        *slot++ = NULL;
    }
}
