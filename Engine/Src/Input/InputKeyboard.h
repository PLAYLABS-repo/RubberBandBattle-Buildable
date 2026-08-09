#include "Engine/dependencies/include.h"

#pragma once
namespace Absolut{
  bool KeyDown(int key){
    return GetAsyncKeyState(key) & 0x8000;
}
  bool KeyPressed(int key) {
        static bool previousState[256] = {};

        bool currentState = KeyDown(key);
        bool pressed = currentState && !previousState[key];

        previousState[key] = currentState;

        return pressed;
    }

}


