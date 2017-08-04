#include "MemoryMapReader.h"
#include <windows.h>

#define memfile_block_size 64*1024

//------------------------------------------------------------------------------------
class CMemoryMapReader::Impl
{

        HANDLE                  handle_file_;        //打开句柄
        HANDLE                  handle_map_;         //映射句柄

        unsigned int            file_size_;          //文件大小
        unsigned int            cur_block_pos_;      //当前映射块的第一个字节在文件中的偏移
        char*                   cur_block_ptr_;      //当前映射块指针
        unsigned int            cur_block_size_;     //当前映射块的大小
        unsigned int            cur_block_offset_;   //当前映射块读写位置

public:
        Impl();
        ~Impl();

public:
        bool  Open(const TCHAR* path);
        void  Close();

        bool  IsEOF();
        unsigned int GetFileSize();

        unsigned int GetReaderOffset();
        bool  SetReaderOffeset(const unsigned int& offset);

        unsigned int ReadData(void* data_buf, const unsigned int& read_len);

private:
        bool i_OpenFile(const TCHAR* file);
        bool i_CreateFileMap();
        bool i_MapNextBlock();
        void i_CloseFileMap();
        void i_CloseFile();
};

//------------------------------------------------------------------------------------
CMemoryMapReader::Impl::Impl()
        : handle_file_()
        , handle_map_()
        , file_size_()
        , cur_block_pos_()
        , cur_block_ptr_()
        , cur_block_size_()
        , cur_block_offset_()
{
}

CMemoryMapReader::Impl::~Impl()
{
        Close();
}

void CMemoryMapReader::Impl::i_CloseFileMap()
{
        if (cur_block_ptr_)
        {
                ::UnmapViewOfFile(cur_block_ptr_);
        }

        if (handle_map_ && (handle_map_ != INVALID_HANDLE_VALUE))
        {
                ::CloseHandle(handle_map_);
        }

        cur_block_ptr_ = 0;
        handle_map_ = 0;
}

void CMemoryMapReader::Impl::i_CloseFile()
{
        if (handle_file_ && (handle_file_ != INVALID_HANDLE_VALUE))
        {
                ::CloseHandle(handle_file_);
        }

        handle_file_ = 0;
        file_size_ = 0;
        cur_block_ptr_ = 0;
        cur_block_pos_ = 0;

        cur_block_size_ = 0;
        cur_block_offset_ = 0;
}

bool CMemoryMapReader::Impl::i_OpenFile(const TCHAR* file)
{
        Close();

        handle_file_ = ::CreateFile(file,
                                    GENERIC_READ | GENERIC_WRITE,
                                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                                    0,
                                    OPEN_EXISTING,
                                    FILE_ATTRIBUTE_NORMAL,
                                    0);

        if (handle_file_ == INVALID_HANDLE_VALUE)
        {
                return false;
        }

        file_size_ = ::GetFileSize(handle_file_, 0); // max file size : 4G

        return i_CreateFileMap();
}

bool CMemoryMapReader::Impl::i_CreateFileMap()
{
        do
        {
                if (file_size_ == 0xFFFFFFFF)
                {
                        break;
                }

                handle_map_ = ::CreateFileMapping(handle_file_,
                                                  0,
                                                  PAGE_READWRITE,
                                                  0,
                                                  file_size_,
                                                  0);

                if (handle_map_ == INVALID_HANDLE_VALUE)
                {
                        break;
                }

                cur_block_pos_ = 0;
                cur_block_size_ = memfile_block_size;

                if ((file_size_ - cur_block_pos_) < memfile_block_size)
                {
                        cur_block_size_ = file_size_ - cur_block_pos_; // rest data of current block is less than 64K
                }

                cur_block_ptr_ = (char*)::MapViewOfFile(handle_map_,
                                                        FILE_MAP_READ | FILE_MAP_WRITE,
                                                        0,
                                                        cur_block_pos_,
                                                        cur_block_size_);
                cur_block_offset_ = 0;

                return true;
        }
        while (0);

        Close();

        return false;
}

