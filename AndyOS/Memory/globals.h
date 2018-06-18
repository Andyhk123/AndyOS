#pragma once

#define BLOCK_SIZE	0x1000
#define BYTES_TO_BLOCKS(x)	(1 + (((x) - 1) / BLOCK_SIZE))

void* operator new(long unsigned size);
void operator delete(void* p);
void operator delete(void* p, long unsigned size);
void* operator new[](long unsigned size);
void operator delete[](void* p);