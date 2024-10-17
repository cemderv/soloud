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

#pragma once

#include "soloud_fader.hpp"
#include "soloud_filter.hpp"
#include "soloud_vec3.hpp"
#include <array>
#include <memory>

namespace SoLoud
{
class AudioSource;
class AudioSourceInstance;
class AudioSourceInstance3dData;

class AudioCollider
{
  public:
    virtual ~AudioCollider() noexcept = default;

    // Calculate volume multiplier. Assumed to return value between 0 and 1.
    virtual float collide(Engine*                    aSoloud,
                          AudioSourceInstance3dData& aAudioInstance3dData,
                          int                        aUserData) = 0;
};

class AudioAttenuator
{
  public:
    virtual ~AudioAttenuator() noexcept = default;

    virtual float attenuate(float aDistance,
                            float aMinDistance,
                            float aMaxDistance,
                            float aRolloffFactor) = 0;
};

enum class AudioSourceInstanceFlags
{
    None = 0,
    // This audio instance loops (if supported)
    Looping = 1,
    // This audio instance is protected - won't get stopped if we run out of voices
    Protected = 2,
    // This audio instance is paused
    Paused = 4,
    // This audio instance is affected by 3d processing
    Process3D = 8,
    // This audio instance has listener-relative 3d coordinates
    ListenerRelative = 16,
    // Currently inaudible
    Inaudible = 32,
    // If inaudible, should be killed (default = don't kill kill)
    InaudibleKill = 64,
    // If inaudible, should still be ticked (default = pause)
    InaudibleTick = 128,
    // Don't auto-stop sound
    DisableAutostop = 256
};

class AudioSourceInstance3dData
{
  public:
    AudioSourceInstance3dData() = default;

    explicit AudioSourceInstance3dData(AudioSource& aSource);

    // 3d position
    vec3 m3dPosition;

    // 3d velocity
    vec3 m3dVelocity;

    // 3d min distance
    float m3dMinDistance = 0.0f;

    // 3d max distance
    float m3dMaxDistance = 1000000.0f;

    // 3d attenuation rolloff factor
    float m3dAttenuationRolloff = 1.0f;

    // 3d attenuation model
    size_t m3dAttenuationModel = 0;

    // 3d doppler factor
    float m3dDopplerFactor = 1.0f;

    // Pointer to a custom audio collider object
    AudioCollider* mCollider = nullptr;

    // Pointer to a custom audio attenuator object
    AudioAttenuator* mAttenuator = nullptr;

    // User data related to audio collider
    int mColliderData = 0;

    // Doppler sample rate multiplier
    float mDopplerValue = 0.0f;

    // Overall 3d volume
    float m3dVolume = 0.0f;

    // Channel volume
    std::array<float, MAX_CHANNELS> mChannelVolume{};

    // Copy of flags
    AudioSourceInstanceFlags mFlags = AudioSourceInstanceFlags::None;

    // Latest handle for this voice
    handle mHandle = 0;
};

// Base class for audio instances
class AudioSourceInstance
{
  public:
    AudioSourceInstance();

    virtual ~AudioSourceInstance() noexcept = default;

    // Play index; used to identify instances from handles
    size_t mPlayIndex = 0;

    // Loop count
    size_t mLoopCount = 0;

    AudioSourceInstanceFlags mFlags = AudioSourceInstanceFlags::None;

    bool hasFlag(AudioSourceInstanceFlags flag) const;

    // Pan value, for getPan()
    float mPan = 0.0f;

    // Volume for each channel (panning)
    std::array<float, MAX_CHANNELS> mChannelVolume{};

    // Set volume
    float mSetVolume = 1.0f;

    // Overall volume overall = set * 3d
    float mOverallVolume = 0.0f;

    // Base samplerate; samplerate = base samplerate * relative play speed
    float mBaseSamplerate = 44100.0f;

    // Samplerate; samplerate = base samplerate * relative play speed
    float mSamplerate = 44100.0f;

    // Number of channels this audio source produces
    size_t mChannels = 1;

