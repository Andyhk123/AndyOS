#include "panic.h"
#include "drawing.h"

void Panic::KernelPanic(char* err, char* msg)
{
	_asm cli

	Drawing::DrawText(0, 0, err, 0xFF0000, 0);

	if (msg)
		Drawing::DrawText(0, 16, msg, 0xFF0000, 0);

	Drawing::Draw();

	_asm cli
	_asm hlt
}
