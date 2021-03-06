#include <FS/pipefs.h>
#include <FS/vfs.h>
#include <debug.h>
#include <driver.h>
#include <string.h>

int PipeFS::Mount(BlockDriver *driver)
{
    VFS::AllocInode(root_dentry, 1, S_IFDIR);
    return -1;
}

int PipeFS::Open(FILE *file)
{
    return -1;
}

int PipeFS::Close(FILE *file)
{
    Pipe *pipe = GetPipe(file);

    if (!pipe)
        return -1;

    if (file->dentry->refs == 1) {
        kprintf("Closing pipe...\n");
        return pipe->Close(file);
    }

    return 0;
}

int PipeFS::Read(FILE *file, void *buf, size_t size)
{
    Pipe *pipe = GetPipe(file);

    if (!pipe)
        return -1;

    return pipe->Read(file, buf, size);
}

int PipeFS::Write(FILE *file, const void *buf, size_t size)
{
    Pipe *pipe = GetPipe(file);

    if (!pipe)
        return -1;

    return pipe->Write(file, buf, size);
}

int PipeFS::Seek(FILE *file, long offset, int origin)
{
    return -1;
}

int PipeFS::GetChildren(DENTRY *parent, const char *find_name)
{
    return -1;
}

int PipeFS::Create(DENTRY *&dentry, int flags)
{
    int ino = pipes.Count();
    char namebuf[32];
    sprintf(namebuf, "pipe:[%u]", ino);

    dentry = VFS::AllocDentry(0, namebuf);
    VFS::AllocInode(dentry, ino, S_IFIFO);
    pipes.Add(new Pipe());

    return 0;
}

Pipe *PipeFS::GetPipe(FILE *file)
{
    int ino = file->dentry->inode->ino;

    if (ino < 0 || ino >= pipes.Count())
        return 0;

    return pipes[ino];
}
