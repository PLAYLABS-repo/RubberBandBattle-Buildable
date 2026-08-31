#pragma once

#include "Engine/dependencies/include.h"
#include "Geom/Mesh.h"
namespace RubberBandBattle{
Absolut::Mesh ground;
Absolut::Collision groundCollider;

void GroundInit(int AmountReachableW, int AmountReachableH)
{
    ground = Absolut::Mesh::CreateCube(1.0f);

    ground.position = Absolut::Vec3(
        0.0f,
        -3.0f,
        0.0f
    );

    ground.scale = Absolut::Vec3(
        (float)AmountReachableW,
        1.0f,
        (float)AmountReachableH
    );

    ground.r = 0.0f;
    ground.g = 1.0f;
    ground.b = 0.0f;

    // Create collider with the same size as the ground.
    groundCollider = Absolut::Collision::FromCenter(
        ground.position,
        Absolut::Vec3(
            AmountReachableW * 0.5f,
            0.5f,
            AmountReachableH * 0.5f
        )
    );
}

void UpdateGroundCollider()
{
    groundCollider.SetPosition(
        ground.position
    );
}

bool GroundCollision(const Absolut::Collision& playerCollider)
{
    return playerCollider.Intersects(
        groundCollider
    );
}

float GroundTop()
{
    return groundCollider.max.y;
}

void GroundDraw()
{
    ground.draw();
}
}
