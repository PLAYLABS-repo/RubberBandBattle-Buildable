#pragma once

#include "Engine/dependencies/include.h"
#include "Engine/Src/Input/InputMouse.h"

#include <GLES2/gl2.h>
#include <EGL/egl.h>

namespace Absolut
{

class Window
{
private:

    HWND hwnd;
    HDC dc;

    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
    EGLConfig config;

    bool fullscreen;

    DWORD windowStyle;
    RECT windowRect;

    static Window* instance;

    static LRESULT CALLBACK Proc(
        HWND h,
        UINT m,
        WPARAM w,
        LPARAM l)
    {
        switch (m)
        {
            case WM_DESTROY:
            {
                PostQuitMessage(0);
                return 0;
            }

            case WM_SIZE:
            {
                if (instance &&
                    instance->display != EGL_NO_DISPLAY &&
                    instance->context != EGL_NO_CONTEXT)
                {
                    int width = LOWORD(l);
                    int height = HIWORD(l);

                    if (width < 1)
                        width = 1;

                    if (height < 1)
                        height = 1;

                    glViewport(
                        0,
                        0,
                        width,
                        height
                    );
                }

                return 0;
            }

            case WM_KEYDOWN:
            {
                if (w == VK_F11 && instance)
                {
                    instance->toggleFullscreen();
                    return 0;
                }

                break;
            }

            // --------------------------------------------------
            // MOUSE
            //
            // Previously these messages were never forwarded
            // anywhere, so Mouse::isLDown / x / y / gameX / gameY
            // never changed - nothing could ever register as
            // hovered/pressed/clicked.
            // --------------------------------------------------

            case WM_MOUSEMOVE:
            case WM_LBUTTONDOWN:
            case WM_LBUTTONUP:
            case WM_RBUTTONDOWN:
            case WM_RBUTTONUP:
            case WM_MBUTTONDOWN:
            case WM_MBUTTONUP:
            {
                Mouse::Get().UpdateMessage(m, w, l);
                break;
            }
        }

        return DefWindowProc(
            h,
            m,
            w,
            l
        );
    }

public:

    Window()
        : hwnd(0),
          dc(0),
          display(EGL_NO_DISPLAY),
          surface(EGL_NO_SURFACE),
          context(EGL_NO_CONTEXT),
          config(0),
          fullscreen(false),
          windowStyle(0)
    {
        windowRect = {};
    }

    ~Window()
    {
        destroy();
    }

