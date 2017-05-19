#include "MyApi.h"
#include "util.h"
#include "DiskLib.h"
#ifdef OLDVERSION


extern FILE* fat12;//这句将会被替换
#endif // OLDVERSION

RootEntry* dwHandles[30] = { NULL };//文件句柄？
extern struct BPB* bpb_ptr;
extern struct RootEntry* rootEntry_ptr;
//下面都为固定值。
int  BytsPerSec;    //每扇区字节数
int  SecPerClus;    //每簇扇区数
int  RsvdSecCnt;    //Boot记录占用的扇区数
int  NumFATs;   //FAT表个数
int  RootEntCnt;    //根目录最大文件数
int  FATSz; //FAT扇区数
DWORD MyCreateFile(char *pszFolderPath, char *pszFileName)
{
	DWORD FileHandle = 0;
	printBPB();
	cout << "[output]trying to create file named " << pszFileName;
	if (strcmp(pszFolderPath, "") != 0)
		cout << " in folder " << pszFolderPath << endl;
	else cout << endl;
	//準備尋找
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
	cout << "[output]constant base:" << base << endl;
	int check;
	char realName[12];  //暂存将空格替换成点后的文件名
	//依次处理根目录中的各个条目
	int i;
	int max_FstClus = 0;//用于寻找最大的首簇FstClus,新文件添加在其之后。
	int max_FinalClus = 0;//终止簇=FstClus+m
	for (i = 0; i < RootEntCnt; i++)
	{
		
		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(rootEntry_ptr, 32, NULL);
		printRootEntryStruct(rootEntry_ptr);
		if (max_FstClus < rootEntry_ptr->DIR_FstClus)
		{
			max_FstClus = rootEntry_ptr->DIR_FstClus;
			u32 cuSize = SecPerClus*BytsPerSec; //512
			u16 needCu = (rootEntry_ptr->DIR_FileSize + cuSize - 1) / cuSize;
			max_FinalClus = max_FstClus + needCu;
		}
		base += 32;
		//不需要过滤！
		
		if (strcmp(pszFileName, rootEntry_ptr->DIR_Name)== 0)//已经存在（重名）
			return 0;
		if (rootEntry_ptr->DIR_Name[0] == '\0') //continue;     //非空条目，不可以写入文件
			//空条目，可以写入文件

		{
			RootEntry *FileInfo_ptr = (RootEntry*)malloc(sizeof(RootEntry));
			cout << "[output]into loop2" << endl;
			//rootEntry_ptr->DIR_Name=pszFileName;
			strcpy(FileInfo_ptr->DIR_Name, pszFileName);
			for (int j = sizeof(pszFileName);j<11;j++)
			{
				FileInfo_ptr->DIR_Name[j] = pszFileName[j];
				//memcpy(&FileInfo_ptr->DIR_Name[j], &pszFileName[j],1);
			}
			
			//memcpy(&FileInfo_ptr->DIR_Name[8],&pszFileName[i],3);
			FileInfo_ptr->DIR_Attr = 0x20;
			fillTime(FileInfo_ptr->DIR_WrtDate, FileInfo_ptr->DIR_WrtTime);
			FileInfo_ptr->DIR_FileSize = 10000;//随机的，待修改
			u32 cuSize = SecPerClus*BytsPerSec; //512
			u16 needCu = (FileInfo_ptr->DIR_FileSize + cuSize - 1) / cuSize;//向上取整，获得需要的簇的个数
// TODO (812015941#1#): 需要寻找放入的位置
			FileInfo_ptr->DIR_FstClus = max_FinalClus +1;//！最大的文件占用的最后一个簇+1，就是当前新文件的首簇！
			cout << "[output]constant cuSize :" << cuSize << endl;
			cout << "[output]constant needCu :" << needCu << endl;
			cout << "[output]constant input file name:" << FileInfo_ptr->DIR_Name << endl;
			cout << "[output]constant input FileSize:" << FileInfo_ptr->DIR_FileSize << endl;
			printf("[output]constant input file attr:%x\n", FileInfo_ptr->DIR_Attr);
			cout << "[output]constant input file date:" << FileInfo_ptr->DIR_WrtDate << endl;
			cout << "[output]constant input file time:" << FileInfo_ptr->DIR_WrtTime << endl;
			cout << "[output]constant input file firstClus:" << FileInfo_ptr->DIR_FstClus << endl;
			// TODO (812015941#1#): 写入磁盘中
			SetHeaderOffset(base-32, NULL, FILE_BEGIN);
			if (WriteToDisk(FileInfo_ptr, 32, NULL))
			{
				FileHandle=createHandle(FileInfo_ptr);
				break;
			}

			else cout << "[debug]Write to disk fail!" << endl;
		}

	}
	return FileHandle;
}

/** \brief
要求：打开指定目录下的指定文件，如果目录不存在或者文件不存在，则返回0表示失败；
如果成功则返回一个表示该文件的标识（类似于Window的句柄，内部数据结构及映射方法你来定）
pszFolderPath：目录路径，如"C:\\Test\\Test01"等等
pszFileName：文件名，如"Test.txt"等等
*/
DWORD MyOpenFile(char *pszFolderPath, char *pszFileName)
{
	return 0;
}