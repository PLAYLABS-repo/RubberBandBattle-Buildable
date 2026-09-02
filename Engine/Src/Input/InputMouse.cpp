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

            // This was the actual bug: gameX/gameY were never
            // assigned anywhere, so they stayed at 0.0f forever.
            // Interface::Element::Update() hit-tests against
            // gameX/gameY, so nothing could ever register as
            // hovered/clicked except an element sitting at (0,0).
            gameX = (float)x;
            gameY = (float)y;
            break;

        case WM_LBUTTONDOWN:
            // Also update position here, in case a click message
            // ever arrives without a preceding WM_MOUSEMOVE.
            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);
            gameX = (float)x;
            gameY = (float)y;
            isLDown = true;
            break;

        case WM_LBUTTONUP:
            isLDown = false;
            break;

        case WM_RBUTTONDOWN: isRDown = true; break;
        case WM_RBUTTONUP:   isRDown = false; break;
        case WM_MBUTTONDOWN: isMDown = true; break;
        case WM_MBUTTONUP:   isMDown = false; break;
    }
}
}
