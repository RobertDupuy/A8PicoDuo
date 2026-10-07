#ifndef A8DUO_CAS_FILE_H
#define A8DUO_CAS_FILE_H
#include <stdbool.h>
#include <stdint.h>
#include "ff.h"
#define CAS_MAX_RECORDS 4096u
void cas_menu_select(char *path, char *filename, int *cart_type);
void cas_menu_load_os(void);
void cas_reset(void);
bool cas_prepare(const char *path, char error[40]);
bool cas_active(void);
bool cas_matches(const char *path);
bool cas_is_file(FIL *f);
void cas_forget(FIL *f);
FRESULT cas_open(FIL *f);
FRESULT cas_read(FIL *f, void *buf, UINT n, UINT *got);
FRESULT cas_seek(FIL *f, FSIZE_t off);
FRESULT cas_close(FIL *f);
void cas_patch_os(uint8_t *dst);
#endif
