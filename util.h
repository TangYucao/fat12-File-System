#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <windows.h>
#include <iostream>
#include <iomanip>
#include <vector>
#include "DiskLib.h"
#define MAX_NUM 1024
using namespace std;
/** \brief
文件句柄容器
*/
vector<RootEntry> dwHandles;
extern struct RootEntry* rootEntry_ptr;
//extern RootEntry* dwHandles[MAX_NUM];
/** \brief
打印一个Fat12的文件基本信息
*/
void printRootEntryStruct(RootEntry* rootEntry_ptr);
/** \brief
打印所有Fat12的文件基本信息
*/
void printAllRootEntryStruct();
/** \brief
打印整个文件系统的基本信息
*/
void printBPB();
/** \brief
打印所有的文件名和其根目录，来自网络
*/
void printFilesNew(struct RootEntry* rootEntry_ptr);
/** \brief
打印所有的子目录的文件名，来自网络
*/
void printChildrenNew(char * directory, int startClus);
/** \brief
来自网络
*/
int  getFATValueNew(int num);
template<typename T>
/** \brief
已弃用，获取C数组长度
*/
int getHandleLength(T &x);
/** \brief
已弃用，创建句柄
*/
DWORD createHandle(RootEntry* FileInfo);
/** \brief
把文件句柄填满
*/
void fillHandles();
/** \brief
文件是否存在？还没写
*/
BOOL isFileExist(char *pszFileName, u16 FstClus);

