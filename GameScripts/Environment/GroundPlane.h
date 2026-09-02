#pragma once

#include "Engine/dependencies/engineincludes.h"

namespace RubberBandBattle
{
    void GroundInit(int width, int depth);
    void UpdateGroundCollider();
    bool GroundCollision(const Absolut::Collision& collision);
    float GroundTop();
    void GroundDraw();
}
