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

#include "soloud_error.hpp"
#include "soloud_internal.hpp"
#include "soloud_vec3.hpp"
#include <array>
#include <cmath>

// 3d audio operations

namespace SoLoud
{
struct mat3 : std::array<vec3, 3>
{
};

static vec3 operator*(const mat3& m, const vec3& a)
{
    return {
        m[0].mX * a.mX + m[0].mY * a.mY + m[0].mZ * a.mZ,
        m[1].mX * a.mX + m[1].mY * a.mY + m[1].mZ * a.mZ,
        m[2].mX * a.mX + m[2].mY * a.mY + m[2].mZ * a.mZ,
    };
}

static mat3 lookatRH(const vec3& at, vec3 up)
{
    const auto z = normalize(at);
    const auto x = normalize(up.cross(z));
    const auto y = z.cross(x);

    return {x, y, z};
}

static mat3 lookatLH(const vec3& at, vec3 up)
{
    const auto z = normalize(at);
    auto       x = normalize(up.cross(z));
    const auto y = z.cross(x);
    x            = -x;

    return {x, y, z};
}

#ifndef MIN
#define MIN(a, b) ((a) < (b)) ? (a) : (b)
#endif

#ifndef MAX
#define MAX(a, b) ((a) > (b)) ? (a) : (b)
#endif

float doppler(
    vec3        aDeltaPos,
    const vec3& aSrcVel,
    const vec3& aDstVel,
    float       aFactor,
    float       aSoundSpeed)
{
    float deltamag = aDeltaPos.mag();
    if (deltamag == 0)
        return 1.0f;
    float vls      = aDeltaPos.dot(aDstVel) / deltamag;
    float vss      = aDeltaPos.dot(aSrcVel) / deltamag;
    float maxspeed = aSoundSpeed / aFactor;
    vss            = MIN(vss, maxspeed);
    vls            = MIN(vls, maxspeed);
    return (aSoundSpeed - aFactor * vls) / (aSoundSpeed - aFactor * vss);
}

float attenuateInvDistance(float aDistance,
                           float aMinDistance,
                           float aMaxDistance,
                           float aRolloffFactor)
{
    float distance = MAX(aDistance, aMinDistance);
    distance       = MIN(distance, aMaxDistance);
    return aMinDistance / (aMinDistance + aRolloffFactor * (distance - aMinDistance));
}

float attenuateLinearDistance(float aDistance,
                              float aMinDistance,
                              float aMaxDistance,
                              float aRolloffFactor)
{
    float distance = MAX(aDistance, aMinDistance);
    distance       = MIN(distance, aMaxDistance);
    return 1 - aRolloffFactor * (distance - aMinDistance) / (aMaxDistance - aMinDistance);
}

float attenuateExponentialDistance(float aDistance,
                                   float aMinDistance,
                                   float aMaxDistance,
                                   float aRolloffFactor)
{
    float distance = MAX(aDistance, aMinDistance);
    distance       = MIN(distance, aMaxDistance);
    return pow(distance / aMinDistance, -aRolloffFactor);
}

void Soloud::update3dVoices_internal(unsigned int* aVoiceArray, unsigned int aVoiceCount)
{
    auto speaker = std::array<vec3, MAX_CHANNELS>{};

    for (unsigned int i = 0; i < mChannels; ++i)
        speaker[i]      = normalize(m3dSpeakerPosition[i]);

    const auto lpos = m3dPosition;
    const auto lvel = m3dVelocity;
    const auto at   = m3dAt;
    const auto up   = m3dUp;
    const auto m    = mFlags & LEFT_HANDED_3D ? lookatLH(at, up) : lookatRH(at, up);

    for (unsigned int i = 0; i < aVoiceCount; i++)
    {
        auto& v = m3dData[aVoiceArray[i]];

        auto vol = v.mCollider != nullptr
                       ? v.mCollider->collide(this, v, v.mColliderData)
                       : 1.0f;

        auto       pos = v.m3dPosition;
        const auto vel = v.m3dVelocity;

        if (!testFlag(v.mFlags, AudioSourceInstanceFlags::LISTENER_RELATIVE))
        {
            pos = pos - lpos;
        }

        const float dist = pos.mag();

        // attenuation

        if (v.mAttenuator != nullptr)
        {
            vol *= v.mAttenuator->attenuate(dist,
                                            v.m3dMinDistance,
                                            v.m3dMaxDistance,
                                            v.m3dAttenuationRolloff);
        }
        else
        {
            switch (v.m3dAttenuationModel)
            {
                case AudioSource::INVERSE_DISTANCE: vol *= attenuateInvDistance(dist,
                                                        v.m3dMinDistance,
                                                        v.m3dMaxDistance,
                                                        v.m3dAttenuationRolloff);
                    break;
                case AudioSource::LINEAR_DISTANCE: vol *= attenuateLinearDistance(dist,
                                                       v.m3dMinDistance,
                                                       v.m3dMaxDistance,
                                                       v.m3dAttenuationRolloff);
                    break;
                case AudioSource::EXPONENTIAL_DISTANCE: vol *= attenuateExponentialDistance(dist,
                                                            v.m3dMinDistance,
                                                            v.m3dMaxDistance,
                                                            v.m3dAttenuationRolloff);
                    break;
                default:
                    // case AudioSource::NO_ATTENUATION:
                    break;
            }
        }

        // cone
        // (todo) vol *= conev;

        // doppler
        v.mDopplerValue = doppler(pos, vel, lvel, v.m3dDopplerFactor, m3dSoundSpeed);

        // panning
        pos = normalize(m * pos);

        v.mChannelVolume = {};

        // Apply volume to channels based on speaker vectors
        for (unsigned int j = 0; j < mChannels; j++)
        {
            float speakervol = (speaker[j].dot(pos) + 1) / 2;
            if (speaker[j].null())
                speakervol = 1;
            // Different speaker "focus" calculations to try, if the default "bleeds" too much..
            // speakervol = (speakervol * speakervol + speakervol) / 2;
            // speakervol = speakervol * speakervol;
            v.mChannelVolume[j] = vol * speakervol;
        }

        v.m3dVolume = vol;
    }
}

void Soloud::update3dAudio()
{
    unsigned int voicecount = 0;
    unsigned int voices[VOICE_COUNT];

    // Step 1 - find voices that need 3d processing
    lockAudioMutex_internal();
    int i;
    for (i = 0; i < (signed)mHighestVoice; i++)
    {
        if (mVoice[i] && mVoice[i]->hasFlag(AudioSourceInstanceFlags::PROCESS_3D))
        {
            voices[voicecount] = i;
            voicecount++;
            m3dData[i].mFlags = mVoice[i]->mFlags;
        }
    }
    unlockAudioMutex_internal();

    // Step 2 - do 3d processing

    update3dVoices_internal(voices, voicecount);

    // Step 3 - update SoLoud voices

    lockAudioMutex_internal();
    for (i = 0; i < (int)voicecount; i++)
    {
        AudioSourceInstance3dData* v  = &m3dData[voices[i]];
        AudioSourceInstance*       vi = mVoice[voices[i]];
        if (vi)
        {
            updateVoiceRelativePlaySpeed_internal(voices[i]);
            updateVoiceVolume_internal(voices[i]);
            int j;
            for (j = 0; j < MAX_CHANNELS; j++)
            {
                vi->mChannelVolume[j] = v->mChannelVolume[j];
            }

            if (vi->mOverallVolume < 0.001f)
            {
                // Inaudible.
                vi->mFlags |= AudioSourceInstanceFlags::INAUDIBLE;

                if (testFlag(vi->mFlags, AudioSourceInstanceFlags::INAUDIBLE_KILL))
                {
                    stopVoice_internal(voices[i]);
                }
            }
            else
            {
                vi->mFlags &= ~AudioSourceInstanceFlags::INAUDIBLE;
            }
        }
    }

    mActiveVoiceDirty = true;
    unlockAudioMutex_internal();
}


handle Soloud::play3d(AudioSource& aSound,
                      vec3         aPos,
                      vec3         aVel,
                      float        aVolume,
                      bool         aPaused,
                      unsigned int aBus)
{
    handle h = play(aSound, aVolume, 0, 1, aBus);
    lockAudioMutex_internal();
    int v = getVoiceFromHandle_internal(h);
    if (v < 0)
    {
        unlockAudioMutex_internal();
        return h;
    }
    m3dData[v].mHandle = h;
    mVoice[v]->mFlags |= AudioSourceInstanceFlags::PROCESS_3D;
    set3dSourceParameters(h, aPos, aVel);

    int samples = 0;
    if (aSound.mFlags & AudioSource::DISTANCE_DELAY)
    {
        const auto pos = testFlag(mVoice[v]->mFlags, AudioSourceInstanceFlags::LISTENER_RELATIVE)
                             ? aPos
                             : aPos - m3dPosition;

        const float dist = pos.mag();
        samples += int(floor(dist / m3dSoundSpeed * float(mSamplerate)));
    }

    update3dVoices_internal((unsigned int*)&v, 1);
    updateVoiceRelativePlaySpeed_internal(v);
    int j;
    for (j = 0; j < MAX_CHANNELS; j++)
    {
        mVoice[v]->mChannelVolume[j] = m3dData[v].mChannelVolume[j];
    }

    updateVoiceVolume_internal(v);

    // Fix initial voice volume ramp up
    int i;
    for (i = 0; i < MAX_CHANNELS; i++)
    {
        mVoice[v]->mCurrentChannelVolume[i] =
            mVoice[v]->mChannelVolume[i] * mVoice[v]->mOverallVolume;
    }

    if (mVoice[v]->mOverallVolume < 0.01f)
    {
        // Inaudible.
        mVoice[v]->mFlags |= AudioSourceInstanceFlags::INAUDIBLE;

        if (mVoice[v]->hasFlag(AudioSourceInstanceFlags::INAUDIBLE_KILL))
        {
            stopVoice_internal(v);
        }
    }
    else
    {
        mVoice[v]->mFlags &= ~AudioSourceInstanceFlags::INAUDIBLE;
    }
    mActiveVoiceDirty = true;

    unlockAudioMutex_internal();
    setDelaySamples(h, samples);
    setPause(h, aPaused);
    return h;
}

handle Soloud::play3dClocked(time         aSoundTime,
                             AudioSource& aSound,
                             vec3         aPos,
                             vec3         aVel,
                             float        aVolume,
                             unsigned int aBus)
{
    handle h = play(aSound, aVolume, 0, 1, aBus);
    lockAudioMutex_internal();
    int v = getVoiceFromHandle_internal(h);
    if (v < 0)
    {
        unlockAudioMutex_internal();
        return h;
    }
    m3dData[v].mHandle = h;
    mVoice[v]->mFlags |= AudioSourceInstanceFlags::PROCESS_3D;
    set3dSourceParameters(h, aPos, aVel);
    time lasttime = mLastClockedTime;
    if (lasttime == 0)
    {
        lasttime         = aSoundTime;
        mLastClockedTime = aSoundTime;
    }
    unlockAudioMutex_internal();

    int samples = int(floor((aSoundTime - lasttime) * mSamplerate));
    // Make sure we don't delay too much (or overflow)
    if (samples < 0 || samples > 2048)
        samples = 0;

    if (aSound.mFlags & AudioSource::DISTANCE_DELAY)
    {
        const float dist = aPos.mag();
        samples += int(floor((dist / m3dSoundSpeed) * mSamplerate));
    }

    update3dVoices_internal(reinterpret_cast<unsigned int*>(&v), 1);
    lockAudioMutex_internal();
    updateVoiceRelativePlaySpeed_internal(v);
    int j;
    for (j = 0; j < MAX_CHANNELS; j++)
    {
        mVoice[v]->mChannelVolume[j] = m3dData[v].mChannelVolume[j];
    }

    updateVoiceVolume_internal(v);

    // Fix initial voice volume ramp up
    for (int i = 0; i < MAX_CHANNELS; i++)
    {
        mVoice[v]->mCurrentChannelVolume[i] =
            mVoice[v]->mChannelVolume[i] * mVoice[v]->mOverallVolume;
    }

    if (mVoice[v]->mOverallVolume < 0.01f)
    {
        // Inaudible.
        mVoice[v]->mFlags |= AudioSourceInstanceFlags::INAUDIBLE;

        if (mVoice[v]->hasFlag(AudioSourceInstanceFlags::INAUDIBLE_KILL))
        {
            stopVoice_internal(v);
        }
    }
    else
    {
        mVoice[v]->mFlags &= ~AudioSourceInstanceFlags::INAUDIBLE;
    }
    mActiveVoiceDirty = true;
    unlockAudioMutex_internal();

    setDelaySamples(h, samples);
    setPause(h, false);
    return h;
}


result Soloud::set3dSoundSpeed(float aSpeed)
{
    if (aSpeed <= 0)
        return INVALID_PARAMETER;
    m3dSoundSpeed = aSpeed;
    return SO_NO_ERROR;
}


float Soloud::get3dSoundSpeed() const
{
    return m3dSoundSpeed;
}


void Soloud::set3dListenerParameters(vec3 pos,
                                     vec3 at,
                                     vec3 up,
                                     vec3 velocity
    )
{
    m3dPosition = pos;
    m3dAt       = at;
    m3dUp       = up;
    m3dVelocity = velocity;
}


void Soloud::set3dListenerPosition(vec3 value)
{
    m3dPosition = value;
}


void Soloud::set3dListenerAt(vec3 value)
{
    m3dAt = value;
}


void Soloud::set3dListenerUp(vec3 value)
{
    m3dUp = value;
}


void Soloud::set3dListenerVelocity(vec3 value)
{
    m3dVelocity = value;
}


void Soloud::set3dSourceParameters(handle aVoiceHandle,
                                   vec3   aPos,
                                   vec3   aVelocity)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dPosition = aPos;
        m3dData[ch].m3dVelocity = aVelocity;
    FOR_ALL_VOICES_POST_3D
}


