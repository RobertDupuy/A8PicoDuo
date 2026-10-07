/* A8Next: synchronous block transport on the two unused GPIOs. */
#include <string.h>
#include "block_client.h"
#include "block_transport.h"
static uint32_t znext_sectors, znext_sequence;
static bool znext_uart_ready;
static uint8_t znext_tx[NX_HEADER+NX_MAX_PAYLOAD+4],znext_rx[NX_HEADER+NX_MAX_PAYLOAD+4];
static void link_init(void) {
 if(znext_uart_ready)return;
 uart_init(uart0,NX_BAUD);uart_set_format(uart0,8,1,UART_PARITY_NONE);
 uart_set_hw_flow(uart0,false,false);uart_set_fifo_enabled(uart0,true);uart_set_irq_enables(uart0,false,false);
 gpio_set_function(28,GPIO_FUNC_UART);gpio_set_function(29,GPIO_FUNC_UART);
 gpio_pull_up(29);znext_uart_ready=true;
}
static bool transact(uint8_t command,uint16_t length,uint16_t expected) {
 link_init();uint32_t id=++znext_sequence;
 nx_header(znext_tx,command,0,id,length);
 /* No automatic retries of writes: a lost reply has an indeterminate commit
    outcome. Report an I/O error rather than silently replaying filesystem I/O. */
 while(uart_is_readable(uart0))(void)uart_getc(uart0);
 uint64_t end=time_us_64()+NX_TIMEOUT_US;
 if(!nx_send(uart0,znext_tx,end))return false;
 while(time_us_64()<end){
  if(!nx_receive(uart0,znext_rx,end))return false;
  if(nx_get32(znext_rx+8)!=id)continue;
  return znext_rx[5]==command&&nx_get16(znext_rx+6)==NX_OK&&nx_get16(znext_rx+12)==expected;
 }return false;
}
bool nx_storage_open(void){
 znext_sectors=0;
 /* Remote flash-map initialization occurs on this explicit request, after boot ROM has
    already begun servicing Atari. It does not delay initial cartridge boot. */
 if(!transact(NX_INFO,0,4))return false;
 znext_sectors=nx_get32(znext_rx+NX_HEADER);
 return znext_sectors==30464u;
}
uint32_t nx_storage_sectors(void){return znext_sectors;}
bool nx_storage_read(uint32_t lba,uint8_t *b){
 if(lba>=znext_sectors)return false;nx_put32(znext_tx+NX_HEADER,lba);
 if(!transact(NX_READ,4,512))return false;memcpy(b,znext_rx+NX_HEADER,512);return true;
}
bool nx_storage_write(uint32_t lba,const uint8_t *b){
 if(lba>=znext_sectors)return false;nx_put32(znext_tx+NX_HEADER,lba);memcpy(znext_tx+NX_HEADER+4,b,512);
 return transact(NX_WRITE,516,0);
}
bool nx_storage_sync(void){return transact(NX_SYNC,0,0);}

/* Synchronous game transactions inside the existing ATR command service gap. */
static bool game_exchange(uint8_t command,uint16_t length,uint8_t result[512]) {
 link_init();uint32_t id=++znext_sequence;nx_header(znext_tx,command,0,id,length);
 while(uart_is_readable(uart0))(void)uart_getc(uart0);
 uint64_t end=time_us_64()+100000;
 if(!nx_send(uart0,znext_tx,end)||!nx_receive(uart0,znext_rx,end))return false;
 if(nx_get32(znext_rx+8)!=id||znext_rx[5]!=command||nx_get16(znext_rx+6)!=NX_OK||nx_get16(znext_rx+12)!=512)return false;
 memcpy(result,znext_rx+NX_HEADER,512);return true;
}
bool nx_game_command(const uint8_t input[8],uint8_t result[2048]){
 memcpy(znext_tx+NX_HEADER,input,8);
 if(!game_exchange(NX_GAME_FRAME,8,result))return false;
 for(unsigned p=1;p<4;p++){znext_tx[NX_HEADER]=p;if(!game_exchange(NX_GAME_PAGE,1,result+p*512))return false;}
 return true;
}
