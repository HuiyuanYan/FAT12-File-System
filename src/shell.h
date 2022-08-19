#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FAT12.h"
extern char shellPath[256];
void ExecShell();
int HandleLs(char cmd[][30], int argc);