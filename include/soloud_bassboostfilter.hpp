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

#include "soloud.hpp"
#include "soloud_fftfilter.hpp"

namespace SoLoud
{
class BassboostFilter;

class BassboostFilterInstance : public FFTFilterInstance
{
  public:
    BassboostFilterInstance(BassboostFilter* aParent);

    void fftFilterChannel(float*       aFFTBuffer,
                          size_t aSamples,
                          float        aSamplerate,
                          time_t       aTime,
                          size_t aChannel,
                          size_t aChannels) override;

  protected:
    enum FILTERATTRIBUTE
    {
        WET   = 0,
        BOOST = 1
    };

    BassboostFilter* mParent;
};

class BassboostFilter : public FFTFilter
{
  public:
    enum FILTERATTRIBUTE
    {
        WET   = 0,
        BOOST = 1
    };

    FilterInstance* createInstance() override;

    float mBoost = 2.0f;
};
} // namespace SoLoud
