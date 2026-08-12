#pragma once

#include <windows.h>

namespace Absolut
{

struct MouseState
{
    int x = 0;
    int y = 0;

    float gameX = 0.0f;
    float gameY = 0.0f;

    bool isLDown = false;
    bool isRDown = false;
    bool isMDown = false;
};

extern MouseState Mouse;

void UpdateMouseMessage(UINT msg, WPARAM wParam, LPARAM lParam);

}
