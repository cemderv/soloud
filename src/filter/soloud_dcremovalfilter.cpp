/*
SoLoud audio engine
Copyright (c) 2015 Jari Komppa

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

#include "soloud_dcremovalfilter.hpp"

namespace SoLoud
{
DCRemovalFilterInstance::DCRemovalFilterInstance(DCRemovalFilter* aParent)
{
    mParent       = aParent;
    mBuffer       = 0;
    mBufferLength = 0;
    mTotals       = 0;
    mOffset       = 0;
    initParams(1);
}

void DCRemovalFilterInstance::filter(float* aBuffer,
                                     size_t aSamples,
                                     size_t aBufferSize,
                                     size_t aChannels,
                                     float  aSamplerate,
                                     double aTime)
{
    updateParams(aTime);

    if (mBuffer == 0)
    {
        mBufferLength = (int)ceil(mParent->mLength * aSamplerate);
        mBuffer       = std::make_unique<float[]>(mBufferLength * aChannels);
        mTotals       = std::make_unique<float[]>(aChannels);
    }

    size_t i, j;
    int    prevofs = (mOffset + mBufferLength - 1) % mBufferLength;
    for (i = 0; i < aSamples; i++)
    {
        for (j = 0; j < aChannels; j++)
        {
            int chofs  = j * mBufferLength;
            int bchofs = j * aBufferSize;

            float n = aBuffer[i + bchofs];
            mTotals[j] -= mBuffer[mOffset + chofs];
            mTotals[j] += n;
            mBuffer[mOffset + chofs] = n;

            n -= mTotals[j] / mBufferLength;

            aBuffer[i + bchofs] += (n - aBuffer[i + bchofs]) * mParam[0];
        }
        prevofs = mOffset;
        mOffset = (mOffset + 1) % mBufferLength;
    }
}

std::shared_ptr<FilterInstance> DCRemovalFilter::createInstance()
{
    return std::make_shared<DCRemovalFilterInstance>(this);
}
} // namespace SoLoud
