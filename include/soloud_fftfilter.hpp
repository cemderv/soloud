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

#include "soloud_filter.hpp"

namespace SoLoud
{
class FFTFilter;

class FFTFilterInstance : public FilterInstance
{
    std::unique_ptr<float[]> mTemp;
    std::unique_ptr<float[]> mInputBuffer;
    std::unique_ptr<float[]> mMixBuffer;
    std::unique_ptr<float[]> mLastPhase;
    std::unique_ptr<float[]> mSumPhase;
    size_t                   mInputOffset[MAX_CHANNELS];
    size_t                   mMixOffset[MAX_CHANNELS];
    size_t                   mReadOffset[MAX_CHANNELS];
    FFTFilter*               mParent;

  public:
    virtual void fftFilterChannel(float* aFFTBuffer,
                                  size_t aSamples,
                                  float  aSamplerate,
                                  time_t aTime,
                                  size_t aChannel,
                                  size_t aChannels);
    void         filterChannel(float* aBuffer,
                               size_t aSamples,
                               float  aSamplerate,
                               time_t aTime,
                               size_t aChannel,
                               size_t aChannels) override;

    explicit FFTFilterInstance(FFTFilter* aParent);
    FFTFilterInstance();
    void comp2MagPhase(float* aFFTBuffer, size_t aSamples);
    void magPhase2MagFreq(float* aFFTBuffer, size_t aSamples, float aSamplerate, size_t aChannel);
    void magFreq2MagPhase(float* aFFTBuffer, size_t aSamples, float aSamplerate, size_t aChannel);
    void magPhase2Comp(float* aFFTBuffer, size_t aSamples);
    void init();
};

class FFTFilter : public Filter
{
  public:
    std::shared_ptr<FilterInstance> createInstance() override;
};
} // namespace SoLoud
