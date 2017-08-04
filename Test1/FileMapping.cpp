
/* ************************************
*《精通Windows API》
* 示例代码
* file_map.c
* 4.4.1  使用Mapping File提高文件读写的效率
**************************************/

/* 头文件　*/
#include <windows.h>
#include <stdio.h>
#include <tchar.h>

/* 预处理申明　*/
#define BUFFSIZE 1024                  // 内存大小
#define FILE_MAP_START 0x28804         // 文件映射的起始的位置

/* 全局变量　*/
LPTSTR lpcTheFile = _T("test.dat");    // 文件名

/* ************************************
* int main(void)
* 功能        演示使用文件mapping
*
* 参数        无
*
* 返回值     0代表执行完成，1代表发生错误
**************************************/
int main(void)
{
        HANDLE hMapFile;      // 文件内存映射区域的句柄
        HANDLE hFile;         // 文件的句柄
        DWORD dBytesWritten;  // 写入的字节数

        DWORD access_mode = (GENERIC_READ | GENERIC_WRITE);//存取模式
        DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;//共享模式
        DWORD flags = FILE_ATTRIBUTE_NORMAL;//文件属性
        //FILE_FLAG_SEQUENTIAL_SCAN|FILE_FLAG_WRITE_THROUGH|FILE_FLAG_NO_BUFFERING;

        DWORD dwFileSize;     // 文件大小
        DWORD dwFileMapSize;  // 文件映射的大小
        DWORD dwMapViewSize;  // 视图（View）的大小
        DWORD dwFileMapStart; // 文件映射视图的起始位置
        DWORD dwSysGran;      // 系统内存分配的粒度

        SYSTEM_INFO SysInfo;  // 系统信息
        LPVOID lpMapAddress;  // 内存映射区域的起始位置

        PCHAR pData;          // 数据
        INT i;                // 循环变量
        INT iData;
        INT iViewDelta;
        BYTE cMapBuffer[32];  // 存储从mapping中计出的数据

        // 创建一个文件
        hFile = CreateFile(lpcTheFile,
                           access_mode,
                           share_mode/*0*/,
                           NULL,
                           CREATE_ALWAYS,
                           flags,
                           NULL);

        //判断文件是否创建成功
        if (hFile == INVALID_HANDLE_VALUE)
        {
                printf("CreateFile error %d \n", GetLastError());
                return 1;
        }

        // 依次写入整数，一共写入65535个整数
        // 在32位平台下，大小为65535*32
        for (i = 0; i < 65535; i++)
        {
                WriteFile(hFile, &i, sizeof(i), &dBytesWritten, NULL);
        }

        // 查看写入完成后的文件大小
        // DWORD high_size;
        // dwFileSize = GetFileSize(hFile, &high_size);
        dwFileSize = GetFileSize(hFile,  NULL);
        printf("文件大小: %d\n", dwFileSize);

        //获取系统信息，内存分配粒度
        //获取分配粒度，进行下面的几个计算，
        //目的是为了映射的数据与系统内存分配粒度对齐，提高内存访问效率
        GetSystemInfo(&SysInfo);
        dwSysGran = SysInfo.dwAllocationGranularity;
        printf("%d\n", dwSysGran);
        //计算mapping的起始位置
        dwFileMapStart = (FILE_MAP_START / dwSysGran) * dwSysGran;
        // 计算mapping view的大小
        dwMapViewSize = (FILE_MAP_START % dwSysGran) + BUFFSIZE;
        // 计算mapping的大小
        dwFileMapSize = FILE_MAP_START + BUFFSIZE;
        // 计算需要读取的数据的偏移
        iViewDelta = FILE_MAP_START - dwFileMapStart;

        DWORD size_high = 0;
        // 创建File mapping
        hMapFile = CreateFileMapping(hFile,               // 需要映射的文件的句柄
                                     NULL,                // 安全选项：默认
                                     PAGE_READWRITE,      // 可读，可写
                                     size_high,           // mapping对象的大小，高位
                                     dwFileMapSize,       // mapping对象的大小，低位,共64位，所以支持的最大文件长度为16EB
                                     NULL/*shared_name*/);// mapping对象的名字,用于与其它进程共享文件映射对象

        if (hMapFile == NULL)
        {
                printf("CreateFileMapping error: %d\n", GetLastError());
                return 1;
        }

        // 映射view,把文件数据映射到进程的地址空间
        lpMapAddress = MapViewOfFile(hMapFile,            // mapping对象的句柄
                                     FILE_MAP_ALL_ACCESS, // 可读，可写
                                     0,                   // 映射的文件偏移，高32位
                                     dwFileMapStart,      // 映射的文件偏移，低32位
                                     dwMapViewSize);      // 映射到View的数据大小

        if (lpMapAddress == NULL)
        {
                printf("MapViewOfFile error: %d\n", GetLastError());
                return 1;
        }

        //unsigned char *p = (unsigned char*)lpMapAddress;
        //至此，就获得了外部文件test.dat在内存地址空间的映射，
        //下面就可以用指针p"折磨"这个文件了
        //CString s;
        //p[size_low - 1] = '!';
        //p[size_low - 2] = 'X'; //修改该文件的最后两个字节(文件大小<4GB高32位为0)
        //s.Format("%s", p);
        //读文件的最后3个字节

        printf("文件map view相对于文件的起始位置： 0x%x\n",
               dwFileMapStart);
        printf("文件map view的大小：0x%x\n",     dwMapViewSize);
        printf("文件mapping对象的大小：0x%x\n", dwFileMapSize);
        printf("从相对于map view 0x%x 字节的位置读取数据，", iViewDelta);

        //char write_chars[] = "hello chars";
        //const size_t write_chars_size = sizeof(write_chars);

        //向内存映射视图中写数据
        //CopyMemory((PVOID)lpMapAddress, write_chars, write_chars_size);
        //memcpy(lpMapAddress,write_chars,write_chars_size);

        //size_t position = 0;
        //char read_chars[write_chars_size];

        //读数据
        //memcpy(read_chars, lpMapAddress, write_chars_size);
        //cout << "read chars " << read_chars << endl;

        //将指向数据的指针偏移，到达我们关心的地方
        pData = (PCHAR) lpMapAddress + iViewDelta;
        //读取数据，赋值给变量
        iData = *(PINT)pData;
        //显示读取的数据
        printf("为：0x%.8x\n", iData);

        //从mapping中复制数据，32个字节，并打印
        CopyMemory(cMapBuffer, lpMapAddress, 32);
        //memcpy(cMapBuffer,lpMapAddress,32);
        printf("lpMapAddress起始的32字节是：");

        for (i = 0; i < 32; i++)
        {
                printf("0x%.2x ", cMapBuffer[i]);
        }

        // 将mapping的前32个字节用0xff填充
        FillMemory(lpMapAddress, 32, (BYTE)0xff);
        // 将映射的数据写回到硬盘上
        FlushViewOfFile(lpMapAddress, dwMapViewSize);
        printf("\n已经将lpMapAddress开始的32字节使用0xff填充。\n");

        //UnmapViewOfFile(pvFile); //撤销映射
        //关闭mapping对象
        if (!CloseHandle(hMapFile))
        {
                printf("\nclosing the mapping object error %d!",
                       GetLastError());
        }

        //关闭文件
        if (!CloseHandle(hFile))
        {
                printf("\nError %ld occurred closing the file!",
                       GetLastError());
        }

        return 0;
}
