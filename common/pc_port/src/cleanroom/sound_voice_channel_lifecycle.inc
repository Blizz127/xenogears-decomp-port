static void sound_assign_voice_to_channel(SoundVoiceData *voice, u32 channel, int play)
{
    if (channel >= 24) {
        return;
    }

    SoundVoiceData *volatile *slot = &g_SoundChannels[channel];
    volatile SoundVoiceData *owner = *slot;
    volatile s32 *processing = &g_unk_VoicesNeedingProcessing;
    u32 bit = 1u << channel;

    if (owner != voice) {
        volatile SoundVoiceData *requested = voice;
        if (requested == NULL) {
            requested = (SoundVoiceData *)PSX_ADDR(0);
        }
        if (owner != NULL) {
            s16 oldPriority = owner->priority;
            s16 newPriority = requested->priority;
            if (oldPriority > newPriority) {
                return;
            }
        }

        requested->flags = 0xffff;
        s32 pending = *processing;
        requested->assignedVoice = (s16)channel;
        *slot = voice;
        *processing = pending | (s32)bit;
    } else if (!play) {
        *processing |= (s32)bit;
        return;
    }

    volatile u32 *keyOn = &g_SoundKeyOnFlags;
    u32 keys = *keyOn;
    *keyOn = play ? keys | bit : keys & ~bit;
}

void SoundAssignVoiceToChannelAndStop(SoundVoiceData *voice, u32 channel)
{
    sound_assign_voice_to_channel(voice, channel, 0);
}

void SoundAssignVoiceToChannelAndPlay(SoundVoiceData *voice, u32 channel)
{
    sound_assign_voice_to_channel(voice, channel, 1);
}

void SoundReleaseVoiceFromChannel(SoundVoiceData *voice, uint channel)
{
    if (channel >= 24) {
        return;
    }
    SoundVoiceData *volatile *slot = &g_SoundChannels[channel];
    if (*slot == voice) {
        u32 bit = 1u << channel;
        *slot = NULL;
        *(volatile s32 *)&g_unk_VoicesNeedingProcessing |= (s32)bit;
        *(volatile u32 *)&g_SoundKeyOnFlags &= ~bit;
    }
}

void SoundStopVoiceOnChannel(SoundVoiceData *voice, u32 channel)
{
    if (channel >= 24) {
        return;
    }
    SoundVoiceData *volatile *slot = &g_SoundChannels[channel];
    if (*slot == voice) {
        *(volatile u32 *)&g_SoundKeyOffFlags |= 1u << channel;
    }
}
