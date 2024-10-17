/*
SoLoud audio engine
Copyright (c) 2013-2020 Jari Komppa

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

#include "soloud_bus.hpp"
#include "soloud_fft.hpp"
#include "soloud_internal.hpp"

namespace SoLoud
{
BusInstance::BusInstance(Bus* aParent)
    : mParent(aParent)
    , mScratchSize(SAMPLE_GRANULARITY)
    , mScratch(mScratchSize * MAX_CHANNELS)
{
    mFlags |= AudioSourceInstanceFlags::Protected | AudioSourceInstanceFlags::InaudibleTick;
}

size_t BusInstance::getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize)
{
    int handle = mParent->mChannelHandle;
    if (handle == 0)
    {
        // Avoid reuse of scratch data if this bus hasn't played anything yet
        size_t i;
        for (i = 0; i < aBufferSize * mChannels; ++i)
            aBuffer[i] = 0;
        return aSamplesToRead;
    }

    Engine* s = mParent->mSoloud;

    s->mixBus_internal(aBuffer,
                       aSamplesToRead,
                       aBufferSize,
                       mScratch.mData,
                       handle,
                       mSamplerate,
                       mChannels,
                       mParent->mResampler);

    int i;
    if (mParent->mFlags & AudioSource::VISUALIZATION_DATA)
    {
        for (i = 0; i < MAX_CHANNELS; ++i)
            mVisualizationChannelVolume[i] = 0;

        if (aSamplesToRead > 255)
        {
            for (i = 0; i < 256; ++i)
            {
                int j;
                mVisualizationWaveData[i] = 0;
                for (j = 0; j < (signed)mChannels; ++j)
                {
                    float sample = aBuffer[i + aBufferSize * j];
                    float absvol = (float)fabs(sample);
                    if (absvol > mVisualizationChannelVolume[j])
                        mVisualizationChannelVolume[j] = absvol;
                    mVisualizationWaveData[i] += sample;
                }
            }
        }
        else
        {
            // Very unlikely failsafe branch
            for (i = 0; i < 256; ++i)
            {
                int j;
                mVisualizationWaveData[i] = 0;
                for (j = 0; j < (signed)mChannels; ++j)
                {
                    float sample = aBuffer[(i % aSamplesToRead) + aBufferSize * j];
                    float absvol = (float)fabs(sample);
                    if (absvol > mVisualizationChannelVolume[j])
                        mVisualizationChannelVolume[j] = absvol;
                    mVisualizationWaveData[i] += sample;
                }
            }
        }
    }
    return aSamplesToRead;
}

bool BusInstance::hasEnded()
{
    return false;
}

BusInstance::~BusInstance() noexcept
{
    Engine* s = mParent->mSoloud;
    int     i;
    for (i = 0; i < (signed)s->mHighestVoice; ++i)
    {
        if (s->mVoice[i] && s->mVoice[i]->mBusHandle == mParent->mChannelHandle)
        {
            s->stopVoice_internal(i);
        }
    }
}

Bus::Bus()
{
    mChannels = 2;
}

std::shared_ptr<AudioSourceInstance> Bus::createInstance()
{
    if (mChannelHandle)
    {
        stop();
        mChannelHandle = 0;
    }
    mInstance = std::make_shared<BusInstance>(this);
    return mInstance;
}

void Bus::findBusHandle()
{
    if (mChannelHandle == 0)
    {
        // Find the channel the bus is playing on to calculate handle..
        int i;
        for (i = 0; mChannelHandle == 0 && i < (signed)mSoloud->mHighestVoice; ++i)
        {
            if (mSoloud->mVoice[i].get() == mInstance.get())
            {
                mChannelHandle = mSoloud->getHandleFromVoice_internal(i);
            }
        }
    }
}

handle Bus::play(AudioSource& aSound, float aVolume, float aPan, bool aPaused)
{
    if (!mInstance || !mSoloud)
    {
        return 0;
    }

    findBusHandle();

    if (mChannelHandle == 0)
    {
        return 0;
    }
    return mSoloud->play(aSound, aVolume, aPan, aPaused, mChannelHandle);
}


handle Bus::playClocked(time_t aSoundTime, AudioSource& aSound, float aVolume, float aPan)
{
    if (!mInstance || !mSoloud)
    {
        return 0;
    }

    findBusHandle();

    if (mChannelHandle == 0)
    {
        return 0;
    }

    return mSoloud->playClocked(aSoundTime, aSound, aVolume, aPan, mChannelHandle);
}

handle Bus::play3d(AudioSource& aSound, vec3 aPos, vec3 aVel, float aVolume, bool aPaused)
{
    if (!mInstance || !mSoloud)
    {
        return 0;
    }

    findBusHandle();

    if (mChannelHandle == 0)
    {
        return 0;
    }
    return mSoloud->play3d(aSound, aPos, aVel, aVolume, aPaused, mChannelHandle);
}

handle Bus::play3dClocked(
    time_t aSoundTime, AudioSource& aSound, vec3 aPos, vec3 aVel, float aVolume)
{
    if (!mInstance || !mSoloud)
    {
        return 0;
    }

    findBusHandle();

    if (mChannelHandle == 0)
    {
        return 0;
    }
    return mSoloud->play3dClocked(aSoundTime, aSound, aPos, aVel, aVolume, mChannelHandle);
}

void Bus::annexSound(handle aVoiceHandle)
{
    findBusHandle();
    FOR_ALL_VOICES_PRE_EXT
    mSoloud->mVoice[ch]->mBusHandle = mChannelHandle;
    FOR_ALL_VOICES_POST_EXT
}

void Bus::setFilter(size_t aFilterId, Filter* aFilter)
{
    if (aFilterId >= FILTERS_PER_STREAM)
        return;

    mFilter[aFilterId] = aFilter;

    if (mInstance)
    {
        mSoloud->lockAudioMutex_internal();
        if (aFilter)
        {
            mInstance->mFilter[aFilterId] = mFilter[aFilterId]->createInstance();
        }
        mSoloud->unlockAudioMutex_internal();
    }
}

void Bus::setChannels(size_t aChannels)
{
    assert(aChannels != 0 && aChannels != 3 && aChannels != 5 && aChannels != 7);
    assert(aChannels <= MAX_CHANNELS);

    mChannels = aChannels;
}

void Bus::setVisualizationEnable(bool aEnable)
{
    if (aEnable)
    {
        mFlags |= AudioSource::VISUALIZATION_DATA;
    }
    else
    {
        mFlags &= ~AudioSource::VISUALIZATION_DATA;
    }
}

float* Bus::calcFFT()
{
    if (mInstance && mSoloud)
    {
        mSoloud->lockAudioMutex_internal();
        float temp[1024];
        int   i;
        for (i = 0; i < 256; ++i)
        {
            temp[i * 2]     = mInstance->mVisualizationWaveData[i];
            temp[i * 2 + 1] = 0;
            temp[i + 512]   = 0;
            temp[i + 768]   = 0;
        }
        mSoloud->unlockAudioMutex_internal();

        SoLoud::FFT::fft1024(temp);

        for (i = 0; i < 256; ++i)
        {
            float real  = temp[i * 2];
            float imag  = temp[i * 2 + 1];
            mFFTData[i] = (float)sqrt(real * real + imag * imag);
        }
    }

    return mFFTData.data();
}

float* Bus::getWave()
{
    if (mInstance && mSoloud)
    {
        int i;
        mSoloud->lockAudioMutex_internal();
        for (i = 0; i < 256; ++i)
            mWaveData[i] = mInstance->mVisualizationWaveData[i];
        mSoloud->unlockAudioMutex_internal();
    }
    return mWaveData.data();
}

float Bus::getApproximateVolume(size_t aChannel)
{
    if (aChannel > mChannels)
        return 0;
    float vol = 0;
    if (mInstance && mSoloud)
    {
        mSoloud->lockAudioMutex_internal();
        vol = mInstance->mVisualizationChannelVolume[aChannel];
        mSoloud->unlockAudioMutex_internal();
    }
    return vol;
}

size_t Bus::getActiveVoiceCount()
{
    int    i;
    size_t count = 0;
    findBusHandle();
    mSoloud->lockAudioMutex_internal();
    for (i = 0; i < VOICE_COUNT; ++i)
        if (mSoloud->mVoice[i] && mSoloud->mVoice[i]->mBusHandle == mChannelHandle)
            count++;
    mSoloud->unlockAudioMutex_internal();
    return count;
}

Resampler Bus::getResampler() const
{
    return mResampler;
}

void Bus::setResampler(Resampler aResampler)
{
    mResampler = aResampler;
}
}; // namespace SoLoud
