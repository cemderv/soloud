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

#include "../src/audiosource/speech/darray.h"
#include "../src/audiosource/speech/klatt.h"
#include "soloud.hpp"

namespace SoLoud
{
class Speech;

class Speech : public AudioSource
{
    // copy of the enum in klatt.h for codegen purposes
    enum KLATT_WAVEFORM
    {
        KW_SAW,
        KW_TRIANGLE,
        KW_SIN,
        KW_SQUARE,
        KW_PULSE,
        KW_NOISE,
        KW_WARBLE
    };

public:
    int    mBaseFrequency;
    float  mBaseSpeed;
    float  mBaseDeclination;
    int    mBaseWaveform;
    int    mFrames;
    darray mElement;
    Speech();
    result setText(const char* aText);
    result setParams(unsigned int aBaseFrequency   = 1330,
                     float        aBaseSpeed       = 10.0f,
                     float        aBaseDeclination = 0.5f,
                     int          aBaseWaveform    = KW_TRIANGLE);
    ~Speech() override;
    AudioSourceInstance* createInstance() override;
};

class SpeechInstance : public AudioSourceInstance
{
    klatt   mSynth;
    Speech* mParent;
    short*  mSample;
    int     mSampleCount;
    int     mOffset;

public:
    explicit SpeechInstance(Speech* aParent);
    ~SpeechInstance() override;
    unsigned int getAudio(float*       aBuffer,
                          unsigned int aSamplesToRead,
                          unsigned int aBufferSize) override;
    result rewind() override;
    bool   hasEnded() override;
};
}; // namespace SoLoud
