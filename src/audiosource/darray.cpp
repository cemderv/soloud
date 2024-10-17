#include "darray.h"
#include <cstdlib>
#include <cstring>

darray::darray()
{
    mAllocChunk = 128;
    mAllocated = mUsed = 0;
    mData              = nullptr;
}

void darray::clear()
{
    free(mData);
    mAllocChunk = 128;
    mAllocated = mUsed = 0;
    mData              = nullptr;
}

darray::~darray()
{
    clear();
}

char* darray::getDataInPos(int aPosition)
{
    if (aPosition < mAllocated && aPosition < mUsed)
        return mData + aPosition;

    if (aPosition >= mAllocated)
    {
        int newsize = mAllocated;

        while (newsize <= aPosition)
        {
            newsize += mAllocChunk;
            mAllocChunk *= 2;
        }

        char* newdata = (char*)realloc(mData, newsize);
        if (!newdata)
        {
            free(mData);
            mData      = nullptr;
            mAllocated = mUsed = 0;
            return nullptr;
        }
        else
        {
            memset(newdata + mAllocated, 0, newsize - mAllocated);
        }

        mData      = newdata;
        mAllocated = newsize;
    }

    if (aPosition >= mUsed)
    {
        mUsed = aPosition + 1;
    }

    return mData + aPosition;
}

void darray::put(int aData)
{
    char* s = getDataInPos(mUsed);

    *s = (char)aData;
}
