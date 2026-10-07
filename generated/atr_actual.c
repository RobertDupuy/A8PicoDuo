#include <stdint.h>
#include <string.h>
#include "ff.h"
// ATR format
#define ATR_HEADER_SIZE 16
#define ATR_SIGNATURE 0x0296
typedef struct {
  uint16_t signature;
  uint16_t pars;
  uint16_t secSize;
  uint16_t parsHigh;
  uint8_t flags;
  uint16_t protInfo;
  uint8_t unused[5];
} ATRHeader;

typedef struct {
	char path[256];
	ATRHeader atrHeader;
	int	filesize;
	FIL fil;
} MountedATR;

MountedATR mountedATRs[1] = {0};

FATFS FatFs;
int doneFatFsInit = 0;

int mount_atr(char *filename) {
	// returns 0 for success or error code
	// 1 = no media, 2 = no file, 3 = bad atr
	if (!doneFatFsInit) {
		if (f_mount(&FatFs, "", 1) != FR_OK)
			return 1;
		doneFatFsInit = 1;
	}
	MountedATR *mountedATR = &mountedATRs[0];
	if (f_open(&mountedATR->fil, filename, FA_READ|FA_WRITE) != FR_OK)
		return 2;
	UINT br;
	if (f_read(&mountedATR->fil, &mountedATR->atrHeader, ATR_HEADER_SIZE, &br) != FR_OK || br != ATR_HEADER_SIZE ||
			mountedATR->atrHeader.signature != ATR_SIGNATURE) {
		f_close(&mountedATR->fil);
		return 3;
	}
	// success
	strcpy(mountedATR->path, filename);
	mountedATR->filesize = f_size(&mountedATR->fil);
	return 0;
}

int read_atr_sector(uint16_t sector, uint8_t page, uint8_t *buf) {
	// returns 0 for success or error code
	// 1 = no ATR mounted, 2 = invalid sector
	MountedATR *mountedATR = &mountedATRs[0];
	if (!mountedATR->path[0]) return 1;
	if (sector == 0) return 2;

	int offset = ATR_HEADER_SIZE;
	// first 3 sectors are always 128 bytes
	if (sector <=3)
		offset += (sector - 1) * 128;
	else
		offset += (3 * 128) + ((sector - 4) * mountedATR->atrHeader.secSize) + (page * 128);
	// check we're not reading beyond the end of the file..
	if (offset > (mountedATR->filesize - 128)) {
		memset(buf, 0 , 128);	// return blank sector?
		return 0;
	}
	UINT br;
	if (f_lseek(&mountedATR->fil, offset) != FR_OK || f_read(&mountedATR->fil, buf, 128, &br) != FR_OK || br != 128)
		return 2;
	return 0;
}

int write_atr_sector(uint16_t sector, uint8_t page, uint8_t *buf) {
	// returns 0 for success or error code
	// 1 = no ATR mounted, 2 = write error
	MountedATR *mountedATR = &mountedATRs[0];
	if (!mountedATR->path[0]) return 1;
	if (sector == 0) return 2;

	int offset = ATR_HEADER_SIZE;
	// first 3 sectors are always 128 bytes
	if (sector <=3)
		offset += (sector - 1) * 128;
	else
		offset += (3 * 128) + ((sector - 4) * mountedATR->atrHeader.secSize) + (page * 128);
	// check we're not writing beyond the end of the file..
	if (offset > (mountedATR->filesize - 128))
		return 2;
	UINT bw;
	if (f_lseek(&mountedATR->fil, offset) != FR_OK || f_write(&mountedATR->fil, buf, 128, &bw) != FR_OK || f_sync(&mountedATR->fil) != FR_OK  || bw != 128)
		return 2;
	return 0;
}

/* CARTRIDGE/XEX HANDLING */

