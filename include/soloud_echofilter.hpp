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

#pragma once

#include "soloud_filter.hpp"

namespace SoLoud
{
class EchoFilter;

class EchoFilterInstance : public FilterInstance
{
    std::unique_ptr<float[]> mBuffer;
    int                      mBufferLength;
    int                      mBufferMaxLength;
    int                      mOffset;

  public:
    void filter(float* aBuffer,
                size_t aSamples,
                size_t aBufferSize,
                size_t aChannels,
                float  aSamplerate,
                time_t aTime) override;

    explicit EchoFilterInstance(EchoFilter* aParent);
};

class EchoFilter : public Filter
{
  public:
    enum FILTERATTRIBUTE
    {
        WET = 0,
        DELAY,
        DECAY,
        FILTER
    };

    std::shared_ptr<FilterInstance> createInstance() override;

    float mDelay  = 0.3f;
    float mDecay  = 0.7f;
    float mFilter = 0.0f;
};
} // namespace SoLoud
