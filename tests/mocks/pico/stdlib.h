#pragma once
#include <stdint.h>
static inline uint64_t time_us_64(void){static uint64_t n;return ++n;}