void Soloud::set3dSourcePosition(handle aVoiceHandle, vec3 value)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dPosition = value;
    FOR_ALL_VOICES_POST_3D
}


void Soloud::set3dSourceVelocity(handle aVoiceHandle,
                                 vec3   velocity)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dVelocity = velocity;
    FOR_ALL_VOICES_POST_3D
}


void Soloud::set3dSourceMinMaxDistance(handle aVoiceHandle, float aMinDistance, float aMaxDistance)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dMinDistance = aMinDistance;
        m3dData[ch].m3dMaxDistance = aMaxDistance;
    FOR_ALL_VOICES_POST_3D
}


void Soloud::set3dSourceAttenuation(handle       aVoiceHandle,
                                    unsigned int aAttenuationModel,
                                    float        aAttenuationRolloffFactor)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dAttenuationModel   = aAttenuationModel;
        m3dData[ch].m3dAttenuationRolloff = aAttenuationRolloffFactor;
    FOR_ALL_VOICES_POST_3D
}


void Soloud::set3dSourceDopplerFactor(handle aVoiceHandle, float aDopplerFactor)
{
    FOR_ALL_VOICES_PRE_3D
        m3dData[ch].m3dDopplerFactor = aDopplerFactor;
    FOR_ALL_VOICES_POST_3D
}
}; // namespace SoLoud
