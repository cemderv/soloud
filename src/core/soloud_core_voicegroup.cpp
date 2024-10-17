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

#include "soloud_engine.hpp"

// Voice group operations

namespace SoLoud
{
// Create a voice group. Returns 0 if unable (out of voice groups / out of memory)
handle Engine::createVoiceGroup()
{
    lockAudioMutex_internal();

    unsigned int i;
    // Check if there's any deleted voice groups and re-use if found
    for (i = 0; i < mVoiceGroupCount; i++)
    {
        if (mVoiceGroup[i] == nullptr)
        {
            mVoiceGroup[i] = new unsigned int[17];
            if (mVoiceGroup[i] == nullptr)
            {
                unlockAudioMutex_internal();
                return 0;
            }
            mVoiceGroup[i][0] = 16;
            mVoiceGroup[i][1] = 0;
            unlockAudioMutex_internal();
            return 0xfffff000 | i;
        }
    }
    if (mVoiceGroupCount == 4096)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    unsigned int oldcount = mVoiceGroupCount;
    if (mVoiceGroupCount == 0)
    {
        mVoiceGroupCount = 4;
    }
    mVoiceGroupCount *= 2;
    unsigned int** vg = new unsigned int*[mVoiceGroupCount];
    if (vg == nullptr)
    {
        mVoiceGroupCount = oldcount;
        unlockAudioMutex_internal();
        return 0;
    }
    for (i = 0; i < oldcount; i++)
    {
        vg[i] = mVoiceGroup[i];
    }

    for (; i < mVoiceGroupCount; i++)
    {
        vg[i] = nullptr;
    }

    delete[] mVoiceGroup;
    mVoiceGroup    = vg;
    i              = oldcount;
    mVoiceGroup[i] = new unsigned int[17];
    if (mVoiceGroup[i] == nullptr)
    {
        unlockAudioMutex_internal();
        return 0;
    }
    mVoiceGroup[i][0] = 16;
    mVoiceGroup[i][1] = 0;
    unlockAudioMutex_internal();
    return 0xfffff000 | i;
}

// Destroy a voice group.
void Engine::destroyVoiceGroup(handle aVoiceGroupHandle)
{
    if (!isVoiceGroup(aVoiceGroupHandle))
        return;

    const int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();
    delete[] mVoiceGroup[c];
    mVoiceGroup[c] = nullptr;
    unlockAudioMutex_internal();
}

// Add a voice handle to a voice group
void Engine::addVoiceToGroup(handle aVoiceGroupHandle, handle aVoiceHandle)
{
    if (!isVoiceGroup(aVoiceGroupHandle))
        return;

    // Don't consider adding invalid voice handles as an error, since the voice may just have ended.
    if (!isValidVoiceHandle(aVoiceHandle))
        return;

    trimVoiceGroup_internal(aVoiceGroupHandle);

    int          c = aVoiceGroupHandle & 0xfff;
    unsigned int i;

    lockAudioMutex_internal();

    for (i = 1; i < mVoiceGroup[c][0]; i++)
    {
        if (mVoiceGroup[c][i] == aVoiceHandle)
        {
            unlockAudioMutex_internal();
            return; // already there
        }

        if (mVoiceGroup[c][i] == 0)
        {
            mVoiceGroup[c][i]     = aVoiceHandle;
            mVoiceGroup[c][i + 1] = 0;

            unlockAudioMutex_internal();
            return;
        }
    }

    // Full group, allocate more memory
    const auto n = new unsigned int[mVoiceGroup[c][0] * 2 + 1];

    for (i = 0; i < mVoiceGroup[c][0]; i++)
        n[i] = mVoiceGroup[c][i];

    n[n[0]]     = aVoiceHandle;
    n[n[0] + 1] = 0;
    n[0] *= 2;
    delete[] mVoiceGroup[c];
    mVoiceGroup[c] = n;
    unlockAudioMutex_internal();
}

// Is this handle a valid voice group?
bool Engine::isVoiceGroup(handle aVoiceGroupHandle)
{
    if ((aVoiceGroupHandle & 0xfffff000) != 0xfffff000)
        return 0;
    unsigned int c = aVoiceGroupHandle & 0xfff;
    if (c >= mVoiceGroupCount)
        return 0;

    lockAudioMutex_internal();
    bool res = mVoiceGroup[c] != nullptr;
    unlockAudioMutex_internal();

    return res;
}

// Is this voice group empty?
bool Engine::isVoiceGroupEmpty(handle aVoiceGroupHandle)
{
    // If not a voice group, yeah, we're empty alright..
    if (!isVoiceGroup(aVoiceGroupHandle))
        return 1;
    trimVoiceGroup_internal(aVoiceGroupHandle);
    int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();
    bool res = mVoiceGroup[c][1] == 0;
    unlockAudioMutex_internal();

    return res;
}

// Remove all non-active voices from group
void Engine::trimVoiceGroup_internal(handle aVoiceGroupHandle)
{
    if (!isVoiceGroup(aVoiceGroupHandle))
        return;
    int c = aVoiceGroupHandle & 0xfff;

    lockAudioMutex_internal();
    // empty group
    if (mVoiceGroup[c][1] == 0)
    {
        unlockAudioMutex_internal();
        return;
    }

    unsigned int i;
    // first item in voice group is number of allocated indices
    for (i = 1; i < mVoiceGroup[c][0]; i++)
    {
        // If we hit a voice in the group that's not set, we're done
        if (mVoiceGroup[c][i] == 0)
        {
            unlockAudioMutex_internal();
            return;
        }

        unlockAudioMutex_internal();
        while (!isValidVoiceHandle(
            mVoiceGroup[c][i])) // function locks mutex, so we need to unlock it before the call
        {
            lockAudioMutex_internal();
            // current index is an invalid handle, move all following handles backwards
            unsigned int j;
            for (j = i; j < mVoiceGroup[c][0] - 1; j++)
            {
                mVoiceGroup[c][j] = mVoiceGroup[c][j + 1];
                // not a full group, we can stop copying
                if (mVoiceGroup[c][j] == 0)
                    break;
            }
            // be sure to mark the last one as unused in any case
            mVoiceGroup[c][mVoiceGroup[c][0] - 1] = 0;
            // did we end up with an empty group? we're done then
            if (mVoiceGroup[c][i] == 0)
            {
                unlockAudioMutex_internal();
                return;
            }
            unlockAudioMutex_internal();
        }
        lockAudioMutex_internal();
    }
    unlockAudioMutex_internal();
}

handle* Engine::voiceGroupHandleToArray_internal(handle aVoiceGroupHandle) const
{
    if ((aVoiceGroupHandle & 0xfffff000) != 0xfffff000)
        return nullptr;
    unsigned int c = aVoiceGroupHandle & 0xfff;
    if (c >= mVoiceGroupCount)
        return nullptr;
    if (mVoiceGroup[c] == nullptr)
        return nullptr;
    return mVoiceGroup[c] + 1;
}

} // namespace SoLoud
