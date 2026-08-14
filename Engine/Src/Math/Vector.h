/************************************
 filename:Vec2.h

 creation time: 8/07/2026 7:19 pm

***********************************/

#pragma once

namespace Absolut {

class Vec2 {

public:
    float x;
    float y;

    Vec2(float x , float y){

    this->x = x;
    this->y = y;

    }


};
class Vec3
{
public:

    float x;
    float y;
    float z;

    Vec3()
        : x(0.0f), y(0.0f), z(0.0f)
    {
    }

    Vec3(float x, float y, float z)
        : x(x), y(y), z(z)
    {
    }
};

}
