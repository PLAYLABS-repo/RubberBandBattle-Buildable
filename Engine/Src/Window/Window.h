#pragma once

namespace Absolut
{

class Window
{
    HWND hwnd;
    HDC dc;
    HGLRC gl;

    static LRESULT CALLBACK Proc(HWND h, UINT m, WPARAM w, LPARAM l)
    {
        if (m == WM_DESTROY)
            PostQuitMessage(0);

        return DefWindowProc(h, m, w, l);
    }

public:

    Window() : hwnd(0), dc(0), gl(0) {}

    bool create(const char* title, int w, int h)
    {
        WNDCLASS wc = {};
        wc.lpfnWndProc = Proc;
        wc.hInstance = GetModuleHandle(0);
        wc.lpszClassName = "Window";
        wc.style = CS_OWNDC;

        RegisterClass(&wc);

        hwnd = CreateWindow(
            "Window",
            title,
            WS_OVERLAPPEDWINDOW,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            w,
            h,
            0,
            0,
            wc.hInstance,
            0
        );

        dc = GetDC(hwnd);

        PIXELFORMATDESCRIPTOR pfd = {};
        pfd.nSize = sizeof(pfd);
        pfd.nVersion = 1;
        pfd.dwFlags =
            PFD_DRAW_TO_WINDOW |
            PFD_SUPPORT_OPENGL |
            PFD_DOUBLEBUFFER;

        pfd.iPixelType = PFD_TYPE_RGBA;
        pfd.cColorBits = 32;
        pfd.cDepthBits = 24;

        int format = ChoosePixelFormat(dc, &pfd);
        SetPixelFormat(dc, format, &pfd);

        gl = wglCreateContext(dc);
        wglMakeCurrent(dc, gl);

        ShowWindow(hwnd, SW_SHOW);

        return true;
    }

    bool update()
    {
        MSG msg;

        while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
                return false;

            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }

        return true;
    }

    void swap()
    {
        SwapBuffers(dc);
    }
};

}

