#pragma once
#include <stdint.h>
#include <stdbool.h>
bool nx_storage_open(void);
uint32_t nx_storage_sectors(void);
bool nx_storage_read(uint32_t lba,uint8_t *buffer);
bool nx_storage_write(uint32_t lba,const uint8_t *buffer);
bool nx_storage_sync(void);

bool nx_game_command(const uint8_t input[8],uint8_t result[2048]);
