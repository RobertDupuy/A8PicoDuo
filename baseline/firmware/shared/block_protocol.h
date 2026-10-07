/* A8Next private prototype additions, 2026. Upstream Atari code is unchanged. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#define NX_MAGIC 0x584e3841u
#define NX_VERSION 1
#define NX_BAUD 3000000u
#define NX_MAX_PAYLOAD 516u
#define NX_HEADER 16u
#define NX_TIMEOUT_US 10000000u
#define NX_INFO 1
#define NX_READ 2
#define NX_WRITE 3
#define NX_SYNC 4
#define NX_GAME_INIT 0x40
#define NX_GAME_FRAME 0x41
#define NX_GAME_PAGE 0x42
#define NX_OK 0
#define NX_NO_MEDIA 1
#define NX_IO_ERROR 2
#define NX_BAD_REQUEST 3
#define NX_OWNED_BY_USB 4
static inline uint32_t nx_get32(const uint8_t *p) { return (uint32_t)p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static inline void nx_put32(uint8_t *p,uint32_t v) { for(int i=0;i<4;i++)p[i]=(uint8_t)(v>>(8*i)); }
static inline uint16_t nx_get16(const uint8_t *p) { return p[0]|(uint16_t)p[1]<<8; }
static inline void nx_put16(uint8_t *p,uint16_t v) { p[0]=v;p[1]=v>>8; }
static inline uint32_t nx_crc32(const uint8_t *p,size_t n) {
 uint32_t c=~0u; while(n--){c^=*p++;for(int k=0;k<8;k++)c=(c>>1)^(0xedb88320u & (0u-(c&1)));}return ~c;
}
static inline void nx_header(uint8_t *p,uint8_t cmd,uint16_t status,uint32_t id,uint16_t len) {
 nx_put32(p,NX_MAGIC);p[4]=NX_VERSION;p[5]=cmd;nx_put16(p+6,status);nx_put32(p+8,id);nx_put16(p+12,len);nx_put16(p+14,0);
}
