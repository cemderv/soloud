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

// Getters - return information about SoLoud state

namespace SoLoud
{
float Soloud::getPostClipScaler() const
{
    return mPostClipScaler;
}

Resampler Soloud::getMainResampler() const
{
    return mResampler;
}

float Soloud::getGlobalVolume() const
{
    return mGlobalVolume;
}

handle Soloud::getHandleFromVoice_internal(unsigned int aVoice) const
{
    if (mVoice[aVoice] == 0)
        return 0;
    return (aVoice + 1) | (mVoice[aVoice]->mPlayIndex << 12);
}

int Soloud::getVoiceFromHandle_internal(handle aVoiceHandle) const
{
    // If this is a voice group handle, pick the first handle from the group
    handle* h = voiceGroupHandleToArray_internal(aVoiceHandle);
    if (h != nullptr)
        aVoiceHandle = *h;

    if (aVoiceHandle == 0)
    {
        return -1;
    }

    int          ch  = (aVoiceHandle & 0xfff) - 1;
    unsigned int idx = aVoiceHandle >> 12;
    if (mVoice[ch] && (mVoice[ch]->mPlayIndex & 0xfffff) == idx)
    {
        return ch;
    }
    return -1;
}

unsigned int Soloud::getMaxActiveVoiceCount() const
{
    return mMaxActiveVoices;
}

unsigned int Soloud::getActiveVoiceCount()
{
    lockAudioMutex_internal();
    if (mActiveVoiceDirty)
        calcActiveVoices_internal();
    unsigned int c = mActiveVoiceCount;
    unlockAudioMutex_internal();
    return c;
}

unsigned int Soloud::getVoiceCount()
{
    lockAudioMutex_internal();
    int i;
    int c = 0;
    for (i = 0; i < (signed)mHighestVoice; i++)
    {
        if (mVoice[i])
        {
            c++;
        }
    }
    unlockAudioMutex_internal();
    return c;
}

bool Soloud::isValidVoiceHandle(handle aVoiceHandle)
{
    // voice groups are not valid voice handles
    if ((aVoiceHandle & 0xfffff000) == 0xfffff000)
        return false;

    lockAudioMutex_internal();
    if (getVoiceFromHandle_internal(aVoiceHandle) != -1)
    {
        unlockAudioMutex_internal();
        return true;
    }
    unlockAudioMutex_internal();
    return false;
}


time_t Soloud::getLoopPoint(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const time_t v = mVoice[ch]->mLoopPoint;
    unlockAudioMutex_internal();
    return v;
}

bool Soloud::getLooping(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const bool v = testFlag(mVoice[ch]->mFlags, AudioSourceInstanceFlags::Looping);
    unlockAudioMutex_internal();
    return v;
}

bool Soloud::getAutoStop(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = testFlag(mVoice[ch]->mFlags, AudioSourceInstanceFlags::DisableAutostop);
    unlockAudioMutex_internal();
    return !v;
}

float Soloud::getInfo(handle aVoiceHandle, unsigned int mInfoKey)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->getInfo(mInfoKey);
    unlockAudioMutex_internal();
    return v;
}

float Soloud::getVolume(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mSetVolume;
    unlockAudioMutex_internal();
    return v;
}

float Soloud::getOverallVolume(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mOverallVolume;
    unlockAudioMutex_internal();
    return v;
}

float Soloud::getPan(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mPan;
    unlockAudioMutex_internal();
    return v;
}

time_t Soloud::getStreamTime(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const double v = mVoice[ch]->mStreamTime;
    unlockAudioMutex_internal();
    return v;
}

time_t Soloud::getStreamPosition(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const double v = mVoice[ch]->mStreamPosition;
    unlockAudioMutex_internal();
    return v;
}

float Soloud::getRelativePlaySpeed(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 1;
    }
    const float v = mVoice[ch]->mSetRelativePlaySpeed;
    unlockAudioMutex_internal();
    return v;
}

float Soloud::getSamplerate(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    const float v = mVoice[ch]->mBaseSamplerate;
    unlockAudioMutex_internal();
    return v;
}

bool Soloud::getPause(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = testFlag(mVoice[ch]->mFlags, AudioSourceInstanceFlags::Paused);
    unlockAudioMutex_internal();
    return v;
}

bool Soloud::getProtectVoice(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    const int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return false;
    }
    const auto v = testFlag(mVoice[ch]->mFlags, AudioSourceInstanceFlags::Protected);
    unlockAudioMutex_internal();
    return v;
}

int Soloud::findFreeVoice_internal()
{
    unsigned int lowest_play_index_value = 0xffffffff;
    int          lowest_play_index       = -1;

    // (slowly) drag the highest active voice index down
    if (mHighestVoice > 0 && mVoice[mHighestVoice - 1] == nullptr)
        mHighestVoice--;

    for (int i = 0; i < VOICE_COUNT; i++)
    {
        if (mVoice[i] == nullptr)
        {
            if (i + 1 > signed(mHighestVoice))
            {
                mHighestVoice = i + 1;
            }
            return i;
        }
        if (testFlag(mVoice[i]->mFlags, AudioSourceInstanceFlags::Protected) == 0 &&
            mVoice[i]->mPlayIndex < lowest_play_index_value)
        {
            lowest_play_index_value = mVoice[i]->mPlayIndex;
            lowest_play_index       = i;
        }
    }
    stopVoice_internal(lowest_play_index);
    return lowest_play_index;
}

unsigned int Soloud::getLoopCount(handle aVoiceHandle)
{
    lockAudioMutex_internal();
    int ch = getVoiceFromHandle_internal(aVoiceHandle);
    if (ch == -1)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    int v = mVoice[ch]->mLoopCount;
    unlockAudioMutex_internal();
    return v;
}

// Returns current backend channel count (1 mono, 2 stereo, etc)
unsigned int Soloud::getBackendChannels() const
{
    return mChannels;
}

// Returns current backend sample rate
unsigned int Soloud::getBackendSamplerate() const
{
    return mSamplerate;
}

// Returns current backend buffer size
unsigned int Soloud::getBackendBufferSize() const
{
    return mBufferSize;
}

// Get speaker position in 3d space
vec3 Soloud::getSpeakerPosition(unsigned int aChannel) const
{
    return m3dSpeakerPosition.at(aChannel);
}
} // namespace SoLoud
