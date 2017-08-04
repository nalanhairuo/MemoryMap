#pragma once
#include <tchar.h>

//======================================================================================
class CMemoryMapWriter
{
public:
    CMemoryMapWriter();
    ~CMemoryMapWriter();

public:
    bool Open(const TCHAR* path, bool cover_write);
    void Close();

    unsigned int WriteData(void* ptr, const unsigned int& len);

private:
    class impl;
    impl* self;
};
