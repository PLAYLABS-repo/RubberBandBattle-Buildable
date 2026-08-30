#pragma once

#include "Engine/dependencies/include.h"
#include <chrono>

class Player
{
public:

    Absolut::Vec3 position;

    Absolut::Collision collision;

    float verticalVelocity;
    bool isGrounded;

    Player()
        : position(0.0f, 0.0f, 0.0f),
          collision(
              Absolut::Vec3(-0.5f, 0.0f, -0.5f),
              Absolut::Vec3(0.5f, 2.0f, 0.5f)
          ),
          verticalVelocity(0.0f),
          isGrounded(true)
    {
    }

    void UpdateCollision()
    {
        collision.SetPosition(
            Absolut::Vec3(
                position.x,
                position.y + 1.0f,
                position.z
            )
        );
    }
 int lastAnimation = -1;
 bool isJumping = false;
 float jumpTimer = 0.0f;

void UpdatePlayer(float dt)
{
    float speed = 5.0f;
    float gravity = 20.0f;
    float jumpForce = 8.0f;

    bool moving =
        Absolut::KeyDown('W') ||
        Absolut::KeyDown('S') ||
        Absolut::KeyDown('A') ||
        Absolut::KeyDown('D');

    // Movement
    if (Absolut::KeyDown('W')) {
        position.z -= speed * dt;
         Absolut::myModel.rotation.y = 180.0f;
    }

    if (Absolut::KeyDown('S')) {
        position.z += speed * dt;
         Absolut::myModel.rotation.y = 0.0f;

    }

    if (Absolut::KeyDown('A')) {
        position.x -= speed * dt;
         Absolut::myModel.rotation.y = -90.0f;
    }

    if (Absolut::KeyDown('D')) {
        position.x += speed * dt;
         Absolut::myModel.rotation.y = 90.0f;
    }

    // Jump
    if (Absolut::KeyPressed(VK_SPACE) && isGrounded) {
        verticalVelocity = jumpForce;
        isGrounded = false;

        isJumping = true;
        jumpTimer = 0.0f;

        Absolut::myModel.SetAnimation(1, false);
        lastAnimation = 1;
    }

    // Gravity
    if (!isGrounded) {
        verticalVelocity -= gravity * dt;
        position.y += verticalVelocity * dt;
    }

    // Ground
    if (position.y <= 0.0f) {
        position.y = 0.0f;
        verticalVelocity = 0.0f;
        isGrounded = true;
    }

    // Jump animation timer
    if (isJumping) {
        jumpTimer += dt;

        if (jumpTimer >= 0.5f) {
            isJumping = false;
        }
    }

    // Idle / walking animation
    if (!isJumping) {
        int animation = moving ? 4 : 0;

        if (animation != lastAnimation) {
            Absolut::myModel.SetAnimation(animation, true);
            lastAnimation = animation;
        }
    }

    UpdateCollision();
}
};
