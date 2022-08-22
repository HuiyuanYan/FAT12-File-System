#include "shell.h"
char shellPath[256] = "/";
char *fgets_no_endline(char *str, int n, FILE *stream)
{
    char *ret = fgets(str, n, stream);
    unsigned int l;
    if (ret)
    {
        l = strlen(str) - 1;
        if (str[l] == '\n')
            str[l] = 0;
    }
    return ret;
}
void GetTime(uint16_t wrtDate, uint16_t wrtTime)
{
    int tm_year, tm_mon, tm_day;
    int tm_hour, tm_min;

    tm_year = ((wrtDate & 0xFE00) >> 9) + 1980;
    tm_mon = (wrtDate & 0x1E0) >> 5;
    tm_day = wrtDate & 0x1F;

    tm_hour = (wrtTime & 0xF800) >> 11;
    tm_min = (wrtTime & 0x07E0) >> 5;

    printf("%d-%d-%d ", tm_year, tm_mon, tm_day);
    printf("%d:%d\t", tm_hour, tm_min);
}

//以'/'作为分隔符，找到path的上级目录，并赋值给fatherPath
void findFatherPath(char *path, char *fatherPath)
{
    char *lastR = strrchr(path, '/');
    int lastIndex = strlen(path) - strlen(lastR) + 1;
    strncpy(fatherPath, path, lastIndex);
}

