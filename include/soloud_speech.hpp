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

#include "../src/audiosource/darray.h"
#include "../src/audiosource/klatt.h"
#include "soloud_audiosource.hpp"
#include <memory>

namespace SoLoud
{
class Speech : public AudioSource
{
  public:
    int    mBaseFrequency;
    float  mBaseSpeed;
    float  mBaseDeclination;
    int    mBaseWaveform;
    int    mFrames;
    darray mElement;

    Speech();

    void setText(const char* aText);

    void setParams(size_t aBaseFrequency   = 1330,
                   float  aBaseSpeed       = 10.0f,
                   float  aBaseDeclination = 0.5f,
                   int    aBaseWaveform    = KW_TRIANGLE);

    ~Speech() override;

    std::shared_ptr<AudioSourceInstance> createInstance() override;
};

class SpeechInstance : public AudioSourceInstance
{
  public:
    explicit SpeechInstance(Speech* aParent);

    size_t getAudio(float* aBuffer, size_t aSamplesToRead, size_t aBufferSize) override;

    bool rewind() override;

    bool hasEnded() override;

  private:
    Speech*                  mParent = nullptr;
    klatt                    mSynth;
    std::unique_ptr<short[]> mSample;
    size_t                   mSampleCount = 0;
    size_t                   mOffset      = 0;
};
}; // namespace SoLoud
