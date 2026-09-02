#pragma once
#include <windows.h>
namespace Absolut
{
class Mouse
{
public:
    static Mouse& Get()
    {
        static Mouse instance;
        return instance;
    }
    // ------------------------------------------------------
    // Window-message-driven state (unchanged) - buttons and
    // raw cursor position as reported by WM_MOUSEMOVE / etc.
    // Still useful for UI/menu-style absolute-position input.
    // ------------------------------------------------------
    void UpdateMessage(UINT msg, WPARAM wParam, LPARAM lParam);
    int x = 0;
    int y = 0;
    float gameX = 0.0f;
    float gameY = 0.0f;
    bool isLDown = false;
    bool isRDown = false;
    bool isMDown = false;
    // ------------------------------------------------------
    // FPS LOOK (delta-based)
    //
    // Call BindWindow() once after your window/HWND exists.
    // Call EnableFPSMode(true) to hide the cursor and start
    // tracking deltas; false restores the normal cursor.
    //
    // Call Update() once per frame (after processing input,
    // before/instead of relying on x/y above for camera look).
    // deltaX/deltaY are how far the mouse moved THIS frame -
    // feed these into yaw/pitch instead of absolute position.
    // ------------------------------------------------------
    void BindWindow(HWND hwnd)
    {
        window = hwnd;
    }
    void EnableFPSMode(bool enable)
    {
        fpsMode = enable;
        ShowCursor(enable ? FALSE : TRUE);
        if (enable && window)
        {
            RecenterCursor();
            hasLastPos = false; // avoid a big delta jump on the first frame
        }
    }
    void Update()
    {
        deltaX = 0.0f;
        deltaY = 0.0f;
        if (!fpsMode || !window)
            return;
        POINT cursor;
        GetCursorPos(&cursor);
        RECT rect;
        GetClientRect(window, &rect);
        POINT center;
        center.x = rect.right / 2;
        center.y = rect.bottom / 2;
        ClientToScreen(window, &center);
        if (hasLastPos)
        {
            deltaX = (float)(cursor.x - center.x);
            deltaY = (float)(cursor.y - center.y);
        }
        hasLastPos = true;
        RecenterCursor();
    }
    float deltaX = 0.0f;
    float deltaY = 0.0f;
private:
    Mouse() = default;
    void RecenterCursor()
    {
        RECT rect;
        GetClientRect(window, &rect);
        POINT center;
        center.x = rect.right / 2;
        center.y = rect.bottom / 2;
        ClientToScreen(window, &center);
        SetCursorPos(center.x, center.y);
    }
    HWND window = nullptr;
    bool fpsMode = false;
    bool hasLastPos = false;
};
}
