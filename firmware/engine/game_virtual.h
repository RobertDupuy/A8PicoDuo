#pragma once
#include <stdbool.h>
#include "ff.h"
bool game_path(const char *path);
FRESULT game_open(FIL *file);
void game_forget(FIL *file);
