#pragma once
#include <keycodes.h>

namespace Input {
void Init();

void GetMouseButtons(bool &left, bool &right, bool &middle);
void GetMouseMovement(int &x, int &y);
bool GetKeyboardInput(KEYCODE &code, bool &pressed);
} // namespace Input
