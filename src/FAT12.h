#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

//规定磁盘信息
#define SECTOR_SIZE 512
#define ROOT_DICT_NUM 224
#define DATA_SECTOR_NUM 2880
#define FAT_SECTOR_NUM 9
#define CLUS_NUM 384
#define ROOT_DICT_NUM 224
//规定文件类型
#define REGULAR_TYPE 0x20
#define DIRECTORY_TYPE 0x10

//当前文件路径

#define B2int(num, a, b, c) \
    num = a << 16 | b << 8 | c

#define int2B(a, b, c, num) \
    a = num | 0x0F00;       \
    b = num | 0x00F0;       \
    c = num | 0x000F

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef char Sector[512];
struct MBR_Header;
struct File_Descriptor;
typedef struct MBR_Header
{
    char BS_jmpBOOT[3];
    char BS_OEMName[8];
    // 每个扇区字节数 512
    uint16_t BPB_BytesPerSec;
    // 每簇扇区数 1
    uint8_t BPB_SecPerClus;
    // boot引导占扇区数 1
    uint16_t BPB_ResvdSecCnt;
    //一共有几个FAT表 2
    uint8_t BPB_NumFATs;
    //根目录文件最大数  0xe0 = 224
    uint16_t BPB_RootEntCnt;
    //扇区总数  0xb40 = 2880
    uint16_t BPB_TotSec16;
    //介质描述  0xf0
    uint8_t BPB_Media;
    //每个FAT表占扇区数 9
    uint16_t BPB_FATSz16;
    // 每个磁道占扇区数 0x12
    uint16_t BPB_SecPerTrk;
    // 磁头数   0x2
    uint16_t BPB_NumHeads;
    // 隐藏扇区数 0
    uint32_t BPB_HiddSec;
    // 如果BPB_TotSec16=0,则由这里给出扇区数 0
    uint32_t BPB_TotSec32;
    // INT 13H的驱动号 0
    uint8_t BS_DrvNum;
    //保留，未用    0
    uint8_t BS_Reserved1;
    //扩展引导标记  0x29
    uint8_t BS_BootSig;
    // 卷序列号 0
    uint32_t BS_VollD;
    // 卷标 'yxr620'
    uint8_t BS_VolLab[11];
    // 文件系统类型 'FAT12'
    uint8_t BS_FileSysType[8];
    //引导代码
    char code[448];
    //结束标志
    char end_point[2];

} __attribute__((packed)) MBR_Header;

typedef struct FAT_2Entry //由于按字节寻址，所以基本单位是字节，扩展FAT表项成为2个（2*1.5=3字节）
{
    int firstEntry : 12;
    int secondEntry : 12;
} __attribute__((packed)) FAT_2Entry;

typedef struct File_Descriptor
{
    uint8_t DIR_Name[8];   //文件名
    uint8_t DIR_Type[3];   //文件类型/扩展名
    uint8_t DIR_Attr;      //文件类型
    uint8_t Reserved[10];  //保留位
    uint16_t WrtTime;      //最后一次写入时间
    uint16_t WrtDate;      //最后一次写入日期
    uint16_t DIR_FstClus;  //此条目对应的开始簇数
    uint32_t DIR_FileSize; //文件大小
} __attribute__((packed)) File_Descriptor;

typedef struct Disk
{
    MBR_Header MBR;                               // 1个扇区
    FAT_2Entry FAT1[CLUS_NUM / 2];                // 9个扇区;
    FAT_2Entry FAT2[CLUS_NUM / 2];                // 9个扇区
    File_Descriptor rootDirectory[ROOT_DICT_NUM]; // 14个扇区
    Sector dataSector[DATA_SECTOR_NUM];           // 2880个扇区
} __attribute__((packed)) Disk;

extern Disk disk;

void InitMBR();
void InitFAT();
void MakeDirEntry(File_Descriptor *dirEntry, char dirName[8], char dirType[3], uint8_t dirAttr, uint32_t fileSz);
void SetTime(File_Descriptor *dirEntry);
int AllocSector(uint32_t sz);
int CreatRootDict(char *dictName);
void CreateDotDict(File_Descriptor *dotFp, const File_Descriptor *fp);
void CreateDDotDict(File_Descriptor *ddotFp, int fatherClus);
void WriteSector(char *buf, int idx);
void ReadSector(char *buf, int idx);
int ReadFp(char *path, File_Descriptor *fp);
int WriteFp(char *path, File_Descriptor *fp);
void WriteNewEntry(File_Descriptor *faFp, File_Descriptor *fp, int *newClus);
void ReadData(File_Descriptor *fp, char *buf);
void WriteData(char *buf, int bufSz, File_Descriptor *fp);
int ReadRootDict(char *rootName, File_Descriptor *fp);
int WriteRootDict(char *rootName, File_Descriptor *fp);
int MatchDict(char *fileName, char *buf, int bufSz, File_Descriptor *fp);
int CreateFile(char *fatherPath, char fileName[8], char fileType[3], uint8_t fileAttr, uint32_t fileSz);
void RemoveFp(File_Descriptor *fp);
int RemoveFile(char *fatherPath, char *fileName);

int ReadFile(char *path, char *buf);
int WriteFile(char *path, char *buf, int bufSz);