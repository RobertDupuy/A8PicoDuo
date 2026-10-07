#pragma once
#include <stdint.h>
#include <stdbool.h>
typedef struct {uint32_t dr,fr;} uart_hw_t;
typedef int uart_inst_t;
#define UART_UARTFR_BUSY_BITS 8
extern bool flood;extern uint16_t input[2048];extern unsigned used,cursor;extern uart_hw_t reg;
static inline bool uart_is_readable(uart_inst_t*u){(void)u;return flood||cursor<used;}
static inline bool uart_is_writable(uart_inst_t*u){(void)u;return true;}
static inline uart_hw_t *uart_get_hw(uart_inst_t*u){(void)u;if(cursor<used)reg.dr=input[cursor++];else reg.dr=0;return &reg;}
