#pragma once
#include <stdint.h>
#include <stdbool.h>
#include "flash_fs.h"
#define SECTOR_NUM 30464u
#define SECTOR_SIZE 512u
/* 15 MiB storage minus 60 KiB map and 68 KiB spare = 14.875 MiB per module. */
bool mount_fatfs_disk(void);
void create_fatfs_disk(void);
bool fatfs_is_mounted(void);
uint32_t fatfs_disk_read(uint8_t *,uint32_t,uint32_t);
uint32_t fatfs_disk_write(const uint8_t *,uint32_t,uint32_t);
void fatfs_disk_sync(void);
bool fatfs_disk_sync_ok(void);
bool duo_remote_ready(void);
