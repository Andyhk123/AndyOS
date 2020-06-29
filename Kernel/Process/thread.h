#pragma once
#include "types.h"
#include "Memory/memory.h"

enum THREAD_STATE
{
	THREAD_STATE_INITIALIZED,
	THREAD_STATE_READY,
	THREAD_STATE_RUNNING,
	THREAD_STATE_BLOCKING,
	THREAD_STATE_TERMINATED

};

struct PROCESS;

struct THREAD
{
	size_t stack;
	size_t kernel_esp;
	int id;
	THREAD_STATE state;
	THREAD* next;
	THREAD* procNext;
	PROCESS* process;
	void* fpu_state;
	size_t sleep_until;
	ADDRESS_SPACE addr_space;
};