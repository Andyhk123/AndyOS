#include "panic.h"
#include "string.h"
#include "stdio.h"
#include "Drawing/drawing.h"
#include "Process/process.h"
#include "Process/scheduler.h"

void Panic::KernelPanic(char* err, char* msg, ...)
{
	_asm cli

	char buffer[256];
	memset(buffer, 0, 256);

	va_list args;
	va_start(args, msg);

	vsprintf(buffer, msg, args);

	int line = 0;

	Drawing::DrawText(0, line++ * 16, err, 0xFFFF0000, 0xFF000000, Drawing::gc_direct);

	if (msg)
		Drawing::DrawText(0, line++ * 16, buffer, 0xFFFF0000, 0xFF000000, Drawing::gc_direct);

	vprintf(buffer, "Thread id: %i    Page dir: %ux", Scheduler::current_thread->id, VMem::GetCurrentDir());
	Drawing::DrawText(0, line++ * 16, buffer, 0xFFFF0000, 0xFF000000, Drawing::gc_direct);

	_asm cli
	_asm hlt
}
