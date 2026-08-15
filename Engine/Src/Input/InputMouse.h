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

    void UpdateMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    int x = 0;
    int y = 0;

    float gameX = 0.0f;
    float gameY = 0.0f;

    bool isLDown = false;
    bool isRDown = false;
    bool isMDown = false;

private:
    Mouse() = default;
};

}
