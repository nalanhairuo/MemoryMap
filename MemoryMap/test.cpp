
#include <stdio.h>
#include <Windows.h>
#include <stdlib.h>

#include "MemoryMapWriter.h"
#include "MemoryMapReader.h"

DWORD WriteTest(LPCTSTR path)
{
    CMemoryMapWriter writer;

    if (false == writer.Open(path, false))
    {
        printf("open fail \n");
        return 1;
    }


    for (int i = 0; i < 64 * 1024; ++i)
    {
        writer.WriteData(&i, sizeof(int));
        // writer.WriteData("\n", 1);
    }

    writer.Close();
}


DWORD ReadTest(LPCTSTR path)
{
    CMemoryMapReader reader;
    reader.Open(path);
    //printf("%ld\n", reader.GetFileSize());

    int len = reader.GetFileSize() / 65536;
    int data;

    for (int i = 0; i < 65536; i++)
    {


        reader.ReadData(&data, sizeof(int));

        printf("%d\n", data);
    }


    reader.Close();
    return 0;
}

int main(int argc, _TCHAR* argv[])
{

    //WriteTest(_T("test.dat"));

    ReadTest(_T("test.dat"));


    return 0;
}

