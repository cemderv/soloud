/*
SoLoud audio engine
Copyright (c) 2013-2018 Jari Komppa

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

struct stb_vorbis;

namespace SoLoud
{
class Wav;
class File;
class MemoryFile;

class WavInstance : public AudioSourceInstance
{
    Wav*         mParent;
    unsigned int mOffset;

public:
    explicit WavInstance(Wav* aParent);

    unsigned int getAudio(float*       aBuffer,
                          unsigned int aSamplesToRead,
                          unsigned int aBufferSize) override;

    result rewind() override;

    bool hasEnded() override;
};

class Wav : public AudioSource
{
    result loadwav(MemoryFile* aReader);
    result loadogg(MemoryFile* aReader);
    result loadmp3(MemoryFile* aReader);
    result loadflac(MemoryFile* aReader);
    result testAndLoadFile(MemoryFile* aReader);

public:
    float*       mData;
    unsigned int mSampleCount;

    Wav();

    ~Wav() override;

    result loadMem(const unsigned char* aMem,
                   unsigned int         aLength,
                   bool                 aCopy          = false,
                   bool                 aTakeOwnership = true);

    result loadFile(File* aFile);

    result loadRawWave8(unsigned char* aMem,
                        unsigned int   aLength,
                        float          aSamplerate = 44100.0f,
                        unsigned int   aChannels   = 1);

    result loadRawWave16(short*       aMem,
                         unsigned int aLength,
                         float        aSamplerate = 44100.0f,
                         unsigned int aChannels   = 1);

    result loadRawWave(float*       aMem,
                       unsigned int aLength,
                       float        aSamplerate    = 44100.0f,
                       unsigned int aChannels      = 1,
                       bool         aCopy          = false,
                       bool         aTakeOwnership = true);

    AudioSourceInstance* createInstance() override;
    time                 getLength();
};
}; // namespace SoLoud
