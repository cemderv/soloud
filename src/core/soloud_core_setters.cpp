/*
SoLoud audio engine
Copyright (c) 2013-2015 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.
*/

#include "soloud_internal.hpp"

// Setters - set various bits of SoLoud state

namespace SoLoud
{
void Soloud::setPostClipScaler(float aScaler)
{
    mPostClipScaler = aScaler;
}

void Soloud::setMainResampler(RESAMPLER aResampler)
{
    mResampler = aResampler;
}

void Soloud::setGlobalVolume(float aVolume)
{
    mGlobalVolumeFader.mActive = 0;
    mGlobalVolume              = aVolume;
}

void Soloud::setRelativePlaySpeed(handle aVoiceHandle, float aSpeed)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mRelativePlaySpeedFader.mActive = 0;
        setVoiceRelativePlaySpeed_internal(ch, aSpeed);
    FOR_ALL_VOICES_POST
}

void Soloud::setSamplerate(handle aVoiceHandle, float aSamplerate)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mBaseSamplerate = aSamplerate;
        updateVoiceRelativePlaySpeed_internal(ch);
    FOR_ALL_VOICES_POST
}

void Soloud::setPause(handle aVoiceHandle, bool aPause)
{
    FOR_ALL_VOICES_PRE
        setVoicePause_internal(ch, aPause);
    FOR_ALL_VOICES_POST
}

void Soloud::setMaxActiveVoiceCount(unsigned int aVoiceCount)
{
    assert(aVoiceCount > 0);
    assert(aVoiceCount <= VOICE_COUNT);

    lockAudioMutex_internal();
    mMaxActiveVoices = aVoiceCount;

    mResampleData.resize(aVoiceCount * 2);
    mResampleDataOwner.resize(aVoiceCount);

    mResampleDataBuffer = AlignedFloatBuffer{SAMPLE_GRANULARITY * MAX_CHANNELS * aVoiceCount * 2};

    for (size_t i        = 0; i < aVoiceCount * 2; i++)
        mResampleData[i] = mResampleDataBuffer.mData + (SAMPLE_GRANULARITY * MAX_CHANNELS * i);

    for (size_t i             = 0; i < aVoiceCount; i++)
        mResampleDataOwner[i] = nullptr;

    mActiveVoiceDirty = true;
    unlockAudioMutex_internal();
}

void Soloud::setPauseAll(bool aPause)
{
    lockAudioMutex_internal();
    int ch;
    for (ch = 0; ch < (signed)mHighestVoice; ch++)
    {
        setVoicePause_internal(ch, aPause);
    }
    unlockAudioMutex_internal();
}

void Soloud::setProtectVoice(handle aVoiceHandle, bool aProtect)
{
    FOR_ALL_VOICES_PRE
        if (aProtect)
        {
            mVoice[ch]->mFlags |= AudioSourceInstanceFlags::PROTECTED;
        }
        else
        {
            mVoice[ch]->mFlags &= ~AudioSourceInstanceFlags::PROTECTED;
        }
    FOR_ALL_VOICES_POST
}

void Soloud::setPan(handle aVoiceHandle, float aPan)
{
    FOR_ALL_VOICES_PRE
        setVoicePan_internal(ch, aPan);
    FOR_ALL_VOICES_POST
}

void Soloud::setChannelVolume(handle aVoiceHandle, unsigned int aChannel, float aVolume)
{
    FOR_ALL_VOICES_PRE
        if (mVoice[ch]->mChannels > aChannel)
        {
            mVoice[ch]->mChannelVolume[aChannel] = aVolume;
        }
    FOR_ALL_VOICES_POST
}

