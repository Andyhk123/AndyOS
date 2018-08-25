#include "test.h"
#include "panic.h"
#include "stdio.h"
#include "PCI/pci.h"
#include "Drivers/ac97.h"
#include "Drivers/e1000.h"
#include "Net/net.h"
#include "Net/tcpsocket.h"
#include "Net/udpsocket.h"
#include "Net/dns.h"
#include "Net/dhcp.h"
#include "Net/http.h"
#include "Process/scheduler.h"
#include "Drivers/serial.h"
#include "Drivers/keyboard.h"
#include "FS/vfs.h"
#include "FS/iso.h"
#include "Process/process.h"
#include "task.h"
#include "timer.h"
#include "syscall_list.h"
#include "debug.h"

namespace Test
{
	void COM_Receive()
	{
		while (1)
		{
			char c = Serial::Receive(COM_PORT1);
			debug_print("%c", c);
		}
	}

	void COM()
	{
		if (!Serial::Init(COM_PORT1, 9600))
			debug_print("COM init failed\n");

		debug_print("COM initialized\n");

		THREAD* t = Task::CreateKernelThread(COM_Receive);
		Scheduler::InsertThread(t);

		while (1)
		{
			/*KEY_PACKET key = Keyboard::GetLastKey();

			if (key.key != KEY_INVALID)
			{
				char c = key.character;
				Keyboard::DiscardLastKey();
				Serial::Transmit(COM_PORT1, c);
			}*/
		}
	}

	void File()
	{
		char* buf;
		int size = VFS::ReadFile("files/bunny.obj", buf);

		if (size)
		{
			for (int i = 0; i < size; i++)
				debug_print("%c", buf[i]);
		}
		else
		{
			debug_print("File not found");
		}

		while (1);
	}

	void GUI()
	{
		PROCESS* proc = ProcessManager::Exec("1winman");
		Scheduler::SleepThread(100, Scheduler::CurrentThread());
		ProcessManager::Exec("1term");
		//ProcessManager::Exec("1test");
		//ProcessManager::Exec("1mndlbrt");

		#ifdef __i386__
		#include "Arch/regs.h"
		while (1)
		{
			PROCESS* proc = ProcessManager::GetFirst();

			while (proc)
			{
				if (!proc->messages.IsEmpty())
				{
					MESSAGE* msg = proc->messages.Get();

					if (msg)
					{
						disable();
						VMem::SwapAddressSpace(proc->addr_space);

						if (msg->type == MESSAGE_TYPE_SIGNAL)
						{
							if (proc->signal_handler)
							{
								THREAD* thread = ProcessManager::CreateThread(proc, (void(*)())proc->signal_handler);

								REGS* regs = (REGS*)thread->stack;
								uint32* stack_ptr = (uint32*)regs->user_stack;
								*--stack_ptr = msg->param;
								regs->user_stack -= 8;
								
								Scheduler::InsertThread(thread);
							}
						}
						else
						{
							if (proc->message_handler)
							{
								THREAD* thread = ProcessManager::CreateThread(proc, (void(*)())proc->message_handler);

								REGS* regs = (REGS*)thread->stack;
								char* data_ptr = (char*)regs->user_stack - msg->size - 4;
								memcpy(data_ptr, msg->data, msg->size);

								uint32* stack_ptr = (uint32*)data_ptr;
								*--stack_ptr = msg->size;
								*--stack_ptr = (int)data_ptr;
								*--stack_ptr = msg->param;
								*--stack_ptr = msg->src_proc;
								*--stack_ptr = msg->id;
								regs->user_stack -= 28 + msg->size;

								Scheduler::InsertThread(thread);
							}
						}

						enable();
					}
				}

				proc = proc->next;
			}
		}
		#endif
	}

	void _Net()
	{
		Net::Init();

		PciDevice* dev = PCI::GetDevice(2, 0, 0);

		if (!dev)
		{
			debug_print("Network card not found\n");
			return;
		}

		debug_print("Found network card\n");

		E1000* intf = new E1000(dev);
		Timer::Sleep(1000);

		//DNS::Query(intf, "google.com");
		IPv4Address addr(0xc0, 0xa8, 0x00, 0x7b);
		DHCP::Discover(intf);

		UdpSocket* socket = UDP::CreateSocket(9876);
		
		uint8* buffer;
		while (1)
		{
			int length = socket->Receive(buffer, Net::BroadcastIPv4);
			debug_dump(buffer, length, 1);

			//char* data = "Hello";
			//socket->Send(addr, (uint8*)data, 5);
		}
		return;

		MacAddress dst(0x30, 0x9C, 0x23, 0x21, 0xEB, 0xFE);

		NetPacket* pkt = IPv4::CreatePacket(intf, addr, IP_PROTOCOL_ICMP, 0);

		if (pkt)
		{
			intf->Send(pkt);
		}
		else
		{
			debug_print("PACKET ERR\n");
		}
	}

	void Start()
	{
		GUI();
		//File();
		//_Net();
		//Audio();
		//COM();
		while (1) pause();
	}
}