void SoundAssignVoiceToChannel(SoundVoiceData *voice, u32 channel)
{
    if (channel < 24u) {
        SoundVoiceData *volatile *const slot = &g_SoundChannels[channel];
        SoundVoiceData *const incumbent = *slot;
        volatile SoundVoiceData *requested = voice;

        if (incumbent == voice) {
            return;
        }
        if (!requested) {
            requested = (SoundVoiceData *)PSX_ADDR(0);
        }
        if (incumbent) {
            const s16 currentPriority = ((volatile SoundVoiceData *)incumbent)->priority;
            const s16 requestPriority = requested->priority;

            if (requestPriority < currentPriority) {
                return;
            }
        }
        requested->assignedVoice = (s16)channel;
        *slot = voice;
    }
}

void SoundAbortVoiceOnChannel(SoundVoiceData *voice, u32 channel)
{
    if (channel < 24u) {
        SoundVoiceData *volatile *const slot = &g_SoundChannels[channel];

        if (*slot == voice) {
            const u32 bit = 1u << channel;
            volatile u32 *const pending = (volatile u32 *)&g_unk_VoicesNeedingProcessing;
            volatile u32 *const keyOn = &g_SoundKeyOnFlags;

            *pending = *pending | bit;
            *keyOn = *keyOn & ~bit;
        }
    }
}

void SoundAbortAllVoices(AudioManager *manager)
{
    u32 remaining = ((volatile AudioManager *)manager)->elementCount;
    if (remaining == 0) {
        return;
    }
    uintptr_t address = (uintptr_t)&manager->elements[0];

    do {
        AudioElement *const element = (AudioElement *)address;
        const volatile AudioElement *const fields = element;

        if (fields->active_flag) {
            const u32 channel = fields->voice_number;

            SoundAbortVoiceOnChannel(&element->voice_data, channel);
        }
        address += sizeof(AudioElement);
    } while (--remaining);
}

void SoundReleaseAllVoices(AudioManager *manager)
{
    u32 remaining = ((volatile AudioManager *)manager)->elementCount;
    if (remaining == 0) {
        return;
    }
    uintptr_t address = (uintptr_t)&manager->elements[0];

    do {
        AudioElement *const element = (AudioElement *)address;
        const u32 channel = ((const volatile AudioElement *)element)->voice_number;

        SoundReleaseVoiceFromChannel(&element->voice_data, channel);
        address += sizeof(AudioElement);
    } while (--remaining);
}
