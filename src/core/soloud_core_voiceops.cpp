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

#include "soloud_engine.hpp"

// Direct voice operations (no mutexes - called from other functions)

namespace SoLoud
{
void Soloud::setVoiceRelativePlaySpeed_internal(unsigned int aVoice, float aSpeed)
{
    assert(aVoice < VOICE_COUNT);
    assert(mInsideAudioThreadMutex);
    assert(aSpeed > 0.0f);

    if (mVoice[aVoice])
    {
        mVoice[aVoice]->mSetRelativePlaySpeed = aSpeed;
        updateVoiceRelativePlaySpeed_internal(aVoice);
    }
}

void Soloud::setVoicePause_internal(unsigned int aVoice, int aPause)
{
    assert(aVoice < VOICE_COUNT);
    assert(mInsideAudioThreadMutex);
    mActiveVoiceDirty = true;

    if (mVoice[aVoice])
    {
        mVoice[aVoice]->mPauseScheduler.mActive = 0;

        if (aPause)
        {
            mVoice[aVoice]->mFlags |= AudioSourceInstanceFlags::PAUSED;
        }
        else
        {
            mVoice[aVoice]->mFlags &= ~AudioSourceInstanceFlags::PAUSED;
        }
    }
}

void Soloud::setVoicePan_internal(unsigned int aVoice, float aPan)
{
    assert(aVoice < VOICE_COUNT);
    assert(mInsideAudioThreadMutex);
    if (mVoice[aVoice])
    {
        mVoice[aVoice]->mPan              = aPan;
        float l                           = float(std::cos((aPan + 1) * M_PI / 4));
        float r                           = float(std::sin((aPan + 1) * M_PI / 4));
        mVoice[aVoice]->mChannelVolume[0] = l;
        mVoice[aVoice]->mChannelVolume[1] = r;
        if (mVoice[aVoice]->mChannels == 4)
        {
            mVoice[aVoice]->mChannelVolume[2] = l;
            mVoice[aVoice]->mChannelVolume[3] = r;
        }
        if (mVoice[aVoice]->mChannels == 6)
        {
            mVoice[aVoice]->mChannelVolume[2] = 1.0f / std::sqrt(2.0f);
            mVoice[aVoice]->mChannelVolume[3] = 1;
            mVoice[aVoice]->mChannelVolume[4] = l;
            mVoice[aVoice]->mChannelVolume[5] = r;
        }
        if (mVoice[aVoice]->mChannels == 8)
        {
            mVoice[aVoice]->mChannelVolume[2] = 1.0f / std::sqrt(2.0f);
            mVoice[aVoice]->mChannelVolume[3] = 1;
            mVoice[aVoice]->mChannelVolume[4] = l;
            mVoice[aVoice]->mChannelVolume[5] = r;
            mVoice[aVoice]->mChannelVolume[6] = l;
            mVoice[aVoice]->mChannelVolume[7] = r;
        }
    }
}

void Soloud::setVoiceVolume_internal(unsigned int aVoice, float aVolume)
{
    assert(aVoice < VOICE_COUNT);
    assert(mInsideAudioThreadMutex);
    mActiveVoiceDirty = true;
    if (mVoice[aVoice])
    {
        mVoice[aVoice]->mSetVolume = aVolume;
        updateVoiceVolume_internal(aVoice);
    }
}

void Soloud::stopVoice_internal(unsigned int aVoice)
{
    assert(aVoice < VOICE_COUNT);
    assert(mInsideAudioThreadMutex);
    mActiveVoiceDirty = true;
    if (mVoice[aVoice])
    {
        // Delete via temporary variable to avoid recursion
        AudioSourceInstance* v = mVoice[aVoice];
        mVoice[aVoice]         = 0;

        unsigned int i;
        for (i = 0; i < mMaxActiveVoices; i++)
        {
            if (mResampleDataOwner[i] == v)
            {
                mResampleDataOwner[i] = nullptr;
            }
        }

        delete v;
    }
}

void Soloud::updateVoiceRelativePlaySpeed_internal(unsigned int aVoice)
{
    assert(aVoice < VOICE_COUNT);
    assert(mInsideAudioThreadMutex);
    mVoice[aVoice]->mOverallRelativePlaySpeed =
        m3dData[aVoice].mDopplerValue * mVoice[aVoice]->mSetRelativePlaySpeed;
    mVoice[aVoice]->mSamplerate =
        mVoice[aVoice]->mBaseSamplerate * mVoice[aVoice]->mOverallRelativePlaySpeed;
}

void Soloud::updateVoiceVolume_internal(unsigned int aVoice)
{
    assert(aVoice < VOICE_COUNT);
    assert(mInsideAudioThreadMutex);
    mVoice[aVoice]->mOverallVolume = mVoice[aVoice]->mSetVolume * m3dData[aVoice].m3dVolume;
    if (testFlag(mVoice[aVoice]->mFlags, AudioSourceInstanceFlags::PAUSED))
    {
        int i;
        for (i = 0; i < MAX_CHANNELS; i++)
        {
            mVoice[aVoice]->mCurrentChannelVolume[i] =
                mVoice[aVoice]->mChannelVolume[i] * mVoice[aVoice]->mOverallVolume;
        }
    }
}
} // namespace SoLoud