    bool create(
        const char* title,
        int w,
        int h)
    {
        WNDCLASS wc = {};

        wc.lpfnWndProc = Proc;
        wc.hInstance = GetModuleHandle(0);
        wc.lpszClassName = "AbsolutWindow";
        wc.style = CS_OWNDC;

        if (!RegisterClass(&wc))
        {
            if (GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
                return false;
        }

        hwnd = CreateWindow(
            "AbsolutWindow",
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

        if (!hwnd)
            return false;

        instance = this;

        dc = GetDC(hwnd);

        if (!dc)
        {
            destroy();
            return false;
        }

        /*
            IMPORTANT:

            For Windows EGL, use the native DISPLAY/device
            expected by the EGL implementation.

            Most Mesa/ANGLE implementations accept the
            device context here, but some implementations
            use EGL_DEFAULT_DISPLAY.
        */

       display = eglGetDisplay(
    EGL_DEFAULT_DISPLAY

);
        if (display == EGL_NO_DISPLAY)
        {
            destroy();
            return false;
        }

        EGLint major = 0;
        EGLint minor = 0;

        if (!eglInitialize(
                display,
                &major,
                &minor))
        {
            destroy();
            return false;
        }

        /*
            Tell EGL that we want an OpenGL ES API.
        */

        if (!eglBindAPI(EGL_OPENGL_ES_API))
        {
            destroy();
            return false;
        }

        /*
            Normal 8-bit RGBA framebuffer.

            Do NOT request EGL_SRGB_COLORSPACE here.

            This gives us ordinary framebuffer RGB values,
            which is what a normal 2D engine expects.
        */

        EGLint configAttributes[] =
        {
            EGL_SURFACE_TYPE,
            EGL_WINDOW_BIT,

            EGL_RENDERABLE_TYPE,
            EGL_OPENGL_ES2_BIT,

            EGL_RED_SIZE,
            8,

            EGL_GREEN_SIZE,
            8,

            EGL_BLUE_SIZE,
            8,

            EGL_ALPHA_SIZE,
            8,

            EGL_DEPTH_SIZE,
            24,

            EGL_STENCIL_SIZE,
            8,

            EGL_COLOR_BUFFER_TYPE,
            EGL_RGB_BUFFER,

            EGL_NONE
        };

        EGLint numConfigs = 0;

        if (!eglChooseConfig(
                display,
                configAttributes,
                &config,
                1,
                &numConfigs))
        {
            destroy();
            return false;
        }

        if (numConfigs <= 0 || config == 0)
        {
            destroy();
            return false;
        }

        /*
            Normal window surface.

            No EGL_GL_COLORSPACE_KHR /
            EGL_GL_COLORSPACE_SRGB_KHR attribute.

            This is intentional.
        */

        surface = eglCreateWindowSurface(
            display,
            config,
            (EGLNativeWindowType)hwnd,
            0
        );

        if (surface == EGL_NO_SURFACE)
        {
            destroy();
            return false;
        }

        /*
            GLES 2.0 context.
        */

        EGLint contextAttributes[] =
        {
            EGL_CONTEXT_CLIENT_VERSION,
            2,

            EGL_NONE
        };

        context = eglCreateContext(
            display,
            config,
            EGL_NO_CONTEXT,
            contextAttributes
        );

        if (context == EGL_NO_CONTEXT)
        {
            destroy();
            return false;
        }

        if (!eglMakeCurrent(
                display,
                surface,
                surface,
                context))
        {
            destroy();
            return false;
        }

        /*
            Important for normal 8-bit texture uploads.

            stb_image normally gives us tightly packed RGB/RGBA
            data. GL_UNPACK_ALIGNMENT = 1 prevents RGB rows
            whose width isn't divisible by 4 from being
            interpreted incorrectly.
        */

        glPixelStorei(
            GL_UNPACK_ALIGNMENT,
            1
        );

        /*
            Disable anything that could unexpectedly modify
            the displayed RGB values.
        */

        glDisable(GL_DITHER);

        /*
            Normal alpha blending.
        */

        glEnable(GL_BLEND);

        glBlendFunc(
            GL_SRC_ALPHA,
            GL_ONE_MINUS_SRC_ALPHA
        );

        ShowWindow(
            hwnd,
            SW_SHOW
        );

        UpdateWindow(hwnd);

        /*
            Get actual client dimensions.
        */

        RECT client = {};

        GetClientRect(
            hwnd,
            &client
        );

        int width =
            client.right - client.left;

        int height =
            client.bottom - client.top;

        if (width < 1)
            width = 1;

        if (height < 1)
            height = 1;

        glViewport(
            0,
            0,
            width,
            height
        );
printf("EGL error after make current: 0x%04X\n", eglGetError());

GLint viewport[4] = {0, 0, 0, 0};

glGetIntegerv(
    GL_VIEWPORT,
    viewport
);

printf(
    "GL viewport: %d %d %d %d\n",
    viewport[0],
    viewport[1],
    viewport[2],
    viewport[3]
);

printf(
    "GL_RENDERER: %s\n",
    glGetString(GL_RENDERER)
);

printf(
    "GL_VERSION: %s\n",
    glGetString(GL_VERSION)
);
        return true;
    }

    void destroy()
    {
        if (display != EGL_NO_DISPLAY)
        {
            eglMakeCurrent(
                display,
                EGL_NO_SURFACE,
                EGL_NO_SURFACE,
                EGL_NO_CONTEXT
            );

            if (context != EGL_NO_CONTEXT)
            {
                eglDestroyContext(
                    display,
                    context
                );

                context = EGL_NO_CONTEXT;
            }

            if (surface != EGL_NO_SURFACE)
            {
                eglDestroySurface(
                    display,
                    surface
                );

                surface = EGL_NO_SURFACE;
            }

            eglTerminate(display);

            display = EGL_NO_DISPLAY;
        }

        config = 0;

        if (dc && hwnd)
        {
            ReleaseDC(
                hwnd,
                dc
            );

            dc = 0;
        }

        if (hwnd)
        {
            DestroyWindow(hwnd);
            hwnd = 0;
        }

        if (instance == this)
            instance = 0;
    }

    bool update()
    {
        MSG msg;

        while (PeekMessage(
            &msg,
            0,
            0,
            0,
            PM_REMOVE))
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
        if (display != EGL_NO_DISPLAY &&
            surface != EGL_NO_SURFACE)
        {
            eglSwapBuffers(
                display,
                surface
            );
        }
    }

    void setVSync(bool enabled)
    {
        if (display == EGL_NO_DISPLAY)
            return;

        eglSwapInterval(
            display,
            enabled ? 1 : 0
        );
    }

    void toggleFullscreen()
    {
        if (!hwnd)
            return;

        if (!fullscreen)
        {
            windowStyle =
                GetWindowLong(
                    hwnd,
                    GWL_STYLE
                );

            GetWindowRect(
                hwnd,
                &windowRect
            );

            SetWindowLong(
                hwnd,
                GWL_STYLE,
                windowStyle &
                ~(WS_CAPTION |
                  WS_THICKFRAME |
                  WS_MINIMIZE |
                  WS_MAXIMIZE |
                  WS_SYSMENU)
            );

            MONITORINFO mi = {};
            mi.cbSize = sizeof(mi);

            GetMonitorInfo(
                MonitorFromWindow(
                    hwnd,
                    MONITOR_DEFAULTTONEAREST
                ),
                &mi
            );

            SetWindowPos(
                hwnd,
                HWND_TOP,
                mi.rcMonitor.left,
                mi.rcMonitor.top,
                mi.rcMonitor.right -
                    mi.rcMonitor.left,
                mi.rcMonitor.bottom -
                    mi.rcMonitor.top,
                SWP_NOOWNERZORDER |
                SWP_FRAMECHANGED
            );

            fullscreen = true;
        }
        else
        {
            SetWindowLong(
                hwnd,
                GWL_STYLE,
                windowStyle
            );

            SetWindowPos(
                hwnd,
                HWND_TOP,
                windowRect.left,
                windowRect.top,
                windowRect.right -
                    windowRect.left,
                windowRect.bottom -
                    windowRect.top,
                SWP_NOOWNERZORDER |
                SWP_FRAMECHANGED
            );

            fullscreen = false;
        }

        RECT client = {};

        GetClientRect(
            hwnd,
            &client
        );

        int width =
            client.right - client.left;

        int height =
            client.bottom - client.top;

        if (width < 1)
            width = 1;

        if (height < 1)
            height = 1;

        glViewport(
            0,
            0,
            width,
            height
        );
    }

    HWND getHandle() const
    {
        return hwnd;
    }

    int getWidth() const
    {
        if (!hwnd)
            return 0;

        RECT r;

        GetClientRect(
            hwnd,
            &r
        );

        return r.right - r.left;
    }

    int getHeight() const
    {
        if (!hwnd)
            return 0;

        RECT r;

        GetClientRect(
            hwnd,
            &r
        );

        return r.bottom - r.top;
    }

};

}