    // Relative play speed; samplerate = base samplerate * relative play speed
    float mSetRelativePlaySpeed = 1.0f;

    // Overall relative plays peed; overall = set * 3d
    float mOverallRelativePlaySpeed = 1.0f;

    // How long this stream has played, in seconds.
    time_t mStreamTime = 0.0f;

    // Position of this stream, in seconds.
    time_t mStreamPosition = 0.0f;

    // Fader for the audio panning
    Fader mPanFader;

    // Fader for the audio volume
    Fader mVolumeFader;

    // Fader for the relative play speed
    Fader mRelativePlaySpeedFader;

    // Fader used to schedule pausing of the stream
    Fader mPauseScheduler;

    // Fader used to schedule stopping of the stream
    Fader mStopScheduler;

    // Affected by some fader
    int mActiveFader = 0;

    // Current channel volumes, used to ramp the volume changes to avoid clicks
    std::array<float, MAX_CHANNELS> mCurrentChannelVolume{};

    // ID of the sound source that generated this instance
    size_t mAudioSourceID = 0;

    // Handle of the bus this audio instance is playing on. 0 for root.
    size_t mBusHandle = ~0u;

    // Filter pointer
    std::array<std::shared_ptr<FilterInstance>, FILTERS_PER_STREAM> mFilter{};

    // Initialize instance. Mostly internal use.
    void init(AudioSource& aSource, int aPlayIndex);

    // Pointers to buffers for the resampler
    std::array<float*, 2> mResampleData{};

    // Sub-sample playhead; 16.16 fixed point
    size_t mSrcOffset = 0;

    // Samples left over from earlier pass
    size_t mLeftoverSamples = 0;

    // Number of samples to delay streaming
    size_t mDelaySamples = 0;

    // When looping, start playing from this time
    time_t mLoopPoint = 0;

    // Get N samples from the stream to the buffer. Report samples written.
    virtual size_t getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize) = 0;

    // Has the stream ended?
    virtual bool hasEnded() = 0;

    // Seek to certain place in the stream. Base implementation is generic "tape" seek (and slow).
    virtual bool seek(time_t aSeconds, float* mScratch, size_t mScratchSize);

    // Rewind stream. Base implementation returns NOT_IMPLEMENTED, meaning it can't rewind.
    virtual bool rewind();

    // Get information. Returns 0 by default.
    virtual float getInfo(size_t aInfoKey);
};

static inline AudioSourceInstanceFlags operator&(AudioSourceInstanceFlags lhs,
                                                 AudioSourceInstanceFlags rhs)
{
    return AudioSourceInstanceFlags(int(lhs) & int(rhs));
}

static inline AudioSourceInstanceFlags operator|(AudioSourceInstanceFlags lhs,
                                                 AudioSourceInstanceFlags rhs)
{
    return AudioSourceInstanceFlags(int(lhs) | int(rhs));
}

static inline AudioSourceInstanceFlags operator~(AudioSourceInstanceFlags value)
{
    return AudioSourceInstanceFlags(~int(value));
}

