/* EXPLICIT DESTRUCTIVE RECOVERY IMAGE: erases only this module's file area.
   NEVER run in an Atari. Back up first. Normal firmware never calls this. */
#include "pico/stdlib.h"
#include "pico/bootrom.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
int main(void){
 sleep_ms(1500);
 uint32_t state=save_and_disable_interrupts();
 flash_range_erase(1024u*1024u,15u*1024u*1024u);
 restore_interrupts(state);
 reset_usb_boot(0,0);while(1)tight_loop_contents();
}
