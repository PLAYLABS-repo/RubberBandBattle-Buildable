#include "Engine/dependencies/include.h"
#pragma once
namespace Absolut{


 class Quad {
public:
    float r = -2.0f;
    float g = -2.0f;
    float b = -2.0f;
    float x = 0.0f;
    float y = 0.0f;
    float w = 0.0f;
    float h = 0.0f;
    float u0 = 0.0f, v0 = 0.0f;
    float u1 = 1.0f, v1 = 1.0f;
    float SkewX = 0.0f;
    float SkewY = 0.0f;
    float Rotation = 0.0f;
    float PivotX;
    float PivotY;
    float BitmapOffsetX = 0.0f;
    float BitmapOffsetY = 0.0f;
    GLuint texture = 0;


    void draw();

};



}
