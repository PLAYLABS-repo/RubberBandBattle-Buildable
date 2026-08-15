#include "InputMouse.h"
#include <windowsx.h>

namespace Absolut
{

void Mouse::UpdateMessage(UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_MOUSEMOVE:
            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);
            break;

        case WM_LBUTTONDOWN: isLDown = true; break;
        case WM_LBUTTONUP:   isLDown = false; break;

        case WM_RBUTTONDOWN: isRDown = true; break;
        case WM_RBUTTONUP:   isRDown = false; break;

        case WM_MBUTTONDOWN: isMDown = true; break;
        case WM_MBUTTONUP:   isMDown = false; break;
    }
}

}
