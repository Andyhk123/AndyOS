#pragma once
#include <string>
#include "element.h"

namespace gui
{
	class Button : public Element
	{
	public:
		std::string text;

		Button(const std::string& text);

		void (*OnClick)() = 0;

		virtual void Paint();
		virtual void MouseDown();
		virtual void MouseUp();
	};
}