void Soloud::setPanAbsolute(handle aVoiceHandle, float aLVolume, float aRVolume)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mPanFader.mActive = 0;
        mVoice[ch]->mChannelVolume[0] = aLVolume;
        mVoice[ch]->mChannelVolume[1] = aRVolume;
        if (mVoice[ch]->mChannels == 4)
        {
            mVoice[ch]->mChannelVolume[2] = aLVolume;
            mVoice[ch]->mChannelVolume[3] = aRVolume;
        }
        if (mVoice[ch]->mChannels == 6)
        {
            mVoice[ch]->mChannelVolume[2] = (aLVolume + aRVolume) * 0.5f;
            mVoice[ch]->mChannelVolume[3] = (aLVolume + aRVolume) * 0.5f;
            mVoice[ch]->mChannelVolume[4] = aLVolume;
            mVoice[ch]->mChannelVolume[5] = aRVolume;
        }
        if (mVoice[ch]->mChannels == 8)
        {
            mVoice[ch]->mChannelVolume[2] = (aLVolume + aRVolume) * 0.5f;
            mVoice[ch]->mChannelVolume[3] = (aLVolume + aRVolume) * 0.5f;
            mVoice[ch]->mChannelVolume[4] = aLVolume;
            mVoice[ch]->mChannelVolume[5] = aRVolume;
            mVoice[ch]->mChannelVolume[6] = aLVolume;
            mVoice[ch]->mChannelVolume[7] = aRVolume;
        }
    FOR_ALL_VOICES_POST
}

void Soloud::setInaudibleBehavior(handle aVoiceHandle, bool aMustTick, bool aKill)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mFlags &=
            ~(AudioSourceInstanceFlags::INAUDIBLE_KILL | AudioSourceInstanceFlags::INAUDIBLE_TICK);
        if (aMustTick)
        {
            mVoice[ch]->mFlags |= AudioSourceInstanceFlags::INAUDIBLE_TICK;
        }
        if (aKill)
        {
            mVoice[ch]->mFlags |= AudioSourceInstanceFlags::INAUDIBLE_KILL;
        }
    FOR_ALL_VOICES_POST
}

void Soloud::setLoopPoint(handle aVoiceHandle, time_t aLoopPoint)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mLoopPoint = aLoopPoint;
    FOR_ALL_VOICES_POST
}

void Soloud::setLooping(handle aVoiceHandle, bool aLooping)
{
    FOR_ALL_VOICES_PRE
        if (aLooping)
        {
            mVoice[ch]->mFlags |= AudioSourceInstanceFlags::LOOPING;
        }
        else
        {
            mVoice[ch]->mFlags &= ~AudioSourceInstanceFlags::LOOPING;
        }
    FOR_ALL_VOICES_POST
}

void Soloud::setAutoStop(handle aVoiceHandle, bool aAutoStop)
{
    FOR_ALL_VOICES_PRE
        if (aAutoStop)
        {
            mVoice[ch]->mFlags &= ~AudioSourceInstanceFlags::DISABLE_AUTOSTOP;
        }
        else
        {
            mVoice[ch]->mFlags |= AudioSourceInstanceFlags::DISABLE_AUTOSTOP;
        }
    FOR_ALL_VOICES_POST
}

void Soloud::setVolume(handle aVoiceHandle, float aVolume)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mVolumeFader.mActive = 0;
        setVoiceVolume_internal(ch, aVolume);
    FOR_ALL_VOICES_POST
}

void Soloud::setDelaySamples(handle aVoiceHandle, unsigned int aSamples)
{
    FOR_ALL_VOICES_PRE
        mVoice[ch]->mDelaySamples = aSamples;
    FOR_ALL_VOICES_POST
}

void Soloud::setVisualizationEnable(bool aEnable)
{
    if (aEnable)
    {
        mFlags |= FLAGS::ENABLE_VISUALIZATION;
    }
    else
    {
        mFlags &= ~FLAGS::ENABLE_VISUALIZATION;
    }
}

void Soloud::setSpeakerPosition(unsigned int aChannel, vec3 value)
{
    m3dSpeakerPosition.at(aChannel) = value;
}

} // namespace SoLoud
