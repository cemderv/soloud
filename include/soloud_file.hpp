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
#include <cstdio>

typedef void* Soloud_Filehack;

namespace SoLoud
{
class File
{
  public:
    virtual ~File() noexcept = default;

    unsigned int         read8();
    unsigned int         read16();
    unsigned int         read32();
    virtual int          eof()                                          = 0;
    virtual unsigned int read(unsigned char* aDst, unsigned int aBytes) = 0;
    virtual unsigned int length()                                       = 0;
    virtual void         seek(int aOffset)                              = 0;
    virtual unsigned int pos()                                          = 0;

    virtual FILE* getFilePtr()
    {
        return nullptr;
    }

    virtual const unsigned char* getMemPtr()
    {
        return nullptr;
    }
};

class MemoryFile : public File
{
  public:
    const unsigned char* mDataPtr;
    unsigned int         mDataLength;
    unsigned int         mOffset;
    bool                 mDataOwned;

    int                  eof() override;
    unsigned int         read(unsigned char* aDst, unsigned int aBytes) override;
    unsigned int         length() override;
    void                 seek(int aOffset) override;
    unsigned int         pos() override;
    const unsigned char* getMemPtr() override;
    ~MemoryFile() override;
    MemoryFile();
    void openMem(const unsigned char* aData,
                 unsigned int         aDataLength,
                 bool                 aCopy          = false,
                 bool                 aTakeOwnership = true);
    void openFileToMem(File* aFile);
};
}; // namespace SoLoud
