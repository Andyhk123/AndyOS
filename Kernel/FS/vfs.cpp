#include "vfs.h"
#include "string.h"
#include "iso.h"
#include "Lib/debug.h"
#include "Process/process.h"
#include "Process/scheduler.h"
#include "Drivers/driver.h"
#include "filesystem.h"

#define NODE_COUNT 256

namespace VFS
{
	FileSystem* first_fs;
	FileSystem* primary_fs;

	FNODE* nodes[NODE_COUNT];

	int Mount(BlockDriver* driver, char* mount_point)
	{
		FileSystem* fs = new ISO_FS(driver);
		fs->mount_point = mount_point;

		first_fs = fs;
		primary_fs = fs;
	}

	bool AddNode(FNODE* node)
	{
		for (int i = 0; i < NODE_COUNT; i++)
		{
			if (nodes[i] == 0)
			{
				nodes[i] = node;
				return true;
			}
		}

		return false;
	}

	FileSystem* GetFS(const Path* path)
	{
		return primary_fs;
	}

	FNODE* GetNode(const Path* path)
	{
		if (path->count == 0)
			return 0;

		for (int i = 0; i < NODE_COUNT; i++)
		{
			FNODE* node = nodes[i];

			if (node)
			{
				if (node->path == path)
				{
					return node;
				}
			}
		}

		FileSystem* fs = GetFS(path);
		FNODE* node = new FNODE;

		if (fs)
		{
			if (fs->GetFile(path, node) == 0)
			{
				node->io = fs;
			}
			else if (path->count >= 2)
			{
				if (strcmp(path->parts[0], "dev") == 0)
				{
					Driver* drv = DriverManager::GetDriver(path->parts[1]);

					if (drv && drv->type == DRIVER_TYPE_CHAR)
					{
						node->io = drv;
						node->type = FILE_TYPE_CHAR;
					}
				}
				else if (strcmp(path->parts[0], "proc") == 0)
				{
					if (path->count == 4)
					{
						int pid = atoi(path->parts[1]);
						int fd = atoi(path->parts[3]);

						if (pid != 0 && strcmp(path->parts[2], "fd") == 0)
						{
							node->type = FILE_TYPE_CHAR;
						}
					}
				}
			}
		}

		if (!node->io)
		{
			delete node;
			return 0;
		}

		node->parent = GetNode(path->Parent());

		if (node->parent)
		{
			node->next = node->parent->first_child;
			node->parent->first_child = node;
		}

		node->path = (Path*)path;
		AddNode(node);

		return node;
	}

	int AddFile(FILE* file)
	{
		PROCESS* proc = Scheduler::CurrentThread()->process;

		//Find first empty
		for (int i = 3; i < FILE_TABLE_SIZE; i++)
		{
			if (proc->file_table[i] == 0)
			{
				proc->file_table[i] = file;
				return i;
			}
		}

		return ERROR;
	}

	FILE* GetFile(int fd)
	{
		PROCESS* proc = Scheduler::CurrentThread()->process;

		if (fd >= FILE_TABLE_SIZE)
			return 0;

		return proc->file_table[fd];
	}

	int Open(const char* filename)
	{
		Path* path = new Path(filename);
		FNODE* node = GetNode(path);

		if (!node)
			return ERROR;

		THREAD* thread = Scheduler::CurrentThread();
		FILE* file = new FILE(node, thread);
		FileIO* io = file->node->io;

		if (!io)
			return ERROR;

		if (io->Open(file->node, file) != SUCCESS)
			return ERROR;

		return AddFile(file);
	}

	int Close(int fd)
	{
		FILE* file = GetFile(fd);
		delete file;
		return 1;
	}

	size_t Read(int fd, char* dst, size_t size)
	{
		FILE* file = GetFile(fd);

		if (!file)
			return ERROR;

		FNODE* node = file->node;

		int read = node->io->Read(file, dst, size);
		file->pos += read;
		return read;
	}

	size_t Write(int fd, const char* buf, size_t size)
	{
	}

	int Seek(int fd, long int offset, int origin)
	{
		FILE* file = GetFile(fd);

		if (!file)
			return ERROR;

		switch (origin)
		{
		case SEEK_SET:
			file->pos = offset;
			return SUCCESS;

		case SEEK_CUR:
			file->pos += offset;
			return SUCCESS;

		case SEEK_END:
			file->pos = file->node->size + offset;
			return SUCCESS;

		default:
			return ERROR;
		}
	}

	uint32 ReadFile(const char* filename, char*& buffer)
	{
		Path* path = new Path(filename);

		FILE file;
		file.node = GetNode(path);
		file.thread = Scheduler::CurrentThread();

		if (!file.node)
			return 0;

		buffer = new char[file.node->size];
		int length = file.node->io->Read(&file, buffer, file.node->size);

		return length;
	}

	STATUS Init()
	{
		memset(nodes, 0, sizeof(FNODE*) * NODE_COUNT);

		first_fs = 0;
		primary_fs = 0;

		Driver* drv = DriverManager::GetDriver();

		while (drv)
		{
			if (drv->type == DRIVER_TYPE_BLOCK)
			{
				Mount((BlockDriver*)drv, "/");
				return STATUS_SUCCESS;
			}

			drv = drv->next;
		}

		return STATUS_SUCCESS;
	}
}