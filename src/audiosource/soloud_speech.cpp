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
#include "soloud_speech.hpp"
#include "../src/audiosource/tts.h"

namespace SoLoud
{
SpeechInstance::SpeechInstance(Speech* aParent)
    : mParent(aParent)
    , mSynth(mParent->mBaseFrequency,
             mParent->mBaseSpeed,
             mParent->mBaseDeclination,
             mParent->mBaseWaveform)
{
    mSample = std::make_unique<short[]>(mSynth.mNspFr * 100);

    mSynth.initsynth(mParent->mElement.getSize(),
                     reinterpret_cast<unsigned char*>(mParent->mElement.getData()));

    mOffset      = 10;
    mSampleCount = 10;
}

static void writesamples(short* aSrc, float* aDst, int aCount)
{
    for (int i = 0; i < aCount; i++)
    {
        aDst[i] = aSrc[i] * (1 / (float)0x8000);
    }
}

size_t SpeechInstance::getAudio(float*       aBuffer,
                                      size_t aSamplesToRead,
                                      size_t /*aBufferSize*/)
{
    // TODO: check this, was .init() before (mLastElement etc)
    mSynth = klatt{mParent->mBaseFrequency,
                   mParent->mBaseSpeed,
                   mParent->mBaseDeclination,
                   mParent->mBaseWaveform};

    size_t samples_out = 0;
    if (mSampleCount > mOffset)
    {
        size_t copycount = mSampleCount - mOffset;
        if (copycount > aSamplesToRead)
        {
            copycount = aSamplesToRead;
        }
        writesamples(mSample.get() + mOffset, aBuffer, copycount);
        mOffset += copycount;
        samples_out += copycount;
    }

    while (mSampleCount >= 0 && samples_out < aSamplesToRead)
    {
        mOffset      = 0;
        mSampleCount = mSynth.synth(mSynth.mNspFr, mSample.get());
        if (mSampleCount > 0)
        {
            size_t copycount = mSampleCount;
            if (copycount > aSamplesToRead - samples_out)
            {
                copycount = aSamplesToRead - samples_out;
            }
            writesamples(mSample.get(), aBuffer + samples_out, copycount);
            mOffset += copycount;
            samples_out += copycount;
        }
    }
    return samples_out;
}

bool SpeechInstance::rewind()
{
    // TODO: check this, was .init() before (mLastElement etc)
    mSynth = klatt{mParent->mBaseFrequency,
                   mParent->mBaseSpeed,
                   mParent->mBaseDeclination,
                   mParent->mBaseWaveform};

    mSynth.initsynth(mParent->mElement.getSize(),
                     reinterpret_cast<unsigned char*>(mParent->mElement.getData()));

    mOffset         = 10;
    mSampleCount    = 10;
    mStreamPosition = 0.0f;

    return true;
}

bool SpeechInstance::hasEnded()
{
    return mSampleCount < 0;
}

void Speech::setParams(size_t aBaseFrequency,
                       float        aBaseSpeed,
                       float        aBaseDeclination,
                       int          aBaseWaveform)
{
    mBaseFrequency   = aBaseFrequency;
    mBaseSpeed       = aBaseSpeed;
    mBaseDeclination = aBaseDeclination;
    mBaseWaveform    = aBaseWaveform;
}

void Speech::setText(const char* aText)
{
    stop();
    mElement.clear();
    darray phone;
    xlate_string(aText, &phone);
    mFrames = klatt::phone_to_elm(phone.getData(), phone.getSize(), &mElement);
}

Speech::Speech()
{
    mBaseSamplerate  = 11025;
    mFrames          = 0;
    mBaseFrequency   = 1330;
    mBaseSpeed       = 10;
    mBaseDeclination = 0.5f;
    mBaseWaveform    = KW_SQUARE;
}

Speech::~Speech()
{
    stop();
}

AudioSourceInstance* Speech::createInstance()
{
    return new SpeechInstance(this);
}
}; // namespace SoLoud
