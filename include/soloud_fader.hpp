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

#include "soloud.hpp"

namespace SoLoud
{
// Helper class to process faders
class Fader
{
  public:
    // Ctor
    Fader();

    // Set up LFO
    void setLFO(float aFrom, float aTo, time_t aTime, time_t aStartTime);

    // Set up fader
    void set(float aFrom, float aTo, time_t aTime, time_t aStartTime);

    // Get the current fading value
    float get(time_t aCurrentTime);

    // Value to fade from
    float mFrom;

    // Value to fade to
    float mTo;

    // Delta between from and to
    float mDelta;

    // Total time to fade
    time_t mTime;

    // Time fading started
    time_t mStartTime;

    // Time fading will end
    time_t mEndTime;

    // Current value. Used in case time rolls over.
    float mCurrent;

    // Active flag; 0 means disabled, 1 is active, 2 is LFO, -1 means was active, but stopped
    int mActive;
};
}; // namespace SoLoud