static inline AudioSourceInstanceFlags& operator&=(AudioSourceInstanceFlags& lhs,
                                                   AudioSourceInstanceFlags  rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

static inline AudioSourceInstanceFlags& operator|=(AudioSourceInstanceFlags& lhs,
                                                   AudioSourceInstanceFlags  rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

static inline bool testFlag(AudioSourceInstanceFlags value, AudioSourceInstanceFlags toTest)
{
    return (int(value) & int(toTest)) == int(toTest);
}

class Engine;

// Base class for audio sources
class AudioSource
{
  public:
    enum FLAGS
    {
        // The instances from this audio source should loop
        SHOULD_LOOP = 1,
        // Only one instance of this audio source should play at the same time
        SINGLE_INSTANCE = 2,
        // Visualization data gathering enabled. Only for busses.
        VISUALIZATION_DATA = 4,
        // Audio instances created from this source are affected by 3d processing
        PROCESS_3D = 8,
        // Audio instances created from this source have listener-relative 3d coordinates
        LISTENER_RELATIVE = 16,
        // Delay start of sound by the distance from listener
        DISTANCE_DELAY = 32,
        // If inaudible, should be killed (default)
        INAUDIBLE_KILL = 64,
        // If inaudible, should still be ticked (default = pause)
        INAUDIBLE_TICK = 128,
        // Disable auto-stop
        DISABLE_AUTOSTOP = 256
    };

    enum ATTENUATION_MODELS
    {
        // No attenuation
        NO_ATTENUATION = 0,
        // Inverse distance attenuation model
        INVERSE_DISTANCE = 1,
        // Linear distance attenuation model
        LINEAR_DISTANCE = 2,
        // Exponential distance attenuation model
        EXPONENTIAL_DISTANCE = 3
    };

    AudioSource() = default;

    virtual ~AudioSource() noexcept;

    // Create instance from the audio source. Called from within Soloud class.
    virtual std::shared_ptr<AudioSourceInstance> createInstance() = 0;

    // Set filter. Set to nullptr to clear the filter.
    virtual void setFilter(size_t aFilterId, Filter* aFilter);

    // Stop all instances of this audio source
    void stop();

    // Set default volume for instances
    void setVolume(float aVolume);

    // Set the looping of the instances created from this audio source
    void setLooping(bool aLoop);

    // Set whether only one instance of this sound should ever be playing at the same time
    void setSingleInstance(bool aSingleInstance);

    // Set whether audio should auto-stop when it ends or not
    void setAutoStop(bool aAutoStop);

    // Set the minimum and maximum distances for 3d audio source (closer to min distance = max vol)
    void set3dMinMaxDistance(float aMinDistance, float aMaxDistance);

    // Set attenuation model and rolloff factor for 3d audio source
    void set3dAttenuation(size_t aAttenuationModel, float aAttenuationRolloffFactor);

    // Set doppler factor to reduce or enhance doppler effect, default = 1.0
    void set3dDopplerFactor(float aDopplerFactor);

    // Set the coordinates for this audio source to be relative to listener's coordinates.
    void set3dListenerRelative(bool aListenerRelative);

    // Enable delaying the start of the sound based on the distance.
    void set3dDistanceDelay(bool aDistanceDelay);

    // Set a custom 3d audio collider. Set to nullptr to disable.
    void set3dCollider(AudioCollider* aCollider, int aUserData = 0);

    // Set a custom attenuator. Set to nullptr to disable.
    void set3dAttenuator(AudioAttenuator* aAttenuator);

    // Set behavior for inaudible sounds
    void setInaudibleBehavior(bool aMustTick, bool aKill);

    // Set time to jump to when looping
    void setLoopPoint(time_t aLoopPoint);

    // Get current loop point value
    time_t getLoopPoint();

    // Flags. See AudioSource::FLAGS
    size_t mFlags = 0;

    // Base sample rate, used to initialize instances
    float mBaseSamplerate = 44'100.0f;

    // Default volume for created instances
    float mVolume = 1.0f;

    // Number of channels this audio source produces
    size_t mChannels = 1;

    // Sound source ID. Assigned by SoLoud the first time it's played.
    size_t mAudioSourceID = 0;

    // 3d min distance
    float m3dMinDistance = 1.0f;

    // 3d max distance
    float m3dMaxDistance = 1'000'000.0f;

    // 3d attenuation rolloff factor
    float m3dAttenuationRolloff = 1.0f;

    // 3d attenuation model
    size_t m3dAttenuationModel = NO_ATTENUATION;

    // 3d doppler factor
    float m3dDopplerFactor = 1.0f;

    // Filter pointer
    std::array<Filter*, FILTERS_PER_STREAM> mFilter{};

    // Pointer to the Soloud object. Needed to stop all instances in dtor.
    Engine* mSoloud = nullptr;

    // Pointer to a custom audio collider object
    AudioCollider* mCollider = nullptr;

    // Pointer to custom attenuator object
    AudioAttenuator* mAttenuator = nullptr;

    // User data related to audio collider
    int mColliderData = 0;

    // When looping, start playing from this time
    time_t mLoopPoint = 0;
};
}; // namespace SoLoud