void printRootEntryStruct(RootEntry* rootEntry_ptr)
{
	cout <<setw(22) << "[debug]DIR_Name:" << setw(14) << rootEntry_ptr->DIR_Name << endl;
	//	cout << setw(22) << "[debug]DIR_Attr:"  <<hex << rootEntry_ptr->DIR_Attr << endl;
	printf("      [debug]DIR_Attr:%x\n", rootEntry_ptr->DIR_Attr);
	//	cout << setw(22) << "[debug]reserved:" <<  setw(14) << rootEntry_ptr->reserved << endl;
	cout << setw(22) << "[debug]DIR_WrtTime:" << setw(14) << rootEntry_ptr->DIR_WrtTime << endl;
	cout << setw(22) << "[debug]DIR_WrtDate:" << setw(14) << rootEntry_ptr->DIR_WrtDate << endl;
	cout << setw(22) << "[debug]DIR_FstClus:" << setw(14) << rootEntry_ptr->DIR_FstClus << endl;
	cout << setw(22) << "[debug]DIR_FileSize:" << setw(14) << rootEntry_ptr->DIR_FileSize << endl;
	cout << setw(22) << "------------end-------------------" << endl;
}
void printAllRootEntryStruct()
{
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
	RootEntry *FileInfo_ptr = (RootEntry*)malloc(sizeof(RootEntry));
	for (int i = 0; i < RootEntCnt; i++)
	{


		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(FileInfo_ptr, 32, NULL);
		if (FileInfo_ptr->DIR_Name[0] == '\0')
		{
			base += 32;
			continue;

		}
		printRootEntryStruct(FileInfo_ptr);
		base += 32;

	}
}
void printBPB()
{
	cout << setw(22) << "[debug]BytsPerSec:" << setw(14) << BytsPerSec << endl;
	cout << setw(22) << "[debug]SecPerClus:" << setw(14) << SecPerClus << endl;
	cout << setw(22) << "[debug]RsvdSecCnt:" << setw(14) << RsvdSecCnt << endl;
	cout << setw(22) << "[debug]NumFATs:" << setw(14) << NumFATs << endl;
	cout << setw(22) << "[debug]RootEntCnt:" << setw(14) << RootEntCnt << endl;
	cout << setw(22) << "[debug]FATSz:" << setw(14) << FATSz << endl;
	cout << setw(22) << "------------end-------------------" << endl;
}
void fillTime(u16 &DIR_WrtDate, u16 &DIR_WrtTime)
{
	time_t tt = time(NULL);//这句返回的只是一个时间cuo
	tm* t = localtime(&tt);
	stringstream stream;
	stream << t->tm_year - 100 << t->tm_mon + 1 << t->tm_mday;
	string tmp = stream.str();
	DIR_WrtDate = atoi(tmp.c_str());
	stream.str("");
	stream << t->tm_hour << t->tm_min << t->tm_sec;
	tmp = stream.str();
	DIR_WrtTime = atoi(tmp.c_str());
}
void fillHandles()
{
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
	RootEntry *FileInfo_ptr = (RootEntry*)malloc(sizeof(RootEntry));
	for (int i = 0; i < RootEntCnt; i++)
	{


		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(FileInfo_ptr, 32, NULL);
		if (FileInfo_ptr->DIR_Name[0] == '\0')
		{
			base += 32;
			continue;

		}
		else {
			//printRootEntryStruct(FileInfo_ptr);
			base += 32;
			int i;
			dwHandles.push_back(*FileInfo_ptr);
		}

	}
}
/** \brief
寻找空簇，返回空簇的簇号，来填入文件根目录信息。
*/
u16 findEmptyFat()
{
	int offset_fat = RsvdSecCnt*BytsPerSec + 1;//+3是从第二个簇开始寻找，也是第一个存放的fat
	//cout << "[debug]offset_fat: " << offset_fat << " (10 hexadecimal) " << hex << offset_fat << "(16 hexadecimal) in file!" << endl;
	bool _oven = true;//初始簇为2
	u16 result = 2;  //初始簇为2
	while (1)
	{
		if (_oven == true)
		{
			offset_fat += 2;
			SetHeaderOffset(offset_fat, NULL, FILE_BEGIN);
		}
		else
		{
			offset_fat += 1;
			SetHeaderOffset(offset_fat, NULL, FILE_BEGIN);
		}
		u16 bytes;
		u16 * bytes_ptr = &bytes;
		ReadFromDisk(bytes_ptr, 2, NULL);
		if (_oven == true) {
			bytes = bytes << 4;
			bytes = bytes >> 4;
		}
		else {
			bytes = bytes >> 4;
			bytes = bytes << 4;
		}
		//cout << "0x" << hex << bytes << "H" << endl;//得到了簇2对应的值
		if (bytes == 0x0)//如果找到了空的fat
		{
			break;
		}
		result++;

		_oven = !_oven;
	}
	return result;
}
/** \brief
根据传递的簇号,找到下一个的簇号！
*/
u16 findNextFat(u16 FstClus)
{
	int offset_fat = RsvdSecCnt*BytsPerSec;//+3是从第二个簇开始寻找，也是第一个存放的fat
											   //cout << "[debug]offset_fat: " << offset_fat << " (10 hexadecimal) " << hex << offset_fat << "(16 hexadecimal) in file!" << endl;
	bool _oven;
	if (FstClus % 2 == 0) {//偶数
		offset_fat += 1.5*FstClus + 1;
		_oven = true;
	}
	else
	{
		offset_fat += 1.5*FstClus + 0.5;
		_oven = false;
	}
	//cout << "[debug]Cluster " << FstClus << "'s offset in fat is " << offset_fat - RsvdSecCnt*BytsPerSec << endl;
	SetHeaderOffset(offset_fat - 1, NULL, FILE_BEGIN);
	u16 bytes;
	u16 * bytes_ptr = &bytes;

	ReadFromDisk(bytes_ptr, 2, NULL);
	if (_oven == true) {
		bytes = bytes << 4;
		bytes = bytes >> 4;
	}
	else {
		bytes = bytes >> 4;
	}
	//cout << "[debug]Cluster " << FstClus << "'s next cluster is 0x" << hex << bytes << "H" << endl;
	return bytes;
}
/** \brief
写入FAT对应的簇。
*/
void writeFat(u16 FstClus, u16 bytes) 
{
	int offset_fat = RsvdSecCnt*BytsPerSec;
	bool _oven;
	if (FstClus % 2 == 0) {//偶数
		offset_fat += 1.5*FstClus + 1;
		_oven = true;
	}
	else
	{
		offset_fat += 1.5*FstClus + 0.5;
		_oven = false;
	}
	SetHeaderOffset(offset_fat - 1, NULL, FILE_BEGIN);
	u16 bytesOri;
	u16 * bytes_ptrOri = &bytesOri;
	u16 * bytes_ptr = &bytes;
	ReadFromDisk(bytes_ptrOri, 2, NULL);
//	cout << "[debug]writeFat() origin cluster is 0x" << hex << bytes_ptrOri << "H" << endl;
	if (_oven == true) {
		bytesOri = bytesOri >> 12;//只留下前四位
		bytesOri = bytesOri << 12;
		bytes = bytes << 4;
		bytes = bytes >> 4;

	}
	else {
		bytesOri = bytesOri << 12;//只留下后四位
		bytesOri = bytesOri >> 12;
		bytes = bytes >> 4;
		bytes = bytes << 4;
	}
	bytes = bytes | bytesOri;
	SetHeaderOffset(offset_fat - 1, NULL, FILE_BEGIN);
	WriteToDisk(bytes_ptr,2,NULL);
}

