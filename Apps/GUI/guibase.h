#pragma once
#include <AndyOS.h>
#include <sys/drawing.h>
#include "GUI/messages.h"
#include "input.h"

namespace gui
{
    class GUIBase
    {
    public:
        GUIBase* parent;
        List<GUIBase*> elements;

        GC gc;
        Rect bounds;

        bool isHovering;

        GUIBase();
        void AddChild(GUIBase* child);

        virtual void Paint() { }

    //protected:
        virtual void Focus() { };

        virtual void KeyDown(KEY_PACKET packet) { };
        virtual void KeyUp(KEY_PACKET packet) { };
        virtual void KeyPress(KEY_PACKET packet) { };
        
        virtual void MouseDown() { };
        virtual void MouseUp() { };

        virtual void MouseEnter() { }
        virtual void MouseLeft() { }
    };
}