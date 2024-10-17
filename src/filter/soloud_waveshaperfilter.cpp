/*
SoLoud audio engine
Copyright (c) 2013-2020 Jari Komppa

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

#include "soloud_waveshaperfilter.hpp"
#include <cmath>

namespace SoLoud
{

WaveShaperFilterInstance::WaveShaperFilterInstance(WaveShaperFilter* aParent)
{
    mParent = aParent;
    initParams(2);
    mParam[WaveShaperFilter::AMOUNT] = mParent->mAmount;
}

void WaveShaperFilterInstance::filterChannel(float*       aBuffer,
                                             unsigned int aSamples,
                                             float /*aSamplerate*/,
                                             double aTime,
                                             unsigned int /*aChannel*/,
                                             unsigned int /*aChannels*/)
{
    updateParams(aTime);

    unsigned int i;
    float        k = 0;
    if (mParam[1] == 1)
        k = 2 * mParam[WaveShaperFilter::AMOUNT] / 0.01f;
    else
        k = 2 * mParam[WaveShaperFilter::AMOUNT] / (1 - mParam[1]);

    for (i = 0; i < aSamples; i++)
    {
        float dry = aBuffer[i];
        float wet = (1 + k) * aBuffer[i] / (1 + k * (float)fabs(aBuffer[i]));
        aBuffer[i] += (wet - dry) * mParam[WaveShaperFilter::WET];
    }
}

WaveShaperFilterInstance::~WaveShaperFilterInstance()
{
}

void WaveShaperFilter::setParams(float aAmount)
{
    assert(aAmount >= -1 && aAmount <= 1);
    mAmount = aAmount;
}

WaveShaperFilter::WaveShaperFilter()
{
    mAmount = 0.0f;
}

WaveShaperFilter::~WaveShaperFilter()
{
}

WaveShaperFilterInstance* WaveShaperFilter::createInstance()
{
    return new WaveShaperFilterInstance(this);
}
} // namespace SoLoud
