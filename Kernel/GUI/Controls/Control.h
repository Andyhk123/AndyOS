#pragma once
#include "GUI/Message.h"
#include "Drawing/drawing.h"

namespace gui
{
	class Window;

	enum CONTROL_TYPE
	{
		CONTROL_TYPE_NONE,
		CONTROL_TYPE_LABEL,
		CONTROL_TYPE_BUTTON,
		CONTROL_TYPE_TEXTBOX,
		CONTROL_TYPE_RICHTEXT,
		CONTROL_TYPE_CHECKBOX,
		CONTROL_TYPE_CANVAS
	};

	class Control
	{
	public:
		int id;
		CONTROL_TYPE type;

		char* name;

		Rect bounds;
		GC gc;
		bool separate_gc = 0;

		Window* parent;
		Control* next;
		Control* previous;

		int child_count = 0;
		Control* first_child = 0;
		Control* last_child = 0;

		uint32 background = 0;
		uint32 foreground = 0;

		virtual void Paint() {};
		virtual void ReceiveSendMessage(WND_MSG msg) {}

		void Close();

	private:
		void SendMessage();
	};
}