#include "MyApi.h"
#include "util.h"
#include "DiskLib.h"
#ifdef OLDVERSION


extern FILE* fat12;//这句将会被替换
#endif // OLDVERSION
using namespace std;
extern struct BPB* bpb_ptr;
extern struct RootEntry* rootEntry_ptr;
//下面都为固定值。
int  BytsPerSec;    //每扇区字节数
int  SecPerClus;    //每簇扇区数
int  RsvdSecCnt;    //Boot记录占用的扇区数
int  NumFATs;   //FAT表个数
int  RootEntCnt;    //根目录最大文件数
int  FATSz; //FAT扇区数
BOOL MyCreateDirectory(char *pszFolderPath, char *pszFolderName)//path需要有， 但是folder需要无
{
	u16 FstClus = findEmptyFat();//用来填写DIR_FstClus
	u16 FstClusHJQ;//用来存储根目录的DIR_FstClus
	int base;
	cout << "[debug]findEmptyFat():" << FstClus << endl;
	cout << "[output]trying to create Directory named " << pszFolderName;
	if (strcmp(pszFolderPath, "") != 0)
	{
		cout << " in folder " << pszFolderPath << endl;
		if ((FstClusHJQ = isPathExist(pszFolderPath)) || strlen(pszFolderPath) == 3)//存在才能继续,以后添加的文件应
		{
			if (isDirectoryExist(pszFolderName, FstClusHJQ))
			{
				cout << "[output]" << pszFolderPath << '\\' << pszFolderName << " folder has existed!" << endl;
				return false;//获得pszFolderName的簇号！
			}
			if (FstClusHJQ == 0) {
				// 根目录区偏移
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
			}
			else {
				// 数据区文件首址偏移
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClusHJQ - 2) * BytsPerSec;//=
				cout << "[debug]" << pszFolderPath << "'s FstClus value is:" << FstClusHJQ << ";postion (16位): is " << hex << base << endl;
			}
		}
		else {
			cout << "[output]" << pszFolderPath << '\\' << pszFolderName << " path does not exist!" << endl;

			return false;
		}
	}//---------------------------------------------上面是输入了路径的情况。
	else {
		cout << endl;
		base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
	}//---------------------------------------------没有路径，创建在根目录下。
	cout << "[output]" << pszFolderName << "'s RootEntry is ready to write in position:" << hex << base << " (16 hexadecimal)" << endl;

	for (int i = 0; i < RootEntCnt; i++)//寻找空的目录！
	{
		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(rootEntry_ptr, 32, NULL);
		base += 32;
		if (strcmp(pszFolderName, rootEntry_ptr->DIR_Name) == 0)//已经存在（重名）
			return false;

		if (rootEntry_ptr->DIR_Name[0] == '\0') //continue;     //非空条目，不可以写入文件
												//空条目，可以写入文件
		{
			RootEntry *DirInfo_ptr = (RootEntry*)malloc(sizeof(RootEntry));
			strcpy(DirInfo_ptr->DIR_Name, pszFolderName);
			DirInfo_ptr->DIR_Attr = 0x10;
			fillTime(DirInfo_ptr->DIR_WrtDate, DirInfo_ptr->DIR_WrtTime);
			DirInfo_ptr->DIR_FileSize = 0;
			DirInfo_ptr->DIR_FstClus = FstClus;
			if (strcmp(pszFolderPath, "") != 0)//如果有根路径
			{
				SetHeaderOffset(base - 32, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				//TODO:准备添加'.' 与 '..'，“.”是当前目录的别名，“..”首簇就改指向上级目录文件的首簇。
				//添加到它分配的簇。
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (DirInfo_ptr->DIR_FstClus - 2) * BytsPerSec;//重新计算base，在自己的数据扇区写"."！
				int selfFstClus = DirInfo_ptr->DIR_FstClus;//暂存自己的首簇，用来填"."
				cout << "[debug]该文件夹的地址（有根）路径 :" << hex << base << endl;
				//TODO:把对应扇区从F6清0！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!------------------------------------------
				clearCu(DirInfo_ptr->DIR_FstClus);
				SetHeaderOffset(base, NULL, FILE_BEGIN);
				initFileInfo(DirInfo_ptr, ".", 0x10, 0, DirInfo_ptr->DIR_FstClus);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				initFileInfo(DirInfo_ptr, "..", 0x10, 0, FstClusHJQ);//填上一个跟目录的首簇
				SetHeaderOffset(base + 32, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				writeFat(FstClus, 0xffff);
				return true;
			}
			if (strcmp(pszFolderPath, "") == 0)//如果无根路径，在根文件目录写；并且应该在数据扇区相应的位置添加.和..才对。
			{

				SetHeaderOffset(base - 32, NULL, FILE_BEGIN);//写在根目录
				WriteToDisk(DirInfo_ptr, 32, NULL);
				FstClusHJQ = DirInfo_ptr->DIR_FstClus;//暂存自己的首簇，用来填"."
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32;//10176
				base += (FstClusHJQ - 2) * BytsPerSec;//写在对应的扇区！
				cout << "[debug]该文件夹的地址（无根） :" << hex << base << endl;
				//TODO:把对应扇区从F6清0！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!------------------------------------------
				clearCu(DirInfo_ptr->DIR_FstClus);
				SetHeaderOffset(base, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				initFileInfo(DirInfo_ptr, ".", 0x10, 0, DirInfo_ptr->DIR_FstClus);//簇就是自己！
				SetHeaderOffset(base, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				initFileInfo(DirInfo_ptr, "..", 0x10, 0, 0x00);//上一个簇就是根目录，0簇
				SetHeaderOffset(base + 32, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				writeFat(FstClus, 0xffff);
				return true;
			}
		}

	}
	return false;
}
DWORD MyCreateFile(char *pszFolderPath, char *pszFileName)
{
	fillHandles();
	int dwHandle = 0;
	u16 FstClus = findEmptyFat();//用来填写DIR_FstClus
	u16 FstClusHJQ;//用来存储根目录的DIR_FstClus
	int base;
	cout << "[debug]findEmptyFat():" << FstClus << endl;
	cout << "[output]trying to create Directory named " << pszFileName;
	if (strcmp(pszFolderPath, "") != 0)
	{
		cout << " in folder " << pszFolderPath << endl;
		if ((FstClusHJQ = isPathExist(pszFolderPath)) || strlen(pszFolderPath) == 3)//存在才能继续,以后添加的文件应
		{
			if (isFileExist(pszFileName, FstClusHJQ))
			{
				cout << "[output]" << pszFolderPath << '\\' << pszFileName << " file has existed!" << endl;
				return false;//获得pszFolderName的簇号！
			}
			if (FstClusHJQ == 0) {
				// 根目录区偏移
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
			}
			else {
				// 数据区文件首址偏移
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClusHJQ - 2) * BytsPerSec;//=
				cout << "[debug]" << pszFolderPath << "'s FstClus value is:" << FstClusHJQ << ";postion (16位): is " << hex << base << endl;
			}
		}
		else {
			cout << "[output]" << pszFolderPath << '\\' << pszFileName << " path does not exist!" << endl;

			return false;
		}
	}//---------------------------------------------上面是输入了路径的情况。
	else {
		cout << endl;
		base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
	}//---------------------------------------------没有路径，创建在根目录下。
	cout << "[output]" << pszFileName << "'s RootEntry is ready to write in position:" << hex << base << " (16 hexadecimal)" << endl;
	for (int i = 0; i < RootEntCnt; i++)//寻找空的目录！
	{
		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(rootEntry_ptr, 32, NULL);
		base += 32;
		if (strcmp(pszFileName, rootEntry_ptr->DIR_Name) == 0)//已经存在（重名）
			return false;
		if (rootEntry_ptr->DIR_Name[0] == '\0') //continue;     //非空条目，不可以写入文件
												//空条目，可以写入文件
		{
			RootEntry *DirInfo_ptr = (RootEntry*)malloc(sizeof(RootEntry));
			strcpy(DirInfo_ptr->DIR_Name, pszFileName);
			DirInfo_ptr->DIR_Attr = 0x20;
			fillTime(DirInfo_ptr->DIR_WrtDate, DirInfo_ptr->DIR_WrtTime);
			DirInfo_ptr->DIR_FileSize = 0;
			DirInfo_ptr->DIR_FstClus = FstClus;
			if (strcmp(pszFolderPath, "") != 0)//如果有根路径
			{
				FileHandle fileHandle;
				fileHandle.fileInfo = *DirInfo_ptr;
				dwHandles.push_back(fileHandle);
				dwHandle = dwHandles.size();
				SetHeaderOffset(base - 32, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				//TODO:准备添加'.' 与 '..'，“.”是当前目录的别名，“..”首簇就改指向上级目录文件的首簇。
				//添加到它分配的簇。
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (DirInfo_ptr->DIR_FstClus - 2) * BytsPerSec;//重新计算base,得到这个文件簇所在的物理地址
				int selfFstClus = DirInfo_ptr->DIR_FstClus;//暂存自己的首簇，用来填"."
				cout << "[debug]该文件的地址（有根）路径 :" << hex << base << endl;
				//TODO:把对应扇区从F6清0！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!------------------------------------------
				clearCu(DirInfo_ptr->DIR_FstClus);
				writeFat(FstClus, 0xffff);
				return dwHandle;
			}
			else if (strcmp(pszFolderPath, "") == 0)//如果无根路径，在根文件目录写；并且应该在数据扇区相应的位置添加.和..才对。
			{
				FileHandle fileHandle;
				fileHandle.fileInfo = *DirInfo_ptr;
				dwHandles.push_back(fileHandle);
				dwHandle = dwHandles.size();
				SetHeaderOffset(base - 32, NULL, FILE_BEGIN);//写在根目录
				WriteToDisk(DirInfo_ptr, 32, NULL);
				FstClusHJQ = DirInfo_ptr->DIR_FstClus;//暂存自己的首簇，用来填"."
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32;//10176
				base += (FstClusHJQ - 2) * BytsPerSec;//写在对应的扇区！
				cout << "[debug]该文件的地址（无根） :" << hex << base << endl;
				//TODO:把对应扇区从F6清0！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!------------------------------------------
				clearCu(DirInfo_ptr->DIR_FstClus);
				writeFat(FstClus, 0xffff);
				return dwHandle;
			}
		}
	}
	return dwHandle;



	/*
	for (int i = 0; i < RootEntCnt; i++)
	{

		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(rootEntry_ptr, 32, NULL);
		base += 32;
		//不需要过滤！

		if (strcmp(pszFileName, rootEntry_ptr->DIR_Name) == 0)//已经存在（重名）
			return 0;
		if (rootEntry_ptr->DIR_Name[0] == '\0'|| rootEntry_ptr->DIR_Name[0] == '\0') //continue;     //非空条目，不可以写入文件
			//空条目，可以写入文件

		{
			RootEntry *FileInfo_ptr = (RootEntry*)malloc(sizeof(RootEntry));
			cout << "[output]into loop2" << endl;
			strcpy(FileInfo_ptr->DIR_Name, pszFileName);
			FileInfo_ptr->DIR_Attr = 0x20;
			fillTime(FileInfo_ptr->DIR_WrtDate, FileInfo_ptr->DIR_WrtTime);
			FileInfo_ptr->DIR_FileSize = 0;
			u32 cuSize = SecPerClus*BytsPerSec; //512
			u16 needCu = (FileInfo_ptr->DIR_FileSize + cuSize - 1) / cuSize;//向上取整，获得需要的簇的个数
//TODO：开始簇需要改写。
			FileInfo_ptr->DIR_FstClus = FstClus;
// TODO (812015941#1#): 写入磁盘中
			SetHeaderOffset(base - 32, NULL, FILE_BEGIN);
			if (WriteToDisk(FileInfo_ptr, 32, NULL))
			{
				dwHandles.push_back(*FileInfo_ptr);
				FileHandle = dwHandles.size();
				writeFat(FstClus, 0xffff);
				break;
			}

			else cout << "[debug]Write to disk fail!" << endl;
		}

	}
	return FileHandle;
	*/
}

/** \brief
要求：打开指定目录下的指定文件，如果目录不存在或者文件不存在，则返回0表示失败；
如果成功则返回一个表示该文件的标识（类似于Window的句柄，内部数据结构及映射方法你来定）
pszFolderPath：目录路径，如"C:\\Test\\Test01"等等
pszFileName：文件名，如"Test.txt"等等
*/
DWORD MyOpenFile(char *pszFolderPath, char *pszFileName)
{
	fillHandles();
	DWORD FileHandle = 0;
	bool trick = true;//trick:为真才能返回。
	int HandleBack = 0;
	//printBPB();
	cout << "[output]trying to open file named " << pszFileName;
	if (strcmp(pszFolderPath, "") != 0)
	{
		//trick:如果没有目录，第一次出现就取出。如果没有目录，那么第二次取出
		trick = !trick;
		cout << " in folder " << pszFolderPath << endl;
	}
	else cout << endl;
	//準備尋找
	int size = dwHandles.size();
	for (int i = 0; i < size; i++)
	{
		if (strcmp(pszFileName, dwHandles[i].fileInfo.DIR_Name) == 0)
		{
			FileHandle = i;
			if (trick) {
				return FileHandle;//为真，立刻返回！
			}
			else
			{
				trick = !trick;
				HandleBack = FileHandle;
			}
		}
	}
	/*for (int i = 1; i < dwHandles.size(); i++)
	{
		if (strcmp(pszFileName, dwHandles[i]->DIR_Name) == 0)
		{
			FileHandle = i;
			return FileHandle;
		}
		else iter++;
	}*/
	return HandleBack;
}
void MyCloseFile(DWORD dwHandle) {
	dwHandles.clear();
}
BOOL MyDeleteFile(char *pszFolderPath, char *pszFileName)
{
	u16 FstClusHJQ;//用来存储根目录的DIR_FstClus
	int base;
	cout << "[output]trying to delete file named " << pszFileName;
	if (strcmp(pszFolderPath, "") != 0)
	{
		cout << " in folder " << pszFolderPath << endl;
		if ((FstClusHJQ = isPathExist(pszFolderPath)) || strlen(pszFolderPath) == 3)//存在才能继续,以后添加的文件应
		{
			if (isFileExist(pszFileName, FstClusHJQ))
			{
				cout << "[output]" << pszFolderPath << '\\' << pszFileName << " file existed! Continuing..." << endl;
			}
			if (FstClusHJQ == 0) {
				// 根目录区偏移
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
			}
			else {
				// 数据区文件首址偏移
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClusHJQ - 2) * BytsPerSec;//=
				cout << "[debug]" << pszFolderPath << "'s FstClus value is:" << FstClusHJQ << ";postion (16位): is " << hex << base << endl;
			}
		}
		else {
			cout << "[output]" << pszFolderPath << '\\' << pszFileName << " path does not exist!" << endl;
			return false;
		}
	}//---------------------------------------------上面是输入了路径的情况。
	else {
		cout << endl;
		base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
	}//---------------------------------------------没有路径，文件在根目录下。

	cout << "[output]" << pszFileName << "'s RootEntry is ready to be cleared in position:" << hex << base << " (16 hexadecimal)" << endl;
	for (int i = 0; i < RootEntCnt; i++)//找到要删除的文件！
	{
		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(rootEntry_ptr, 32, NULL);
		base += 32;
		if (strcmp(pszFileName, rootEntry_ptr->DIR_Name) == 0)//找到啦
		{
			RootEntry *DirInfo_ptr = (RootEntry*)malloc(sizeof(RootEntry));
			SetHeaderOffset(base - 32, NULL, FILE_BEGIN);//写在根目录
			initFileInfo(DirInfo_ptr, "", 0x0, 0, 0);
			WriteToDisk(DirInfo_ptr, 32, NULL);
			FstClusHJQ = rootEntry_ptr->DIR_FstClus;//暂存自己的首簇
			int nextClus = 0;
			do {
				if (nextClus != 0)//神来之笔！用来挨个删除东西！
				{
					FstClusHJQ = nextClus;
				}
				clearCu(FstClusHJQ);
				nextClus = findNextFat(FstClusHJQ);
				writeFat(FstClusHJQ, 0x0000);
			} while (nextClus != 0xFFF && nextClus != 0);
			return true;
		}
	}
	return false;


	/*
	cout << "[output]trying to delete file named " << pszFileName;
	if (strcmp(pszFolderPath, "") != 0)
		cout << " in folder " << pszFolderPath << endl;
	else cout << endl;
	//準備尋找
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
	int i;
	for (i = 0; i < RootEntCnt; i++)
	{

		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(rootEntry_ptr, 32, NULL);
		base += 32;
		//不需要过滤！

		if (strcmp(pszFileName, rootEntry_ptr->DIR_Name) == 0)//找到
		{
			RootEntry *FileInfo_ptr = (RootEntry*)malloc(sizeof(RootEntry));
			for (int j = 0; j < 11; j++)
			{
				FileInfo_ptr->DIR_Name[j] = '\0';
			}

			FileInfo_ptr->DIR_Attr = 0x00;
			fillTime(FileInfo_ptr->DIR_WrtDate, FileInfo_ptr->DIR_WrtTime);
			FileInfo_ptr->DIR_FileSize = 0;
			u32 cuSize = SecPerClus*BytsPerSec; //512
// TODO (812015941#1#): 需要寻找放入的位置
			SetHeaderOffset(base - 32, NULL, FILE_BEGIN);
			if (WriteToDisk(FileInfo_ptr, 32, NULL))
			{
				FileHandle = createHandle(FileInfo_ptr);
				//删除fat中对应的内容。
				u16 FstClus = rootEntry_ptr->DIR_FstClus;
				while (FstClus != 0xfff)
				{
					u16 tmpClus = findNextFat(FstClus);
					writeFat(FstClus, 0x0000);
					FstClus = tmpClus;
				}
				if (FstClus == 0xfff)//不能替换0x200H的保留簇
					writeFat(FstClus, 0x0000);
				break;
			}

			else cout << "[debug]Write to disk fail!" << endl;
		}

	}
	return;
	*/
}
BOOL MyDeleteDirectory(char *pszFolderPath, char *pszFolderName) {
	u16 FstClusHJQ;//用来存储根目录的DIR_FstClus
	int base;
	cout << "[output]trying to delete directory named " << pszFolderName;
	if (strcmp(pszFolderPath, "") != 0)
	{
		cout << " in folder " << pszFolderPath << endl;
		if ((FstClusHJQ = isPathExist(pszFolderPath)) || strlen(pszFolderPath) == 3)//存在才能继续,以后添加的文件应
		{
			if (isDirectoryExist(pszFolderName, FstClusHJQ))
			{
				cout << "[output]" << pszFolderPath << '\\' << pszFolderName << "folder existed! Continuing..." << endl;
			}
			if (FstClusHJQ == 0) {
				// 根目录区偏移
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
			}
			else {
				// 数据区文件首址偏移
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClusHJQ - 2) * BytsPerSec;//=
				cout << "[debug]" << pszFolderPath << "'s FstClus value is:" << FstClusHJQ << ";postion (16位): is " << hex << base << endl;
			}
		}
		else {
			cout << "[output]" << pszFolderPath << '\\' << pszFolderName << " path does not exist!" << endl;
			return false;
		}
	}//---------------------------------------------上面是输入了路径的情况。
	else {
		cout << endl;
		base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;
	}//---------------------------------------------没有路径，文件在根目录下。
	cout << "[output]" << pszFolderName << "'s RootEntry is ready to be cleared in position:" << hex << base << " (16 hexadecimal)" << endl;
	for (int i = 0; i < RootEntCnt; i++)//找到要删除的文件！
	{
		SetHeaderOffset(base, NULL, FILE_BEGIN);
		ReadFromDisk(rootEntry_ptr, 32, NULL);
		base += 32;
		int base_back = base;
		if (strcmp(pszFolderName, rootEntry_ptr->DIR_Name) == 0)//找到啦
		{
			RootEntry *DirInfo_ptr = (RootEntry*)malloc(sizeof(RootEntry));

			FstClusHJQ = rootEntry_ptr->DIR_FstClus;//暂存自己的首簇
//TODO:删除目录下的文件与目录。文件：删除记录即可！需要清空其簇！所以调用函数即可
			base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClusHJQ - 2) * BytsPerSec;//1 先去删除他空间的文件
			for (int i = 0; i < RootEntCnt; i++)//找到要删除的文件！
			{
				SetHeaderOffset(base, NULL, FILE_BEGIN);
				ReadFromDisk(rootEntry_ptr, 32, NULL);
				base += 32;
				if (rootEntry_ptr->DIR_Attr == 0x20 && rootEntry_ptr->DIR_Name[0] != '.')//2找到了文件
				{


					char tmpPath1[sizeof(pszFolderName) + 3] = { 0 };
					strcat(tmpPath1, "c:\\");
					strcat(tmpPath1, pszFolderName);
					char tmpName[sizeof(rootEntry_ptr->DIR_Name)];
					strcpy(tmpName, rootEntry_ptr->DIR_Name);
					MyDeleteFile(tmpPath1, tmpName);
					continue;
				}
				if (rootEntry_ptr->DIR_Attr == 0x10 && rootEntry_ptr->DIR_Name[0] != '.')//3找到了目录
				{
					char tmpPath1[sizeof(pszFolderName) + 3] = { 0 };
					strcat(tmpPath1, "c:\\");
					strcat(tmpPath1, pszFolderName);
					MyDeleteDirectory(tmpPath1, rootEntry_ptr->DIR_Name);
					continue;
				}
			}
			SetHeaderOffset(base_back - 32, NULL, FILE_BEGIN);//写在根目录//注意：这里后面要用！所以先不着急清除！
			initFileInfo(DirInfo_ptr, "", 0x0, 0, 0);
			WriteToDisk(DirInfo_ptr, 32, NULL);
			clearCu(FstClusHJQ);
			writeFat(FstClusHJQ, 0x0000);
			//int nextClus = 0;
			//do {
			//	if (nextClus != 0)//神来之笔！用来挨个删除东西！
			//	{
			//		FstClusHJQ = nextClus;
			//	}
			//	clearCu(FstClusHJQ);
			//	nextClus = findNextFat(FstClusHJQ);
			//	writeFat(FstClusHJQ, 0x0000);
			//} while (nextClus != 0xFFF && nextClus != 0);
			return true;
		}
	}
	return false;
}

//TODO: 写文件
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
DWORD MyWriteFile(DWORD dwHandle, LPVOID pBuffer, DWORD dwBytesToWrite) {
	int lenOfBuffer = dwBytesToWrite; // 缓冲区待写入长度
	char* cBuffer = (char*)malloc(sizeof(u8)*lenOfBuffer);
	memcpy(cBuffer, pBuffer, lenOfBuffer); // 复制过来
	struct RootEntry* FileInfo_ptr = &dwHandles[dwHandle].fileInfo;
	u32 updatedFileSize = dwBytesToWrite;
	int dataBase = 0, dataOffset = 0;
	//TODO:根据header去读
	u16 FstClus = FileInfo_ptr->DIR_FstClus;
	u16 nextEmptyClus = findEmptyFat();
	LONG offset = dwHandles[dwHandle].offset;//获取记录的当前偏移
	int curClusNum = offset / (BytsPerSec*SecPerClus); // 当前指针在第几个扇区,如果为0则在首簇，为1则说明在链接的第一个簇
	int curClusOffset = (offset )% (BytsPerSec*SecPerClus); // 当前在扇区内偏移
	u16 needClu = (curClusOffset + dwBytesToWrite + (BytsPerSec*SecPerClus) - 1) / (BytsPerSec*SecPerClus);//需要的簇数量。是一个整数，比如1,2,3，
	//--------------------------------下面是更新FAT。
	if (needClu == 1)//仅仅需要一个簇，填进去就是了
	{
	}
	else if (needClu > 1)
	{
		u16 nextClus;
		u16 FstClusUse = FstClus;//保证FstClus不变。
		u16 needClusBack = needClu;//保证FstClus不变。
		int originClus = 1;//文件拥有的簇的个数
		//int curClusNum_PLUS_needClu = curClusNum + needClu;
		while (1)//这个循环用来统计原来的文件分配了多少个簇
		{
			if (findNextFat(FstClusUse) == 0xfff)
			{
				//如果是新文件，只有一个簇，那么originClus=1
				break;
			}
			else {
				FstClusUse = findNextFat(FstClusUse);
				originClus++;
			}
		}
#define Empty_File 1
		//已经确定了他是个新文件、小文件、大文件。
		if (originClus >= needClusBack+curClusNum)//大文件，用他以前的扇区就行了。等式右边最大值是MY_FILE_END的情况。
		{

		}
		else if (originClus == Empty_File)//空文件，则分配新的足够数量的扇区
		{
			needClu = needClusBack;
			while (needClu > 1)//（必定会进入），这个循环用来分配新fat
			{
				nextClus = findEmptyFat();
				writeFat(FstClusUse, nextClus);//将自己从0XFFF更新成下一簇。
				needClu--;
				FstClusUse = nextClus;
				continue;
			}
			writeFat(FstClusUse, 0xffff);//将末尾簇写成0xfff
		}
		else if (originClus > Empty_File && originClus < needClusBack+curClusNum)//一般大小文件，需要扩展他分配的簇。比如以前有2簇，需要3簇，那么还需要分配一簇。
		{

			needClu = needClusBack;
			FstClusUse = FstClus;
			while (findNextFat(FstClusUse) != 0xFFF && findNextFat(FstClusUse) != 0)//寻找文件已经分配的最后一个簇。
			{
				FstClusUse = findNextFat(FstClusUse);
			}
			while (needClu > 1)//（必定会进入），这个循环用来分配新fat
			{
				nextClus = findEmptyFat();
				writeFat(FstClusUse, nextClus);//将自己从0XFFF更新成下一簇。
				needClu--;
				FstClusUse = nextClus;
				continue;
			}
			writeFat(FstClusUse, 0xffff);//将末尾簇写成0xfff
		}




	}

	else {
		cout << "[debug]需要的簇计算出错，请调试。needClu:" << needClu << endl;
		return -1;
	}
	dataBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (curClusNum + FstClus - 2) * BytsPerSec;//设定首簇偏移值
	dataOffset = dataBase + curClusOffset;//保存在dataOffset中
	//------------------下面是更新RootENTRY
	fillTime(FileInfo_ptr->DIR_WrtDate, FileInfo_ptr->DIR_WrtTime);
	if (FileInfo_ptr->DIR_FileSize >= (lenOfBuffer + offset))//如果写入的长度+偏移<=原文件大小,那么文件大小不变。
	{
	}
	else {
		FileInfo_ptr->DIR_FileSize = offset + lenOfBuffer;
	}
	updateRootEntry(dwHandles[dwHandle].parentClus, FileInfo_ptr);
	//------------------下面是写入。
	int hasWritten = 0;
	if (lenOfBuffer < 512 - curClusOffset)//很小，就写一个簇。簇中剩下的足够填入。
	{
		SetHeaderOffset(dataOffset, NULL, FILE_BEGIN);
		WriteToDisk(cBuffer, lenOfBuffer, NULL);
		return lenOfBuffer;
	}
	do {//要写多个簇
		dataOffset = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (curClusNum + FstClus - 2) * BytsPerSec + curClusOffset;//dataOffset更新。
		//第一次写以后curClusOffset必定会变0。
		SetHeaderOffset(dataOffset, NULL, FILE_BEGIN);

		if (hasWritten + 512-curClusOffset <= lenOfBuffer)//第一次写入，根据当前的偏移写。如果剩下的空间已经足够写入，那么么写入即可
		{
			WriteToDisk(&cBuffer[hasWritten], 512 - curClusOffset, NULL);
		}
		else {
			WriteToDisk(&cBuffer[hasWritten], lenOfBuffer - hasWritten, NULL);//写入剩下的字符。
		}
		hasWritten += 512 - curClusOffset;
		curClusOffset = 0;//写了第一次以后，它就变成0，以后每次都写512字节。
	} while ((FstClus = findNextFat(FstClus)) != 0xfff && FstClus != 0);
		return lenOfBuffer;


	//int FstClus = dwHandles[dwHandle].fileInfo.DIR_FstClus;
	//int dataBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClus - 2)*BytsPerSec;
	//cout << "[debug] " << dwHandles[dwHandle].fileInfo.DIR_Name << " locates at " << dataBase << " (10 hexadecimal) 0x" << DecIntToHexStr(dataBase) << "H (16 hexadecimal) in file!" << endl;
	////获取了！要写的地址
	////根目录区的起始地址为（1+9+9）*512=0x2600h，
	////假定根目录区大小为0x1c00（软盘缺省值224*32Bytes），则数据区起始偏移为0x2600+0x1c00=0x4200。
	//SetHeaderOffset(dataBase, NULL, FILE_BEGIN);
	//if (WriteToDisk(pBuffer, dwBytesToWrite, NULL))
	//{
	//	dataBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;//写回fat
	//	dwHandles[dwHandle].fileInfo.DIR_FileSize = dwBytesToWrite;
	//	struct RootEntry* FileInfo_ptr = &dwHandles[dwHandle].fileInfo;
	//	updateRootEntry(FileInfo_ptr);
	//	return dwBytesToWrite;
	//}
	return -1;
}
DWORD MyReadFile(DWORD dwHandle, LPVOID pBuffer, DWORD dwBytesToRead)
{
	struct RootEntry* FileInfo_ptr = &dwHandles[dwHandle].fileInfo;
	//TODO:根据header去读
	u16 FstClus = FileInfo_ptr->DIR_FstClus;
	LONG offset = dwHandles[dwHandle].offset;//获取记录的当前偏移
	int curClusNum = offset / BytsPerSec; // 当前指针在第几个扇区
	int curClusOffset = offset % BytsPerSec; // 当前在扇区内偏移
	while (curClusNum) {
		if (findNextFat(FstClus) == 0xFFF) {
			break;
		}
		FstClus = findNextFat(FstClus);
		curClusNum--;
	}// 获取当前指针所指扇区


	//memset(cBuffer, 0, lenOfBuffer);
	int hasRead = 0;
	int base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClus - 2)*BytsPerSec;
	int dataOffset = base + curClusOffset;
	int lenOfBuffer = dwBytesToRead;// 缓冲区待读入长度
	if (FileInfo_ptr->DIR_FileSize - offset < lenOfBuffer) {
		lenOfBuffer = FileInfo_ptr->DIR_FileSize - offset;
	}
	char* cBuffer = (char*)malloc(sizeof(u8)*lenOfBuffer);// 创建一个缓冲区
	memset(cBuffer, 0, lenOfBuffer);
	cout << "[output]" << dwHandles[dwHandle].fileInfo.DIR_Name << "'s FstClus value is:" << FstClus << ";postion (16位): is " << hex << base << endl;
	int count = BytsPerSec*SecPerClus / 512;//每一个簇需要读取的次数
	char byte[512];
	do {
		base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClus - 2)*BytsPerSec;
		for (int i = 0; i < count; i++)
		{
			SetHeaderOffset(base, NULL, FILE_BEGIN);
			ReadFromDisk(byte, 512, NULL);
		}
		//cout << byte ;
		FstClus = findNextFat(FstClus);
		if (hasRead > lenOfBuffer)//如果加上下一个512字节已经比我们要求的字节多了
		{
			memcpy(&cBuffer[hasRead], byte, lenOfBuffer - hasRead + 512);
			break;
		}
		memcpy(&cBuffer[hasRead], byte, 512);
		hasRead += 512;
	} while (FstClus != 0xfff && FstClus != 0);
	//cout << endl;
	cout << "[output]File contend:" << endl << cBuffer << endl;
	memcpy(pBuffer, cBuffer, 512);



	/*int FstClus = dwHandles[dwHandle].DIR_FstClus;
	int dataBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClus - 2)*BytsPerSec;
	cout << "[debug] " << dwHandles[dwHandle].DIR_Name << " locates at " << dataBase << " (10 hexadecimal) 0x" << DecIntToHexStr(dataBase) << "H (16 hexadecimal) in file!" << endl;
	//获取了！要写的地址
	//根目录区的起始地址为（1+9+9）*512=0x2600h，
	//假定根目录区大小为0x1c00（软盘缺省值224*32Bytes），则数据区起始偏移为0x2600+0x1c00=0x4200。
	SetHeaderOffset(dataBase, NULL, FILE_BEGIN);
	if (ReadFromDisk(pBuffer, dwHandles[dwHandle].DIR_FileSize, NULL) != 0)
	{
		u8 ptr[15];
		//u8 *ptr= (u8*)malloc(dwBytesToRead);
//TODO:想办法输出
		//hex_to_str(ptr, pBuffer, sizeof(pBuffer));

		///printf("[output]Read file:%s\ncontents: %s \n", dwHandles[dwHandle].DIR_Name, get_raw_string(pBuffer));
		return dwBytesToRead;
	}
	*/
	return -1;
}
BOOL MySetFilePointer(DWORD dwFileHandle, int nOffset, DWORD dwMoveMethod) {
	FileHandle *hd = &dwHandles[dwFileHandle];
	//if (hd == NULL ) return FALSE; // 句柄不存在
	LONG curOffset = nOffset + hd->offset; // current模式下偏移后的位置
	u16 currentClus = hd->fileInfo.DIR_FstClus; // 首簇
	int fileSize = hd->fileInfo.DIR_FileSize; // 文件大小
	switch (dwMoveMethod) {
	case MY_FILE_BEGIN:
		if (nOffset < 0) {
			hd->offset = 0; // 小于0，置为0
		}
		else if (nOffset > fileSize) {
			hd->offset = fileSize;
		}
		else {
			hd->offset = nOffset;
		}
		break;
	case MY_FILE_CURRENT:
		if (curOffset < 0) {
			hd->offset = 0;
		}
		else if (curOffset > fileSize) {
			hd->offset = fileSize;
		}
		else {
			hd->offset = curOffset;
		}
		break;
	case MY_FILE_END:
		if (nOffset > 0) {
			hd->offset = fileSize;
		}
		else if (nOffset < -fileSize) {
			hd->offset = 0;
		}
		else {
			hd->offset = fileSize + nOffset;
		}
		break;
	}
	return TRUE;
}