void printFilesNew(struct RootEntry* rootEntry_ptr)
{
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec; //根目录首字节的偏移数
	int check;
	char realName[12];  //暂存将空格替换成点后的文件名

						//依次处理根目录中的各个条目
	int i;
	for (i = 0; i < RootEntCnt; i++)
	{

		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(rootEntry_ptr, 32, NULL);
		base += 32;

		if (rootEntry_ptr->DIR_Name[0] == '\0') continue;     //空条目不输出

															  //过滤非目标文件
		int j;
		int boolean = 0;
		for (j = 0; j < 11; j++)
		{
			if (!(((rootEntry_ptr->DIR_Name[j] >= 48) && (rootEntry_ptr->DIR_Name[j] <= 57)) ||
				((rootEntry_ptr->DIR_Name[j] >= 65) && (rootEntry_ptr->DIR_Name[j] <= 90)) ||
				((rootEntry_ptr->DIR_Name[j] >= 97) && (rootEntry_ptr->DIR_Name[j] <= 122)) ||
				(rootEntry_ptr->DIR_Name[j] == ' ')))
			{
				boolean = 1;    //非英文及数字、空格
				break;
			}
		}
		if (boolean == 1) continue;  //非目标文件不输出

		int k;
		if ((rootEntry_ptr->DIR_Attr & 0x10) == 0)
		{
			//文件
			int tempLong = -1;
			for (k = 0; k < 11; k++)
			{
				if (rootEntry_ptr->DIR_Name[k] != ' ')
				{
					tempLong++;
					realName[tempLong] = rootEntry_ptr->DIR_Name[k];
				}
				else
				{
					tempLong++;
					realName[tempLong] = '.';
					while (rootEntry_ptr->DIR_Name[k] == ' ') k++;
					k--;
				}
			}
			tempLong++;
			realName[tempLong] = '\0';  //到此为止，把文件名提取出来放到了realName里

										//输出文件
			printf("%s\n", realName);
		}
		else
		{
			//目录
			int tempLong = -1;
			for (k = 0; k < 11; k++)
			{
				if (rootEntry_ptr->DIR_Name[k] != ' ')
				{
					tempLong++;
					realName[tempLong] = rootEntry_ptr->DIR_Name[k];
				}
				else
				{
					tempLong++;
					realName[tempLong] = '\0';
					break;
				}
			}   //到此为止，把目录名提取出来放到了realName

				//输出目录及子文件
			printChildrenNew(realName, rootEntry_ptr->DIR_FstClus);
		}
	}
}

