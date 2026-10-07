/* Module B: local USB library or polled storage server, never both owners. */
#include "block_transport.h"
#include "fatfs_disk.h"
#include "tusb.h"
#include "../../game/game_engine.h"
static unsigned owner; /* 0 undecided, 1 USB, 2 UART; latch until power cycle */
static uint8_t request[NX_HEADER+NX_MAX_PAYLOAD+4],reply[NX_HEADER+NX_MAX_PAYLOAD+4];
static void serve(void){
 if(!nx_receive(uart0,request,time_us_64()+NX_TIMEOUT_US))return;
 uint8_t cmd=request[5];uint16_t len=nx_get16(request+12),status=NX_BAD_REQUEST,n=0;
 uint32_t lba=len>=4?nx_get32(request+NX_HEADER):0;
 if(owner==1)status=NX_OWNED_BY_USB;
 else if(nx_get16(request+6)!=0)status=NX_BAD_REQUEST;
 else{
  owner=2;tud_disconnect();
  if(cmd==NX_GAME_FRAME&&len==8){status=hg_command(request+NX_HEADER,reply+NX_HEADER)?NX_OK:NX_BAD_REQUEST;if(status==NX_OK)n=512;}
  else if(cmd==NX_GAME_PAGE&&len==1){status=hg_page(request[NX_HEADER],reply+NX_HEADER)?NX_OK:NX_BAD_REQUEST;if(status==NX_OK)n=512;}
  else if(cmd==NX_INFO&&len==0){status=mount_fatfs_disk()?NX_OK:NX_NO_MEDIA;if(status==NX_OK){nx_put32(reply+NX_HEADER,SECTOR_NUM);n=4;}}
  else if(cmd==NX_READ&&len==4){status=fatfs_disk_read(reply+NX_HEADER,lba,1)==0?NX_OK:NX_IO_ERROR;if(status==NX_OK)n=512;}
  else if(cmd==NX_WRITE&&len==516)status=fatfs_disk_write(request+NX_HEADER+4,lba,1)==0?NX_OK:NX_IO_ERROR;
  else if(cmd==NX_SYNC&&len==0)status=fatfs_disk_sync_ok()?NX_OK:NX_IO_ERROR;
 }
 nx_header(reply,cmd,status,nx_get32(request+8),n);(void)nx_send(uart0,reply,time_us_64()+NX_TIMEOUT_US);
}
int main(void){
 uart_init(uart0,NX_BAUD);uart_set_format(uart0,8,1,UART_PARITY_NONE);uart_set_hw_flow(uart0,false,false);uart_set_fifo_enabled(uart0,true);uart_set_irq_enables(uart0,false,false);
 gpio_set_function(0,GPIO_FUNC_UART);gpio_set_function(1,GPIO_FUNC_UART);gpio_pull_up(1);
 tud_init(0);
 while(1){if(owner!=2)tud_task();if(uart_is_readable(uart0))serve();else tight_loop_contents();}
}
void tud_mount_cb(void){if(owner==2){tud_disconnect();return;}owner=1;if(!mount_fatfs_disk())create_fatfs_disk();}
