#pragma once
#include "definitions.h"

STATUS debug_init(bool serial);
void debug_pos(int x, int y);
void debug_color(uint32 foreground, uint32 background = 0);
void debug_print(char* str, ...);
void debug_putc(char c, bool escape = true);
void debug_clear(uint32 c);
void debug_dump(void* addr, int length, bool str = 0);