bool CMemoryMapReader::Impl::i_MapNextBlock()
{
        if (cur_block_ptr_)
        {
                ::UnmapViewOfFile(cur_block_ptr_);
        }

        cur_block_ptr_ = 0;

        cur_block_pos_ += cur_block_size_;

        if (file_size_ <= cur_block_pos_)
        {
                return false;//end of file
        }

        cur_block_size_ = memfile_block_size;

        if (file_size_ - cur_block_pos_ < memfile_block_size)
        {
                cur_block_size_ = file_size_ - cur_block_pos_;
        }

        cur_block_ptr_ = (char*)::MapViewOfFile(handle_map_,
                                                FILE_MAP_READ | FILE_MAP_WRITE,
                                                0,
                                                cur_block_pos_,
                                                cur_block_size_);
        cur_block_offset_ = 0;

        return (cur_block_ptr_ != NULL);
}

bool CMemoryMapReader::Impl::Open(const TCHAR* path)
{
        if (!path)
        {
                return false;
        }

        bool flag = i_OpenFile(path);

        if (false == flag)
        {
                return false;
        }

        return SetReaderOffeset(0);
}

void CMemoryMapReader::Impl::Close()
{
        i_CloseFileMap();
        i_CloseFile();
}

unsigned int CMemoryMapReader::Impl::GetFileSize()
{
        return file_size_;
}

unsigned int CMemoryMapReader::Impl::GetReaderOffset()
{
        return (cur_block_pos_ + cur_block_offset_);
}

bool CMemoryMapReader::Impl::IsEOF()
{
        return (GetReaderOffset() >= file_size_);
}

bool CMemoryMapReader::Impl::SetReaderOffeset(const unsigned int& offset)
{
        if (!handle_map_ || handle_map_ == INVALID_HANDLE_VALUE)
        {
                return false;
        }

        if (file_size_ < offset)
        {
                return false;
        }

        unsigned int offTemp = offset;

        if (cur_block_ptr_)
        {
                ::UnmapViewOfFile(cur_block_ptr_);
        }

        cur_block_ptr_ = 0;

        unsigned int real_set = offTemp;

        if (offTemp % memfile_block_size)
        {
                offTemp = memfile_block_size * (offTemp / memfile_block_size);
        }

        real_set -= offTemp; // real_set == offset of one block

        cur_block_pos_ = offTemp;
        cur_block_size_ = memfile_block_size;

        if (file_size_ - cur_block_pos_ < memfile_block_size)
        {
                cur_block_size_ = file_size_ - cur_block_pos_; // rest data of current block is less than 64K
        }

        cur_block_ptr_ = (char*)::MapViewOfFile(handle_map_,
                                                FILE_MAP_READ | FILE_MAP_WRITE,
                                                0,
                                                cur_block_pos_,
                                                cur_block_size_);
        cur_block_offset_ = real_set;

        return (cur_block_ptr_ != NULL);
}

unsigned int CMemoryMapReader::Impl::ReadData(void* data_buf, const unsigned int& read_len)
{
        if (!cur_block_ptr_ || !data_buf)
        {
                return 0;
        }

        char* dst_buf = (char*)data_buf;
        unsigned int  len = read_len;
        unsigned int  rest_data;        // rest len of current block

        while (len > 0)
        {
                rest_data = cur_block_size_ - cur_block_offset_;

                if (rest_data > len)
                {
                        rest_data = len;
                }

                memcpy(dst_buf, cur_block_ptr_ + cur_block_offset_, rest_data);
                cur_block_offset_ += rest_data;

                dst_buf += rest_data;
                len -= rest_data;

                if (cur_block_size_ == cur_block_offset_)
                {
                        if (false == i_MapNextBlock())
                        {
                                break;
                        }
                }
        }

        return read_len - len;
}

//===================================================================================================
CMemoryMapReader::CMemoryMapReader()
{
        self = new Impl();
}

CMemoryMapReader::~CMemoryMapReader()
{
        delete self;
}

bool  CMemoryMapReader::Open(const TCHAR* path)
{
        return self->Open(path);
}

void  CMemoryMapReader::Close()
{
        self->Close();
}

unsigned int CMemoryMapReader::GetFileSize()
{
        return self->GetFileSize();
}

unsigned int CMemoryMapReader::GetReaderOffset()
{
        return self->GetReaderOffset();
}

bool  CMemoryMapReader::SetReaderOffeset(const unsigned int& offset)
{
        return self->SetReaderOffeset(offset);
}

bool  CMemoryMapReader::IsEOF()
{
        return self->IsEOF();
}

unsigned int CMemoryMapReader::ReadData(void* data_buf, const unsigned int& read_len)
{
        return self->ReadData(data_buf, read_len);
}
