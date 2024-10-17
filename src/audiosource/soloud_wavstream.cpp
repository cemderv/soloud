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

#include "dr_flac.h"
#include "dr_mp3.h"
#include "dr_wav.h"

#include "soloud.hpp"
#include "soloud_file.hpp"
#include "soloud_wavstream.hpp"
#include "stb_vorbis.h"
#include <cstring>

namespace SoLoud
{
size_t drflac_read_func(void* pUserData, void* pBufferOut, size_t bytesToRead)
{
    File* fp = (File*)pUserData;
    return fp->read((unsigned char*)pBufferOut, (unsigned int)bytesToRead);
}

size_t drmp3_read_func(void* pUserData, void* pBufferOut, size_t bytesToRead)
{
    File* fp = (File*)pUserData;
    return fp->read((unsigned char*)pBufferOut, (unsigned int)bytesToRead);
}

size_t drwav_read_func(void* pUserData, void* pBufferOut, size_t bytesToRead)
{
    File* fp = (File*)pUserData;
    return fp->read((unsigned char*)pBufferOut, (unsigned int)bytesToRead);
}

drflac_bool32 drflac_seek_func(void* pUserData, int offset, drflac_seek_origin origin)
{
    File* fp = (File*)pUserData;
    if (origin != drflac_seek_origin_start)
        offset += fp->pos();
    fp->seek(offset);
    return 1;
}

drmp3_bool32 drmp3_seek_func(void* pUserData, int offset, drmp3_seek_origin origin)
{
    File* fp = (File*)pUserData;
    if (origin != drmp3_seek_origin_start)
        offset += fp->pos();
    fp->seek(offset);
    return 1;
}

drmp3_bool32 drwav_seek_func(void* pUserData, int offset, drwav_seek_origin origin)
{
    File* fp = (File*)pUserData;
    if (origin != drwav_seek_origin_start)
        offset += fp->pos();
    fp->seek(offset);
    return 1;
}

WavStreamInstance::WavStreamInstance(WavStream* aParent)
{
    mOggFrameSize = 0;
    mParent       = aParent;
    mOffset       = 0;
    mCodec.mOgg   = 0;
    mCodec.mFlac  = 0;
    mFile         = 0;
    if (aParent->mMemFile)
    {
        MemoryFile* mf = new MemoryFile();
        mFile          = mf;
        mf->openMem(aParent->mMemFile->getMemPtr(), aParent->mMemFile->length(), false, false);
    }
    else if (aParent->mStreamFile)
    {
        mFile = aParent->mStreamFile;
        mFile->seek(0); // stb_vorbis assumes file offset to be at start of ogg
    }
    else
    {
        return;
    }

    if (mFile)
    {
        if (mParent->mFiletype == WAVSTREAM_WAV)
        {
            mCodec.mWav = new drwav;
            if (!drwav_init(mCodec.mWav, drwav_read_func, drwav_seek_func, (void*)mFile, nullptr))
            {
                delete mCodec.mWav;
                mCodec.mWav = 0;
                if (mFile != mParent->mStreamFile)
                    delete mFile;
                mFile = 0;
            }
        }
        else if (mParent->mFiletype == WAVSTREAM_OGG)
        {
            int e;

            mCodec.mOgg = stb_vorbis_open_file((Soloud_Filehack*)mFile, 0, &e, 0);

            if (!mCodec.mOgg)
            {
                if (mFile != mParent->mStreamFile)
                    delete mFile;
                mFile = 0;
            }
            mOggFrameSize   = 0;
            mOggFrameOffset = 0;
            mOggOutputs     = 0;
        }
        else if (mParent->mFiletype == WAVSTREAM_FLAC)
        {
            mCodec.mFlac = drflac_open(drflac_read_func, drflac_seek_func, (void*)mFile, nullptr);
            if (!mCodec.mFlac)
            {
                if (mFile != mParent->mStreamFile)
                    delete mFile;
                mFile = 0;
            }
        }
        else if (mParent->mFiletype == WAVSTREAM_MP3)
        {
            mCodec.mMp3 = new drmp3;
            if (!drmp3_init(mCodec.mMp3, drmp3_read_func, drmp3_seek_func, (void*)mFile, nullptr))
            {
                delete mCodec.mMp3;
                mCodec.mMp3 = 0;
                if (mFile != mParent->mStreamFile)
                    delete mFile;
                mFile = 0;
            }
        }
        else
        {
            if (mFile != mParent->mStreamFile)
                delete mFile;
            mFile = nullptr;
            return;
        }
    }
}

WavStreamInstance::~WavStreamInstance()
{
    switch (mParent->mFiletype)
    {
        case WAVSTREAM_OGG:
            if (mCodec.mOgg)
            {
                stb_vorbis_close(mCodec.mOgg);
            }
            break;
        case WAVSTREAM_FLAC:
            if (mCodec.mFlac)
            {
                drflac_close(mCodec.mFlac);
            }
            break;
        case WAVSTREAM_MP3:
            if (mCodec.mMp3)
            {
                drmp3_uninit(mCodec.mMp3);
                delete mCodec.mMp3;
                mCodec.mMp3 = 0;
            }
            break;
        case WAVSTREAM_WAV:
            if (mCodec.mWav)
            {
                drwav_uninit(mCodec.mWav);
                delete mCodec.mWav;
                mCodec.mWav = 0;
            }
            break;
    }
    if (mFile != mParent->mStreamFile)
    {
        delete mFile;
    }
}

static int getOggData(float** aOggOutputs,
                      float*  aBuffer,
                      int     aSamples,
                      int     aPitch,
                      int     aFrameSize,
                      int     aFrameOffset,
                      int     aChannels)
{
    if (aFrameSize <= 0)
        return 0;

    int samples = aSamples;
    if (aFrameSize - aFrameOffset < samples)
    {
        samples = aFrameSize - aFrameOffset;
    }

    int i;
    for (i = 0; i < aChannels; i++)
    {
        memcpy(aBuffer + aPitch * i, aOggOutputs[i] + aFrameOffset, sizeof(float) * samples);
    }
    return samples;
}


unsigned int WavStreamInstance::getAudio(float*       aBuffer,
                                         unsigned int aSamplesToRead,
                                         unsigned int aBufferSize)
{
    unsigned int offset = 0;
    float        tmp[512 * MAX_CHANNELS];
    if (mFile == nullptr)
        return 0;
    switch (mParent->mFiletype)
    {
        case WAVSTREAM_FLAC: {
            unsigned int i, j, k;

            for (i = 0; i < aSamplesToRead; i += 512)
            {
                unsigned int blockSize = (aSamplesToRead - i) > 512 ? 512 : aSamplesToRead - i;
                offset += (unsigned int)drflac_read_pcm_frames_f32(mCodec.mFlac, blockSize, tmp);

                for (j = 0; j < blockSize; j++)
                {
                    for (k = 0; k < mChannels; k++)
                    {
                        aBuffer[k * aSamplesToRead + i + j] = tmp[j * mCodec.mFlac->channels + k];
                    }
                }
            }
            mOffset += offset;
            return offset;
        }
        break;
        case WAVSTREAM_MP3: {
            unsigned int i, j, k;

            for (i = 0; i < aSamplesToRead; i += 512)
            {
                unsigned int blockSize = (aSamplesToRead - i) > 512 ? 512 : aSamplesToRead - i;
                offset += (unsigned int)drmp3_read_pcm_frames_f32(mCodec.mMp3, blockSize, tmp);

                for (j = 0; j < blockSize; j++)
                {
                    for (k = 0; k < mChannels; k++)
                    {
                        aBuffer[k * aSamplesToRead + i + j] = tmp[j * mCodec.mMp3->channels + k];
                    }
                }
            }
            mOffset += offset;
            return offset;
        }
        break;
        case WAVSTREAM_OGG: {
            if (mOggFrameOffset < mOggFrameSize)
            {
                int b = getOggData(mOggOutputs,
                                   aBuffer,
                                   aSamplesToRead,
                                   aBufferSize,
                                   mOggFrameSize,
                                   mOggFrameOffset,
                                   mChannels);
                mOffset += b;
                offset += b;
                mOggFrameOffset += b;
            }

            while (offset < aSamplesToRead)
            {
                mOggFrameSize   = stb_vorbis_get_frame_float(mCodec.mOgg, nullptr, &mOggOutputs);
                mOggFrameOffset = 0;
                int b           = getOggData(mOggOutputs,
                                   aBuffer + offset,
                                   aSamplesToRead - offset,
                                   aBufferSize,
                                   mOggFrameSize,
                                   mOggFrameOffset,
                                   mChannels);
                mOffset += b;
                offset += b;
                mOggFrameOffset += b;

                if (mOffset >= mParent->mSampleCount || b == 0)
                {
                    mOffset += offset;
                    return offset;
                }
            }
        }
        break;
        case WAVSTREAM_WAV: {
            unsigned int i, j, k;

            for (i = 0; i < aSamplesToRead; i += 512)
            {
                unsigned int blockSize = (aSamplesToRead - i) > 512 ? 512 : aSamplesToRead - i;
                offset += (unsigned int)drwav_read_pcm_frames_f32(mCodec.mWav, blockSize, tmp);

                for (j = 0; j < blockSize; j++)
                {
                    for (k = 0; k < mChannels; k++)
                    {
                        aBuffer[k * aSamplesToRead + i + j] = tmp[j * mCodec.mWav->channels + k];
                    }
                }
            }
            mOffset += offset;
            return offset;
        }
        break;
    }
    return aSamplesToRead;
}

bool WavStreamInstance::seek(double aSeconds, float* mScratch, unsigned int mScratchSize)
{
    if (mCodec.mOgg)
    {
        const auto pos = int(floor(mBaseSamplerate * aSeconds));
        stb_vorbis_seek(mCodec.mOgg, pos);
        // Since the position that we just sought to might not be *exactly*
        // the position we asked for, we're re-calculating the position just
        // for the sake of correctness.
        mOffset            = stb_vorbis_get_sample_offset(mCodec.mOgg);
        double newPosition = float(mOffset / mBaseSamplerate);
        mStreamPosition    = newPosition;

        return false;
    }
    else
    {
        return AudioSourceInstance::seek(aSeconds, mScratch, mScratchSize);
    }
}

bool WavStreamInstance::rewind()
{
    switch (mParent->mFiletype)
    {
        case WAVSTREAM_OGG:
            if (mCodec.mOgg)
            {
                stb_vorbis_seek_start(mCodec.mOgg);
            }
            break;
        case WAVSTREAM_FLAC:
            if (mCodec.mFlac)
            {
                drflac_seek_to_pcm_frame(mCodec.mFlac, 0);
            }
            break;
        case WAVSTREAM_MP3:
            if (mCodec.mMp3)
            {
                drmp3_seek_to_pcm_frame(mCodec.mMp3, 0);
            }
            break;
        case WAVSTREAM_WAV:
            if (mCodec.mWav)
            {
                drwav_seek_to_pcm_frame(mCodec.mWav, 0);
            }
            break;
    }
    mOffset         = 0;
    mStreamPosition = 0.0f;
    return 0;
}

bool WavStreamInstance::hasEnded()
{
    if (mOffset >= mParent->mSampleCount)
    {
        return 1;
    }
    return 0;
}

WavStream::WavStream()
{
    mSampleCount = 0;
    mFiletype    = WAVSTREAM_WAV;
    mMemFile     = 0;
    mStreamFile  = 0;
}

WavStream::~WavStream()
{
    stop();
    delete mMemFile;
}

#define MAKEDWORD(a, b, c, d) (((d) << 24) | ((c) << 16) | ((b) << 8) | (a))

void WavStream::loadwav(File& fp)
{
    fp.seek(0);
    drwav decoder;

    if (!drwav_init(&decoder, drwav_read_func, drwav_seek_func, &fp, nullptr))
    {
        throw std::runtime_error{"Failed to load WAV file"};
    }

    mChannels = decoder.channels;
    if (mChannels > MAX_CHANNELS)
    {
        mChannels = MAX_CHANNELS;
    }

    mBaseSamplerate = float(decoder.sampleRate);
    mSampleCount    = (unsigned int)decoder.totalPCMFrameCount;
    mFiletype       = WAVSTREAM_WAV;
    drwav_uninit(&decoder);
}

void WavStream::loadogg(File& fp)
{
    fp.seek(0);

    int         e = 0;
    stb_vorbis* v = stb_vorbis_open_file(reinterpret_cast<Soloud_Filehack*>(&fp), 0, &e, 0);

    if (v == nullptr)
    {
        throw std::runtime_error{"Failed to load OGG file"};
    }

    stb_vorbis_info info = stb_vorbis_get_info(v);
    mChannels            = info.channels;
    if (info.channels > MAX_CHANNELS)
    {
        mChannels = MAX_CHANNELS;
    }
    mBaseSamplerate = (float)info.sample_rate;
    int samples     = stb_vorbis_stream_length_in_samples(v);
    stb_vorbis_close(v);
    mFiletype = WAVSTREAM_OGG;

    mSampleCount = samples;
}

void WavStream::loadflac(File& fp)
{
    fp.seek(0);
    drflac* decoder = drflac_open(drflac_read_func, drflac_seek_func, &fp, nullptr);

    if (decoder == nullptr)
    {
        throw std::runtime_error{"Failed to load FLAC file"};
    }

    mChannels = decoder->channels;
    if (mChannels > MAX_CHANNELS)
    {
        mChannels = MAX_CHANNELS;
    }

    mBaseSamplerate = (float)decoder->sampleRate;
    mSampleCount    = (unsigned int)decoder->totalPCMFrameCount;
    mFiletype       = WAVSTREAM_FLAC;
    drflac_close(decoder);
}

void WavStream::loadmp3(File& fp)
{
    fp.seek(0);

    drmp3 decoder;
    if (!drmp3_init(&decoder, drmp3_read_func, drmp3_seek_func, &fp, nullptr))
    {
        throw std::runtime_error{"Failed to load MP3 file"};
    }

    mChannels = decoder.channels;
    if (mChannels > MAX_CHANNELS)
    {
        mChannels = MAX_CHANNELS;
    }

    drmp3_uint64 samples = drmp3_get_pcm_frame_count(&decoder);

    mBaseSamplerate = float(decoder.sampleRate);
    mSampleCount    = (unsigned int)samples;
    mFiletype       = WAVSTREAM_MP3;
    drmp3_uninit(&decoder);
}

void WavStream::loadMem(const unsigned char* aData,
                        unsigned int         aDataLen,
                        bool                 aCopy,
                        bool                 aTakeOwnership)
{
    delete mMemFile;
    mStreamFile  = nullptr;
    mMemFile     = nullptr;
    mSampleCount = 0;

    assert(aData != nullptr);
    assert(aDataLen > 0);

    auto* mf = new MemoryFile();

    try
    {
        mf->openMem(aData, aDataLen, aCopy, aTakeOwnership);
        parse(*mf);
    }
    catch (const std::exception&)
    {
        delete mf;
        throw;
    }

    mMemFile = mf;
}

void WavStream::parse(File& aFile)
{
    switch (aFile.read32())
    {
        case MAKEDWORD('O', 'g', 'g', 'S'): loadogg(aFile); break;
        case MAKEDWORD('R', 'I', 'F', 'F'): loadwav(aFile); break;
        case MAKEDWORD('f', 'L', 'a', 'C'): loadflac(aFile); break;
        default: loadmp3(aFile); break;
    }
}

AudioSourceInstance* WavStream::createInstance()
{
    return new WavStreamInstance(this);
}

double WavStream::getLength() const
{
    if (mBaseSamplerate == 0)
        return 0;
    return mSampleCount / mBaseSamplerate;
}
}; // namespace SoLoud
