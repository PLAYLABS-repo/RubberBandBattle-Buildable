#pragma once

#include "Engine/dependencies/include.h"
#include "../../RenderHandling/GroundPlane.h"

namespace RubberBandBattle
{

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

    // Coyote time
    float coyoteTimer;
    float coyoteTime;

    int Health;
    int MaxHealth;

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
          jumpTimer(0.0f),
          coyoteTimer(0.12f),
          coyoteTime(0.12f),
          Health(100),
          MaxHealth(100)
    {
        UpdateCollision();
    }

    bool IsDead() const
    {
        return Health <= 0;
    }

    void TakeDamage(int damage)
    {
        if (IsDead())
            return;

        Health -= damage;

        if (Health < 0)
            Health = 0;

        if (IsDead())
            OnDeath();
    }

    void Heal(int amount)
    {
        if (IsDead())
            return;

        Health += amount;

        if (Health > MaxHealth)
            Health = MaxHealth;
    }

    void SetHealth(int health)
    {
        Health = health;

        if (Health < 0)
            Health = 0;

        if (Health > MaxHealth)
            Health = MaxHealth;
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

    void OnDeath()
    {
        isJumping = false;
        verticalVelocity = 0.0f;

        Absolut::Player.SetAnimation(3, false);
        lastAnimation = 3;
    }

    void UpdatePlayer(float dt)
    {
        if (IsDead())
            return;

        float speed = 5.0f;
        float gravity = 20.0f;
        float jumpForce = 8.0f;

        bool moving =
            Absolut::KeyDown('W') ||
            Absolut::KeyDown('S') ||
            Absolut::KeyDown('A') ||
            Absolut::KeyDown('D');

        // -----------------------------------------------------
        // Movement
        // -----------------------------------------------------

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

        // -----------------------------------------------------
        // Update collider
        // -----------------------------------------------------

        UpdateCollision();

        // -----------------------------------------------------
        // Ground collision
        // -----------------------------------------------------

        if (GroundCollision(collision) &&
            verticalVelocity <= 0.0f)
        {
            position.y = GroundTop();

            verticalVelocity = 0.0f;
            isGrounded = true;

            coyoteTimer = coyoteTime;

            UpdateCollision();
        }
        else
        {
            isGrounded = false;
        }

        // -----------------------------------------------------
        // Coyote time
        // -----------------------------------------------------

        if (isGrounded)
        {
            coyoteTimer = coyoteTime;
        }
        else
        {
            coyoteTimer -= dt;

            if (coyoteTimer < 0.0f)
                coyoteTimer = 0.0f;
        }

        // -----------------------------------------------------
        // Jump
        // -----------------------------------------------------

        if (Absolut::KeyPressed(VK_SPACE) &&
            coyoteTimer > 0.0f)
        {
            verticalVelocity = jumpForce;

            isGrounded = false;

            // Consume coyote time.
            coyoteTimer = 0.0f;

            isJumping = true;
            jumpTimer = 0.0f;

            Absolut::Player.SetAnimation(1, false);
            lastAnimation = 1;
        }

        // -----------------------------------------------------
        // Gravity
        // -----------------------------------------------------

        if (!isGrounded)
        {
            verticalVelocity -= gravity * dt;

            position.y += verticalVelocity * dt;
        }

        // -----------------------------------------------------
        // Update collider after movement
        // -----------------------------------------------------

        UpdateCollision();

        // -----------------------------------------------------
        // Resolve ground collision
        // -----------------------------------------------------

        if (GroundCollision(collision) &&
            verticalVelocity <= 0.0f)
        {
            position.y = GroundTop();

            verticalVelocity = 0.0f;
            isGrounded = true;

            coyoteTimer = coyoteTime;

            UpdateCollision();
        }
        else
        {
            isGrounded = false;
        }

        // -----------------------------------------------------
        // Jump animation
        // -----------------------------------------------------

        if (isJumping)
        {
            jumpTimer += dt;

            if (jumpTimer >= 0.5f)
            {
                isJumping = false;
            }
        }

        // -----------------------------------------------------
        // Idle / walking animation
        // -----------------------------------------------------

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

#ifdef _DEBUG

    std::string PlayerGetAnimationName()
    {
        return Absolut::Player.GetAnimationName(lastAnimation);
    }

#endif

};

}
