/*
SoLoud audio engine
Copyright (c) 2013-2018 Jari Komppa

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

#define SOLOUD_QUEUE_MAX 32

namespace SoLoud
{
class Queue;

class QueueInstance : public AudioSourceInstance
{
    Queue* mParent = nullptr;

public:
    explicit QueueInstance(Queue* aParent);

    unsigned int getAudio(float*       aBuffer,
                          unsigned int aSamplesToRead,
                          unsigned int aBufferSize) override;

    bool hasEnded() override;
};

class Queue : public AudioSource
{
public:
    QueueInstance* createInstance() override;

    // Play sound through the queue
    result play(AudioSource& aSound);

    // Number of audio sources queued for replay
    unsigned int getQueueCount();

    // Is this audio source currently playing?
    bool isCurrentlyPlaying(AudioSource& aSound);

    // Set params by reading them from an audio source
    result setParamsFromAudioSource(AudioSource& aSound);

    // Set params manually
    result setParams(float aSamplerate, unsigned int aChannels = 2);

    void findQueueHandle();

    unsigned int                                       mReadIndex  = 0;
    unsigned int                                       mWriteIndex = 0;
    unsigned int                                       mCount      = 0;
    std::array<AudioSourceInstance*, SOLOUD_QUEUE_MAX> mSource{};
    QueueInstance*                                     mInstance    = nullptr;
    handle                                             mQueueHandle = 0;

};
}; // namespace SoLoud
