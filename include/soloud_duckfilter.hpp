/*
SoLoud audio engine
Copyright (c) 2013-2021 Jari Komppa

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
class DuckFilter;

class DuckFilterInstance : public FilterInstance
{
  public:
    void filter(float*       aBuffer,
                unsigned int aSamples,
                unsigned int aBufferSize,
                unsigned int aChannels,
                float        aSamplerate,
                time_t       aTime) override;

    ~DuckFilterInstance() override;

    explicit DuckFilterInstance(DuckFilter* aParent);

  private:
    handle  mListenTo;
    Soloud* mSoloud;
    float   mCurrentLevel;
};

class DuckFilter : public Filter
{
  public:
    enum FILTERATTRIBUTE
    {
        WET = 0,
        ONRAMP,
        OFFRAMP,
        LEVEL
    };

    FilterInstance* createInstance() override;

    Soloud* mSoloud   = nullptr;
    float   mOnRamp   = 0.1f;
    float   mOffRamp  = 0.5f;
    float   mLevel    = 0.5f;
    handle  mListenTo = 0;
};
} // namespace SoLoud
