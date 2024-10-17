#pragma once

class darray
{
protected:
	char *mData;
	int mUsed;
	int mAllocated;
	int mAllocChunk;
public:
	darray();
  darray(const darray& other)=delete;
  darray& operator=(const darray& other)=delete;
  darray(darray&& other)noexcept=delete;
  darray& operator=(darray&& other) noexcept=delete;
	~darray();
	void clear();
	char *getDataInPos(int aPosition);
	void put(int aData);
	int getSize() const { return mUsed; }
	char *getData() { return mData; } 
};



