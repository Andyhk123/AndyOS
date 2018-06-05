#include "pmem.h"
#include "memory.h"

uint32 PMem::mem_size;
uint32* PMem::mem_map;

uint32 PMem::num_blocks;
uint32 PMem::num_free;

STATUS PMem::Init(uint32 size, uint32* map)
{
	mem_size = size;
	mem_map = map;

	num_blocks = BYTES_TO_BLOCKS(mem_size);
	num_free = num_blocks;

	DeInitRegion(0, mem_size);
	return STATUS_SUCCESS;
}

void PMem::InitRegion(void* addr, uint32 size)
{
	int align = (uint32)addr / BLOCK_SIZE;
	int num = size / BLOCK_SIZE;

	for (int i = 0; i < num; i++)
	{
		UnsetBit(align++);
		num_free++;
	}

	SetBit(0);
}

void PMem::DeInitRegion(void* addr, uint32 size)
{
	int align = (uint32)addr / BLOCK_SIZE;
	int blocks = size / BLOCK_SIZE;

	for (int i = 0; i < blocks; i++)
	{
		SetBit(align++);
		num_free--;
	}

	SetBit(0);
}

void* PMem::AllocBlocks(uint32 size)
{
	uint32 frame = FirstFreeNum(size);

	if (frame == 0)
		return 0;

	for (int i = 0; i < size; i++)
	{
		SetBit(frame + i);
		num_free--;
	}

	return (void*)(frame * BLOCK_SIZE);
}

void PMem::FreeBlocks(void* addr, uint32 size)
{
	int frame = (uint32)addr / BLOCK_SIZE;

	if (frame == 0 || size == 0)
		return;

	for (int i = 0; i < size; i++)
	{
		if (GetBit(frame + i))
		{
			UnsetBit(frame + i);
			num_free++;
		}
	}
}

inline void PMem::SetBit(uint32 bit)
{
	mem_map[bit / 32] |= (1 << (bit % 32));
}

inline void PMem::UnsetBit(uint32 bit)
{
	mem_map[bit / 32] &= ~(1 << (bit % 32));
}

inline uint8 PMem::GetBit(uint32 bit)
{
	return (mem_map[bit / 32] >> (bit % 32)) & 1;
}


uint32 PMem::FirstFree()
{
	for (int i = 0; i < num_blocks; i++)
		if (!GetBit(i))
			return i;

	return 0;
}

uint32 PMem::FirstFreeNum(uint32 size)
{
	if (size == 0)
		return 0;

	if (size == 1)
		return FirstFree();

	int i, j;

	for (i = 1; i <= num_blocks - size; i++)
	{
		if (!GetBit(i))
		{
			bool found = true;

			for (j = 1; j < size; j++)
			{
				if (GetBit(i + j))
					found = false;
			}

			if (found)
				return i;
			else
				i += j;
		}
	}

	return 0;
}