void printChildrenNew(char * directory, int startClus)
{
	//数据区的第一个簇（即2号簇）的偏移字节
	int dataBase = BytsPerSec * (RsvdSecCnt + FATSz * NumFATs + (RootEntCnt * 32 + BytsPerSec - 1) / BytsPerSec);
	char fullName[24];  //存放文件路径及全名
	int strLength = strlen(directory);
	strcpy(fullName, directory);
	fullName[strLength] = '/';
	strLength++;
	fullName[strLength] = '\0';
	char* fileName = &fullName[strLength];

	int currentClus = startClus;
	int value = 0;//value為十六進制數，每次存儲2/4個字節
	int ifOnlyDirectory = 0;
	while (value < 0xFF8)
	{
		value = getFATValueNew(currentClus);
		if (value == 0xFF7)
		{
			printf("bad cluster!Reading fails!\n");
			break;
		}

		char* str = (char*)malloc(SecPerClus * BytsPerSec); //暂存从簇中读出的数据
		char* content = str;

		int startByte = dataBase + (currentClus - 2) * SecPerClus * BytsPerSec;
		int check;
		//check = fseek(fat12, startByte, SEEK_SET);
		//if (check == -1)
		//	printf("fseek in printChildren failed!");

		//check = fread(content, 1, SecPerClus * BytsPerSec, fat12);
		//if (check != SecPerClus * BytsPerSec)
		//	printf("fread in printChildren failed!");
		SetHeaderOffset(startByte, NULL, FILE_BEGIN);
		check = ReadFromDisk(content, SecPerClus * BytsPerSec, NULL);
		//解析content中的数据,依次处理各个条目,目录下每个条目结构与根目录下的目录结构相同
		int count = SecPerClus * BytsPerSec; //每簇的字节数
		int loop = 0;
		while (loop < count)
		{
			int i;
			char tempName[12];  //暂存替换空格为点后的文件名
			if (content[loop] == '\0')
			{
				loop += 32;
				continue;
			}   //空条目不输出
				//过滤非目标文件
			int j;
			int boolean = 0;
			for (j = loop; j < loop + 11; j++)
			{
				if (!(((content[j] >= 48) && (content[j] <= 57)) ||
					((content[j] >= 65) && (content[j] <= 90)) ||
					((content[j] >= 97) && (content[j] <= 122)) ||
					(content[j] == ' ')))
				{
					boolean = 1;    //非英文及数字、空格
					break;
				}
			}
			if (boolean == 1)
			{
				loop += 32;
				continue;
			}   //非目标文件不输出
			int k;
			int tempLong = -1;
			for (k = 0; k < 11; k++)
			{
				if (content[loop + k] != ' ')
				{
					tempLong++;
					tempName[tempLong] = content[loop + k];
				}
				else
				{
					tempLong++;
					tempName[tempLong] = '.';
					while (content[loop + k] == ' ') k++;
					k--;
				}
			}
			tempLong++;
			tempName[tempLong] = '\0';  //到此为止，把文件名提取出来放到tempName里

			strcpy(fileName, tempName);
			printf("%s\n", fullName);
			ifOnlyDirectory = 1;
			loop += 32;
		}

		free(str);

		currentClus = value;
	};

	if (ifOnlyDirectory == 0)
		printf("%s\n", fullName);  //空目录的情况下，输出目录
}
int  getFATValueNew(int num)
{
	//FAT1的偏移字节
	int fatBase = RsvdSecCnt * BytsPerSec;
	//FAT项的偏移字节
	int fatPos = fatBase + num * 3 / 2;
	//奇偶FAT项处理方式不同，分类进行处理，从0号FAT项开始
	int type = 0;
	if (num % 2 == 0)
	{
		type = 0;
	}
	else
	{
		type = 1;
	}

	//先读出FAT项所在的两个字节
	u16 bytes;
	u16* bytes_ptr = &bytes;
	int check;
	//check = fseek(fat12, fatPos, SEEK_SET);
	//if (check == -1)
	//	printf("fseek in getFATValue failed!");

	//check = fread(bytes_ptr, 1, 2, fat12);
	//if (check != 2)
	//	printf("fread in getFATValue failed!");
	SetHeaderOffset(fatPos, NULL, FILE_BEGIN);
	check = ReadFromDisk(bytes_ptr, 2, NULL);
	//u16为short，结合存储的小尾顺序和FAT项结构可以得到
	//type为0的话，取byte2的低4位和byte1构成的值，type为1的话，取byte2和byte1的高4位构成的值
	if (type == 0)
	{
		return bytes << 4;
	}
	else
	{
		return bytes >> 4;
	}


}
template<typename T>
int getHandleLength(T &x)
{
	int s1 = sizeof(x);
	int s2 = sizeof(x[0]);
	int result = s1 / s2;
	return result;
}
BOOL isFileExist(char *pszFileName, u16 FstClus)
{


	return true;
}
DWORD createHandle(RootEntry* FileInfo) {
	int i;
	dwHandles.push_back(*FileInfo);
	i = dwHandles.size();
	return i;
}
/** \brief
10进制数字转换为16进制string
*/

string DecIntToHexStr(int num)
{
	string str;
	int Temp = num / 16;
	int left = num % 16;
	if (Temp > 0)
		str += DecIntToHexStr(Temp);
	if (left < 10)
		str += (left + '0');
	else
		str += ('A' + left - 10);
	return str;
}
/** \brief
16进制字符串char*转换为10进制string
*/
string ToHexString(u8* buf, int len, std::string tok = "")
{
	std::string output;
	char temp[8];
	for (int i = 0; i < len; ++i)
	{
		sprintf(temp, "0x%.2x", (u8)buf[i]);
		output.append(temp, 4);
		output.append(tok);
	}

	return output;
}
/** \brief
更新Fat表项
*/
void updateRootEntry(RootEntry *FileInfo_ptr)
{
	int dataBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;//写回fat
	for (int i = 0; i < RootEntCnt; i++)
	{
		SetHeaderOffset(dataBase, NULL, FILE_BEGIN);
		ReadFromDisk(rootEntry_ptr, 32, NULL);
		dataBase += 32;
		if (strcmp(rootEntry_ptr->DIR_Name, FileInfo_ptr->DIR_Name) == 0)
		{
			SetHeaderOffset(dataBase - 32, NULL, FILE_BEGIN);
			WriteToDisk(FileInfo_ptr, 32, NULL);
			return;
		}
	}

}
#endif // UTIL_H_INCLUDED
