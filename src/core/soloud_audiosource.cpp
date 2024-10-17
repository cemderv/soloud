/*
SoLoud audio engine
Copyright (c) 2013-2014 Jari Komppa

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

#include "soloud_audiosource.hpp"
#include "soloud_engine.hpp"
#include <algorithm>
#include <ranges>

namespace SoLoud
{
AudioSourceInstance3dData::AudioSourceInstance3dData(AudioSource& aSource)
{
    m3dAttenuationModel   = aSource.m3dAttenuationModel;
    m3dAttenuationRolloff = aSource.m3dAttenuationRolloff;
    m3dDopplerFactor      = aSource.m3dDopplerFactor;
    m3dMaxDistance        = aSource.m3dMaxDistance;
    m3dMinDistance        = aSource.m3dMinDistance;
    mCollider             = aSource.mCollider;
    mColliderData         = aSource.mColliderData;
    mAttenuator           = aSource.mAttenuator;
    m3dVolume             = 1.0f;
    mDopplerValue         = 1.0f;
}

AudioSourceInstance::AudioSourceInstance()
{
    // Default all volumes to 1.0 so sound behind N mix busses isn't super quiet.
    std::fill(mChannelVolume.begin(), mChannelVolume.end(), 1.0f);
}

AudioSourceInstance::~AudioSourceInstance() noexcept
{
    for (int i = 0; i < FILTERS_PER_STREAM; i++)
    {
        delete mFilter[i];
    }
}

bool AudioSourceInstance::hasFlag(AudioSourceInstanceFlags flag) const
{
    return testFlag(mFlags, flag);
}

void AudioSourceInstance::init(AudioSource& aSource, int aPlayIndex)
{
    mPlayIndex      = aPlayIndex;
    mBaseSamplerate = aSource.mBaseSamplerate;
    mSamplerate     = mBaseSamplerate;
    mChannels       = aSource.mChannels;
    mStreamTime     = 0.0f;
    mStreamPosition = 0.0f;
    mLoopPoint      = aSource.mLoopPoint;

    if (aSource.mFlags & AudioSource::SHOULD_LOOP)
    {
        mFlags |= AudioSourceInstanceFlags::Looping;
    }
    if (aSource.mFlags & AudioSource::PROCESS_3D)
    {
        mFlags |= AudioSourceInstanceFlags::Process3D;
    }
    if (aSource.mFlags & AudioSource::LISTENER_RELATIVE)
    {
        mFlags |= AudioSourceInstanceFlags::ListenerRelative;
    }
    if (aSource.mFlags & AudioSource::INAUDIBLE_KILL)
    {
        mFlags |= AudioSourceInstanceFlags::InaudibleKill;
    }
    if (aSource.mFlags & AudioSource::INAUDIBLE_TICK)
    {
        mFlags |= AudioSourceInstanceFlags::InaudibleTick;
    }
    if (aSource.mFlags & AudioSource::DISABLE_AUTOSTOP)
    {
        mFlags |= AudioSourceInstanceFlags::DisableAutostop;
    }
}

bool AudioSourceInstance::rewind()
{
    return false;
}

bool AudioSourceInstance::seek(double aSeconds, float* mScratch, unsigned int mScratchSize)
{
    double offset = aSeconds - mStreamPosition;
    if (offset <= 0)
    {
        if (!rewind())
        {
            // can't do generic seek backwards unless we can rewind.
            return false;
        }
        offset = aSeconds;
    }
    int samples_to_discard = (int)floor(mSamplerate * offset);

    while (samples_to_discard)
    {
        int samples = mScratchSize / mChannels;
        if (samples > samples_to_discard)
            samples = samples_to_discard;
        getAudio(mScratch, samples, samples);
        samples_to_discard -= samples;
    }

    mStreamPosition = aSeconds;

    return true;
}

AudioSource::~AudioSource() noexcept
{
    stop();
}

void AudioSource::setVolume(float aVolume)
{
    mVolume = aVolume;
}

void AudioSource::setLoopPoint(time_t aLoopPoint)
{
    mLoopPoint = aLoopPoint;
}

time_t AudioSource::getLoopPoint()
{
    return mLoopPoint;
}

void AudioSource::setLooping(bool aLoop)
{
    if (aLoop)
    {
        mFlags |= SHOULD_LOOP;
    }
    else
    {
        mFlags &= ~SHOULD_LOOP;
    }
}

void AudioSource::setSingleInstance(bool aSingleInstance)
{
    if (aSingleInstance)
    {
        mFlags |= SINGLE_INSTANCE;
    }
    else
    {
        mFlags &= ~SINGLE_INSTANCE;
    }
}

void AudioSource::setAutoStop(bool aAutoStop)
{
    if (aAutoStop)
    {
        mFlags &= ~DISABLE_AUTOSTOP;
    }
    else
    {
        mFlags |= DISABLE_AUTOSTOP;
    }
}

void AudioSource::setFilter(unsigned int aFilterId, Filter* aFilter)
{
    if (aFilterId >= FILTERS_PER_STREAM)
        return;

    mFilter[aFilterId] = aFilter;
}

void AudioSource::stop()
{
    if (mSoloud)
    {
        mSoloud->stopAudioSource(*this);
    }
}

void AudioSource::set3dMinMaxDistance(float aMinDistance, float aMaxDistance)
{
    m3dMinDistance = aMinDistance;
    m3dMaxDistance = aMaxDistance;
}

void AudioSource::set3dAttenuation(unsigned int aAttenuationModel, float aAttenuationRolloffFactor)
{
    m3dAttenuationModel   = aAttenuationModel;
    m3dAttenuationRolloff = aAttenuationRolloffFactor;
}

void AudioSource::set3dDopplerFactor(float aDopplerFactor)
{
    m3dDopplerFactor = aDopplerFactor;
}

void AudioSource::set3dListenerRelative(bool aListenerRelative)
{
    if (aListenerRelative)
    {
        mFlags |= LISTENER_RELATIVE;
    }
    else
    {
        mFlags &= ~LISTENER_RELATIVE;
    }
}


void AudioSource::set3dDistanceDelay(bool aDistanceDelay)
{
    if (aDistanceDelay)
    {
        mFlags |= DISTANCE_DELAY;
    }
    else
    {
        mFlags &= ~DISTANCE_DELAY;
    }
}

void AudioSource::set3dCollider(AudioCollider* aCollider, int aUserData)
{
    mCollider     = aCollider;
    mColliderData = aUserData;
}

void AudioSource::set3dAttenuator(AudioAttenuator* aAttenuator)
{
    mAttenuator = aAttenuator;
}

void AudioSource::setInaudibleBehavior(bool aMustTick, bool aKill)
{
    mFlags &= ~(AudioSource::INAUDIBLE_KILL | AudioSource::INAUDIBLE_TICK);
    if (aMustTick)
    {
        mFlags |= AudioSource::INAUDIBLE_TICK;
    }
    if (aKill)
    {
        mFlags |= AudioSource::INAUDIBLE_KILL;
    }
}


float AudioSourceInstance::getInfo(unsigned int /*aInfoKey*/)
{
    return 0;
}


}; // namespace SoLoud
