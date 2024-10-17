/*
MONOTONE module for SoLoud audio engine
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

#include "soloud_audiosource.hpp"
#include <span>

namespace SoLoud
{
class Monotone;
class MemoryFile;

struct MonotoneSong
{
    char*         mTitle;
    char*         mComment;
    unsigned char mVersion; // must be 1
    unsigned char mTotalPatterns;
    unsigned char mTotalTracks;
    unsigned char mCellSize; // must be 2 for version 1
    unsigned char mOrder[256];
    size_t* mPatternData; // 64 rows * mTotalPatterns * mTotalTracks
};

struct MonotoneChannel
{
    int mEnabled;
    int mActive;
    int mFreq[3];
    int mPortamento;
    int mArpCounter;
    int mArp;
    int mLastNote;
    int mPortamentoToNote;
    int mVibrato;
    int mVibratoIndex;
    int mVibratoDepth;
    int mVibratoSpeed;
};

struct MonotoneHardwareChannel
{
    int   mEnabled;
    float mSamplePos;
    float mSamplePosInc;
};

class MonotoneInstance : public AudioSourceInstance
{
    Monotone* mParent;

  public:
    MonotoneChannel         mChannel[12]{};
    MonotoneHardwareChannel mOutput[12]{};
    int                     mNextChannel;
    int                     mTempo; // ticks / row. Tick = 60hz. Default 4.
    int                     mOrder;
    int                     mRow;
    int                     mSampleCount;
    int                     mRowTick;

    explicit MonotoneInstance(Monotone* aParent);
    size_t getAudio(float* aBuffer, size_t aSamples, size_t aBufferSize) override;
    bool         hasEnded() override;
};

class Monotone : public AudioSource
{
  public:
    int          mNotesHz[800];
    int          mVibTable[32];
    int          mHardwareChannels;
    Waveform     mWaveform;
    MonotoneSong mSong;

    Monotone();
    ~Monotone() override;

    void setParams(int aHardwareChannels, Waveform aWaveform = Waveform::Square);

    void loadMem(std::span<const std::byte> aData);

    AudioSourceInstance* createInstance() override;

  public:
    void clear();
};
}; // namespace SoLoud
