#ifndef MYAPI_H_INCLUDED
#define MYAPI_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <sstream>
#include <iostream>

//#define OLDVERSION
using namespace std;
typedef unsigned char u8;   //1字节
typedef unsigned short u16; //2字节
typedef unsigned int u32;   //4字节
extern int  BytsPerSec;    //每扇区字节数
extern int  SecPerClus;    //每簇扇区数
extern int  RsvdSecCnt;    //Boot记录占用的扇区数
extern int  NumFATs;   //FAT表个数
extern int  RootEntCnt;    //根目录最大文件数
extern int  FATSz;
#pragma pack (1) /*指定按1字节对齐*/
struct BPB {
    u16  BPB_BytsPerSec;    //每扇区字节数
    u8   BPB_SecPerClus;    //每簇扇区数
    u16  BPB_RsvdSecCnt;    //Boot记录占用的扇区数
    u8   BPB_NumFATs=2;   //FAT表个数
    u16  BPB_RootEntCnt;    //根目录最大文件数
    u16  BPB_TotSec16;
    u8   BPB_Media;
    u16  BPB_FATSz16=9;   //FAT扇区数
    u16  BPB_SecPerTrk;
    u16  BPB_NumHeads;
    u32  BPB_HiddSec;
    u32  BPB_TotSec32;  //如果BPB_FATSz16为0，该值为FAT扇区数
};
struct RootEntry {
    char DIR_Name[11]; //11byte
    u8   DIR_Attr;      //文件属性 1byte
    char reserved[10]; //10byte
    u16  DIR_WrtTime;//写入、修改时间
    u16  DIR_WrtDate;//
    u16  DIR_FstClus;   //开始簇号
    u32  DIR_FileSize;
};
/**
 要求：在指定目录下创建指定文件，如果该文件不存在的话。如果目录不存在或文件已存在，则函数失败返回0；
如果成功则返回一个表示该文件的标识（类似于Window的句柄，内部数据结构及映射方法你来定）
pszFolderPath：目录路径，如"C:\\Test\\Test01"等等
pszFileName：文件名，如"Test.txt"等等
 */
DWORD MyCreateFile(char *pszFolderPath, char *pszFileName);
/** \brief
要求：打开指定目录下的指定文件，如果目录不存在或者文件不存在，则返回0表示失败；
如果成功则返回一个表示该文件的标识（类似于Window的句柄，内部数据结构及映射方法你来定）
pszFolderPath：目录路径，如"C:\\Test\\Test01"等等
pszFileName：文件名，如"Test.txt"等等
 */
DWORD MyOpenFile(char *pszFolderPath, char *pszFileName);
/** \brief
 *
要求：关闭该文件；
传入参数：
dwHandle：传入标识该文件的句柄，就是MyCreateFile返回的那个
 *
 */
void MyCloseFile(DWORD dwHandle);
/** \brief
 *
 要求：删除指定目录下的指定文件，如果目录不存在或者文件不存在，则返回0表示失败，否则返回TRUE表示成功；
传入参数：
pszFolderPath：目录路径，如"C:\\Test\\Test01"等等
pszFileName：文件名，如"Test.txt"等等
 *
 */
void MyDeleteFile(char *pszFolderPath, char *pszFileName);
/** \brief
 *
要求：将pBuffer中dwBytesToWrite长度的数据写入指定文件的文件指针位置。
传入参数：
dwHandle：MyOpenFile返回的值，在这个函数中又原封不动的传给你，其内部数据结构由你来定。
pBuffer：指向待写入数据的缓冲区
dwBytesToWrite：待写入数据的长度
返回值：成功写入的长度，-1表示失败。
 *
 */
DWORD MyWriteFile(DWORD dwHandle, LPVOID pBuffer, DWORD dwBytesToWrite);

/** \brief
 *
 要求：读取指定文件中、指定长度的数据到传入的缓冲区。
传入参数：
dwHandle：MyOpenFile返回的值，在这个函数中又原封不动的传给你，其内部数据结构由你来定。
pBuffer：指向接收数据的缓冲区
dwBytesToRead：待读取数据的长度
返回值：成功读取的长度，-1表示失败。
 *
 */
DWORD MyReadFile(DWORD dwHandle, LPVOID pBuffer, DWORD dwBytesToRead);
/** \brief
 *
要求：在指定路径下，创建指定名称的文件夹。如果目录不存在或待创建的文件夹已存在，则返回FALSE。
创建成功返回TRUE；
传入参数：
pszFolderPath：目录路径，如"C:\\Test\\Test01"等等
pszFolderName：文件夹名称，如"MyFolder"等等
返回值：如果目录不存在或待创建的文件夹已存在，则返回FALSE。创建成功返回TRUE；
 *
 */

BOOL MyCreateDirectory(char *pszFolderPath, char *pszFolderName);
/** \brief
 *
 要求：在指定路径下，删除指定名称的文件夹。如果目录不存在或待创建的文件夹不存在，则返回FALSE。
删除成功返回TRUE；
传入参数：
pszFolderPath：目录路径，如"C:\\Test\\Test01"等等
pszFolderName：文件夹名称，如"MyFolder"等等
返回值：如果目录不存在或待创建的文件夹不存在，则返回FALSE。删除成功返回TRUE；
 *
 */

BOOL MyDeleteDirectory(char *pszFolderPath, char *pszFolderName);
/** \brief
 *
用途：移动指定文件的文件头，读写不同位置。如果文件句柄不存在，返回FALSE，否则返回TRUE
dwHandle：MyOpenFile返回的值，在这个函数中又原封不动的传给你，其内部数据结构由你来定。
nOffset：32位偏移量，可正可负可为零。
dwMoveMethod：偏移的起始位置，有如下三种类型可选：
MY_FILE_BEGIN：从头部开始计算偏移
MY_FILE_CURRENT：从当前磁头位置开始计算便宜
MY_FILE_END：从末尾开始计算偏移
 *
 */

BOOL MySetFilePointer(DWORD dwFileHandle, int nOffset, DWORD dwMoveMethod);
#endif // MYAPI_H_INCLUDED
