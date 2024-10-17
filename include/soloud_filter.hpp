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

#include "soloud_fader.hpp"
#include <memory>

namespace SoLoud
{
class FilterInstance
{
  public:
    FilterInstance() = default;

    virtual void initParams(int aNumParams);

    virtual void updateParams(time_t aTime);

    virtual void filter(float*       aBuffer,
                        size_t aSamples,
                        size_t aBufferSize,
                        size_t aChannels,
                        float        aSamplerate,
                        time_t       aTime);

    virtual void filterChannel(float*       aBuffer,
                               size_t aSamples,
                               float        aSamplerate,
                               time_t       aTime,
                               size_t aChannel,
                               size_t aChannels);

    virtual float getFilterParameter(size_t aAttributeId);

    virtual void setFilterParameter(size_t aAttributeId, float aValue);

    virtual void fadeFilterParameter(size_t aAttributeId,
                                     float        aTo,
                                     time_t       aTime,
                                     time_t       aStartTime);

    virtual void oscillateFilterParameter(
        size_t aAttributeId, float aFrom, float aTo, time_t aTime, time_t aStartTime);

  protected:
    size_t mNumParams    = 0;
    size_t mParamChanged = 0;
    std::unique_ptr<float[]>       mParam       ;
    std::unique_ptr<Fader[]>       mParamFader   ;
};

class Filter
{
  public:
    Filter() = default;

    virtual ~Filter() noexcept = default;

    virtual std::shared_ptr<FilterInstance> createInstance() = 0;
};
}; // namespace SoLoud
