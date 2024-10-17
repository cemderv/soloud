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

#include "soloud_bassboostfilter.hpp"
#include "soloud.hpp"

namespace SoLoud
{
BassboostFilterInstance::BassboostFilterInstance(BassboostFilter* aParent)
{
    mParent = aParent;
    initParams(2);
    mParam[BOOST] = aParent->mBoost;
}

void BassboostFilterInstance::fftFilterChannel(float* aFFTBuffer,
                                               size_t /*aSamples*/,
                                               float /*aSamplerate*/,
                                               time_t /*aTime*/,
                                               size_t /*aChannel*/,
                                               size_t /*aChannels*/)
{
    comp2MagPhase(aFFTBuffer, 2);
    for (size_t i = 0; i < 2; i++)
    {
        aFFTBuffer[i * 2] *= mParam[BOOST];
    }
    magPhase2Comp(aFFTBuffer, 2);
}

std::shared_ptr<FilterInstance> BassboostFilter::createInstance()
{
    return std::make_shared<BassboostFilterInstance>(this);
}
} // namespace SoLoud
