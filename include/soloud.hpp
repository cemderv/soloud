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

#ifdef WITH_SDL
#undef WITH_SDL2
#undef WITH_SDL1
#define WITH_SDL1
#define WITH_SDL2
#endif

#ifdef WITH_SDL_STATIC
#undef WITH_SDL1_STATIC
#define WITH_SDL1_STATIC
#endif

#ifndef M_PI
#define M_PI 3.14159265359
#endif

#if defined(_WIN32) || defined(_WIN64)
#define WINDOWS_VERSION
#endif

#if !defined(DISABLE_SIMD)
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386) || defined(_M_IX86)
#define SOLOUD_SSE_INTRINSICS
#endif
#endif

/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////
// Configuration defines

// Maximum number of filters per stream
static constexpr size_t FILTERS_PER_STREAM = 8;

// Number of samples to process on one go
static constexpr size_t SAMPLE_GRANULARITY = 512;

// Maximum number of concurrent voices (hard limit is 4095)
static constexpr size_t VOICE_COUNT = 1024;

// 1)mono, 2)stereo 4)quad 6)5.1 8)7.1
static constexpr size_t MAX_CHANNELS = 8;

//
/////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////

// Typedefs have to be made before the includes, as the
// includes depend on them.
namespace SoLoud
{
class Soloud;
typedef void (*      mutexCallFunction)(void* aMutexPtr);
typedef void (*      soloudCallFunction)(Soloud* aSoloud);
typedef bool (*      soloudResultFunction)(Soloud* aSoloud);
typedef unsigned int handle;
typedef double       time_t;

enum class FLAGS
{
    // Use round-off clipper
    None = 0,
    CLIP_ROUNDOFF = 1,
    ENABLE_VISUALIZATION = 2,
    NO_FPU_REGISTER_CHANGE = 4
};

static inline FLAGS operator&(FLAGS lhs,
                              FLAGS rhs)
{
    return FLAGS(int(lhs) & int(rhs));
}

static inline FLAGS operator|(FLAGS lhs,
                              FLAGS rhs)
{
    return FLAGS(int(lhs) | int(rhs));
}

static inline FLAGS operator~(FLAGS value)
{
    return FLAGS(~int(value));
}

static inline FLAGS& operator&=(FLAGS& lhs,
                                FLAGS  rhs)
{
    lhs = lhs & rhs;
    return lhs;
}

static inline FLAGS& operator|=(FLAGS& lhs,
                                FLAGS  rhs)
{
    lhs = lhs | rhs;
    return lhs;
}

static inline bool testFlag(FLAGS value, FLAGS toTest)
{
    return (int(value) & int(toTest)) == int(toTest);
}

enum class WAVEFORM
{
    SQUARE = 0,
    SAW,
    SIN,
    TRIANGLE,
    BOUNCE,
    JAWS,
    HUMPS,
    FSQUARE,
    FSAW
};

enum class RESAMPLER
{
    POINT,
    LINEAR,
    CATMULLROM
};

// Default resampler for both main and bus mixers
static constexpr auto SOLOUD_DEFAULT_RESAMPLER = RESAMPLER::LINEAR;
}; // namespace SoLoud
