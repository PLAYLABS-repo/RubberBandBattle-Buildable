#pragma once

#include "Engine/dependencies/include.h"
#include "GameScripts/RenderHandling/GroundPlane.h"
#include <chrono>
namespace RubberBandBattle{
class Player
{
public:

    Absolut::Vec3 position;

    Absolut::Collision collision;

    float verticalVelocity;
    bool isGrounded;

    int lastAnimation;
    bool isJumping;
    float jumpTimer;

    Player()
        : position(0.0f, -2.5f, 0.0f),
          collision(
              Absolut::Vec3(-0.5f, 0.0f, -0.5f),
              Absolut::Vec3(0.5f, 2.0f, 0.5f)
          ),
          verticalVelocity(0.0f),
          isGrounded(true),
          lastAnimation(-1),
          isJumping(false),
          jumpTimer(0.0f)
    {
        UpdateCollision();
    }

    // ============================================================
    // UPDATE COLLISION BOX
    // ============================================================

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

        if (Absolut::KeyDown('W'))
        {
            position.z -= speed * dt;
            Absolut::Player.rotation.y = 180.0f;
        }

        if (Absolut::KeyDown('S'))
        {
            position.z += speed * dt;
            Absolut::Player.rotation.y = 0.0f;
        }

        if (Absolut::KeyDown('A'))
        {
            position.x -= speed * dt;
            Absolut::Player.rotation.y = -90.0f;
        }

        if (Absolut::KeyDown('D'))
        {
            position.x += speed * dt;
            Absolut::Player.rotation.y = 90.0f;
        }


        if (Absolut::KeyPressed(VK_SPACE) && isGrounded)
        {
            verticalVelocity = jumpForce;
            isGrounded = false;

            isJumping = true;
            jumpTimer = 0.0f;

            Absolut::Player.SetAnimation(1, false);
            lastAnimation = 1;
        }

        if (!isGrounded)
        {
            verticalVelocity -= gravity * dt;
            position.y += verticalVelocity * dt;
        }

        UpdateCollision();

        if (GroundCollision(collision))
        {
            // Only resolve downward movement.
            if (verticalVelocity <= 0.0f)
            {
                position.y = GroundTop();

                verticalVelocity = 0.0f;
                isGrounded = true;

                UpdateCollision();
            }
        }
        else
        {
            isGrounded = false;
        }


        if (isJumping)
        {
            jumpTimer += dt;

            if (jumpTimer >= 0.5f)
            {
                isJumping = false;
            }
        }


        if (!isJumping)
        {
            int animation = moving ? 4 : 0;

            if (animation != lastAnimation)
            {
                Absolut::Player.SetAnimation(
                    animation,
                    true
                );

                lastAnimation = animation;
            }
        }
    }
};

}
