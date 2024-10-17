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

#include "soloud_queue.hpp"
#include "soloud_engine.hpp"

namespace SoLoud
{
QueueInstance::QueueInstance(Queue* aParent)
{
    mParent = aParent;
    mFlags |= AudioSourceInstanceFlags::Protected;
}

size_t QueueInstance::getAudio(float*       aBuffer,
                                     size_t aSamplesToRead,
                                     size_t aBufferSize)
{
    if (mParent->mCount == 0)
    {
        return 0;
    }
    size_t copycount = aSamplesToRead;
    size_t copyofs   = 0;
    while (copycount && mParent->mCount)
    {
        int readcount = mParent->mSource[mParent->mReadIndex]->getAudio(aBuffer + copyofs,
                                                                        copycount,
                                                                        aBufferSize);
        copyofs += readcount;
        copycount -= readcount;
        if (mParent->mSource[mParent->mReadIndex]->hasEnded())
        {
            delete mParent->mSource[mParent->mReadIndex];
            mParent->mSource[mParent->mReadIndex] = 0;
            mParent->mReadIndex                   = (mParent->mReadIndex + 1) % SOLOUD_QUEUE_MAX;
            mParent->mCount--;
            mLoopCount++;
        }
    }
    return copyofs;
}

bool QueueInstance::hasEnded()
{
    return mLoopCount != 0 && mParent->mCount == 0;
}

QueueInstance* Queue::createInstance()
{
    if (mInstance)
    {
        stop();
        mInstance = 0;
    }
    mInstance = new QueueInstance(this);
    return mInstance;
}

void Queue::findQueueHandle()
{
    // Find the channel the queue is playing on to calculate handle..
    for (int i = 0; mQueueHandle == 0 && i < (signed)mSoloud->mHighestVoice; i++)
    {
        if (mSoloud->mVoice[i] == mInstance)
        {
            mQueueHandle = mSoloud->getHandleFromVoice_internal(i);
        }
    }
}

void Queue::play(AudioSource& aSound)
{
    assert(mSoloud != nullptr);

    findQueueHandle();

    assert(mQueueHandle != 0);
    assert(mCount < SOLOUD_QUEUE_MAX);

    if (!aSound.mAudioSourceID)
    {
        aSound.mAudioSourceID = mSoloud->mAudioSourceID;
        mSoloud->mAudioSourceID++;
    }

    auto* instance = aSound.createInstance();

    instance->init(aSound, 0);
    instance->mAudioSourceID = aSound.mAudioSourceID;

    mSoloud->lockAudioMutex_internal();
    mSource[mWriteIndex] = instance;
    mWriteIndex          = (mWriteIndex + 1) % SOLOUD_QUEUE_MAX;
    mCount++;
    mSoloud->unlockAudioMutex_internal();
}


size_t Queue::getQueueCount() const
{
    if (!mSoloud)
    {
        return 0;
    }

    mSoloud->lockAudioMutex_internal();
    size_t count = mCount;
    mSoloud->unlockAudioMutex_internal();
    return count;
}

bool Queue::isCurrentlyPlaying(const AudioSource& aSound) const
{
    if (mSoloud == nullptr || mCount == 0 || aSound.mAudioSourceID == 0)
    {
        return false;
    }

    mSoloud->lockAudioMutex_internal();
    const bool res = mSource[mReadIndex]->mAudioSourceID == aSound.mAudioSourceID;
    mSoloud->unlockAudioMutex_internal();
    return res;
}

void Queue::setParamsFromAudioSource(const AudioSource& aSound)
{
    mChannels       = aSound.mChannels;
    mBaseSamplerate = aSound.mBaseSamplerate;
}

void Queue::setParams(float aSamplerate, size_t aChannels)
{
    assert(aChannels >= 1);
    assert(aChannels <= MAX_CHANNELS);

    mChannels       = aChannels;
    mBaseSamplerate = aSamplerate;
}
}; // namespace SoLoud