//打印文件目录项的详细信息
void PrintDir(FileDescriptor *fd)
{
    if (fd->DIR_Attr == REGULAR_TYPE)
        printf("\033[1;32m%s\033[0m\t", (char *)fd->DIR_Name);
    else if (fd->DIR_Attr == DIRECTORY_TYPE)
        printf("\033[42;1;30m%s\033[0m\t", (char *)fd->DIR_Name);
    printf("%s\t", (char *)fd->DIR_Type);
    if (fd->DIR_Attr == REGULAR_TYPE)
        printf("<REGULAR>\t");
    else if (fd->DIR_Attr == DIRECTORY_TYPE)
        printf("<DIRECTORY>\t");
    printf("%d\t", fd->DIR_FileSize);
    GetTime(fd->WrtDate, fd->WrtTime);
    printf("\n");
}
// ls命令实现，只支持一些简单的参数和格式
int HandleLs(char cmd[][30], int argc)
{
    int optionA = 0, optionL = 0, optionH = 0;
    int fileExist = 0; //用于标识ls命令目标文件是否已经确定
    char filePath[50] = {0};
    for (int i = 1; i < argc; i++)
    {
        if (cmd[i][0] != '-')
        {
            //如果识别到文件路径
            if (fileExist == 0)
            {
                strcpy(filePath, cmd[i]);
                fileExist = 1;
            }
            else
            {
                printf("Only one file can be identified at a time.\n");
                return -1;
            }
        }
        else
        {
            if (strcmp(cmd[i], "-a") == 0)
                optionA = 1;
            else if (strcmp(cmd[i], "-l") == 0)
                optionL = 1;
            else if (strcmp(cmd[i], "--help") == 0)
            {
                printf("Usage: ls [OPTION]... [FILE]...\n"
                       "List information about the FILEs (the current directory by default).\n"
                       "Mandatory arguments to long options are mandatory for short options too.\n"
                       "-a                  do not ignore entries starting with .\n"
                       "-l                  use a long listing format.\n");
                return 0;
            }
            else
            {
                printf("ls: invalid option \'%s\'\n", cmd[i]);
                printf("Try 'ls --help' for more information.\n");
                return -1;
            }
        }
        // printf("%s\n", cmd[i]);
    }

    //合成路径
    char tmpPath[256] = {0};
    strcpy(tmpPath, shellPath);
    strcat(tmpPath, filePath);
    FileDescriptor fp;
    if (ReadFp(tmpPath, &fp) == -1)
        return -1;
    if (fp.DIR_Attr == REGULAR_TYPE)
        return -1;
    int bufSz = ((fp.DIR_FileSize + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
    char *buf = (char *)calloc(bufSz, sizeof(char));
    ReadData(&fp, buf);
    FileDescriptor *tmpFp;
    int listJudge;          //用于判断某文件是否需要列出
    char tmpName[11] = {0}; //用于输出文件名字
    for (int i = 0; i < bufSz; i += sizeof(FileDescriptor))
    {
        listJudge = 1;
        tmpFp = (FileDescriptor *)(buf + i);
        if (tmpFp->DIR_Name[0] != 0)
        {
            if (strcmp((char *)tmpFp->DIR_Name, ".") == 0 || strcmp((char *)tmpFp->DIR_Name, "..") == 0)
            {
                if (optionA == 0)

                    listJudge = 0;
            }
            if (listJudge == 1)
            {
                if (optionL == 1)
                    PrintDir(tmpFp);
                else
                {
                    strcpy(tmpName, (char *)tmpFp->DIR_Name);
                    if (tmpFp->DIR_Type[0] != 0)
                    {
                        strcat(tmpName, ".");
                        strcat(tmpName, (char *)tmpFp->DIR_Type);
                    }
                    if (tmpFp->DIR_Attr == REGULAR_TYPE)
                        printf("\033[1;32m%s\033[0m\t", tmpName);
                    else if (tmpFp->DIR_Attr == DIRECTORY_TYPE)
                        printf("\033[42;1;30m%s\033[0m\t", tmpName);
                }
            }
        }
    }
    printf("\n");
    return 0;
}

// pwd命令，只支持无参数，显示当前shell路径
int HandlePwd()
{
    printf("%s\n", shellPath);
    return 0;
}

// cd命令，只支持切换当前文件夹下的目录
int HandleCd(char *path)
{
    if (strcmp(path, ".") == 0)
        return 0;
    else if (strcmp(path, "..") == 0)
    {
        //切换至父级目录
        char faPath[256] = {0};
        findFatherPath(shellPath, faPath);
        strcpy(shellPath, faPath);
        return 0;
    }
    char tmpPath[256] = {0};
    strcpy(tmpPath, shellPath);
    strcat(tmpPath, path);
    FileDescriptor fp;
    if (ReadFp(tmpPath, &fp) == -1)
        return -1;
    if (fp.DIR_Attr != DIRECTORY_TYPE)
    {
        printf("Not a directory.\n");
        return -1;
    }
    strcpy(shellPath, tmpPath);
    return 0;
}

//魔改cat命令，用于把常规文件的内容输出到屏幕上
int HandleCat(char *path)
{
    char tmpPath[256];
    strcpy(tmpPath, shellPath);
    strcat(tmpPath, path);
    FileDescriptor fp;

    if (ReadFp(tmpPath, &fp) == -1)
    {
        return -1;
    }
    if (fp.DIR_Attr == DIRECTORY_TYPE)
    {
        printf("Is a directory.\n");
        return -1;
    }
    int bufSz = ((fp.DIR_FileSize + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
    char *buf = (char *)calloc(bufSz, sizeof(char));
    ReadData(&fp, buf);
    printf("%s", buf);
    return 0;
}

// mkdir命令的简易实现，创建一个新文件夹
int HandleMkdir(char *fileName)
{
    int nameSz = strlen(fileName);
    // step0:检查名字的合法性(不能为空，不能含有'.','/','-'等)
    if (nameSz <= 0)
    {
        printf("missing operand\n"
               "Try 'mkdir --help' for more information.\n");
        return -1;
    }
    if (strcmp(fileName, "--help") == 0)
    {
        printf("Create the DIRECTORY(ies), if they do not already exist.\n");
        return 0;
    }
    for (int i = 0; i < nameSz; i++)
    {
        if (fileName[i] == '.' || fileName[i] == '/' || fileName[i] == '-')
        {
            printf("illegal dir name");
            return -1;
        }
    }

    // step1:调用api创建文件
    int ret = CreateFile(shellPath, fileName, "", DIRECTORY_TYPE, 512);
    if (ret == 0)
        return 0;
    if (ret == -2)
    {
        printf("directory \"%s\" already exists\n", fileName);
        return -1;
    }
}

// touch命令简单实现：支持对普通文件的修改时间和创建
int HandleTouch(char *fileName)
{
    int nameSz = strlen(fileName);
    // step0:检查名字的合法性(不能为空，不能含有'.','/','-'等)
    if (nameSz <= 0)
    {
        printf("missing operand\n"
               "Try 'touch --help' for more information.\n");
        return -1;
    }
    if (strcmp(fileName, "--help") == 0)
    {
        printf("Usage: touch FILE\n"
               "Update the access and modification times of the FILE (only one at a time) to the current time.\n"
               "Create the FILE if it dosen't exist.\n");
        return 0;
    }
    if (fileName[0] == '.') //开头不能为'.'
    {
        printf("illegal file name\n");
        return -1;
    }
    for (int i = 0; i < nameSz; i++)
    {
        if (fileName[i] == '/' || fileName[i] == '-')
        {
            printf("illegal file name\n");
            return -1;
        }
    }

    //检查是否文件已经创建
    FileDescriptor fatherFp;
    ReadFp(shellPath, &fatherFp);
    int bufSz = ((fatherFp.DIR_FileSize + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
    char *buf = (char *)calloc(bufSz, sizeof(char));
    ReadData(&fatherFp, buf);
    FileDescriptor *tmpFp;
    char tmpName[20] = {0};
    for (int i = 0; i < bufSz; i += sizeof(FileDescriptor))
    {
        tmpFp = (FileDescriptor *)(buf + i);
        if (tmpFp->DIR_Name[0] != 0)
        {
            if (tmpFp->DIR_Attr == DIRECTORY_TYPE)
                continue; //我们目前不处理目录类型
            else if (tmpFp->DIR_Attr == REGULAR_TYPE)
            {
                strcpy(tmpName, (char *)tmpFp->DIR_Name);
                if (tmpFp->DIR_Type[0] != 0)
                {
                    strcat(tmpName, ".");
                    strcat(tmpName, (char *)tmpFp->DIR_Type);
                }
                if (strcmp(fileName, tmpName) == 0)
                {
                    //说明已经有了该常规文件，那么修改它的时间并写回即可
                    printf("TOUCH FILENAME=%s\n", fileName);
                    SetTime(tmpFp);
                    WriteData(buf, bufSz, &fatherFp);
                    return 0;
                }
            }
        }
    }

    //如果没有，则新建
    char *Type = strrchr(fileName, '.');
    if (Type == NULL)
    {
        CreateFile(shellPath, fileName, "", REGULAR_TYPE, 0);
    }
    else
    {
        char fileType[3] = {0};
        char realName[8] = {0};
        strncpy(realName, fileName, nameSz - strlen(Type));
        strncpy(fileType, Type + 1, strlen(Type) - 1);
        CreateFile(shellPath, realName, fileType, REGULAR_TYPE, 0);
    }
    return 0;
}

// rm命令:删除常规文件
int HandleRm(char *fileName)
{
    int nameSz = strlen(fileName);
    // step0:检查名字的合法性(不能为空，不能含有'.','/','-'等)
    if (nameSz <= 0)
    {
        printf("missing operand\n"
               "Try 'rm --help' for more information.\n");
        return -1;
    }
    if (strcmp(fileName, "--help") == 0)
    {
        printf("Usage: rm FILE\n"
               "Remove the FILE.\n");
        return 0;
    }
    if (fileName[0] == '.') //开头不能为'.'
    {
        printf("illegal file name\n");
        return -1;
    }
    for (int i = 0; i < nameSz; i++)
    {
        if (fileName[i] == '/' || fileName[i] == '-')
        {
            printf("illegal file name\n");
            return -1;
        }
    }

    //检查是否文件已经创建
    FileDescriptor fatherFp;
    ReadFp(shellPath, &fatherFp);
    int bufSz = ((fatherFp.DIR_FileSize + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
    char *buf = (char *)calloc(bufSz, sizeof(char));
    ReadData(&fatherFp, buf);
    FileDescriptor *tmpFp;
    char tmpName[20] = {0};
    for (int i = 0; i < bufSz; i += sizeof(FileDescriptor))
    {
        tmpFp = (FileDescriptor *)(buf + i);
        if (tmpFp->DIR_Name[0] != 0)
        {
            strcpy(tmpName, (char *)tmpFp->DIR_Name);
            if (tmpFp->DIR_Type[0] != 0)
            {
                strcat(tmpName, ".");
                strcat(tmpName, (char *)tmpFp->DIR_Type);
            }
            if (strcmp(fileName, tmpName) == 0)
            {
                if (tmpFp->DIR_Attr == DIRECTORY_TYPE)
                {
                    printf("Is a directory.\n");
                    return -1;
                }
                else if (tmpFp->DIR_Attr == REGULAR_TYPE)
                {
                    RemoveFile(shellPath, fileName);
                    return 0;
                }
            }
        }
    }
    printf("No such file or directory\n");
    return -1;
}

// rmdir命令:删除目录文件
int HandleRmDir(char *fileName)
{
    int nameSz = strlen(fileName);
    // step0:检查名字的合法性(不能为空，不能含有'.','/','-'等)
    if (nameSz <= 0)
    {
        printf("missing operand\n"
               "Try 'rmdir --help' for more information.\n");
        return -1;
    }
    if (strcmp(fileName, "--help") == 0)
    {
        printf("Usage: rmdir DIRECTORY\n"
               "Remove the DIRECTORY.\n");
        return 0;
    }
    if (fileName[0] == '.') //开头不能为'.'
    {
        printf("illegal file name\n");
        return -1;
    }
    for (int i = 0; i < nameSz; i++)
    {
        if (fileName[i] == '/' || fileName[i] == '-')
        {
            printf("illegal file name\n");
            return -1;
        }
    }

    //检查是否文件已经创建
    FileDescriptor fatherFp;
    ReadFp(shellPath, &fatherFp);
    int bufSz = ((fatherFp.DIR_FileSize + SECTOR_SIZE - 1) / SECTOR_SIZE) * SECTOR_SIZE;
    char *buf = (char *)calloc(bufSz, sizeof(char));
    ReadData(&fatherFp, buf);
    FileDescriptor *tmpFp;
    char tmpName[20] = {0};
    for (int i = 0; i < bufSz; i += sizeof(FileDescriptor))
    {
        tmpFp = (FileDescriptor *)(buf + i);
        if (tmpFp->DIR_Name[0] != 0)
        {
            strcpy(tmpName, (char *)tmpFp->DIR_Name);
            if (tmpFp->DIR_Type[0] != 0)
            {
                strcat(tmpName, ".");
                strcat(tmpName, (char *)tmpFp->DIR_Type);
            }
            if (strcmp(fileName, tmpName) == 0)
            {
                if (tmpFp->DIR_Attr == DIRECTORY_TYPE)
                {
                    RemoveFile(shellPath, fileName);
                    return -1;
                }
                else if (tmpFp->DIR_Attr == REGULAR_TYPE)
                {
                    printf("Is a file.\n");
                    return 0;
                }
            }
        }
    }
    printf("No such file or directory\n");
    return -1;
}

// copy命令，把位于主机的某文件内容复制到FAT12文件系统中的文件中去，如果目的文件不存在那么会先创建目的文件
int HandleCopy(char *srcFile, char *destFile)
{
    if (strlen(srcFile) <= 0 || strlen(destFile) <= 0)
    {
        printf("missing operand\n"
               "Try 'copy --help' for more information.\n");
        return -1;
    }
    if (strcmp(srcFile, "--help") == 0)
    {
        printf("Usage: copy SRCFILE DSTFILE\n"
               "Copy the SRCFILE on the host to the DSTFILE in FAT12.\n"
               "Create the DSTFILE first if it dosen't exist.\n");
    }

    FILE *srcFp;
    int srcFileSz;               //保存文件字符数
    char *buf;                   //将文件内容读到此指针位置
    srcFp = fopen(srcFile, "r"); //打开文件
    if (srcFp == NULL)
    {
        printf("can not open the file \'%s\'\n", srcFile);
        return -1;
    }
    fseek(srcFp, 0, SEEK_END);                      //将文件指针指向该文件的最后
    srcFileSz = ftell(srcFp);                       //根据指针位置，此时可以算出文件的字符数
    buf = (char *)malloc(srcFileSz * sizeof(char)); //根据文件大小为tmp动态分配空间
    memset(buf, 0, srcFileSz);                      //初始化此控件内容，否则可能会有乱码
    fseek(srcFp, 0, SEEK_SET);                      //重新将指针指向文件首部
    fread(buf, sizeof(char), srcFileSz, srcFp);     //开始读取整个文件
    fclose(srcFp);
    //将内容写入FAT12的目标文件中
    FileDescriptor dstFp;
    char fullPath[20] = {0};
    strcpy(fullPath, shellPath);
    strcat(fullPath, destFile);

    if (ReadFp(fullPath, &dstFp) < 0)
    {
        //说明目的文件不存在，那么先创建
        int nameSz = strlen(destFile);
        char *Type = strrchr(destFile, '.');
        if (Type == NULL)
        {
            CreateFile(shellPath, destFile, "", REGULAR_TYPE, srcFileSz);
        }
        else
        {
            char fileType[3] = {0};
            char realName[8] = {0};
            strncpy(realName, destFile, nameSz - strlen(Type));
            strncpy(fileType, Type + 1, strlen(Type) - 1);
            CreateFile(shellPath, realName, fileType, REGULAR_TYPE, srcFileSz);
        }

        //再写入
        WriteFile(fullPath, buf, srcFileSz);
    }
    else
    {
        //如果存在则直接写入
        WriteFile(fullPath, buf, srcFileSz);
    }

    return 0;
}

//魔改命令，load用于加载主机bin文件到disk之中
int HandleLoad(char *filePath)
{
    if (strlen(filePath) <= 0)
    {
        printf("missing operand\n"
               "Try 'Load --help' for more information.\n");
        return -1;
    }

    if (strcmp(filePath, "--help") == 0)
    {
        printf("Usage: load FILE\n"
               "Load the FAT12 contents from the host file.\n"
               "The FILE must be in binary format.\n");
        return 0;
    }
    char *tmp = strrchr(filePath, '.');
    if (tmp == NULL || strcmp(tmp, ".bin") != 0)
    {
        printf("illegal file type\n");
        return -1;
    }

    FILE *inFile = NULL;
    if (!(inFile = fopen(filePath, "rb")))
    {
        printf("No such file or directory.\n");
        return -1;
    }

    fread(&disk, sizeof(disk), 1, inFile);
    fclose(inFile);
    printf("FAT12 contents have been successfully loaded from file '%s'.\n", filePath);
}

//魔改命令，save用于将disk内容保存到当前文件夹的filePath之中
int HandleSave(char *filePath)
{
    if (strlen(filePath) <= 0)
    {
        printf("missing operand\n"
               "Try 'Save --help' for more information.\n");
        return -1;
    }

    if (strcmp(filePath, "--help") == 0)
    {
        printf("Usage: save FILE\n"
               "Save the FAT12 contents to the host file.\n"
               "The FILE must be in binary format.\n");
        return 0;
    }
    char *tmp = strrchr(filePath, '.');
    if (tmp == NULL || strcmp(tmp, ".bin") != 0)
    {
        printf("illegal file type\n");
        return -1;
    }

    //打开（如果没有则新创建）该文件
    FILE *outFile = NULL;
    if (!(outFile = fopen(filePath, "wb")))
    {
        return -1;
    }
    fwrite(&disk, sizeof(Disk), 1, outFile);
    fclose(outFile);
    printf("The contents of FAT12 have been successfully saved to '%s'\n.", filePath);
    return 0;
}
void ExecShell()
{
    printf("\033[1;35mWelcome to the Shell.\n"
           "Author: yhy\n"
           "File System: FAT12\n\033[0m");
    char buff[50];
    char cmd[10][30];
    int i;
    char *subStr;
    while (1)
    {
        memset(buff, 0, 50);
        //清空cmd
        for (int i = 0; i < 10; i++)
            memset(cmd[i], 0, 30);
        i = 0;
        //打印行首提示符
        printf("\033[1;32m%s@%s\033[0m:\033[1;36m%s\033[0m$ ", disk.MBR.BS_OEMName, disk.MBR.BS_VolLab, shellPath);
        //从标准输入中获取命令
        fgets_no_endline(buff, 50, stdin);

        //解析命令
        subStr = strtok(buff, " ");
        while (subStr != NULL)
        {
            strcpy(cmd[i], subStr);
            // printf("cmd[%d]=%s,sz=%ld\n", i, cmd[i], strlen(cmd[i]));
            subStr = strtok(NULL, " ");
            i++;
        }
        if (strcmp(cmd[0], "exit") == 0)
        {
            return;
        }
        else if (strcmp(cmd[0], "ls") == 0)
        {
            // printf("123456\n");
            HandleLs(cmd, i);
        }
        else if (strcmp(cmd[0], "pwd") == 0)
        {
            HandlePwd();
        }
        else if (strcmp(cmd[0], "cd") == 0)
        {
            HandleCd(cmd[1]);
        }
        else if (strcmp(cmd[0], "cat") == 0)
        {
            HandleCat(cmd[1]);
        }
        else if (strcmp(cmd[0], "mkdir") == 0)
        {
            HandleMkdir(cmd[1]);
        }
        else if (strcmp(cmd[0], "touch") == 0)
        {
            HandleTouch(cmd[1]);
        }
        else if (strcmp(cmd[0], "rm") == 0)
        {
            HandleRm(cmd[1]);
        }
        else if (strcmp(cmd[0], "rmdir") == 0)
        {
            HandleRmDir(cmd[1]);
        }
        else if (strcmp(cmd[0], "copy") == 0)
        {
            HandleCopy(cmd[1], cmd[2]);
        }
        else if (strcmp(cmd[0], "load") == 0)
        {
            HandleLoad(cmd[1]);
        }
        else if (strcmp(cmd[0], "save") == 0)
        {
            HandleSave(cmd[1]);
        }
        else if (strcmp(cmd[0], "help") == 0)
        {
            printf("Commands Supported(Limited):\n"
                   "cat\tcd\tcopy\tload\tls\tmkdir\tpwd\tsave\trm\trmdir\ttouch\n"
                   "Type [Command] --help for help.\n"
                   "Type \'exit\' to quit shell.\n");
        }
        else
        {
            printf("%s: command not found.\n"
                   "Type \'help\' for help.\n",
                   cmd[0]);
        }
    }
}
