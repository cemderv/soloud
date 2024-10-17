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

#pragma once

#include "soloud_filter.hpp"

namespace SoLoud
{
class BiquadResonantFilter;

struct BQRStateData
{
    float mY1, mY2, mX1, mX2;
};

class BiquadResonantFilterInstance : public FilterInstance
{
  public:
    explicit BiquadResonantFilterInstance(BiquadResonantFilter* aParent);

    void filterChannel(float* aBuffer,
                       size_t aSamples,
                       float  aSamplerate,
                       time_t aTime,
                       size_t aChannel,
                       size_t aChannels) override;

  protected:
    enum FilterAttribute
    {
        Wet = 0,
        Type,
        Frequency,
        Resonance
    };

    std::array<BQRStateData, 8> mState{};
    float                       mA0 = 0.0f;
    float                       mA1 = 0.0f;
    float                       mA2 = 0.0f;
    float                       mB1 = 0.0f;
    float                       mB2 = 0.0f;
    int                         mDirty{};
    float                       mSamplerate;

    BiquadResonantFilter* mParent;
    void                  calcBQRParams();
};

class BiquadResonantFilter : public Filter
{
  public:
    enum FILTERTYPE
    {
        LOWPASS  = 0,
        HIGHPASS = 1,
        BANDPASS = 2
    };

    enum FILTERATTRIBUTE
    {
        WET = 0,
        TYPE,
        FREQUENCY,
        RESONANCE
    };

    std::shared_ptr<FilterInstance> createInstance() override;

    int   mFilterType = LOWPASS;
    float mFrequency  = 1000.0f;
    float mResonance  = 2.0f;
};
} // namespace SoLoud
