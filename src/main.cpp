#include "FAT12.h"
#include "shell.h"
#include <iostream>
using namespace std;

void PrintDisk(Disk &d)
{
    char *p = (char *)&d;
    char c;
    for (int i = 0; i < sizeof(d.MBR); i += 1)
    {
        cout << *(p + i);
    }
    cout << endl;
}

int main()
{
    InitMBR();
    InitFAT();
    CreatRootDict("/");
    CreateFile("/", "usr", "", DIRECTORY_TYPE, 512);
    CreateFile("/", "NJU", "md", REGULAR_TYPE, 0);
    char txt[30] = "诚朴雄伟，敦学励行";
    char buf[512] = {0};
    WriteFile("/NJU.md", txt, 30);
    ReadFile("/NJU.md", buf);
    cout << "buf=" << buf << endl;
    // cout << "Remove=" << RemoveFile("/", "test.md") << endl;
    // PrintDir(&fp);
    ExecShell();
}