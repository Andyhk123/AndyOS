#include <io.h>

uint8 mmio_read8(uint32 addr)
{
    return *((volatile uint8 *)addr);
}

uint16 mmio_read16(uint32 addr)
{
    return *((volatile uint16 *)addr);
}

uint32 mmio_read32(uint32 addr)
{
    return *((volatile uint32 *)addr);
}

void mmio_write8(uint32 addr, uint8 val)
{
    *((volatile uint8 *)addr) = val;
}

void mmio_write16(uint32 addr, uint16 val)
{
    *((volatile uint16 *)addr) = val;
}

void mmio_write32(uint32 addr, uint32 val)
{
    *((volatile uint32 *)addr) = val;
}
