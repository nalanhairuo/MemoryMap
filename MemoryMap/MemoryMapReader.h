#pragma once
#include <tchar.h>

//======================================================================================
class CMemoryMapReader
{
public:
    CMemoryMapReader();
    ~CMemoryMapReader();

public:
    bool  Open(const TCHAR* path);
    void  Close();

    bool  IsEOF();
    unsigned int GetFileSize();

    bool  SetReaderOffeset(const unsigned int& offset);
    unsigned int GetReaderOffset();

    unsigned int ReadData(void* data_buf, const unsigned int& read_len); // return : the len of returned data

private:
    class Impl;
    Impl* self;
};
