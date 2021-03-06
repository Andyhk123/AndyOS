#include <Arch/exec.h>
#include <FS/vfs.h>
#include <Kernel/task.h>
#include <Process/dispatcher.h>
#include <Process/elf.h>
#include <Process/process.h>
#include <Process/scheduler.h>
#include <debug.h>
#include <hal.h>
#include <memory.h>
#include <string.h>

PROCESS::PROCESS(ADDRESS_SPACE addr_space)
{
    this->addr_space = addr_space;
    this->stack_ptr = STACK_BASE;
    this->filetable = new Filetable(3);
    this->pwd = VFS::GetRoot();
    this->pwd->refs += 1;

    for (int i = 0; i < SIGNAL_TABLE_SIZE; i++)
        ProcessManager::SetSignalHandler(this, i, SIG_DFL);
}

PROCESS::~PROCESS()
{
    delete this->filetable;
}

namespace ProcessManager {
PROCESS *first = 0;

pid_t pid_counter = 1;
pid_t AssignPid(PROCESS *proc)
{
    if (!proc)
        return 0;

    proc->id = pid_counter++;
    proc->gid = proc->id;
    return proc->id;
}

PROCESS *AddProcess(PROCESS *proc, PROCESS *parent)
{
    if (!proc)
        return 0;

    if (proc->id <= 0)
        AssignPid(proc);

    proc->parent = parent;
    proc->next = first;
    first = proc;

    if (parent) {
        if (parent->first_child)
            proc->next_sibling = parent->first_child;
        else
            proc->next_sibling = 0;

        parent->first_child = proc;
    }

    // Start threads
    THREAD *thread = proc->main_thread;
    while (thread) {
        Scheduler::InsertThread(thread);
        thread = thread->proc_next;
    }

    return proc;
}

void RemoveProcess(PROCESS *proc)
{}

THREAD *CreateThread(PROCESS *proc, void (*entry)(), const void (*start_routine)(), void *arg)
{
    THREAD *thread = 0;

    Scheduler::Disable();
    VMem::SwapAddressSpace(proc->addr_space);

    pflags_t flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
    int blocks = 16;

    void *virt = (void *)proc->stack_ptr;
    proc->stack_ptr = (size_t)virt + blocks * BLOCK_SIZE;

    if (proc->stack_ptr > STACK_END) {
        kprintf("Out of stack memory\n");
        sys_halt();
    }

    void *phys = PMem::AllocBlocks(blocks);
    VMem::MapPages(virt, phys, blocks, flags);

    thread = Task::CreateUserThread(entry, (void *)proc->stack_ptr);

    if (start_routine) {
        if (Exec::Arch::SetupThreadMain(thread, start_routine, arg))
            return 0;
    }

    Scheduler::Enable();

    if (AddThread(proc, thread))
        return thread;

    return 0;
}

bool AddThread(PROCESS *proc, THREAD *thread)
{
    thread->process = proc;
    thread->addr_space = proc->addr_space;

    // Insert into thread list
    if (proc->main_thread == 0) {
        proc->main_thread = thread;
        thread->proc_next = 0;
    } else {
        thread->proc_next = proc->main_thread->proc_next;
        proc->main_thread->proc_next = thread;
    }

    return true;
}

bool RemoveThread(THREAD *thread)
{
    if (!thread || !thread->process)
        return false;

    PROCESS *proc = thread->process;
    THREAD *prev = 0;
    THREAD *t = proc->main_thread;

    while (t) {
        if (t == thread) {
            if (prev)
                prev->proc_next = t->proc_next;
            else
                proc->main_thread = t->proc_next;

            break;
        }

        prev = t;
        t = t->proc_next;
    }

    return true;
}

bool StopThreads(PROCESS *proc)
{
    THREAD *thread = proc->main_thread;
    while (thread) {
        Scheduler::ExitThread(0, thread);
        thread = thread->proc_next;
    }

    return true;
}

bool FreeAllMemory(PROCESS *proc)
{
    size_t count = (USER_END - USER_BASE) / PAGE_SIZE;
    VMem::FreePages((void *)USER_BASE, count);
    return true;
}

bool CloseFiles(PROCESS *proc)
{
    for (int fd = 0; fd < FILE_TABLE_SIZE; fd++) {
        FILE *file = proc->filetable->Get(fd);
        proc->filetable->Remove(fd);

        if (file) {
            if (VFS::Close(proc->filetable, fd) == -1)
                return false;
        }
    }

    return true;
}

void Terminate(PROCESS *proc)
{
    CloseFiles(proc);
    StopThreads(proc);

    FreeAllMemory(proc);

    ADDRESS_SPACE space = VMem::GetAddressSpace();
    VMem::DestroyAddressSpace(space);
}

void *AdjustHeap(PROCESS *proc, int increment)
{
    size_t prev_end = proc->heap_end;
    size_t next_end = prev_end + increment;
    int cur_block = (prev_end - 1) / BLOCK_SIZE + 1;
    int next_block = (next_end - 1) / BLOCK_SIZE + 1;
    int blocks = next_block - cur_block;

    // kprintf("sbrk pid:%d %d=0x%x %i %i %p-%p\n", proc->id, increment, increment,
    // BYTES_TO_BLOCKS(increment), blocks, prev_end, next_end);

    if (next_end > HEAP_END)
        return 0;

    if (blocks > 0) {
        pflags_t flags = PAGE_PRESENT | PAGE_WRITE | PAGE_USER;
        size_t virt = cur_block * BLOCK_SIZE;
        void *phys = PMem::AllocBlocks(blocks);
        VMem::MapPages((void *)virt, phys, blocks, flags);

        // kprintf("%p-%p -> %p-%p  %p\n", virt, virt + blocks * BLOCK_SIZE, phys, (size_t)phys
        // + blocks * BLOCK_SIZE, next_end);
    } else if (blocks < 0) {
        // TODO
        blocks = -blocks;
        void *virt = (void *)((cur_block - blocks) * BLOCK_SIZE);
        VMem::FreePages(virt, blocks);
    }

    proc->heap_end = next_end;
    return (void *)prev_end;
}

void Exit(PROCESS *proc, int code)
{
    // proc->signal_mutex.Aquire();
    kprintf("Exit code:%d pid:%d\n", code, proc->id);

    proc->siginfo.si_pid = proc->id;
    proc->siginfo.si_status = code;
    proc->siginfo.si_code = CLD_EXITED;
    proc->state = PROCESS_STATE_ZOMBIE;

    if (proc->parent)
        HandleSignal(proc->parent, SIGCHLD);

    // proc->signal_mutex.Release();

    Dispatcher::HandleSignal(proc);
    Terminate(proc);
}

PROCESS *GetProcess(pid_t id)
{
    PROCESS *proc = first;

    while (proc) {
        if (proc->id == id)
            return proc;

        proc = proc->next;
    }

    return 0;
}

PROCESS *GetFirst()
{
    return first;
}
} // namespace ProcessManager
