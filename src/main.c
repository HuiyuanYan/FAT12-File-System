#include "FAT12.h"
#include "shell.h"
#include "FAT12.h"
int main()
{
    InitMBR();
    InitFAT();
    CreatRootDict("/");
    CreateFile("/", "usr", "", DIRECTORY_TYPE, 512);
    CreateFile("/", "NJU", "md", REGULAR_TYPE, 0);
    char txt[30] = "诚朴雄伟，励行敦行";
    char buf[512] = {0};
    WriteFile("/NJU.md", txt, 30);
    ReadFile("/NJU.md", buf);
    ExecShell();
}