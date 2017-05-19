
#include "DiskLib.h"
#include "MyApi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#pragma comment(lib,"DiskLib.lib")
using namespace std;

#ifdef OLDVERSION

FILE* fat12 = fopen("floppy.img", "rb");//这句将会被替换
#endif // OLDVERSION
void printFilesNew(struct RootEntry* rootEntry_ptr);//从util中实现的函数
struct BPB bpb;
struct BPB* bpb_ptr = &bpb;
struct RootEntry rootEntry;
struct RootEntry* rootEntry_ptr = &rootEntry;
const char* fs = "floppy.img";
int  getFATValue(FILE * fat12, int num);
void printFiles(FILE * fat12, struct RootEntry* rootEntry_ptr);  //打印文件名，这个函数在打印目录时会调用下面的printChildren
void fillBPB(FILE * fat12, struct BPB* bpb_ptr);
void printChildren(FILE * fat12, char * directory, int startClus);  //打印目录及目录下子文件名

int main()
{
	
	StartupDisk(fs);
	SetHeaderOffset(11, NULL, FILE_BEGIN);
	if (ReadFromDisk(bpb_ptr, 25, NULL) != 0) {
		BytsPerSec = bpb_ptr->BPB_BytsPerSec;
		SecPerClus = bpb_ptr->BPB_SecPerClus;
		RsvdSecCnt = bpb_ptr->BPB_RsvdSecCnt;
		NumFATs = bpb_ptr->BPB_NumFATs;
		RootEntCnt = bpb_ptr->BPB_RootEntCnt;
		if (bpb_ptr->BPB_FATSz16 != 0)
		{
			FATSz = bpb_ptr->BPB_FATSz16;
		}
		else
		{
			FATSz = bpb_ptr->BPB_TotSec32;
		}
	}
	else {
		cout << "[debug]:Read BPB fail!" << endl;
	}
	
	MyCreateFile("", "t32.txt");
	printFilesNew( rootEntry_ptr);
#ifdef OLDVERSION
	//载入fat12文件给BPB.fat12必须是11~25字节存放了BPB中的数据，比如：存放了每扇区字节数=13
	fillBPB(fat12, bpb_ptr);
	//初始化各个全局变量

	if (bpb_ptr->BPB_FATSz16 != 0)
	{
		FATSz = bpb_ptr->BPB_FATSz16;
	}
	else
	{
		FATSz = bpb_ptr->BPB_TotSec32;
	}

	//	struct RootEntry rootEntry;
	//	struct RootEntry* rootEntry_ptr = &rootEntry;
	MyCreateFile("", "tyc.txt");
	//打印文件名
	printFiles(fat12, rootEntry_ptr);
	//	cout << "[output]" << bpb_ptr->BPB_FATSz16 << endl;
	//	cout << "[output]" << bpb_ptr->BPB_TotSec16 << endl;
	//	cout << "[output]" << bpb_ptr->BPB_TotSec32 << endl;
#endif // OLDVERSION
}

void fillBPB(FILE* fat12, struct BPB* bpb_ptr)
{
	int check;

	//BPB从偏移11个字节处开始
	check = fseek(fat12, 11, SEEK_SET); //将读写函数的指针移动到指定位置
	if (check == -1)
		printf("fseek in fillBPB failed!");

	//BPB长度为25字节
	check = fread(bpb_ptr, 1, 25, fat12); //读取25个1字节到bpb_ptr
	if (check != 25)
		printf("fread in fillBPB failed!");
}

int  getFATValue(FILE * fat12, int num)   //NUM=开始簇号DIR_FstClus
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
	check = fseek(fat12, fatPos, SEEK_SET);
	if (check == -1)
		printf("fseek in getFATValue failed!");

	check = fread(bytes_ptr, 1, 2, fat12);
	if (check != 2)
		printf("fread in getFATValue failed!");

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
void printChildren(FILE * fat12, char * directory, int startClus)
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
		value = getFATValue(fat12, currentClus);
		if (value == 0xFF7)
		{
			printf("bad cluster!Reading fails!\n");
			break;
		}

		char* str = (char*)malloc(SecPerClus * BytsPerSec); //暂存从簇中读出的数据
		char* content = str;

		int startByte = dataBase + (currentClus - 2) * SecPerClus * BytsPerSec;
		int check;
		check = fseek(fat12, startByte, SEEK_SET);
		if (check == -1)
			printf("fseek in printChildren failed!");

		check = fread(content, 1, SecPerClus * BytsPerSec, fat12);
		if (check != SecPerClus * BytsPerSec)
			printf("fread in printChildren failed!");

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
void printFiles(FILE * fat12, struct RootEntry* rootEntry_ptr)
{
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec; //根目录首字节的偏移数
	int check;
	char realName[12];  //暂存将空格替换成点后的文件名

						//依次处理根目录中的各个条目
	int i;
	for (i = 0; i < RootEntCnt; i++)
	{

		check = fseek(fat12, base, SEEK_SET);
		if (check == -1)
			printf("fseek in printFiles failed!");

		check = fread(rootEntry_ptr, 1, 32, fat12);
		if (check != 32)
			printf("fread in printFiles failed!");

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
			printChildren(fat12, realName, rootEntry_ptr->DIR_FstClus);
		}
	}
}
