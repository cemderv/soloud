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
#include <memory>

namespace SoLoud
{
class DCRemovalFilter;

class DCRemovalFilterInstance : public FilterInstance
{
  public:
    void filter(float* aBuffer,
                size_t aSamples,
                size_t aBufferSize,
                size_t aChannels,
                float  aSamplerate,
                time_t aTime) override;

    explicit DCRemovalFilterInstance(DCRemovalFilter* aParent);

  private:
    std::unique_ptr<float[]> mBuffer;
    std::unique_ptr<float[]> mTotals;
    int                      mBufferLength;
    DCRemovalFilter*         mParent;
    int                      mOffset;
};

class DCRemovalFilter : public Filter
{
  public:
    std::shared_ptr<FilterInstance> createInstance() override;

    float mLength = 0.1f;
};
} // namespace SoLoud
