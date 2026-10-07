#pragma once
#include <stdbool.h>
#include <stdint.h>
#define HG_LEVELS 30
#define HG_FRAME_SIZE 2048
/* Entire game transaction is RAM-only. The Atari polls the physical controls. */
bool hg_command(const uint8_t input[8], uint8_t first_page[512]);
bool hg_page(uint8_t page, uint8_t out[512]);
#ifdef HG_TEST
void hg_test_position(int x,int y);
unsigned hg_test_glyphs(void);
unsigned hg_test_hazards(void);
int hg_test_wall(int x,int y);
void hg_test_tick(unsigned tick);
const uint8_t *hg_test_pixels(void);
#endif
