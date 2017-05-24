#include "MyApi.h"
#include "util.h"
#include "DiskLib.h"
#ifdef OLDVERSION


extern FILE* fat12;//这句将会被替换
#endif // OLDVERSION
using namespace std;
//RootEntry* dwHandles[MAX_NUM] = { NULL };//文件句柄？
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
				cout << "[debug]" << pszFolderPath << "'s FstClus value is:"<< FstClusHJQ << ";postion (16位): is " << hex << base << endl;
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
	cout << "[output]"<< pszFolderName<<"'s RootEntry is ready to write in position:" << hex<<base<<" (16 hexadecimal)" << endl;
	
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
				SetHeaderOffset(base-32, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				dwHandles.push_back(*DirInfo_ptr);
				//TODO:准备添加'.' 与 '..'，“.”是当前目录的别名，“..”首簇就改指向上级目录文件的首簇。
				//添加到它分配的簇。
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (DirInfo_ptr->DIR_FstClus - 2) * BytsPerSec;//重新计算base，在自己的数据扇区写"."！
				int selfFstClus = DirInfo_ptr->DIR_FstClus;//暂存自己的首簇，用来填"."
				cout << "[debug]该文件夹的地址（有根）路径 :" << hex << base << endl;
				//TODO:把对应扇区从F6清0！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!------------------------------------------
				clearCu(DirInfo_ptr->DIR_FstClus);
				SetHeaderOffset(base, NULL, FILE_BEGIN);
				initFileInfo(DirInfo_ptr,".", 0x10, 0, DirInfo_ptr->DIR_FstClus);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				initFileInfo(DirInfo_ptr, "..", 0x10, 0, FstClusHJQ);//填上一个跟目录的首簇
				SetHeaderOffset(base+32, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				writeFat(FstClus, 0xffff);
				return true;
			}
			if (strcmp(pszFolderPath, "") == 0)//如果无根路径，在根文件目录写；并且应该在数据扇区相应的位置添加.和..才对。
			{
				
				SetHeaderOffset(base - 32, NULL, FILE_BEGIN);//写在根目录
				WriteToDisk(DirInfo_ptr, 32, NULL);
				dwHandles.push_back(*DirInfo_ptr);
				FstClusHJQ = DirInfo_ptr->DIR_FstClus;//暂存自己的首簇，用来填"."
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32;//10176
				base+=(FstClusHJQ - 2) * BytsPerSec;//写在对应的扇区！
				cout << "[debug]该文件夹的地址（无根） :" << hex<<base << endl;
				//TODO:把对应扇区从F6清0！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!------------------------------------------
				clearCu(DirInfo_ptr->DIR_FstClus);
				SetHeaderOffset(base , NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				initFileInfo(DirInfo_ptr, ".", 0x10, 0, DirInfo_ptr->DIR_FstClus);//簇就是自己！
				SetHeaderOffset(base, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				initFileInfo(DirInfo_ptr, "..", 0x10, 0, 0x00);//上一个簇就是根目录，0簇
				SetHeaderOffset(base+32, NULL, FILE_BEGIN);
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
	int FileHandle = 0;
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
				dwHandles.push_back(*DirInfo_ptr);
				FileHandle = dwHandles.size();
				SetHeaderOffset(base - 32, NULL, FILE_BEGIN);
				WriteToDisk(DirInfo_ptr, 32, NULL);
				dwHandles.push_back(*DirInfo_ptr);
				//TODO:准备添加'.' 与 '..'，“.”是当前目录的别名，“..”首簇就改指向上级目录文件的首簇。
				//添加到它分配的簇。
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (DirInfo_ptr->DIR_FstClus - 2) * BytsPerSec;//重新计算base,得到这个文件簇所在的物理地址
				int selfFstClus = DirInfo_ptr->DIR_FstClus;//暂存自己的首簇，用来填"."
				cout << "[debug]该文件的地址（有根）路径 :" << hex << base << endl;
				//TODO:把对应扇区从F6清0！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!------------------------------------------
				clearCu(DirInfo_ptr->DIR_FstClus);
				writeFat(FstClus, 0xffff);
				return FileHandle;
			}
			else if (strcmp(pszFolderPath, "") == 0)//如果无根路径，在根文件目录写；并且应该在数据扇区相应的位置添加.和..才对。
			{
				dwHandles.push_back(*DirInfo_ptr);
				FileHandle = dwHandles.size();
				SetHeaderOffset(base - 32, NULL, FILE_BEGIN);//写在根目录
				WriteToDisk(DirInfo_ptr, 32, NULL);
				dwHandles.push_back(*DirInfo_ptr);
				FstClusHJQ = DirInfo_ptr->DIR_FstClus;//暂存自己的首簇，用来填"."
				base = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32;//10176
				base += (FstClusHJQ - 2) * BytsPerSec;//写在对应的扇区！
				cout << "[debug]该文件的地址（无根） :" << hex << base << endl;
				//TODO:把对应扇区从F6清0！!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!------------------------------------------
				clearCu(DirInfo_ptr->DIR_FstClus);
				writeFat(FstClus, 0xffff);
				return FileHandle;
			}
		}
	}
	return FileHandle;



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
	int HandleBack=0;
	//printBPB();
	cout << "[output]trying to delete file named " << pszFileName;
	if (strcmp(pszFolderPath, "") != 0)
	{
		//trick:如果没有目录，第一次出现就取出。如果没有目录，那么第二次取出
		trick = !trick;
		cout << " in folder " << pszFolderPath << endl;
	}
	else cout << endl;
	//準備尋找
	int i = 0;
	vector<RootEntry>::iterator it;// = dwHandles.begin();
	for (it = dwHandles.begin(); it != dwHandles.end(); it++)
	{
		if (strcmp(pszFileName, dwHandles[i].DIR_Name) == 0)
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
		i++;

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

BOOL MyDeleteFile(char *pszFolderPath, char *pszFileName)//没必要删除数据簇的内容？?
{
	int FileHandle = 0;
	u16 FstClus = findEmptyFat();//用来填写DIR_FstClus
	u16 FstClusHJQ;//用来存储根目录的DIR_FstClus
	int base;
	cout << "[debug]findEmptyFat():" << FstClus << endl;
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
	}//---------------------------------------------没有路径，创建在根目录下。

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
	int FstClus = dwHandles[dwHandle].DIR_FstClus;
	int dataBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec + RootEntCnt * 32 + (FstClus - 2)*BytsPerSec;
	cout << "[debug] " << dwHandles[dwHandle].DIR_Name << " locates at " << dataBase << " (10 hexadecimal) 0x" << DecIntToHexStr(dataBase) << "H (16 hexadecimal) in file!" << endl;
	//获取了！要写的地址
	//根目录区的起始地址为（1+9+9）*512=0x2600h，
	//假定根目录区大小为0x1c00（软盘缺省值224*32Bytes），则数据区起始偏移为0x2600+0x1c00=0x4200。
	SetHeaderOffset(dataBase, NULL, FILE_BEGIN);
	if (WriteToDisk(pBuffer, dwBytesToWrite, NULL))
	{
		dataBase = (RsvdSecCnt + NumFATs * FATSz) * BytsPerSec;//写回fat
		dwHandles[dwHandle].DIR_FileSize = dwBytesToWrite;
		struct RootEntry* FileInfo_ptr = &dwHandles[dwHandle];
		updateRootEntry(FileInfo_ptr);


		return dwBytesToWrite;
	}
	return -1;
}
DWORD MyReadFile(DWORD dwHandle, LPVOID pBuffer, DWORD dwBytesToRead)
{
	int FstClus = dwHandles[dwHandle].DIR_FstClus;
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
	return -1;
}